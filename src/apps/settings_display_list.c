#include "apps/settings.h"
#include "auth/session.h"
#include "auth/user.h"
#include "core/system_init.h"
#include "core/version.h"
#include "drivers/input/keyboard.h"
#include "gui/compositor.h"
#include "gui/font.h"
#include "gui/widget.h"
#include "lang/app_language.h"
#include "lang/localization.h"
#include "net/stack.h"
#include "services/update_agent.h"
#include "util/kstring.h"

#include "internal/settings_internal.h"
#include "internal/app_display_list_bridge.h"

#include <stddef.h>
#include <stdint.h>

#if defined(CAPYOS_HAVE_CAPYUI_WIDGET)
#define SETTINGS_DL_CMD_CAP 256u
#define SETTINGS_DL_TEXT_CAP 8192u

static struct capy_dl_cmd g_settings_dl_cmds[APP_DISPLAY_LIST_BRIDGE_FRAME_COUNT][SETTINGS_DL_CMD_CAP];
static char g_settings_dl_text[APP_DISPLAY_LIST_BRIDGE_FRAME_COUNT][SETTINGS_DL_TEXT_CAP];
static struct app_display_list_bridge g_settings_dl_bridge;

static void settings_dl_init_once(void) {
  if (g_settings_dl_bridge.initialized) return;
  app_display_list_bridge_init(&g_settings_dl_bridge,
                               g_settings_dl_cmds[0],
                               g_settings_dl_cmds[1],
                               SETTINGS_DL_CMD_CAP,
                               g_settings_dl_text[0],
                               g_settings_dl_text[1],
                               SETTINGS_DL_TEXT_CAP);
}

void settings_display_list_reset(void) {
  app_display_list_bridge_reset(&g_settings_dl_bridge);
}

static void settings_dl_fit_text(const struct font *f, const char *src,
                                 uint32_t max_width, char *out,
                                 size_t out_len) {
  size_t len = 0;
  size_t max_chars = 0;
  if (!out || out_len == 0) return;
  out[0] = '\0';
  if (!f || !src || max_width == 0 || f->glyph_width == 0) return;
  max_chars = max_width / f->glyph_width;
  if (max_chars == 0) return;
  while (src[len]) len++;
  if (len <= max_chars) {
    kstrcpy(out, out_len, src);
    return;
  }
  if (max_chars <= 3) {
    size_t n = max_chars;
    if (n >= out_len) n = out_len - 1;
    for (size_t i = 0; i < n; i++) out[i] = '.';
    out[n] = '\0';
    return;
  }
  {
    size_t copy = max_chars - 3;
    if (copy > out_len - 4) copy = out_len - 4;
    for (size_t i = 0; i < copy; i++) out[i] = src[i];
    out[copy] = '.';
    out[copy + 1] = '.';
    out[copy + 2] = '.';
    out[copy + 3] = '\0';
  }
}

static int settings_dl_emit_fit(struct capy_display_list *out,
                                const struct font *f,
                                int32_t x,
                                int32_t y,
                                uint32_t max_width,
                                const char *text,
                                uint32_t color) {
  char fitted[96];
  settings_dl_fit_text(f, text, max_width, fitted, sizeof(fitted));
  if (!fitted[0]) return 0;
  return app_display_list_emit_text(out, x, y, max_width, f->glyph_height,
                                    fitted, color, 16u);
}

static int settings_dl_emit_option_row(struct capy_display_list *out,
                                       const struct font *f,
                                       const struct gui_theme_palette *theme,
                                       int32_t x,
                                       int32_t y,
                                       int32_t w,
                                       const char *label,
                                       int selected,
                                       uint16_t kind,
                                       const char *arg) {
  uint32_t bg = selected ? theme->accent : theme->accent_alt;
  uint32_t h = 22u;
  char buf[64];
  size_t bl;
  if (app_display_list_emit_rect(out, x, y, (uint32_t)w, h, bg) != 0) return -1;
  if (app_display_list_emit_border_rect(out, x, y, (uint32_t)w, h,
                                        theme->window_border, 1u) != 0) {
    return -1;
  }
  if (label) {
    if (selected) kstrcpy(buf, sizeof(buf), ">> ");
    else kstrcpy(buf, sizeof(buf), "   ");
    bl = kstrlen(buf);
    for (size_t i = 0; label[i] && bl + 1 < sizeof(buf); ++i) buf[bl++] = label[i];
    buf[bl] = '\0';
    if (settings_dl_emit_fit(out, f, x + 6, y + 7,
                             (w > 12) ? (uint32_t)(w - 12) : 0u,
                             buf, theme->accent_text) != 0) {
      return -1;
    }
  }
  rows_add(x, y, x + w, y + (int32_t)h, kind, arg);
  return y + (int32_t)h + 4;
}

