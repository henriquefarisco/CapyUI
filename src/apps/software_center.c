#include "apps/software_center.h"
#include "gui/compositor.h"
#include "gui/font.h"
#include "lang/app_language.h"
#include "lang/localization.h"
#include "memory/kmem.h"
#include "util/kstring.h"

#include <stddef.h>
#include <stdint.h>

struct software_center_state {
  struct gui_window *window;
  struct software_center_package packages[SOFTWARE_CENTER_PACKAGE_MAX];
  size_t count;
  int selected;
  char status[96];
};

static struct software_center_backend g_backend;
static struct software_center_state g_center;

static const char *sc_t(const char *pt, const char *en, const char *es) {
  return localization_select(app_current_language(), pt, en, es);
}

static void sc_fill(struct gui_surface *s, int32_t x, int32_t y,
                    uint32_t width, uint32_t height, uint32_t color) {
  for (uint32_t by = 0u; by < height; ++by) {
    int32_t py = y + (int32_t)by;
    if (py < 0 || (uint32_t)py >= s->height) continue;
    uint32_t *line = (uint32_t *)((uint8_t *)s->pixels + (uint32_t)py * s->pitch);
    for (uint32_t bx = 0u; bx < width; ++bx) {
      int32_t px = x + (int32_t)bx;
      if (px >= 0 && (uint32_t)px < s->width) line[px] = color;
    }
  }
}

static void sc_reload(int fetch) {
  g_center.count = 0u;
  g_center.selected = -1;
  if (!g_backend.count || !g_backend.get || (fetch &&
      (!g_backend.refresh || g_backend.refresh() != 0))) {
    kstrcpy(g_center.status, sizeof(g_center.status),
            sc_t("Falha ao sincronizar catalogo", "Catalog sync failed",
                 "Fallo al sincronizar catalogo"));
    return;
  }
  size_t total = g_backend.count();
  if (total > SOFTWARE_CENTER_PACKAGE_MAX) total = SOFTWARE_CENTER_PACKAGE_MAX;
  for (size_t i = 0u; i < total; ++i) {
    if (g_backend.get(i, &g_center.packages[g_center.count]) == 0) {
      ++g_center.count;
    }
  }
  kstrcpy(g_center.status, sizeof(g_center.status),
          sc_t("Catalogo verificado", "Verified catalog", "Catalogo verificado"));
}

static void sc_paint(struct gui_window *win) {
  const struct gui_theme_palette *theme = compositor_theme();
  const struct font *font = font_default();
  struct gui_surface *s;
  if (!win || !font) return;
  s = &win->surface;
  sc_fill(s, 0, 0, s->width, s->height, theme->window_bg);
  sc_fill(s, 8, 8, 104, 26, theme->accent_alt);
  font_draw_string(s, font, 18, 14,
                   sc_t("Atualizar", "Refresh", "Actualizar"), theme->text);
  for (size_t i = 0u; i < g_center.count; ++i) {
    int32_t y = 46 + (int32_t)i * 34;
    const struct software_center_package *pkg = &g_center.packages[i];
    if ((int)i == g_center.selected) {
      sc_fill(s, 8, y - 3, s->width - 16u, 31, theme->accent_alt);
    }
    font_draw_string(s, font, 14, y, pkg->name, theme->text);
    font_draw_string(s, font, (int32_t)s->width - 92, y,
                     pkg->installed ? sc_t("Instalado", "Installed", "Instalado")
                                    : pkg->version,
                     pkg->installed ? theme->accent : theme->text_muted);
  }
  if (g_center.selected >= 0) {
    sc_fill(s, 8, (int32_t)s->height - 58, 120, 26, theme->accent_alt);
    font_draw_string(s, font, 16, (int32_t)s->height - 52,
      g_center.packages[g_center.selected].installed
        ? sc_t("Remover", "Remove", "Eliminar")
        : sc_t("Instalar", "Install", "Instalar"), theme->text);
  }
  font_draw_string(s, font, 8, (int32_t)s->height - 22,
                   g_center.status, theme->text_muted);
}

static void sc_close(struct gui_window *win) {
  (void)win;
  g_center.window = NULL;
  g_center.count = 0u;
  g_center.selected = -1;
}

static void sc_mouse(struct gui_window *win, int32_t x, int32_t y,
                     uint8_t buttons) {
  int rc = -1;
  if (!win || !(buttons & 1u)) return;
  if (x >= 8 && x < 112 && y >= 8 && y < 34) {
    sc_reload(1);
  } else if (y >= 43 && y < 43 + (int32_t)g_center.count * 34) {
    g_center.selected = (y - 43) / 34;
  } else if (g_center.selected >= 0 && x >= 8 && x < 128 &&
             y >= (int32_t)win->surface.height - 58 &&
             y < (int32_t)win->surface.height - 32) {
    struct software_center_package *pkg = &g_center.packages[g_center.selected];
    if (pkg->installed && g_backend.remove) rc = g_backend.remove(pkg->name);
    if (!pkg->installed && g_backend.install) rc = g_backend.install(pkg->name);
    kstrcpy(g_center.status, sizeof(g_center.status), rc == 0
      ? sc_t("Operacao concluida", "Operation completed", "Operacion completada")
      : sc_t("Operacao recusada", "Operation refused", "Operacion rechazada"));
    sc_reload(0);
  }
  compositor_invalidate(win->id);
}

void software_center_set_backend(const struct software_center_backend *backend) {
  kmemzero(&g_backend, sizeof(g_backend));
  if (backend) g_backend = *backend;
}

void software_center_open(void) {
  if (g_center.window) {
    compositor_show_window(g_center.window->id);
    compositor_focus_window(g_center.window->id);
    return;
  }
  kmemzero(&g_center, sizeof(g_center));
  g_center.selected = -1;
  g_center.window = compositor_create_window("Software Center", 110, 70, 580, 410);
  if (!g_center.window) return;
  g_center.window->user_data = &g_center;
  g_center.window->on_paint = sc_paint;
  g_center.window->on_mouse = sc_mouse;
  g_center.window->on_close = sc_close;
  sc_reload(1);
  compositor_show_window(g_center.window->id);
  compositor_focus_window(g_center.window->id);
}

static int sc_smoke_refresh(void) { return 0; }
static size_t sc_smoke_count(void) { return 1u; }
static int sc_smoke_get(size_t index, struct software_center_package *out) {
  if (index != 0u || !out) return -1;
  kmemzero(out, sizeof(*out));
  kstrcpy(out->name, sizeof(out->name), "org.capyos.test");
  kstrcpy(out->version, sizeof(out->version), "1.0.0");
  return 0;
}
static int sc_smoke_op(const char *name) {
  return name && name[0] ? 0 : -1;
}

int software_center_smoke_roundtrip(void) {
  struct software_center_backend saved = g_backend;
  struct software_center_backend smoke = {
    sc_smoke_refresh, sc_smoke_count, sc_smoke_get, sc_smoke_op, sc_smoke_op
  };
  software_center_set_backend(&smoke);
  sc_reload(1);
  int ok = g_center.count == 1u &&
           sc_smoke_op(g_center.packages[0].name) == 0;
  software_center_set_backend(&saved);
  return ok ? 0 : 1;
}
