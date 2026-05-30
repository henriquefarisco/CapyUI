#include "apps/task_manager.h"
#include "gui/compositor.h"
#include "gui/font.h"
#include "kernel/process.h"
#include "kernel/process_iter.h"
#include "kernel/task.h"
#include "kernel/task_iter.h"
#include "services/service_manager.h"
#include "lang/app_language.h"
#include "lang/localization.h"

#include "internal/app_display_list_bridge.h"

#include <stddef.h>
#include <stdint.h>

#if defined(CAPYOS_HAVE_CAPYUI_WIDGET)
#define TASK_MANAGER_DL_CMD_CAP 384u
#define TASK_MANAGER_DL_TEXT_CAP 8192u
#define TASK_MANAGER_TAB_COUNT 3
#define TASK_MANAGER_TAB_HEIGHT 22

static struct capy_dl_cmd g_tm_dl_cmds[APP_DISPLAY_LIST_BRIDGE_FRAME_COUNT][TASK_MANAGER_DL_CMD_CAP];
static char g_tm_dl_text[APP_DISPLAY_LIST_BRIDGE_FRAME_COUNT][TASK_MANAGER_DL_TEXT_CAP];
static struct app_display_list_bridge g_tm_dl_bridge;

static void task_manager_dl_init_once(void) {
  if (g_tm_dl_bridge.initialized) return;
  app_display_list_bridge_init(&g_tm_dl_bridge,
                               g_tm_dl_cmds[0],
                               g_tm_dl_cmds[1],
                               TASK_MANAGER_DL_CMD_CAP,
                               g_tm_dl_text[0],
                               g_tm_dl_text[1],
                               TASK_MANAGER_DL_TEXT_CAP);
}

void task_manager_display_list_reset(void) {
  app_display_list_bridge_reset(&g_tm_dl_bridge);
}

static void task_manager_dl_itoa(int v, char *buf, int buflen) {
  int p = 0;
  char tmp[12];
  int tp = 0;
  if (!buf || buflen <= 0) return;
  if (v < 0 && p < buflen - 1) {
    buf[p++] = '-';
    v = -v;
  }
  if (v == 0) {
    if (p < buflen - 1) buf[p++] = '0';
    buf[p] = '\0';
    return;
  }
  while (v > 0 && tp < 11) {
    tmp[tp++] = (char)('0' + (v % 10));
    v /= 10;
  }
  for (int i = tp - 1; i >= 0 && p < buflen - 1; i--) buf[p++] = tmp[i];
  buf[p] = '\0';
}

static int task_manager_dl_count_tasks(void) {
  struct task_iter it;
  struct task_stats s;
  int n = 0;
  for (int ok = task_iter_first(&it, &s); ok; ok = task_iter_next(&it, &s)) {
    if (s.state != TASK_STATE_ZOMBIE && s.state != TASK_STATE_DEAD) n++;
  }
  return n;
}

static int task_manager_dl_count_processes(void) {
  struct process_iter it;
  struct process_stats s;
  int n = 0;
  for (int ok = process_iter_first(&it, &s); ok; ok = process_iter_next(&it, &s)) {
    if (s.state != PROC_STATE_ZOMBIE) n++;
  }
  return n;
}

static int task_manager_dl_visible_count(enum task_manager_view view) {
  switch (view) {
  case TASK_MANAGER_VIEW_TASKS:
    return task_manager_dl_count_tasks();
  case TASK_MANAGER_VIEW_PROCESSES:
    return task_manager_dl_count_processes();
  case TASK_MANAGER_VIEW_SERVICES:
  default:
    return (int)service_manager_count();
  }
}

static int32_t task_manager_dl_row_y(int row_index) {
  return (int32_t)(TASK_MANAGER_TAB_HEIGHT + 24) + row_index * 18;
}

static int task_manager_dl_emit_text(struct capy_display_list *out,
                                     int32_t x,
                                     int32_t y,
                                     uint32_t width,
                                     const struct font *f,
                                     const char *text,
                                     uint32_t color) {
  if (!f) return 0;
  return app_display_list_emit_text(out, x, y, width, f->glyph_height,
                                    text, color, 16u);
}

