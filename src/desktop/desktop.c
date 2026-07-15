#include "gui/desktop.h"
#include "gui/desktop_runtime.h"
#include "gui/compositor.h"
#include "gui/taskbar.h"
#include "gui/terminal.h"
#include "gui/font.h"
#include "gui/event.h"
#include "gui/widget.h"
#include "shell/core.h"
#include "drivers/input/mouse.h"
#include "drivers/input/keyboard_layout.h"
#include "drivers/rtc/rtc.h"
#include "util/kstring.h"
#include "memory/kmem.h"
#include "apps/calculator.h"
#include "apps/file_manager.h"
#include "apps/text_editor.h"
#include "apps/settings.h"
#include "apps/task_manager.h"
#include "gui/context_menu.h"
#include "gui/desktop_icons.h"
#include "gui/inline_prompt.h"
#include "auth/session.h"
#include "lang/app_language.h"
#include "net/stack.h"
#include "arch/x86_64/apic.h"
#include <stddef.h>

static struct terminal g_desktop_terminal;
static struct terminal *g_shell_output_term = NULL;
static struct desktop_session *g_menu_desktop = NULL;
static int g_terminal_open = 0;

static void desktop_shell_write(const char *text) {
  if (g_shell_output_term && text) terminal_write_string(g_shell_output_term, text);
}

static void desktop_shell_putc(char ch) {
  if (g_shell_output_term) terminal_write_char(g_shell_output_term, ch);
}

/* Post-M5 W1: clear hook paired with desktop_shell_write/putc.
 * Routes the shell's mess builtin to the active terminal widget
 * instead of the framebuffer console hidden behind the GUI. */
static void desktop_shell_clear(void) {
  if (g_shell_output_term) {
    terminal_clear(g_shell_output_term);
  }
}

static void desktop_terminal_prompt(struct terminal *term) {
  char prompt[128];
  struct session_context *sess = kernel_desktop_shell_session();
  const struct user_record *user = sess ? session_user(sess) : NULL;
  const char *cwd = sess ? session_cwd(sess) : "/";
  if (!term) return;
  shell_build_prompt(user, g_menu_desktop ? g_menu_desktop->settings : NULL,
                     cwd, prompt, sizeof(prompt));
  terminal_write_string(term, prompt[0] ? prompt : "$ ");
}

static void desktop_shell_begin_terminal_output(struct terminal *term) {
  g_shell_output_term = term;
}

static void desktop_shell_end_terminal_output(void) {
  g_shell_output_term = NULL;
}

static void desktop_terminal_paint(struct gui_window *win) {
  if (!win || !win->user_data) return;
  terminal_paint((struct terminal *)win->user_data);
}

/* 2026-05-02: repaint the terminal after a user resize drag. The
 * compositor has already cleared the new pixel buffer to bg_color;
 * the terminal painter walks the scrollback ring and re-renders
 * onto the surface so dropping the new dimensions in is enough. */
static void desktop_terminal_resize(struct gui_window *win,
                                    uint32_t w, uint32_t h) {
  (void)w;
  (void)h;
  if (!win || !win->user_data) return;
  terminal_paint((struct terminal *)win->user_data);
}

static void desktop_terminal_key(struct gui_window *win, uint32_t keycode,
                                 uint8_t mods) {
  (void)mods;
  if (!win || !win->user_data) return;
  struct terminal *term = (struct terminal *)win->user_data;

  /* Special keys scroll the view through the scrollback ring so long
   * outputs (help-any, logs) stay fully readable: PgUp/PgDn by a page,
   * Up/Down by a line. Typing any printable snaps back to the live
   * screen (terminal_handle_key resets scroll_offset). */
  if (keycode >= 0x80) {
    uint32_t page = term->rows > 1u ? term->rows - 1u : 1u;
    switch (keycode) {
      case KEY_PGUP: terminal_scroll_up(term, page); break;
      case KEY_PGDN: terminal_scroll_down(term, page); break;
      case KEY_UP:   terminal_scroll_up(term, 1u); break;
      case KEY_DOWN: terminal_scroll_down(term, 1u); break;
      default: return;
    }
    compositor_invalidate(win->id);
    return;
  }

  terminal_handle_key(term, keycode, (char)keycode);
}

static void desktop_terminal_scroll(struct gui_window *win, int32_t delta) {
  if (!win || !win->user_data) return;
  terminal_handle_mouse_scroll((struct terminal *)win->user_data, (int)delta);
  compositor_invalidate(win->id);
}

