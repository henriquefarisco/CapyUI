#ifndef CAPY_UI_WIDGET_H
#define CAPY_UI_WIDGET_H

#include <stddef.h>
#include <stdint.h>

#define CAPY_WIDGET_MAX_TEXT 256u
#define CAPY_WIDGET_MAX_CHILDREN 32u

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
};

struct capy_widget_event {
  enum capy_widget_event_type type;
  int32_t x;
  int32_t y;
  uint32_t buttons;
  uint32_t keycode;
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
  struct capy_widget_allocator allocator;
};

struct capy_widget_context {
  uint32_t next_id;
  struct capy_widget_allocator allocator;
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

#endif