static int settings_dl_emit_action_button(struct capy_display_list *out,
                                          const struct font *f,
                                          const struct gui_theme_palette *theme,
                                          int32_t x,
                                          int32_t y,
                                          int32_t w,
                                          const char *label,
                                          uint16_t kind) {
  uint32_t h = 22u;
  if (app_display_list_emit_rect(out, x, y, (uint32_t)w, h, theme->accent) != 0) return -1;
  if (app_display_list_emit_border_rect(out, x, y, (uint32_t)w, h,
                                        theme->window_border, 1u) != 0) {
    return -1;
  }
  if (settings_dl_emit_fit(out, f, x + 6, y + 7,
                           (w > 12) ? (uint32_t)(w - 12) : 0u,
                           label, theme->accent_text) != 0) {
    return -1;
  }
  rows_add(x, y, x + w, y + (int32_t)h, kind, NULL);
  return y + (int32_t)h + 4;
}

static int settings_dl_emit_tabs(struct capy_display_list *out,
                                 struct settings_app *app,
                                 const struct gui_theme_palette *theme) {
  for (int i = 0; i < SETTINGS_TAB_COUNT; i++) {
    if (app->tab_buttons[i]) {
      struct widget_style st = widget_button_style();
      st.bg_color = (i == (int)app->active_tab) ? theme->accent : theme->accent_alt;
      st.text_color = (i == (int)app->active_tab) ? theme->accent_text : theme->text;
      widget_set_style(app->tab_buttons[i], &st);
      if (app_display_list_emit_widget(out, app->tab_buttons[i]) != 0) return -1;
    }
  }
  return 0;
}

static int settings_dl_emit_display_tab(struct capy_display_list *out,
                                        const struct font *f,
                                        const struct gui_theme_palette *theme,
                                        int32_t cx,
                                        int32_t cy,
                                        int32_t row_w,
                                        uint32_t content_w) {
  const char *lang = app_current_language();
  struct system_settings live;
  const char *current = "capyos";
  char line[80];
  if (system_load_settings(&live) == 0 && live.theme[0]) current = live.theme;
  if (settings_dl_emit_fit(out, f, cx, cy, content_w,
                           localization_select(lang, "Tema (clique para aplicar):",
                                               "Theme (click to apply):",
                                               "Tema (clic para aplicar):"),
                           theme->text) != 0) return -1;
  cy += 18;
  static const char *const k_themes[] = {
    "capyos", "classic-modern", "ocean", "forest", "love", "high-contrast"
  };
  for (size_t i = 0; i < sizeof(k_themes) / sizeof(k_themes[0]); ++i) {
    cy = settings_dl_emit_option_row(out, f, theme, cx, cy, row_w, k_themes[i],
                                     kstreq(current, k_themes[i]),
                                     SETTINGS_ROW_THEME, k_themes[i]);
    if (cy < 0) return -1;
  }
  cy += 4;
  line[0] = '\0';
  kbuf_append(line, sizeof(line), localization_select(lang, "Splash: ", "Splash: ", "Splash: "));
  kbuf_append(line, sizeof(line),
              (system_load_settings(&live) == 0 && live.splash_enabled)
                  ? localization_select(lang, "habilitado", "enabled", "habilitado")
                  : localization_select(lang, "desabilitado", "disabled", "deshabilitado"));
  return settings_dl_emit_fit(out, f, cx, cy, content_w, line, theme->text_muted);
}