static void desktop_terminal_on_close(struct gui_window *win) {
  (void)win;
  if (g_menu_desktop && g_menu_desktop->active_terminal == &g_desktop_terminal) {
    g_menu_desktop->active_terminal = NULL;
  }
  terminal_set_output_callback(&g_desktop_terminal, NULL, NULL);
  g_desktop_terminal.window = NULL;
  if (g_shell_output_term == &g_desktop_terminal) desktop_shell_end_terminal_output();
  g_terminal_open = 0;
}

static int desktop_update_tray(struct desktop_session *ds) {
  char tray[TASKBAR_TRAY_TEXT_MAX];
  struct net_stack_status ns;
  if (!ds) return 0;
  if (net_stack_status(&ns) == 0) {
    kstrcpy(tray, sizeof(tray), ns.ready ? "net-on" :
            (ns.runtime_supported ? "net-wait" : "net-off"));
  } else {
    kstrcpy(tray, sizeof(tray), "net-off");
  }
  return taskbar_update_tray(&ds->taskbar, tray);
}

static void desktop_sample_dispatcher_health(struct desktop_session *ds) {
  if (!ds) return;
  if (gui_window_dispatcher_health_snapshot(&ds->dispatcher_health)) {
    ds->dispatcher_health_samples++;
  }
}

int desktop_session_health_snapshot(const struct desktop_session *ds,
                                    struct desktop_session_health *out) {
  struct gui_window *focused = NULL;
  if (!ds || !out) return 0;
  focused = compositor_focused_window();
  out->active = ds->active ? 1 : 0;
  out->framebuffer_ready = ds->framebuffer ? 1 : 0;
  out->dimensions_ready = (ds->screen_w > 0u && ds->screen_h > 0u &&
                           ds->pitch > 0u) ? 1 : 0;
  out->mouse_initialized = ds->mouse_initialized ? 1 : 0;
  out->cursor_valid = ds->cursor_valid ? 1 : 0;
  out->taskbar_ready = ds->taskbar.window ? 1 : 0;
  out->overlay_active = inline_prompt_is_open() || context_menu_is_open() ||
                        ds->taskbar.menu_open;
  out->taskbar_menu_open = ds->taskbar.menu_open ? 1 : 0;
  out->window_manager_drag_active =
      (ds->wm.drag_mode != WM_DRAG_NONE) ? 1 : 0;
  out->focused_window_id = focused ? focused->id : 0;
  out->dispatcher_health_samples = ds->dispatcher_health_samples;
  out->dispatcher = ds->dispatcher_health;
  out->dispatcher_health_ready =
      gui_window_dispatcher_health_snapshot(&out->dispatcher) ? 1 : 0;
  return 1;
}

int desktop_session_smoke_readiness_snapshot(
    const struct desktop_session *ds,
    struct desktop_session_smoke_readiness *out) {
  struct desktop_session_health health;
  if (!out) return 0;
  if (!desktop_session_health_snapshot(ds, &health)) {
    return desktop_session_smoke_readiness_from_health(NULL, out);
  }
  return desktop_session_smoke_readiness_from_health(&health, out);
}

static int desktop_handle_overlay_escape(struct desktop_session *ds,
                                         uint32_t keycode, char ch) {
  int closed = 0;
  if (ch != 0x1B && keycode != 0x1B) return 0;
  if (context_menu_is_open()) {
    context_menu_close();
    closed = 1;
  }
  if (ds && ds->taskbar.menu_open) {
    taskbar_toggle_menu(&ds->taskbar);
    closed = 1;
  }
  if (closed) compositor_invalidate_all();
  return closed;
}

