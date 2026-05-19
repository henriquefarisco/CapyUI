#ifndef CAPY_UI_WIDGET_H
#define CAPY_UI_WIDGET_H

#include <stddef.h>
#include <stdint.h>

#include "capy_layout.h"

#define CAPY_WIDGET_MAX_TEXT 256u
#define CAPY_WIDGET_MAX_CHILDREN 32u

/* Focus key codes (since 0.3) */
#define CAPY_KEY_TAB         9u
#define CAPY_KEY_ENTER       13u
#define CAPY_KEY_ESCAPE      27u
#define CAPY_KEY_SPACE       32u
#define CAPY_KEY_ARROW_UP    0x101u
#define CAPY_KEY_ARROW_DOWN  0x102u
#define CAPY_KEY_ARROW_LEFT  0x103u
#define CAPY_KEY_ARROW_RIGHT 0x104u

/* Event modifier flags (since 0.3; expanded in 0.10) */
#define CAPY_MOD_NONE  0x0u
#define CAPY_MOD_CTRL  0x1u
#define CAPY_MOD_ALT   0x2u
#define CAPY_MOD_SHIFT 0x4u
#define CAPY_MOD_META  0x8u

enum capy_widget_type {
  CAPY_WIDGET_NONE = 0,
  CAPY_WIDGET_LABEL,
  CAPY_WIDGET_BUTTON,
  CAPY_WIDGET_TEXTBOX,
  CAPY_WIDGET_CHECKBOX,
  CAPY_WIDGET_LIST,
  CAPY_WIDGET_PANEL,
  CAPY_WIDGET_SCROLLBAR,
  CAPY_WIDGET_MENUBAR,
  CAPY_WIDGET_MENU_ITEM,
  CAPY_WIDGET_DIALOG,
  CAPY_WIDGET_PROGRESS,
  CAPY_WIDGET_TABS
};

enum capy_widget_event_type {
  CAPY_WIDGET_EVENT_NONE = 0,
  CAPY_WIDGET_EVENT_POINTER_MOVE,
  CAPY_WIDGET_EVENT_POINTER_DOWN,
  CAPY_WIDGET_EVENT_POINTER_UP,
  CAPY_WIDGET_EVENT_KEY_DOWN,
  CAPY_WIDGET_EVENT_KEY_UP
};

struct capy_ui_rect {
  int32_t x;
  int32_t y;
  uint32_t width;
  uint32_t height;
};

struct capy_widget_style {
  uint32_t bg_color;
  uint32_t fg_color;
  uint32_t border_color;
  uint32_t hover_color;
  uint32_t active_color;
  uint32_t text_color;
  uint8_t border_width;
  uint8_t padding;
  uint8_t margin;
  uint8_t font_size;
  /* Theme tokens (since 0.6; 0 = use literal *_color above) */
  uint8_t bg_token;
  uint8_t fg_token;
  uint8_t border_token;
  uint8_t reserved;
};

struct capy_widget_event {
  enum capy_widget_event_type type;
  int32_t x;
  int32_t y;
  uint32_t buttons;
  uint32_t keycode;
  uint32_t modifiers;
};

/* Text edit state (since 0.4, valid when type==TEXTBOX) */
struct capy_text_edit {
  uint16_t caret;
  uint16_t sel_start;
  uint16_t sel_end;
  uint8_t multiline;
  uint8_t readonly;
  uint8_t password;
  uint8_t reserved;
};

/* Animation easings (since 0.5; integer-only, no float) */
enum capy_anim_easing {
  CAPY_ANIM_LINEAR = 0,
  CAPY_ANIM_EASE_IN,
  CAPY_ANIM_EASE_OUT,
  CAPY_ANIM_EASE_IN_OUT
};

/* Animation state (since 0.5; deterministic ticks, no float, no clock) */
struct capy_anim {
  uint32_t start_tick;
  uint32_t duration_ticks;
  int32_t from;
  int32_t to;
  uint8_t easing;
  uint8_t active;
  uint16_t reserved;
};

/* Theme tokens (since 0.6) */
enum capy_token {
  CAPY_TOKEN_NONE = 0,
  CAPY_TOKEN_BG_BASE,
  CAPY_TOKEN_BG_RAISED,
  CAPY_TOKEN_BG_SUNKEN,
  CAPY_TOKEN_FG_PRIMARY,
  CAPY_TOKEN_FG_MUTED,
  CAPY_TOKEN_FG_INVERSE,
  CAPY_TOKEN_ACCENT,
  CAPY_TOKEN_ACCENT_HOVER,
  CAPY_TOKEN_BORDER,
  CAPY_TOKEN_BORDER_FOCUS,
  CAPY_TOKEN_FOCUS_RING,
  CAPY_TOKEN_DANGER,
  CAPY_TOKEN_WARNING,
  CAPY_TOKEN_SUCCESS,
  CAPY_TOKEN_INFO,
  CAPY_TOKEN_DISABLED,
  CAPY_TOKEN_COUNT
};

#define CAPY_THEME_VARIANT_LIGHT          0u
#define CAPY_THEME_VARIANT_DARK           1u
#define CAPY_THEME_VARIANT_HIGH_CONTRAST  2u