static int task_manager_dl_emit_tabs(struct capy_display_list *out,
                                     struct task_manager_app *app,
                                     const struct font *f,
                                     const struct gui_theme_palette *theme,
                                     uint32_t width) {
  const char *lang = app_current_language();
  const char *labels[TASK_MANAGER_TAB_COUNT] = {
    localization_select(lang, "Servicos", "Services", "Servicios"),
    localization_select(lang, "Tarefas", "Tasks", "Tareas"),
    localization_select(lang, "Processos", "Processes", "Procesos")
  };
  uint32_t third = width / 3u;
  if (third == 0u) return 0;
  if (app_display_list_emit_rect(out, 0, 0, width, TASK_MANAGER_TAB_HEIGHT,
                                 theme->window_bg) != 0) return -1;
  for (int i = 0; i < TASK_MANAGER_TAB_COUNT; i++) {
    int32_t tx = (int32_t)(third * (uint32_t)i);
    uint32_t tw = (i == TASK_MANAGER_TAB_COUNT - 1)
                      ? width - third * (uint32_t)i
                      : third;
    int active = ((int)app->view == i);
    uint32_t bg = active ? theme->accent_alt : theme->window_bg;
    uint32_t text_color = active ? theme->text : theme->text_muted;
    if (app_display_list_emit_rect(out, tx, 0, tw, TASK_MANAGER_TAB_HEIGHT, bg) != 0) return -1;
    if (task_manager_dl_emit_text(out, tx + 8, 4, (tw > 16u) ? tw - 16u : tw,
                                  f, labels[i], text_color) != 0) return -1;
  }
  return app_display_list_emit_rect(out, 0, TASK_MANAGER_TAB_HEIGHT - 1,
                                    width, 1u, theme->window_border);
}

static int task_manager_dl_emit_header(struct capy_display_list *out,
                                       const struct font *f,
                                       const struct gui_theme_palette *theme,
                                       uint32_t width,
                                       const char *header_text) {
  int32_t header_y = TASK_MANAGER_TAB_HEIGHT + 4;
  int32_t border_y = TASK_MANAGER_TAB_HEIGHT + 20;
  if (task_manager_dl_emit_text(out, 8, header_y, width > 16u ? width - 16u : width,
                                f, header_text, theme->accent) != 0) return -1;
  return app_display_list_emit_rect(out, 0, border_y, width, 1u, theme->window_border);
}

static int task_manager_dl_highlight_row(struct capy_display_list *out,
                                         uint32_t width,
                                         const struct gui_theme_palette *theme,
                                         int32_t ypos) {
  return app_display_list_emit_rect(out, 0, ypos, width, 18u, theme->accent_alt);
}

static int task_manager_dl_emit_services_rows(struct capy_display_list *out,
                                             struct task_manager_app *app,
                                             const struct font *f,
                                             const struct gui_theme_palette *theme,
                                             uint32_t width,
                                             int32_t footer_y) {
  int row = 0;
  for (size_t i = 0; i < service_manager_count(); i++) {
    struct system_service_status service;
    char id_buf[8];
    int32_t ypos;
    if (service_manager_get_at(i, &service) != 0) continue;
    if (row < app->scroll_offset) { row++; continue; }
    ypos = task_manager_dl_row_y(row - app->scroll_offset);
    if (ypos + 18 > footer_y) break;
    if (row == app->selected && task_manager_dl_highlight_row(out, width, theme, ypos) != 0) return -1;
    task_manager_dl_itoa((int)service.id, id_buf, sizeof(id_buf));
    if (task_manager_dl_emit_text(out, 8, ypos, 24u, f, id_buf, theme->text) != 0) return -1;
    if (task_manager_dl_emit_text(out, 32, ypos, 88u, f,
                                  service_manager_state_label(service.state),
                                  service.state == SYSTEM_SERVICE_STATE_READY
                                      ? theme->accent : theme->text_muted) != 0) return -1;
    if (task_manager_dl_emit_text(out, 124, ypos, 44u, f,
                                  service_manager_startup_label(service.startup),
                                  theme->text) != 0) return -1;
    if (task_manager_dl_emit_text(out, 172, ypos, width > 180u ? width - 180u : 0u,
                                  f, service.name, theme->text) != 0) return -1;
    row++;
  }
  return 0;
}