static void desktop_apply_theme(struct desktop_session *ds) {
  const char *theme = NULL;
  const struct gui_theme_palette *palette = NULL;
  if (!ds) return;
  theme = (ds->settings && ds->settings->theme[0]) ? ds->settings->theme : "capyos";
  compositor_apply_theme(theme, ds->screen_w, ds->screen_h);
  palette = compositor_theme();
  ds->wallpaper_color = palette->wallpaper;
  kstrcpy(ds->theme_name, sizeof(ds->theme_name), theme);
  compositor_set_wallpaper(palette->wallpaper);
  ds->taskbar.bg_color = palette->taskbar_bg;
  ds->taskbar.fg_color = palette->taskbar_fg;
  ds->taskbar.highlight_color = palette->taskbar_highlight;
  if (ds->taskbar.window) {
    ds->taskbar.window->bg_color = palette->taskbar_bg;
    ds->taskbar.window->border_color = palette->window_border;
  }
  if (ds->taskbar.menu_popup) {
    ds->taskbar.menu_popup->bg_color = palette->window_bg;
    ds->taskbar.menu_popup->border_color = palette->window_border;
  }
  if (ds->active_terminal) {
    terminal_set_color(ds->active_terminal, palette->terminal_fg, palette->terminal_bg);
    if (ds->active_terminal->window) {
      ds->active_terminal->window->bg_color = palette->terminal_bg;
      ds->active_terminal->window->border_color = palette->window_border;
    }
  }
}

static void menu_action_terminal(void *user_data) {
  (void)user_data;
  if (g_menu_desktop) desktop_open_terminal(g_menu_desktop);
}

/* Register only real app windows, never taskbar/menu overlays or stale focus. */
static void register_focused_in_taskbar(const char *expected_title,
                                        const char *name) {
  struct gui_window *win = compositor_focused_window();
  if (!win || !g_menu_desktop || !win->id || !win->visible || !win->decorated) return;
  if (expected_title && !kstreq(win->title, expected_title)) return;
  taskbar_add_window(&g_menu_desktop->taskbar, win->id, name);
  taskbar_set_focused(&g_menu_desktop->taskbar, win->id);
}

static void menu_action_file_manager(void *user_data) {
  (void)user_data;
  file_manager_open();
  register_focused_in_taskbar("File Manager", "Files");
}

static void menu_action_text_editor(void *user_data) {
  (void)user_data;
  text_editor_open(NULL);
  register_focused_in_taskbar("Text Editor", "Editor");
}

static void menu_action_calculator(void *user_data) {
  (void)user_data;
  calculator_open();
  register_focused_in_taskbar("Calculator", "Calculator");
}

static void menu_action_settings(void *user_data) {
  (void)user_data;
  settings_open();
  register_focused_in_taskbar("Settings", "Settings");
}

static void menu_action_task_manager(void *user_data) {
  (void)user_data;
  task_manager_open();
  register_focused_in_taskbar("Task Manager", "Tasks");
}

/* Etapa 7 / Slice 7.5: o navegador grafico volta ao menu iniciar como
 * adaptador versionado — a acao delega ao hook estavel do CapyOS
 * (kernel_desktop_open_browser_graphical, declarado em
 * gui/desktop_runtime.h), que faz o spawn ring-3 do capygfx quando o
 * blob esta embutido e retorna erro caso contrario. Falha nunca e
 * silenciosa: sem pipeline de notificacao integrado ainda, o feedback
 * sai no terminal (abrindo-o se preciso). */
static void menu_action_browser(void *user_data) {
  (void)user_data;
  if (kernel_desktop_open_browser_graphical() == 0) return;
  if (!g_menu_desktop) return;
  desktop_open_terminal(g_menu_desktop);
  if (g_menu_desktop->active_terminal) {
    terminal_write_string(
        g_menu_desktop->active_terminal,
        APP_T("\n[navegador] indisponivel neste kernel (blob/module ausente).\n",
              "\n[browser] unavailable in this kernel (blob/module missing).\n",
              "\n[navegador] no disponible en este kernel (blob/module ausente).\n"));
    desktop_terminal_prompt(g_menu_desktop->active_terminal);
  }
}

static void menu_action_capyai(void *user_data) {
  (void)user_data;
  (void)desktop_launch_capyai();
}

static void menu_action_logout(void *user_data) {
  (void)user_data;
  (void)desktop_request_logout();
}

static void menu_action_reboot(void *user_data) {
  (void)user_data;
  extern void desktop_stop(void);
  desktop_stop();
  extern void kernel_request_reboot(void);
  kernel_request_reboot();
}

static void menu_action_shutdown(void *user_data) {
  (void)user_data;
  extern void desktop_stop(void);
  desktop_stop();
  extern void kernel_request_shutdown(void);
  kernel_request_shutdown();
}