static int settings_dl_emit_network_tab(struct capy_display_list *out,
                                        const struct font *f,
                                        const struct gui_theme_palette *theme,
                                        int32_t cx,
                                        int32_t cy,
                                        uint32_t content_w) {
  const char *lang = app_current_language();
  struct net_stack_status ns;
  char line[80];
  if (net_stack_status(&ns) == 0) {
    if (settings_dl_emit_fit(out, f, cx, cy, content_w,
                             ns.ready ? localization_select(lang, "Status: pronto", "Status: ready", "Estado: listo")
                                      : localization_select(lang, "Status: indisponivel", "Status: not ready", "Estado: no listo"),
                             theme->text) != 0) return -1;
    cy += 18;
    line[0] = '\0'; kbuf_append(line, sizeof(line), "IPv4: ");
    { char ip[16]; ipv4_str(ns.ipv4.addr, ip, 16); kbuf_append(line, sizeof(line), ip); }
    if (settings_dl_emit_fit(out, f, cx, cy, content_w, line, theme->text) != 0) return -1;
    cy += 18;
    line[0] = '\0'; kbuf_append(line, sizeof(line), localization_select(lang, "Gateway: ", "Gateway: ", "Puerta: "));
    { char ip[16]; ipv4_str(ns.ipv4.gateway, ip, 16); kbuf_append(line, sizeof(line), ip); }
    if (settings_dl_emit_fit(out, f, cx, cy, content_w, line, theme->text) != 0) return -1;
    cy += 18;
    line[0] = '\0'; kbuf_append(line, sizeof(line), "DNS: ");
    { char ip[16]; ipv4_str(ns.ipv4.dns, ip, 16); kbuf_append(line, sizeof(line), ip); }
    if (settings_dl_emit_fit(out, f, cx, cy, content_w, line, theme->text) != 0) return -1;
    cy += 18;
  } else {
    if (settings_dl_emit_fit(out, f, cx, cy, content_w,
                             localization_select(lang, "Rede: indisponivel",
                                                 "Network: unavailable", "Red: no disponible"),
                             theme->text_muted) != 0) return -1;
    cy += 18;
  }
  return settings_dl_emit_fit(out, f, cx, cy, content_w,
                              localization_select(lang,
                                                  "Use o CLI 'net-set' para configurar",
                                                  "Use CLI net-set to configure",
                                                  "Use el CLI 'net-set' para configurar"),
                              theme->text_muted);
}

static int settings_dl_emit_keyboard_tab(struct capy_display_list *out,
                                         const struct font *f,
                                         const struct gui_theme_palette *theme,
                                         int32_t cx,
                                         int32_t cy,
                                         int32_t row_w,
                                         uint32_t content_w) {
  const char *lang = app_current_language();
  const char *current = keyboard_current_layout();
  size_t n;
  if (!current) current = "us";
  if (settings_dl_emit_fit(out, f, cx, cy, content_w,
                           localization_select(lang,
                                               "Layout (clique para aplicar):",
                                               "Layout (click to apply):",
                                               "Distribucion (clic para aplicar):"),
                           theme->text) != 0) return -1;
  cy += 18;
  n = keyboard_layout_count();
  for (size_t i = 0; i < n; ++i) {
    const char *name = keyboard_layout_name(i);
    if (!name) continue;
    cy = settings_dl_emit_option_row(out, f, theme, cx, cy, row_w, name,
                                     kstreq(current, name),
                                     SETTINGS_ROW_KEYBOARD, name);
    if (cy < 0) return -1;
  }
  return 0;
}

static int settings_dl_emit_language_tab(struct capy_display_list *out,
                                         const struct font *f,
                                         const struct gui_theme_palette *theme,
                                         int32_t cx,
                                         int32_t cy,
                                         int32_t row_w,
                                         uint32_t content_w) {
  const char *lang = app_current_language();
  static const char *const k_langs[] = { "pt-BR", "en", "es" };
  if (settings_dl_emit_fit(out, f, cx, cy, content_w,
                           localization_select(lang,
                                               "Idioma (clique para aplicar):",
                                               "Language (click to apply):",
                                               "Idioma (clic para aplicar):"),
                           theme->text) != 0) return -1;
  cy += 18;
  for (size_t i = 0; i < sizeof(k_langs) / sizeof(k_langs[0]); ++i) {
    cy = settings_dl_emit_option_row(out, f, theme, cx, cy, row_w, k_langs[i],
                                     kstreq(lang, k_langs[i]),
                                     SETTINGS_ROW_LANGUAGE, k_langs[i]);
    if (cy < 0) return -1;
  }
  cy += 4;
  return settings_dl_emit_fit(out, f, cx, cy, content_w,
                              localization_select(lang,
                                                  "Salvo por-usuario em /home/<user>/.capyos/prefs.ini",
                                                  "Saved per-user in /home/<user>/.capyos/prefs.ini",
                                                  "Guardado por usuario en /home/<user>/.capyos/prefs.ini"),
                              theme->text_muted);
}