static int task_manager_dl_emit_tasks_rows(struct capy_display_list *out,
                                           struct task_manager_app *app,
                                           const struct font *f,
                                           const struct gui_theme_palette *theme,
                                           uint32_t width,
                                           int32_t footer_y) {
  struct task_iter it;
  struct task_stats t;
  int row = 0;
  for (int ok = task_iter_first(&it, &t); ok; ok = task_iter_next(&it, &t)) {
    char pid_buf[8];
    int32_t ypos;
    if (t.state == TASK_STATE_ZOMBIE || t.state == TASK_STATE_DEAD) continue;
    if (row < app->scroll_offset) { row++; continue; }
    ypos = task_manager_dl_row_y(row - app->scroll_offset);
    if (ypos + 18 > footer_y) break;
    if (row == app->selected && task_manager_dl_highlight_row(out, width, theme, ypos) != 0) return -1;
    task_manager_dl_itoa((int)t.pid, pid_buf, sizeof(pid_buf));
    if (task_manager_dl_emit_text(out, 8, ypos, 36u, f, pid_buf, theme->text) != 0) return -1;
    if (task_manager_dl_emit_text(out, 48, ypos, 72u, f, task_state_label(t.state),
                                  t.state == TASK_STATE_RUNNING || t.state == TASK_STATE_READY
                                      ? theme->accent : theme->text_muted) != 0) return -1;
    if (task_manager_dl_emit_text(out, 124, ypos, 44u, f, task_priority_label(t.priority),
                                  theme->text) != 0) return -1;
    if (task_manager_dl_emit_text(out, 172, ypos, width > 180u ? width - 180u : 0u,
                                  f, t.name[0] ? t.name : "(unnamed)", theme->text) != 0) return -1;
    row++;
  }
  if (row == 0) {
    int32_t ypos = task_manager_dl_row_y(0);
    return task_manager_dl_emit_text(out, 8, ypos, width > 16u ? width - 16u : width,
                                     f, "(no active tasks yet)", theme->text_muted);
  }
  return 0;
}

static int task_manager_dl_emit_processes_rows(struct capy_display_list *out,
                                               struct task_manager_app *app,
                                               const struct font *f,
                                               const struct gui_theme_palette *theme,
                                               uint32_t width,
                                               int32_t footer_y) {
  struct process_iter it;
  struct process_stats p;
  int row = 0;
  for (int ok = process_iter_first(&it, &p); ok; ok = process_iter_next(&it, &p)) {
    char pid_buf[8];
    char uid_buf[8];
    int32_t ypos;
    if (p.state == PROC_STATE_ZOMBIE) continue;
    if (row < app->scroll_offset) { row++; continue; }
    ypos = task_manager_dl_row_y(row - app->scroll_offset);
    if (ypos + 18 > footer_y) break;
    if (row == app->selected && task_manager_dl_highlight_row(out, width, theme, ypos) != 0) return -1;
    task_manager_dl_itoa((int)p.pid, pid_buf, sizeof(pid_buf));
    task_manager_dl_itoa((int)p.uid, uid_buf, sizeof(uid_buf));
    if (task_manager_dl_emit_text(out, 8, ypos, 36u, f, pid_buf, theme->text) != 0) return -1;
    if (task_manager_dl_emit_text(out, 48, ypos, 72u, f, process_state_label(p.state),
                                  p.state == PROC_STATE_RUNNING ? theme->accent : theme->text_muted) != 0) return -1;
    if (task_manager_dl_emit_text(out, 124, ypos, 44u, f, uid_buf, theme->text) != 0) return -1;
    if (task_manager_dl_emit_text(out, 172, ypos, width > 180u ? width - 180u : 0u,
                                  f, p.name[0] ? p.name : "(unnamed)", theme->text) != 0) return -1;
    row++;
  }
  if (row == 0) {
    int32_t ypos = task_manager_dl_row_y(0);
    return task_manager_dl_emit_text(out, 8, ypos, width > 16u ? width - 16u : width,
                                     f, "(no active processes yet)", theme->text_muted);
  }
  return 0;
}

static const char *task_manager_dl_view_label(enum task_manager_view view) {
  switch (view) {
  case TASK_MANAGER_VIEW_SERVICES:  return "Services: ";
  case TASK_MANAGER_VIEW_TASKS:     return "Tasks: ";
  case TASK_MANAGER_VIEW_PROCESSES: return "Processes: ";
  default:                          return "Items: ";
  }
}