/* Theme state (since 0.6; standalone struct stored in widget context) */
struct capy_theme {
  uint32_t tokens[CAPY_TOKEN_COUNT];
  uint8_t variant;
  uint8_t high_contrast;
  uint16_t version;
};

struct capy_widget;

typedef void (*capy_widget_callback)(struct capy_widget *widget,
                                     void *user_data);
typedef void *(*capy_widget_alloc_fn)(size_t size, void *user_data);
typedef void (*capy_widget_free_fn)(void *ptr, void *user_data);

struct capy_widget_allocator {
  capy_widget_alloc_fn alloc;
  capy_widget_free_fn free;
  void *user_data;
};

struct capy_widget {
  uint32_t id;
  enum capy_widget_type type;
  struct capy_ui_rect bounds;
  struct capy_widget_style style;
  char text[CAPY_WIDGET_MAX_TEXT];
  uint8_t visible;
  uint8_t enabled;
  uint8_t focused;
  uint8_t hovered;
  uint8_t checked;
  int32_t value;
  int32_t min_value;
  int32_t max_value;
  struct capy_widget *parent;
  struct capy_widget *children[CAPY_WIDGET_MAX_CHILDREN];
  uint32_t child_count;
  capy_widget_callback on_click;
  capy_widget_callback on_change;
  capy_widget_callback on_submit;
  void *user_data;
  struct capy_layout_node layout;
  struct capy_widget_allocator allocator;
  uint8_t focusable;
  int16_t tab_index;
  struct capy_text_edit text_edit;
};

struct capy_widget_context {
  uint32_t next_id;
  struct capy_widget_allocator allocator;
  struct capy_theme theme;
};

void capy_widget_context_init(struct capy_widget_context *ctx,
                              const struct capy_widget_allocator *allocator);
struct capy_widget *capy_widget_create(struct capy_widget_context *ctx,
                                       enum capy_widget_type type);
void capy_widget_destroy(struct capy_widget *widget);
void capy_widget_set_bounds(struct capy_widget *widget, int32_t x, int32_t y,
                            uint32_t width, uint32_t height);
void capy_widget_set_text(struct capy_widget *widget, const char *text);
void capy_widget_set_visible(struct capy_widget *widget, int visible);
void capy_widget_set_enabled(struct capy_widget *widget, int enabled);
void capy_widget_set_style(struct capy_widget *widget,
                           const struct capy_widget_style *style);
int capy_widget_add_child(struct capy_widget *parent,
                          struct capy_widget *child);
int capy_widget_remove_child(struct capy_widget *parent,
                             struct capy_widget *child);
int capy_widget_handle_event(struct capy_widget *widget,
                             const struct capy_widget_event *event);
struct capy_widget *capy_widget_find_at(struct capy_widget *root, int32_t x,
                                        int32_t y);
void capy_widget_focus(struct capy_widget *widget);
void capy_widget_set_on_click(struct capy_widget *widget,
                              capy_widget_callback callback, void *user_data);
void capy_widget_set_on_change(struct capy_widget *widget,
                               capy_widget_callback callback, void *user_data);
struct capy_widget_style capy_widget_default_style(void);
struct capy_widget_style capy_widget_button_style(void);
struct capy_widget_style capy_widget_textbox_style(void);

/* Focus traversal (since 0.3) */
void capy_widget_set_focusable(struct capy_widget *widget, int focusable,
                               int16_t tab_index);
struct capy_widget *capy_widget_focus_next(struct capy_widget *root,
                                           struct capy_widget *current);
struct capy_widget *capy_widget_focus_prev(struct capy_widget *root,
                                           struct capy_widget *current);
void capy_widget_clear_focus(struct capy_widget *root);
struct capy_widget *capy_widget_get_focused(struct capy_widget *root);
int capy_widget_dispatch_key(struct capy_widget *root,
                             const struct capy_widget_event *event);

/* Text editing (since 0.4) */
int capy_textbox_insert(struct capy_widget *widget, const char *utf8,
                        uint16_t len);
int capy_textbox_delete(struct capy_widget *widget, int16_t count);
void capy_textbox_set_selection(struct capy_widget *widget, uint16_t start,
                                uint16_t end);
int capy_textbox_copy(struct capy_widget *widget, char *out, uint16_t out_cap);
int capy_widget_ime_compose(struct capy_widget *widget, const char *preedit,
                            uint16_t len);

/* Animation (since 0.5) */
void capy_anim_start(struct capy_anim *a, uint32_t now, uint32_t duration,
                     int32_t from, int32_t to, uint8_t easing);
int capy_anim_sample(const struct capy_anim *a, uint32_t now, int32_t *out);
void capy_widget_tick(struct capy_widget *root, uint32_t now);

/* Theme (since 0.6) */
struct capy_theme capy_theme_default_light(void);
struct capy_theme capy_theme_default_dark(void);
struct capy_theme capy_theme_high_contrast(void);
void capy_theme_apply(struct capy_widget_context *ctx,
                      const struct capy_theme *theme);
uint32_t capy_theme_resolve(const struct capy_theme *theme, uint8_t token,
                            uint32_t fallback);

#endif
