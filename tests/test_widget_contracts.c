#include "capy_widget.h"

static int failures;

#define EXPECT(expr)      \
  do {                    \
    if (!(expr)) {        \
      ++failures;         \
      return;             \
    }                     \
  } while (0)

struct test_heap {
  unsigned char storage[8192];
  unsigned used;
  unsigned frees;
};

static void *test_alloc(size_t size, void *user_data) {
  struct test_heap *heap = (struct test_heap *)user_data;
  void *ptr;
  if (!heap || size > sizeof(heap->storage) - heap->used) {
    return 0;
  }
  ptr = heap->storage + heap->used;
  heap->used += (unsigned)size;
  return ptr;
}

static void test_free(void *ptr, void *user_data) {
  struct test_heap *heap = (struct test_heap *)user_data;
  if (heap && ptr) {
    ++heap->frees;
  }
}

static void click_count(struct capy_widget *widget, void *user_data) {
  int *count = (int *)user_data;
  (void)widget;
  ++*count;
}

static void test_create_find_and_click(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *panel;
  struct capy_widget *button;
  struct capy_widget_event event;
  int clicks = 0;
  capy_widget_context_init(&ctx, &allocator);
  panel = capy_widget_create(&ctx, CAPY_WIDGET_PANEL);
  button = capy_widget_create(&ctx, CAPY_WIDGET_BUTTON);
  EXPECT(panel != 0);
  EXPECT(button != 0);
  capy_widget_set_bounds(panel, 0, 0, 100u, 100u);
  capy_widget_set_bounds(button, 10, 10, 20u, 20u);
  capy_widget_set_on_click(button, click_count, &clicks);
  EXPECT(capy_widget_add_child(panel, button) == 0);
  EXPECT(capy_widget_find_at(panel, 11, 11) == button);
  event.type = CAPY_WIDGET_EVENT_POINTER_DOWN;
  event.x = 11;
  event.y = 11;
  event.buttons = 1u;
  event.keycode = 0u;
  EXPECT(capy_widget_handle_event(panel, &event) == 1);
  EXPECT(clicks == 1);
  capy_widget_destroy(panel);
  EXPECT(heap.frees == 2u);
}

static void test_disabled_widget_ignores_input(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *checkbox;
  struct capy_widget_event event;
  capy_widget_context_init(&ctx, &allocator);
  checkbox = capy_widget_create(&ctx, CAPY_WIDGET_CHECKBOX);
  EXPECT(checkbox != 0);
  capy_widget_set_bounds(checkbox, 0, 0, 10u, 10u);
  capy_widget_set_enabled(checkbox, 0);
  event.type = CAPY_WIDGET_EVENT_POINTER_DOWN;
  event.x = 1;
  event.y = 1;
  event.buttons = 1u;
  event.keycode = 0u;
  EXPECT(capy_widget_handle_event(checkbox, &event) == 0);
  EXPECT(checkbox->checked == 0u);
  capy_widget_destroy(checkbox);
}

int main(void) {
  test_create_find_and_click();
  test_disabled_widget_ignores_input();
  return failures == 0 ? 0 : 1;
}