/* ------------------------------------------------------------------ *
 * Registro de apps do menu iniciar ("Lista completa").
 *
 * Fonte unica dos apps lancaveis do desktop: o modo resumido mostra os
 * favoritos fixados; "Lista completa" injeta TODOS os apps do registro
 * na categoria "Apps" (ordenados), inclusive apps entregues por modulo
 * (Navegador) — a disponibilidade e sondada no clique, entao uma nova
 * instalacao ja encontra o atalho pronto no menu, sem rebuild da UI.
 * Novos apps (in-kernel ou de modulo) entram adicionando UMA linha aqui.
 * ------------------------------------------------------------------ */
struct desktop_menu_app {
  const char *label;            /* localizado no momento do registro */
  void (*action)(void *);
  int pinned;                   /* favorito no modo resumido */
};

static int g_menu_show_all = 0;

static void desktop_menu_apps(struct desktop_menu_app *out, uint32_t cap,
                              uint32_t *count) {
  uint32_t n = 0;
  if (!out || cap < 8u) { if (count) *count = 0; return; }
  out[n].label = APP_T("Arquivos", "Files", "Archivos");
  out[n].action = menu_action_file_manager; out[n].pinned = 1; n++;
  out[n].label = APP_T("Calculadora", "Calculator", "Calculadora");
  out[n].action = menu_action_calculator; out[n].pinned = 1; n++;
  out[n].label = APP_T("Configuracoes", "Settings", "Ajustes");
  out[n].action = menu_action_settings; out[n].pinned = 0; n++;
  out[n].label = APP_T("Editor", "Editor", "Editor");
  out[n].action = menu_action_text_editor; out[n].pinned = 1; n++;
  out[n].label = APP_T("Navegador", "Browser", "Navegador");
  out[n].action = menu_action_browser; out[n].pinned = 1; n++;
  out[n].label = "CapyAI";
  out[n].action = menu_action_capyai; out[n].pinned = 1; n++;
  out[n].label = APP_T("Tarefas", "Tasks", "Tareas");
  out[n].action = menu_action_task_manager; out[n].pinned = 0; n++;
  out[n].label = APP_T("Terminal", "Terminal", "Terminal");
  out[n].action = menu_action_terminal; out[n].pinned = 1; n++;
  if (count) *count = n;
}

static void menu_action_toggle_all(void *user_data);

static void desktop_populate_menu(struct desktop_session *ds) {
  struct desktop_menu_app apps[8];
  uint32_t app_count = 0;
  uint32_t i;
  if (!ds) return;
  ds->taskbar.menu_entry_count = 0;
  desktop_menu_apps(apps, 8u, &app_count);

  /* Favoritos fixados (grupo "Pinned"). */
  for (i = 0; i < app_count; i++) {
    if (!apps[i].pinned) continue;
    taskbar_add_menu_entry_pinned(&ds->taskbar, apps[i].label,
                                  apps[i].action, ds);
  }
  /* Lista completa: todos os apps do registro, ja em ordem alfabetica
   * (grupo "Apps"), atalho pronto inclusive para apps de modulo. */
  if (g_menu_show_all) {
    for (i = 0; i < app_count; i++) {
      taskbar_add_menu_entry(&ds->taskbar, apps[i].label, apps[i].action, ds);
    }
  }
  taskbar_add_menu_separator(&ds->taskbar);
  taskbar_add_menu_entry(&ds->taskbar,
                         g_menu_show_all
                             ? APP_T("Lista resumida", "Compact list",
                                     "Lista resumida")
                             : APP_T("Lista completa", "All apps",
                                     "Lista completa"),
                         menu_action_toggle_all, ds);
  taskbar_add_menu_entry(&ds->taskbar,
                         APP_T("Configuracoes", "Settings", "Ajustes"),
                         menu_action_settings, ds);
  taskbar_add_menu_entry(&ds->taskbar,
                         APP_T("Tarefas", "Tasks", "Tareas"),
                         menu_action_task_manager, ds);
  taskbar_add_menu_separator(&ds->taskbar);
  taskbar_add_menu_entry(&ds->taskbar,
                         APP_T("Sair", "Logout", "Cerrar sesion"),
                         menu_action_logout, ds);
  taskbar_add_menu_entry(&ds->taskbar,
                         APP_T("Reiniciar", "Reboot", "Reiniciar"),
                         menu_action_reboot, ds);
  taskbar_add_menu_entry(&ds->taskbar,
                         APP_T("Desligar", "Shutdown", "Apagar"),
                         menu_action_shutdown, ds);
}