static int settings_dl_emit_browser_tab(struct capy_display_list *out,
                                        const struct font *f,
                                        const struct gui_theme_palette *theme,
                                        int32_t cx,
                                        int32_t cy,
                                        int32_t action_w,
                                        uint32_t content_w) {
  const char *lang = app_current_language();
  struct system_settings live;
  char line[80];
  const char *hp = "https://wikipedia.org";
  if (system_load_settings(&live) == 0 && live.browser_homepage[0]) hp = live.browser_homepage;
  line[0] = '\0';
  kbuf_append(line, sizeof(line), localization_select(lang, "Pagina inicial: ", "Home: ", "Pagina principal: "));
  {
    int prefix_len = (int)kstrlen(line);
    int remain = (int)sizeof(line) - 1 - prefix_len;
    int hp_len = 0;
    while (hp[hp_len] && hp_len < 256) hp_len++;
    if (hp_len <= remain) kbuf_append(line, sizeof(line), hp);
    else {
      char trunc[64];
      int copy = remain - 3;
      if (copy < 0) copy = 0;
      if (copy > (int)sizeof(trunc) - 1) copy = (int)sizeof(trunc) - 1;
      for (int i = 0; i < copy; i++) trunc[i] = hp[i];
      trunc[copy] = '\0';
      kbuf_append(line, sizeof(line), trunc);
      kbuf_append(line, sizeof(line), "...");
    }
  }
  if (settings_dl_emit_fit(out, f, cx, cy, content_w, line, theme->text) != 0) return -1;
  cy += 18;
  cy = settings_dl_emit_action_button(out, f, theme, cx, cy, action_w,
                                      localization_select(lang,
                                                          "Editar pagina inicial",
                                                          "Edit homepage",
                                                          "Editar pagina principal"),
                                      SETTINGS_ROW_HOMEPAGE);
  if (cy < 0) return -1;
  cy += 4;
  if (settings_dl_emit_fit(out, f, cx, cy, content_w,
                           localization_select(lang,
                                               "Fallback offline: file://capyos/wikipedia",
                                               "Offline fallback: file://capyos/wikipedia",
                                               "Respaldo offline: file://capyos/wikipedia"),
                           theme->text_muted) != 0) return -1;
  cy += 18;
  return settings_dl_emit_fit(out, f, cx, cy, content_w,
                              localization_select(lang,
                                                  "Toolbar: < Voltar  > Avancar  R Recarregar  H Inicial  Ir",
                                                  "Toolbar: < Back  > Forward  R Reload  H Home  Go",
                                                  "Toolbar: < Atras  > Adelante  R Recargar  H Inicio  Ir"),
                              theme->text_muted);
}

static int settings_dl_emit_users_tab(struct capy_display_list *out,
                                      const struct font *f,
                                      const struct gui_theme_palette *theme,
                                      int32_t cx,
                                      int32_t cy,
                                      int32_t action_w,
                                      uint32_t content_w) {
  const char *lang = app_current_language();
  char line[80];
  line[0] = '\0';
  kbuf_append(line, sizeof(line), localization_select(lang, "Usuarios: ", "Users: ", "Usuarios: "));
  kbuf_append(line, sizeof(line),
              userdb_has_any_user()
                  ? localization_select(lang, "configurados", "configured", "configurados")
                  : localization_select(lang, "nenhum", "none", "ninguno"));
  if (settings_dl_emit_fit(out, f, cx, cy, content_w, line, theme->text) != 0) return -1;
  cy += 18;
  cy = settings_dl_emit_action_button(out, f, theme, cx, cy, action_w,
                                      localization_select(lang,
                                                          "Adicionar usuario...",
                                                          "Add user...",
                                                          "Anadir usuario..."),
                                      SETTINGS_ROW_NEWUSER);
  if (cy < 0) return -1;
  cy += 4;
  if (settings_dl_emit_fit(out, f, cx, cy, content_w,
                           localization_select(lang,
                                               "Armazenado em /etc/users.db (PBKDF2-SHA256, 64k)",
                                               "Stored in /etc/users.db (PBKDF2-SHA256, 64k rounds)",
                                               "Almacenado en /etc/users.db (PBKDF2-SHA256, 64k)"),
                           theme->text_muted) != 0) return -1;
  cy += 18;
  return settings_dl_emit_fit(out, f, cx, cy, content_w,
                              localization_select(lang,
                                                  "CLI: list-users, passwd <usuario>",
                                                  "CLI: list-users, passwd <user>",
                                                  "CLI: list-users, passwd <usuario>"),
                              theme->text_muted);
}