static int task_manager_dl_emit_footer(struct capy_display_list *out,
                                       struct task_manager_app *app,
                                       const struct font *f,
                                       const struct gui_theme_palette *theme,
                                       uint32_t width,
                                       int32_t footer_y) {
  char count_buf[40];
  char tmp[12];
  const char *label = task_manager_dl_view_label(app->view);
  const char *lang = app_current_language();
  const char *btn_label;
  struct system_service_status service;
  int start_action = 0;
  int p = 0;
  int total = task_manager_dl_visible_count(app->view);
  int action_enabled = app->selected >= 0;
  uint32_t btn_w = 64u;
  int32_t btn_x = (int32_t)(width - btn_w - 8u);
  uint32_t btn_bg = action_enabled ? 0x00CC3333u : theme->accent_alt;
  for (int i = 0; label[i] && p < (int)sizeof(count_buf) - 12; i++) count_buf[p++] = label[i];
  if (total < 0) total = 0;
  task_manager_dl_itoa(total, tmp, sizeof(tmp));
  for (int i = 0; tmp[i] && p < (int)sizeof(count_buf) - 1; i++) count_buf[p++] = tmp[i];
  count_buf[p] = '\0';
  if (task_manager_dl_emit_text(out, 8, footer_y + 2, 72u, f, count_buf, theme->text_muted) != 0) return -1;
  if (app_display_list_emit_rect(out, 80, footer_y, 76u, 20u, theme->accent_alt) != 0) return -1;
  if (task_manager_dl_emit_text(out, 88, footer_y + 2, 68u, f,
                                localization_select(lang, "Atualizar", "Refresh", "Actualizar"),
                                theme->text) != 0) return -1;
  if (app->view == TASK_MANAGER_VIEW_SERVICES) {
    start_action = app->selected >= 0 &&
        service_manager_get_at((size_t)app->selected, &service) == 0 &&
        (service.state == SYSTEM_SERVICE_STATE_STOPPED ||
         service.state == SYSTEM_SERVICE_STATE_UNKNOWN);
    btn_label = start_action ? localization_select(lang, "Iniciar", "Start", "Iniciar")
                             : localization_select(lang, "Parar", "Stop", "Parar");
    if (action_enabled && start_action) btn_bg = theme->accent;
  } else {
    btn_label = localization_select(lang, "Matar", "Kill", "Matar");
  }
  if (app_display_list_emit_rect(out, btn_x, footer_y, btn_w, 20u, btn_bg) != 0) return -1;
  return task_manager_dl_emit_text(out, btn_x + 4, footer_y + 2, btn_w - 8u, f,
                                   btn_label,
                                   action_enabled ? 0x00FFFFFFu : theme->text_muted);
}

static int task_manager_emit_display_list(void *producer, struct capy_display_list *out) {
  struct task_manager_app *app = (struct task_manager_app *)producer;
  struct gui_surface *s;
  const struct font *f = font_default();
  const struct gui_theme_palette *theme = compositor_theme();
  int32_t footer_y;
  if (!app || !app->window || !out || !f) return -1;
  s = &app->window->surface;
  footer_y = (int32_t)(s->height - 22);
  out->count = 0u;
  out->text_used = 0u;
  if (app_display_list_emit_rect(out, 0, 0, s->width, s->height, theme->window_bg) != 0) return -1;
  if (task_manager_dl_emit_tabs(out, app, f, theme, s->width) != 0) return -1;
  switch (app->view) {
  case TASK_MANAGER_VIEW_TASKS:
    if (task_manager_dl_emit_header(out, f, theme, s->width, "PID    STATE    PRI       NAME") != 0) return -1;
    if (task_manager_dl_emit_tasks_rows(out, app, f, theme, s->width, footer_y) != 0) return -1;
    break;
  case TASK_MANAGER_VIEW_PROCESSES:
    if (task_manager_dl_emit_header(out, f, theme, s->width, "PID    STATE    UID       NAME") != 0) return -1;
    if (task_manager_dl_emit_processes_rows(out, app, f, theme, s->width, footer_y) != 0) return -1;
    break;
  case TASK_MANAGER_VIEW_SERVICES:
  default:
    if (task_manager_dl_emit_header(out, f, theme, s->width, "ID  STATE      START NAME") != 0) return -1;
    if (task_manager_dl_emit_services_rows(out, app, f, theme, s->width, footer_y) != 0) return -1;
    break;
  }
  return task_manager_dl_emit_footer(out, app, f, theme, s->width, footer_y);
}

int task_manager_render_display_list(struct task_manager_app *app) {
  if (!app || !app->window) return -1;
  task_manager_dl_init_once();
  return app_display_list_bridge_render_window(&g_tm_dl_bridge, app->window,
                                               task_manager_emit_display_list, app);
}
#endif