static void menu_action_toggle_all(void *user_data) {
  struct desktop_session *ds = (struct desktop_session *)user_data;
  g_menu_show_all = !g_menu_show_all;
  if (!ds) return;
  desktop_populate_menu(ds);
  /* A ativacao ja fechou o popup (taskbar_toggle_menu); reabre com o
   * novo conjunto de entradas para o toggle parecer in-place. */
  if (!ds->taskbar.menu_open) taskbar_toggle_menu(&ds->taskbar);
  compositor_invalidate_all();
}

void desktop_init(struct desktop_session *ds, uint32_t *fb, uint32_t w,
                  uint32_t h, uint32_t pitch,
                  const struct system_settings *settings) {
  if (!ds || !fb) return;
  kmemzero(ds, sizeof(*ds));
  ds->framebuffer = fb;
  ds->screen_w = w;
  ds->screen_h = h;
  ds->pitch = pitch;
  ds->settings = settings;
  ds->active = 1;

  font_init();
  gui_event_init();
  widget_system_init();
  compositor_init(fb, w, h, pitch);
  taskbar_init(&ds->taskbar, w, h);
  wm_init(&ds->wm, w, h);
  desktop_apply_theme(ds);

  /* Etapa UX W7-ish (2026-05-03): wallpaper renderiza os icons da pasta
   * Desktop do user (a la Desktop do W7/Ubuntu). A pasta <home>/Desktop e
   * provisionada pelo core CapyOS (user_home_prepare). Para usuarios sem
   * home definido cai pra root. Compositor recebe o paint callback; clique
   * e right-click no espaco vazio sao roteados pra desktop_icons em
   * desktop_handle_mouse. */
  {
    struct session_context *sess = session_active();
    const struct user_record *user = sess ? session_user(sess) : NULL;
    const char *home = (user && user->home[0]) ? user->home : "/";
    /* Render <home>/Desktop, not the whole home: the home now holds
     * Desktop/Documents/Personal/Professional and only Desktop belongs on
     * the wallpaper. Fall back to the home root when no user home exists. */
    char desktop_path[160];
    const char *icons_path = home;
    if (user && user->home[0]) {
      kstrcpy(desktop_path, sizeof(desktop_path), home);
      kbuf_append(desktop_path, sizeof(desktop_path), "/Desktop");
      icons_path = desktop_path;
    }
    desktop_icons_init(icons_path, TASKBAR_HEIGHT);
    compositor_set_desktop_callback(desktop_icons_paint);
  }

  g_menu_desktop = ds;
  /* Etapa F4 i18n (2026-05-03): nomes localizados via APP_T no
   * idioma da sessao ativa. Note: o taskbar copia a string ao
   * adicionar -- mudancas de idioma em runtime so afetam a UI
   * apos relogin (re-init do desktop). O conjunto de entradas vem do
   * registro unico desktop_menu_apps (modo resumido por padrao; a
   * entrada "Lista completa" alterna para o catalogo inteiro). */
  g_menu_show_all = 0;
  desktop_populate_menu(ds);

  desktop_update_tray(ds);

  mouse_set_bounds((int32_t)w, (int32_t)h);
  mouse_set_position((int32_t)(w / 2), (int32_t)(h / 2));
  ds->mouse_initialized = 1;
  ds->active_terminal = NULL;
}

static void desktop_terminal_command(struct terminal *term, const char *data,
                                     size_t len) {
  char line[256];
  size_t i = 0;
  if (!term || !data) return;

  if (data[0] == '\0') {
    desktop_terminal_prompt(term);
    return;
  }

  if (len >= sizeof(line)) len = sizeof(line) - 1;
  for (i = 0; i < len; i++) line[i] = data[i];
  line[len] = '\0';

  /* `exit` closes ONLY this terminal window (like the title-bar X), never
   * the whole desktop session -- tearing the session down on a shell
   * reflex command was a repeated operator complaint. Leaving the desktop
   * remains explicit: the launcher's Logout entry or the shell `bye`. */
  if (kstreq(line, "exit")) {
    struct gui_window *win = term->window;
    if (win) {
      if (g_menu_desktop)
        taskbar_remove_window(&g_menu_desktop->taskbar, win->id);
      /* compositor_destroy_window runs win->on_close
       * (desktop_terminal_on_close) exactly once, which detaches the
       * shell sink and clears g_terminal_open. */
      compositor_destroy_window(win->id);
    }
    return;
  }

  if (kstreq(line, "clear")) {
    terminal_clear(term);
    desktop_terminal_prompt(term);
    return;
  }

  {
    int dispatch_status;
    desktop_shell_begin_terminal_output(term);
    dispatch_status = kernel_desktop_dispatch_shell_command_scoped(
        NULL, line, NULL, 0, desktop_shell_write, desktop_shell_putc,
        desktop_shell_clear);
    desktop_shell_end_terminal_output();
    if (dispatch_status == DESKTOP_SHELL_DISPATCH_BUSY) {
      terminal_write_string(term,
                            "[ocupado] CapyAI esta executando um comando; "
                            "tente novamente.\n");
      desktop_terminal_prompt(term);
      return;
    }
    if (dispatch_status != DESKTOP_SHELL_DISPATCH_HANDLED) {
      terminal_write_string(term, "[erro] comando desconhecido\n");
      terminal_write_string(term, "Use help-any para listar comandos.\n");
      desktop_terminal_prompt(term);
      return;
    }
  }
  if (kernel_desktop_shell_should_stop()) {
    desktop_stop();
    return;
  }
  desktop_terminal_prompt(term);
}