static int settings_dl_emit_updates_tab(struct capy_display_list *out,
                                        const struct font *f,
                                        const struct gui_theme_palette *theme,
                                        int32_t cx,
                                        int32_t cy,
                                        uint32_t content_w) {
  const char *lang = app_current_language();
  struct system_update_status us;
  char line[80];
  update_agent_status_get(&us);
  line[0] = '\0';
  kbuf_append(line, sizeof(line), localization_select(lang, "Canal: ", "Channel: ", "Canal: "));
  kbuf_append(line, sizeof(line), us.channel[0] ? us.channel : "stable");
  if (settings_dl_emit_fit(out, f, cx, cy, content_w, line, theme->text) != 0) return -1;
  cy += 18;
  line[0] = '\0';
  kbuf_append(line, sizeof(line), localization_select(lang, "Branch: ", "Branch: ", "Rama: "));
  kbuf_append(line, sizeof(line), us.branch[0] ? us.branch : "main");
  if (settings_dl_emit_fit(out, f, cx, cy, content_w, line, theme->text) != 0) return -1;
  cy += 18;
  if (settings_dl_emit_fit(out, f, cx, cy, content_w,
                           us.update_available
                               ? localization_select(lang, "Atualizacao: disponivel", "Update: available", "Actualizacion: disponible")
                               : localization_select(lang, "Atualizacao: em dia", "Update: up to date", "Actualizacion: al dia"),
                           theme->text) != 0) return -1;
  cy += 18;
  if (us.available_version[0]) {
    line[0] = '\0';
    kbuf_append(line, sizeof(line), localization_select(lang, "Versao: ", "Version: ", "Version: "));
    kbuf_append(line, sizeof(line), us.available_version);
    return settings_dl_emit_fit(out, f, cx, cy, content_w, line, theme->text);
  }
  return 0;
}

static int settings_dl_emit_about_tab(struct capy_display_list *out,
                                      const struct font *f,
                                      const struct gui_theme_palette *theme,
                                      int32_t cx,
                                      int32_t cy,
                                      uint32_t content_w) {
  const char *lang = app_current_language();
  char line[80];
  if (settings_dl_emit_fit(out, f, cx, cy, content_w, "CapyOS " CAPYOS_VERSION_FULL,
                           theme->text) != 0) return -1;
  cy += 18;
  line[0] = '\0';
  kbuf_append(line, sizeof(line), localization_select(lang, "Canal: ", "Channel: ", "Canal: "));
  kbuf_append(line, sizeof(line), CAPYOS_VERSION_CHANNEL);
  if (settings_dl_emit_fit(out, f, cx, cy, content_w, line, theme->text) != 0) return -1;
  cy += 18;
  if (settings_dl_emit_fit(out, f, cx, cy, content_w,
                           localization_select(lang,
                                               "Desenvolvedor: Henrique Schwarz Souza Farisco",
                                               "Developer: Henrique Schwarz Souza Farisco",
                                               "Desarrollador: Henrique Schwarz Souza Farisco"),
                           theme->text) != 0) return -1;
  cy += 18;
  if (settings_dl_emit_fit(out, f, cx, cy, content_w,
                           localization_select(lang, "Licenca: Apache-2.0",
                                               "License: Apache-2.0", "Licencia: Apache-2.0"),
                           theme->text) != 0) return -1;
  cy += 18;
  return settings_dl_emit_fit(out, f, cx, cy, content_w,
                              localization_select(lang, "Plataforma: UEFI/GPT/x86_64",
                                                  "Track: UEFI/GPT/x86_64",
                                                  "Plataforma: UEFI/GPT/x86_64"),
                              theme->text_muted);
}