void desktop_open_terminal_here(const char *target_path) {
  struct session_context *sess = kernel_desktop_shell_session();
  if (!sess) sess = session_active();
  if (target_path && target_path[0] && sess) session_set_cwd(sess, target_path);
  if (g_menu_desktop) desktop_open_terminal(g_menu_desktop);
}

void desktop_open_terminal(struct desktop_session *ds) {
  uint8_t scale = compositor_ui_scale();
  int32_t margin_x = 28 + 24 * (scale - 1);
  int32_t margin_y = 28 + 16 * (scale - 1);
  uint32_t width = 0;
  uint32_t height = 0;
  if (!ds) return;

  /* If already open, just focus the existing window */
  if (g_terminal_open && g_desktop_terminal.window) {
    compositor_show_window(g_desktop_terminal.window->id);
    compositor_focus_window(g_desktop_terminal.window->id);
    return;
  }

  g_terminal_open = 0;

  width = ds->screen_w > (uint32_t)(margin_x * 2)
              ? ds->screen_w - (uint32_t)(margin_x * 2)
              : ds->screen_w;
  height = ds->screen_h > (uint32_t)(margin_y * 2 + TASKBAR_HEIGHT)
               ? ds->screen_h - (uint32_t)(margin_y * 2 + TASKBAR_HEIGHT)
               : ds->screen_h - TASKBAR_HEIGHT;

  {
    struct gui_window *win = compositor_create_window(
        "Terminal", margin_x, margin_y, width, height);
    if (!win) return;
    compositor_show_window(win->id);
    compositor_focus_window(win->id);
    taskbar_add_window(&ds->taskbar, win->id, "Terminal");
    taskbar_set_focused(&ds->taskbar, win->id);

    terminal_init(&g_desktop_terminal, win);
    terminal_set_color(&g_desktop_terminal, compositor_theme()->terminal_fg,
                       compositor_theme()->terminal_bg);
    terminal_clear(&g_desktop_terminal);
    terminal_write_string(&g_desktop_terminal, "CapyOS Desktop Terminal\n");
    terminal_write_string(&g_desktop_terminal,
                          "Type 'exit' to close this terminal window.\n");
    terminal_write_string(&g_desktop_terminal, "Type 'bye' to log out.\n");
    terminal_write_string(&g_desktop_terminal,
                          "Scroll: mouse wheel / PgUp / PgDn.\n\n");
    desktop_terminal_prompt(&g_desktop_terminal);
    terminal_set_output_callback(&g_desktop_terminal, desktop_terminal_command, NULL);
    win->user_data = &g_desktop_terminal;
    win->on_paint = desktop_terminal_paint;
    win->on_key = desktop_terminal_key;
    win->on_scroll = desktop_terminal_scroll;
    win->on_close = desktop_terminal_on_close;
    win->on_resize = desktop_terminal_resize;
    win->bg_color = compositor_theme()->terminal_bg;
    win->border_color = compositor_theme()->window_border;
    ds->active_terminal = &g_desktop_terminal;
    g_terminal_open = 1;
  }
}

void desktop_handle_input(struct desktop_session *ds, uint32_t keycode,
                          char ch) {
  struct gui_window *focused = NULL;
  if (!ds) return;

  if (keycode == KEY_SUPER) {
    if (inline_prompt_is_open()) return;
    if (context_menu_is_open()) context_menu_close();
    taskbar_toggle_menu(&ds->taskbar);
    compositor_invalidate_all();
    return;
  }

  /* Etapa UX W7-ish (2026-05-03): inline_prompt absorve teclas
   * antes do dispatch normal (Enter/Esc/Backspace/printable). Util
   * para Rename/New no desktop_icons + file_manager. */
  if (inline_prompt_is_open()) {
    if (inline_prompt_handle_key(keycode, ch)) {
      compositor_invalidate_all();
      return;
    }
  }
  if (desktop_handle_overlay_escape(ds, keycode, ch)) return;
  if (ds->taskbar.menu_open &&
      taskbar_handle_menu_key(&ds->taskbar, keycode, ch)) {
    return;
  }

  focused = compositor_focused_window();
  {
    struct gui_event ev;
    uint32_t wid = focused ? focused->id : 0;
    /* For printable characters pass the ASCII value as keycode so that
     * window handlers can treat it uniformly.  For special keys (arrows,
     * function keys etc.) keycode already carries the KEY_* constant and
     * ch is 0, so we pass keycode directly. */
    uint32_t kc = ch ? (uint32_t)(uint8_t)ch : keycode;
    ev.type = GUI_EVENT_KEY_DOWN;
    ev.window_id = wid;
    ev.key.keycode = kc;
    ev.key.modifiers = 0;
    ev.key.ch = ch;
    ev.timestamp = 0;
    (void)gui_window_dispatch_event(&ev);
  }
}

int desktop_run_frame(struct desktop_session *ds) {
  int work_done = 0;
  if (!ds || !ds->active) return 0;
  if (ds->settings) {
    const char *theme = ds->settings->theme[0] ? ds->settings->theme : "capyos";
    if (!kstreq(ds->theme_name, theme)) {
      desktop_apply_theme(ds);
      work_done = 1;
    }
  }

  if (desktop_handle_mouse(ds)) work_done = 1;
  desktop_sample_dispatcher_health(ds);

  {
    struct rtc_time rtc;
    char clock_buf[16];
    int clock_changed = 0;
    rtc_read(&rtc);
    rtc_format_time(&rtc, clock_buf, sizeof(clock_buf));
    clock_changed = taskbar_update_clock(&ds->taskbar, clock_buf);
    if (clock_changed) work_done = 1;
    if (clock_changed && desktop_update_tray(ds)) work_done = 1;
  }

  /* Post-M5 W2: Task Manager auto-refresh tick. Cheap when the
   * window is closed (single load + branch); when open it
   * invalidates ~every TASK_MANAGER_AUTO_REFRESH_FRAMES frames so
   * apps started after the window opened (and processes that
   * exited) reflect in the row list within ~0.5s on real hw. */
  task_manager_tick();

  /* Legacy browser per-frame tick erradicado na sessao 6 (2026-05-05).
   * Um sucessor deve voltar somente como adaptador versionado na etapa correta. */

  {
    struct mouse_state ms;
    int scene_dirty = compositor_needs_render();
    int cursor_changed = 0;
    mouse_get_state(&ms);
    cursor_changed = !ds->cursor_valid || ds->cursor_x != ms.x ||
                     ds->cursor_y != ms.y ||
                     compositor_cursor_needs_render(ms.x, ms.y);
    if (scene_dirty) {
      compositor_render();
      work_done = 1;
    }
    if (scene_dirty || cursor_changed) {
      compositor_render_cursor(ms.x, ms.y);
      ds->cursor_valid = 1;
      ds->cursor_x = ms.x;
      ds->cursor_y = ms.y;
      if (cursor_changed) work_done = 1;
    }
  }

  return work_done;
}

void desktop_shutdown(struct desktop_session *ds) {
  if (!ds) return;
  compositor_shutdown();
  /* Reset taskbar menu popup pointer (compositor_init will free the surface) */
  ds->taskbar.menu_popup = NULL;
  ds->taskbar.menu_open = 0;
  ds->taskbar.menu_entry_count = 0;
  ds->taskbar.recent_count = 0;
  ds->taskbar.item_count = 0;
  ds->taskbar.window = NULL;
  ds->active_terminal = NULL;
  g_desktop_terminal.window = NULL;
  g_terminal_open = 0;
  g_shell_output_term = NULL;
  g_menu_desktop = NULL;
  shell_set_output_callbacks(NULL, NULL);
  shell_set_clear_callback(NULL);
  ds->active = 0;
}