static int settings_emit_display_list(void *producer, struct capy_display_list *out) {
  struct settings_app *app = (struct settings_app *)producer;
  struct gui_surface *s;
  const struct font *f = font_default();
  const struct gui_theme_palette *theme = compositor_theme();
  uint8_t scale = compositor_ui_scale();
  int32_t sidebar_w;
  uint32_t content_w;
  int32_t cx;
  int32_t cy;
  int32_t row_w;
  int32_t action_w;
  if (!app || !app->window || !out || !f) return -1;
  s = &app->window->surface;
  sidebar_w = settings_sidebar_width(s->width, scale);
  content_w = (s->width > (uint32_t)sidebar_w + 22u) ? s->width - (uint32_t)sidebar_w - 22u : 0u;
  settings_layout_tabs(app);
  rows_reset();
  out->count = 0u;
  out->text_used = 0u;
  if (app_display_list_emit_rect(out, 0, 0, s->width, s->height, theme->window_bg) != 0) return -1;
  if (app_display_list_emit_rect(out, 0, 0, (uint32_t)sidebar_w, s->height, theme->terminal_bg) != 0) return -1;
  if (settings_dl_emit_tabs(out, app, theme) != 0) return -1;
  cx = sidebar_w + 10;
  cy = 12;
  row_w = (content_w > 240u) ? 220 : (int32_t)content_w;
  action_w = (content_w > 190u) ? 170 : (int32_t)content_w;
  if (row_w < 96) row_w = 96;
  if (action_w < 96) action_w = 96;
  if (settings_dl_emit_fit(out, f, cx, cy, content_w, tab_label(app->active_tab), theme->accent) != 0) return -1;
  cy += 24;
  if ((uint32_t)cy < s->height) {
    uint32_t x_end = (s->width > 8u) ? s->width - 8u : s->width;
    if (x_end > (uint32_t)cx) {
      if (app_display_list_emit_rect(out, cx, cy, x_end - (uint32_t)cx, 1u,
                                     theme->window_border) != 0) return -1;
    }
  }
  cy += 8;
  switch (app->active_tab) {
    case SETTINGS_TAB_DISPLAY:
      if (settings_dl_emit_display_tab(out, f, theme, cx, cy, row_w, content_w) != 0) return -1;
      break;
    case SETTINGS_TAB_NETWORK:
      if (settings_dl_emit_network_tab(out, f, theme, cx, cy, content_w) != 0) return -1;
      break;
    case SETTINGS_TAB_KEYBOARD:
      if (settings_dl_emit_keyboard_tab(out, f, theme, cx, cy, row_w, content_w) != 0) return -1;
      break;
    case SETTINGS_TAB_LANGUAGE:
      if (settings_dl_emit_language_tab(out, f, theme, cx, cy, row_w, content_w) != 0) return -1;
      break;
    case SETTINGS_TAB_BROWSER:
      if (settings_dl_emit_browser_tab(out, f, theme, cx, cy, action_w, content_w) != 0) return -1;
      break;
    case SETTINGS_TAB_USERS:
      if (settings_dl_emit_users_tab(out, f, theme, cx, cy, action_w, content_w) != 0) return -1;
      break;
    case SETTINGS_TAB_UPDATES:
      if (settings_dl_emit_updates_tab(out, f, theme, cx, cy, content_w) != 0) return -1;
      break;
    case SETTINGS_TAB_ABOUT:
      if (settings_dl_emit_about_tab(out, f, theme, cx, cy, content_w) != 0) return -1;
      break;
    default:
      break;
  }
  if (app->status_text[0] && s->height > f->glyph_height + 6u) {
    int32_t status_y = (int32_t)s->height - (int32_t)f->glyph_height - 3;
    if (settings_dl_emit_fit(out, f, cx, status_y, content_w, app->status_text,
                             app->status_color ? app->status_color : theme->text_muted) != 0) {
      return -1;
    }
  }
  return 0;
}

int settings_render_display_list(struct settings_app *app) {
  if (!app || !app->window) return -1;
  settings_dl_init_once();
  return app_display_list_bridge_render_window(&g_settings_dl_bridge, app->window,
                                               settings_emit_display_list, app);
}
#endif
