#include "capy_widget.h"
#include "capy_display_list.h"
#include "capy_dl_gpu.h"

#include <stdio.h>

static int failures;

#define EXPECT(expr)      \
  do {                    \
    if (!(expr)) {        \
      fprintf(stderr, "EXPECT failed at %s:%d: %s\n", __FILE__, __LINE__, #expr); \
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

static void zero_mem(void *ptr, size_t n) {
  unsigned char *p = (unsigned char *)ptr;
  while (n--) {
    *p++ = 0u;
  }
}

static void test_layout_stack_equal_grow(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *panel;
  struct capy_widget *a;
  struct capy_widget *b;
  struct capy_widget *c;
  struct capy_layout_node ln;
  capy_widget_context_init(&ctx, &allocator);
  panel = capy_widget_create(&ctx, CAPY_WIDGET_PANEL);
  a = capy_widget_create(&ctx, CAPY_WIDGET_PANEL);
  b = capy_widget_create(&ctx, CAPY_WIDGET_PANEL);
  c = capy_widget_create(&ctx, CAPY_WIDGET_PANEL);
  EXPECT(panel != 0);
  EXPECT(a != 0);
  EXPECT(b != 0);
  EXPECT(c != 0);
  capy_widget_set_bounds(panel, 0, 0, 100u, 90u);
  EXPECT(capy_widget_add_child(panel, a) == 0);
  EXPECT(capy_widget_add_child(panel, b) == 0);
  EXPECT(capy_widget_add_child(panel, c) == 0);
  zero_mem(&ln, sizeof(ln));
  ln.kind = CAPY_LAYOUT_STACK;
  ln.c.axis = 1u;
  capy_widget_set_layout(panel, &ln);
  a->layout.grow = 1u;
  b->layout.grow = 1u;
  c->layout.grow = 1u;
  EXPECT(capy_widget_arrange(panel) == 0);
  EXPECT(a->bounds.height == 30u);
  EXPECT(b->bounds.height == 30u);
  EXPECT(c->bounds.height == 30u);
  EXPECT(a->bounds.y == 0);
  EXPECT(b->bounds.y == 30);
  EXPECT(c->bounds.y == 60);
  EXPECT(a->bounds.width == 100u);
  capy_widget_destroy(panel);
}

static void test_layout_grid_2x2(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *root;
  struct capy_widget *ch[4];
  struct capy_layout_node ln;
  int i;
  capy_widget_context_init(&ctx, &allocator);
  root = capy_widget_create(&ctx, CAPY_WIDGET_PANEL);
  EXPECT(root != 0);
  capy_widget_set_bounds(root, 0, 0, 60u, 60u);
  for (i = 0; i < 4; ++i) {
    ch[i] = capy_widget_create(&ctx, CAPY_WIDGET_PANEL);
    EXPECT(ch[i] != 0);
    EXPECT(capy_widget_add_child(root, ch[i]) == 0);
  }
  zero_mem(&ln, sizeof(ln));
  ln.kind = CAPY_LAYOUT_GRID;
  ln.c.cols = 2u;
  ln.c.gap = 4u;
  ln.c.pad_l = 8u;
  ln.c.pad_t = 8u;
  ln.c.pad_r = 8u;
  ln.c.pad_b = 8u;
  capy_widget_set_layout(root, &ln);
  EXPECT(capy_widget_arrange(root) == 0);
  EXPECT(ch[0]->bounds.width == 20u);
  EXPECT(ch[0]->bounds.height == 20u);
  EXPECT(ch[0]->bounds.x == 8);
  EXPECT(ch[0]->bounds.y == 8);
  EXPECT(ch[1]->bounds.x == 32);
  EXPECT(ch[1]->bounds.y == 8);
  EXPECT(ch[2]->bounds.x == 8);
  EXPECT(ch[2]->bounds.y == 32);
  EXPECT(ch[3]->bounds.x == 32);
  EXPECT(ch[3]->bounds.y == 32);
  capy_widget_destroy(root);
}

static void test_layout_idempotent(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *root;
  struct capy_widget *a;
  struct capy_widget *b;
  struct capy_layout_node ln;
  int32_t x_a;
  int32_t y_a;
  uint32_t w_a;
  uint32_t h_a;
  int32_t x_b;
  int32_t y_b;
  uint32_t w_b;
  uint32_t h_b;
  capy_widget_context_init(&ctx, &allocator);
  root = capy_widget_create(&ctx, CAPY_WIDGET_PANEL);
  a = capy_widget_create(&ctx, CAPY_WIDGET_PANEL);
  b = capy_widget_create(&ctx, CAPY_WIDGET_PANEL);
  EXPECT(root != 0);
  EXPECT(a != 0);
  EXPECT(b != 0);
  capy_widget_set_bounds(root, 0, 0, 80u, 40u);
  EXPECT(capy_widget_add_child(root, a) == 0);
  EXPECT(capy_widget_add_child(root, b) == 0);
  zero_mem(&ln, sizeof(ln));
  ln.kind = CAPY_LAYOUT_STACK;
  ln.c.axis = 0u;
  ln.c.gap = 2u;
  capy_widget_set_layout(root, &ln);
  a->layout.grow = 1u;
  b->layout.grow = 1u;
  EXPECT(capy_widget_arrange(root) == 0);
  x_a = a->bounds.x;
  y_a = a->bounds.y;
  w_a = a->bounds.width;
  h_a = a->bounds.height;
  x_b = b->bounds.x;
  y_b = b->bounds.y;
  w_b = b->bounds.width;
  h_b = b->bounds.height;
  EXPECT(capy_widget_arrange(root) == 0);
  EXPECT(a->bounds.x == x_a);
  EXPECT(a->bounds.y == y_a);
  EXPECT(a->bounds.width == w_a);
  EXPECT(a->bounds.height == h_a);
  EXPECT(b->bounds.x == x_b);
  EXPECT(b->bounds.y == y_b);
  EXPECT(b->bounds.width == w_b);
  EXPECT(b->bounds.height == h_b);
  capy_widget_destroy(root);
}

static void test_layout_min_max_constraints(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *w;
  struct capy_layout_node ln;
  capy_widget_context_init(&ctx, &allocator);
  w = capy_widget_create(&ctx, CAPY_WIDGET_PANEL);
  EXPECT(w != 0);
  zero_mem(&ln, sizeof(ln));
  ln.kind = CAPY_LAYOUT_NONE;
  ln.c.min_w = 50u;
  ln.c.max_w = 100u;
  ln.c.min_h = 20u;
  ln.c.max_h = 60u;
  capy_widget_set_layout(w, &ln);
  EXPECT(capy_widget_measure(w, 10u, 10u) == 0);
  EXPECT(w->bounds.width == 50u);
  EXPECT(w->bounds.height == 20u);
  EXPECT(capy_widget_measure(w, 75u, 40u) == 0);
  EXPECT(w->bounds.width == 75u);
  EXPECT(w->bounds.height == 40u);
  EXPECT(capy_widget_measure(w, 200u, 200u) == 0);
  EXPECT(w->bounds.width == 100u);
  EXPECT(w->bounds.height == 60u);
  capy_widget_destroy(w);
}

static void test_displaylist_emit_simple_tree(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *panel;
  struct capy_widget *button;
  struct capy_dl_cmd cmd_buf[32];
  char text_buf[64];
  struct capy_display_list dl;
  capy_widget_context_init(&ctx, &allocator);
  panel = capy_widget_create(&ctx, CAPY_WIDGET_PANEL);
  button = capy_widget_create(&ctx, CAPY_WIDGET_BUTTON);
  EXPECT(panel != 0);
  EXPECT(button != 0);
  capy_widget_set_bounds(panel, 0, 0, 100u, 100u);
  capy_widget_set_bounds(button, 10, 10, 20u, 20u);
  EXPECT(capy_widget_add_child(panel, button) == 0);
  capy_display_list_init(&dl, cmd_buf, 32u, text_buf, 64u);
  EXPECT(dl.version == CAPY_DISPLAY_LIST_SCHEMA_VERSION);
  EXPECT(capy_widget_emit(panel, &dl) == 0);
  EXPECT(dl.count == 8u);
  EXPECT(dl.cmds[0].op == CAPY_DL_CLIP_PUSH);
  EXPECT(dl.cmds[1].op == CAPY_DL_RECT);
  EXPECT(dl.cmds[2].op == CAPY_DL_BORDER);
  EXPECT(dl.cmds[3].op == CAPY_DL_CLIP_PUSH);
  EXPECT(dl.cmds[4].op == CAPY_DL_RECT);
  EXPECT(dl.cmds[5].op == CAPY_DL_BORDER);
  EXPECT(dl.cmds[6].op == CAPY_DL_CLIP_POP);
  EXPECT(dl.cmds[7].op == CAPY_DL_CLIP_POP);
  EXPECT(dl.cmds[0].rect.width == 100u);
  EXPECT(dl.cmds[3].rect.width == 20u);
  capy_widget_destroy(panel);
}

static void test_displaylist_clip_balance(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *root;
  struct capy_widget *a;
  struct capy_widget *b;
  struct capy_dl_cmd cmd_buf[64];
  char text_buf[64];
  struct capy_display_list dl;
  uint32_t pushes = 0u;
  uint32_t pops = 0u;
  uint32_t i;
  capy_widget_context_init(&ctx, &allocator);
  root = capy_widget_create(&ctx, CAPY_WIDGET_PANEL);
  a = capy_widget_create(&ctx, CAPY_WIDGET_BUTTON);
  b = capy_widget_create(&ctx, CAPY_WIDGET_LABEL);
  EXPECT(root != 0);
  EXPECT(a != 0);
  EXPECT(b != 0);
  capy_widget_set_bounds(root, 0, 0, 200u, 200u);
  capy_widget_set_bounds(a, 0, 0, 50u, 30u);
  capy_widget_set_bounds(b, 60, 0, 80u, 30u);
  EXPECT(capy_widget_add_child(root, a) == 0);
  EXPECT(capy_widget_add_child(root, b) == 0);
  capy_display_list_init(&dl, cmd_buf, 64u, text_buf, 64u);
  EXPECT(capy_widget_emit(root, &dl) == 0);
  for (i = 0u; i < dl.count; ++i) {
    if (dl.cmds[i].op == CAPY_DL_CLIP_PUSH) {
      ++pushes;
    }
    if (dl.cmds[i].op == CAPY_DL_CLIP_POP) {
      ++pops;
    }
  }
  EXPECT(pushes == pops);
  EXPECT(pushes == 3u);
  capy_widget_destroy(root);
}

static void test_displaylist_overflow_rollback(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *panel;
  struct capy_dl_cmd cmd_buf[2];
  char text_buf[16];
  struct capy_display_list dl;
  capy_widget_context_init(&ctx, &allocator);
  panel = capy_widget_create(&ctx, CAPY_WIDGET_PANEL);
  EXPECT(panel != 0);
  capy_widget_set_bounds(panel, 0, 0, 100u, 100u);
  capy_display_list_init(&dl, cmd_buf, 2u, text_buf, 16u);
  EXPECT(capy_widget_emit(panel, &dl) == -1);
  EXPECT(dl.count == 0u);
  EXPECT(dl.text_used == 0u);
  EXPECT(dl.capacity == 2u);
  capy_widget_destroy(panel);
}

static void test_displaylist_reset_and_text(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *label;
  struct capy_dl_cmd cmd_buf[16];
  char text_buf[64];
  struct capy_display_list dl;
  uint32_t i;
  uint32_t text_idx = 0u;
  int found_text = 0;
  capy_widget_context_init(&ctx, &allocator);
  label = capy_widget_create(&ctx, CAPY_WIDGET_LABEL);
  EXPECT(label != 0);
  capy_widget_set_bounds(label, 0, 0, 100u, 30u);
  capy_widget_set_text(label, "Hello");
  capy_display_list_init(&dl, cmd_buf, 16u, text_buf, 64u);
  EXPECT(capy_widget_emit(label, &dl) == 0);
  for (i = 0u; i < dl.count; ++i) {
    if (dl.cmds[i].op == CAPY_DL_TEXT) {
      text_idx = i;
      found_text = 1;
      break;
    }
  }
  EXPECT(found_text == 1);
  EXPECT(dl.cmds[text_idx].text_offset == 0u);
  EXPECT(dl.cmds[text_idx].text_len == 5u);
  EXPECT(dl.text_pool[0] == 'H');
  EXPECT(dl.text_pool[4] == 'o');
  EXPECT(dl.text_used == 5u);
  capy_display_list_reset(&dl);
  EXPECT(dl.count == 0u);
  EXPECT(dl.text_used == 0u);
  EXPECT(dl.capacity == 16u);
  EXPECT(dl.text_capacity == 64u);
  EXPECT(dl.version == CAPY_DISPLAY_LIST_SCHEMA_VERSION);
  EXPECT(capy_widget_emit(label, &dl) == 0);
  EXPECT(dl.count > 0u);
  capy_widget_destroy(label);
}

static void test_focus_defaults_and_set(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *btn;
  struct capy_widget *lbl;
  struct capy_widget *panel;
  capy_widget_context_init(&ctx, &allocator);
  btn = capy_widget_create(&ctx, CAPY_WIDGET_BUTTON);
  lbl = capy_widget_create(&ctx, CAPY_WIDGET_LABEL);
  panel = capy_widget_create(&ctx, CAPY_WIDGET_PANEL);
  EXPECT(btn != 0);
  EXPECT(lbl != 0);
  EXPECT(panel != 0);
  EXPECT(btn->focusable == 1u);
  EXPECT(lbl->focusable == 0u);
  EXPECT(panel->focusable == 0u);
  capy_widget_set_focusable(lbl, 1, 5);
  EXPECT(lbl->focusable == 1u);
  EXPECT(lbl->tab_index == 5);
  capy_widget_set_focusable(btn, 0, 0);
  EXPECT(btn->focusable == 0u);
  capy_widget_destroy(btn);
  capy_widget_destroy(lbl);
  capy_widget_destroy(panel);
}

static void test_focus_traversal_order(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *root;
  struct capy_widget *a;
  struct capy_widget *b;
  struct capy_widget *c;
  struct capy_widget *focused;
  capy_widget_context_init(&ctx, &allocator);
  root = capy_widget_create(&ctx, CAPY_WIDGET_PANEL);
  a = capy_widget_create(&ctx, CAPY_WIDGET_BUTTON);
  b = capy_widget_create(&ctx, CAPY_WIDGET_BUTTON);
  c = capy_widget_create(&ctx, CAPY_WIDGET_BUTTON);
  EXPECT(root != 0);
  EXPECT(a != 0);
  EXPECT(b != 0);
  EXPECT(c != 0);
  EXPECT(capy_widget_add_child(root, a) == 0);
  EXPECT(capy_widget_add_child(root, b) == 0);
  EXPECT(capy_widget_add_child(root, c) == 0);
  focused = capy_widget_focus_next(root, 0);
  EXPECT(focused == a);
  focused = capy_widget_focus_next(root, a);
  EXPECT(focused == b);
  focused = capy_widget_focus_next(root, b);
  EXPECT(focused == c);
  focused = capy_widget_focus_next(root, c);
  EXPECT(focused == a);
  capy_widget_destroy(root);
}

static void test_focus_tab_index_priority(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *root;
  struct capy_widget *a;
  struct capy_widget *b;
  struct capy_widget *c;
  struct capy_widget *focused;
  capy_widget_context_init(&ctx, &allocator);
  root = capy_widget_create(&ctx, CAPY_WIDGET_PANEL);
  a = capy_widget_create(&ctx, CAPY_WIDGET_BUTTON);
  b = capy_widget_create(&ctx, CAPY_WIDGET_BUTTON);
  c = capy_widget_create(&ctx, CAPY_WIDGET_BUTTON);
  EXPECT(root != 0);
  EXPECT(a != 0);
  EXPECT(b != 0);
  EXPECT(c != 0);
  capy_widget_set_focusable(a, 1, 2);
  capy_widget_set_focusable(b, 1, 3);
  capy_widget_set_focusable(c, 1, 1);
  EXPECT(capy_widget_add_child(root, a) == 0);
  EXPECT(capy_widget_add_child(root, b) == 0);
  EXPECT(capy_widget_add_child(root, c) == 0);
  focused = capy_widget_focus_next(root, 0);
  EXPECT(focused == c);
  focused = capy_widget_focus_next(root, c);
  EXPECT(focused == a);
  focused = capy_widget_focus_next(root, a);
  EXPECT(focused == b);
  focused = capy_widget_focus_next(root, b);
  EXPECT(focused == c);
  capy_widget_destroy(root);
}

static void test_focus_skip_non_traversable(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *root;
  struct capy_widget *a;
  struct capy_widget *b;
  struct capy_widget *c;
  struct capy_widget *focused;
  capy_widget_context_init(&ctx, &allocator);
  root = capy_widget_create(&ctx, CAPY_WIDGET_PANEL);
  a = capy_widget_create(&ctx, CAPY_WIDGET_BUTTON);
  b = capy_widget_create(&ctx, CAPY_WIDGET_BUTTON);
  c = capy_widget_create(&ctx, CAPY_WIDGET_BUTTON);
  EXPECT(root != 0);
  EXPECT(a != 0);
  EXPECT(b != 0);
  EXPECT(c != 0);
  capy_widget_set_enabled(b, 0);
  EXPECT(capy_widget_add_child(root, a) == 0);
  EXPECT(capy_widget_add_child(root, b) == 0);
  EXPECT(capy_widget_add_child(root, c) == 0);
  focused = capy_widget_focus_next(root, a);
  EXPECT(focused == c);
  focused = capy_widget_focus_next(root, c);
  EXPECT(focused == a);
  capy_widget_destroy(root);
}

static void test_focus_prev_wrap(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *root;
  struct capy_widget *a;
  struct capy_widget *b;
  struct capy_widget *focused;
  capy_widget_context_init(&ctx, &allocator);
  root = capy_widget_create(&ctx, CAPY_WIDGET_PANEL);
  a = capy_widget_create(&ctx, CAPY_WIDGET_BUTTON);
  b = capy_widget_create(&ctx, CAPY_WIDGET_BUTTON);
  EXPECT(root != 0);
  EXPECT(a != 0);
  EXPECT(b != 0);
  EXPECT(capy_widget_add_child(root, a) == 0);
  EXPECT(capy_widget_add_child(root, b) == 0);
  focused = capy_widget_focus_prev(root, a);
  EXPECT(focused == b);
  focused = capy_widget_focus_prev(root, b);
  EXPECT(focused == a);
  capy_widget_destroy(root);
}

static void test_dispatch_key_tab_cycles(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *root;
  struct capy_widget *a;
  struct capy_widget *b;
  struct capy_widget_event ev;
  capy_widget_context_init(&ctx, &allocator);
  root = capy_widget_create(&ctx, CAPY_WIDGET_PANEL);
  a = capy_widget_create(&ctx, CAPY_WIDGET_BUTTON);
  b = capy_widget_create(&ctx, CAPY_WIDGET_BUTTON);
  EXPECT(root != 0);
  EXPECT(a != 0);
  EXPECT(b != 0);
  EXPECT(capy_widget_add_child(root, a) == 0);
  EXPECT(capy_widget_add_child(root, b) == 0);
  ev.type = CAPY_WIDGET_EVENT_KEY_DOWN;
  ev.x = 0;
  ev.y = 0;
  ev.buttons = 0u;
  ev.keycode = CAPY_KEY_TAB;
  ev.modifiers = CAPY_MOD_NONE;
  EXPECT(capy_widget_dispatch_key(root, &ev) == 1);
  EXPECT(a->focused == 1u);
  EXPECT(capy_widget_dispatch_key(root, &ev) == 1);
  EXPECT(a->focused == 0u);
  EXPECT(b->focused == 1u);
  EXPECT(capy_widget_dispatch_key(root, &ev) == 1);
  EXPECT(b->focused == 0u);
  EXPECT(a->focused == 1u);
  ev.modifiers = CAPY_MOD_SHIFT;
  EXPECT(capy_widget_dispatch_key(root, &ev) == 1);
  EXPECT(a->focused == 0u);
  EXPECT(b->focused == 1u);
  capy_widget_destroy(root);
}

static void test_dispatch_key_enter_button(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *btn;
  struct capy_widget_event ev;
  int clicks = 0;
  capy_widget_context_init(&ctx, &allocator);
  btn = capy_widget_create(&ctx, CAPY_WIDGET_BUTTON);
  EXPECT(btn != 0);
  capy_widget_focus(btn);
  capy_widget_set_on_click(btn, click_count, &clicks);
  ev.type = CAPY_WIDGET_EVENT_KEY_DOWN;
  ev.x = 0;
  ev.y = 0;
  ev.buttons = 0u;
  ev.keycode = CAPY_KEY_ENTER;
  ev.modifiers = CAPY_MOD_NONE;
  EXPECT(capy_widget_dispatch_key(btn, &ev) == 1);
  EXPECT(clicks == 1);
  ev.keycode = CAPY_KEY_SPACE;
  EXPECT(capy_widget_dispatch_key(btn, &ev) == 1);
  EXPECT(clicks == 2);
  capy_widget_destroy(btn);
}

static void test_dispatch_key_space_checkbox(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *cb;
  struct capy_widget_event ev;
  int changes = 0;
  capy_widget_context_init(&ctx, &allocator);
  cb = capy_widget_create(&ctx, CAPY_WIDGET_CHECKBOX);
  EXPECT(cb != 0);
  capy_widget_focus(cb);
  capy_widget_set_on_change(cb, click_count, &changes);
  ev.type = CAPY_WIDGET_EVENT_KEY_DOWN;
  ev.x = 0;
  ev.y = 0;
  ev.buttons = 0u;
  ev.keycode = CAPY_KEY_SPACE;
  ev.modifiers = CAPY_MOD_NONE;
  EXPECT(cb->checked == 0u);
  EXPECT(capy_widget_dispatch_key(cb, &ev) == 1);
  EXPECT(cb->checked == 1u);
  EXPECT(changes == 1);
  EXPECT(capy_widget_dispatch_key(cb, &ev) == 1);
  EXPECT(cb->checked == 0u);
  EXPECT(changes == 2);
  capy_widget_destroy(cb);
}

static void test_dispatch_key_escape_dialog(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *dlg;
  struct capy_widget_event ev;
  int cancels = 0;
  capy_widget_context_init(&ctx, &allocator);
  dlg = capy_widget_create(&ctx, CAPY_WIDGET_DIALOG);
  EXPECT(dlg != 0);
  capy_widget_focus(dlg);
  capy_widget_set_on_click(dlg, click_count, &cancels);
  ev.type = CAPY_WIDGET_EVENT_KEY_DOWN;
  ev.x = 0;
  ev.y = 0;
  ev.buttons = 0u;
  ev.keycode = CAPY_KEY_ESCAPE;
  ev.modifiers = CAPY_MOD_NONE;
  EXPECT(capy_widget_dispatch_key(dlg, &ev) == 1);
  EXPECT(cancels == 1);
  capy_widget_destroy(dlg);
}

static void test_displaylist_focus_ring(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *btn;
  struct capy_dl_cmd cmd_buf[16];
  char text_buf[32];
  struct capy_display_list dl;
  uint32_t i;
  int found_ring = 0;
  capy_widget_context_init(&ctx, &allocator);
  btn = capy_widget_create(&ctx, CAPY_WIDGET_BUTTON);
  EXPECT(btn != 0);
  capy_widget_set_bounds(btn, 0, 0, 50u, 20u);
  capy_widget_focus(btn);
  capy_display_list_init(&dl, cmd_buf, 16u, text_buf, 32u);
  EXPECT(dl.version == CAPY_DISPLAY_LIST_SCHEMA_VERSION);
  EXPECT(capy_widget_emit(btn, &dl) == 0);
  for (i = 0u; i < dl.count; ++i) {
    if (dl.cmds[i].op == CAPY_DL_FOCUS_RING) {
      found_ring = 1;
      break;
    }
  }
  EXPECT(found_ring == 1);
  capy_widget_destroy(btn);
}

static void test_text_edit_insert_basic(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *tb;
  capy_widget_context_init(&ctx, &allocator);
  tb = capy_widget_create(&ctx, CAPY_WIDGET_TEXTBOX);
  EXPECT(tb != 0);
  EXPECT(capy_textbox_insert(tb, "Hello", 5u) == 0);
  EXPECT(tb->text[0] == 'H');
  EXPECT(tb->text[4] == 'o');
  EXPECT(tb->text[5] == '\0');
  EXPECT(tb->text_edit.caret == 5u);
  capy_widget_destroy(tb);
}

static void test_text_edit_insert_middle_utf8(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *tb;
  capy_widget_context_init(&ctx, &allocator);
  tb = capy_widget_create(&ctx, CAPY_WIDGET_TEXTBOX);
  EXPECT(tb != 0);
  EXPECT(capy_textbox_insert(tb, "Hello", 5u) == 0);
  tb->text_edit.caret = 2u;
  EXPECT(capy_textbox_insert(tb, "\xC3\xA1", 2u) == 0);
  EXPECT(tb->text[0] == 'H');
  EXPECT(tb->text[1] == 'e');
  EXPECT((uint8_t)tb->text[2] == 0xC3u);
  EXPECT((uint8_t)tb->text[3] == 0xA1u);
  EXPECT(tb->text[4] == 'l');
  EXPECT(tb->text[5] == 'l');
  EXPECT(tb->text[6] == 'o');
  EXPECT(tb->text[7] == '\0');
  EXPECT(tb->text_edit.caret == 4u);
  capy_widget_destroy(tb);
}

static void test_text_edit_insert_overflow(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *tb;
  char big[280];
  uint16_t i;
  capy_widget_context_init(&ctx, &allocator);
  tb = capy_widget_create(&ctx, CAPY_WIDGET_TEXTBOX);
  EXPECT(tb != 0);
  for (i = 0u; i < 280u; ++i) {
    big[i] = 'A';
  }
  EXPECT(capy_textbox_insert(tb, big, 280u) == -1);
  EXPECT(tb->text[0] == '\0');
  EXPECT(tb->text_edit.caret == 0u);
  capy_widget_destroy(tb);
}

static void test_text_edit_delete_selection(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *tb;
  capy_widget_context_init(&ctx, &allocator);
  tb = capy_widget_create(&ctx, CAPY_WIDGET_TEXTBOX);
  EXPECT(tb != 0);
  EXPECT(capy_textbox_insert(tb, "Hello World", 11u) == 0);
  capy_textbox_set_selection(tb, 6u, 11u);
  EXPECT(tb->text_edit.sel_start == 6u);
  EXPECT(tb->text_edit.sel_end == 11u);
  EXPECT(capy_textbox_delete(tb, 0) == 0);
  EXPECT(tb->text[0] == 'H');
  EXPECT(tb->text[4] == 'o');
  EXPECT(tb->text[5] == ' ');
  EXPECT(tb->text[6] == '\0');
  EXPECT(tb->text_edit.caret == 6u);
  EXPECT(tb->text_edit.sel_start == tb->text_edit.sel_end);
  capy_widget_destroy(tb);
}

static void test_text_edit_backspace_utf8(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *tb;
  capy_widget_context_init(&ctx, &allocator);
  tb = capy_widget_create(&ctx, CAPY_WIDGET_TEXTBOX);
  EXPECT(tb != 0);
  EXPECT(capy_textbox_insert(tb, "He\xC3\xA1", 4u) == 0);
  EXPECT(tb->text_edit.caret == 4u);
  EXPECT(capy_textbox_delete(tb, -1) == 0);
  EXPECT(tb->text[0] == 'H');
  EXPECT(tb->text[1] == 'e');
  EXPECT(tb->text[2] == '\0');
  EXPECT(tb->text_edit.caret == 2u);
  capy_widget_destroy(tb);
}

static void test_displaylist_password_mode(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *tb;
  struct capy_dl_cmd cmd_buf[16];
  char text_buf[32];
  struct capy_display_list dl;
  uint32_t i;
  int found_text = 0;
  capy_widget_context_init(&ctx, &allocator);
  tb = capy_widget_create(&ctx, CAPY_WIDGET_TEXTBOX);
  EXPECT(tb != 0);
  tb->text_edit.password = 1u;
  capy_widget_set_text(tb, "abc");
  capy_widget_set_bounds(tb, 0, 0, 100u, 20u);
  capy_display_list_init(&dl, cmd_buf, 16u, text_buf, 32u);
  EXPECT(capy_widget_emit(tb, &dl) == 0);
  for (i = 0u; i < dl.count; ++i) {
    if (dl.cmds[i].op == CAPY_DL_TEXT) {
      EXPECT(dl.cmds[i].text_len == 3u);
      EXPECT(dl.text_pool[dl.cmds[i].text_offset] == '*');
      EXPECT(dl.text_pool[dl.cmds[i].text_offset + 1u] == '*');
      EXPECT(dl.text_pool[dl.cmds[i].text_offset + 2u] == '*');
      found_text = 1;
      break;
    }
  }
  EXPECT(found_text == 1);
  capy_widget_destroy(tb);
}

static void test_text_edit_readonly_blocks_insert(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *tb;
  char buf[16];
  int copied;
  capy_widget_context_init(&ctx, &allocator);
  tb = capy_widget_create(&ctx, CAPY_WIDGET_TEXTBOX);
  EXPECT(tb != 0);
  capy_widget_set_text(tb, "hi");
  tb->text_edit.readonly = 1u;
  EXPECT(capy_textbox_insert(tb, "X", 1u) == -1);
  EXPECT(capy_textbox_delete(tb, -1) == -1);
  EXPECT(tb->text[0] == 'h');
  EXPECT(tb->text[1] == 'i');
  EXPECT(tb->text[2] == '\0');
  copied = capy_textbox_copy(tb, buf, sizeof(buf));
  EXPECT(copied == 2);
  EXPECT(buf[0] == 'h');
  EXPECT(buf[1] == 'i');
  EXPECT(buf[2] == '\0');
  capy_widget_destroy(tb);
}

static void test_text_edit_ime_stub(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *tb;
  capy_widget_context_init(&ctx, &allocator);
  tb = capy_widget_create(&ctx, CAPY_WIDGET_TEXTBOX);
  EXPECT(tb != 0);
  EXPECT(capy_widget_ime_compose(tb, "preedit", 7u) == 0);
  EXPECT(tb->text[0] == '\0');
  EXPECT(capy_widget_ime_compose(tb, "\xFF\xFE", 2u) == -1);
  EXPECT(tb->text[0] == '\0');
  capy_widget_destroy(tb);
}

static void test_text_edit_utf8_invalid_rejected(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *tb;
  capy_widget_context_init(&ctx, &allocator);
  tb = capy_widget_create(&ctx, CAPY_WIDGET_TEXTBOX);
  EXPECT(tb != 0);
  EXPECT(capy_textbox_insert(tb, "\xFF\xFE", 2u) == -1);
  EXPECT(tb->text[0] == '\0');
  EXPECT(capy_textbox_insert(tb, "\xC3", 1u) == -1);
  EXPECT(tb->text[0] == '\0');
  EXPECT(capy_textbox_insert(tb, "\xC3\x28", 2u) == -1);
  EXPECT(tb->text[0] == '\0');
  capy_widget_destroy(tb);
}

static void test_text_edit_determinism(void) {
  struct test_heap heap_a = {{0}, 0u, 0u};
  struct test_heap heap_b = {{0}, 0u, 0u};
  struct capy_widget_allocator alloc_a = {test_alloc, test_free, &heap_a};
  struct capy_widget_allocator alloc_b = {test_alloc, test_free, &heap_b};
  struct capy_widget_context ctx_a;
  struct capy_widget_context ctx_b;
  struct capy_widget *a;
  struct capy_widget *b;
  uint16_t i;
  capy_widget_context_init(&ctx_a, &alloc_a);
  capy_widget_context_init(&ctx_b, &alloc_b);
  a = capy_widget_create(&ctx_a, CAPY_WIDGET_TEXTBOX);
  b = capy_widget_create(&ctx_b, CAPY_WIDGET_TEXTBOX);
  EXPECT(a != 0);
  EXPECT(b != 0);
  EXPECT(capy_textbox_insert(a, "Hello", 5u) == 0);
  EXPECT(capy_textbox_insert(b, "Hello", 5u) == 0);
  a->text_edit.caret = 2u;
  b->text_edit.caret = 2u;
  EXPECT(capy_textbox_insert(a, "\xC3\xA1", 2u) == 0);
  EXPECT(capy_textbox_insert(b, "\xC3\xA1", 2u) == 0);
  capy_textbox_set_selection(a, 0u, 2u);
  capy_textbox_set_selection(b, 0u, 2u);
  EXPECT(capy_textbox_delete(a, 0) == 0);
  EXPECT(capy_textbox_delete(b, 0) == 0);
  for (i = 0u; i < CAPY_WIDGET_MAX_TEXT; ++i) {
    EXPECT(a->text[i] == b->text[i]);
    if (a->text[i] == '\0') {
      break;
    }
  }
  EXPECT(a->text_edit.caret == b->text_edit.caret);
  capy_widget_destroy(a);
  capy_widget_destroy(b);
}

static void test_anim_sample_start_returns_from(void) {
  struct capy_anim a;
  int32_t v = 0;
  capy_anim_start(&a, 100u, 200u, 0, 100, CAPY_ANIM_LINEAR);
  EXPECT(capy_anim_sample(&a, 100u, &v) == 0);
  EXPECT(v == 0);
}

static void test_anim_sample_end_returns_to(void) {
  struct capy_anim a;
  int32_t v = 0;
  capy_anim_start(&a, 100u, 200u, 0, 100, CAPY_ANIM_LINEAR);
  EXPECT(capy_anim_sample(&a, 300u, &v) == 0);
  EXPECT(v == 100);
  EXPECT(capy_anim_sample(&a, 500u, &v) == 0);
  EXPECT(v == 100);
}

static void test_anim_sample_inactive_returns_from(void) {
  struct capy_anim a;
  int32_t v = 0;
  uint8_t *p = (uint8_t *)&a;
  uint32_t i;
  for (i = 0u; i < sizeof(a); ++i) {
    p[i] = 0u;
  }
  a.from = 42;
  EXPECT(capy_anim_sample(&a, 100u, &v) == 0);
  EXPECT(v == 42);
}

static void test_anim_linear_midpoint(void) {
  struct capy_anim a;
  int32_t v = 0;
  capy_anim_start(&a, 0u, 100u, 0, 100, CAPY_ANIM_LINEAR);
  EXPECT(capy_anim_sample(&a, 50u, &v) == 0);
  EXPECT(v == 50);
  capy_anim_start(&a, 0u, 200u, 100, 300, CAPY_ANIM_LINEAR);
  EXPECT(capy_anim_sample(&a, 100u, &v) == 0);
  EXPECT(v == 200);
}

static void test_anim_easings_monotonic_and_endpoints(void) {
  struct capy_anim a;
  int32_t v0, v1, v2, v3, v4;
  uint8_t easings[4];
  uint32_t k;
  easings[0] = CAPY_ANIM_LINEAR;
  easings[1] = CAPY_ANIM_EASE_IN;
  easings[2] = CAPY_ANIM_EASE_OUT;
  easings[3] = CAPY_ANIM_EASE_IN_OUT;
  for (k = 0u; k < 4u; ++k) {
    capy_anim_start(&a, 0u, 100u, 0, 100, easings[k]);
    EXPECT(capy_anim_sample(&a, 0u, &v0) == 0);
    EXPECT(capy_anim_sample(&a, 25u, &v1) == 0);
    EXPECT(capy_anim_sample(&a, 50u, &v2) == 0);
    EXPECT(capy_anim_sample(&a, 75u, &v3) == 0);
    EXPECT(capy_anim_sample(&a, 100u, &v4) == 0);
    EXPECT(v0 == 0);
    EXPECT(v4 == 100);
    EXPECT(v0 <= v1);
    EXPECT(v1 <= v2);
    EXPECT(v2 <= v3);
    EXPECT(v3 <= v4);
  }
}

static void test_anim_now_rewound_stable(void) {
  struct capy_anim a;
  int32_t v = 0;
  capy_anim_start(&a, 100u, 200u, 50, 150, CAPY_ANIM_LINEAR);
  EXPECT(capy_anim_sample(&a, 50u, &v) == 0);
  EXPECT(v == 50);
  EXPECT(capy_anim_sample(&a, 0u, &v) == 0);
  EXPECT(v == 50);
  EXPECT(capy_anim_sample(&a, 100u, &v) == 0);
  EXPECT(v == 50);
}

static void test_anim_zero_duration_returns_from(void) {
  struct capy_anim a;
  int32_t v = 0;
  capy_anim_start(&a, 100u, 0u, 50, 150, CAPY_ANIM_LINEAR);
  EXPECT(capy_anim_sample(&a, 100u, &v) == 0);
  EXPECT(v == 50);
  EXPECT(capy_anim_sample(&a, 500u, &v) == 0);
  EXPECT(v == 50);
}

static void test_anim_multiple_independent(void) {
  struct capy_anim a, b;
  int32_t va = 0;
  int32_t vb = 0;
  capy_anim_start(&a, 0u, 100u, 0, 100, CAPY_ANIM_LINEAR);
  capy_anim_start(&b, 50u, 100u, 200, 300, CAPY_ANIM_LINEAR);
  EXPECT(capy_anim_sample(&a, 50u, &va) == 0);
  EXPECT(capy_anim_sample(&b, 50u, &vb) == 0);
  EXPECT(va == 50);
  EXPECT(vb == 200);
  EXPECT(capy_anim_sample(&a, 100u, &va) == 0);
  EXPECT(capy_anim_sample(&b, 100u, &vb) == 0);
  EXPECT(va == 100);
  EXPECT(vb == 250);
}

static void test_anim_determinism(void) {
  struct capy_anim a, b;
  int32_t va = 0;
  int32_t vb = 0;
  uint32_t ticks[5];
  uint32_t i;
  ticks[0] = 0u;
  ticks[1] = 25u;
  ticks[2] = 50u;
  ticks[3] = 75u;
  ticks[4] = 100u;
  capy_anim_start(&a, 0u, 100u, -50, 250, CAPY_ANIM_EASE_IN_OUT);
  capy_anim_start(&b, 0u, 100u, -50, 250, CAPY_ANIM_EASE_IN_OUT);
  for (i = 0u; i < 5u; ++i) {
    EXPECT(capy_anim_sample(&a, ticks[i], &va) == 0);
    EXPECT(capy_anim_sample(&b, ticks[i], &vb) == 0);
    EXPECT(va == vb);
  }
}

static void test_widget_tick_walks_tree(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *panel;
  struct capy_widget *button;
  struct capy_widget *label;
  capy_widget_context_init(&ctx, &allocator);
  panel = capy_widget_create(&ctx, CAPY_WIDGET_PANEL);
  button = capy_widget_create(&ctx, CAPY_WIDGET_BUTTON);
  label = capy_widget_create(&ctx, CAPY_WIDGET_LABEL);
  EXPECT(panel != 0);
  EXPECT(button != 0);
  EXPECT(label != 0);
  EXPECT(capy_widget_add_child(panel, button) == 0);
  EXPECT(capy_widget_add_child(panel, label) == 0);
  capy_widget_tick(panel, 0u);
  capy_widget_tick(panel, 1000u);
  capy_widget_tick(panel, 500u);
  capy_widget_tick(0, 1000u);
  capy_widget_destroy(panel);
}

static void test_theme_defaults_snapshot(void) {
  struct capy_theme light = capy_theme_default_light();
  struct capy_theme dark = capy_theme_default_dark();
  struct capy_theme hc = capy_theme_high_contrast();
  EXPECT(light.variant == CAPY_THEME_VARIANT_LIGHT);
  EXPECT(light.high_contrast == 0u);
  EXPECT(light.version == (uint16_t)CAPY_THEME_FORMAT_VERSION);
  EXPECT(light.tokens[CAPY_TOKEN_NONE] == 0u);
  EXPECT(light.tokens[CAPY_TOKEN_BG_BASE] == 0xFFF5F6F8u);
  EXPECT(light.tokens[CAPY_TOKEN_FG_PRIMARY] == 0xFF1A1B1Fu);
  EXPECT(light.tokens[CAPY_TOKEN_ACCENT] == 0xFF2563EBu);
  EXPECT(dark.variant == CAPY_THEME_VARIANT_DARK);
  EXPECT(dark.high_contrast == 0u);
  EXPECT(dark.tokens[CAPY_TOKEN_BG_BASE] == 0xFF1A1B1Fu);
  EXPECT(dark.tokens[CAPY_TOKEN_FG_PRIMARY] == 0xFFE8ECEFu);
  EXPECT(dark.tokens[CAPY_TOKEN_ACCENT] == 0xFF3B82F6u);
  EXPECT(hc.variant == CAPY_THEME_VARIANT_HIGH_CONTRAST);
  EXPECT(hc.high_contrast == 1u);
  EXPECT(hc.tokens[CAPY_TOKEN_BG_BASE] == 0xFF000000u);
  EXPECT(hc.tokens[CAPY_TOKEN_FG_PRIMARY] == 0xFFFFFFFFu);
}

static void test_theme_apply_stores_in_context(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_theme dark = capy_theme_default_dark();
  capy_widget_context_init(&ctx, &allocator);
  EXPECT(ctx.theme.variant == 0u);
  capy_theme_apply(&ctx, &dark);
  EXPECT(ctx.theme.variant == CAPY_THEME_VARIANT_DARK);
  EXPECT(ctx.theme.tokens[CAPY_TOKEN_BG_BASE] == 0xFF1A1B1Fu);
  EXPECT(ctx.theme.tokens[CAPY_TOKEN_ACCENT] == 0xFF3B82F6u);
  capy_theme_apply(0, &dark);
  capy_theme_apply(&ctx, 0);
  EXPECT(ctx.theme.variant == CAPY_THEME_VARIANT_DARK);
}

static void test_theme_resolve(void) {
  struct capy_theme t = capy_theme_default_light();
  EXPECT(capy_theme_resolve(&t, 0u, 0xDEADBEEFu) == 0xDEADBEEFu);
  EXPECT(capy_theme_resolve(&t, (uint8_t)CAPY_TOKEN_ACCENT, 0u) ==
         0xFF2563EBu);
  EXPECT(capy_theme_resolve(&t, (uint8_t)CAPY_TOKEN_BG_BASE, 0u) ==
         0xFFF5F6F8u);
  EXPECT(capy_theme_resolve(&t, 250u, 0xDEADBEEFu) == 0xDEADBEEFu);
  EXPECT(capy_theme_resolve((const struct capy_theme *)0,
                            (uint8_t)CAPY_TOKEN_ACCENT,
                            0xDEADBEEFu) == 0xDEADBEEFu);
}

static void test_theme_high_contrast_meets_wcag_aa(void) {
  struct capy_theme hc = capy_theme_high_contrast();
  uint32_t bg = hc.tokens[CAPY_TOKEN_BG_BASE];
  uint32_t fg = hc.tokens[CAPY_TOKEN_FG_PRIMARY];
  uint32_t bg_sum = ((bg >> 16) & 0xFFu) + ((bg >> 8) & 0xFFu) + (bg & 0xFFu);
  uint32_t fg_sum = ((fg >> 16) & 0xFFu) + ((fg >> 8) & 0xFFu) + (fg & 0xFFu);
  EXPECT(bg_sum < 96u);
  EXPECT(fg_sum > 672u);
  EXPECT(hc.high_contrast == 1u);
}

static void test_displaylist_uses_theme_when_token_set(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *btn;
  struct capy_dl_cmd cmd_buf[16];
  char text_buf[32];
  struct capy_display_list dl;
  struct capy_theme dark = capy_theme_default_dark();
  uint32_t i;
  int found_rect = 0;
  capy_widget_context_init(&ctx, &allocator);
  btn = capy_widget_create(&ctx, CAPY_WIDGET_BUTTON);
  EXPECT(btn != 0);
  capy_widget_set_bounds(btn, 0, 0, 50u, 20u);
  btn->style.bg_token = (uint8_t)CAPY_TOKEN_ACCENT;
  btn->style.bg_color = 0x00112233u;
  capy_display_list_init(&dl, cmd_buf, 16u, text_buf, 32u);
  dl.theme = &dark;
  EXPECT(capy_widget_emit(btn, &dl) == 0);
  for (i = 0u; i < dl.count; ++i) {
    if (dl.cmds[i].op == CAPY_DL_RECT) {
      EXPECT(dl.cmds[i].color == dark.tokens[CAPY_TOKEN_ACCENT]);
      EXPECT(dl.cmds[i].color != 0x00112233u);
      found_rect = 1;
      break;
    }
  }
  EXPECT(found_rect == 1);
  capy_widget_destroy(btn);
}

static void test_displaylist_backward_compat_with_literal(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *btn;
  struct capy_dl_cmd cmd_buf[16];
  char text_buf[32];
  struct capy_display_list dl;
  struct capy_theme dark = capy_theme_default_dark();
  uint32_t i;
  int found_rect = 0;
  capy_widget_context_init(&ctx, &allocator);
  btn = capy_widget_create(&ctx, CAPY_WIDGET_BUTTON);
  EXPECT(btn != 0);
  capy_widget_set_bounds(btn, 0, 0, 50u, 20u);
  EXPECT(btn->style.bg_token == 0u);
  EXPECT(btn->style.bg_color == 0xFF2563EBu);
  capy_display_list_init(&dl, cmd_buf, 16u, text_buf, 32u);
  dl.theme = &dark;
  EXPECT(capy_widget_emit(btn, &dl) == 0);
  for (i = 0u; i < dl.count; ++i) {
    if (dl.cmds[i].op == CAPY_DL_RECT) {
      EXPECT(dl.cmds[i].color == 0xFF2563EBu);
      found_rect = 1;
      break;
    }
  }
  EXPECT(found_rect == 1);
  capy_widget_destroy(btn);
}

static void test_displaylist_theme_change_preserves_structure(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *btn;
  struct capy_dl_cmd cmd_buf_a[16];
  struct capy_dl_cmd cmd_buf_b[16];
  char text_buf_a[32];
  char text_buf_b[32];
  struct capy_display_list dl_a;
  struct capy_display_list dl_b;
  struct capy_theme light = capy_theme_default_light();
  struct capy_theme dark = capy_theme_default_dark();
  uint32_t i;
  uint32_t bg_a = 0u;
  uint32_t bg_b = 0u;
  capy_widget_context_init(&ctx, &allocator);
  btn = capy_widget_create(&ctx, CAPY_WIDGET_BUTTON);
  EXPECT(btn != 0);
  capy_widget_set_bounds(btn, 0, 0, 50u, 20u);
  btn->style.bg_token = (uint8_t)CAPY_TOKEN_BG_BASE;
  btn->style.border_token = (uint8_t)CAPY_TOKEN_BORDER;
  btn->style.fg_token = (uint8_t)CAPY_TOKEN_FG_PRIMARY;
  capy_display_list_init(&dl_a, cmd_buf_a, 16u, text_buf_a, 32u);
  dl_a.theme = &light;
  EXPECT(capy_widget_emit(btn, &dl_a) == 0);
  capy_display_list_init(&dl_b, cmd_buf_b, 16u, text_buf_b, 32u);
  dl_b.theme = &dark;
  EXPECT(capy_widget_emit(btn, &dl_b) == 0);
  EXPECT(dl_a.count == dl_b.count);
  for (i = 0u; i < dl_a.count; ++i) {
    EXPECT(dl_a.cmds[i].op == dl_b.cmds[i].op);
    if (dl_a.cmds[i].op == CAPY_DL_RECT && bg_a == 0u && bg_b == 0u) {
      bg_a = dl_a.cmds[i].color;
      bg_b = dl_b.cmds[i].color;
    }
  }
  EXPECT(bg_a == light.tokens[CAPY_TOKEN_BG_BASE]);
  EXPECT(bg_b == dark.tokens[CAPY_TOKEN_BG_BASE]);
  EXPECT(bg_a != bg_b);
  capy_widget_destroy(btn);
}

/* ── v0.8: capy_widget_diff + CAPY_DL_DIRTY_HINT (since 0.8.0) ──────────── */

static void test_displaylist_diff_identical_zero_rects(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *panel;
  struct capy_widget *button;
  struct capy_dl_cmd cmd_a[32];
  struct capy_dl_cmd cmd_b[32];
  char text_a[32];
  char text_b[32];
  struct capy_display_list prev;
  struct capy_display_list next;
  struct capy_ui_rect dirty[16];
  int rc;
  capy_widget_context_init(&ctx, &allocator);
  panel = capy_widget_create(&ctx, CAPY_WIDGET_PANEL);
  button = capy_widget_create(&ctx, CAPY_WIDGET_BUTTON);
  EXPECT(panel != 0);
  EXPECT(button != 0);
  capy_widget_set_bounds(panel, 0, 0, 100u, 100u);
  capy_widget_set_bounds(button, 10, 10, 20u, 20u);
  EXPECT(capy_widget_add_child(panel, button) == 0);
  capy_display_list_init(&prev, cmd_a, 32u, text_a, 32u);
  capy_display_list_init(&next, cmd_b, 32u, text_b, 32u);
  EXPECT(prev.version == CAPY_DISPLAY_LIST_SCHEMA_VERSION);
  EXPECT(capy_widget_emit(panel, &prev) == 0);
  EXPECT(capy_widget_emit(panel, &next) == 0);
  rc = capy_widget_diff(&prev, &next, dirty, 16u);
  EXPECT(rc == 0);
  capy_widget_destroy(panel);
}

static void test_displaylist_diff_button_color_change(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *panel;
  struct capy_widget *button;
  struct capy_dl_cmd cmd_a[32];
  struct capy_dl_cmd cmd_b[32];
  char text_a[32];
  char text_b[32];
  struct capy_display_list prev;
  struct capy_display_list next;
  struct capy_ui_rect dirty[16];
  int rc;
  uint32_t i;
  capy_widget_context_init(&ctx, &allocator);
  panel = capy_widget_create(&ctx, CAPY_WIDGET_PANEL);
  button = capy_widget_create(&ctx, CAPY_WIDGET_BUTTON);
  EXPECT(panel != 0);
  EXPECT(button != 0);
  capy_widget_set_bounds(panel, 0, 0, 200u, 100u);
  capy_widget_set_bounds(button, 10, 10, 20u, 20u);
  button->style.bg_color = 0xFF112233u;
  EXPECT(capy_widget_add_child(panel, button) == 0);
  capy_display_list_init(&prev, cmd_a, 32u, text_a, 32u);
  EXPECT(capy_widget_emit(panel, &prev) == 0);
  button->style.bg_color = 0xFFAABBCCu;
  capy_display_list_init(&next, cmd_b, 32u, text_b, 32u);
  EXPECT(capy_widget_emit(panel, &next) == 0);
  EXPECT(prev.count == next.count);
  rc = capy_widget_diff(&prev, &next, dirty, 16u);
  EXPECT(rc > 0);
  EXPECT(rc <= (int)prev.count);
  /* No dirty rect should cover the full panel — only the button rect. */
  for (i = 0u; i < (uint32_t)rc; ++i) {
    EXPECT(dirty[i].width <= 20u);
    EXPECT(dirty[i].height <= 20u);
    EXPECT(dirty[i].x == 10);
    EXPECT(dirty[i].y == 10);
  }
  capy_widget_destroy(panel);
}

static void test_displaylist_diff_coalesces_adjacent(void) {
  /* When several consecutive cmds share the same rect (CLIP_PUSH, RECT,
   * BORDER all on the button bounds), changing all of them must yield a
   * single coalesced dirty rect, not three. */
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *button;
  struct capy_dl_cmd cmd_a[16];
  struct capy_dl_cmd cmd_b[16];
  char text_a[16];
  char text_b[16];
  struct capy_display_list prev;
  struct capy_display_list next;
  struct capy_ui_rect dirty[8];
  int rc;
  capy_widget_context_init(&ctx, &allocator);
  button = capy_widget_create(&ctx, CAPY_WIDGET_BUTTON);
  EXPECT(button != 0);
  capy_widget_set_bounds(button, 5, 5, 40u, 30u);
  button->style.bg_color = 0xFF000001u;
  button->style.border_color = 0xFF000002u;
  button->style.border_width = 2u;
  capy_display_list_init(&prev, cmd_a, 16u, text_a, 16u);
  EXPECT(capy_widget_emit(button, &prev) == 0);
  /* Change bg + border colour at once. Both cmds share the same rect. */
  button->style.bg_color = 0xFFAAAAAAu;
  button->style.border_color = 0xFFBBBBBBu;
  capy_display_list_init(&next, cmd_b, 16u, text_b, 16u);
  EXPECT(capy_widget_emit(button, &next) == 0);
  rc = capy_widget_diff(&prev, &next, dirty, 8u);
  EXPECT(rc == 1);
  EXPECT(dirty[0].x == 5);
  EXPECT(dirty[0].y == 5);
  EXPECT(dirty[0].width == 40u);
  EXPECT(dirty[0].height == 30u);
  capy_widget_destroy(button);
}

static void test_displaylist_diff_overflow_returns_minus_one(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *a;
  struct capy_widget *b;
  struct capy_widget *parent_a;
  struct capy_widget *parent_b;
  struct capy_dl_cmd cmd_a[32];
  struct capy_dl_cmd cmd_b[32];
  char text_a[32];
  char text_b[32];
  struct capy_display_list prev;
  struct capy_display_list next;
  struct capy_ui_rect dirty[1];
  int rc;
  capy_widget_context_init(&ctx, &allocator);
  parent_a = capy_widget_create(&ctx, CAPY_WIDGET_PANEL);
  parent_b = capy_widget_create(&ctx, CAPY_WIDGET_PANEL);
  a = capy_widget_create(&ctx, CAPY_WIDGET_BUTTON);
  b = capy_widget_create(&ctx, CAPY_WIDGET_BUTTON);
  EXPECT(parent_a && parent_b && a && b);
  capy_widget_set_bounds(parent_a, 0, 0, 100u, 100u);
  capy_widget_set_bounds(parent_b, 0, 0, 100u, 100u);
  capy_widget_set_bounds(a, 10, 10, 20u, 20u);
  capy_widget_set_bounds(b, 50, 50, 20u, 20u);
  EXPECT(capy_widget_add_child(parent_a, a) == 0);
  EXPECT(capy_widget_add_child(parent_b, b) == 0);
  capy_display_list_init(&prev, cmd_a, 32u, text_a, 32u);
  capy_display_list_init(&next, cmd_b, 32u, text_b, 32u);
  EXPECT(capy_widget_emit(parent_a, &prev) == 0);
  EXPECT(capy_widget_emit(parent_b, &next) == 0);
  /* Two disjoint button bounds → at least 2 unique dirty rects expected. */
  rc = capy_widget_diff(&prev, &next, dirty, 1u);
  EXPECT(rc == -1);
  /* NULL out_dirty with non-zero cap is also -1. */
  EXPECT(capy_widget_diff(&prev, &next, 0, 4u) == -1);
  capy_widget_destroy(parent_a);
  capy_widget_destroy(parent_b);
}

static void test_displaylist_diff_determinism(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *panel;
  struct capy_widget *button;
  struct capy_dl_cmd cmd_a[32];
  struct capy_dl_cmd cmd_b[32];
  char text_a[32];
  char text_b[32];
  struct capy_display_list prev;
  struct capy_display_list next;
  struct capy_ui_rect dirty_a[16];
  struct capy_ui_rect dirty_b[16];
  int rc_a;
  int rc_b;
  int i;
  capy_widget_context_init(&ctx, &allocator);
  panel = capy_widget_create(&ctx, CAPY_WIDGET_PANEL);
  button = capy_widget_create(&ctx, CAPY_WIDGET_BUTTON);
  EXPECT(panel != 0);
  EXPECT(button != 0);
  capy_widget_set_bounds(panel, 0, 0, 200u, 100u);
  capy_widget_set_bounds(button, 12, 7, 33u, 19u);
  button->style.bg_color = 0xFF010203u;
  EXPECT(capy_widget_add_child(panel, button) == 0);
  capy_display_list_init(&prev, cmd_a, 32u, text_a, 32u);
  EXPECT(capy_widget_emit(panel, &prev) == 0);
  button->style.bg_color = 0xFF040506u;
  capy_display_list_init(&next, cmd_b, 32u, text_b, 32u);
  EXPECT(capy_widget_emit(panel, &next) == 0);
  rc_a = capy_widget_diff(&prev, &next, dirty_a, 16u);
  rc_b = capy_widget_diff(&prev, &next, dirty_b, 16u);
  EXPECT(rc_a > 0);
  EXPECT(rc_a == rc_b);
  for (i = 0; i < rc_a; ++i) {
    EXPECT(dirty_a[i].x == dirty_b[i].x);
    EXPECT(dirty_a[i].y == dirty_b[i].y);
    EXPECT(dirty_a[i].width == dirty_b[i].width);
    EXPECT(dirty_a[i].height == dirty_b[i].height);
  }
  capy_widget_destroy(panel);
}

static void test_displaylist_dirty_hint_op_not_emitted(void) {
  /* Schema is now 3 and CAPY_DL_DIRTY_HINT is a defined enum value, but
   * capy_widget_emit must never produce it: it is reserved for higher-level
   * producers and capy_widget_diff (which writes rects, not display-list
   * cmds). Consumers below schema 3 must ignore any DIRTY_HINT op. */
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *panel;
  struct capy_widget *button;
  struct capy_dl_cmd cmd_buf[32];
  char text_buf[32];
  struct capy_display_list dl;
  uint32_t i;
  EXPECT(CAPY_DISPLAY_LIST_SCHEMA_VERSION >= 3u);
  EXPECT((int)CAPY_DL_DIRTY_HINT > (int)CAPY_DL_FOCUS_RING);
  capy_widget_context_init(&ctx, &allocator);
  panel = capy_widget_create(&ctx, CAPY_WIDGET_PANEL);
  button = capy_widget_create(&ctx, CAPY_WIDGET_BUTTON);
  EXPECT(panel != 0);
  EXPECT(button != 0);
  capy_widget_set_bounds(panel, 0, 0, 100u, 100u);
  capy_widget_set_bounds(button, 10, 10, 20u, 20u);
  EXPECT(capy_widget_add_child(panel, button) == 0);
  capy_display_list_init(&dl, cmd_buf, 32u, text_buf, 32u);
  EXPECT(dl.version == CAPY_DISPLAY_LIST_SCHEMA_VERSION);
  EXPECT(capy_widget_emit(panel, &dl) == 0);
  for (i = 0u; i < dl.count; ++i) {
    EXPECT(dl.cmds[i].op != CAPY_DL_DIRTY_HINT);
  }
  capy_widget_destroy(panel);
}

/* ── v0.9: text metrics hook + font_id in display-list (since 0.9.0) ────── */

struct test_metrics_state {
  int calls;
  uint16_t last_font_id;
  uint8_t last_font_size;
  uint16_t last_len;
  int return_code;
  uint32_t width_to_return;
  uint32_t height_to_return;
};

static int test_metrics_hook(uint16_t font_id, uint8_t font_size,
                             const char *text, uint16_t len,
                             uint32_t *out_width, uint32_t *out_height,
                             void *user_data) {
  struct test_metrics_state *s = (struct test_metrics_state *)user_data;
  (void)text;
  ++s->calls;
  s->last_font_id = font_id;
  s->last_font_size = font_size;
  s->last_len = len;
  if (s->return_code == 0) {
    *out_width = s->width_to_return;
    *out_height = s->height_to_return;
  }
  return s->return_code;
}

static void test_text_metrics_fallback_deterministic(void) {
  struct capy_widget_context ctx;
  uint32_t w = 0u;
  uint32_t h = 0u;
  capy_widget_context_init(&ctx, 0);
  EXPECT(ctx.text_metrics_fn == 0);
  EXPECT(ctx.text_metrics_user == 0);
  EXPECT(capy_widget_measure_text(&ctx, 0u, 12u, "hello", 5u, &w, &h) == 0);
  EXPECT(w == (12u / 2u) * 5u); /* 30 */
  EXPECT(h == 12u);
  /* font_size == 0 → effective 16 */
  EXPECT(capy_widget_measure_text(&ctx, 0u, 0u, "abc", 3u, &w, &h) == 0);
  EXPECT(w == (16u / 2u) * 3u); /* 24 */
  EXPECT(h == 16u);
  /* len == 0 → width 0 */
  EXPECT(capy_widget_measure_text(&ctx, 0u, 14u, "ignored", 0u, &w, &h) == 0);
  EXPECT(w == 0u);
  EXPECT(h == 14u);
  /* NULL out args → -1 */
  EXPECT(capy_widget_measure_text(&ctx, 0u, 12u, "x", 1u, 0, &h) == -1);
  EXPECT(capy_widget_measure_text(&ctx, 0u, 12u, "x", 1u, &w, 0) == -1);
}

static void test_text_metrics_hook_called_when_set(void) {
  struct capy_widget_context ctx;
  struct test_metrics_state s = {0, 0u, 0u, 0u, 0, 999u, 42u};
  uint32_t w = 0u;
  uint32_t h = 0u;
  capy_widget_context_init(&ctx, 0);
  capy_widget_set_text_metrics_hook(&ctx, test_metrics_hook, &s);
  EXPECT(ctx.text_metrics_fn == test_metrics_hook);
  EXPECT(ctx.text_metrics_user == &s);
  EXPECT(capy_widget_measure_text(&ctx, 7u, 20u, "abcd", 4u, &w, &h) == 0);
  EXPECT(s.calls == 1);
  EXPECT(s.last_font_id == 7u);
  EXPECT(s.last_font_size == 20u);
  EXPECT(s.last_len == 4u);
  EXPECT(w == 999u);
  EXPECT(h == 42u);
}

static void test_text_metrics_hook_cleared_restores_fallback(void) {
  struct capy_widget_context ctx;
  struct test_metrics_state s = {0, 0u, 0u, 0u, 0, 50u, 50u};
  uint32_t w = 0u;
  uint32_t h = 0u;
  capy_widget_context_init(&ctx, 0);
  capy_widget_set_text_metrics_hook(&ctx, test_metrics_hook, &s);
  EXPECT(capy_widget_measure_text(&ctx, 0u, 10u, "ab", 2u, &w, &h) == 0);
  EXPECT(s.calls == 1);
  EXPECT(w == 50u);
  /* Clear hook → fallback again. */
  capy_widget_set_text_metrics_hook(&ctx, 0, 0);
  EXPECT(ctx.text_metrics_fn == 0);
  EXPECT(capy_widget_measure_text(&ctx, 0u, 10u, "ab", 2u, &w, &h) == 0);
  EXPECT(s.calls == 1); /* unchanged after clear */
  EXPECT(w == (10u / 2u) * 2u); /* 10 */
  EXPECT(h == 10u);
}

static void test_text_metrics_hook_failure_falls_back(void) {
  struct capy_widget_context ctx;
  struct test_metrics_state s = {0, 0u, 0u, 0u, -1, 999u, 999u};
  uint32_t w = 0u;
  uint32_t h = 0u;
  capy_widget_context_init(&ctx, 0);
  capy_widget_set_text_metrics_hook(&ctx, test_metrics_hook, &s);
  EXPECT(capy_widget_measure_text(&ctx, 3u, 8u, "xyz", 3u, &w, &h) == 0);
  EXPECT(s.calls == 1); /* hook was invoked... */
  EXPECT(w == (8u / 2u) * 3u); /* ...but fallback ran because rc == -1 */
  EXPECT(h == 8u);
}

static void test_displaylist_emits_font_id(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *label;
  struct capy_dl_cmd cmd_buf[16];
  char text_buf[32];
  struct capy_display_list dl;
  uint32_t i;
  int found_text = 0;
  EXPECT(CAPY_DISPLAY_LIST_SCHEMA_VERSION == 7u);
  capy_widget_context_init(&ctx, &allocator);
  label = capy_widget_create(&ctx, CAPY_WIDGET_LABEL);
  EXPECT(label != 0);
  capy_widget_set_bounds(label, 0, 0, 100u, 20u);
  capy_widget_set_text(label, "hi");
  label->style.font_size = 12u;
  label->font_id = 42u;
  capy_display_list_init(&dl, cmd_buf, 16u, text_buf, 32u);
  EXPECT(dl.version == 7u);
  EXPECT(capy_widget_emit(label, &dl) == 0);
  for (i = 0u; i < dl.count; ++i) {
    if (dl.cmds[i].op == CAPY_DL_TEXT) {
      EXPECT(dl.cmds[i].font_id == 42u);
      EXPECT(dl.cmds[i].font_size == 12u);
      found_text = 1;
    }
  }
  EXPECT(found_text == 1);
  capy_widget_destroy(label);
}

static void test_displaylist_font_id_default_zero(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *label;
  struct capy_dl_cmd cmd_buf[16];
  char text_buf[32];
  struct capy_display_list dl;
  uint32_t i;
  int found_text = 0;
  capy_widget_context_init(&ctx, &allocator);
  label = capy_widget_create(&ctx, CAPY_WIDGET_LABEL);
  EXPECT(label != 0);
  EXPECT(label->font_id == 0u); /* default after create */
  capy_widget_set_bounds(label, 0, 0, 100u, 20u);
  capy_widget_set_text(label, "hi");
  label->style.font_size = 16u;
  capy_display_list_init(&dl, cmd_buf, 16u, text_buf, 32u);
  EXPECT(capy_widget_emit(label, &dl) == 0);
  for (i = 0u; i < dl.count; ++i) {
    if (dl.cmds[i].op == CAPY_DL_TEXT) {
      EXPECT(dl.cmds[i].font_id == 0u);
      found_text = 1;
    }
  }
  EXPECT(found_text == 1);
  capy_widget_destroy(label);
}

/* ── v0.10: input plumbing — WHEEL/TOUCH/GAMEPAD + modifiers (since 0.10) ─ */

static void change_count(struct capy_widget *widget, void *user_data) {
  int *count = (int *)user_data;
  (void)widget;
  ++*count;
}

static void test_event_wheel_on_list_scrolls(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *list;
  struct capy_widget_event event = {CAPY_WIDGET_EVENT_NONE, 0, 0, 0u, 0u,
                                    0u, 0};
  struct capy_wheel_payload wheel = {0, 0};
  int changes = 0;
  capy_widget_context_init(&ctx, &allocator);
  list = capy_widget_create(&ctx, CAPY_WIDGET_LIST);
  EXPECT(list != 0);
  capy_widget_set_bounds(list, 0, 0, 100u, 200u);
  list->min_value = 0;
  list->max_value = 100;
  list->value = 10;
  capy_widget_set_on_change(list, change_count, &changes);
  event.type = CAPY_WIDGET_EVENT_WHEEL;
  event.x = 50;
  event.y = 50;
  wheel.delta_y = 5;
  event.payload = &wheel;
  EXPECT(capy_widget_handle_event(list, &event) == 1);
  EXPECT(list->value == 15);
  EXPECT(changes == 1);
  capy_widget_destroy(list);
}

static void test_event_wheel_clamps_to_range(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *list;
  struct capy_widget_event event = {CAPY_WIDGET_EVENT_NONE, 0, 0, 0u, 0u,
                                    0u, 0};
  struct capy_wheel_payload wheel = {0, 0};
  capy_widget_context_init(&ctx, &allocator);
  list = capy_widget_create(&ctx, CAPY_WIDGET_LIST);
  EXPECT(list != 0);
  capy_widget_set_bounds(list, 0, 0, 100u, 200u);
  list->min_value = 0;
  list->max_value = 20;
  list->value = 18;
  event.type = CAPY_WIDGET_EVENT_WHEEL;
  event.x = 50;
  event.y = 50;
  wheel.delta_y = 100;
  event.payload = &wheel;
  EXPECT(capy_widget_handle_event(list, &event) == 1);
  EXPECT(list->value == 20);
  /* negative delta clamps to min */
  wheel.delta_y = -500;
  EXPECT(capy_widget_handle_event(list, &event) == 1);
  EXPECT(list->value == 0);
  capy_widget_destroy(list);
}

static void test_event_wheel_on_panel_returns_unhandled(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *panel;
  struct capy_widget_event event = {CAPY_WIDGET_EVENT_NONE, 0, 0, 0u, 0u,
                                    0u, 0};
  struct capy_wheel_payload wheel = {0, 3};
  capy_widget_context_init(&ctx, &allocator);
  panel = capy_widget_create(&ctx, CAPY_WIDGET_PANEL);
  EXPECT(panel != 0);
  capy_widget_set_bounds(panel, 0, 0, 100u, 100u);
  event.type = CAPY_WIDGET_EVENT_WHEEL;
  event.x = 50;
  event.y = 50;
  event.payload = &wheel;
  EXPECT(capy_widget_handle_event(panel, &event) == 0);
  /* PANEL has no value to scroll → nothing changed. */
  EXPECT(panel->value == 0);
  capy_widget_destroy(panel);
}

static void test_event_touch_single_routes_as_pointer(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *panel;
  struct capy_widget *button;
  struct capy_widget_event event = {CAPY_WIDGET_EVENT_NONE, 0, 0, 0u, 0u,
                                    0u, 0};
  struct capy_touch_payload touch = {1u, {0, 0}, 256u};
  int clicks = 0;
  capy_widget_context_init(&ctx, &allocator);
  panel = capy_widget_create(&ctx, CAPY_WIDGET_PANEL);
  button = capy_widget_create(&ctx, CAPY_WIDGET_BUTTON);
  EXPECT(panel && button);
  capy_widget_set_bounds(panel, 0, 0, 100u, 100u);
  capy_widget_set_bounds(button, 10, 10, 30u, 30u);
  EXPECT(capy_widget_add_child(panel, button) == 0);
  capy_widget_set_on_click(button, click_count, &clicks);
  event.type = CAPY_WIDGET_EVENT_TOUCH_BEGIN;
  touch.pos.x = 20;
  touch.pos.y = 20;
  event.payload = &touch;
  EXPECT(capy_widget_handle_event(panel, &event) == 1);
  EXPECT(clicks == 1);
  capy_widget_destroy(panel);
}

static void test_event_touch_uses_payload_position_over_event_xy(void) {
  /* Payload position must override event.x/y when present. */
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *panel;
  struct capy_widget *button;
  struct capy_widget_event event = {CAPY_WIDGET_EVENT_NONE, 0, 0, 0u, 0u,
                                    0u, 0};
  struct capy_touch_payload touch = {1u, {0, 0}, 256u};
  int clicks = 0;
  capy_widget_context_init(&ctx, &allocator);
  panel = capy_widget_create(&ctx, CAPY_WIDGET_PANEL);
  button = capy_widget_create(&ctx, CAPY_WIDGET_BUTTON);
  EXPECT(panel && button);
  capy_widget_set_bounds(panel, 0, 0, 200u, 200u);
  capy_widget_set_bounds(button, 100, 100, 30u, 30u);
  EXPECT(capy_widget_add_child(panel, button) == 0);
  capy_widget_set_on_click(button, click_count, &clicks);
  event.type = CAPY_WIDGET_EVENT_TOUCH_BEGIN;
  event.x = 5; /* outside button */
  event.y = 5;
  touch.pos.x = 115; /* inside button */
  touch.pos.y = 115;
  event.payload = &touch;
  EXPECT(capy_widget_handle_event(panel, &event) == 1);
  EXPECT(clicks == 1);
  capy_widget_destroy(panel);
}

static void test_event_gamepad_no_crash(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *button;
  struct capy_widget_event event = {CAPY_WIDGET_EVENT_NONE, 0, 0, 0u, 0u,
                                    0u, 0};
  struct capy_gamepad_payload gp = {0u, {0, 0, 0, 0}};
  capy_widget_context_init(&ctx, &allocator);
  button = capy_widget_create(&ctx, CAPY_WIDGET_BUTTON);
  EXPECT(button != 0);
  capy_widget_set_bounds(button, 0, 0, 50u, 20u);
  capy_widget_focus(button);
  event.type = CAPY_WIDGET_EVENT_GAMEPAD;
  gp.button_mask = 0x0001u;
  gp.axis[0] = 32000;
  event.payload = &gp;
  /* GAMEPAD reserved — must accept silently (return 0), never crash. */
  EXPECT(capy_widget_handle_event(button, &event) == 0);
  capy_widget_destroy(button);
}

static void test_event_unknown_type_ignored(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *button;
  struct capy_widget_event event = {CAPY_WIDGET_EVENT_NONE, 0, 0, 0u, 0u,
                                    0u, 0};
  capy_widget_context_init(&ctx, &allocator);
  button = capy_widget_create(&ctx, CAPY_WIDGET_BUTTON);
  EXPECT(button != 0);
  capy_widget_set_bounds(button, 0, 0, 50u, 20u);
  /* Synthesize an event type number beyond the known range (future ABI). */
  event.type = (enum capy_widget_event_type)((int)CAPY_WIDGET_EVENT_GAMEPAD +
                                             7);
  event.x = 10;
  event.y = 10;
  event.buttons = 1u;
  EXPECT(capy_widget_handle_event(button, &event) == 0);
  capy_widget_destroy(button);
}

static void test_event_modifier_flags_distinct(void) {
  /* CAPSLOCK and NUMLOCK joined the bitmask in 0.10; they must not overlap
   * with the existing CTRL/ALT/SHIFT/META flags or with each other. */
  uint32_t all = CAPY_MOD_CTRL | CAPY_MOD_ALT | CAPY_MOD_SHIFT |
                 CAPY_MOD_META | CAPY_MOD_CAPSLOCK | CAPY_MOD_NUMLOCK;
  EXPECT(CAPY_MOD_NONE == 0u);
  EXPECT(CAPY_MOD_CTRL != 0u);
  EXPECT(CAPY_MOD_ALT != 0u);
  EXPECT(CAPY_MOD_SHIFT != 0u);
  EXPECT(CAPY_MOD_META != 0u);
  EXPECT(CAPY_MOD_CAPSLOCK != 0u);
  EXPECT(CAPY_MOD_NUMLOCK != 0u);
  EXPECT((CAPY_MOD_CTRL & CAPY_MOD_ALT) == 0u);
  EXPECT((CAPY_MOD_CTRL & CAPY_MOD_SHIFT) == 0u);
  EXPECT((CAPY_MOD_CTRL & CAPY_MOD_META) == 0u);
  EXPECT((CAPY_MOD_CAPSLOCK & CAPY_MOD_NUMLOCK) == 0u);
  EXPECT((CAPY_MOD_CAPSLOCK &
          (CAPY_MOD_CTRL | CAPY_MOD_ALT | CAPY_MOD_SHIFT | CAPY_MOD_META)) ==
         0u);
  EXPECT((CAPY_MOD_NUMLOCK &
          (CAPY_MOD_CTRL | CAPY_MOD_ALT | CAPY_MOD_SHIFT | CAPY_MOD_META)) ==
         0u);
  /* The union of all flags fits in a uint32_t bitmask. */
  EXPECT(all == 0x3Fu);
}

/* ── v0.11: accessibility tree snapshot (since 0.11.0) ──────────────────── */

static int a11y_streq(const char *a, const char *b) {
  uint32_t i;
  if (!a || !b) {
    return 0;
  }
  for (i = 0u; a[i] != '\0' && b[i] != '\0'; ++i) {
    if (a[i] != b[i]) {
      return 0;
    }
  }
  return a[i] == b[i];
}

static void test_a11y_snapshot_basic(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *button;
  struct capy_a11y_node nodes[4];
  int rc;
  capy_widget_context_init(&ctx, &allocator);
  button = capy_widget_create(&ctx, CAPY_WIDGET_BUTTON);
  EXPECT(button != 0);
  capy_widget_set_text(button, "Save");
  rc = capy_widget_a11y_snapshot(button, nodes, 4u);
  EXPECT(rc == 1);
  EXPECT(nodes[0].parent_id == 0u);
  EXPECT(nodes[0].widget_id != 0u);
  EXPECT(a11y_streq(nodes[0].role, "button"));
  EXPECT(a11y_streq(nodes[0].label, "Save"));
  EXPECT(nodes[0].description == 0);
  EXPECT((nodes[0].state_flags & CAPY_A11Y_DISABLED) == 0u);
  EXPECT((nodes[0].state_flags & CAPY_A11Y_HIDDEN) == 0u);
  capy_widget_destroy(button);
}

static void test_a11y_snapshot_hierarchy(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *panel;
  struct capy_widget *button;
  struct capy_widget *checkbox;
  struct capy_a11y_node nodes[8];
  int rc;
  capy_widget_context_init(&ctx, &allocator);
  panel = capy_widget_create(&ctx, CAPY_WIDGET_PANEL);
  button = capy_widget_create(&ctx, CAPY_WIDGET_BUTTON);
  checkbox = capy_widget_create(&ctx, CAPY_WIDGET_CHECKBOX);
  EXPECT(panel && button && checkbox);
  capy_widget_set_text(button, "OK");
  capy_widget_set_text(checkbox, "Accept");
  EXPECT(capy_widget_add_child(panel, button) == 0);
  EXPECT(capy_widget_add_child(panel, checkbox) == 0);
  rc = capy_widget_a11y_snapshot(panel, nodes, 8u);
  EXPECT(rc == 3);
  /* Pre-order: panel, button, checkbox. */
  EXPECT(a11y_streq(nodes[0].role, "panel"));
  EXPECT(nodes[0].parent_id == 0u);
  EXPECT(a11y_streq(nodes[1].role, "button"));
  EXPECT(nodes[1].parent_id == nodes[0].widget_id);
  EXPECT(a11y_streq(nodes[2].role, "checkbox"));
  EXPECT(nodes[2].parent_id == nodes[0].widget_id);
  /* Sibling ids must differ. */
  EXPECT(nodes[1].widget_id != nodes[2].widget_id);
  capy_widget_destroy(panel);
}

static void test_a11y_roles_cover_focusable_types(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *root;
  struct capy_widget *children[6];
  enum capy_widget_type types[6] = {
      CAPY_WIDGET_BUTTON,    CAPY_WIDGET_TEXTBOX, CAPY_WIDGET_CHECKBOX,
      CAPY_WIDGET_LIST,      CAPY_WIDGET_MENU_ITEM, CAPY_WIDGET_TABS};
  const char *expected[6] = {"button", "textbox", "checkbox",
                             "list",   "menuitem", "tab"};
  struct capy_a11y_node nodes[16];
  int rc;
  uint32_t i;
  capy_widget_context_init(&ctx, &allocator);
  root = capy_widget_create(&ctx, CAPY_WIDGET_PANEL);
  EXPECT(root != 0);
  for (i = 0u; i < 6u; ++i) {
    children[i] = capy_widget_create(&ctx, types[i]);
    EXPECT(children[i] != 0);
    EXPECT(capy_widget_add_child(root, children[i]) == 0);
  }
  rc = capy_widget_a11y_snapshot(root, nodes, 16u);
  EXPECT(rc == 7);
  /* Children appear in indices 1..6 in pre-order. */
  for (i = 0u; i < 6u; ++i) {
    EXPECT(a11y_streq(nodes[i + 1u].role, expected[i]));
    EXPECT(nodes[i + 1u].role != 0);
  }
  capy_widget_destroy(root);
}

static void test_a11y_widget_id_stable_across_snapshots(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *panel;
  struct capy_widget *button;
  struct capy_a11y_node a[4];
  struct capy_a11y_node b[4];
  int rc_a;
  int rc_b;
  int i;
  capy_widget_context_init(&ctx, &allocator);
  panel = capy_widget_create(&ctx, CAPY_WIDGET_PANEL);
  button = capy_widget_create(&ctx, CAPY_WIDGET_BUTTON);
  EXPECT(panel && button);
  capy_widget_set_text(button, "X");
  EXPECT(capy_widget_add_child(panel, button) == 0);
  rc_a = capy_widget_a11y_snapshot(panel, a, 4u);
  rc_b = capy_widget_a11y_snapshot(panel, b, 4u);
  EXPECT(rc_a == 2);
  EXPECT(rc_a == rc_b);
  for (i = 0; i < rc_a; ++i) {
    EXPECT(a[i].widget_id == b[i].widget_id);
    EXPECT(a[i].parent_id == b[i].parent_id);
    EXPECT(a11y_streq(a[i].role, b[i].role));
    EXPECT(a11y_streq(a[i].label, b[i].label));
    EXPECT(a[i].state_flags == b[i].state_flags);
  }
  capy_widget_destroy(panel);
}

static void test_a11y_state_flags_match_widget_state(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *cb;
  struct capy_widget *tb;
  struct capy_widget *btn;
  struct capy_widget *root;
  struct capy_a11y_node nodes[16];
  int rc;
  capy_widget_context_init(&ctx, &allocator);
  root = capy_widget_create(&ctx, CAPY_WIDGET_PANEL);
  cb = capy_widget_create(&ctx, CAPY_WIDGET_CHECKBOX);
  tb = capy_widget_create(&ctx, CAPY_WIDGET_TEXTBOX);
  btn = capy_widget_create(&ctx, CAPY_WIDGET_BUTTON);
  EXPECT(root && cb && tb && btn);
  cb->checked = 1u;
  capy_widget_focus(cb);
  tb->text_edit.readonly = 1u;
  capy_widget_set_enabled(btn, 0);
  capy_widget_set_visible(btn, 0);
  EXPECT(capy_widget_add_child(root, cb) == 0);
  EXPECT(capy_widget_add_child(root, tb) == 0);
  EXPECT(capy_widget_add_child(root, btn) == 0);
  rc = capy_widget_a11y_snapshot(root, nodes, 16u);
  EXPECT(rc == 4);
  /* nodes[1]=checkbox, [2]=textbox, [3]=button (pre-order). */
  EXPECT((nodes[1].state_flags & CAPY_A11Y_FOCUSED) != 0u);
  EXPECT((nodes[1].state_flags & CAPY_A11Y_CHECKED) != 0u);
  EXPECT((nodes[2].state_flags & CAPY_A11Y_READONLY) != 0u);
  EXPECT((nodes[3].state_flags & CAPY_A11Y_DISABLED) != 0u);
  EXPECT((nodes[3].state_flags & CAPY_A11Y_HIDDEN) != 0u);
  capy_widget_destroy(root);
}

static void test_a11y_overflow_partial_fill_returns_minus_one(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *panel;
  struct capy_widget *button;
  struct capy_a11y_node nodes[1];
  int rc;
  capy_widget_context_init(&ctx, &allocator);
  panel = capy_widget_create(&ctx, CAPY_WIDGET_PANEL);
  button = capy_widget_create(&ctx, CAPY_WIDGET_BUTTON);
  EXPECT(panel && button);
  EXPECT(capy_widget_add_child(panel, button) == 0);
  rc = capy_widget_a11y_snapshot(panel, nodes, 1u);
  EXPECT(rc == -1);
  /* The root entry must still be visible in out[0] (partial fill). */
  EXPECT(a11y_streq(nodes[0].role, "panel"));
  EXPECT(nodes[0].parent_id == 0u);
  /* cap == 0 with non-NULL out → -1 (sizing query rejected). */
  EXPECT(capy_widget_a11y_snapshot(panel, nodes, 0u) == -1);
  /* NULL out with non-zero cap → -1. */
  EXPECT(capy_widget_a11y_snapshot(panel, 0, 4u) == -1);
  /* NULL root → -1. */
  EXPECT(capy_widget_a11y_snapshot(0, nodes, 4u) == -1);
  capy_widget_destroy(panel);
}

static void test_a11y_label_untitled_fallback(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *button;
  struct capy_a11y_node nodes[2];
  int rc;
  capy_widget_context_init(&ctx, &allocator);
  button = capy_widget_create(&ctx, CAPY_WIDGET_BUTTON);
  EXPECT(button != 0);
  /* No text set → label falls back to "Untitled". */
  rc = capy_widget_a11y_snapshot(button, nodes, 2u);
  EXPECT(rc == 1);
  EXPECT(a11y_streq(nodes[0].label, "Untitled"));
  /* Set text → label switches to widget->text. */
  capy_widget_set_text(button, "Click");
  rc = capy_widget_a11y_snapshot(button, nodes, 2u);
  EXPECT(rc == 1);
  EXPECT(a11y_streq(nodes[0].label, "Click"));
  capy_widget_destroy(button);
}

/* ── v0.13: locale + RTL + plural + format (since 0.13.0) ──────────────── */

static int locale_streq(const char *a, const char *b) {
  uint32_t i;
  if (!a || !b) {
    return 0;
  }
  for (i = 0u; a[i] != '\0' && b[i] != '\0'; ++i) {
    if (a[i] != b[i]) {
      return 0;
    }
  }
  return a[i] == b[i];
}

static void test_locale_default_seeds_context(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_locale d;
  capy_widget_context_init(&ctx, &allocator);
  d = capy_locale_default();
  EXPECT(locale_streq(d.tag, "en-US"));
  EXPECT(d.rtl == 0u);
  EXPECT(d.plural_rule == (uint8_t)CAPY_PLURAL_EN);
  /* context_init seeded the same default. */
  EXPECT(locale_streq(ctx.locale.tag, "en-US"));
  EXPECT(ctx.locale.rtl == 0u);
  EXPECT(ctx.locale.plural_rule == (uint8_t)CAPY_PLURAL_EN);
}

static void test_locale_set_null_restores_default(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_locale ar;
  uint32_t i;
  capy_widget_context_init(&ctx, &allocator);
  /* Set a non-default locale. */
  for (i = 0u; i < sizeof(ar.tag); ++i) {
    ar.tag[i] = '\0';
  }
  ar.tag[0] = 'a';
  ar.tag[1] = 'r';
  ar.tag[2] = '-';
  ar.tag[3] = 'E';
  ar.tag[4] = 'G';
  ar.rtl = 1u;
  ar.plural_rule = (uint8_t)CAPY_PLURAL_AR;
  ar.reserved = 0u;
  capy_context_set_locale(&ctx, &ar);
  EXPECT(locale_streq(ctx.locale.tag, "ar-EG"));
  EXPECT(ctx.locale.rtl == 1u);
  EXPECT(ctx.locale.plural_rule == (uint8_t)CAPY_PLURAL_AR);
  /* Now restore default with NULL. */
  capy_context_set_locale(&ctx, 0);
  EXPECT(locale_streq(ctx.locale.tag, "en-US"));
  EXPECT(ctx.locale.rtl == 0u);
  EXPECT(ctx.locale.plural_rule == (uint8_t)CAPY_PLURAL_EN);
}

static void test_locale_plural_en_pt(void) {
  struct capy_locale en = capy_locale_default();
  struct capy_locale pt = en;
  pt.plural_rule = (uint8_t)CAPY_PLURAL_PT;
  /* EN: one (idx 0) only when n==1, else other (idx 1). */
  EXPECT(capy_locale_plural(&en, 0u) == 1);
  EXPECT(capy_locale_plural(&en, 1u) == 0);
  EXPECT(capy_locale_plural(&en, 2u) == 1);
  EXPECT(capy_locale_plural(&en, 21u) == 1);
  /* PT shares the same simple rule. */
  EXPECT(capy_locale_plural(&pt, 0u) == 1);
  EXPECT(capy_locale_plural(&pt, 1u) == 0);
  EXPECT(capy_locale_plural(&pt, 5u) == 1);
  /* NULL locale falls back to EN. */
  EXPECT(capy_locale_plural(0, 1u) == 0);
  EXPECT(capy_locale_plural(0, 7u) == 1);
}

static void test_locale_plural_ar(void) {
  struct capy_locale ar = capy_locale_default();
  ar.plural_rule = (uint8_t)CAPY_PLURAL_AR;
  /* zero=0, one=1, two=2, few=3, many=4, other=5. */
  EXPECT(capy_locale_plural(&ar, 0u) == 0);
  EXPECT(capy_locale_plural(&ar, 1u) == 1);
  EXPECT(capy_locale_plural(&ar, 2u) == 2);
  EXPECT(capy_locale_plural(&ar, 5u) == 3);   /* mod100 in [3..10] → few */
  EXPECT(capy_locale_plural(&ar, 21u) == 4);  /* mod100 in [11..99] → many */
  EXPECT(capy_locale_plural(&ar, 100u) == 5); /* other */
}

static void test_locale_plural_ru(void) {
  struct capy_locale ru = capy_locale_default();
  ru.plural_rule = (uint8_t)CAPY_PLURAL_RU;
  /* one=0, few=1, many=2, other=3 — but our impl only produces 0/1/2. */
  EXPECT(capy_locale_plural(&ru, 1u) == 0);   /* one */
  EXPECT(capy_locale_plural(&ru, 21u) == 0);  /* mod10=1, mod100!=11 → one */
  EXPECT(capy_locale_plural(&ru, 2u) == 1);   /* few */
  EXPECT(capy_locale_plural(&ru, 4u) == 1);   /* few */
  EXPECT(capy_locale_plural(&ru, 11u) == 2);  /* many (special) */
  EXPECT(capy_locale_plural(&ru, 5u) == 2);   /* many */
  EXPECT(capy_locale_plural(&ru, 0u) == 2);   /* many */
  EXPECT(capy_locale_plural(&ru, 100u) == 2); /* many */
}

static void test_locale_format_subset(void) {
  struct capy_locale en = capy_locale_default();
  char buf[64];
  int rc;
  /* %d signed, including negative + INT32_MIN safe path. */
  rc = capy_locale_format(&en, buf, sizeof(buf), "n=%d", -42);
  EXPECT(rc > 0);
  EXPECT(locale_streq(buf, "n=-42"));
  rc = capy_locale_format(&en, buf, sizeof(buf), "%d", (int)0);
  EXPECT(rc == 1);
  EXPECT(locale_streq(buf, "0"));
  /* %u unsigned. */
  rc = capy_locale_format(&en, buf, sizeof(buf), "%u files", 7u);
  EXPECT(rc > 0);
  EXPECT(locale_streq(buf, "7 files"));
  /* %x lowercase hex. */
  rc = capy_locale_format(&en, buf, sizeof(buf), "0x%x", 0xabc12u);
  EXPECT(rc > 0);
  EXPECT(locale_streq(buf, "0xabc12"));
  /* %s. */
  rc = capy_locale_format(&en, buf, sizeof(buf), "hi %s!", "world");
  EXPECT(rc > 0);
  EXPECT(locale_streq(buf, "hi world!"));
  /* %% literal. */
  rc = capy_locale_format(&en, buf, sizeof(buf), "100%% sure");
  EXPECT(rc > 0);
  EXPECT(locale_streq(buf, "100% sure"));
  /* Mixed with all specifiers in one go. */
  rc = capy_locale_format(&en, buf, sizeof(buf), "%d/%u %s 0x%x %%",
                          -1, 2u, "ok", 0xfu);
  EXPECT(rc > 0);
  EXPECT(locale_streq(buf, "-1/2 ok 0xf %"));
}

static void test_locale_format_overflow_returns_minus_one(void) {
  struct capy_locale en = capy_locale_default();
  char buf[8];
  int rc;
  /* "%d items" with n=12345 = 13 chars + NUL; cap=8 → overflow. */
  rc = capy_locale_format(&en, buf, sizeof(buf), "%d items", 12345);
  EXPECT(rc == -1);
  /* NULL out / zero cap / NULL fmt all fail closed. */
  EXPECT(capy_locale_format(&en, 0, 16u, "x") == -1);
  EXPECT(capy_locale_format(&en, buf, 0u, "x") == -1);
  EXPECT(capy_locale_format(&en, buf, sizeof(buf), 0) == -1);
  /* Trailing lone '%' is rejected. */
  EXPECT(capy_locale_format(&en, buf, sizeof(buf), "oops %") == -1);
  /* Unknown specifier is rejected. */
  EXPECT(capy_locale_format(&en, buf, sizeof(buf), "%q", 1) == -1);
}

static void test_layout_rtl_mirrors_horizontal_stack(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *root;
  struct capy_widget *a;
  struct capy_widget *b;
  struct capy_layout_node layout = {CAPY_LAYOUT_STACK,
                                    {0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u,
                                     0u, 0u},
                                    0u, 0u};
  int32_t a_x_first;
  int32_t b_x_first;
  capy_widget_context_init(&ctx, &allocator);
  root = capy_widget_create(&ctx, CAPY_WIDGET_PANEL);
  a = capy_widget_create(&ctx, CAPY_WIDGET_BUTTON);
  b = capy_widget_create(&ctx, CAPY_WIDGET_BUTTON);
  EXPECT(root && a && b);
  capy_widget_set_bounds(root, 0, 0, 100u, 50u);
  capy_widget_set_bounds(a, 0, 0, 30u, 50u);
  capy_widget_set_bounds(b, 0, 0, 30u, 50u);
  EXPECT(capy_widget_add_child(root, a) == 0);
  EXPECT(capy_widget_add_child(root, b) == 0);
  layout.c.axis = 0u; /* horizontal */
  layout.c.rtl = 1u;
  capy_widget_set_layout(root, &layout);
  EXPECT(capy_widget_arrange(root) == 0);
  /* In LTR: a at x=0, b at x=30. In RTL: mirror within [0, 100):
   * a (old=0,w=30) → x = 0 + 100 - 0 - 30 = 70.
   * b (old=30,w=30) → x = 0 + 100 - 30 - 30 = 40. */
  EXPECT(a->bounds.x == 70);
  EXPECT(b->bounds.x == 40);
  /* Heights/y unchanged. */
  EXPECT(a->bounds.y == 0);
  EXPECT(b->bounds.y == 0);
  /* Determinism: arrange twice → same positions. */
  a_x_first = a->bounds.x;
  b_x_first = b->bounds.x;
  EXPECT(capy_widget_arrange(root) == 0);
  EXPECT(a->bounds.x == a_x_first);
  EXPECT(b->bounds.x == b_x_first);
  capy_widget_destroy(root);
}

static void test_layout_rtl_vertical_stack_x_unchanged(void) {
  /* Vertical stack uses cross = iw; mirror is then a no-op for child x. */
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *root;
  struct capy_widget *a;
  struct capy_widget *b;
  struct capy_layout_node layout = {CAPY_LAYOUT_STACK,
                                    {0u, 0u, 0u, 0u, 1u, 0u, 0u, 0u, 0u, 0u,
                                     0u, 1u},
                                    0u, 0u};
  capy_widget_context_init(&ctx, &allocator);
  root = capy_widget_create(&ctx, CAPY_WIDGET_PANEL);
  a = capy_widget_create(&ctx, CAPY_WIDGET_BUTTON);
  b = capy_widget_create(&ctx, CAPY_WIDGET_BUTTON);
  EXPECT(root && a && b);
  capy_widget_set_bounds(root, 0, 0, 100u, 100u);
  capy_widget_set_bounds(a, 0, 0, 40u, 30u);
  capy_widget_set_bounds(b, 0, 0, 40u, 30u);
  EXPECT(capy_widget_add_child(root, a) == 0);
  EXPECT(capy_widget_add_child(root, b) == 0);
  capy_widget_set_layout(root, &layout);
  EXPECT(capy_widget_arrange(root) == 0);
  /* Vertical: cross width = iw = 100, so child.width gets stretched to 100
   * and x stays 0. Mirror with width==iw yields the same x. */
  EXPECT(a->bounds.x == 0);
  EXPECT(b->bounds.x == 0);
  EXPECT(a->bounds.width == 100u);
  EXPECT(b->bounds.width == 100u);
  capy_widget_destroy(root);
}

/* ── v0.14: theme serialize/deserialize (since 0.14.0) ─────────────────── */

static int theme_tokens_equal(const struct capy_theme *a,
                              const struct capy_theme *b) {
  uint32_t i;
  for (i = 0u; i < (uint32_t)CAPY_TOKEN_COUNT; ++i) {
    if (a->tokens[i] != b->tokens[i]) {
      return 0;
    }
  }
  return a->variant == b->variant && a->high_contrast == b->high_contrast;
}

static int theme_buf_streq(const char *a, const char *b) {
  uint32_t i;
  for (i = 0u; a[i] != '\0' && b[i] != '\0'; ++i) {
    if (a[i] != b[i]) {
      return 0;
    }
  }
  return a[i] == b[i];
}

static void test_theme_serialize_round_trip(void) {
  struct capy_theme original = capy_theme_default_dark();
  struct capy_theme restored;
  char buf[1024];
  int rc = capy_theme_serialize(&original, buf, sizeof(buf));
  EXPECT(rc > 0);
  EXPECT(rc < (int)sizeof(buf));
  /* Output is NUL-terminated. */
  EXPECT(buf[rc] == '\0');
  /* First lines match the canonical opening. */
  EXPECT(buf[0] == 'v' && buf[1] == 'e' && buf[2] == 'r' && buf[7] == '=');
  /* Round-trip preserves tokens + variant + high_contrast. */
  EXPECT(capy_theme_deserialize(&restored, buf, (uint32_t)rc) == 0);
  EXPECT(theme_tokens_equal(&original, &restored));
  EXPECT(restored.version == (uint16_t)CAPY_THEME_FORMAT_VERSION);
}

static void test_theme_serialize_determinism(void) {
  struct capy_theme t = capy_theme_default_light();
  char buf_a[1024];
  char buf_b[1024];
  int rc_a = capy_theme_serialize(&t, buf_a, sizeof(buf_a));
  int rc_b = capy_theme_serialize(&t, buf_b, sizeof(buf_b));
  EXPECT(rc_a > 0);
  EXPECT(rc_a == rc_b);
  EXPECT(theme_buf_streq(buf_a, buf_b));
}

static void test_theme_serialize_overflow_returns_minus_one(void) {
  struct capy_theme t = capy_theme_default_light();
  char tiny[8];
  EXPECT(capy_theme_serialize(&t, tiny, sizeof(tiny)) == -1);
  /* NULL guards. */
  EXPECT(capy_theme_serialize(0, tiny, sizeof(tiny)) == -1);
  EXPECT(capy_theme_serialize(&t, 0, sizeof(tiny)) == -1);
  EXPECT(capy_theme_serialize(&t, tiny, 0u) == -1);
  /* Invalid variant rejected. */
  t.variant = 99u;
  {
    char buf[256];
    EXPECT(capy_theme_serialize(&t, buf, sizeof(buf)) == -1);
  }
}

static void test_theme_deserialize_rejects_future_version(void) {
  struct capy_theme out;
  const char *input = "version=3\nvariant=light\nhigh_contrast=0\n";
  EXPECT(capy_theme_deserialize(&out, input,
                                (uint32_t)sizeof("version=3\nvariant=light\nhigh_contrast=0\n") -
                                    1u) == -1);
}

static void test_theme_deserialize_rejects_malformed(void) {
  struct capy_theme out;
  /* No '=' on a non-empty/non-comment line. */
  {
    const char *bad = "version=2\nthis-line-has-no-eq\n";
    EXPECT(capy_theme_deserialize(&out, bad,
                                  (uint32_t)sizeof(
                                      "version=2\nthis-line-has-no-eq\n") -
                                      1u) == -1);
  }
  /* Bad hex value. */
  {
    const char *bad = "version=2\nbg_base=NOTHEX\n";
    EXPECT(capy_theme_deserialize(&out, bad,
                                  (uint32_t)sizeof(
                                      "version=2\nbg_base=NOTHEX\n") -
                                      1u) == -1);
  }
  /* Invalid variant value. */
  {
    const char *bad = "version=2\nvariant=fuchsia\n";
    EXPECT(capy_theme_deserialize(&out, bad,
                                  (uint32_t)sizeof(
                                      "version=2\nvariant=fuchsia\n") -
                                      1u) == -1);
  }
  /* Bad boolean for high_contrast. */
  {
    const char *bad = "version=2\nhigh_contrast=2\n";
    EXPECT(capy_theme_deserialize(&out, bad,
                                  (uint32_t)sizeof(
                                      "version=2\nhigh_contrast=2\n") -
                                      1u) == -1);
  }
  /* Missing version=. */
  {
    const char *bad = "variant=light\nhigh_contrast=0\n";
    EXPECT(capy_theme_deserialize(&out, bad,
                                  (uint32_t)sizeof(
                                      "variant=light\nhigh_contrast=0\n") -
                                      1u) == -1);
  }
  /* NULL guards. */
  EXPECT(capy_theme_deserialize(0, "version=2\n", 10u) == -1);
  EXPECT(capy_theme_deserialize(&out, 0, 10u) == -1);
}

static void test_theme_deserialize_ignores_unknown_keys(void) {
  struct capy_theme out;
  const char *input =
      "# canonical theme\n"
      "version=2\n"
      "variant=dark\n"
      "high_contrast=0\n"
      "bg_base=0xFF010203\n"
      "future_token_v9=0xDEADBEEF\n" /* unknown — must be ignored */
      "fg_primary=0xFF040506\n";
  uint32_t len = 0u;
  while (input[len] != '\0') {
    ++len;
  }
  EXPECT(capy_theme_deserialize(&out, input, len) == 0);
  EXPECT(out.variant == (uint8_t)CAPY_THEME_VARIANT_DARK);
  EXPECT(out.tokens[CAPY_TOKEN_BG_BASE] == 0xFF010203u);
  EXPECT(out.tokens[CAPY_TOKEN_FG_PRIMARY] == 0xFF040506u);
  /* Missing tokens stay zero (per contract). */
  EXPECT(out.tokens[CAPY_TOKEN_BG_RAISED] == 0u);
}

static void test_theme_deserialize_tolerates_whitespace_and_comments(void) {
  struct capy_theme out;
  const char *input =
      "# leading comment line\n"
      "\n"
      "   version  =  2  \n"
      "variant=light\n"
      "   high_contrast =0\n"
      "  bg_base=  0xFFAABBCC  \n";
  uint32_t len = 0u;
  while (input[len] != '\0') {
    ++len;
  }
  EXPECT(capy_theme_deserialize(&out, input, len) == 0);
  EXPECT(out.variant == (uint8_t)CAPY_THEME_VARIANT_LIGHT);
  EXPECT(out.high_contrast == 0u);
  EXPECT(out.tokens[CAPY_TOKEN_BG_BASE] == 0xFFAABBCCu);
  EXPECT(out.version == 2u);
}

/* ── v0.15: pool allocator + measure cache (since 0.15.0) ──────────────── */

static void test_pool_sequential_allocations(void) {
  _Alignas(16) unsigned char buf[4096];
  struct capy_widget_pool pool;
  void *a;
  void *b;
  void *c;
  capy_widget_pool_init(&pool, buf, sizeof(buf));
  EXPECT(pool.buffer == buf);
  EXPECT(pool.size == sizeof(buf));
  EXPECT(pool.used == 0u);
  EXPECT(pool.high_water == 0u);
  a = pool.buffer; /* expected base */
  (void)a;
  /* Allocate a tiny block, then a larger one. */
  {
    struct capy_widget_allocator alloc = capy_widget_allocator_from_pool(&pool);
    void *p1 = alloc.alloc(32u, alloc.user_data);
    void *p2 = alloc.alloc(128u, alloc.user_data);
    EXPECT(p1 != 0);
    EXPECT(p2 != 0);
    /* p2 must come after p1 + at least 32 bytes (with 16-byte alignment). */
    EXPECT((unsigned char *)p2 > (unsigned char *)p1);
    EXPECT((unsigned char *)p2 >= (unsigned char *)p1 + 32);
    /* used must reflect the bump cursor. */
    EXPECT(pool.used >= 32u + 128u);
    EXPECT(pool.high_water == pool.used);
    /* free is a no-op but must not crash or change used. */
    {
      uint32_t before = pool.used;
      alloc.free(p1, alloc.user_data);
      alloc.free(p2, alloc.user_data);
      EXPECT(pool.used == before);
    }
    b = p1;
    c = p2;
  }
  (void)b;
  (void)c;
}

static void test_pool_overflow_returns_null(void) {
  _Alignas(16) unsigned char buf[64];
  struct capy_widget_pool pool;
  struct capy_widget_allocator alloc;
  void *p1;
  void *p2;
  capy_widget_pool_init(&pool, buf, sizeof(buf));
  alloc = capy_widget_allocator_from_pool(&pool);
  p1 = alloc.alloc(32u, alloc.user_data);
  EXPECT(p1 != 0);
  /* Second 64-byte alloc exceeds remaining capacity. */
  p2 = alloc.alloc(64u, alloc.user_data);
  EXPECT(p2 == 0);
  /* Pool state is intact after the failed alloc. */
  EXPECT(pool.used <= pool.size);
  EXPECT(pool.high_water == pool.used);
  /* NULL/zero edge cases. */
  EXPECT(alloc.alloc(0u, alloc.user_data) == 0);
  EXPECT(alloc.alloc(8u, 0) == 0);
}

static void test_pool_reset_keeps_high_water(void) {
  _Alignas(16) unsigned char buf[256];
  struct capy_widget_pool pool;
  struct capy_widget_allocator alloc;
  uint32_t hw_before;
  capy_widget_pool_init(&pool, buf, sizeof(buf));
  alloc = capy_widget_allocator_from_pool(&pool);
  EXPECT(alloc.alloc(128u, alloc.user_data) != 0);
  EXPECT(alloc.alloc(64u, alloc.user_data) != 0);
  hw_before = pool.high_water;
  EXPECT(hw_before > 0u);
  capy_widget_pool_reset(&pool);
  EXPECT(pool.used == 0u);
  EXPECT(pool.high_water == hw_before); /* preserved for diagnostics */
  /* After reset we can allocate again from scratch. */
  EXPECT(alloc.alloc(96u, alloc.user_data) != 0);
}

static void test_pool_alignment_16(void) {
  _Alignas(16) unsigned char buf[512];
  struct capy_widget_pool pool;
  struct capy_widget_allocator alloc;
  void *p1;
  void *p2;
  void *p3;
  capy_widget_pool_init(&pool, buf, sizeof(buf));
  alloc = capy_widget_allocator_from_pool(&pool);
  p1 = alloc.alloc(1u, alloc.user_data);  /* odd size */
  p2 = alloc.alloc(7u, alloc.user_data);  /* odd size */
  p3 = alloc.alloc(15u, alloc.user_data); /* odd size */
  EXPECT(p1 && p2 && p3);
  /* All returned pointers are 16-byte aligned. */
  EXPECT(((uintptr_t)p1 & 15u) == 0u);
  EXPECT(((uintptr_t)p2 & 15u) == 0u);
  EXPECT(((uintptr_t)p3 & 15u) == 0u);
}

static void test_pool_backs_widget_create(void) {
  _Alignas(16) unsigned char buf[4096];
  struct capy_widget_pool pool;
  struct capy_widget_allocator alloc;
  struct capy_widget_context ctx;
  struct capy_widget *root;
  struct capy_widget *child;
  capy_widget_pool_init(&pool, buf, sizeof(buf));
  alloc = capy_widget_allocator_from_pool(&pool);
  capy_widget_context_init(&ctx, &alloc);
  root = capy_widget_create(&ctx, CAPY_WIDGET_PANEL);
  child = capy_widget_create(&ctx, CAPY_WIDGET_BUTTON);
  EXPECT(root != 0);
  EXPECT(child != 0);
  EXPECT(capy_widget_add_child(root, child) == 0);
  /* The pool absorbed both widget allocations. */
  EXPECT(pool.used > 0u);
  EXPECT(pool.high_water == pool.used);
  /* Destroy is a no-op for the pool (free does nothing); used stays put. */
  {
    uint32_t before = pool.used;
    capy_widget_destroy(root); /* recurses, calls free (no-op) */
    EXPECT(pool.used == before);
    EXPECT(pool.high_water == before);
  }
  /* Pool reset reclaims everything for a fresh tree. */
  capy_widget_pool_reset(&pool);
  EXPECT(pool.used == 0u);
}

static void test_layout_measure_cache_short_circuit(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *root;
  uint32_t v_after_first;
  capy_widget_context_init(&ctx, &allocator);
  root = capy_widget_create(&ctx, CAPY_WIDGET_PANEL);
  EXPECT(root != 0);
  EXPECT(root->layout_dirty == 1u);
  EXPECT(root->layout_version == 0u);
  /* First measure does real work: dirty cleared, version bumped to 1. */
  EXPECT(capy_widget_measure(root, 200u, 150u) == 0);
  EXPECT(root->bounds.width == 200u);
  EXPECT(root->bounds.height == 150u);
  EXPECT(root->layout_dirty == 0u);
  EXPECT(root->layout_version == 1u);
  v_after_first = root->layout_version;
  /* Second measure with same args is a cache hit: version unchanged. */
  EXPECT(capy_widget_measure(root, 200u, 150u) == 0);
  EXPECT(root->layout_version == v_after_first);
  /* Changing avail forces recompute → version bump. */
  EXPECT(capy_widget_measure(root, 300u, 150u) == 0);
  EXPECT(root->bounds.width == 300u);
  EXPECT(root->layout_version == v_after_first + 1u);
  capy_widget_destroy(root);
}

static void test_invalidate_walks_up_parents(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *root;
  struct capy_widget *middle;
  struct capy_widget *leaf;
  uint32_t root_v;
  capy_widget_context_init(&ctx, &allocator);
  root = capy_widget_create(&ctx, CAPY_WIDGET_PANEL);
  middle = capy_widget_create(&ctx, CAPY_WIDGET_PANEL);
  leaf = capy_widget_create(&ctx, CAPY_WIDGET_BUTTON);
  EXPECT(root && middle && leaf);
  EXPECT(capy_widget_add_child(root, middle) == 0);
  EXPECT(capy_widget_add_child(middle, leaf) == 0);
  /* Drain dirty flags via initial measure on each so we can observe
   * invalidate behaviour cleanly. */
  EXPECT(capy_widget_measure(root, 100u, 100u) == 0);
  EXPECT(capy_widget_measure(middle, 100u, 100u) == 0);
  EXPECT(capy_widget_measure(leaf, 50u, 30u) == 0);
  EXPECT(root->layout_dirty == 0u);
  EXPECT(middle->layout_dirty == 0u);
  EXPECT(leaf->layout_dirty == 0u);
  root_v = root->layout_version;
  /* Invalidate from leaf walks up: leaf, middle, root all become dirty. */
  capy_widget_invalidate(leaf);
  EXPECT(leaf->layout_dirty == 1u);
  EXPECT(middle->layout_dirty == 1u);
  EXPECT(root->layout_dirty == 1u);
  /* Re-measure root with same args still recomputes because dirty is set. */
  EXPECT(capy_widget_measure(root, 100u, 100u) == 0);
  EXPECT(root->layout_version == root_v + 1u);
  EXPECT(root->layout_dirty == 0u);
  /* NULL guard. */
  capy_widget_invalidate(0);
  capy_widget_destroy(root);
}

/* ── v1.1: damage tracking + diff_incremental + frame budget (since 1.1.0) ─ */

static void test_invalidate_bumps_dirty_version(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *root;
  struct capy_widget *child;
  uint32_t root_dv;
  uint32_t child_dv;
  capy_widget_context_init(&ctx, &allocator);
  root = capy_widget_create(&ctx, CAPY_WIDGET_PANEL);
  child = capy_widget_create(&ctx, CAPY_WIDGET_BUTTON);
  EXPECT(root && child);
  EXPECT(capy_widget_add_child(root, child) == 0);
  EXPECT(root->dirty_version == 0u);
  EXPECT(child->dirty_version == 0u);
  root_dv = root->dirty_version;
  child_dv = child->dirty_version;
  capy_widget_invalidate(child);
  /* Both walked widgets (child + its parent root) must bump. */
  EXPECT(child->dirty_version == child_dv + 1u);
  EXPECT(root->dirty_version == root_dv + 1u);
  /* Layout dirty flag also still propagates (0.15 behaviour preserved). */
  EXPECT(child->layout_dirty == 1u);
  EXPECT(root->layout_dirty == 1u);
  capy_widget_destroy(root);
}

static void test_invalidate_subtree_walks_down(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *root;
  struct capy_widget *middle;
  struct capy_widget *leaf_a;
  struct capy_widget *leaf_b;
  uint32_t root_dv;
  uint32_t middle_dv;
  uint32_t leaf_a_dv;
  uint32_t leaf_b_dv;
  capy_widget_context_init(&ctx, &allocator);
  root = capy_widget_create(&ctx, CAPY_WIDGET_PANEL);
  middle = capy_widget_create(&ctx, CAPY_WIDGET_PANEL);
  leaf_a = capy_widget_create(&ctx, CAPY_WIDGET_BUTTON);
  leaf_b = capy_widget_create(&ctx, CAPY_WIDGET_BUTTON);
  EXPECT(root && middle && leaf_a && leaf_b);
  EXPECT(capy_widget_add_child(root, middle) == 0);
  EXPECT(capy_widget_add_child(middle, leaf_a) == 0);
  EXPECT(capy_widget_add_child(middle, leaf_b) == 0);
  /* Drain dirty flags from the create-time defaults. */
  root->layout_dirty = 0u;
  middle->layout_dirty = 0u;
  leaf_a->layout_dirty = 0u;
  leaf_b->layout_dirty = 0u;
  root_dv = root->dirty_version;
  middle_dv = middle->dirty_version;
  leaf_a_dv = leaf_a->dirty_version;
  leaf_b_dv = leaf_b->dirty_version;
  capy_widget_invalidate_subtree(middle);
  /* middle and both leaves bumped; root NOT touched (subtree, not parents). */
  EXPECT(middle->dirty_version == middle_dv + 1u);
  EXPECT(leaf_a->dirty_version == leaf_a_dv + 1u);
  EXPECT(leaf_b->dirty_version == leaf_b_dv + 1u);
  EXPECT(root->dirty_version == root_dv);
  EXPECT(middle->layout_dirty == 1u);
  EXPECT(leaf_a->layout_dirty == 1u);
  EXPECT(leaf_b->layout_dirty == 1u);
  EXPECT(root->layout_dirty == 0u);
  /* NULL guard. */
  capy_widget_invalidate_subtree(0);
  capy_widget_destroy(root);
}

static void test_dirty_version_accessor(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *w;
  capy_widget_context_init(&ctx, &allocator);
  w = capy_widget_create(&ctx, CAPY_WIDGET_LABEL);
  EXPECT(w != 0);
  EXPECT(capy_widget_dirty_version(w) == 0u);
  capy_widget_invalidate(w);
  EXPECT(capy_widget_dirty_version(w) == 1u);
  capy_widget_invalidate(w);
  EXPECT(capy_widget_dirty_version(w) == 2u);
  /* NULL → 0 sentinel. */
  EXPECT(capy_widget_dirty_version(0) == 0u);
  capy_widget_destroy(w);
}

static void test_frame_budget_round_trip(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  capy_widget_context_init(&ctx, &allocator);
  EXPECT(ctx.frame_budget_microseconds == 0u);
  capy_widget_frame_budget(&ctx, 16666u);
  EXPECT(ctx.frame_budget_microseconds == 16666u);
  capy_widget_frame_budget(&ctx, 0u);
  EXPECT(ctx.frame_budget_microseconds == 0u);
  /* NULL guard: must not crash and must not write through. */
  capy_widget_frame_budget(0, 9999u);
}

static void test_diff_incremental_matches_diff_identical(void) {
  /* When prev == next, both diff variants must produce zero dirty rects. */
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *root;
  struct capy_widget *button;
  struct capy_dl_cmd cmd_a[32];
  struct capy_dl_cmd cmd_b[32];
  char text_a[32];
  char text_b[32];
  struct capy_display_list prev;
  struct capy_display_list next;
  struct capy_ui_rect dirty_inc[16];
  struct capy_ui_rect dirty_full[16];
  int rc_inc;
  int rc_full;
  capy_widget_context_init(&ctx, &allocator);
  root = capy_widget_create(&ctx, CAPY_WIDGET_PANEL);
  button = capy_widget_create(&ctx, CAPY_WIDGET_BUTTON);
  EXPECT(root && button);
  capy_widget_set_bounds(root, 0, 0, 100u, 100u);
  capy_widget_set_bounds(button, 10, 10, 20u, 20u);
  EXPECT(capy_widget_add_child(root, button) == 0);
  capy_display_list_init(&prev, cmd_a, 32u, text_a, 32u);
  capy_display_list_init(&next, cmd_b, 32u, text_b, 32u);
  EXPECT(capy_widget_emit(root, &prev) == 0);
  EXPECT(capy_widget_emit(root, &next) == 0);
  rc_inc = capy_display_list_diff_incremental(&prev, &next, dirty_inc, 16u);
  rc_full = capy_widget_diff(&prev, &next, dirty_full, 16u);
  EXPECT(rc_inc == 0);
  EXPECT(rc_full == 0);
  EXPECT(rc_inc == rc_full);
}

static void test_diff_incremental_matches_diff_trailing_change(void) {
  /* When only a later cmd differs, incremental still produces the same dirty
   * rect sequence as the full positional diff (byte-identical). */
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *root;
  struct capy_widget *button;
  struct capy_dl_cmd cmd_a[32];
  struct capy_dl_cmd cmd_b[32];
  char text_a[32];
  char text_b[32];
  struct capy_display_list prev;
  struct capy_display_list next;
  struct capy_ui_rect dirty_inc[16];
  struct capy_ui_rect dirty_full[16];
  int rc_inc;
  int rc_full;
  int i;
  capy_widget_context_init(&ctx, &allocator);
  root = capy_widget_create(&ctx, CAPY_WIDGET_PANEL);
  button = capy_widget_create(&ctx, CAPY_WIDGET_BUTTON);
  EXPECT(root && button);
  capy_widget_set_bounds(root, 0, 0, 200u, 100u);
  capy_widget_set_bounds(button, 50, 30, 40u, 30u);
  button->style.bg_color = 0xFF111111u;
  EXPECT(capy_widget_add_child(root, button) == 0);
  capy_display_list_init(&prev, cmd_a, 32u, text_a, 32u);
  EXPECT(capy_widget_emit(root, &prev) == 0);
  button->style.bg_color = 0xFFEEEEEEu;
  capy_display_list_init(&next, cmd_b, 32u, text_b, 32u);
  EXPECT(capy_widget_emit(root, &next) == 0);
  rc_inc = capy_display_list_diff_incremental(&prev, &next, dirty_inc, 16u);
  rc_full = capy_widget_diff(&prev, &next, dirty_full, 16u);
  EXPECT(rc_inc == rc_full);
  EXPECT(rc_inc > 0);
  /* Sequence equality, byte-for-byte. */
  for (i = 0; i < rc_inc; ++i) {
    EXPECT(dirty_inc[i].x == dirty_full[i].x);
    EXPECT(dirty_inc[i].y == dirty_full[i].y);
    EXPECT(dirty_inc[i].width == dirty_full[i].width);
    EXPECT(dirty_inc[i].height == dirty_full[i].height);
  }
  capy_widget_destroy(root);
}

static void test_diff_incremental_overflow_returns_minus_one(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *root;
  struct capy_widget *a;
  struct capy_widget *b;
  struct capy_widget *root2;
  struct capy_dl_cmd cmd_a[32];
  struct capy_dl_cmd cmd_b[32];
  char text_a[32];
  char text_b[32];
  struct capy_display_list prev;
  struct capy_display_list next;
  struct capy_ui_rect dirty[1];
  capy_widget_context_init(&ctx, &allocator);
  root = capy_widget_create(&ctx, CAPY_WIDGET_PANEL);
  root2 = capy_widget_create(&ctx, CAPY_WIDGET_PANEL);
  a = capy_widget_create(&ctx, CAPY_WIDGET_BUTTON);
  b = capy_widget_create(&ctx, CAPY_WIDGET_BUTTON);
  EXPECT(root && root2 && a && b);
  capy_widget_set_bounds(root, 0, 0, 100u, 100u);
  capy_widget_set_bounds(root2, 0, 0, 100u, 100u);
  capy_widget_set_bounds(a, 10, 10, 20u, 20u);
  capy_widget_set_bounds(b, 50, 50, 20u, 20u);
  EXPECT(capy_widget_add_child(root, a) == 0);
  EXPECT(capy_widget_add_child(root2, b) == 0);
  capy_display_list_init(&prev, cmd_a, 32u, text_a, 32u);
  capy_display_list_init(&next, cmd_b, 32u, text_b, 32u);
  EXPECT(capy_widget_emit(root, &prev) == 0);
  EXPECT(capy_widget_emit(root2, &next) == 0);
  /* Two disjoint button bounds → ≥2 unique rects; cap=1 → overflow. */
  EXPECT(capy_display_list_diff_incremental(&prev, &next, dirty, 1u) == -1);
  /* NULL out with non-zero cap → -1. */
  EXPECT(capy_display_list_diff_incremental(&prev, &next, 0, 4u) == -1);
  /* NULL lists treated as empty: identical empties → 0. */
  EXPECT(capy_display_list_diff_incremental(0, 0, dirty, 1u) == 0);
  capy_widget_destroy(root);
  capy_widget_destroy(root2);
}

/* ── v1.4: gesture engine (since 1.4.0) ─────────────────────────────────── */

static void gesture_make_touch(struct capy_widget_event *ev,
                               struct capy_touch_payload *p,
                               enum capy_widget_event_type type,
                               uint32_t touch_id, int32_t x, int32_t y) {
  ev->type = type;
  ev->x = x;
  ev->y = y;
  ev->buttons = 0u;
  ev->keycode = 0u;
  ev->modifiers = 0u;
  p->touch_id = touch_id;
  p->pos.x = x;
  p->pos.y = y;
  p->pressure_x256 = 256u;
  ev->payload = p;
}

static void test_gesture_tap_emits_once(void) {
  struct capy_gesture_recognizer r;
  struct capy_widget_event ev;
  struct capy_touch_payload p;
  struct capy_gesture out;
  capy_gesture_recognizer_init(&r);
  gesture_make_touch(&ev, &p, CAPY_WIDGET_EVENT_TOUCH_BEGIN, 1u, 100, 100);
  EXPECT(capy_gesture_feed(&r, &ev, 1000u) == 0);
  gesture_make_touch(&ev, &p, CAPY_WIDGET_EVENT_TOUCH_END, 1u, 102, 101);
  EXPECT(capy_gesture_feed(&r, &ev, 1080u) == 1);
  EXPECT(capy_gesture_poll(&r, &out) == 1);
  EXPECT(out.kind == (uint8_t)CAPY_GESTURE_TAP);
  EXPECT(out.duration_ms == 80u);
  EXPECT(out.start.x == 100 && out.start.y == 100);
  /* Queue drains. */
  EXPECT(capy_gesture_poll(&r, &out) == 0);
}

static void test_gesture_double_tap_within_gap(void) {
  struct capy_gesture_recognizer r;
  struct capy_widget_event ev;
  struct capy_touch_payload p;
  struct capy_gesture out;
  capy_gesture_recognizer_init(&r);
  /* First tap. */
  gesture_make_touch(&ev, &p, CAPY_WIDGET_EVENT_TOUCH_BEGIN, 1u, 50, 50);
  EXPECT(capy_gesture_feed(&r, &ev, 1000u) == 0);
  gesture_make_touch(&ev, &p, CAPY_WIDGET_EVENT_TOUCH_END, 1u, 50, 50);
  EXPECT(capy_gesture_feed(&r, &ev, 1050u) == 1);
  EXPECT(capy_gesture_poll(&r, &out) == 1);
  EXPECT(out.kind == (uint8_t)CAPY_GESTURE_TAP);
  /* Second tap within 300 ms gap. */
  gesture_make_touch(&ev, &p, CAPY_WIDGET_EVENT_TOUCH_BEGIN, 2u, 51, 49);
  EXPECT(capy_gesture_feed(&r, &ev, 1200u) == 0);
  gesture_make_touch(&ev, &p, CAPY_WIDGET_EVENT_TOUCH_END, 2u, 51, 49);
  EXPECT(capy_gesture_feed(&r, &ev, 1250u) == 1);
  EXPECT(capy_gesture_poll(&r, &out) == 1);
  EXPECT(out.kind == (uint8_t)CAPY_GESTURE_DOUBLE_TAP);
}

static void test_gesture_long_press_via_tick(void) {
  struct capy_gesture_recognizer r;
  struct capy_widget_event ev;
  struct capy_touch_payload p;
  struct capy_gesture out;
  capy_gesture_recognizer_init(&r);
  gesture_make_touch(&ev, &p, CAPY_WIDGET_EVENT_TOUCH_BEGIN, 7u, 200, 200);
  EXPECT(capy_gesture_feed(&r, &ev, 0u) == 0);
  /* Before threshold: no emit. */
  EXPECT(capy_gesture_tick(&r, 100u) == 0);
  /* After threshold (>= 500 ms default), no movement: emit LONG_PRESS. */
  EXPECT(capy_gesture_tick(&r, 600u) == 1);
  EXPECT(capy_gesture_poll(&r, &out) == 1);
  EXPECT(out.kind == (uint8_t)CAPY_GESTURE_LONG_PRESS);
  EXPECT(out.duration_ms == 600u);
  /* Subsequent tick must not re-emit. */
  EXPECT(capy_gesture_tick(&r, 700u) == 0);
  /* Release does not produce a TAP because long-press already fired. */
  gesture_make_touch(&ev, &p, CAPY_WIDGET_EVENT_TOUCH_END, 7u, 200, 200);
  EXPECT(capy_gesture_feed(&r, &ev, 800u) == 0);
  EXPECT(capy_gesture_poll(&r, &out) == 0);
}

static void test_gesture_swipe_four_directions(void) {
  struct {
    int32_t dx;
    int32_t dy;
    uint8_t expected;
  } cases[4] = {
      {100, 0, (uint8_t)CAPY_GESTURE_SWIPE_RIGHT},
      {-100, 0, (uint8_t)CAPY_GESTURE_SWIPE_LEFT},
      {0, 100, (uint8_t)CAPY_GESTURE_SWIPE_DOWN},
      {0, -100, (uint8_t)CAPY_GESTURE_SWIPE_UP},
  };
  uint32_t i;
  for (i = 0u; i < 4u; ++i) {
    struct capy_gesture_recognizer r;
    struct capy_widget_event ev;
    struct capy_touch_payload p;
    struct capy_gesture out;
    capy_gesture_recognizer_init(&r);
    gesture_make_touch(&ev, &p, CAPY_WIDGET_EVENT_TOUCH_BEGIN, 1u, 200, 200);
    EXPECT(capy_gesture_feed(&r, &ev, 0u) == 0);
    /* End past the swipe threshold (50 px default) — no MOVE so DRAG does
     * not fire mid-stream; END alone decides SWIPE. Duration also above
     * tap_max_duration to avoid TAP candidate. */
    gesture_make_touch(&ev, &p, CAPY_WIDGET_EVENT_TOUCH_END, 1u,
                       200 + cases[i].dx, 200 + cases[i].dy);
    EXPECT(capy_gesture_feed(&r, &ev, 250u) == 1);
    EXPECT(capy_gesture_poll(&r, &out) == 1);
    EXPECT(out.kind == cases[i].expected);
    EXPECT(out.magnitude == 100);
  }
}

static void test_gesture_drag_emitted_at_threshold_cross(void) {
  struct capy_gesture_recognizer r;
  struct capy_widget_event ev;
  struct capy_touch_payload p;
  struct capy_gesture out;
  capy_gesture_recognizer_init(&r);
  gesture_make_touch(&ev, &p, CAPY_WIDGET_EVENT_TOUCH_BEGIN, 1u, 100, 100);
  EXPECT(capy_gesture_feed(&r, &ev, 0u) == 0);
  /* Small move under threshold: no emit. */
  gesture_make_touch(&ev, &p, CAPY_WIDGET_EVENT_TOUCH_MOVE, 1u, 110, 105);
  EXPECT(capy_gesture_feed(&r, &ev, 20u) == 0);
  /* Move past 50 px threshold: DRAG emits exactly once. */
  gesture_make_touch(&ev, &p, CAPY_WIDGET_EVENT_TOUCH_MOVE, 1u, 200, 110);
  EXPECT(capy_gesture_feed(&r, &ev, 40u) == 1);
  EXPECT(capy_gesture_poll(&r, &out) == 1);
  EXPECT(out.kind == (uint8_t)CAPY_GESTURE_DRAG);
  EXPECT(out.magnitude == 100);
  /* Further move: no re-emit. */
  gesture_make_touch(&ev, &p, CAPY_WIDGET_EVENT_TOUCH_MOVE, 1u, 250, 130);
  EXPECT(capy_gesture_feed(&r, &ev, 60u) == 0);
  /* END after drag must not emit a SWIPE. */
  gesture_make_touch(&ev, &p, CAPY_WIDGET_EVENT_TOUCH_END, 1u, 250, 130);
  EXPECT(capy_gesture_feed(&r, &ev, 80u) == 0);
  EXPECT(capy_gesture_poll(&r, &out) == 0);
}

static void test_gesture_determinism(void) {
  struct capy_gesture_recognizer r1, r2;
  struct capy_widget_event ev;
  struct capy_touch_payload p;
  struct capy_gesture o1, o2;
  uint32_t i;
  static const struct {
    enum capy_widget_event_type type;
    uint32_t now;
    int32_t x;
    int32_t y;
  } seq[6] = {
      {CAPY_WIDGET_EVENT_TOUCH_BEGIN, 0u, 100, 100},
      {CAPY_WIDGET_EVENT_TOUCH_MOVE, 10u, 110, 100},
      {CAPY_WIDGET_EVENT_TOUCH_MOVE, 20u, 180, 105},
      {CAPY_WIDGET_EVENT_TOUCH_MOVE, 30u, 220, 108},
      {CAPY_WIDGET_EVENT_TOUCH_END, 40u, 230, 110},
      {CAPY_WIDGET_EVENT_TOUCH_BEGIN, 100u, 50, 50},
  };
  capy_gesture_recognizer_init(&r1);
  capy_gesture_recognizer_init(&r2);
  for (i = 0u; i < 6u; ++i) {
    int r_a, r_b;
    gesture_make_touch(&ev, &p, seq[i].type, 1u, seq[i].x, seq[i].y);
    r_a = capy_gesture_feed(&r1, &ev, seq[i].now);
    gesture_make_touch(&ev, &p, seq[i].type, 1u, seq[i].x, seq[i].y);
    r_b = capy_gesture_feed(&r2, &ev, seq[i].now);
    EXPECT(r_a == r_b);
    if (r_a == 1) {
      EXPECT(capy_gesture_poll(&r1, &o1) == 1);
      EXPECT(capy_gesture_poll(&r2, &o2) == 1);
      EXPECT(o1.kind == o2.kind);
      EXPECT(o1.start.x == o2.start.x && o1.start.y == o2.start.y);
      EXPECT(o1.end.x == o2.end.x && o1.end.y == o2.end.y);
      EXPECT(o1.magnitude == o2.magnitude);
      EXPECT(o1.duration_ms == o2.duration_ms);
    }
  }
}

static void test_gesture_ignores_non_touch_and_extra_touch_ids(void) {
  struct capy_gesture_recognizer r;
  struct capy_widget_event ev;
  struct capy_touch_payload p;
  struct capy_gesture out;
  capy_gesture_recognizer_init(&r);
  /* Non-touch events with no payload do not crash and return 0. */
  ev.type = CAPY_WIDGET_EVENT_POINTER_DOWN;
  ev.x = 0;
  ev.y = 0;
  ev.buttons = 1u;
  ev.keycode = 0u;
  ev.modifiers = 0u;
  ev.payload = 0;
  EXPECT(capy_gesture_feed(&r, &ev, 0u) == 0);
  ev.type = CAPY_WIDGET_EVENT_KEY_DOWN;
  EXPECT(capy_gesture_feed(&r, &ev, 0u) == 0);
  ev.type = CAPY_WIDGET_EVENT_WHEEL;
  EXPECT(capy_gesture_feed(&r, &ev, 0u) == 0);
  ev.type = CAPY_WIDGET_EVENT_GAMEPAD;
  EXPECT(capy_gesture_feed(&r, &ev, 0u) == 0);
  /* NULL guards. */
  EXPECT(capy_gesture_feed(0, &ev, 0u) == -1);
  EXPECT(capy_gesture_feed(&r, 0, 0u) == -1);
  EXPECT(capy_gesture_tick(0, 0u) == -1);
  EXPECT(capy_gesture_poll(0, &out) == -1);
  EXPECT(capy_gesture_poll(&r, 0) == -1);
  /* TOUCH_BEGIN with no payload does not crash. */
  ev.type = CAPY_WIDGET_EVENT_TOUCH_BEGIN;
  ev.payload = 0;
  EXPECT(capy_gesture_feed(&r, &ev, 0u) == 0);
  /* A real BEGIN. */
  gesture_make_touch(&ev, &p, CAPY_WIDGET_EVENT_TOUCH_BEGIN, 1u, 10, 10);
  EXPECT(capy_gesture_feed(&r, &ev, 10u) == 0);
  /* A second BEGIN with a different touch_id while already active opens a
   * two-finger session. Single-touch TAP must be suppressed until reset. */
  gesture_make_touch(&ev, &p, CAPY_WIDGET_EVENT_TOUCH_BEGIN, 99u, 500, 500);
  EXPECT(capy_gesture_feed(&r, &ev, 20u) == 0);
  EXPECT(r.touch_id == 1u);
  EXPECT(r.start_pos.x == 10 && r.start_pos.y == 10);
  EXPECT(r.touch2_active == 1u);
  EXPECT(r.touch2_id == 99u);
  /* Ending either touch resets the two-finger session without queuing TAP. */
  gesture_make_touch(&ev, &p, CAPY_WIDGET_EVENT_TOUCH_END, 1u, 11, 9);
  EXPECT(capy_gesture_feed(&r, &ev, 30u) == 0);
  EXPECT(capy_gesture_poll(&r, &out) == 0);
  EXPECT(r.active == 0u);
  EXPECT(r.touch2_active == 0u);
}

/* ── v2.3: undo/redo stack (since 2.3.0) ────────────────────────────────── */

/* All v2.3 tests share the same buffer-init helper. The buffer is sized
 * for ~16 entries plus the header so eviction can be exercised
 * deterministically. */
#define UNDO_TEST_BUF_BYTES \
  (sizeof(struct capy_undo_stack) + 16u * sizeof(struct capy_undo_entry))

static void test_undo_init_partitions_caller_buffer(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  uint8_t buf[UNDO_TEST_BUF_BYTES];
  capy_widget_context_init(&ctx, &allocator);
  EXPECT(ctx.undo_stack == 0);
  EXPECT(capy_undo_stack_init(&ctx, buf, sizeof(buf)) == 0);
  EXPECT(ctx.undo_stack != 0);
  EXPECT(ctx.undo_stack->capacity == 16u);
  EXPECT(ctx.undo_stack->count == 0u);
  EXPECT(ctx.undo_stack->redo_count == 0u);
  EXPECT(ctx.undo_stack->coalesce_window_ms == 500u);
}

static void test_undo_push_undo_redo_roundtrip(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  uint8_t buf[UNDO_TEST_BUF_BYTES];
  struct capy_undo_entry out;
  const char *redo = "REDO";
  const char *undo = "UNDO";
  capy_widget_context_init(&ctx, &allocator);
  capy_undo_stack_init(&ctx, buf, sizeof(buf));
  EXPECT(capy_undo_can_undo(&ctx) == 0);
  EXPECT(capy_undo_can_redo(&ctx) == 0);
  EXPECT(capy_undo_push(&ctx, "edit.insert", redo, 4u, undo, 4u) == 0);
  EXPECT(capy_undo_can_undo(&ctx) == 1);
  EXPECT(capy_undo_can_redo(&ctx) == 0);
  /* Undo pops the entry off the live stack. */
  EXPECT(capy_undo_undo(&ctx, &out) == 0);
  EXPECT(out.action_id != 0);
  EXPECT(out.action_id[0] == 'e' && out.action_id[5] == 'i');
  EXPECT(out.redo_data == redo);
  EXPECT(out.undo_data == undo);
  EXPECT(out.redo_len == 4u && out.undo_len == 4u);
  EXPECT(capy_undo_can_undo(&ctx) == 0);
  EXPECT(capy_undo_can_redo(&ctx) == 1);
  /* Redo moves the entry back onto the live stack. */
  EXPECT(capy_undo_redo(&ctx, &out) == 0);
  EXPECT(out.action_id == ctx.undo_stack->entries[0].action_id);
  EXPECT(capy_undo_can_undo(&ctx) == 1);
  EXPECT(capy_undo_can_redo(&ctx) == 0);
}

static void test_undo_push_truncates_redo(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  uint8_t buf[UNDO_TEST_BUF_BYTES];
  capy_widget_context_init(&ctx, &allocator);
  capy_undo_stack_init(&ctx, buf, sizeof(buf));
  capy_undo_push(&ctx, "a", 0, 0u, 0, 0u);
  capy_undo_push(&ctx, "b", 0, 0u, 0, 0u);
  capy_undo_push(&ctx, "c", 0, 0u, 0, 0u);
  EXPECT(ctx.undo_stack->count == 3u);
  /* Undo twice → 2 entries on redo stack. */
  capy_undo_undo(&ctx, 0);
  capy_undo_undo(&ctx, 0);
  EXPECT(ctx.undo_stack->count == 1u);
  EXPECT(ctx.undo_stack->redo_count == 2u);
  /* Push branches off and drops the redo stack. */
  capy_undo_push(&ctx, "d", 0, 0u, 0, 0u);
  EXPECT(ctx.undo_stack->count == 2u);
  EXPECT(ctx.undo_stack->redo_count == 0u);
  /* redo now returns -1. */
  EXPECT(capy_undo_redo(&ctx, 0) == -1);
}

static void test_undo_fifo_eviction_on_full_buffer(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  /* Buffer sized for exactly 2 entries to force eviction. */
  uint8_t buf[sizeof(struct capy_undo_stack) +
              2u * sizeof(struct capy_undo_entry)];
  static const char *const ids[3] = {"first", "second", "third"};
  capy_widget_context_init(&ctx, &allocator);
  EXPECT(capy_undo_stack_init(&ctx, buf, sizeof(buf)) == 0);
  EXPECT(ctx.undo_stack->capacity == 2u);
  capy_undo_push(&ctx, ids[0], 0, 0u, 0, 0u);
  capy_undo_push(&ctx, ids[1], 0, 0u, 0, 0u);
  EXPECT(ctx.undo_stack->count == 2u);
  /* Third push evicts the oldest ("first"). */
  capy_undo_push(&ctx, ids[2], 0, 0u, 0, 0u);
  EXPECT(ctx.undo_stack->count == 2u);
  /* Stack top is now `third`; below it is `second`. */
  {
    struct capy_undo_entry top;
    EXPECT(capy_undo_undo(&ctx, &top) == 0);
    EXPECT(top.action_id == ids[2]);
    EXPECT(capy_undo_undo(&ctx, &top) == 0);
    EXPECT(top.action_id == ids[1]);
    /* The oldest was evicted; no third undo. */
    EXPECT(capy_undo_undo(&ctx, &top) == -1);
  }
}

static void test_undo_on_empty_returns_negative(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  uint8_t buf[UNDO_TEST_BUF_BYTES];
  capy_widget_context_init(&ctx, &allocator);
  /* Before init: every API returns -1 / 0. */
  EXPECT(capy_undo_undo(&ctx, 0) == -1);
  EXPECT(capy_undo_redo(&ctx, 0) == -1);
  EXPECT(capy_undo_can_undo(&ctx) == 0);
  EXPECT(capy_undo_can_redo(&ctx) == 0);
  EXPECT(capy_undo_push(&ctx, "noop", 0, 0u, 0, 0u) == -1);
  /* After init: empty stack still returns -1 for undo/redo. */
  capy_undo_stack_init(&ctx, buf, sizeof(buf));
  EXPECT(capy_undo_undo(&ctx, 0) == -1);
  EXPECT(capy_undo_redo(&ctx, 0) == -1);
}

static void test_undo_null_guards_and_too_small_buffer(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  uint8_t buf[8]; /* Too small for header + 1 entry. */
  capy_widget_context_init(&ctx, &allocator);
  EXPECT(capy_undo_stack_init(0, buf, sizeof(buf)) == -1);
  EXPECT(capy_undo_stack_init(&ctx, 0, sizeof(buf)) == -1);
  EXPECT(capy_undo_stack_init(&ctx, buf, sizeof(buf)) == -1);
  /* NULL ctx for all helpers. */
  EXPECT(capy_undo_push(0, "x", 0, 0u, 0, 0u) == -1);
  EXPECT(capy_undo_undo(0, 0) == -1);
  EXPECT(capy_undo_redo(0, 0) == -1);
  EXPECT(capy_undo_can_undo(0) == 0);
  EXPECT(capy_undo_can_redo(0) == 0);
}

static void test_undo_determinism_same_sequence(void) {
  /* Two contexts with identical buffers should produce identical
   * `(count, redo_count, top.action_id)` after the same operation
   * sequence. */
  struct test_heap heap_a = {{0}, 0u, 0u};
  struct test_heap heap_b = {{0}, 0u, 0u};
  struct capy_widget_allocator alloc_a = {test_alloc, test_free, &heap_a};
  struct capy_widget_allocator alloc_b = {test_alloc, test_free, &heap_b};
  struct capy_widget_context ctx_a;
  struct capy_widget_context ctx_b;
  uint8_t buf_a[UNDO_TEST_BUF_BYTES];
  uint8_t buf_b[UNDO_TEST_BUF_BYTES];
  static const char *const seq[5] = {"one", "two", "three", "four", "five"};
  uint32_t i;
  capy_widget_context_init(&ctx_a, &alloc_a);
  capy_widget_context_init(&ctx_b, &alloc_b);
  capy_undo_stack_init(&ctx_a, buf_a, sizeof(buf_a));
  capy_undo_stack_init(&ctx_b, buf_b, sizeof(buf_b));
  for (i = 0u; i < 5u; ++i) {
    capy_undo_push(&ctx_a, seq[i], 0, 0u, 0, 0u);
    capy_undo_push(&ctx_b, seq[i], 0, 0u, 0, 0u);
  }
  for (i = 0u; i < 2u; ++i) {
    capy_undo_undo(&ctx_a, 0);
    capy_undo_undo(&ctx_b, 0);
  }
  capy_undo_redo(&ctx_a, 0);
  capy_undo_redo(&ctx_b, 0);
  EXPECT(ctx_a.undo_stack->count == ctx_b.undo_stack->count);
  EXPECT(ctx_a.undo_stack->redo_count == ctx_b.undo_stack->redo_count);
  EXPECT(ctx_a.undo_stack->entries[ctx_a.undo_stack->count - 1u].action_id ==
         ctx_b.undo_stack->entries[ctx_b.undo_stack->count - 1u].action_id);
}

#undef UNDO_TEST_BUF_BYTES

/* ── v2.4: theme packs (since 2.4.0) ────────────────────────────────────── */

static uint32_t theme_pack_test_fnv1a(const uint8_t *bytes, uint32_t len) {
  uint32_t h = 2166136261u;
  uint32_t i;
  for (i = 0u; i < len; ++i) {
    h ^= (uint32_t)bytes[i];
    h *= 16777619u;
  }
  return h;
}

static void theme_pack_test_write_u16(uint8_t *p, uint16_t v) {
  p[0] = (uint8_t)(v & 0xFFu);
  p[1] = (uint8_t)((v >> 8) & 0xFFu);
}

static void theme_pack_test_write_u32(uint8_t *p, uint32_t v) {
  p[0] = (uint8_t)(v & 0xFFu);
  p[1] = (uint8_t)((v >> 8) & 0xFFu);
  p[2] = (uint8_t)((v >> 16) & 0xFFu);
  p[3] = (uint8_t)((v >> 24) & 0xFFu);
}

/* Build a well-formed pack with `n_entries` entries into `out` of size
 * `cap`. Each entry sets token (i+1) to colour (0x80000000u | i). Returns
 * total bytes written, or 0 on overflow. */
static uint32_t theme_pack_test_build(uint8_t *out, uint32_t cap,
                                      uint16_t n_entries, uint8_t variant,
                                      uint8_t high_contrast) {
  uint32_t total = (uint32_t)CAPY_THEME_PACK_HEADER_SIZE +
                   (uint32_t)n_entries * (uint32_t)CAPY_THEME_PACK_ENTRY_SIZE;
  uint16_t i;
  uint32_t checksum;
  if (total > cap) {
    return 0u;
  }
  out[0] = CAPY_THEME_PACK_MAGIC0;
  out[1] = CAPY_THEME_PACK_MAGIC1;
  out[2] = CAPY_THEME_PACK_MAGIC2;
  out[3] = CAPY_THEME_PACK_MAGIC3;
  theme_pack_test_write_u16(out + 4, (uint16_t)CAPY_THEME_PACK_FORMAT_VERSION);
  out[6] = variant;
  out[7] = high_contrast;
  /* checksum placeholder; filled below */
  theme_pack_test_write_u32(out + 8, 0u);
  theme_pack_test_write_u16(out + 12, n_entries);
  theme_pack_test_write_u16(out + 14, 0u);
  for (i = 0u; i < n_entries; ++i) {
    uint8_t *e = out + CAPY_THEME_PACK_HEADER_SIZE +
                 (uint32_t)i * (uint32_t)CAPY_THEME_PACK_ENTRY_SIZE;
    e[0] = (uint8_t)((uint32_t)(i + 1u) & 0xFFu);
    e[1] = 0u;
    theme_pack_test_write_u32(e + 2, 0x80000000u | (uint32_t)i);
  }
  checksum = theme_pack_test_fnv1a(out + CAPY_THEME_PACK_HEADER_SIZE,
                                   total - (uint32_t)CAPY_THEME_PACK_HEADER_SIZE);
  theme_pack_test_write_u32(out + 8, checksum);
  return total;
}

static void test_theme_pack_validate_well_formed_zero(void) {
  uint8_t buf[64];
  uint32_t len = theme_pack_test_build(buf, sizeof buf, 4u,
                                       CAPY_THEME_VARIANT_DARK, 0u);
  EXPECT(len > 0u);
  EXPECT(capy_theme_pack_validate(buf, len) == 0);
  EXPECT(capy_theme_pack_validate(0, len) == -1);
  EXPECT(capy_theme_pack_validate(buf, 0u) == -1);
}

static void test_theme_pack_load_applies_tokens(void) {
  uint8_t buf[64];
  uint32_t len;
  struct capy_theme theme = capy_theme_default_light();
  uint16_t baseline_version = theme.version;
  len = theme_pack_test_build(buf, sizeof buf, 3u,
                              CAPY_THEME_VARIANT_DARK, 1u);
  EXPECT(len > 0u);
  EXPECT(capy_theme_pack_load(buf, len, &theme) == 0);
  /* Tokens 1, 2, 3 were overwritten with 0x80000000, 0x80000001, 0x80000002. */
  EXPECT(theme.tokens[1] == 0x80000000u);
  EXPECT(theme.tokens[2] == 0x80000001u);
  EXPECT(theme.tokens[3] == 0x80000002u);
  /* Untouched tokens keep light defaults (non-zero). */
  EXPECT(theme.tokens[CAPY_TOKEN_COUNT - 1u] != 0x80000000u);
  EXPECT(theme.variant == CAPY_THEME_VARIANT_DARK);
  EXPECT(theme.high_contrast == 1u);
  EXPECT(theme.version == baseline_version);
}

static void test_theme_pack_bad_magic_rejected(void) {
  uint8_t buf[40];
  uint32_t len = theme_pack_test_build(buf, sizeof buf, 2u, 0u, 0u);
  EXPECT(len > 0u);
  buf[0] = 'X';
  EXPECT(capy_theme_pack_validate(buf, len) == -1);
  {
    struct capy_theme t = capy_theme_default_light();
    EXPECT(capy_theme_pack_load(buf, len, &t) == -1);
  }
}

static void test_theme_pack_future_version_rejected(void) {
  uint8_t buf[40];
  uint32_t len = theme_pack_test_build(buf, sizeof buf, 2u, 0u, 0u);
  uint32_t checksum;
  EXPECT(len > 0u);
  theme_pack_test_write_u16(buf + 4,
                            (uint16_t)(CAPY_THEME_PACK_FORMAT_VERSION + 1u));
  /* Re-checksum so version becomes the only fault. */
  checksum = theme_pack_test_fnv1a(buf + CAPY_THEME_PACK_HEADER_SIZE,
                                   len - (uint32_t)CAPY_THEME_PACK_HEADER_SIZE);
  theme_pack_test_write_u32(buf + 8, checksum);
  EXPECT(capy_theme_pack_validate(buf, len) == -1);
}

static void test_theme_pack_checksum_mismatch_rejected(void) {
  uint8_t buf[40];
  uint32_t len = theme_pack_test_build(buf, sizeof buf, 2u, 0u, 0u);
  EXPECT(len > 0u);
  /* Flip one byte in the body without recomputing the checksum. */
  buf[CAPY_THEME_PACK_HEADER_SIZE] ^= 0xFFu;
  EXPECT(capy_theme_pack_validate(buf, len) == -1);
}

static void test_theme_pack_unknown_token_skipped(void) {
  uint8_t buf[CAPY_THEME_PACK_HEADER_SIZE + 2u * CAPY_THEME_PACK_ENTRY_SIZE];
  uint32_t len = (uint32_t)sizeof buf;
  uint32_t checksum;
  struct capy_theme theme = capy_theme_default_light();
  buf[0] = CAPY_THEME_PACK_MAGIC0;
  buf[1] = CAPY_THEME_PACK_MAGIC1;
  buf[2] = CAPY_THEME_PACK_MAGIC2;
  buf[3] = CAPY_THEME_PACK_MAGIC3;
  theme_pack_test_write_u16(buf + 4, (uint16_t)CAPY_THEME_PACK_FORMAT_VERSION);
  buf[6] = CAPY_THEME_VARIANT_LIGHT;
  buf[7] = 0u;
  theme_pack_test_write_u32(buf + 8, 0u);
  theme_pack_test_write_u16(buf + 12, 2u);
  theme_pack_test_write_u16(buf + 14, 0u);
  /* Entry 0: unknown token id 250 with colour 0xDEADBEEF. */
  buf[16] = 250u;
  buf[17] = 0u;
  theme_pack_test_write_u32(buf + 18, 0xDEADBEEFu);
  /* Entry 1: token 1 with colour 0xCAFEBABE. */
  buf[22] = 1u;
  buf[23] = 0u;
  theme_pack_test_write_u32(buf + 24, 0xCAFEBABEu);
  checksum = theme_pack_test_fnv1a(buf + CAPY_THEME_PACK_HEADER_SIZE,
                                   len - (uint32_t)CAPY_THEME_PACK_HEADER_SIZE);
  theme_pack_test_write_u32(buf + 8, checksum);
  EXPECT(capy_theme_pack_validate(buf, len) == 0);
  EXPECT(capy_theme_pack_load(buf, len, &theme) == 0);
  /* Token 1 was overwritten; the unknown entry was silently skipped. */
  EXPECT(theme.tokens[1] == 0xCAFEBABEu);
}

static void test_theme_pack_truncated_buffer_rejected(void) {
  uint8_t buf[40];
  uint32_t len = theme_pack_test_build(buf, sizeof buf, 3u, 0u, 0u);
  EXPECT(len > 0u);
  /* Declared 3 entries but pass len that fits only 2. */
  EXPECT(capy_theme_pack_validate(buf, len - 1u) == -1);
  /* Header alone — declared count would not match. */
  EXPECT(capy_theme_pack_validate(buf, (uint32_t)CAPY_THEME_PACK_HEADER_SIZE) == -1);
  /* Below header size. */
  EXPECT(capy_theme_pack_validate(buf, 8u) == -1);
}

static void test_theme_pack_reserved_field_nonzero_rejected(void) {
  uint8_t buf[40];
  uint32_t len = theme_pack_test_build(buf, sizeof buf, 2u, 0u, 0u);
  uint32_t checksum;
  EXPECT(len > 0u);
  /* Set the 16-bit reserved field at offset 14 to a non-zero value. */
  theme_pack_test_write_u16(buf + 14, 0x1234u);
  /* Re-checksum to ensure reserved is the only fault. */
  checksum = theme_pack_test_fnv1a(buf + CAPY_THEME_PACK_HEADER_SIZE,
                                   len - (uint32_t)CAPY_THEME_PACK_HEADER_SIZE);
  theme_pack_test_write_u32(buf + 8, checksum);
  EXPECT(capy_theme_pack_validate(buf, len) == -1);
}

/* ── v2.5: devtools / inspector (since 2.5.0) ───────────────────────────── */

static void test_inspector_root_only_returns_one_node(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *root;
  struct capy_inspector_node nodes[4];
  char text_pool[32];
  uint32_t node_count = 0xFFu;
  uint32_t text_used = 0xFFu;
  capy_widget_context_init(&ctx, &allocator);
  root = capy_widget_create(&ctx, CAPY_WIDGET_PANEL);
  EXPECT(root != 0);
  capy_widget_set_bounds(root, 1, 2, 30u, 40u);
  EXPECT(capy_widget_inspect(root, nodes, 4u, text_pool, sizeof text_pool,
                             &node_count, &text_used) == 0);
  EXPECT(node_count == 1u);
  EXPECT(text_used == 0u);
  EXPECT(nodes[0].id == root->id);
  EXPECT(nodes[0].type == (uint16_t)CAPY_WIDGET_PANEL);
  EXPECT(nodes[0].depth == 0u);
  EXPECT(nodes[0].parent_index == CAPY_INSPECTOR_NO_PARENT);
  EXPECT(nodes[0].child_count == 0u);
  EXPECT(nodes[0].x == 1);
  EXPECT(nodes[0].y == 2);
  EXPECT(nodes[0].w == 30);
  EXPECT(nodes[0].h == 40);
  EXPECT((nodes[0].flags & CAPY_INSPECTOR_FLAG_VISIBLE) != 0u);
}

static void test_inspector_depth_first_preorder(void) {
  /* Tree:   A
   *        / \
   *       B   D
   *       |
   *       C
   * Expected order: A, B, C, D with depths 0,1,2,1. */
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *a;
  struct capy_widget *b;
  struct capy_widget *c;
  struct capy_widget *d;
  struct capy_inspector_node nodes[8];
  char text_pool[64];
  uint32_t node_count = 0u;
  uint32_t text_used = 0u;
  capy_widget_context_init(&ctx, &allocator);
  a = capy_widget_create(&ctx, CAPY_WIDGET_PANEL);
  b = capy_widget_create(&ctx, CAPY_WIDGET_PANEL);
  c = capy_widget_create(&ctx, CAPY_WIDGET_BUTTON);
  d = capy_widget_create(&ctx, CAPY_WIDGET_LABEL);
  EXPECT(a && b && c && d);
  capy_widget_add_child(a, b);
  capy_widget_add_child(b, c);
  capy_widget_add_child(a, d);
  EXPECT(capy_widget_inspect(a, nodes, 8u, text_pool, sizeof text_pool,
                             &node_count, &text_used) == 0);
  EXPECT(node_count == 4u);
  EXPECT(nodes[0].id == a->id);
  EXPECT(nodes[0].depth == 0u);
  EXPECT(nodes[1].id == b->id);
  EXPECT(nodes[1].depth == 1u);
  EXPECT(nodes[1].parent_index == 0u);
  EXPECT(nodes[2].id == c->id);
  EXPECT(nodes[2].depth == 2u);
  EXPECT(nodes[2].parent_index == 1u);
  EXPECT(nodes[3].id == d->id);
  EXPECT(nodes[3].depth == 1u);
  EXPECT(nodes[3].parent_index == 0u);
}

static void test_inspector_text_pool_copies_label_text(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *root;
  struct capy_widget *lbl;
  struct capy_inspector_node nodes[4];
  char text_pool[16];
  uint32_t node_count = 0u;
  uint32_t text_used = 0u;
  uint32_t i;
  const char *want = "hello";
  capy_widget_context_init(&ctx, &allocator);
  root = capy_widget_create(&ctx, CAPY_WIDGET_PANEL);
  lbl = capy_widget_create(&ctx, CAPY_WIDGET_LABEL);
  EXPECT(root && lbl);
  capy_widget_add_child(root, lbl);
  capy_widget_set_text(lbl, want);
  EXPECT(capy_widget_inspect(root, nodes, 4u, text_pool, sizeof text_pool,
                             &node_count, &text_used) == 0);
  EXPECT(node_count == 2u);
  EXPECT(nodes[0].text_len == 0u);
  EXPECT(nodes[1].text_len == 5u);
  EXPECT(nodes[1].text_offset == 0u);
  EXPECT(text_used == 5u);
  for (i = 0u; i < 5u; ++i) {
    EXPECT(text_pool[i] == want[i]);
  }
}

static void test_inspector_capacity_overflow_fails_closed(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *a;
  struct capy_widget *b;
  struct capy_widget *c;
  struct capy_inspector_node nodes[2];
  char text_pool[16];
  uint32_t node_count = 7u;
  uint32_t text_used = 7u;
  capy_widget_context_init(&ctx, &allocator);
  a = capy_widget_create(&ctx, CAPY_WIDGET_PANEL);
  b = capy_widget_create(&ctx, CAPY_WIDGET_PANEL);
  c = capy_widget_create(&ctx, CAPY_WIDGET_PANEL);
  EXPECT(a && b && c);
  capy_widget_add_child(a, b);
  capy_widget_add_child(a, c);
  /* 3 nodes but cap = 2 → -1 + rollback. */
  EXPECT(capy_widget_inspect(a, nodes, 2u, text_pool, sizeof text_pool,
                             &node_count, &text_used) == -1);
  EXPECT(node_count == 0u);
  EXPECT(text_used == 0u);
}

static void test_inspector_text_pool_overflow_fails_closed(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *lbl;
  struct capy_inspector_node nodes[4];
  char text_pool[2];
  uint32_t node_count = 7u;
  uint32_t text_used = 7u;
  capy_widget_context_init(&ctx, &allocator);
  lbl = capy_widget_create(&ctx, CAPY_WIDGET_LABEL);
  EXPECT(lbl != 0);
  capy_widget_set_text(lbl, "long-text");
  EXPECT(capy_widget_inspect(lbl, nodes, 4u, text_pool, sizeof text_pool,
                             &node_count, &text_used) == -1);
  EXPECT(node_count == 0u);
  EXPECT(text_used == 0u);
}

static void test_inspector_null_guards(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *w;
  struct capy_inspector_node nodes[2];
  char text_pool[4];
  uint32_t node_count = 0u;
  uint32_t text_used = 0u;
  capy_widget_context_init(&ctx, &allocator);
  w = capy_widget_create(&ctx, CAPY_WIDGET_PANEL);
  EXPECT(w != 0);
  EXPECT(capy_widget_inspect(0, nodes, 2u, text_pool, sizeof text_pool,
                             &node_count, &text_used) == -1);
  EXPECT(capy_widget_inspect(w, 0, 2u, text_pool, sizeof text_pool,
                             &node_count, &text_used) == -1);
  EXPECT(capy_widget_inspect(w, nodes, 2u, text_pool, sizeof text_pool,
                             0, &text_used) == -1);
  EXPECT(capy_widget_inspect(w, nodes, 2u, text_pool, sizeof text_pool,
                             &node_count, 0) == -1);
  /* text_cap > 0 but NULL pool is rejected. */
  EXPECT(capy_widget_inspect(w, nodes, 2u, 0, 4u, &node_count,
                             &text_used) == -1);
}

static void test_perf_counters_snapshot_populates_fields(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *root;
  struct capy_widget *child;
  struct capy_perf_counters perf;
  capy_widget_context_init(&ctx, &allocator);
  root = capy_widget_create(&ctx, CAPY_WIDGET_PANEL);
  child = capy_widget_create(&ctx, CAPY_WIDGET_BUTTON);
  EXPECT(root && child);
  capy_widget_add_child(root, child);
  capy_widget_frame_budget(&ctx, 16000u);
  EXPECT(capy_perf_counters_snapshot(&ctx, root, &perf) == 0);
  EXPECT(perf.widget_count == 2u);
  EXPECT(perf.plugin_count == ctx.plugin_count);
  EXPECT(perf.undo_count == 0u);
  EXPECT(perf.undo_redo_count == 0u);
  EXPECT(perf.undo_capacity == 0u);
  EXPECT(perf.dpi_scale_x256 == ctx.dpi_scale_x256);
  EXPECT(perf.frame_budget_microseconds == 16000u);
  EXPECT(perf.theme_variant == (uint16_t)ctx.theme.variant);
  EXPECT(perf.theme_high_contrast == ctx.theme.high_contrast);
  /* NULL ctx → -1, NULL out → -1, but a NULL root is allowed (widget_count=0). */
  EXPECT(capy_perf_counters_snapshot(0, root, &perf) == -1);
  EXPECT(capy_perf_counters_snapshot(&ctx, root, 0) == -1);
  EXPECT(capy_perf_counters_snapshot(&ctx, 0, &perf) == 0);
  EXPECT(perf.widget_count == 0u);
}

/* ── v2.6: display mode (since 2.6.0) ───────────────────────────────────── */

struct display_test_state {
  struct capy_display_mode catalog[4];
  uint32_t catalog_count;
  int enum_calls;
  int set_calls;
  struct capy_display_mode last_set;
  int set_should_fail;
};

static int display_test_enum(void *user_data, struct capy_display_mode *out,
                             uint32_t cap) {
  struct display_test_state *s = (struct display_test_state *)user_data;
  uint32_t i;
  uint32_t n = s->catalog_count;
  ++s->enum_calls;
  if (n > cap) {
    n = cap;
  }
  for (i = 0u; i < n; ++i) {
    out[i] = s->catalog[i];
  }
  return (int)n;
}

static int display_test_set(void *user_data,
                            const struct capy_display_mode *mode) {
  struct display_test_state *s = (struct display_test_state *)user_data;
  ++s->set_calls;
  if (s->set_should_fail) {
    return -1;
  }
  s->last_set = *mode;
  return 0;
}

static void display_test_seed(struct display_test_state *s) {
  s->catalog_count = 3u;
  s->catalog[0].width = 1920u;
  s->catalog[0].height = 1080u;
  s->catalog[0].refresh_hz_q8 = 60u;
  s->catalog[0].bpp = 32u;
  s->catalog[0].flags = CAPY_DISPLAY_MODE_FLAG_PREFERRED;
  s->catalog[1].width = 2560u;
  s->catalog[1].height = 1440u;
  s->catalog[1].refresh_hz_q8 = 144u;
  s->catalog[1].bpp = 32u;
  s->catalog[1].flags = 0u;
  s->catalog[2].width = 3840u;
  s->catalog[2].height = 2160u;
  s->catalog[2].refresh_hz_q8 = 60u;
  s->catalog[2].bpp = 32u;
  s->catalog[2].flags = 0u;
  s->enum_calls = 0;
  s->set_calls = 0;
  s->set_should_fail = 0;
}

static void test_display_enum_modes_forwards_to_host(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct display_test_state state;
  struct capy_display_controller controller;
  struct capy_display_mode out[4];
  int n;
  capy_widget_context_init(&ctx, &allocator);
  display_test_seed(&state);
  zero_mem(&controller, sizeof(controller));
  controller.enum_modes = display_test_enum;
  controller.set_mode = display_test_set;
  controller.user_data = &state;
  capy_widget_set_display_controller(&ctx, &controller);
  n = capy_display_enum_modes(&ctx, out, 4u);
  EXPECT(n == 3);
  EXPECT(state.enum_calls == 1);
  EXPECT(out[0].width == 1920u);
  EXPECT(out[1].refresh_hz_q8 == 144u);
  EXPECT((out[0].flags & CAPY_DISPLAY_MODE_FLAG_PREFERRED) != 0u);
}

static void test_display_set_mode_caches_and_emits_event(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *root;
  struct display_test_state state;
  struct capy_display_controller controller;
  struct capy_display_mode current;
  capy_widget_context_init(&ctx, &allocator);
  root = capy_widget_create(&ctx, CAPY_WIDGET_PANEL);
  EXPECT(root != 0);
  display_test_seed(&state);
  zero_mem(&controller, sizeof(controller));
  controller.enum_modes = display_test_enum;
  controller.set_mode = display_test_set;
  controller.user_data = &state;
  capy_widget_set_display_controller(&ctx, &controller);
  /* Before set: no current. */
  EXPECT(capy_display_current_mode(&ctx, &current) == -1);
  EXPECT(capy_display_set_mode(&ctx, root, &state.catalog[1]) == 0);
  EXPECT(state.set_calls == 1);
  EXPECT(controller.has_current == 1u);
  EXPECT(controller.current_mode.width == 2560u);
  EXPECT(controller.current_mode.refresh_hz_q8 == 144u);
  EXPECT(state.last_set.width == 2560u);
  /* Current mode now readable. */
  EXPECT(capy_display_current_mode(&ctx, &current) == 0);
  EXPECT(current.width == 2560u);
  EXPECT(current.height == 1440u);
}

static void test_display_set_mode_host_failure_no_cache(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *root;
  struct display_test_state state;
  struct capy_display_controller controller;
  capy_widget_context_init(&ctx, &allocator);
  root = capy_widget_create(&ctx, CAPY_WIDGET_PANEL);
  EXPECT(root != 0);
  display_test_seed(&state);
  state.set_should_fail = 1;
  zero_mem(&controller, sizeof(controller));
  controller.enum_modes = display_test_enum;
  controller.set_mode = display_test_set;
  controller.user_data = &state;
  capy_widget_set_display_controller(&ctx, &controller);
  EXPECT(capy_display_set_mode(&ctx, root, &state.catalog[2]) == -1);
  EXPECT(controller.has_current == 0u);
}

static void test_display_no_controller_returns_negative(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_display_mode out[2];
  struct capy_display_mode mode;
  capy_widget_context_init(&ctx, &allocator);
  mode.width = 1024u;
  mode.height = 768u;
  mode.refresh_hz_q8 = 60u;
  mode.bpp = 32u;
  mode.flags = 0u;
  EXPECT(capy_display_enum_modes(&ctx, out, 2u) == -1);
  EXPECT(capy_display_set_mode(&ctx, 0, &mode) == -1);
  EXPECT(capy_display_current_mode(&ctx, out) == -1);
}

static void test_display_set_mode_rejects_zero_dimensions(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct display_test_state state;
  struct capy_display_controller controller;
  struct capy_display_mode bad;
  capy_widget_context_init(&ctx, &allocator);
  display_test_seed(&state);
  zero_mem(&controller, sizeof(controller));
  controller.enum_modes = display_test_enum;
  controller.set_mode = display_test_set;
  controller.user_data = &state;
  capy_widget_set_display_controller(&ctx, &controller);
  bad.width = 0u;
  bad.height = 1080u;
  bad.refresh_hz_q8 = 60u;
  bad.bpp = 32u;
  bad.flags = 0u;
  EXPECT(capy_display_set_mode(&ctx, 0, &bad) == -1);
  bad.width = 1920u;
  bad.height = 0u;
  EXPECT(capy_display_set_mode(&ctx, 0, &bad) == -1);
  EXPECT(state.set_calls == 0);
}

static void test_display_enum_modes_capacity_truncates(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct display_test_state state;
  struct capy_display_controller controller;
  struct capy_display_mode out[2];
  int n;
  capy_widget_context_init(&ctx, &allocator);
  display_test_seed(&state);
  zero_mem(&controller, sizeof(controller));
  controller.enum_modes = display_test_enum;
  controller.set_mode = display_test_set;
  controller.user_data = &state;
  capy_widget_set_display_controller(&ctx, &controller);
  n = capy_display_enum_modes(&ctx, out, 2u);
  EXPECT(n == 2);
  EXPECT(out[0].width == 1920u);
  EXPECT(out[1].width == 2560u);
}

static void test_display_null_guards(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_display_mode out;
  capy_widget_context_init(&ctx, &allocator);
  EXPECT(capy_display_enum_modes(0, &out, 1u) == -1);
  EXPECT(capy_display_set_mode(0, 0, &out) == -1);
  EXPECT(capy_display_current_mode(0, &out) == -1);
  /* No-op on NULL ctx in setter. */
  capy_widget_set_display_controller(0, 0);
}

/* ── v2.7: user management (since 2.7.0) ────────────────────────────────── */

struct user_test_state {
  struct capy_user_account roster[8];
  uint32_t roster_count;
  int list_calls;
  int create_calls;
  int update_calls;
  int delete_calls;
  int avatar_calls;
  uint32_t last_deleted_uid;
  uint32_t last_avatar_uid;
  uint32_t last_avatar_len;
  int last_create_admin;
  char last_password[32];
  int next_call_should_fail;
};

static int user_test_list(void *user_data, struct capy_user_account *out,
                          uint32_t cap) {
  struct user_test_state *s = (struct user_test_state *)user_data;
  uint32_t i;
  uint32_t n = s->roster_count;
  ++s->list_calls;
  if (n > cap) {
    n = cap;
  }
  for (i = 0u; i < n; ++i) {
    out[i] = s->roster[i];
  }
  return (int)n;
}

static void user_test_copy_str(char *dst, const char *src, uint32_t cap) {
  uint32_t i = 0u;
  if (!src || !dst || cap == 0u) {
    return;
  }
  while (src[i] != '\0' && i + 1u < cap) {
    dst[i] = src[i];
    ++i;
  }
  dst[i] = '\0';
}

static int user_test_create(void *user_data,
                            const struct capy_user_account *account,
                            const char *password) {
  struct user_test_state *s = (struct user_test_state *)user_data;
  ++s->create_calls;
  if (s->next_call_should_fail) {
    s->next_call_should_fail = 0;
    return -1;
  }
  s->last_create_admin = account->is_admin;
  user_test_copy_str(s->last_password, password ? password : "",
                     (uint32_t)sizeof s->last_password);
  if (s->roster_count >= 8u) {
    return -1;
  }
  s->roster[s->roster_count] = *account;
  ++s->roster_count;
  return 0;
}

static int user_test_update(void *user_data,
                            const struct capy_user_account *account) {
  struct user_test_state *s = (struct user_test_state *)user_data;
  uint32_t i;
  ++s->update_calls;
  if (s->next_call_should_fail) {
    s->next_call_should_fail = 0;
    return -1;
  }
  for (i = 0u; i < s->roster_count; ++i) {
    if (s->roster[i].uid == account->uid) {
      s->roster[i] = *account;
      return 0;
    }
  }
  return -1;
}

static int user_test_delete(void *user_data, uint32_t uid) {
  struct user_test_state *s = (struct user_test_state *)user_data;
  uint32_t i;
  ++s->delete_calls;
  s->last_deleted_uid = uid;
  if (s->next_call_should_fail) {
    s->next_call_should_fail = 0;
    return -1;
  }
  for (i = 0u; i < s->roster_count; ++i) {
    if (s->roster[i].uid == uid) {
      while (i + 1u < s->roster_count) {
        s->roster[i] = s->roster[i + 1u];
        ++i;
      }
      --s->roster_count;
      return 0;
    }
  }
  return -1;
}

static int user_test_set_avatar(void *user_data, uint32_t uid, const void *png,
                                uint32_t len) {
  struct user_test_state *s = (struct user_test_state *)user_data;
  uint32_t i;
  (void)png;
  ++s->avatar_calls;
  s->last_avatar_uid = uid;
  s->last_avatar_len = len;
  for (i = 0u; i < s->roster_count; ++i) {
    if (s->roster[i].uid == uid) {
      s->roster[i].avatar_image_id = (uint16_t)(len == 0u ? 0u : uid + 1000u);
      return 0;
    }
  }
  return -1;
}

static void user_test_seed(struct user_test_state *s,
                           struct capy_user_directory *d) {
  uint32_t i;
  for (i = 0u; i < (uint32_t)sizeof *s; ++i) {
    ((char *)s)[i] = 0;
  }
  s->roster_count = 2u;
  user_test_copy_str(s->roster[0].username, "root", CAPY_USER_NAME_MAX);
  user_test_copy_str(s->roster[0].display_name, "Root", CAPY_USER_DISPLAY_NAME_MAX);
  s->roster[0].uid = 0u;
  s->roster[0].is_admin = 1u;
  user_test_copy_str(s->roster[1].username, "alice", CAPY_USER_NAME_MAX);
  user_test_copy_str(s->roster[1].display_name, "Alice", CAPY_USER_DISPLAY_NAME_MAX);
  s->roster[1].uid = 1000u;
  s->roster[1].is_admin = 0u;
  d->list = user_test_list;
  d->create = user_test_create;
  d->update = user_test_update;
  d->del = user_test_delete;
  d->set_avatar = user_test_set_avatar;
  d->user_data = s;
}

static void test_user_list_forwards_to_host(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct user_test_state state;
  struct capy_user_directory dir;
  struct capy_user_account out[8];
  int n;
  capy_widget_context_init(&ctx, &allocator);
  user_test_seed(&state, &dir);
  capy_widget_set_user_directory(&ctx, &dir);
  n = capy_user_list(&ctx, out, 8u);
  EXPECT(n == 2);
  EXPECT(state.list_calls == 1);
  EXPECT(out[0].uid == 0u);
  EXPECT(out[0].is_admin == 1u);
  EXPECT(out[1].uid == 1000u);
}

static void test_user_create_appends_and_passes_password(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct user_test_state state;
  struct capy_user_directory dir;
  struct capy_user_account acc;
  capy_widget_context_init(&ctx, &allocator);
  user_test_seed(&state, &dir);
  capy_widget_set_user_directory(&ctx, &dir);
  zero_mem(&acc, sizeof acc);
  user_test_copy_str(acc.username, "bob", CAPY_USER_NAME_MAX);
  user_test_copy_str(acc.display_name, "Bob", CAPY_USER_DISPLAY_NAME_MAX);
  acc.uid = 1001u;
  acc.is_admin = 0u;
  EXPECT(capy_user_create(&ctx, &acc, "s3cret") == 0);
  EXPECT(state.create_calls == 1);
  EXPECT(state.roster_count == 3u);
  EXPECT(state.last_password[0] == 's');
  EXPECT(state.last_password[1] == '3');
  EXPECT(state.last_create_admin == 0);
}

static void test_user_create_rejects_empty_username(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct user_test_state state;
  struct capy_user_directory dir;
  struct capy_user_account acc;
  capy_widget_context_init(&ctx, &allocator);
  user_test_seed(&state, &dir);
  capy_widget_set_user_directory(&ctx, &dir);
  zero_mem(&acc, sizeof acc);
  acc.uid = 2000u;
  /* username[0] == '\0' must be rejected before reaching host. */
  EXPECT(capy_user_create(&ctx, &acc, "p") == -1);
  EXPECT(state.create_calls == 0);
}

static void test_user_update_roundtrip(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct user_test_state state;
  struct capy_user_directory dir;
  struct capy_user_account acc;
  capy_widget_context_init(&ctx, &allocator);
  user_test_seed(&state, &dir);
  capy_widget_set_user_directory(&ctx, &dir);
  acc = state.roster[1];
  acc.is_admin = 1u;
  acc.is_locked = 1u;
  EXPECT(capy_user_update(&ctx, &acc) == 0);
  EXPECT(state.update_calls == 1);
  EXPECT(state.roster[1].is_admin == 1u);
  EXPECT(state.roster[1].is_locked == 1u);
}

static void test_user_delete_rejects_root_uid_zero(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct user_test_state state;
  struct capy_user_directory dir;
  capy_widget_context_init(&ctx, &allocator);
  user_test_seed(&state, &dir);
  capy_widget_set_user_directory(&ctx, &dir);
  /* uid 0 rejected before reaching host. */
  EXPECT(capy_user_delete(&ctx, 0u) == -1);
  EXPECT(state.delete_calls == 0);
  EXPECT(state.roster_count == 2u);
  /* Normal uid passes through. */
  EXPECT(capy_user_delete(&ctx, 1000u) == 0);
  EXPECT(state.delete_calls == 1);
  EXPECT(state.last_deleted_uid == 1000u);
  EXPECT(state.roster_count == 1u);
}

static void test_user_set_avatar_clear_and_set(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct user_test_state state;
  struct capy_user_directory dir;
  static const uint8_t fake_png[16] = {0u};
  capy_widget_context_init(&ctx, &allocator);
  user_test_seed(&state, &dir);
  capy_widget_set_user_directory(&ctx, &dir);
  /* Set: len > 0 + non-NULL. */
  EXPECT(capy_user_set_avatar(&ctx, 1000u, fake_png, (uint32_t)sizeof fake_png) == 0);
  EXPECT(state.avatar_calls == 1);
  EXPECT(state.last_avatar_uid == 1000u);
  EXPECT(state.last_avatar_len == 16u);
  EXPECT(state.roster[1].avatar_image_id != 0u);
  /* Clear: len = 0 + NULL ok. */
  EXPECT(capy_user_set_avatar(&ctx, 1000u, 0, 0u) == 0);
  EXPECT(state.last_avatar_len == 0u);
  EXPECT(state.roster[1].avatar_image_id == 0u);
  /* len > 0 + NULL → -1 without reaching host. */
  EXPECT(capy_user_set_avatar(&ctx, 1000u, 0, 8u) == -1);
  EXPECT(state.avatar_calls == 2);
}

static void test_user_no_directory_returns_negative(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_user_account acc;
  struct capy_user_account out[2];
  capy_widget_context_init(&ctx, &allocator);
  zero_mem(&acc, sizeof acc);
  user_test_copy_str(acc.username, "u", CAPY_USER_NAME_MAX);
  acc.uid = 9000u;
  EXPECT(capy_user_list(&ctx, out, 2u) == -1);
  EXPECT(capy_user_create(&ctx, &acc, "p") == -1);
  EXPECT(capy_user_update(&ctx, &acc) == -1);
  EXPECT(capy_user_delete(&ctx, 9000u) == -1);
  EXPECT(capy_user_set_avatar(&ctx, 9000u, 0, 0u) == -1);
  /* NULL ctx in setter is a no-op (no crash). */
  capy_widget_set_user_directory(0, 0);
}

/* ── v2.8: contrast & accessibility (since 2.8.0) ───────────────────────── */

static void test_contrast_white_on_black_is_21000(void) {
  uint32_t r = capy_theme_contrast_ratio_x1000(0xFFFFFFFFu, 0xFF000000u);
  EXPECT(r == 21000u);
  /* Symmetry: black on white must yield the same ratio. */
  EXPECT(capy_theme_contrast_ratio_x1000(0xFF000000u, 0xFFFFFFFFu) == 21000u);
}

static void test_contrast_same_color_is_1000(void) {
  EXPECT(capy_theme_contrast_ratio_x1000(0xFF7F7F7Fu, 0xFF7F7F7Fu) == 1000u);
  EXPECT(capy_theme_contrast_ratio_x1000(0xFFFF0000u, 0xFFFF0000u) == 1000u);
}

static void test_contrast_alpha_ignored(void) {
  /* High and low alpha must produce the same contrast for the same RGB. */
  uint32_t r1 = capy_theme_contrast_ratio_x1000(0xFFFFFFFFu, 0xFF000000u);
  uint32_t r2 = capy_theme_contrast_ratio_x1000(0x00FFFFFFu, 0x80000000u);
  EXPECT(r1 == r2);
}

static void test_contrast_ratio_in_bounds(void) {
  /* Mid-grey on black sits between 1000 and 21000. */
  uint32_t r = capy_theme_contrast_ratio_x1000(0xFF808080u, 0xFF000000u);
  EXPECT(r >= 1000u);
  EXPECT(r <= 21000u);
  /* Strictly less than white-on-black. */
  EXPECT(r < 21000u);
  EXPECT(r > 1000u);
}

static void test_audit_dry_run_returns_total(void) {
  struct capy_theme t = capy_theme_default_light();
  int total = capy_theme_audit_wcag(&t, 0, 0u);
  EXPECT(total > 0);
  EXPECT(total < 64); /* sanity: not absurd */
}

static void test_audit_high_contrast_all_pass_aa(void) {
  struct capy_theme t = capy_theme_high_contrast();
  struct capy_contrast_finding findings[32];
  int n = capy_theme_audit_wcag(&t, findings, 32u);
  int i;
  EXPECT(n > 0);
  for (i = 0; i < n; ++i) {
    /* DISABLED #808080 vs BG_BASE is the known low-contrast pair in
     * the high_contrast theme (it must look disabled). Skip it for the
     * AA invariant. */
    if (findings[i].fg_token == CAPY_TOKEN_DISABLED) {
      continue;
    }
    EXPECT(findings[i].passes_aa == 1u);
    EXPECT(findings[i].ratio_x1000 >= 4500u);
  }
}

static void test_audit_dark_high_contrast_all_pass_aaa(void) {
  struct capy_theme t = capy_theme_default_dark_high_contrast();
  struct capy_contrast_finding findings[32];
  int n = capy_theme_audit_wcag(&t, findings, 32u);
  int i;
  EXPECT(n > 0);
  EXPECT(t.variant == (uint8_t)CAPY_THEME_VARIANT_DARK_HIGH_CONTRAST);
  EXPECT(t.high_contrast == 1u);
  for (i = 0; i < n; ++i) {
    EXPECT(findings[i].passes_aa == 1u);
    EXPECT(findings[i].passes_aaa == 1u);
    EXPECT(findings[i].ratio_x1000 >= 7000u);
  }
}

static void test_audit_null_and_capacity_overflow(void) {
  struct capy_theme t = capy_theme_default_light();
  struct capy_contrast_finding findings[4];
  int total;
  int n;
  EXPECT(capy_theme_audit_wcag(0, findings, 4u) == -1);
  /* cap > 0 with NULL out → -1. */
  EXPECT(capy_theme_audit_wcag(&t, 0, 4u) == -1);
  total = capy_theme_audit_wcag(&t, 0, 0u);
  EXPECT(total > 4);
  /* Cap below total: returns cap. */
  n = capy_theme_audit_wcag(&t, findings, 4u);
  EXPECT(n == 4);
}

/* ── v2.9: desktop icon arrangement (since 2.9.0) ───────────────────────── */

static struct capy_desktop_layout desktop_test_layout(void) {
  struct capy_desktop_layout l;
  l.cell_w = 96u;
  l.cell_h = 96u;
  l.snap = 1u;
  l.auto_arrange = 0u;
  l.sort_by = CAPY_DESKTOP_SORT_MANUAL;
  l.flags = 0u;
  return l;
}

static void desktop_test_seed_positions(struct capy_desktop_icon_position *p,
                                        uint32_t n) {
  uint32_t i;
  for (i = 0u; i < n; ++i) {
    p[i].icon_id = 1000u + i;
    p[i].x = (int16_t)(i * 96);
    p[i].y = (int16_t)((i / 4u) * 96);
    p[i].grid_x = (int16_t)(i % 4u);
    p[i].grid_y = (int16_t)(i / 4u);
    p[i].pinned = (uint8_t)((i == 0u) ? 1u : 0u);
    p[i].reserved[0] = 0u;
    p[i].reserved[1] = 0u;
    p[i].reserved[2] = 0u;
  }
}

static void test_desktop_layout_roundtrip(void) {
  uint8_t buf[256];
  struct capy_desktop_layout layout = desktop_test_layout();
  struct capy_desktop_icon_position positions[5];
  struct capy_desktop_layout loaded;
  struct capy_desktop_icon_position out_pos[5];
  uint32_t loaded_count = 0u;
  int n;
  uint32_t i;
  desktop_test_seed_positions(positions, 5u);
  n = capy_desktop_layout_serialize(&layout, positions, 5u, buf, sizeof buf);
  EXPECT(n > 0);
  EXPECT((uint32_t)n == 24u + 5u * 16u);
  EXPECT(capy_desktop_layout_validate(buf, (uint32_t)n) == 0);
  EXPECT(capy_desktop_layout_load(buf, (uint32_t)n, &loaded, out_pos, 5u,
                                  &loaded_count) == 0);
  EXPECT(loaded_count == 5u);
  EXPECT(loaded.cell_w == 96u);
  EXPECT(loaded.cell_h == 96u);
  EXPECT(loaded.snap == 1u);
  EXPECT(loaded.sort_by == CAPY_DESKTOP_SORT_MANUAL);
  for (i = 0u; i < 5u; ++i) {
    EXPECT(out_pos[i].icon_id == positions[i].icon_id);
    EXPECT(out_pos[i].x == positions[i].x);
    EXPECT(out_pos[i].grid_x == positions[i].grid_x);
    EXPECT(out_pos[i].pinned == positions[i].pinned);
  }
}

static void test_desktop_layout_empty_roundtrip(void) {
  uint8_t buf[64];
  struct capy_desktop_layout layout = desktop_test_layout();
  struct capy_desktop_layout loaded;
  uint32_t count = 99u;
  int n = capy_desktop_layout_serialize(&layout, 0, 0u, buf, sizeof buf);
  EXPECT(n == 24);
  EXPECT(capy_desktop_layout_load(buf, (uint32_t)n, &loaded, 0, 0u,
                                  &count) == 0);
  EXPECT(count == 0u);
  EXPECT(loaded.cell_w == 96u);
}

static void test_desktop_layout_bad_magic_rejected(void) {
  uint8_t buf[64];
  struct capy_desktop_layout layout = desktop_test_layout();
  int n = capy_desktop_layout_serialize(&layout, 0, 0u, buf, sizeof buf);
  EXPECT(n > 0);
  buf[0] = 'X';
  EXPECT(capy_desktop_layout_validate(buf, (uint32_t)n) == -1);
}

static void test_desktop_layout_checksum_mismatch_rejected(void) {
  uint8_t buf[64];
  struct capy_desktop_layout layout = desktop_test_layout();
  struct capy_desktop_icon_position positions[2];
  int n;
  desktop_test_seed_positions(positions, 2u);
  n = capy_desktop_layout_serialize(&layout, positions, 2u, buf, sizeof buf);
  EXPECT(n > 0);
  /* Flip a body byte without re-checksumming. */
  buf[24] ^= 0xFFu;
  EXPECT(capy_desktop_layout_validate(buf, (uint32_t)n) == -1);
}

static void test_desktop_layout_zero_cell_rejected(void) {
  uint8_t buf[64];
  struct capy_desktop_layout layout = desktop_test_layout();
  layout.cell_w = 0u;
  EXPECT(capy_desktop_layout_serialize(&layout, 0, 0u, buf, sizeof buf) == -1);
  layout = desktop_test_layout();
  layout.cell_h = 0u;
  EXPECT(capy_desktop_layout_serialize(&layout, 0, 0u, buf, sizeof buf) == -1);
}

static void test_desktop_layout_invalid_sort_rejected(void) {
  uint8_t buf[64];
  struct capy_desktop_layout layout = desktop_test_layout();
  layout.sort_by = CAPY_DESKTOP_SORT_COUNT;
  EXPECT(capy_desktop_layout_serialize(&layout, 0, 0u, buf, sizeof buf) == -1);
  layout.sort_by = 250u;
  EXPECT(capy_desktop_layout_serialize(&layout, 0, 0u, buf, sizeof buf) == -1);
}

static void test_desktop_layout_short_buffer_reports_count(void) {
  uint8_t buf[256];
  struct capy_desktop_layout layout = desktop_test_layout();
  struct capy_desktop_icon_position positions[8];
  struct capy_desktop_layout loaded;
  struct capy_desktop_icon_position out_pos[3];
  uint32_t count = 0u;
  int n;
  desktop_test_seed_positions(positions, 8u);
  n = capy_desktop_layout_serialize(&layout, positions, 8u, buf, sizeof buf);
  EXPECT(n > 0);
  /* cap=3 < 8 declared → load still succeeds, count reports 8. */
  EXPECT(capy_desktop_layout_load(buf, (uint32_t)n, &loaded, out_pos, 3u,
                                  &count) == 0);
  EXPECT(count == 8u);
  EXPECT(out_pos[0].icon_id == 1000u);
  EXPECT(out_pos[2].icon_id == 1002u);
}

static void test_desktop_layout_null_guards(void) {
  uint8_t buf[64];
  struct capy_desktop_layout layout = desktop_test_layout();
  struct capy_desktop_layout out_layout;
  uint32_t count = 0u;
  int n = capy_desktop_layout_serialize(&layout, 0, 0u, buf, sizeof buf);
  EXPECT(n > 0);
  /* serialize NULL guards. */
  EXPECT(capy_desktop_layout_serialize(0, 0, 0u, buf, sizeof buf) == -1);
  EXPECT(capy_desktop_layout_serialize(&layout, 0, 0u, 0, sizeof buf) == -1);
  /* count > 0 with NULL positions. */
  EXPECT(capy_desktop_layout_serialize(&layout, 0, 3u, buf, sizeof buf) == -1);
  /* validate NULL. */
  EXPECT(capy_desktop_layout_validate(0, (uint32_t)n) == -1);
  /* load NULL guards. */
  EXPECT(capy_desktop_layout_load(buf, (uint32_t)n, 0, 0, 0u, &count) == -1);
  EXPECT(capy_desktop_layout_load(buf, (uint32_t)n, &out_layout, 0, 0u,
                                  0) == -1);
}

/* ── v2.10: file manager UX plumbing (since 2.10.0) ─────────────────────── */

static void breadcrumb_seed(struct capy_breadcrumb_segment *segs, uint32_t n) {
  uint32_t i;
  for (i = 0u; i < n; ++i) {
    segs[i].id = 100u + i;
    segs[i].text_offset = (uint16_t)(i * 8u);
    segs[i].text_len = 8u;
    segs[i].icon_image_id = (uint16_t)(2000u + i);
    segs[i].is_clickable = 1u;
    segs[i].reserved = 0u;
  }
}

static void test_breadcrumb_full_fit_no_dropdown(void) {
  struct capy_breadcrumb_segment in[4];
  struct capy_breadcrumb_segment out[8];
  uint32_t out_count = 99u;
  uint8_t dropdown = 99u;
  uint32_t i;
  breadcrumb_seed(in, 4u);
  /* 4 segments × 80 px = 320 px <= 800 px → full fit. */
  EXPECT(capy_breadcrumb_truncate(in, 4u, 800u, 80u, out, 8u, &out_count,
                                  &dropdown) == 0);
  EXPECT(out_count == 4u);
  EXPECT(dropdown == 0u);
  for (i = 0u; i < 4u; ++i) {
    EXPECT(out[i].id == in[i].id);
  }
}

static void test_breadcrumb_truncated_root_plus_tail(void) {
  struct capy_breadcrumb_segment in[8];
  struct capy_breadcrumb_segment out[8];
  uint32_t out_count = 0u;
  uint8_t dropdown = 0u;
  breadcrumb_seed(in, 8u);
  /* 8 × 100 = 800 > 300 → truncated. max_inline = 3. */
  EXPECT(capy_breadcrumb_truncate(in, 8u, 300u, 100u, out, 8u, &out_count,
                                  &dropdown) == 0);
  EXPECT(dropdown == 1u);
  EXPECT(out_count == 3u);
  /* Root first. */
  EXPECT(out[0].id == in[0].id);
  /* Last segments are the tail. */
  EXPECT(out[1].id == in[6].id);
  EXPECT(out[2].id == in[7].id);
}

static void test_breadcrumb_min_two_with_narrow_width(void) {
  struct capy_breadcrumb_segment in[5];
  struct capy_breadcrumb_segment out[4];
  uint32_t out_count = 0u;
  uint8_t dropdown = 0u;
  breadcrumb_seed(in, 5u);
  /* available=50, avg=100 → would be 0 inline, but we floor at 2. */
  EXPECT(capy_breadcrumb_truncate(in, 5u, 50u, 100u, out, 4u, &out_count,
                                  &dropdown) == 0);
  EXPECT(out_count == 2u);
  EXPECT(dropdown == 1u);
  EXPECT(out[0].id == in[0].id);
  EXPECT(out[1].id == in[4].id);
}

static void test_breadcrumb_empty_input(void) {
  struct capy_breadcrumb_segment out[4];
  uint32_t out_count = 99u;
  uint8_t dropdown = 99u;
  EXPECT(capy_breadcrumb_truncate(0, 0u, 800u, 80u, out, 4u, &out_count,
                                  &dropdown) == 0);
  EXPECT(out_count == 0u);
  EXPECT(dropdown == 0u);
}

static void test_breadcrumb_single_segment(void) {
  struct capy_breadcrumb_segment in[1];
  struct capy_breadcrumb_segment out[2];
  uint32_t out_count = 0u;
  uint8_t dropdown = 1u;
  breadcrumb_seed(in, 1u);
  EXPECT(capy_breadcrumb_truncate(in, 1u, 800u, 100u, out, 2u, &out_count,
                                  &dropdown) == 0);
  EXPECT(out_count == 1u);
  EXPECT(dropdown == 0u);
  EXPECT(out[0].id == in[0].id);
}

static void test_breadcrumb_null_guards(void) {
  struct capy_breadcrumb_segment in[2];
  struct capy_breadcrumb_segment out[2];
  uint32_t out_count = 0u;
  uint8_t dropdown = 0u;
  breadcrumb_seed(in, 2u);
  /* NULL out_count / out_dropdown. */
  EXPECT(capy_breadcrumb_truncate(in, 2u, 800u, 100u, out, 2u, 0,
                                  &dropdown) == -1);
  EXPECT(capy_breadcrumb_truncate(in, 2u, 800u, 100u, out, 2u, &out_count,
                                  0) == -1);
  /* NULL in/out with non-zero in_count. */
  EXPECT(capy_breadcrumb_truncate(0, 2u, 800u, 100u, out, 2u, &out_count,
                                  &dropdown) == -1);
  EXPECT(capy_breadcrumb_truncate(in, 2u, 800u, 100u, 0, 2u, &out_count,
                                  &dropdown) == -1);
  /* segment_avg_px == 0. */
  EXPECT(capy_breadcrumb_truncate(in, 2u, 800u, 0u, out, 2u, &out_count,
                                  &dropdown) == -1);
  /* cap == 0 with non-zero input. */
  EXPECT(capy_breadcrumb_truncate(in, 2u, 800u, 100u, out, 0u, &out_count,
                                  &dropdown) == -1);
}

static void test_breadcrumb_cap_too_small_for_full_fit(void) {
  struct capy_breadcrumb_segment in[4];
  struct capy_breadcrumb_segment out[2];
  uint32_t out_count = 0u;
  uint8_t dropdown = 0u;
  breadcrumb_seed(in, 4u);
  /* Would full-fit, but cap=2 < in_count=4 → -1. */
  EXPECT(capy_breadcrumb_truncate(in, 4u, 800u, 80u, out, 2u, &out_count,
                                  &dropdown) == -1);
}

static void test_toolbar_group_taxonomy(void) {
  /* Stable taxonomy: NAV/VIEW/ACTION/LAYOUT == 0/1/2/3, COUNT == 4. */
  EXPECT(CAPY_TOOLBAR_GROUP_NAV == 0u);
  EXPECT(CAPY_TOOLBAR_GROUP_VIEW == 1u);
  EXPECT(CAPY_TOOLBAR_GROUP_ACTION == 2u);
  EXPECT(CAPY_TOOLBAR_GROUP_LAYOUT == 3u);
  EXPECT(CAPY_TOOLBAR_GROUP_COUNT == 4u);
  EXPECT(CAPY_FILE_MGR_COMPACT_THRESHOLD_PX == 600u);
}

/* ── v2.11: icon & thumbnail system (since 2.11.0) ──────────────────────── */

struct icon_test_state {
  int resolve_calls;
  int thumb_calls;
  uint32_t last_request_id;
  uint32_t next_request_id;
  uint32_t resolve_return_icon;
  int resolve_should_fail;
  int thumb_should_fail;
  uint32_t thumb_should_return_zero;
};

static int icon_test_resolve(void *user_data, const char *mime,
                             const char *ext, uint32_t *out_id) {
  struct icon_test_state *s = (struct icon_test_state *)user_data;
  ++s->resolve_calls;
  (void)mime;
  (void)ext;
  if (s->resolve_should_fail) {
    return -1;
  }
  *out_id = s->resolve_return_icon;
  return 0;
}

static int icon_test_thumb(void *user_data, const char *path,
                           uint32_t *out_request_id) {
  struct icon_test_state *s = (struct icon_test_state *)user_data;
  ++s->thumb_calls;
  (void)path;
  if (s->thumb_should_fail) {
    return -1;
  }
  if (s->thumb_should_return_zero) {
    *out_request_id = 0u;
    return 0;
  }
  ++s->next_request_id;
  *out_request_id = s->next_request_id;
  s->last_request_id = s->next_request_id;
  return 0;
}

static void icon_test_seed(struct icon_test_state *s,
                           struct capy_icon_provider *p) {
  uint32_t i;
  for (i = 0u; i < (uint32_t)sizeof *s; ++i) {
    ((char *)s)[i] = 0;
  }
  s->resolve_return_icon = 42u;
  s->next_request_id = 100u;
  p->resolve = icon_test_resolve;
  p->thumbnail_request = icon_test_thumb;
  p->user_data = s;
}

static void test_icon_resolve_forwards_to_host(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct icon_test_state state;
  struct capy_icon_provider provider;
  uint32_t id = 99u;
  capy_widget_context_init(&ctx, &allocator);
  icon_test_seed(&state, &provider);
  capy_widget_set_icon_provider(&ctx, &provider);
  EXPECT(capy_icon_resolve(&ctx, "image/png", "png", &id) == 0);
  EXPECT(state.resolve_calls == 1);
  EXPECT(id == 42u);
}

static void test_icon_resolve_fallback_no_provider(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  uint32_t id = 99u;
  capy_widget_context_init(&ctx, &allocator);
  /* No provider attached: deterministic image_id=0 fallback. */
  EXPECT(capy_icon_resolve(&ctx, "image/png", "png", &id) == 0);
  EXPECT(id == 0u);
}

static void test_icon_resolve_fallback_host_failure(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct icon_test_state state;
  struct capy_icon_provider provider;
  uint32_t id = 99u;
  capy_widget_context_init(&ctx, &allocator);
  icon_test_seed(&state, &provider);
  state.resolve_should_fail = 1;
  capy_widget_set_icon_provider(&ctx, &provider);
  EXPECT(capy_icon_resolve(&ctx, "application/x-unknown", "xyz", &id) == 0);
  EXPECT(state.resolve_calls == 1);
  /* Host returned -1 → fallback to 0. */
  EXPECT(id == 0u);
}

static void test_icon_thumbnail_request_forwards(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct icon_test_state state;
  struct capy_icon_provider provider;
  uint32_t req = 0u;
  capy_widget_context_init(&ctx, &allocator);
  icon_test_seed(&state, &provider);
  capy_widget_set_icon_provider(&ctx, &provider);
  EXPECT(capy_icon_thumbnail_request(&ctx, "/home/x.png", &req) == 0);
  EXPECT(state.thumb_calls == 1);
  EXPECT(req == 101u);
}

static void test_icon_thumbnail_request_failure_modes(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct icon_test_state state;
  struct capy_icon_provider provider;
  uint32_t req = 0u;
  capy_widget_context_init(&ctx, &allocator);
  icon_test_seed(&state, &provider);
  capy_widget_set_icon_provider(&ctx, &provider);
  /* Host returns -1 → -1. */
  state.thumb_should_fail = 1;
  EXPECT(capy_icon_thumbnail_request(&ctx, "/x", &req) == -1);
  EXPECT(req == 0u);
  /* Host writes request_id = 0 → -1 (reserved id). */
  state.thumb_should_fail = 0;
  state.thumb_should_return_zero = 1;
  EXPECT(capy_icon_thumbnail_request(&ctx, "/x", &req) == -1);
  EXPECT(req == 0u);
}

static void test_icon_thumbnail_ready_rejects_id_zero(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  capy_widget_context_init(&ctx, &allocator);
  /* request_id=0 is reserved → -1. */
  EXPECT(capy_icon_thumbnail_ready(&ctx, 0, 0u, 555u) == -1);
  /* Otherwise OK even without provider (event is no-op on NULL root). */
  EXPECT(capy_icon_thumbnail_ready(&ctx, 0, 17u, 555u) == 0);
}

static void test_icon_no_provider_thumbnail_request_returns_negative(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  uint32_t req = 999u;
  capy_widget_context_init(&ctx, &allocator);
  EXPECT(capy_icon_thumbnail_request(&ctx, "/p", &req) == -1);
  EXPECT(req == 0u);
}

static void test_icon_null_guards(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  uint32_t id = 99u;
  uint32_t req = 99u;
  capy_widget_context_init(&ctx, &allocator);
  EXPECT(capy_icon_resolve(0, "m", "e", &id) == -1);
  EXPECT(capy_icon_resolve(&ctx, "m", "e", 0) == -1);
  EXPECT(capy_icon_thumbnail_request(0, "/p", &req) == -1);
  EXPECT(capy_icon_thumbnail_request(&ctx, 0, &req) == -1);
  EXPECT(capy_icon_thumbnail_request(&ctx, "/p", 0) == -1);
  EXPECT(capy_icon_thumbnail_ready(0, 0, 1u, 1u) == -1);
  /* set_icon_provider NULL ctx is no-op. */
  capy_widget_set_icon_provider(0, 0);
}

/* ── v2.12: wallpaper management (since 2.12.0) ─────────────────────────── */

static struct capy_wallpaper_config wallpaper_test_config(void) {
  struct capy_wallpaper_config c;
  c.default_image_id = 7u;
  c.slideshow_interval_sec = 30u;
  c.monitor_index = 1u;
  c.fit_mode = CAPY_WALLPAPER_FIT_FILL;
  c.flags = CAPY_WALLPAPER_FLAG_PER_MONITOR;
  c.reserved = 0u;
  return c;
}

static void wallpaper_test_seed_slides(struct capy_wallpaper_slide *s,
                                       uint32_t n) {
  uint32_t i;
  for (i = 0u; i < n; ++i) {
    s[i].image_id = 100u + i;
    s[i].duration_sec = (uint16_t)(15u + i);
    s[i].reserved = 0u;
  }
}

static void test_wallpaper_roundtrip(void) {
  uint8_t buf[128];
  struct capy_wallpaper_config cfg = wallpaper_test_config();
  struct capy_wallpaper_slide slides[3];
  struct capy_wallpaper_config out_cfg;
  struct capy_wallpaper_slide out_slides[3];
  uint32_t count = 0u;
  int n;
  uint32_t i;
  wallpaper_test_seed_slides(slides, 3u);
  n = capy_wallpaper_serialize(&cfg, slides, 3u, buf, sizeof buf);
  EXPECT(n > 0);
  EXPECT((uint32_t)n == 24u + 3u * 8u);
  EXPECT(capy_wallpaper_validate(buf, (uint32_t)n) == 0);
  EXPECT(capy_wallpaper_load(buf, (uint32_t)n, &out_cfg, out_slides, 3u,
                             &count) == 0);
  EXPECT(count == 3u);
  EXPECT(out_cfg.default_image_id == 7u);
  EXPECT(out_cfg.slideshow_interval_sec == 30u);
  EXPECT(out_cfg.monitor_index == 1u);
  EXPECT(out_cfg.fit_mode == CAPY_WALLPAPER_FIT_FILL);
  EXPECT(out_cfg.flags == CAPY_WALLPAPER_FLAG_PER_MONITOR);
  for (i = 0u; i < 3u; ++i) {
    EXPECT(out_slides[i].image_id == slides[i].image_id);
    EXPECT(out_slides[i].duration_sec == slides[i].duration_sec);
  }
}

static void test_wallpaper_no_slides_roundtrip(void) {
  uint8_t buf[64];
  struct capy_wallpaper_config cfg = wallpaper_test_config();
  struct capy_wallpaper_config out_cfg;
  uint32_t count = 99u;
  int n;
  cfg.slideshow_interval_sec = 0u;
  n = capy_wallpaper_serialize(&cfg, 0, 0u, buf, sizeof buf);
  EXPECT(n == 24);
  EXPECT(capy_wallpaper_load(buf, (uint32_t)n, &out_cfg, 0, 0u, &count) == 0);
  EXPECT(count == 0u);
  EXPECT(out_cfg.default_image_id == 7u);
  EXPECT(out_cfg.slideshow_interval_sec == 0u);
}

static void test_wallpaper_invalid_fit_rejected(void) {
  uint8_t buf[64];
  struct capy_wallpaper_config cfg = wallpaper_test_config();
  cfg.fit_mode = CAPY_WALLPAPER_FIT_COUNT;
  EXPECT(capy_wallpaper_serialize(&cfg, 0, 0u, buf, sizeof buf) == -1);
  cfg.fit_mode = 250u;
  EXPECT(capy_wallpaper_serialize(&cfg, 0, 0u, buf, sizeof buf) == -1);
}

static void test_wallpaper_slide_zero_id_rejected(void) {
  uint8_t buf[64];
  struct capy_wallpaper_config cfg = wallpaper_test_config();
  struct capy_wallpaper_slide slides[2];
  wallpaper_test_seed_slides(slides, 2u);
  slides[1].image_id = 0u; /* invalid */
  EXPECT(capy_wallpaper_serialize(&cfg, slides, 2u, buf, sizeof buf) == -1);
  /* Also reject duration_sec == 0. */
  wallpaper_test_seed_slides(slides, 2u);
  slides[0].duration_sec = 0u;
  EXPECT(capy_wallpaper_serialize(&cfg, slides, 2u, buf, sizeof buf) == -1);
}

static void test_wallpaper_bad_magic_and_checksum_rejected(void) {
  uint8_t buf[128];
  struct capy_wallpaper_config cfg = wallpaper_test_config();
  struct capy_wallpaper_slide slides[2];
  int n;
  wallpaper_test_seed_slides(slides, 2u);
  n = capy_wallpaper_serialize(&cfg, slides, 2u, buf, sizeof buf);
  EXPECT(n > 0);
  /* Bad magic. */
  buf[0] = 'X';
  EXPECT(capy_wallpaper_validate(buf, (uint32_t)n) == -1);
  /* Restore magic, corrupt body → checksum mismatch. */
  buf[0] = CAPY_WALLPAPER_MAGIC0;
  buf[24] ^= 0xFFu;
  EXPECT(capy_wallpaper_validate(buf, (uint32_t)n) == -1);
}

static void test_wallpaper_short_buffer_reports_count(void) {
  uint8_t buf[256];
  struct capy_wallpaper_config cfg = wallpaper_test_config();
  struct capy_wallpaper_slide slides[6];
  struct capy_wallpaper_config out_cfg;
  struct capy_wallpaper_slide out_slides[2];
  uint32_t count = 0u;
  int n;
  wallpaper_test_seed_slides(slides, 6u);
  n = capy_wallpaper_serialize(&cfg, slides, 6u, buf, sizeof buf);
  EXPECT(n > 0);
  EXPECT(capy_wallpaper_load(buf, (uint32_t)n, &out_cfg, out_slides, 2u,
                             &count) == 0);
  EXPECT(count == 6u);
  EXPECT(out_slides[0].image_id == 100u);
  EXPECT(out_slides[1].image_id == 101u);
}

static void test_wallpaper_null_guards(void) {
  uint8_t buf[64];
  struct capy_wallpaper_config cfg = wallpaper_test_config();
  struct capy_wallpaper_config out_cfg;
  uint32_t count = 0u;
  int n;
  cfg.slideshow_interval_sec = 0u;
  n = capy_wallpaper_serialize(&cfg, 0, 0u, buf, sizeof buf);
  EXPECT(n > 0);
  EXPECT(capy_wallpaper_serialize(0, 0, 0u, buf, sizeof buf) == -1);
  EXPECT(capy_wallpaper_serialize(&cfg, 0, 0u, 0, sizeof buf) == -1);
  EXPECT(capy_wallpaper_serialize(&cfg, 0, 3u, buf, sizeof buf) == -1);
  EXPECT(capy_wallpaper_validate(0, (uint32_t)n) == -1);
  EXPECT(capy_wallpaper_load(buf, (uint32_t)n, 0, 0, 0u, &count) == -1);
  EXPECT(capy_wallpaper_load(buf, (uint32_t)n, &out_cfg, 0, 0u, 0) == -1);
}

/* ── v2.13: login screen with avatars (since 2.13.0) ────────────────────── */

static void test_login_layout_zero_or_one(void) {
  EXPECT(capy_login_choose_layout(0u) == CAPY_LOGIN_LAYOUT_SINGLE);
  EXPECT(capy_login_choose_layout(1u) == CAPY_LOGIN_LAYOUT_SINGLE);
}

static void test_login_layout_grid_range(void) {
  uint32_t n;
  for (n = 2u; n <= 6u; ++n) {
    EXPECT(capy_login_choose_layout(n) == CAPY_LOGIN_LAYOUT_GRID);
  }
}

static void test_login_layout_list_threshold(void) {
  /* 6 → grid; 7 → list (boundary). */
  EXPECT(capy_login_choose_layout(6u) == CAPY_LOGIN_LAYOUT_GRID);
  EXPECT(capy_login_choose_layout(7u) == CAPY_LOGIN_LAYOUT_LIST);
  EXPECT(capy_login_choose_layout(100u) == CAPY_LOGIN_LAYOUT_LIST);
  EXPECT(capy_login_choose_layout(0xFFFFFFFFu) == CAPY_LOGIN_LAYOUT_LIST);
}

static void test_login_layout_deterministic(void) {
  /* Same input → same output across repeated calls. */
  uint32_t i;
  for (i = 0u; i < 32u; ++i) {
    EXPECT(capy_login_choose_layout(3u) == CAPY_LOGIN_LAYOUT_GRID);
  }
}

static void test_login_layout_taxonomy(void) {
  /* Stable taxonomy: SINGLE/GRID/LIST = 0/1/2, COUNT = 3. */
  EXPECT(CAPY_LOGIN_LAYOUT_SINGLE == 0u);
  EXPECT(CAPY_LOGIN_LAYOUT_GRID == 1u);
  EXPECT(CAPY_LOGIN_LAYOUT_LIST == 2u);
  EXPECT(CAPY_LOGIN_LAYOUT_COUNT == 3u);
  EXPECT(CAPY_LOGIN_GRID_THRESHOLD == 2u);
  EXPECT(CAPY_LOGIN_LIST_THRESHOLD == 7u);
}

static void test_login_power_taxonomy(void) {
  /* Stable taxonomy: NONE/SHUTDOWN/REBOOT/SLEEP/LOCK/LOGOUT = 0..5, COUNT = 6. */
  EXPECT(CAPY_LOGIN_POWER_NONE == 0u);
  EXPECT(CAPY_LOGIN_POWER_SHUTDOWN == 1u);
  EXPECT(CAPY_LOGIN_POWER_REBOOT == 2u);
  EXPECT(CAPY_LOGIN_POWER_SLEEP == 3u);
  EXPECT(CAPY_LOGIN_POWER_LOCK == 4u);
  EXPECT(CAPY_LOGIN_POWER_LOGOUT == 5u);
  EXPECT(CAPY_LOGIN_POWER_COUNT == 6u);
}

/* ── v2.2: virtualization data source (since 2.2.0) ─────────────────────── */

/* Synthetic data source: an array of integers rendered as decimal strings.
 * Tests confirm the widget core threads `user_data` through verbatim and
 * never touches the bytes itself. */
struct virtual_test_data {
  uint32_t count;
  int get_count_calls;
  int get_item_calls;
};

static int virtual_test_get_count(void *user_data) {
  struct virtual_test_data *d = (struct virtual_test_data *)user_data;
  ++d->get_count_calls;
  return (int)d->count;
}

static int virtual_test_get_item(void *user_data, uint32_t index,
                                 char *out_text, uint16_t cap) {
  struct virtual_test_data *d = (struct virtual_test_data *)user_data;
  ++d->get_item_calls;
  if (index >= d->count || cap == 0u) {
    return -1;
  }
  /* Format `index` in decimal, NUL-terminated, no allocation. */
  {
    char tmp[12];
    uint32_t n = 0u;
    uint32_t v = index;
    uint16_t i;
    if (v == 0u) {
      tmp[n++] = '0';
    } else {
      while (v > 0u && n < (uint32_t)sizeof(tmp)) {
        tmp[n++] = (char)('0' + (v % 10u));
        v /= 10u;
      }
    }
    if (n + 1u > (uint32_t)cap) {
      return -1;
    }
    for (i = 0u; i < (uint16_t)n; ++i) {
      out_text[i] = tmp[n - 1u - i];
    }
    out_text[n] = '\0';
  }
  return 0;
}

static void test_virtual_set_source_stores_pointer(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *list;
  struct virtual_test_data data = {1000000u, 0, 0};
  struct capy_virtual_data_source src = {virtual_test_get_count,
                                         virtual_test_get_item, &data};
  capy_widget_context_init(&ctx, &allocator);
  list = capy_widget_create(&ctx, CAPY_WIDGET_LIST);
  EXPECT(list->virtual_source == 0);
  capy_widget_set_virtual_source(list, &src);
  EXPECT(list->virtual_source == &src);
  /* Clear with NULL. */
  capy_widget_set_virtual_source(list, 0);
  EXPECT(list->virtual_source == 0);
  /* NULL widget is a no-op (no crash). */
  capy_widget_set_virtual_source(0, &src);
  capy_widget_destroy(list);
}

static void test_virtual_count_forwards_to_callback(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *list;
  struct virtual_test_data data = {42u, 0, 0};
  struct capy_virtual_data_source src = {virtual_test_get_count,
                                         virtual_test_get_item, &data};
  capy_widget_context_init(&ctx, &allocator);
  list = capy_widget_create(&ctx, CAPY_WIDGET_LIST);
  capy_widget_set_virtual_source(list, &src);
  EXPECT(capy_widget_virtual_count(list) == 42);
  EXPECT(data.get_count_calls == 1);
  capy_widget_destroy(list);
}

static void test_virtual_get_item_decodes_index(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *list;
  struct virtual_test_data data = {1000u, 0, 0};
  struct capy_virtual_data_source src = {virtual_test_get_count,
                                         virtual_test_get_item, &data};
  char buf[16];
  capy_widget_context_init(&ctx, &allocator);
  list = capy_widget_create(&ctx, CAPY_WIDGET_LIST);
  capy_widget_set_virtual_source(list, &src);
  EXPECT(capy_widget_virtual_get_item(list, 0u, buf, sizeof(buf)) == 0);
  EXPECT(buf[0] == '0' && buf[1] == '\0');
  EXPECT(capy_widget_virtual_get_item(list, 42u, buf, sizeof(buf)) == 0);
  EXPECT(buf[0] == '4' && buf[1] == '2' && buf[2] == '\0');
  EXPECT(capy_widget_virtual_get_item(list, 999u, buf, sizeof(buf)) == 0);
  EXPECT(buf[0] == '9' && buf[1] == '9' && buf[2] == '9' && buf[3] == '\0');
  /* Out-of-range index: callback returns -1. */
  EXPECT(capy_widget_virtual_get_item(list, 1000u, buf, sizeof(buf)) == -1);
  EXPECT(data.get_item_calls == 4);
  capy_widget_destroy(list);
}

static void test_virtual_null_source_returns_negative(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *list;
  char buf[8];
  capy_widget_context_init(&ctx, &allocator);
  list = capy_widget_create(&ctx, CAPY_WIDGET_LIST);
  /* No source attached: both helpers must return -1. */
  EXPECT(capy_widget_virtual_count(list) == -1);
  EXPECT(capy_widget_virtual_get_item(list, 0u, buf, sizeof(buf)) == -1);
  /* Attach a source with NULL function pointers. */
  {
    struct capy_virtual_data_source bad = {0, 0, 0};
    capy_widget_set_virtual_source(list, &bad);
    EXPECT(capy_widget_virtual_count(list) == -1);
    EXPECT(capy_widget_virtual_get_item(list, 0u, buf, sizeof(buf)) == -1);
  }
  capy_widget_destroy(list);
}

static void test_virtual_helpers_null_guards(void) {
  struct virtual_test_data data = {10u, 0, 0};
  struct capy_virtual_data_source src = {virtual_test_get_count,
                                         virtual_test_get_item, &data};
  char buf[8];
  /* NULL widget. */
  EXPECT(capy_widget_virtual_count(0) == -1);
  EXPECT(capy_widget_virtual_get_item(0, 0u, buf, sizeof(buf)) == -1);
  /* Suppress unused warnings. */
  (void)src;
  /* NULL out_text and cap=0 in get_item. */
  {
    struct test_heap heap = {{0}, 0u, 0u};
    struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
    struct capy_widget_context ctx;
    struct capy_widget *list;
    capy_widget_context_init(&ctx, &allocator);
    list = capy_widget_create(&ctx, CAPY_WIDGET_LIST);
    capy_widget_set_virtual_source(list, &src);
    EXPECT(capy_widget_virtual_get_item(list, 0u, 0, sizeof(buf)) == -1);
    EXPECT(capy_widget_virtual_get_item(list, 0u, buf, 0u) == -1);
    EXPECT(data.get_item_calls == 0); /* never reached the callback */
    capy_widget_destroy(list);
  }
}

static void test_virtual_request_range_event_routes_without_crash(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *list;
  struct capy_widget_event ev;
  capy_widget_context_init(&ctx, &allocator);
  list = capy_widget_create(&ctx, CAPY_WIDGET_LIST);
  capy_widget_set_bounds(list, 0, 0, 100u, 200u);
  ev.type = CAPY_WIDGET_EVENT_VIRTUAL_REQUEST_RANGE;
  ev.x = 100; /* start index */
  ev.y = 150; /* exclusive end */
  ev.buttons = 0u;
  ev.keycode = 0u;
  ev.modifiers = 0u;
  ev.payload = 0;
  /* Core treats this as a pass-through; host listens via its own
   * dispatcher loop. */
  EXPECT(capy_widget_handle_event(list, &ev) == 0);
  capy_widget_destroy(list);
}

static void test_virtual_source_on_non_list_widget_tolerated(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *label;
  struct virtual_test_data data = {5u, 0, 0};
  struct capy_virtual_data_source src = {virtual_test_get_count,
                                         virtual_test_get_item, &data};
  capy_widget_context_init(&ctx, &allocator);
  /* Virtualization is meaningful on LIST/TREE/TABLE but the core does
   * not enforce that — the source is stored on any widget. The host
   * renderer decides whether to honour it. */
  label = capy_widget_create(&ctx, CAPY_WIDGET_LABEL);
  capy_widget_set_virtual_source(label, &src);
  EXPECT(label->virtual_source == &src);
  EXPECT(capy_widget_virtual_count(label) == 5);
  capy_widget_destroy(label);
}

/* ── v2.1: advanced widget types (since 2.1.0) ──────────────────────────── */

/* Each helper exercises the generic widget pipeline on one of the eight
 * 2.1 widget types so the tail-field shape, layout walk, set_text path
 * and a11y/serialize coexistence remain healthy after the enum grew. */

static void test_advanced_widget_tree_creates_and_lays_out(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *tree;
  capy_widget_context_init(&ctx, &allocator);
  tree = capy_widget_create(&ctx, CAPY_WIDGET_TREE);
  EXPECT(tree != 0);
  EXPECT(tree->type == CAPY_WIDGET_TREE);
  capy_widget_set_bounds(tree, 0, 0, 120u, 200u);
  capy_widget_set_text(tree, "root");
  EXPECT(tree->bounds.width == 120u);
  EXPECT(tree->text[0] == 'r');
  capy_widget_destroy(tree);
}

static void test_advanced_widget_table_in_panel_emits(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *panel;
  struct capy_widget *table;
  struct capy_dl_cmd cmds[8];
  char text[16];
  struct capy_display_list dl;
  capy_widget_context_init(&ctx, &allocator);
  panel = capy_widget_create(&ctx, CAPY_WIDGET_PANEL);
  capy_widget_set_bounds(panel, 0, 0, 200u, 200u);
  table = capy_widget_create(&ctx, CAPY_WIDGET_TABLE);
  capy_widget_set_bounds(table, 5, 5, 180u, 180u);
  capy_widget_add_child(panel, table);
  capy_display_list_init(&dl, cmds, 8u, text, 16u);
  EXPECT(capy_widget_emit(panel, &dl) == 0);
  EXPECT(dl.count >= 2u); /* at least CLIP_PUSH for panel + table */
  capy_widget_destroy(panel);
}

static void test_advanced_widget_rich_text_basic(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *rt;
  capy_widget_context_init(&ctx, &allocator);
  rt = capy_widget_create(&ctx, CAPY_WIDGET_RICH_TEXT);
  EXPECT(rt != 0);
  EXPECT(rt->type == CAPY_WIDGET_RICH_TEXT);
  capy_widget_set_bounds(rt, 10, 20, 200u, 60u);
  capy_widget_set_text(rt, "Hello, world!");
  EXPECT(rt->text[0] == 'H' && rt->text[12] == '!');
  capy_widget_destroy(rt);
}

static void test_advanced_widget_canvas_creates(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *canvas;
  capy_widget_context_init(&ctx, &allocator);
  canvas = capy_widget_create(&ctx, CAPY_WIDGET_CANVAS);
  EXPECT(canvas != 0);
  EXPECT(canvas->type == CAPY_WIDGET_CANVAS);
  capy_widget_set_bounds(canvas, 0, 0, 64u, 64u);
  EXPECT(canvas->visible == 1u);
  EXPECT(canvas->enabled == 1u);
  capy_widget_destroy(canvas);
}

static void test_advanced_widget_chart_creates(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *chart;
  capy_widget_context_init(&ctx, &allocator);
  chart = capy_widget_create(&ctx, CAPY_WIDGET_CHART);
  EXPECT(chart != 0);
  EXPECT(chart->type == CAPY_WIDGET_CHART);
  /* Empty chart should not crash on the standard ops. */
  capy_widget_set_bounds(chart, 0, 0, 100u, 80u);
  capy_widget_set_visible(chart, 0);
  EXPECT(chart->visible == 0u);
  capy_widget_set_visible(chart, 1);
  EXPECT(chart->visible == 1u);
  capy_widget_destroy(chart);
}

static void test_advanced_widget_color_picker_focusable(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *cp;
  capy_widget_context_init(&ctx, &allocator);
  cp = capy_widget_create(&ctx, CAPY_WIDGET_COLOR_PICKER);
  EXPECT(cp != 0);
  EXPECT(cp->type == CAPY_WIDGET_COLOR_PICKER);
  /* Host marks pickers as focusable explicitly until a future minor
   * defaults this for the new types. */
  cp->focusable = 1u;
  cp->tab_index = 3;
  EXPECT(cp->focusable == 1u);
  EXPECT(cp->tab_index == 3);
  capy_widget_destroy(cp);
}

static void test_advanced_widget_date_picker_respects_rtl_context(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *dp;
  struct capy_locale rtl_locale = capy_locale_default();
  capy_widget_context_init(&ctx, &allocator);
  rtl_locale.rtl = 1u;
  capy_context_set_locale(&ctx, &rtl_locale);
  dp = capy_widget_create(&ctx, CAPY_WIDGET_DATE_PICKER);
  EXPECT(dp != 0);
  EXPECT(dp->type == CAPY_WIDGET_DATE_PICKER);
  /* The widget reads the locale through the context (since 0.13);
   * confirm the context flag is observable. */
  EXPECT(ctx.locale.rtl == 1u);
  capy_widget_destroy(dp);
}

static void test_advanced_widget_autocomplete_roundtrips_serialization(void) {
  struct test_heap heap_a = {{0}, 0u, 0u};
  struct test_heap heap_b = {{0}, 0u, 0u};
  struct capy_widget_allocator alloc_a = {test_alloc, test_free, &heap_a};
  struct capy_widget_allocator alloc_b = {test_alloc, test_free, &heap_b};
  struct capy_widget_context ctx_a;
  struct capy_widget_context ctx_b;
  struct capy_widget *ac;
  struct capy_widget *restored = 0;
  uint8_t buf[256];
  uint32_t out_len = 0u;
  capy_widget_context_init(&ctx_a, &alloc_a);
  capy_widget_context_init(&ctx_b, &alloc_b);
  ac = capy_widget_create(&ctx_a, CAPY_WIDGET_AUTOCOMPLETE);
  capy_widget_set_bounds(ac, 4, 5, 80u, 24u);
  capy_widget_set_text(ac, "Capy");
  EXPECT(capy_widget_serialize(ac, buf, sizeof(buf), &out_len) == 0);
  EXPECT(capy_widget_deserialize(buf, out_len, &ctx_b, &restored) == 0);
  EXPECT(restored != 0);
  EXPECT(restored->type == CAPY_WIDGET_AUTOCOMPLETE);
  EXPECT(restored->bounds.width == 80u);
  EXPECT(restored->text[0] == 'C' && restored->text[3] == 'y');
  /* Sanity check: enum ordering — new types come after CAPY_WIDGET_TABS
   * and AUTOCOMPLETE is the current tail entry. */
  EXPECT((int)CAPY_WIDGET_TREE > (int)CAPY_WIDGET_TABS);
  EXPECT((int)CAPY_WIDGET_AUTOCOMPLETE > (int)CAPY_WIDGET_DATE_PICKER);
  capy_widget_destroy(ac);
  capy_widget_destroy(restored);
}

/* ── v2.0: plugin ABI + DL schema 7 (since 2.0.0) ───────────────────────── */

static int plugin_test_init_called;
static int plugin_test_destroy_called;
static void *plugin_test_destroy_ctx_seen;

static int plugin_test_init(struct capy_plugin_context *pc) {
  (void)pc;
  ++plugin_test_init_called;
  return 0;
}

static void plugin_test_destroy(struct capy_plugin_context *pc) {
  ++plugin_test_destroy_called;
  plugin_test_destroy_ctx_seen = (void *)pc->descriptor;
}

static void test_plugin_version_tag_macro(void) {
  /* (major << 16) | (minor << 8) | patch. For 2.22.0 -> 0x00021600. */
  EXPECT(CAPYUI_API_VERSION_MAJOR == 2);
  EXPECT(CAPYUI_API_VERSION_MINOR == 22);
  EXPECT(CAPYUI_API_VERSION_PATCH == 0);
  EXPECT(CAPYUI_API_VERSION_TAG == 0x00021600u);
}

static void test_plugin_register_valid_descriptor(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  static const struct capy_plugin_descriptor desc = {
      "com.test.plugin",
      "0.1.0",
      0x00020000u,
      {0, 0, 0},
      1000u,
      plugin_test_init,
      plugin_test_destroy,
      0,
      0,
  };
  capy_widget_context_init(&ctx, &allocator);
  EXPECT(ctx.plugin_count == 0u);
  EXPECT(capy_plugin_register(&ctx, &desc) == 0);
  EXPECT(ctx.plugin_count == 1u);
  EXPECT(ctx.plugins[0] == &desc);
  /* Duplicate registration of the same descriptor is rejected. */
  EXPECT(capy_plugin_register(&ctx, &desc) == -1);
  EXPECT(ctx.plugin_count == 1u);
  capy_plugin_unregister_all(&ctx);
}

static void test_plugin_rejects_abi_mismatch(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  /* Plugin demanding a host newer than 2.0.x must be rejected. */
  static const struct capy_plugin_descriptor future = {
      "com.test.future",
      "9.9.9",
      0x00090000u, /* requires 9.0.0 host */
      {0, 0, 0},
      0u,
      0,
      0,
      0,
      0,
  };
  /* Plugin requiring exactly the current host version is accepted. */
  static const struct capy_plugin_descriptor current = {
      "com.test.current",
      "1.0.0",
      0x00020000u,
      {0, 0, 0},
      0u,
      0,
      0,
      0,
      0,
  };
  /* Plugin requiring an older host is also accepted. */
  static const struct capy_plugin_descriptor older = {
      "com.test.older",
      "1.0.0",
      0x00010000u,
      {0, 0, 0},
      0u,
      0,
      0,
      0,
      0,
  };
  capy_widget_context_init(&ctx, &allocator);
  EXPECT(capy_plugin_register(&ctx, &future) == -1);
  EXPECT(ctx.plugin_count == 0u);
  EXPECT(capy_plugin_register(&ctx, &current) == 0);
  EXPECT(capy_plugin_register(&ctx, &older) == 1);
  EXPECT(ctx.plugin_count == 2u);
  capy_plugin_unregister_all(&ctx);
}

static void test_plugin_capacity_overflow_rejected(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  static struct capy_plugin_descriptor d[CAPY_MAX_PLUGINS + 1u];
  uint32_t i;
  capy_widget_context_init(&ctx, &allocator);
  for (i = 0u; i < CAPY_MAX_PLUGINS + 1u; ++i) {
    d[i].id = "com.test.fill";
    d[i].version = "0";
    d[i].capy_ui_abi_required = 0x00020000u;
    d[i].allocator.alloc = 0;
    d[i].allocator.free = 0;
    d[i].allocator.user_data = 0;
    d[i].timeout_microseconds = 0u;
    d[i].init = 0;
    d[i].destroy = 0;
    d[i].on_event = 0;
    d[i].emit = 0;
  }
  for (i = 0u; i < CAPY_MAX_PLUGINS; ++i) {
    EXPECT(capy_plugin_register(&ctx, &d[i]) == (int)i);
  }
  /* One past capacity → -1, no mutation. */
  EXPECT(capy_plugin_register(&ctx, &d[CAPY_MAX_PLUGINS]) == -1);
  EXPECT(ctx.plugin_count == CAPY_MAX_PLUGINS);
  capy_plugin_unregister_all(&ctx);
}

static void test_plugin_unregister_all_calls_destroy(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  static const struct capy_plugin_descriptor with_destroy = {
      "com.test.dtor",
      "0.1",
      0x00020000u,
      {0, 0, 0},
      0u,
      0,
      plugin_test_destroy,
      0,
      0,
  };
  static const struct capy_plugin_descriptor without_destroy = {
      "com.test.no-dtor",
      "0.1",
      0x00020000u,
      {0, 0, 0},
      0u,
      0,
      0,
      0,
      0,
  };
  capy_widget_context_init(&ctx, &allocator);
  EXPECT(capy_plugin_register(&ctx, &with_destroy) == 0);
  EXPECT(capy_plugin_register(&ctx, &without_destroy) == 1);
  plugin_test_destroy_called = 0;
  plugin_test_destroy_ctx_seen = 0;
  capy_plugin_unregister_all(&ctx);
  /* destroy fired exactly once for the descriptor that had one. */
  EXPECT(plugin_test_destroy_called == 1);
  EXPECT(plugin_test_destroy_ctx_seen == (void *)&with_destroy);
  /* Registry is empty after unregister_all. */
  EXPECT(ctx.plugin_count == 0u);
  /* Idempotent: second call is a no-op. */
  capy_plugin_unregister_all(&ctx);
  EXPECT(plugin_test_destroy_called == 1);
  EXPECT(ctx.plugin_count == 0u);
}

static void test_plugin_null_guards(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  static const struct capy_plugin_descriptor d = {
      "com.test.nullguard", "0", 0x00020000u, {0, 0, 0}, 0u,
      0,                    0,   0,            0,
  };
  capy_widget_context_init(&ctx, &allocator);
  EXPECT(capy_plugin_register(0, &d) == -1);
  EXPECT(capy_plugin_register(&ctx, 0) == -1);
  /* NULL ctx is also safe in unregister_all. */
  capy_plugin_unregister_all(0);
  /* Reference plugin_test_init to avoid unused-static-function warnings
   * under -Werror. The descriptor here doesn't use it. */
  (void)plugin_test_init_called;
  (void)plugin_test_init;
}

static void test_plugin_op_in_dl_skipped_by_gpu_translator(void) {
  struct capy_dl_cmd cmds[2];
  struct capy_display_list dl;
  struct capy_gpu_quad quads[4];
  uint32_t count = 0u;
  uint32_t i;
  /* Synthesize: PLUGIN_OP + RECT. Translator must skip the plugin op
   * and emit exactly one quad for the RECT. */
  for (i = 0u; i < 2u; ++i) {
    cmds[i].rect.x = 0;
    cmds[i].rect.y = 0;
    cmds[i].rect.width = 0u;
    cmds[i].rect.height = 0u;
    cmds[i].color = 0u;
    cmds[i].text_offset = 0u;
    cmds[i].text_len = 0u;
    cmds[i].border_width = 0u;
    cmds[i].font_size = 0u;
    cmds[i].font_id = 0u;
    cmds[i].image_id = 0u;
    cmds[i].transform = capy_transform_identity();
  }
  cmds[0].op = CAPY_DL_PLUGIN_OP;
  cmds[0].image_id = 0xCAFEu; /* plugin id channel */
  cmds[1].op = CAPY_DL_RECT;
  cmds[1].rect.x = 1;
  cmds[1].rect.y = 2;
  cmds[1].rect.width = 3u;
  cmds[1].rect.height = 4u;
  cmds[1].color = 0xFF334455u;
  dl.cmds = cmds;
  dl.count = 2u;
  dl.capacity = 2u;
  dl.text_pool = 0;
  dl.text_used = 0u;
  dl.text_capacity = 0u;
  dl.version = CAPY_DISPLAY_LIST_SCHEMA_VERSION;
  dl.theme = 0;
  dl.dpi_scale_x256 = 256u;
  dl.reserved_dpi = 0u;
  EXPECT(capy_dl_to_quads(&dl, quads, 4u, &count) == 0);
  EXPECT(count == 1u);
  EXPECT(quads[0].x0 == 1);
  EXPECT(quads[0].y0 == 2);
  EXPECT(quads[0].color == 0xFF334455u);
}

static void test_plugin_dl_schema_bumped_to_7(void) {
  /* Macro and a runtime-initialised display list both report schema 7. */
  struct capy_display_list dl;
  struct capy_dl_cmd cmds[1];
  char text[1];
  EXPECT(CAPY_DISPLAY_LIST_SCHEMA_VERSION == 7u);
  capy_display_list_init(&dl, cmds, 1u, text, 1u);
  EXPECT(dl.version == 7u);
  /* PLUGIN_OP enum value is strictly greater than TRANSFORM_POP (1.9). */
  EXPECT((int)CAPY_DL_PLUGIN_OP > (int)CAPY_DL_TRANSFORM_POP);
}

/* ── v1.10: state serialization (since 1.10.0) ─────────────────────────── */

static struct capy_widget *state_build_tree(struct capy_widget_context *ctx) {
  struct capy_widget *root = capy_widget_create(ctx, CAPY_WIDGET_PANEL);
  struct capy_widget *btn;
  struct capy_widget *lbl;
  if (!root) return 0;
  capy_widget_set_bounds(root, 0, 0, 100u, 50u);
  btn = capy_widget_create(ctx, CAPY_WIDGET_BUTTON);
  capy_widget_set_bounds(btn, 5, 5, 40u, 20u);
  capy_widget_set_text(btn, "OK");
  btn->focused = 1u;
  btn->tab_index = 2;
  capy_widget_add_child(root, btn);
  lbl = capy_widget_create(ctx, CAPY_WIDGET_LABEL);
  capy_widget_set_bounds(lbl, 5, 30, 40u, 15u);
  capy_widget_set_text(lbl, "hi");
  capy_widget_add_child(root, lbl);
  return root;
}

static void test_state_serialize_header_and_magic(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *root;
  uint8_t buf[512];
  uint32_t out_len = 0u;
  capy_widget_context_init(&ctx, &allocator);
  root = state_build_tree(&ctx);
  EXPECT(root != 0);
  EXPECT(capy_widget_serialize(root, buf, sizeof(buf), &out_len) == 0);
  EXPECT(out_len >= CAPY_STATE_HEADER_SIZE);
  /* Magic. */
  EXPECT(buf[0] == 'C' && buf[1] == 'U' && buf[2] == 'I' && buf[3] == 'S');
  /* Format version little-endian. */
  EXPECT(buf[4] == 1u && buf[5] == 0u);
  EXPECT(buf[6] == 0u && buf[7] == 0u);
  /* Node count at offset 12 should be 3 (root + button + label). */
  EXPECT(buf[12] == 3u && buf[13] == 0u && buf[14] == 0u && buf[15] == 0u);
  capy_widget_destroy(root);
}

static void test_state_round_trip(void) {
  struct test_heap heap_a = {{0}, 0u, 0u};
  struct test_heap heap_b = {{0}, 0u, 0u};
  struct capy_widget_allocator alloc_a = {test_alloc, test_free, &heap_a};
  struct capy_widget_allocator alloc_b = {test_alloc, test_free, &heap_b};
  struct capy_widget_context ctx_a;
  struct capy_widget_context ctx_b;
  struct capy_widget *root_a;
  struct capy_widget *root_b = 0;
  uint8_t buf[512];
  uint32_t out_len = 0u;
  capy_widget_context_init(&ctx_a, &alloc_a);
  capy_widget_context_init(&ctx_b, &alloc_b);
  root_a = state_build_tree(&ctx_a);
  EXPECT(capy_widget_serialize(root_a, buf, sizeof(buf), &out_len) == 0);
  EXPECT(capy_widget_deserialize(buf, out_len, &ctx_b, &root_b) == 0);
  EXPECT(root_b != 0);
  /* Compare structural fields. */
  EXPECT(root_b->type == root_a->type);
  EXPECT(root_b->bounds.x == root_a->bounds.x);
  EXPECT(root_b->bounds.width == root_a->bounds.width);
  EXPECT(root_b->child_count == root_a->child_count);
  {
    struct capy_widget *btn_a = root_a->children[0];
    struct capy_widget *btn_b = root_b->children[0];
    EXPECT(btn_b->type == btn_a->type);
    EXPECT(btn_b->focused == btn_a->focused);
    EXPECT(btn_b->tab_index == btn_a->tab_index);
    EXPECT(btn_b->text[0] == 'O' && btn_b->text[1] == 'K' &&
           btn_b->text[2] == '\0');
  }
  capy_widget_destroy(root_a);
  capy_widget_destroy(root_b);
}

static void test_state_determinism(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *root;
  uint8_t buf1[512];
  uint8_t buf2[512];
  uint32_t len1 = 0u;
  uint32_t len2 = 0u;
  uint32_t i;
  capy_widget_context_init(&ctx, &allocator);
  root = state_build_tree(&ctx);
  EXPECT(capy_widget_serialize(root, buf1, sizeof(buf1), &len1) == 0);
  EXPECT(capy_widget_serialize(root, buf2, sizeof(buf2), &len2) == 0);
  EXPECT(len1 == len2);
  for (i = 0u; i < len1; ++i) {
    EXPECT(buf1[i] == buf2[i]);
  }
  capy_widget_destroy(root);
}

static void test_state_rejects_bad_magic_version_checksum(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *root;
  struct capy_widget *out_root = 0;
  uint8_t buf[512];
  uint32_t out_len = 0u;
  capy_widget_context_init(&ctx, &allocator);
  root = state_build_tree(&ctx);
  EXPECT(capy_widget_serialize(root, buf, sizeof(buf), &out_len) == 0);
  /* Corrupt magic. */
  buf[0] = 'X';
  EXPECT(capy_widget_deserialize(buf, out_len, &ctx, &out_root) == -1);
  EXPECT(out_root == 0);
  buf[0] = 'C';
  /* Corrupt version. */
  buf[4] = 99u;
  EXPECT(capy_widget_deserialize(buf, out_len, &ctx, &out_root) == -1);
  buf[4] = 1u;
  /* Corrupt body byte → checksum mismatch. */
  buf[CAPY_STATE_HEADER_SIZE + 5u] ^= 0xFFu;
  EXPECT(capy_widget_deserialize(buf, out_len, &ctx, &out_root) == -1);
  capy_widget_destroy(root);
}

static void test_state_capacity_overflow_no_corruption(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *root;
  uint8_t small[20];
  uint8_t tiny[8];
  uint32_t out_len = 0u;
  capy_widget_context_init(&ctx, &allocator);
  root = state_build_tree(&ctx);
  /* Too small for even the header. */
  EXPECT(capy_widget_serialize(root, tiny, sizeof(tiny), &out_len) == -1);
  EXPECT(out_len == 0u);
  /* Big enough for header but not the body. */
  EXPECT(capy_widget_serialize(root, small, sizeof(small), &out_len) == -1);
  EXPECT(out_len >= CAPY_STATE_HEADER_SIZE);
  /* NULL guards. */
  EXPECT(capy_widget_serialize(0, small, sizeof(small), &out_len) == -1);
  EXPECT(capy_widget_serialize(root, 0, sizeof(small), &out_len) == -1);
  EXPECT(capy_widget_serialize(root, small, sizeof(small), 0) == -1);
  capy_widget_destroy(root);
}

static void test_state_text_dump(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *root;
  char dump[512];
  int n;
  capy_widget_context_init(&ctx, &allocator);
  root = state_build_tree(&ctx);
  n = capy_widget_serialize_text(root, dump, sizeof(dump));
  EXPECT(n > 0);
  /* Trivial sanity checks on the text. */
  EXPECT(dump[0] == '[');
  /* Should contain at least the button text. */
  {
    int saw_ok = 0;
    int i;
    for (i = 0; i < n - 1; ++i) {
      if (dump[i] == 'O' && dump[i + 1] == 'K') {
        saw_ok = 1;
        break;
      }
    }
    EXPECT(saw_ok == 1);
  }
  /* Capacity overflow returns -1 without corruption. */
  EXPECT(capy_widget_serialize_text(root, dump, 8u) == -1);
  EXPECT(capy_widget_serialize_text(0, dump, sizeof(dump)) == -1);
  EXPECT(capy_widget_serialize_text(root, 0, sizeof(dump)) == -1);
  capy_widget_destroy(root);
}

static void test_state_unknown_type_rejected(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *root;
  struct capy_widget *out_root = 0;
  uint8_t buf[512];
  uint32_t out_len = 0u;
  uint32_t cs;
  uint32_t i;
  uint32_t computed;
  capy_widget_context_init(&ctx, &allocator);
  root = state_build_tree(&ctx);
  EXPECT(capy_widget_serialize(root, buf, sizeof(buf), &out_len) == 0);
  /* Stomp on the root node's type byte (offset 16) with an out-of-range
   * value. Recompute the checksum so the corruption is detected as
   * "unknown type" rather than "checksum mismatch". */
  buf[CAPY_STATE_HEADER_SIZE] = 0xFFu;
  /* Recompute FNV-1a of body. */
  cs = 0x811C9DC5u;
  for (i = CAPY_STATE_HEADER_SIZE; i < out_len; ++i) {
    cs ^= (uint32_t)buf[i];
    cs *= 0x01000193u;
  }
  /* Store new checksum at offset 8 (little-endian). */
  buf[8] = (uint8_t)(cs & 0xFFu);
  buf[9] = (uint8_t)((cs >> 8) & 0xFFu);
  buf[10] = (uint8_t)((cs >> 16) & 0xFFu);
  buf[11] = (uint8_t)((cs >> 24) & 0xFFu);
  computed = cs;
  (void)computed;
  EXPECT(capy_widget_deserialize(buf, out_len, &ctx, &out_root) == -1);
  EXPECT(out_root == 0);
  capy_widget_destroy(root);
}

/* ── v1.9: 2D transforms (since 1.9.0) ─────────────────────────────────── */

static void test_transform_identity_is_diagonal(void) {
  struct capy_dl_transform t = capy_transform_identity();
  EXPECT(t.m00 == 0x10000);
  EXPECT(t.m11 == 0x10000);
  EXPECT(t.m01 == 0);
  EXPECT(t.m10 == 0);
  EXPECT(t.m02 == 0);
  EXPECT(t.m12 == 0);
}

static void test_transform_scale_and_translate(void) {
  /* 2× scale: diagonal = 0x20000. */
  struct capy_dl_transform s = capy_transform_scale(0x20000, 0x20000);
  EXPECT(s.m00 == 0x20000);
  EXPECT(s.m11 == 0x20000);
  EXPECT(s.m01 == 0 && s.m10 == 0);
  EXPECT(s.m02 == 0 && s.m12 == 0);
  /* Translate by (5, 7): m02/m12 set, rest identity. */
  {
    struct capy_dl_transform t = capy_transform_translate(5, 7);
    EXPECT(t.m00 == 0x10000);
    EXPECT(t.m11 == 0x10000);
    EXPECT(t.m02 == 5);
    EXPECT(t.m12 == 7);
  }
}

static void test_transform_rotate_quadrants_exact(void) {
  /* 0° = identity. */
  struct capy_dl_transform r0 = capy_transform_rotate_quadrants(0u);
  EXPECT(r0.m00 == 0x10000 && r0.m11 == 0x10000);
  EXPECT(r0.m01 == 0 && r0.m10 == 0);
  /* 90° CCW: (x,y) → (-y, x). */
  {
    struct capy_dl_transform r1 = capy_transform_rotate_quadrants(1u);
    EXPECT(r1.m00 == 0);
    EXPECT(r1.m01 == -0x10000);
    EXPECT(r1.m10 == 0x10000);
    EXPECT(r1.m11 == 0);
  }
  /* 180°. */
  {
    struct capy_dl_transform r2 = capy_transform_rotate_quadrants(2u);
    EXPECT(r2.m00 == -0x10000);
    EXPECT(r2.m11 == -0x10000);
  }
  /* 270°. */
  {
    struct capy_dl_transform r3 = capy_transform_rotate_quadrants(3u);
    EXPECT(r3.m00 == 0);
    EXPECT(r3.m01 == 0x10000);
    EXPECT(r3.m10 == -0x10000);
    EXPECT(r3.m11 == 0);
  }
  /* n is reduced modulo 4. */
  {
    struct capy_dl_transform r5 = capy_transform_rotate_quadrants(5u);
    struct capy_dl_transform r1 = capy_transform_rotate_quadrants(1u);
    EXPECT(r5.m00 == r1.m00 && r5.m01 == r1.m01);
    EXPECT(r5.m10 == r1.m10 && r5.m11 == r1.m11);
  }
}

static void test_transform_multiply_identity_neutral(void) {
  struct capy_dl_transform id = capy_transform_identity();
  struct capy_dl_transform s = capy_transform_scale(0x20000, 0x18000);
  struct capy_dl_transform left = capy_transform_multiply(&id, &s);
  struct capy_dl_transform right = capy_transform_multiply(&s, &id);
  EXPECT(left.m00 == s.m00 && left.m01 == s.m01 && left.m02 == s.m02);
  EXPECT(left.m10 == s.m10 && left.m11 == s.m11 && left.m12 == s.m12);
  EXPECT(right.m00 == s.m00 && right.m01 == s.m01 && right.m02 == s.m02);
  EXPECT(right.m10 == s.m10 && right.m11 == s.m11 && right.m12 == s.m12);
  /* NULL args degrade to identity (no crash). */
  {
    struct capy_dl_transform z = capy_transform_multiply(0, &s);
    struct capy_dl_transform i = capy_transform_identity();
    EXPECT(z.m00 == i.m00 && z.m11 == i.m11);
  }
}

static void test_transform_multiply_composes(void) {
  /* scale 2× then translate (5, 7): expected linear part = 2×, translation
   * remains (5, 7) because translate is applied after scale (right-multiply
   * order: a*b means b applied first, then a). Concretely:
   *   scale * translate: m00=2, m02=2*5=10; m11=2, m12=2*7=14.
   * Verify both orders to confirm non-commutativity. */
  struct capy_dl_transform sc = capy_transform_scale(0x20000, 0x20000);
  struct capy_dl_transform tr = capy_transform_translate(5, 7);
  struct capy_dl_transform sc_then_tr = capy_transform_multiply(&sc, &tr);
  struct capy_dl_transform tr_then_sc = capy_transform_multiply(&tr, &sc);
  EXPECT(sc_then_tr.m00 == 0x20000);
  EXPECT(sc_then_tr.m11 == 0x20000);
  EXPECT(sc_then_tr.m02 == 10);
  EXPECT(sc_then_tr.m12 == 14);
  /* Reverse order: translate then scale. Linear part still 2×; translation
   * unscaled (5, 7) because in this convention scale only affects its own
   * column, not pre-existing m02 in `tr`. */
  EXPECT(tr_then_sc.m00 == 0x20000);
  EXPECT(tr_then_sc.m11 == 0x20000);
  EXPECT(tr_then_sc.m02 == 5);
  EXPECT(tr_then_sc.m12 == 7);
}

static void test_transform_widget_emits_push_pop(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *panel;
  struct capy_dl_cmd cmd_buf[16];
  char text_buf[32];
  struct capy_display_list dl;
  struct capy_dl_transform rot = capy_transform_rotate_quadrants(1u);
  capy_widget_context_init(&ctx, &allocator);
  panel = capy_widget_create(&ctx, CAPY_WIDGET_PANEL);
  capy_widget_set_bounds(panel, 0, 0, 50u, 50u);
  EXPECT(panel->transform == 0);
  capy_widget_set_transform(panel, &rot);
  EXPECT(panel->transform == &rot);
  capy_display_list_init(&dl, cmd_buf, 16u, text_buf, 32u);
  EXPECT(capy_widget_emit(panel, &dl) == 0);
  /* First op is TRANSFORM_PUSH carrying the rotation matrix. */
  EXPECT(dl.count >= 1u);
  EXPECT(dl.cmds[0].op == CAPY_DL_TRANSFORM_PUSH);
  EXPECT(dl.cmds[0].transform.m00 == rot.m00);
  EXPECT(dl.cmds[0].transform.m01 == rot.m01);
  EXPECT(dl.cmds[0].transform.m10 == rot.m10);
  EXPECT(dl.cmds[0].transform.m11 == rot.m11);
  /* Second op is the usual CLIP_PUSH. */
  EXPECT(dl.cmds[1].op == CAPY_DL_CLIP_PUSH);
  /* Last op is TRANSFORM_POP. */
  EXPECT(dl.cmds[dl.count - 1u].op == CAPY_DL_TRANSFORM_POP);
  /* Penultimate is CLIP_POP. */
  EXPECT(dl.cmds[dl.count - 2u].op == CAPY_DL_CLIP_POP);
  /* Clear transform: subsequent emit goes back to byte-equivalent pre-1.9. */
  capy_widget_set_transform(panel, 0);
  capy_display_list_reset(&dl);
  EXPECT(capy_widget_emit(panel, &dl) == 0);
  {
    uint32_t i;
    for (i = 0u; i < dl.count; ++i) {
      EXPECT(dl.cmds[i].op != CAPY_DL_TRANSFORM_PUSH);
      EXPECT(dl.cmds[i].op != CAPY_DL_TRANSFORM_POP);
    }
  }
  capy_widget_destroy(panel);
}

static void test_transform_gpu_translator_skips_transform_ops(void) {
  struct capy_dl_cmd cmds[3];
  struct capy_display_list dl;
  struct capy_gpu_quad quads[4];
  uint32_t count = 0u;
  uint32_t i;
  /* Synthesize: TRANSFORM_PUSH, RECT, TRANSFORM_POP. Translator must skip
   * the two transform ops and emit exactly one quad for the RECT. */
  for (i = 0u; i < 3u; ++i) {
    cmds[i].rect.x = 0;
    cmds[i].rect.y = 0;
    cmds[i].rect.width = 0u;
    cmds[i].rect.height = 0u;
    cmds[i].color = 0u;
    cmds[i].text_offset = 0u;
    cmds[i].text_len = 0u;
    cmds[i].border_width = 0u;
    cmds[i].font_size = 0u;
    cmds[i].font_id = 0u;
    cmds[i].image_id = 0u;
    cmds[i].transform = capy_transform_identity();
  }
  cmds[0].op = CAPY_DL_TRANSFORM_PUSH;
  cmds[0].transform = capy_transform_rotate_quadrants(2u);
  cmds[1].op = CAPY_DL_RECT;
  cmds[1].rect.x = 4;
  cmds[1].rect.y = 5;
  cmds[1].rect.width = 6u;
  cmds[1].rect.height = 7u;
  cmds[1].color = 0xFF112233u;
  cmds[2].op = CAPY_DL_TRANSFORM_POP;
  dl.cmds = cmds;
  dl.count = 3u;
  dl.capacity = 3u;
  dl.text_pool = 0;
  dl.text_used = 0u;
  dl.text_capacity = 0u;
  dl.version = CAPY_DISPLAY_LIST_SCHEMA_VERSION;
  dl.theme = 0;
  dl.dpi_scale_x256 = 256u;
  dl.reserved_dpi = 0u;
  EXPECT(capy_dl_to_quads(&dl, quads, 4u, &count) == 0);
  EXPECT(count == 1u);
  EXPECT(quads[0].x0 == 4);
  EXPECT(quads[0].y0 == 5);
  EXPECT(quads[0].x1 == 10);
  EXPECT(quads[0].y1 == 12);
  EXPECT(quads[0].color == 0xFF112233u);
}

/* ── v1.8: rich IME (since 1.8.0) ──────────────────────────────────────── */

static void test_ime_set_preedit_marks_composing(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *tb;
  capy_widget_context_init(&ctx, &allocator);
  tb = capy_widget_create(&ctx, CAPY_WIDGET_TEXTBOX);
  EXPECT(tb != 0);
  EXPECT(tb->ime_composition_phase == 0u);
  capy_ime_set_preedit(tb, "\xE4\xBD\xA0", 3u, 3u); /* 你 */
  EXPECT(tb->ime_preedit != 0);
  EXPECT(tb->ime_preedit_len == 3u);
  EXPECT(tb->ime_cursor_pos == 3u);
  EXPECT(tb->ime_composition_phase == 1u);
  /* Setting NULL clears preedit fields but keeps phase=composing. */
  capy_ime_set_preedit(tb, 0, 99u, 99u);
  EXPECT(tb->ime_preedit == 0);
  EXPECT(tb->ime_preedit_len == 0u);
  EXPECT(tb->ime_cursor_pos == 0u);
  capy_widget_destroy(tb);
}

static void test_ime_set_candidates_marks_selecting(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *tb;
  static const char *const cands[3] = {"\xE4\xBD\xA0", "\xE5\x80\xAB",
                                       "\xE5\xB0\xBC"};
  capy_widget_context_init(&ctx, &allocator);
  tb = capy_widget_create(&ctx, CAPY_WIDGET_TEXTBOX);
  capy_ime_set_preedit(tb, "ni", 2u, 2u);
  capy_ime_set_candidates(tb, cands, 3u, 1u);
  EXPECT(tb->ime_candidates == cands);
  EXPECT(tb->ime_candidate_count == 3u);
  EXPECT(tb->ime_selected_index == 1u);
  EXPECT(tb->ime_composition_phase == 2u);
  /* Empty list clears candidates but keeps phase. */
  capy_ime_set_candidates(tb, cands, 0u, 0u);
  EXPECT(tb->ime_candidate_count == 0u);
  EXPECT(tb->ime_composition_phase == 2u);
  /* NULL list also clears. */
  capy_ime_set_candidates(tb, 0, 5u, 2u);
  EXPECT(tb->ime_candidates == 0);
  EXPECT(tb->ime_candidate_count == 0u);
  capy_widget_destroy(tb);
}

static void test_ime_commit_inserts_and_clears(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *tb;
  static const char *const cands[1] = {"hi"};
  capy_widget_context_init(&ctx, &allocator);
  tb = capy_widget_create(&ctx, CAPY_WIDGET_TEXTBOX);
  capy_ime_set_preedit(tb, "h", 1u, 1u);
  capy_ime_set_candidates(tb, cands, 1u, 0u);
  EXPECT(capy_ime_commit(tb, "hi", 2u) == 0);
  /* Textbox content now contains "hi". */
  EXPECT(tb->text[0] == 'h' && tb->text[1] == 'i' && tb->text[2] == '\0');
  /* IME state fully cleared. */
  EXPECT(tb->ime_preedit == 0);
  EXPECT(tb->ime_preedit_len == 0u);
  EXPECT(tb->ime_candidates == 0);
  EXPECT(tb->ime_candidate_count == 0u);
  EXPECT(tb->ime_composition_phase == 0u);
  capy_widget_destroy(tb);
}

static void test_ime_cancel_preserves_textbox_content(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *tb;
  capy_widget_context_init(&ctx, &allocator);
  tb = capy_widget_create(&ctx, CAPY_WIDGET_TEXTBOX);
  capy_widget_set_text(tb, "abc");
  capy_ime_set_preedit(tb, "X", 1u, 1u);
  EXPECT(tb->ime_composition_phase == 1u);
  capy_ime_cancel(tb);
  /* Original text preserved; IME cleared. */
  EXPECT(tb->text[0] == 'a' && tb->text[1] == 'b' && tb->text[2] == 'c' &&
         tb->text[3] == '\0');
  EXPECT(tb->ime_preedit == 0);
  EXPECT(tb->ime_composition_phase == 0u);
  capy_widget_destroy(tb);
}

static void test_ime_apis_are_noop_on_non_textbox(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *button;
  static const char *const cands[1] = {"x"};
  capy_widget_context_init(&ctx, &allocator);
  button = capy_widget_create(&ctx, CAPY_WIDGET_BUTTON);
  capy_ime_set_preedit(button, "hi", 2u, 2u);
  EXPECT(button->ime_preedit == 0);
  EXPECT(button->ime_composition_phase == 0u);
  capy_ime_set_candidates(button, cands, 1u, 0u);
  EXPECT(button->ime_candidates == 0);
  EXPECT(capy_ime_commit(button, "x", 1u) == -1);
  capy_ime_cancel(button);
  EXPECT(button->ime_composition_phase == 0u);
  capy_widget_destroy(button);
}

static void test_ime_event_types_routed_without_crash(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *tb;
  struct capy_widget_event ev;
  capy_widget_context_init(&ctx, &allocator);
  tb = capy_widget_create(&ctx, CAPY_WIDGET_TEXTBOX);
  capy_widget_set_bounds(tb, 0, 0, 100u, 20u);
  ev.x = 10;
  ev.y = 10;
  ev.buttons = 0u;
  ev.keycode = 0u;
  ev.modifiers = 0u;
  ev.payload = 0;
  ev.type = CAPY_WIDGET_EVENT_IME_COMPOSE_START;
  EXPECT(capy_widget_handle_event(tb, &ev) == 0);
  ev.type = CAPY_WIDGET_EVENT_IME_PREEDIT_UPDATE;
  EXPECT(capy_widget_handle_event(tb, &ev) == 0);
  ev.type = CAPY_WIDGET_EVENT_IME_COMMIT;
  EXPECT(capy_widget_handle_event(tb, &ev) == 0);
  ev.type = CAPY_WIDGET_EVENT_IME_CANCEL;
  EXPECT(capy_widget_handle_event(tb, &ev) == 0);
  /* Core does not mutate IME state on the events themselves. */
  EXPECT(tb->ime_composition_phase == 0u);
  capy_widget_destroy(tb);
}

static void test_ime_get_state_snapshot(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *tb;
  struct capy_widget *btn;
  struct capy_ime_state snap;
  static const char *const cands[2] = {"a", "b"};
  capy_widget_context_init(&ctx, &allocator);
  tb = capy_widget_create(&ctx, CAPY_WIDGET_TEXTBOX);
  btn = capy_widget_create(&ctx, CAPY_WIDGET_BUTTON);
  capy_ime_set_preedit(tb, "ni", 2u, 2u);
  capy_ime_set_candidates(tb, cands, 2u, 1u);
  capy_ime_get_state(tb, &snap);
  EXPECT(snap.preedit != 0);
  EXPECT(snap.preedit_len == 2u);
  EXPECT(snap.candidate_count == 2u);
  EXPECT(snap.selected_index == 1u);
  EXPECT(snap.composition_phase == 2u);
  /* Non-textbox: zero-fill. */
  capy_ime_get_state(btn, &snap);
  EXPECT(snap.preedit == 0);
  EXPECT(snap.candidate_count == 0u);
  EXPECT(snap.composition_phase == 0u);
  /* NULL guards. */
  capy_ime_get_state(0, &snap);
  EXPECT(snap.composition_phase == 0u);
  capy_ime_get_state(tb, 0); /* no-op, no crash */
  capy_widget_destroy(tb);
  capy_widget_destroy(btn);
}

/* ── v1.7: slab allocator (since 1.7.0) ────────────────────────────────── */

/* Element type for slab tests: 16 bytes so it always exceeds sizeof(void*)
 * and gives the free-list room. Using a `void *` first member would alias
 * the free-list pointer, so we use an opaque scratch struct. */
struct slab_test_elem {
  uint32_t payload[4];
};

static void test_slab_init_partitions_buffer(void) {
  struct capy_slab slab;
  struct slab_test_elem buf[8];
  capy_slab_init(&slab, buf, sizeof(buf), sizeof(struct slab_test_elem));
  EXPECT(slab.buffer == buf);
  EXPECT(slab.size == sizeof(buf));
  EXPECT(slab.element_size == sizeof(struct slab_test_elem));
  EXPECT(slab.high_water == 0u);
  EXPECT(slab.free_list == 0);
}

static void test_slab_alloc_returns_distinct_pointers_until_full(void) {
  struct capy_slab slab;
  struct slab_test_elem buf[4];
  void *p[4];
  uint32_t i;
  uint32_t j;
  capy_slab_init(&slab, buf, sizeof(buf), sizeof(struct slab_test_elem));
  for (i = 0u; i < 4u; ++i) {
    p[i] = capy_slab_alloc(&slab);
    EXPECT(p[i] != 0);
    for (j = 0u; j < i; ++j) {
      EXPECT(p[i] != p[j]);
    }
  }
  /* Exhausted: 5th alloc returns NULL. */
  EXPECT(capy_slab_alloc(&slab) == 0);
  EXPECT(slab.high_water == 4u);
}

static void test_slab_free_returns_to_lifo_pool(void) {
  struct capy_slab slab;
  struct slab_test_elem buf[4];
  void *a;
  void *b;
  void *c;
  void *reuse_b;
  void *reuse_a;
  capy_slab_init(&slab, buf, sizeof(buf), sizeof(struct slab_test_elem));
  a = capy_slab_alloc(&slab);
  b = capy_slab_alloc(&slab);
  c = capy_slab_alloc(&slab);
  EXPECT(a && b && c);
  /* Free in order a, b. LIFO → next alloc returns b, then a. */
  capy_slab_free(&slab, a);
  capy_slab_free(&slab, b);
  reuse_b = capy_slab_alloc(&slab);
  reuse_a = capy_slab_alloc(&slab);
  EXPECT(reuse_b == b);
  EXPECT(reuse_a == a);
  /* Slab is full again (c still held). 5th alloc → NULL (exhausted) or
   * one more slot via bump (we allocated 3 of 4 so one bump slot remains). */
  {
    void *d = capy_slab_alloc(&slab);
    EXPECT(d != 0);
    EXPECT(d != a && d != b && d != c);
    EXPECT(capy_slab_alloc(&slab) == 0);
  }
}

static void test_slab_degenerate_init_returns_null_on_alloc(void) {
  struct capy_slab slab;
  /* element_size < sizeof(void*). */
  capy_slab_init(&slab, (void *)0x1000, 64u, 1u);
  EXPECT(slab.size == 0u);
  EXPECT(capy_slab_alloc(&slab) == 0);
  /* NULL buffer. */
  capy_slab_init(&slab, 0, 64u, 16u);
  EXPECT(slab.size == 0u);
  EXPECT(capy_slab_alloc(&slab) == 0);
  /* Zero size. */
  {
    struct slab_test_elem buf[2];
    capy_slab_init(&slab, buf, 0u, sizeof(struct slab_test_elem));
    EXPECT(slab.size == 0u);
    EXPECT(capy_slab_alloc(&slab) == 0);
  }
}

static void test_slab_null_guards(void) {
  struct capy_slab slab;
  struct slab_test_elem buf[2];
  /* `_init(NULL, ...)` is a no-op (struct is the caller's). */
  capy_slab_init(0, buf, sizeof(buf), sizeof(struct slab_test_elem));
  /* `_alloc(NULL)` returns NULL. */
  EXPECT(capy_slab_alloc(0) == 0);
  /* `_free` on NULL slab or NULL ptr is a no-op. */
  capy_slab_init(&slab, buf, sizeof(buf), sizeof(struct slab_test_elem));
  capy_slab_free(0, buf);
  capy_slab_free(&slab, 0);
  /* After the no-op frees, free_list must still be NULL. */
  EXPECT(slab.free_list == 0);
}

static void test_slab_alloc_pointer_alignment(void) {
  /* Buffer is naturally aligned by being a struct array; verify the slot
   * pointers respect the alignment of `void *` so the inline free-list
   * read in `_alloc` is well-defined. */
  struct capy_slab slab;
  struct slab_test_elem buf[4];
  void *a;
  void *b;
  capy_slab_init(&slab, buf, sizeof(buf), sizeof(struct slab_test_elem));
  a = capy_slab_alloc(&slab);
  b = capy_slab_alloc(&slab);
  EXPECT(((uintptr_t)a % sizeof(void *)) == 0u);
  EXPECT(((uintptr_t)b % sizeof(void *)) == 0u);
  /* Consecutive slots are element_size apart. */
  EXPECT((uintptr_t)b - (uintptr_t)a == sizeof(struct slab_test_elem));
}

static void test_slab_determinism_same_addresses(void) {
  struct capy_slab slab_a;
  struct capy_slab slab_b;
  struct slab_test_elem buf_a[4];
  struct slab_test_elem buf_b[4];
  void *seq_a[6];
  void *seq_b[6];
  uint32_t i;
  /* Run the same alloc/free pattern against two independently-init'd
   * slabs over identically-laid-out buffers. The slot indices (relative
   * to each buffer base) must match step-for-step, proving determinism. */
  capy_slab_init(&slab_a, buf_a, sizeof(buf_a),
                 sizeof(struct slab_test_elem));
  capy_slab_init(&slab_b, buf_b, sizeof(buf_b),
                 sizeof(struct slab_test_elem));
  seq_a[0] = capy_slab_alloc(&slab_a);
  seq_b[0] = capy_slab_alloc(&slab_b);
  seq_a[1] = capy_slab_alloc(&slab_a);
  seq_b[1] = capy_slab_alloc(&slab_b);
  capy_slab_free(&slab_a, seq_a[0]);
  capy_slab_free(&slab_b, seq_b[0]);
  seq_a[2] = capy_slab_alloc(&slab_a);
  seq_b[2] = capy_slab_alloc(&slab_b);
  seq_a[3] = capy_slab_alloc(&slab_a);
  seq_b[3] = capy_slab_alloc(&slab_b);
  seq_a[4] = capy_slab_alloc(&slab_a);
  seq_b[4] = capy_slab_alloc(&slab_b);
  seq_a[5] = capy_slab_alloc(&slab_a);
  seq_b[5] = capy_slab_alloc(&slab_b);
  for (i = 0u; i < 6u; ++i) {
    if (!seq_a[i] || !seq_b[i]) {
      EXPECT(seq_a[i] == seq_b[i]);
      continue;
    }
    uintptr_t idx_a =
        ((uintptr_t)seq_a[i] - (uintptr_t)buf_a) / sizeof(struct slab_test_elem);
    uintptr_t idx_b =
        ((uintptr_t)seq_b[i] - (uintptr_t)buf_b) / sizeof(struct slab_test_elem);
    EXPECT(idx_a == idx_b);
  }
}

/* ── v1.6: drag and drop (since 1.6.0) ─────────────────────────────────── */

static int dnd_test_callback_calls;
static struct capy_widget *dnd_test_callback_target;
static const struct capy_dnd_payload *dnd_test_callback_payload;
static void *dnd_test_callback_user_data;

static int dnd_test_accept(struct capy_widget *target,
                           const struct capy_dnd_payload *payload,
                           void *user_data) {
  ++dnd_test_callback_calls;
  dnd_test_callback_target = target;
  dnd_test_callback_payload = payload;
  dnd_test_callback_user_data = user_data;
  return 1;
}

static void test_dnd_set_draggable_attaches_payload(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *w;
  struct capy_dnd_payload p = {"text/plain", "abc", 3u};
  capy_widget_context_init(&ctx, &allocator);
  w = capy_widget_create(&ctx, CAPY_WIDGET_LABEL);
  EXPECT(w != 0);
  EXPECT(w->drag_payload == 0);
  capy_widget_set_draggable(w, &p);
  EXPECT(w->drag_payload == &p);
  capy_widget_set_draggable(w, 0);
  EXPECT(w->drag_payload == 0);
  /* NULL guard. */
  capy_widget_set_draggable(0, &p);
  capy_widget_destroy(w);
}

static void test_dnd_set_drop_target_attaches_filter(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *w;
  static const char *const types[2] = {"text/*", "image/png"};
  capy_widget_context_init(&ctx, &allocator);
  w = capy_widget_create(&ctx, CAPY_WIDGET_PANEL);
  EXPECT(w != 0);
  capy_widget_set_drop_target(w, types, 2u);
  EXPECT(w->drop_accepted_types == types);
  EXPECT(w->drop_types_count == 2u);
  /* NULL array clears. */
  capy_widget_set_drop_target(w, 0, 2u);
  EXPECT(w->drop_types_count == 0u);
  /* NULL widget no-op. */
  capy_widget_set_drop_target(0, types, 2u);
  capy_widget_destroy(w);
}

static void test_dnd_type_match_exact_wildcard_prefix(void) {
  /* Exact match, case-insensitive. */
  EXPECT(capy_dnd_type_matches("text/plain", "text/plain") == 1);
  EXPECT(capy_dnd_type_matches("Text/Plain", "TEXT/plain") == 1);
  EXPECT(capy_dnd_type_matches("text/plain", "text/html") == 0);
  /* Prefix wildcard. */
  EXPECT(capy_dnd_type_matches("text/*", "text/plain") == 1);
  EXPECT(capy_dnd_type_matches("text/*", "text/html") == 1);
  EXPECT(capy_dnd_type_matches("text/*", "image/png") == 0);
  /* Pure wildcard. */
  EXPECT(capy_dnd_type_matches("*", "anything/here") == 1);
  EXPECT(capy_dnd_type_matches("*", "") == 1);
  /* Length mismatch. */
  EXPECT(capy_dnd_type_matches("text/plain", "text/") == 0);
  EXPECT(capy_dnd_type_matches("text/", "text/plain") == 0);
  /* NULL guards. */
  EXPECT(capy_dnd_type_matches(0, "text/plain") == 0);
  EXPECT(capy_dnd_type_matches("text/plain", 0) == 0);
}

static void test_dnd_drop_invokes_callback_when_compatible(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *target;
  struct capy_dnd_payload p = {"text/plain", "abc", 3u};
  static const char *const types[1] = {"text/*"};
  struct capy_widget_event ev;
  int marker = 7;
  capy_widget_context_init(&ctx, &allocator);
  target = capy_widget_create(&ctx, CAPY_WIDGET_PANEL);
  EXPECT(target != 0);
  capy_widget_set_bounds(target, 0, 0, 100u, 50u);
  capy_widget_set_drop_target(target, types, 1u);
  capy_widget_set_on_drop(target, dnd_test_accept, &marker);
  dnd_test_callback_calls = 0;
  dnd_test_callback_target = 0;
  dnd_test_callback_payload = 0;
  dnd_test_callback_user_data = 0;
  ev.type = CAPY_WIDGET_EVENT_DROP;
  ev.x = 25;
  ev.y = 25;
  ev.buttons = 0u;
  ev.keycode = 0u;
  ev.modifiers = 0u;
  ev.payload = &p;
  EXPECT(capy_widget_handle_event(target, &ev) == 1);
  EXPECT(dnd_test_callback_calls == 1);
  EXPECT(dnd_test_callback_target == target);
  EXPECT(dnd_test_callback_payload == &p);
  EXPECT(dnd_test_callback_user_data == &marker);
  capy_widget_destroy(target);
}

static void test_dnd_drop_silently_ignored_when_incompatible(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *target;
  struct capy_dnd_payload p = {"image/png", 0, 0u};
  static const char *const text_types[1] = {"text/*"};
  struct capy_widget_event ev;
  capy_widget_context_init(&ctx, &allocator);
  target = capy_widget_create(&ctx, CAPY_WIDGET_PANEL);
  capy_widget_set_bounds(target, 0, 0, 100u, 50u);
  capy_widget_set_drop_target(target, text_types, 1u);
  capy_widget_set_on_drop(target, dnd_test_accept, 0);
  dnd_test_callback_calls = 0;
  ev.type = CAPY_WIDGET_EVENT_DROP;
  ev.x = 25;
  ev.y = 25;
  ev.buttons = 0u;
  ev.keycode = 0u;
  ev.modifiers = 0u;
  ev.payload = &p;
  EXPECT(capy_widget_handle_event(target, &ev) == 0);
  EXPECT(dnd_test_callback_calls == 0);
  /* Drop outside bounds is also ignored. */
  capy_widget_set_drop_target(target, (const char *const[]){"*"}, 1u);
  ev.x = 500;
  ev.y = 500;
  EXPECT(capy_widget_handle_event(target, &ev) == 0);
  EXPECT(dnd_test_callback_calls == 0);
  capy_widget_destroy(target);
}

static void test_dnd_event_types_routed_without_crash(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *w;
  struct capy_widget_event ev;
  capy_widget_context_init(&ctx, &allocator);
  w = capy_widget_create(&ctx, CAPY_WIDGET_PANEL);
  capy_widget_set_bounds(w, 0, 0, 100u, 100u);
  /* DRAG_BEGIN/MOVE/END are silently accepted no-ops in 1.6. */
  ev.type = CAPY_WIDGET_EVENT_DRAG_BEGIN;
  ev.x = 10;
  ev.y = 10;
  ev.buttons = 0u;
  ev.keycode = 0u;
  ev.modifiers = 0u;
  ev.payload = 0;
  EXPECT(capy_widget_handle_event(w, &ev) == 0);
  ev.type = CAPY_WIDGET_EVENT_DRAG_MOVE;
  EXPECT(capy_widget_handle_event(w, &ev) == 0);
  ev.type = CAPY_WIDGET_EVENT_DRAG_END;
  EXPECT(capy_widget_handle_event(w, &ev) == 0);
  /* DROP with NULL payload also safe (no callback fires). */
  ev.type = CAPY_WIDGET_EVENT_DROP;
  EXPECT(capy_widget_handle_event(w, &ev) == 0);
  capy_widget_destroy(w);
}

static void test_dnd_dnd_accepts_helper(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *w;
  struct capy_dnd_payload text = {"text/plain", 0, 0u};
  struct capy_dnd_payload png = {"image/png", 0, 0u};
  static const char *const types[2] = {"image/png", "application/json"};
  capy_widget_context_init(&ctx, &allocator);
  w = capy_widget_create(&ctx, CAPY_WIDGET_PANEL);
  /* Without filter configured: nothing accepted. */
  EXPECT(capy_widget_dnd_accepts(w, &text) == 0);
  EXPECT(capy_widget_dnd_accepts(w, &png) == 0);
  capy_widget_set_drop_target(w, types, 2u);
  EXPECT(capy_widget_dnd_accepts(w, &text) == 0);
  EXPECT(capy_widget_dnd_accepts(w, &png) == 1);
  /* NULL guards. */
  EXPECT(capy_widget_dnd_accepts(0, &png) == 0);
  EXPECT(capy_widget_dnd_accepts(w, 0) == 0);
  capy_widget_destroy(w);
}

static void test_dnd_drop_deepest_first(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *outer;
  struct capy_widget *inner;
  struct capy_dnd_payload p = {"text/plain", 0, 0u};
  static const char *const all_types[1] = {"*"};
  struct capy_widget_event ev;
  capy_widget_context_init(&ctx, &allocator);
  outer = capy_widget_create(&ctx, CAPY_WIDGET_PANEL);
  inner = capy_widget_create(&ctx, CAPY_WIDGET_PANEL);
  capy_widget_set_bounds(outer, 0, 0, 200u, 200u);
  capy_widget_set_bounds(inner, 50, 50, 50u, 50u);
  capy_widget_add_child(outer, inner);
  capy_widget_set_drop_target(outer, all_types, 1u);
  capy_widget_set_drop_target(inner, all_types, 1u);
  capy_widget_set_on_drop(outer, dnd_test_accept, (void *)0x1);
  capy_widget_set_on_drop(inner, dnd_test_accept, (void *)0x2);
  dnd_test_callback_calls = 0;
  dnd_test_callback_user_data = 0;
  ev.type = CAPY_WIDGET_EVENT_DROP;
  ev.x = 60;
  ev.y = 60;
  ev.buttons = 0u;
  ev.keycode = 0u;
  ev.modifiers = 0u;
  ev.payload = &p;
  EXPECT(capy_widget_handle_event(outer, &ev) == 1);
  /* Deepest (inner) wins. */
  EXPECT(dnd_test_callback_calls == 1);
  EXPECT(dnd_test_callback_user_data == (void *)0x2);
  capy_widget_destroy(outer);
}

/* ── v1.5: multi-display & DPI scaling (since 1.5.0) ───────────────────── */

static void test_dpi_default_is_256(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  capy_widget_context_init(&ctx, &allocator);
  EXPECT(capy_widget_get_dpi_scale(&ctx) == 256u);
  EXPECT(ctx.dpi_scale_x256 == 256u);
}

static void test_dpi_set_get_roundtrip(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  capy_widget_context_init(&ctx, &allocator);
  capy_widget_set_dpi_scale(&ctx, 384u);
  EXPECT(capy_widget_get_dpi_scale(&ctx) == 384u);
  capy_widget_set_dpi_scale(&ctx, 512u);
  EXPECT(capy_widget_get_dpi_scale(&ctx) == 512u);
  /* Zero clamps to 1 (degenerate but non-zero). */
  capy_widget_set_dpi_scale(&ctx, 0u);
  EXPECT(capy_widget_get_dpi_scale(&ctx) == 1u);
  /* NULL guards. */
  capy_widget_set_dpi_scale(0, 384u);
  EXPECT(capy_widget_get_dpi_scale(0) == 256u);
}

static void test_dpi_scale_dim_helper(void) {
  /* 1.0×: identity. */
  EXPECT(capy_widget_dpi_scale_dim(256u, 100) == 100);
  EXPECT(capy_widget_dpi_scale_dim(256u, -42) == -42);
  /* 2.0×: doubles. */
  EXPECT(capy_widget_dpi_scale_dim(512u, 100) == 200);
  EXPECT(capy_widget_dpi_scale_dim(512u, -42) == -84);
  /* 1.5×: integer truncation toward zero. */
  EXPECT(capy_widget_dpi_scale_dim(384u, 100) == 150);
  EXPECT(capy_widget_dpi_scale_dim(384u, 1) == 1);    /* 384/256 = 1.5 → 1 */
  EXPECT(capy_widget_dpi_scale_dim(384u, -1) == -1);  /* trunc toward zero */
  /* 3.0×. */
  EXPECT(capy_widget_dpi_scale_dim(768u, 100) == 300);
  /* Saturating clamp on overflow. */
  EXPECT(capy_widget_dpi_scale_dim(512u, INT32_MAX) == INT32_MAX);
  EXPECT(capy_widget_dpi_scale_dim(512u, INT32_MIN) == INT32_MIN);
  /* Degenerate scale clamped: helper does not crash. */
  EXPECT(capy_widget_dpi_scale_dim(0u, 100) == 0);
}

static void test_dpi_scope_op_emitted_when_non_default(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *root;
  struct capy_dl_cmd cmd_buf[16];
  char text_buf[32];
  struct capy_display_list dl;
  capy_widget_context_init(&ctx, &allocator);
  root = capy_widget_create(&ctx, CAPY_WIDGET_PANEL);
  EXPECT(root != 0);
  capy_widget_set_bounds(root, 10, 20, 100u, 50u);
  capy_display_list_init(&dl, cmd_buf, 16u, text_buf, 32u);
  dl.dpi_scale_x256 = 384u;
  EXPECT(capy_widget_emit(root, &dl) == 0);
  /* First op must be DPI_SCOPE carrying the scale on `image_id`. */
  EXPECT(dl.count >= 1u);
  EXPECT(dl.cmds[0].op == CAPY_DL_DPI_SCOPE);
  EXPECT(dl.cmds[0].image_id == 384u);
  EXPECT(dl.cmds[0].rect.x == 10);
  EXPECT(dl.cmds[0].rect.y == 20);
  EXPECT(dl.cmds[0].rect.width == 100u);
  EXPECT(dl.cmds[0].rect.height == 50u);
  /* Second op is the usual root CLIP_PUSH. */
  EXPECT(dl.cmds[1].op == CAPY_DL_CLIP_PUSH);
  capy_widget_destroy(root);
}

static void test_dpi_scope_op_omitted_at_default(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *root;
  struct capy_dl_cmd cmd_buf[16];
  char text_buf[32];
  struct capy_display_list dl;
  uint32_t i;
  capy_widget_context_init(&ctx, &allocator);
  root = capy_widget_create(&ctx, CAPY_WIDGET_PANEL);
  capy_widget_set_bounds(root, 0, 0, 50u, 50u);
  capy_display_list_init(&dl, cmd_buf, 16u, text_buf, 32u);
  /* Leave dl.dpi_scale_x256 at the default 256. */
  EXPECT(dl.dpi_scale_x256 == 256u);
  EXPECT(capy_widget_emit(root, &dl) == 0);
  /* No DPI_SCOPE op anywhere — preserves byte-equivalent emit with pre-1.5. */
  for (i = 0u; i < dl.count; ++i) {
    EXPECT(dl.cmds[i].op != CAPY_DL_DPI_SCOPE);
  }
  /* First op is the usual root CLIP_PUSH. */
  EXPECT(dl.count >= 1u);
  EXPECT(dl.cmds[0].op == CAPY_DL_CLIP_PUSH);
  capy_widget_destroy(root);
}

static void test_dpi_independent_contexts(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx_a;
  struct capy_widget_context ctx_b;
  capy_widget_context_init(&ctx_a, &allocator);
  capy_widget_context_init(&ctx_b, &allocator);
  capy_widget_set_dpi_scale(&ctx_a, 384u);
  capy_widget_set_dpi_scale(&ctx_b, 512u);
  EXPECT(capy_widget_get_dpi_scale(&ctx_a) == 384u);
  EXPECT(capy_widget_get_dpi_scale(&ctx_b) == 512u);
  /* Mutating one does not affect the other. */
  capy_widget_set_dpi_scale(&ctx_a, 256u);
  EXPECT(capy_widget_get_dpi_scale(&ctx_a) == 256u);
  EXPECT(capy_widget_get_dpi_scale(&ctx_b) == 512u);
}

static void test_dpi_gpu_translator_skips_dpi_scope(void) {
  struct capy_dl_cmd cmds[2];
  struct capy_display_list dl;
  struct capy_gpu_quad quads[4];
  uint32_t count = 0u;
  /* Synthesize: DPI_SCOPE + RECT. Translator must skip the scope and emit
   * exactly one quad for the RECT. */
  cmds[0].op = CAPY_DL_DPI_SCOPE;
  cmds[0].rect.x = 0;
  cmds[0].rect.y = 0;
  cmds[0].rect.width = 100u;
  cmds[0].rect.height = 100u;
  cmds[0].color = 0u;
  cmds[0].text_offset = 0u;
  cmds[0].text_len = 0u;
  cmds[0].border_width = 0u;
  cmds[0].font_size = 0u;
  cmds[0].font_id = 0u;
  cmds[0].image_id = 512u;
  cmds[1].op = CAPY_DL_RECT;
  cmds[1].rect.x = 10;
  cmds[1].rect.y = 20;
  cmds[1].rect.width = 30u;
  cmds[1].rect.height = 40u;
  cmds[1].color = 0xFFAABBCCu;
  cmds[1].text_offset = 0u;
  cmds[1].text_len = 0u;
  cmds[1].border_width = 0u;
  cmds[1].font_size = 0u;
  cmds[1].font_id = 0u;
  cmds[1].image_id = 0u;
  dl.cmds = cmds;
  dl.count = 2u;
  dl.capacity = 2u;
  dl.text_pool = 0;
  dl.text_used = 0u;
  dl.text_capacity = 0u;
  dl.version = CAPY_DISPLAY_LIST_SCHEMA_VERSION;
  dl.theme = 0;
  dl.dpi_scale_x256 = 512u;
  dl.reserved_dpi = 0u;
  EXPECT(capy_dl_to_quads(&dl, quads, 4u, &count) == 0);
  EXPECT(count == 1u);
  EXPECT(quads[0].x0 == 10);
  EXPECT(quads[0].y0 == 20);
  EXPECT(quads[0].x1 == 40);
  EXPECT(quads[0].y1 == 60);
  EXPECT(quads[0].color == 0xFFAABBCCu);
}

/* ── v1.3: rich animation (tracks, springs, Bezier) (since 1.3.0) ──────── */

static void test_anim_track_sample_keyframes_ordered(void) {
  struct capy_anim_keyframe kfs[3];
  struct capy_anim_track track;
  int32_t out = 0;
  kfs[0].tick = 0u;
  kfs[0].value = 0;
  kfs[0].easing = (uint8_t)CAPY_ANIM_LINEAR;
  kfs[0].reserved = 0u;
  kfs[0].reserved2 = 0u;
  kfs[1].tick = 100u;
  kfs[1].value = 1000;
  kfs[1].easing = (uint8_t)CAPY_ANIM_LINEAR;
  kfs[1].reserved = 0u;
  kfs[1].reserved2 = 0u;
  kfs[2].tick = 200u;
  kfs[2].value = 0;
  kfs[2].easing = (uint8_t)CAPY_ANIM_LINEAR;
  kfs[2].reserved = 0u;
  kfs[2].reserved2 = 0u;
  track.keyframes = kfs;
  track.count = 3u;
  track.capacity = 3u;
  track.loop = 0;
  /* Before start clamps to first value. */
  EXPECT(capy_anim_track_sample(&track, 0u, &out) == 0);
  EXPECT(out == 0);
  /* Midpoint of first segment: linear lerp. */
  EXPECT(capy_anim_track_sample(&track, 50u, &out) == 0);
  EXPECT(out > 400 && out < 600);
  /* Exact second keyframe. */
  EXPECT(capy_anim_track_sample(&track, 100u, &out) == 0);
  EXPECT(out == 1000);
  /* Midpoint of second segment (1000 → 0, linear). */
  EXPECT(capy_anim_track_sample(&track, 150u, &out) == 0);
  EXPECT(out > 400 && out < 600);
  /* Past end clamps to last value (loop == 0). */
  EXPECT(capy_anim_track_sample(&track, 1000u, &out) == 0);
  EXPECT(out == 0);
  /* NULL guards. */
  EXPECT(capy_anim_track_sample(0, 0u, &out) == -1);
  EXPECT(capy_anim_track_sample(&track, 0u, 0) == -1);
}

static void test_anim_track_loop_infinite(void) {
  struct capy_anim_keyframe kfs[2];
  struct capy_anim_track track;
  int32_t out_a = 0;
  int32_t out_b = 0;
  kfs[0].tick = 0u;
  kfs[0].value = 0;
  kfs[0].easing = (uint8_t)CAPY_ANIM_LINEAR;
  kfs[0].reserved = 0u;
  kfs[0].reserved2 = 0u;
  kfs[1].tick = 100u;
  kfs[1].value = 1000;
  kfs[1].easing = (uint8_t)CAPY_ANIM_LINEAR;
  kfs[1].reserved = 0u;
  kfs[1].reserved2 = 0u;
  track.keyframes = kfs;
  track.count = 2u;
  track.capacity = 2u;
  track.loop = -1; /* infinite */
  /* Sample at t=50 and at t=150 (which wraps to t=50). Both must match. */
  EXPECT(capy_anim_track_sample(&track, 50u, &out_a) == 0);
  EXPECT(capy_anim_track_sample(&track, 150u, &out_b) == 0);
  EXPECT(out_a == out_b);
  /* Sample at t=250 (= 2 full loops + 50) also matches. */
  EXPECT(capy_anim_track_sample(&track, 250u, &out_b) == 0);
  EXPECT(out_a == out_b);
  /* Sample at exact loop boundary (t=100 → wraps to 0). */
  EXPECT(capy_anim_track_sample(&track, 100u, &out_b) == 0);
  EXPECT(out_b == 0);
}

static void test_anim_track_loop_finite_clamps(void) {
  struct capy_anim_keyframe kfs[2];
  struct capy_anim_track track;
  int32_t out = -1;
  kfs[0].tick = 0u;
  kfs[0].value = 0;
  kfs[0].easing = (uint8_t)CAPY_ANIM_LINEAR;
  kfs[0].reserved = 0u;
  kfs[0].reserved2 = 0u;
  kfs[1].tick = 100u;
  kfs[1].value = 1000;
  kfs[1].easing = (uint8_t)CAPY_ANIM_LINEAR;
  kfs[1].reserved = 0u;
  kfs[1].reserved2 = 0u;
  track.keyframes = kfs;
  track.count = 2u;
  track.capacity = 2u;
  track.loop = 2; /* play twice then clamp */
  /* Mid-first-loop, mid-second-loop: same value. */
  {
    int32_t a = 0;
    int32_t b = 0;
    EXPECT(capy_anim_track_sample(&track, 50u, &a) == 0);
    EXPECT(capy_anim_track_sample(&track, 150u, &b) == 0);
    EXPECT(a == b);
  }
  /* At the end of the 2nd loop (t = 200 = 2*total) → clamp to last value. */
  EXPECT(capy_anim_track_sample(&track, 200u, &out) == 0);
  EXPECT(out == 1000);
  /* Past end → still clamped. */
  EXPECT(capy_anim_track_sample(&track, 99999u, &out) == 0);
  EXPECT(out == 1000);
}

static void test_anim_spring_converges_with_damping(void) {
  struct capy_spring s;
  uint32_t i;
  s.target = 1000;
  s.position = 0;
  s.velocity = 0;
  s.stiffness = 64u;  /* Q8.8 ≈ 0.25 */
  s.damping = 128u;   /* Q8.8 ≈ 0.5 (heavy enough to converge quickly) */
  /* 500 substeps of 1 ms each. */
  for (i = 0u; i < 500u; ++i) {
    capy_spring_step(&s, 1u);
  }
  /* Should be near target. Use generous tolerance to avoid coupling the
   * test to the exact integrator constants. */
  {
    int32_t delta = s.position - s.target;
    if (delta < 0) {
      delta = -delta;
    }
    EXPECT(delta < 50);
  }
}

static void test_anim_spring_oscillates_undamped(void) {
  struct capy_spring s;
  uint32_t i;
  int saw_above = 0;
  int saw_below = 0;
  s.target = 1000;
  s.position = 0;
  s.velocity = 0;
  s.stiffness = 64u;
  s.damping = 0u; /* undamped: should oscillate forever */
  for (i = 0u; i < 200u; ++i) {
    capy_spring_step(&s, 1u);
    if (s.position > s.target + 50) {
      saw_above = 1;
    }
    if (s.position < s.target - 50 && i > 50u) {
      saw_below = 1;
    }
  }
  /* The undamped spring must overshoot the target at least once and then
   * come back below it. */
  EXPECT(saw_above == 1);
  EXPECT(saw_below == 1);
}

static void test_anim_bezier_monotonic(void) {
  /* Well-formed monotonically increasing control points → monotonic curve. */
  int32_t p0 = 0;
  int32_t p1 = 200;
  int32_t p2 = 800;
  int32_t p3 = 1000;
  int32_t prev = -1;
  int32_t t;
  for (t = 0; t <= 0x10000; t += 0x10000 / 16) {
    int32_t v = capy_anim_bezier_eval(p0, p1, p2, p3, t);
    EXPECT(v >= prev); /* non-decreasing */
    prev = v;
  }
  /* Endpoints exact. */
  EXPECT(capy_anim_bezier_eval(p0, p1, p2, p3, 0) == p0);
  EXPECT(capy_anim_bezier_eval(p0, p1, p2, p3, 0x10000) == p3);
  /* Out-of-range t clamps. */
  EXPECT(capy_anim_bezier_eval(p0, p1, p2, p3, -100) == p0);
  EXPECT(capy_anim_bezier_eval(p0, p1, p2, p3, 0x20000) == p3);
}

static void test_anim_track_determinism(void) {
  struct capy_anim_keyframe kfs[3];
  struct capy_anim_track track;
  int32_t a = 0;
  int32_t b = 0;
  uint32_t now;
  kfs[0].tick = 10u;
  kfs[0].value = -500;
  kfs[0].easing = (uint8_t)CAPY_ANIM_EASE_IN_OUT;
  kfs[0].reserved = 0u;
  kfs[0].reserved2 = 0u;
  kfs[1].tick = 60u;
  kfs[1].value = 1500;
  kfs[1].easing = (uint8_t)CAPY_ANIM_EASE_OUT;
  kfs[1].reserved = 0u;
  kfs[1].reserved2 = 0u;
  kfs[2].tick = 110u;
  kfs[2].value = -750;
  kfs[2].easing = (uint8_t)CAPY_ANIM_LINEAR;
  kfs[2].reserved = 0u;
  kfs[2].reserved2 = 0u;
  track.keyframes = kfs;
  track.count = 3u;
  track.capacity = 3u;
  track.loop = -1;
  for (now = 0u; now < 250u; now += 13u) {
    EXPECT(capy_anim_track_sample(&track, now, &a) == 0);
    EXPECT(capy_anim_track_sample(&track, now, &b) == 0);
    EXPECT(a == b);
  }
}

/* ── v1.2: GPU translator capy_dl_to_quads (since 1.2.0) ───────────────── */

static void test_gpu_rect_to_single_quad(void) {
  struct capy_dl_cmd cmds[1];
  struct capy_display_list dl;
  struct capy_gpu_quad quads[4];
  uint32_t count = 0u;
  cmds[0].op = CAPY_DL_RECT;
  cmds[0].rect.x = 10;
  cmds[0].rect.y = 20;
  cmds[0].rect.width = 30u;
  cmds[0].rect.height = 40u;
  cmds[0].color = 0xFFAABBCCu;
  cmds[0].text_offset = 0u;
  cmds[0].text_len = 0u;
  cmds[0].border_width = 0u;
  cmds[0].font_size = 0u;
  cmds[0].font_id = 0u;
  cmds[0].image_id = 0u;
  dl.cmds = cmds;
  dl.count = 1u;
  dl.capacity = 1u;
  dl.text_pool = 0;
  dl.text_used = 0u;
  dl.text_capacity = 0u;
  dl.version = CAPY_DISPLAY_LIST_SCHEMA_VERSION;
  dl.theme = 0;
  EXPECT(capy_dl_to_quads(&dl, quads, 4u, &count) == 0);
  EXPECT(count == 1u);
  EXPECT(quads[0].x0 == 10);
  EXPECT(quads[0].y0 == 20);
  EXPECT(quads[0].x1 == 40);
  EXPECT(quads[0].y1 == 60);
  EXPECT(quads[0].color == 0xFFAABBCCu);
  EXPECT(quads[0].texture_id == 0u);
}

static void test_gpu_border_to_four_quads(void) {
  struct capy_dl_cmd cmds[1];
  struct capy_display_list dl;
  struct capy_gpu_quad quads[8];
  uint32_t count = 0u;
  cmds[0].op = CAPY_DL_BORDER;
  cmds[0].rect.x = 0;
  cmds[0].rect.y = 0;
  cmds[0].rect.width = 50u;
  cmds[0].rect.height = 30u;
  cmds[0].color = 0xFF112233u;
  cmds[0].text_offset = 0u;
  cmds[0].text_len = 0u;
  cmds[0].border_width = 2u;
  cmds[0].font_size = 0u;
  cmds[0].font_id = 0u;
  cmds[0].image_id = 0u;
  dl.cmds = cmds;
  dl.count = 1u;
  dl.capacity = 1u;
  dl.text_pool = 0;
  dl.text_used = 0u;
  dl.text_capacity = 0u;
  dl.version = CAPY_DISPLAY_LIST_SCHEMA_VERSION;
  dl.theme = 0;
  EXPECT(capy_dl_to_quads(&dl, quads, 8u, &count) == 0);
  EXPECT(count == 4u);
  /* Top strip: full width, border_width thick. */
  EXPECT(quads[0].x0 == 0);
  EXPECT(quads[0].y0 == 0);
  EXPECT(quads[0].x1 == 50);
  EXPECT(quads[0].y1 == 2);
  /* Bottom strip. */
  EXPECT(quads[1].x0 == 0);
  EXPECT(quads[1].y0 == 28);
  EXPECT(quads[1].x1 == 50);
  EXPECT(quads[1].y1 == 30);
  /* Left strip (between top and bottom). */
  EXPECT(quads[2].x0 == 0);
  EXPECT(quads[2].y0 == 2);
  EXPECT(quads[2].x1 == 2);
  EXPECT(quads[2].y1 == 28);
  /* Right strip. */
  EXPECT(quads[3].x0 == 48);
  EXPECT(quads[3].y0 == 2);
  EXPECT(quads[3].x1 == 50);
  EXPECT(quads[3].y1 == 28);
  /* All four share the same colour. */
  EXPECT(quads[0].color == 0xFF112233u);
  EXPECT(quads[1].color == 0xFF112233u);
  EXPECT(quads[2].color == 0xFF112233u);
  EXPECT(quads[3].color == 0xFF112233u);
}

static void test_gpu_clip_intersects_quads(void) {
  /* CLIP_PUSH(rect) … RECT(big) … CLIP_POP. The inner rect should be
   * intersected against the active clip. A second non-overlapping rect
   * after CLIP_POP should be emitted unclipped. */
  struct capy_dl_cmd cmds[4];
  struct capy_display_list dl;
  struct capy_gpu_quad quads[4];
  uint32_t count = 0u;
  /* CLIP_PUSH covering [10,10,30,30]. */
  cmds[0].op = CAPY_DL_CLIP_PUSH;
  cmds[0].rect.x = 10;
  cmds[0].rect.y = 10;
  cmds[0].rect.width = 30u;
  cmds[0].rect.height = 30u;
  cmds[0].color = 0u;
  cmds[0].text_offset = 0u;
  cmds[0].text_len = 0u;
  cmds[0].border_width = 0u;
  cmds[0].font_size = 0u;
  cmds[0].font_id = 0u;
  cmds[0].image_id = 0u;
  /* RECT spanning [0,0,100,100] — clipped to the active rect. */
  cmds[1].op = CAPY_DL_RECT;
  cmds[1].rect.x = 0;
  cmds[1].rect.y = 0;
  cmds[1].rect.width = 100u;
  cmds[1].rect.height = 100u;
  cmds[1].color = 0xFF010203u;
  cmds[1].text_offset = 0u;
  cmds[1].text_len = 0u;
  cmds[1].border_width = 0u;
  cmds[1].font_size = 0u;
  cmds[1].font_id = 0u;
  cmds[1].image_id = 0u;
  /* CLIP_POP. */
  cmds[2].op = CAPY_DL_CLIP_POP;
  cmds[2].rect.x = 0;
  cmds[2].rect.y = 0;
  cmds[2].rect.width = 0u;
  cmds[2].rect.height = 0u;
  cmds[2].color = 0u;
  cmds[2].text_offset = 0u;
  cmds[2].text_len = 0u;
  cmds[2].border_width = 0u;
  cmds[2].font_size = 0u;
  cmds[2].font_id = 0u;
  cmds[2].image_id = 0u;
  /* RECT after pop, fully outside the previous clip — emits unclipped. */
  cmds[3].op = CAPY_DL_RECT;
  cmds[3].rect.x = 60;
  cmds[3].rect.y = 60;
  cmds[3].rect.width = 10u;
  cmds[3].rect.height = 10u;
  cmds[3].color = 0xFF040506u;
  cmds[3].text_offset = 0u;
  cmds[3].text_len = 0u;
  cmds[3].border_width = 0u;
  cmds[3].font_size = 0u;
  cmds[3].font_id = 0u;
  cmds[3].image_id = 0u;
  dl.cmds = cmds;
  dl.count = 4u;
  dl.capacity = 4u;
  dl.text_pool = 0;
  dl.text_used = 0u;
  dl.text_capacity = 0u;
  dl.version = CAPY_DISPLAY_LIST_SCHEMA_VERSION;
  dl.theme = 0;
  EXPECT(capy_dl_to_quads(&dl, quads, 4u, &count) == 0);
  EXPECT(count == 2u);
  /* First quad is the inner rect clipped to [10,10,40,40]. */
  EXPECT(quads[0].x0 == 10);
  EXPECT(quads[0].y0 == 10);
  EXPECT(quads[0].x1 == 40);
  EXPECT(quads[0].y1 == 40);
  EXPECT(quads[0].color == 0xFF010203u);
  /* Second quad is the post-pop rect, unclipped. */
  EXPECT(quads[1].x0 == 60);
  EXPECT(quads[1].y0 == 60);
  EXPECT(quads[1].x1 == 70);
  EXPECT(quads[1].y1 == 70);
  EXPECT(quads[1].color == 0xFF040506u);
}

static void test_gpu_overflow_returns_minus_one(void) {
  struct capy_dl_cmd cmds[2];
  struct capy_display_list dl;
  struct capy_gpu_quad quads[1];
  uint32_t count = 0u;
  cmds[0].op = CAPY_DL_RECT;
  cmds[0].rect.x = 0;
  cmds[0].rect.y = 0;
  cmds[0].rect.width = 10u;
  cmds[0].rect.height = 10u;
  cmds[0].color = 0xFFFFFFFFu;
  cmds[0].text_offset = 0u;
  cmds[0].text_len = 0u;
  cmds[0].border_width = 0u;
  cmds[0].font_size = 0u;
  cmds[0].font_id = 0u;
  cmds[0].image_id = 0u;
  cmds[1] = cmds[0];
  cmds[1].rect.x = 20;
  cmds[1].color = 0xFF000000u;
  dl.cmds = cmds;
  dl.count = 2u;
  dl.capacity = 2u;
  dl.text_pool = 0;
  dl.text_used = 0u;
  dl.text_capacity = 0u;
  dl.version = CAPY_DISPLAY_LIST_SCHEMA_VERSION;
  dl.theme = 0;
  /* cap=1 → first quad fits, second overflows → -1 with partial count=1. */
  EXPECT(capy_dl_to_quads(&dl, quads, 1u, &count) == -1);
  EXPECT(count == 1u);
  /* NULL guards. */
  EXPECT(capy_dl_to_quads(0, quads, 1u, &count) == -1);
  EXPECT(capy_dl_to_quads(&dl, quads, 1u, 0) == -1);
  EXPECT(capy_dl_to_quads(&dl, 0, 4u, &count) == -1);
}

static void test_gpu_unbalanced_clip_rejected(void) {
  struct capy_dl_cmd cmds[2];
  struct capy_display_list dl;
  struct capy_gpu_quad quads[4];
  uint32_t count = 0u;
  /* CLIP_POP without a matching push → -1. */
  cmds[0].op = CAPY_DL_CLIP_POP;
  cmds[0].rect.x = 0;
  cmds[0].rect.y = 0;
  cmds[0].rect.width = 0u;
  cmds[0].rect.height = 0u;
  cmds[0].color = 0u;
  cmds[0].text_offset = 0u;
  cmds[0].text_len = 0u;
  cmds[0].border_width = 0u;
  cmds[0].font_size = 0u;
  cmds[0].font_id = 0u;
  cmds[0].image_id = 0u;
  dl.cmds = cmds;
  dl.count = 1u;
  dl.capacity = 1u;
  dl.text_pool = 0;
  dl.text_used = 0u;
  dl.text_capacity = 0u;
  dl.version = CAPY_DISPLAY_LIST_SCHEMA_VERSION;
  dl.theme = 0;
  EXPECT(capy_dl_to_quads(&dl, quads, 4u, &count) == -1);
  /* Trailing PUSH at end of walk (depth != 0) → -1. */
  cmds[0].op = CAPY_DL_CLIP_PUSH;
  cmds[0].rect.x = 0;
  cmds[0].rect.y = 0;
  cmds[0].rect.width = 10u;
  cmds[0].rect.height = 10u;
  count = 0u;
  EXPECT(capy_dl_to_quads(&dl, quads, 4u, &count) == -1);
}

static void test_gpu_image_ref_carries_texture_and_uvs(void) {
  struct capy_dl_cmd cmds[1];
  struct capy_display_list dl;
  struct capy_gpu_quad quads[2];
  uint32_t count = 0u;
  cmds[0].op = CAPY_DL_IMAGE_REF;
  cmds[0].rect.x = 5;
  cmds[0].rect.y = 7;
  cmds[0].rect.width = 32u;
  cmds[0].rect.height = 32u;
  cmds[0].color = 0xFFFFFFFFu;
  cmds[0].text_offset = 0u;
  cmds[0].text_len = 0u;
  cmds[0].border_width = 0u;
  cmds[0].font_size = 0u;
  cmds[0].font_id = 0u;
  cmds[0].image_id = 9001u;
  dl.cmds = cmds;
  dl.count = 1u;
  dl.capacity = 1u;
  dl.text_pool = 0;
  dl.text_used = 0u;
  dl.text_capacity = 0u;
  dl.version = CAPY_DISPLAY_LIST_SCHEMA_VERSION;
  dl.theme = 0;
  EXPECT(capy_dl_to_quads(&dl, quads, 2u, &count) == 0);
  EXPECT(count == 1u);
  EXPECT(quads[0].texture_id == 9001u);
  EXPECT(quads[0].uv_x0 == 0);
  EXPECT(quads[0].uv_y0 == 0);
  EXPECT(quads[0].uv_x1 == 32);
  EXPECT(quads[0].uv_y1 == 32);
}

static void test_gpu_translator_determinism(void) {
  /* Same DL → same quads byte-identical across runs. */
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *panel;
  struct capy_widget *button;
  struct capy_dl_cmd cmd_buf[32];
  char text_buf[32];
  struct capy_display_list dl;
  struct capy_gpu_quad quads_a[64];
  struct capy_gpu_quad quads_b[64];
  uint32_t count_a = 0u;
  uint32_t count_b = 0u;
  uint32_t i;
  capy_widget_context_init(&ctx, &allocator);
  panel = capy_widget_create(&ctx, CAPY_WIDGET_PANEL);
  button = capy_widget_create(&ctx, CAPY_WIDGET_BUTTON);
  EXPECT(panel && button);
  capy_widget_set_bounds(panel, 0, 0, 100u, 100u);
  capy_widget_set_bounds(button, 10, 10, 30u, 20u);
  EXPECT(capy_widget_add_child(panel, button) == 0);
  capy_display_list_init(&dl, cmd_buf, 32u, text_buf, 32u);
  EXPECT(capy_widget_emit(panel, &dl) == 0);
  EXPECT(capy_dl_to_quads(&dl, quads_a, 64u, &count_a) == 0);
  EXPECT(capy_dl_to_quads(&dl, quads_b, 64u, &count_b) == 0);
  EXPECT(count_a == count_b);
  EXPECT(count_a > 0u);
  for (i = 0u; i < count_a; ++i) {
    EXPECT(quads_a[i].x0 == quads_b[i].x0);
    EXPECT(quads_a[i].y0 == quads_b[i].y0);
    EXPECT(quads_a[i].x1 == quads_b[i].x1);
    EXPECT(quads_a[i].y1 == quads_b[i].y1);
    EXPECT(quads_a[i].color == quads_b[i].color);
    EXPECT(quads_a[i].texture_id == quads_b[i].texture_id);
  }
  capy_widget_destroy(panel);
}

/* ── v1.0: ABI freeze markers (since 1.0.0) ────────────────────────────── */

CAPYUI_API_DEPRECATED("ABI 1.0 freeze marker — exercised only by the test")
static int capyui_v1_freeze_canary(int x) {
  /* This static helper exists solely so the test can take its address and
   * confirm CAPYUI_API_DEPRECATED expands to something valid (attribute on
   * GCC/Clang/MSVC, empty elsewhere) without polluting the public surface. */
  return x + 1;
}

static void test_v1_freeze_markers(void) {
  /* Public version macros must line up with VERSION + Makefile while the
   * v1 freeze anchors remain present. */
  EXPECT(CAPYUI_API_VERSION_MAJOR == 2);
  EXPECT(CAPYUI_API_VERSION_MINOR == 22);
  EXPECT(CAPYUI_API_VERSION_PATCH == 0);
  /* CAPYUI_API_DEPRECATED is invocable as a macro and accepts a string lit;
   * we already saw it expand above on capyui_v1_freeze_canary. The canary
   * must remain callable — deprecation does not change runtime behaviour.
   * Suppress the expected -Wdeprecated-declarations so the test still
   * compiles under -Werror; the goal is to confirm the macro expansion
   * compiles and the symbol still links. */
#if defined(__GNUC__) || defined(__clang__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
  EXPECT(capyui_v1_freeze_canary(41) == 42);
#if defined(__GNUC__) || defined(__clang__)
#  pragma GCC diagnostic pop
#endif
  /* The pre-1.0 ABI lineage is intact: every documented surface introduced
   * in 0.x must still exist and behave identically. We sample one anchor
   * per delivered slice so a future symbol removal trips this immediately
   * instead of waiting for downstream link errors. */
  EXPECT((int)CAPY_WIDGET_BUTTON > (int)CAPY_WIDGET_NONE);          /* 0.0.1 */
  EXPECT((int)CAPY_LAYOUT_STACK > (int)CAPY_LAYOUT_NONE);            /* 0.1   */
  EXPECT((int)CAPY_DL_RECT > (int)CAPY_DL_NONE);                     /* 0.2   */
  EXPECT(CAPY_DISPLAY_LIST_SCHEMA_VERSION == 7u);                    /* 2.0   */
  EXPECT(CAPY_KEY_TAB == 9u);                                        /* 0.3   */
  EXPECT(CAPY_MOD_SHIFT == 0x4u);                                    /* 0.3   */
  EXPECT(CAPY_MOD_CAPSLOCK == 0x10u);                                /* 0.10  */
  EXPECT((int)CAPY_ANIM_EASE_IN_OUT > (int)CAPY_ANIM_LINEAR);        /* 0.5   */
  EXPECT(CAPY_TOKEN_COUNT > CAPY_TOKEN_NONE);                        /* 0.6   */
  EXPECT(CAPY_THEME_FORMAT_VERSION == 2u);                           /* 0.14  */
  EXPECT(CAPY_A11Y_FOCUSED == 0x1u);                                 /* 0.11  */
  EXPECT((int)CAPY_PLURAL_OTHER > (int)CAPY_PLURAL_EN);              /* 0.13  */
  EXPECT((int)CAPY_WIDGET_EVENT_GAMEPAD >                            /* 0.10  */
         (int)CAPY_WIDGET_EVENT_POINTER_UP);
}

/* ── v2.14: advanced widget state — date picker (since 2.14.0) ──────────── */

static void test_date_set_valid_and_get(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *dp;
  struct capy_date d = {0u, 0u, 0u};
  capy_widget_context_init(&ctx, &allocator);
  dp = capy_widget_create(&ctx, CAPY_WIDGET_DATE_PICKER);
  EXPECT(dp != 0);
  EXPECT(capy_widget_set_date(dp, 2024u, 2u, 29u) == 0); /* leap day valid */
  EXPECT(capy_widget_get_date(dp, &d) == 1);
  EXPECT(d.year == 2024u && d.month == 2u && d.day == 29u);
  capy_widget_destroy(dp);
}

static void test_date_unset_get_returns_zero(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *dp;
  struct capy_date d = {1u, 1u, 1u};
  capy_widget_context_init(&ctx, &allocator);
  dp = capy_widget_create(&ctx, CAPY_WIDGET_DATE_PICKER);
  EXPECT(dp != 0);
  /* fresh widget: unset → get returns 0 and copies the zeroed value */
  EXPECT(capy_widget_get_date(dp, &d) == 0);
  EXPECT(d.year == 0u && d.month == 0u && d.day == 0u);
  capy_widget_destroy(dp);
}

static void test_date_invalid_rejected_fail_closed(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *dp;
  struct capy_date d = {0u, 0u, 0u};
  capy_widget_context_init(&ctx, &allocator);
  dp = capy_widget_create(&ctx, CAPY_WIDGET_DATE_PICKER);
  EXPECT(dp != 0);
  EXPECT(capy_widget_set_date(dp, 2023u, 6u, 15u) == 0); /* known-good */
  EXPECT(capy_widget_set_date(dp, 2023u, 2u, 29u) == -1); /* not a leap year */
  EXPECT(capy_widget_set_date(dp, 2024u, 13u, 1u) == -1); /* month > 12 */
  EXPECT(capy_widget_set_date(dp, 2024u, 4u, 31u) == -1); /* Apr has 30 */
  EXPECT(capy_widget_set_date(dp, 2024u, 0u, 10u) == -1); /* month 0 */
  EXPECT(capy_widget_set_date(dp, 2024u, 5u, 0u) == -1);  /* day 0 */
  EXPECT(capy_widget_set_date(dp, 0u, 5u, 10u) == -1);    /* year 0 */
  /* the previously stored value is left unchanged (fail-closed) */
  EXPECT(capy_widget_get_date(dp, &d) == 1);
  EXPECT(d.year == 2023u && d.month == 6u && d.day == 15u);
  capy_widget_destroy(dp);
}

static void test_date_clear_resets(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *dp;
  struct capy_date d = {0u, 0u, 0u};
  capy_widget_context_init(&ctx, &allocator);
  dp = capy_widget_create(&ctx, CAPY_WIDGET_DATE_PICKER);
  EXPECT(dp != 0);
  EXPECT(capy_widget_set_date(dp, 2000u, 12u, 31u) == 0);
  EXPECT(capy_widget_get_date(dp, &d) == 1);
  EXPECT(capy_widget_clear_date(dp) == 0);
  EXPECT(capy_widget_get_date(dp, &d) == 0);
  EXPECT(d.year == 0u && d.month == 0u && d.day == 0u);
  capy_widget_destroy(dp);
}

static void test_date_wrong_widget_type_rejected(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *btn;
  struct capy_date d = {0u, 0u, 0u};
  capy_widget_context_init(&ctx, &allocator);
  btn = capy_widget_create(&ctx, CAPY_WIDGET_BUTTON);
  EXPECT(btn != 0);
  EXPECT(capy_widget_set_date(btn, 2024u, 1u, 1u) == -1);
  EXPECT(capy_widget_clear_date(btn) == -1);
  EXPECT(capy_widget_get_date(btn, &d) == -1);
  capy_widget_destroy(btn);
}

static void test_date_is_valid_calendar_predicate(void) {
  EXPECT(capy_date_is_valid(2000u, 2u, 29u) == 1); /* div by 400 → leap */
  EXPECT(capy_date_is_valid(1900u, 2u, 29u) == 0); /* div 100 not 400 */
  EXPECT(capy_date_is_valid(2024u, 2u, 29u) == 1); /* div 4 not 100 → leap */
  EXPECT(capy_date_is_valid(2023u, 2u, 28u) == 1);
  EXPECT(capy_date_is_valid(2023u, 2u, 29u) == 0);
  EXPECT(capy_date_is_valid(2024u, 1u, 31u) == 1);
  EXPECT(capy_date_is_valid(2024u, 4u, 30u) == 1);
  EXPECT(capy_date_is_valid(2024u, 4u, 31u) == 0);
  EXPECT(capy_date_is_valid(2024u, 12u, 31u) == 1);
  EXPECT(capy_date_is_valid(2024u, 13u, 1u) == 0);
  EXPECT(capy_date_is_valid(1u, 1u, 1u) == 1);
}

static void test_date_null_guards(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *dp;
  capy_widget_context_init(&ctx, &allocator);
  dp = capy_widget_create(&ctx, CAPY_WIDGET_DATE_PICKER);
  EXPECT(dp != 0);
  EXPECT(capy_widget_set_date(0, 2024u, 1u, 1u) == -1);
  EXPECT(capy_widget_clear_date(0) == -1);
  EXPECT(capy_widget_get_date(0, 0) == -1);
  EXPECT(capy_widget_get_date(dp, 0) == -1); /* NULL out */
  capy_widget_destroy(dp);
}

static void test_date_determinism(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *dp;
  struct capy_date a = {0u, 0u, 0u};
  struct capy_date b = {0u, 0u, 0u};
  capy_widget_context_init(&ctx, &allocator);
  dp = capy_widget_create(&ctx, CAPY_WIDGET_DATE_PICKER);
  EXPECT(dp != 0);
  EXPECT(capy_widget_set_date(dp, 2026u, 5u, 29u) == 0);
  EXPECT(capy_widget_get_date(dp, &a) == 1);
  EXPECT(capy_widget_get_date(dp, &b) == 1);
  EXPECT(a.year == b.year && a.month == b.month && a.day == b.day);
  capy_widget_destroy(dp);
}

/* ── v2.15: advanced widget state — color picker (since 2.15.0) ─────────── */

static void test_color_pack_channel_order(void) {
  /* 0xAARRGGBB: alpha high byte, then red, green, blue. */
  uint32_t c = capy_color_pack(0x12u, 0x34u, 0x56u, 0x78u);
  EXPECT(c == 0x78123456u);
  EXPECT(((c >> 24) & 0xFFu) == 0x78u); /* alpha */
  EXPECT(((c >> 16) & 0xFFu) == 0x12u); /* red */
  EXPECT(((c >> 8) & 0xFFu) == 0x34u);  /* green */
  EXPECT((c & 0xFFu) == 0x56u);         /* blue */
}

static void test_color_pack_extremes(void) {
  EXPECT(capy_color_pack(0xFFu, 0xFFu, 0xFFu, 0xFFu) == 0xFFFFFFFFu);
  EXPECT(capy_color_pack(0u, 0u, 0u, 0u) == 0u);
  /* opaque black: the alpha high byte must set without signed-shift UB. */
  EXPECT(capy_color_pack(0u, 0u, 0u, 0xFFu) == 0xFF000000u);
}

static void test_color_set_and_get(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *cp;
  uint32_t got = 0u;
  capy_widget_context_init(&ctx, &allocator);
  cp = capy_widget_create(&ctx, CAPY_WIDGET_COLOR_PICKER);
  EXPECT(cp != 0);
  EXPECT(capy_widget_set_color(cp, 0x80112233u) == 0);
  EXPECT(capy_widget_get_color(cp, &got) == 1);
  EXPECT(got == 0x80112233u);
  capy_widget_destroy(cp);
}

static void test_color_unset_get_returns_zero(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *cp;
  uint32_t got = 0xDEADBEEFu;
  capy_widget_context_init(&ctx, &allocator);
  cp = capy_widget_create(&ctx, CAPY_WIDGET_COLOR_PICKER);
  EXPECT(cp != 0);
  /* fresh widget: unset → get returns 0 and writes 0. */
  EXPECT(capy_widget_get_color(cp, &got) == 0);
  EXPECT(got == 0u);
  capy_widget_destroy(cp);
}

static void test_color_clear_resets(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *cp;
  uint32_t got = 0u;
  capy_widget_context_init(&ctx, &allocator);
  cp = capy_widget_create(&ctx, CAPY_WIDGET_COLOR_PICKER);
  EXPECT(cp != 0);
  EXPECT(capy_widget_set_color(cp, 0xFFAABBCCu) == 0);
  EXPECT(capy_widget_get_color(cp, &got) == 1);
  EXPECT(capy_widget_clear_color(cp) == 0);
  EXPECT(capy_widget_get_color(cp, &got) == 0);
  EXPECT(got == 0u);
  capy_widget_destroy(cp);
}

static void test_color_wrong_widget_type_rejected(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *btn;
  uint32_t got = 0u;
  capy_widget_context_init(&ctx, &allocator);
  btn = capy_widget_create(&ctx, CAPY_WIDGET_BUTTON);
  EXPECT(btn != 0);
  EXPECT(capy_widget_set_color(btn, 0x11223344u) == -1);
  EXPECT(capy_widget_clear_color(btn) == -1);
  EXPECT(capy_widget_get_color(btn, &got) == -1);
  capy_widget_destroy(btn);
}

static void test_color_null_guards(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *cp;
  capy_widget_context_init(&ctx, &allocator);
  cp = capy_widget_create(&ctx, CAPY_WIDGET_COLOR_PICKER);
  EXPECT(cp != 0);
  EXPECT(capy_widget_set_color(0, 0x1u) == -1);
  EXPECT(capy_widget_clear_color(0) == -1);
  EXPECT(capy_widget_get_color(0, 0) == -1);
  EXPECT(capy_widget_get_color(cp, 0) == -1); /* NULL out */
  capy_widget_destroy(cp);
}

static void test_color_determinism(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *cp;
  uint32_t a = 0u;
  uint32_t b = 1u;
  capy_widget_context_init(&ctx, &allocator);
  cp = capy_widget_create(&ctx, CAPY_WIDGET_COLOR_PICKER);
  EXPECT(cp != 0);
  EXPECT(capy_widget_set_color(cp, capy_color_pack(10u, 20u, 30u, 40u)) == 0);
  EXPECT(capy_widget_get_color(cp, &a) == 1);
  EXPECT(capy_widget_get_color(cp, &b) == 1);
  EXPECT(a == b);
  EXPECT(a == capy_color_pack(10u, 20u, 30u, 40u));
  capy_widget_destroy(cp);
}

/* ── v2.16: advanced widget state — table columns (since 2.16.0) ────────── */

static void test_table_set_and_query(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *tbl;
  const uint16_t widths[3] = {100u, 200u, 150u};
  uint16_t w0 = 0u;
  uint16_t w1 = 0u;
  uint16_t w2 = 0u;
  capy_widget_context_init(&ctx, &allocator);
  tbl = capy_widget_create(&ctx, CAPY_WIDGET_TABLE);
  EXPECT(tbl != 0);
  EXPECT(capy_widget_set_table_columns(tbl, widths, 3u) == 0);
  EXPECT(capy_widget_table_column_count(tbl) == 3);
  EXPECT(capy_widget_table_column_width(tbl, 0u, &w0) == 0 && w0 == 100u);
  EXPECT(capy_widget_table_column_width(tbl, 1u, &w1) == 0 && w1 == 200u);
  EXPECT(capy_widget_table_column_width(tbl, 2u, &w2) == 0 && w2 == 150u);
  capy_widget_destroy(tbl);
}

static void test_table_index_out_of_range(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *tbl;
  const uint16_t widths[2] = {10u, 20u};
  uint16_t out = 0u;
  capy_widget_context_init(&ctx, &allocator);
  tbl = capy_widget_create(&ctx, CAPY_WIDGET_TABLE);
  EXPECT(tbl != 0);
  EXPECT(capy_widget_set_table_columns(tbl, widths, 2u) == 0);
  EXPECT(capy_widget_table_column_width(tbl, 2u, &out) == -1);   /* index == count */
  EXPECT(capy_widget_table_column_width(tbl, 100u, &out) == -1); /* far past end */
  capy_widget_destroy(tbl);
}

static void test_table_set_zero_count_clears(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *tbl;
  const uint16_t widths[2] = {10u, 20u};
  uint16_t out = 0u;
  capy_widget_context_init(&ctx, &allocator);
  tbl = capy_widget_create(&ctx, CAPY_WIDGET_TABLE);
  EXPECT(tbl != 0);
  EXPECT(capy_widget_set_table_columns(tbl, widths, 2u) == 0);
  EXPECT(capy_widget_table_column_count(tbl) == 2);
  /* count 0 clears even when a (ignored) pointer is passed */
  EXPECT(capy_widget_set_table_columns(tbl, widths, 0u) == 0);
  EXPECT(capy_widget_table_column_count(tbl) == 0);
  EXPECT(capy_widget_table_column_width(tbl, 0u, &out) == -1);
  capy_widget_destroy(tbl);
}

static void test_table_set_null_widths_rejected(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *tbl;
  const uint16_t widths[1] = {42u};
  capy_widget_context_init(&ctx, &allocator);
  tbl = capy_widget_create(&ctx, CAPY_WIDGET_TABLE);
  EXPECT(tbl != 0);
  EXPECT(capy_widget_set_table_columns(tbl, widths, 1u) == 0); /* known-good */
  EXPECT(capy_widget_set_table_columns(tbl, 0, 3u) == -1);     /* NULL + count>0 */
  /* fail-closed: previous model intact */
  EXPECT(capy_widget_table_column_count(tbl) == 1);
  capy_widget_destroy(tbl);
}

static void test_table_clear(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *tbl;
  const uint16_t widths[2] = {5u, 6u};
  uint16_t out = 0u;
  capy_widget_context_init(&ctx, &allocator);
  tbl = capy_widget_create(&ctx, CAPY_WIDGET_TABLE);
  EXPECT(tbl != 0);
  EXPECT(capy_widget_set_table_columns(tbl, widths, 2u) == 0);
  EXPECT(capy_widget_clear_table_columns(tbl) == 0);
  EXPECT(capy_widget_table_column_count(tbl) == 0);
  EXPECT(capy_widget_table_column_width(tbl, 0u, &out) == -1);
  capy_widget_destroy(tbl);
}

static void test_table_wrong_widget_type_rejected(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *btn;
  const uint16_t widths[1] = {1u};
  uint16_t out = 0u;
  capy_widget_context_init(&ctx, &allocator);
  btn = capy_widget_create(&ctx, CAPY_WIDGET_BUTTON);
  EXPECT(btn != 0);
  EXPECT(capy_widget_set_table_columns(btn, widths, 1u) == -1);
  EXPECT(capy_widget_clear_table_columns(btn) == -1);
  EXPECT(capy_widget_table_column_count(btn) == -1);
  EXPECT(capy_widget_table_column_width(btn, 0u, &out) == -1);
  capy_widget_destroy(btn);
}

static void test_table_null_guards(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *tbl;
  const uint16_t widths[1] = {1u};
  uint16_t out = 0u;
  capy_widget_context_init(&ctx, &allocator);
  tbl = capy_widget_create(&ctx, CAPY_WIDGET_TABLE);
  EXPECT(tbl != 0);
  EXPECT(capy_widget_set_table_columns(0, widths, 1u) == -1);
  EXPECT(capy_widget_clear_table_columns(0) == -1);
  EXPECT(capy_widget_table_column_count(0) == -1);
  EXPECT(capy_widget_table_column_width(0, 0u, &out) == -1);
  EXPECT(capy_widget_set_table_columns(tbl, widths, 1u) == 0);
  EXPECT(capy_widget_table_column_width(tbl, 0u, 0) == -1); /* NULL out */
  capy_widget_destroy(tbl);
}

static void test_table_default_no_columns(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *tbl;
  uint16_t out = 0u;
  capy_widget_context_init(&ctx, &allocator);
  tbl = capy_widget_create(&ctx, CAPY_WIDGET_TABLE);
  EXPECT(tbl != 0);
  /* fresh TABLE: no model → count 0, any width query -1 */
  EXPECT(capy_widget_table_column_count(tbl) == 0);
  EXPECT(capy_widget_table_column_width(tbl, 0u, &out) == -1);
  capy_widget_destroy(tbl);
}

/* ── v2.17: advanced widget state — autocomplete suggestions (since 2.17.0) ─ */

static void test_autocomplete_set_and_query(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *ac;
  const char *const items[] = {"apple", "apricot", "avocado"};
  const char *got = 0;
  capy_widget_context_init(&ctx, &allocator);
  ac = capy_widget_create(&ctx, CAPY_WIDGET_AUTOCOMPLETE);
  EXPECT(ac != 0);
  EXPECT(capy_widget_set_autocomplete(ac, items, 3u) == 0);
  EXPECT(capy_widget_autocomplete_count(ac) == 3);
  EXPECT(capy_widget_autocomplete_item(ac, 0u, &got) == 0 && got == items[0]);
  EXPECT(capy_widget_autocomplete_item(ac, 2u, &got) == 0 && got == items[2]);
  capy_widget_destroy(ac);
}

static void test_autocomplete_item_out_of_range(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *ac;
  const char *const items[] = {"x", "y"};
  const char *got = 0;
  capy_widget_context_init(&ctx, &allocator);
  ac = capy_widget_create(&ctx, CAPY_WIDGET_AUTOCOMPLETE);
  EXPECT(ac != 0);
  EXPECT(capy_widget_autocomplete_item(ac, 0u, &got) == -1); /* no list yet */
  EXPECT(capy_widget_set_autocomplete(ac, items, 2u) == 0);
  EXPECT(capy_widget_autocomplete_item(ac, 2u, &got) == -1); /* index == count */
  EXPECT(capy_widget_autocomplete_item(ac, 50u, &got) == -1);
  capy_widget_destroy(ac);
}

static void test_autocomplete_set_zero_count_clears(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *ac;
  const char *const items[] = {"a", "b"};
  const char *got = 0;
  capy_widget_context_init(&ctx, &allocator);
  ac = capy_widget_create(&ctx, CAPY_WIDGET_AUTOCOMPLETE);
  EXPECT(ac != 0);
  EXPECT(capy_widget_set_autocomplete(ac, items, 2u) == 0);
  EXPECT(capy_widget_autocomplete_count(ac) == 2);
  /* count 0 clears even when a (ignored) pointer is passed */
  EXPECT(capy_widget_set_autocomplete(ac, items, 0u) == 0);
  EXPECT(capy_widget_autocomplete_count(ac) == 0);
  EXPECT(capy_widget_autocomplete_item(ac, 0u, &got) == -1);
  capy_widget_destroy(ac);
}

static void test_autocomplete_set_null_items_rejected(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *ac;
  const char *const items[] = {"keep"};
  capy_widget_context_init(&ctx, &allocator);
  ac = capy_widget_create(&ctx, CAPY_WIDGET_AUTOCOMPLETE);
  EXPECT(ac != 0);
  EXPECT(capy_widget_set_autocomplete(ac, items, 1u) == 0); /* known-good */
  EXPECT(capy_widget_set_autocomplete(ac, 0, 3u) == -1);    /* NULL + count>0 */
  /* fail-closed: previous model intact */
  EXPECT(capy_widget_autocomplete_count(ac) == 1);
  capy_widget_destroy(ac);
}

static void test_autocomplete_clear(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *ac;
  const char *const items[] = {"a", "b"};
  const char *got = 0;
  int32_t sel = 0;
  capy_widget_context_init(&ctx, &allocator);
  ac = capy_widget_create(&ctx, CAPY_WIDGET_AUTOCOMPLETE);
  EXPECT(ac != 0);
  EXPECT(capy_widget_set_autocomplete(ac, items, 2u) == 0);
  EXPECT(capy_widget_set_autocomplete_selected(ac, 1) == 0);
  EXPECT(capy_widget_clear_autocomplete(ac) == 0);
  EXPECT(capy_widget_autocomplete_count(ac) == 0);
  EXPECT(capy_widget_autocomplete_item(ac, 0u, &got) == -1);
  EXPECT(capy_widget_get_autocomplete_selected(ac, &sel) == 0 && sel == -1);
  capy_widget_destroy(ac);
}

static void test_autocomplete_selection_clamp(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *ac;
  const char *const three[] = {"alpha", "beta", "gamma"};
  const char *const one[] = {"solo"};
  int32_t sel = 99;
  capy_widget_context_init(&ctx, &allocator);
  ac = capy_widget_create(&ctx, CAPY_WIDGET_AUTOCOMPLETE);
  EXPECT(ac != 0);
  EXPECT(capy_widget_set_autocomplete(ac, three, 3u) == 0);
  /* fresh list -> nothing selected */
  EXPECT(capy_widget_get_autocomplete_selected(ac, &sel) == 0 && sel == -1);
  /* select a valid entry */
  EXPECT(capy_widget_set_autocomplete_selected(ac, 1) == 0);
  EXPECT(capy_widget_get_autocomplete_selected(ac, &sel) == 1 && sel == 1);
  /* clear selection with -1 */
  EXPECT(capy_widget_set_autocomplete_selected(ac, -1) == 0);
  EXPECT(capy_widget_get_autocomplete_selected(ac, &sel) == 0 && sel == -1);
  /* out-of-range rejected (>= count and < -1) */
  EXPECT(capy_widget_set_autocomplete_selected(ac, 3) == -1);
  EXPECT(capy_widget_set_autocomplete_selected(ac, -2) == -1);
  /* select, then swap to a shorter list -> selection resets to none */
  EXPECT(capy_widget_set_autocomplete_selected(ac, 2) == 0);
  EXPECT(capy_widget_set_autocomplete(ac, one, 1u) == 0);
  EXPECT(capy_widget_get_autocomplete_selected(ac, &sel) == 0 && sel == -1);
  capy_widget_destroy(ac);
}

static void test_autocomplete_wrong_widget_type_rejected(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *btn;
  const char *const items[] = {"a"};
  const char *got = 0;
  int32_t sel = 0;
  capy_widget_context_init(&ctx, &allocator);
  btn = capy_widget_create(&ctx, CAPY_WIDGET_BUTTON);
  EXPECT(btn != 0);
  EXPECT(capy_widget_set_autocomplete(btn, items, 1u) == -1);
  EXPECT(capy_widget_clear_autocomplete(btn) == -1);
  EXPECT(capy_widget_autocomplete_count(btn) == -1);
  EXPECT(capy_widget_autocomplete_item(btn, 0u, &got) == -1);
  EXPECT(capy_widget_set_autocomplete_selected(btn, 0) == -1);
  EXPECT(capy_widget_get_autocomplete_selected(btn, &sel) == -1);
  capy_widget_destroy(btn);
}

static void test_autocomplete_null_guards(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *ac;
  const char *const items[] = {"a"};
  const char *got = 0;
  int32_t sel = 0;
  capy_widget_context_init(&ctx, &allocator);
  ac = capy_widget_create(&ctx, CAPY_WIDGET_AUTOCOMPLETE);
  EXPECT(ac != 0);
  EXPECT(capy_widget_set_autocomplete(0, items, 1u) == -1);
  EXPECT(capy_widget_clear_autocomplete(0) == -1);
  EXPECT(capy_widget_autocomplete_count(0) == -1);
  EXPECT(capy_widget_autocomplete_item(0, 0u, &got) == -1);
  EXPECT(capy_widget_set_autocomplete_selected(0, 0) == -1);
  EXPECT(capy_widget_get_autocomplete_selected(0, &sel) == -1);
  EXPECT(capy_widget_set_autocomplete(ac, items, 1u) == 0);
  EXPECT(capy_widget_autocomplete_item(ac, 0u, 0) == -1);     /* NULL out_item */
  EXPECT(capy_widget_get_autocomplete_selected(ac, 0) == -1); /* NULL out_index */
  capy_widget_destroy(ac);
}

/* ── v2.18: advanced widget state — tree hierarchy (since 2.18.0) ───────── */

static void test_tree_collapsed_default_expanded(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *node;
  capy_widget_context_init(&ctx, &allocator);
  node = capy_widget_create(&ctx, CAPY_WIDGET_TREE);
  EXPECT(node != 0);
  /* fresh TREE node is expanded (collapsed flag 0) and depth 0 */
  EXPECT(capy_widget_tree_is_collapsed(node) == 0);
  EXPECT(capy_widget_tree_depth(node) == 0);
  capy_widget_destroy(node);
}

static void test_tree_set_collapsed_toggle(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *node;
  capy_widget_context_init(&ctx, &allocator);
  node = capy_widget_create(&ctx, CAPY_WIDGET_TREE);
  EXPECT(node != 0);
  EXPECT(capy_widget_set_tree_collapsed(node, 1) == 0);
  EXPECT(capy_widget_tree_is_collapsed(node) == 1);
  EXPECT(capy_widget_set_tree_collapsed(node, 0) == 0);
  EXPECT(capy_widget_tree_is_collapsed(node) == 0);
  /* any nonzero normalises to 1 */
  EXPECT(capy_widget_set_tree_collapsed(node, 42) == 0);
  EXPECT(capy_widget_tree_is_collapsed(node) == 1);
  capy_widget_destroy(node);
}

static void test_tree_depth_set_and_get(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *node;
  capy_widget_context_init(&ctx, &allocator);
  node = capy_widget_create(&ctx, CAPY_WIDGET_TREE);
  EXPECT(node != 0);
  EXPECT(capy_widget_tree_depth(node) == 0);
  EXPECT(capy_widget_set_tree_depth(node, 3u) == 0);
  EXPECT(capy_widget_tree_depth(node) == 3);
  EXPECT(capy_widget_set_tree_depth(node, 0u) == 0);
  EXPECT(capy_widget_tree_depth(node) == 0);
  capy_widget_destroy(node);
}

static void test_tree_row_visible_no_ancestors(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *node;
  capy_widget_context_init(&ctx, &allocator);
  node = capy_widget_create(&ctx, CAPY_WIDGET_TREE);
  EXPECT(node != 0);
  /* no parent -> always visible, even when the node itself is collapsed */
  EXPECT(capy_widget_tree_row_visible(node) == 1);
  EXPECT(capy_widget_set_tree_collapsed(node, 1) == 0);
  EXPECT(capy_widget_tree_row_visible(node) == 1);
  capy_widget_destroy(node);
}

static void test_tree_row_visible_collapsed_ancestor(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *root;
  struct capy_widget *child;
  capy_widget_context_init(&ctx, &allocator);
  root = capy_widget_create(&ctx, CAPY_WIDGET_TREE);
  child = capy_widget_create(&ctx, CAPY_WIDGET_TREE);
  EXPECT(root != 0 && child != 0);
  EXPECT(capy_widget_add_child(root, child) == 0);
  /* root expanded by default -> child visible */
  EXPECT(capy_widget_tree_row_visible(child) == 1);
  EXPECT(capy_widget_set_tree_collapsed(root, 1) == 0);
  EXPECT(capy_widget_tree_row_visible(child) == 0);
  EXPECT(capy_widget_set_tree_collapsed(root, 0) == 0);
  EXPECT(capy_widget_tree_row_visible(child) == 1);
  capy_widget_destroy(root);
}

static void test_tree_row_visible_nested(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *gp;
  struct capy_widget *parent;
  struct capy_widget *leaf;
  capy_widget_context_init(&ctx, &allocator);
  gp = capy_widget_create(&ctx, CAPY_WIDGET_TREE);
  parent = capy_widget_create(&ctx, CAPY_WIDGET_TREE);
  leaf = capy_widget_create(&ctx, CAPY_WIDGET_TREE);
  EXPECT(gp != 0 && parent != 0 && leaf != 0);
  EXPECT(capy_widget_add_child(gp, parent) == 0);
  EXPECT(capy_widget_add_child(parent, leaf) == 0);
  EXPECT(capy_widget_tree_row_visible(leaf) == 1);
  /* collapsing the grandparent hides everything below it */
  EXPECT(capy_widget_set_tree_collapsed(gp, 1) == 0);
  EXPECT(capy_widget_tree_row_visible(leaf) == 0);
  EXPECT(capy_widget_tree_row_visible(parent) == 0);
  /* expand gp, collapse parent: leaf hidden, parent itself still visible */
  EXPECT(capy_widget_set_tree_collapsed(gp, 0) == 0);
  EXPECT(capy_widget_set_tree_collapsed(parent, 1) == 0);
  EXPECT(capy_widget_tree_row_visible(leaf) == 0);
  EXPECT(capy_widget_tree_row_visible(parent) == 1);
  capy_widget_destroy(gp);
}

static void test_tree_wrong_widget_type_rejected(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *btn;
  capy_widget_context_init(&ctx, &allocator);
  btn = capy_widget_create(&ctx, CAPY_WIDGET_BUTTON);
  EXPECT(btn != 0);
  EXPECT(capy_widget_set_tree_collapsed(btn, 1) == -1);
  EXPECT(capy_widget_tree_is_collapsed(btn) == -1);
  EXPECT(capy_widget_set_tree_depth(btn, 1u) == -1);
  EXPECT(capy_widget_tree_depth(btn) == -1);
  EXPECT(capy_widget_tree_row_visible(btn) == -1);
  capy_widget_destroy(btn);
}

static void test_tree_null_guards(void) {
  EXPECT(capy_widget_set_tree_collapsed(0, 1) == -1);
  EXPECT(capy_widget_tree_is_collapsed(0) == -1);
  EXPECT(capy_widget_set_tree_depth(0, 1u) == -1);
  EXPECT(capy_widget_tree_depth(0) == -1);
  EXPECT(capy_widget_tree_row_visible(0) == -1);
}

/* ── v2.19: advanced widget state — chart dataset (since 2.19.0) ─────────── */

static void test_chart_set_and_query(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *chart;
  const int32_t values[3] = {10, 20, 30};
  int32_t got = 0;
  capy_widget_context_init(&ctx, &allocator);
  chart = capy_widget_create(&ctx, CAPY_WIDGET_CHART);
  EXPECT(chart != 0);
  EXPECT(capy_widget_set_chart_data(chart, values, 3u) == 0);
  EXPECT(capy_widget_chart_count(chart) == 3);
  EXPECT(capy_widget_chart_value(chart, 0u, &got) == 0 && got == 10);
  EXPECT(capy_widget_chart_value(chart, 2u, &got) == 0 && got == 30);
  capy_widget_destroy(chart);
}

static void test_chart_value_out_of_range(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *chart;
  const int32_t values[2] = {1, 2};
  int32_t got = 0;
  capy_widget_context_init(&ctx, &allocator);
  chart = capy_widget_create(&ctx, CAPY_WIDGET_CHART);
  EXPECT(chart != 0);
  EXPECT(capy_widget_chart_value(chart, 0u, &got) == -1); /* no data yet */
  EXPECT(capy_widget_set_chart_data(chart, values, 2u) == 0);
  EXPECT(capy_widget_chart_value(chart, 2u, &got) == -1); /* index == count */
  EXPECT(capy_widget_chart_value(chart, 99u, &got) == -1);
  capy_widget_destroy(chart);
}

static void test_chart_set_zero_count_clears(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *chart;
  const int32_t values[2] = {7, 8};
  int32_t got = 0;
  capy_widget_context_init(&ctx, &allocator);
  chart = capy_widget_create(&ctx, CAPY_WIDGET_CHART);
  EXPECT(chart != 0);
  EXPECT(capy_widget_set_chart_data(chart, values, 2u) == 0);
  EXPECT(capy_widget_chart_count(chart) == 2);
  /* count 0 clears even when a (ignored) pointer is passed */
  EXPECT(capy_widget_set_chart_data(chart, values, 0u) == 0);
  EXPECT(capy_widget_chart_count(chart) == 0);
  EXPECT(capy_widget_chart_value(chart, 0u, &got) == -1);
  capy_widget_destroy(chart);
}

static void test_chart_set_null_values_rejected(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *chart;
  const int32_t values[1] = {99};
  capy_widget_context_init(&ctx, &allocator);
  chart = capy_widget_create(&ctx, CAPY_WIDGET_CHART);
  EXPECT(chart != 0);
  EXPECT(capy_widget_set_chart_data(chart, values, 1u) == 0); /* known-good */
  EXPECT(capy_widget_set_chart_data(chart, 0, 3u) == -1);     /* NULL + count>0 */
  /* fail-closed: previous dataset intact */
  EXPECT(capy_widget_chart_count(chart) == 1);
  capy_widget_destroy(chart);
}

static void test_chart_clear(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *chart;
  const int32_t values[2] = {3, 4};
  int32_t got = 0;
  int32_t lo = 123;
  int32_t hi = 456;
  capy_widget_context_init(&ctx, &allocator);
  chart = capy_widget_create(&ctx, CAPY_WIDGET_CHART);
  EXPECT(chart != 0);
  EXPECT(capy_widget_set_chart_data(chart, values, 2u) == 0);
  EXPECT(capy_widget_clear_chart_data(chart) == 0);
  EXPECT(capy_widget_chart_count(chart) == 0);
  EXPECT(capy_widget_chart_value(chart, 0u, &got) == -1);
  /* range on empty dataset: returns 0 and zeroes the outputs */
  EXPECT(capy_widget_chart_range(chart, &lo, &hi) == 0 && lo == 0 && hi == 0);
  capy_widget_destroy(chart);
}

static void test_chart_range_minmax(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *chart;
  const int32_t mixed[4] = {5, -3, 42, 17};
  const int32_t single[1] = {-9};
  int32_t lo = 0;
  int32_t hi = 0;
  capy_widget_context_init(&ctx, &allocator);
  chart = capy_widget_create(&ctx, CAPY_WIDGET_CHART);
  EXPECT(chart != 0);
  /* signed min/max across positives and negatives */
  EXPECT(capy_widget_set_chart_data(chart, mixed, 4u) == 0);
  EXPECT(capy_widget_chart_range(chart, &lo, &hi) == 1 && lo == -3 && hi == 42);
  /* single element: min == max */
  EXPECT(capy_widget_set_chart_data(chart, single, 1u) == 0);
  EXPECT(capy_widget_chart_range(chart, &lo, &hi) == 1 && lo == -9 && hi == -9);
  capy_widget_destroy(chart);
}

static void test_chart_wrong_widget_type_rejected(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *btn;
  const int32_t values[1] = {1};
  int32_t got = 0;
  int32_t lo = 0;
  int32_t hi = 0;
  capy_widget_context_init(&ctx, &allocator);
  btn = capy_widget_create(&ctx, CAPY_WIDGET_BUTTON);
  EXPECT(btn != 0);
  EXPECT(capy_widget_set_chart_data(btn, values, 1u) == -1);
  EXPECT(capy_widget_clear_chart_data(btn) == -1);
  EXPECT(capy_widget_chart_count(btn) == -1);
  EXPECT(capy_widget_chart_value(btn, 0u, &got) == -1);
  EXPECT(capy_widget_chart_range(btn, &lo, &hi) == -1);
  capy_widget_destroy(btn);
}

static void test_chart_null_guards(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *chart;
  const int32_t values[1] = {1};
  int32_t got = 0;
  int32_t lo = 0;
  capy_widget_context_init(&ctx, &allocator);
  chart = capy_widget_create(&ctx, CAPY_WIDGET_CHART);
  EXPECT(chart != 0);
  EXPECT(capy_widget_set_chart_data(0, values, 1u) == -1);
  EXPECT(capy_widget_clear_chart_data(0) == -1);
  EXPECT(capy_widget_chart_count(0) == -1);
  EXPECT(capy_widget_chart_value(0, 0u, &got) == -1);
  EXPECT(capy_widget_chart_range(0, &lo, &lo) == -1);
  EXPECT(capy_widget_set_chart_data(chart, values, 1u) == 0);
  EXPECT(capy_widget_chart_value(chart, 0u, 0) == -1);    /* NULL out_value */
  EXPECT(capy_widget_chart_range(chart, 0, &lo) == -1);   /* NULL out_min */
  EXPECT(capy_widget_chart_range(chart, &lo, 0) == -1);   /* NULL out_max */
  capy_widget_destroy(chart);
}

/* ── v2.20: advanced widget state — rich-text ranges (since 2.20.0) ──────── */

static void test_rich_text_set_and_query(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *rt;
  const struct capy_text_range ranges[3] = {
      {0u, 5u, CAPY_TEXT_STYLE_BOLD, 0u, 0xFFFF0000u},
      {5u, 3u, CAPY_TEXT_STYLE_ITALIC, 0u, 0u},
      {8u, 4u, CAPY_TEXT_STYLE_UNDERLINE | CAPY_TEXT_STYLE_BOLD, 0u, 0xFF00FF00u}};
  struct capy_text_range got;
  capy_widget_context_init(&ctx, &allocator);
  rt = capy_widget_create(&ctx, CAPY_WIDGET_RICH_TEXT);
  EXPECT(rt != 0);
  EXPECT(capy_widget_set_rich_text_ranges(rt, ranges, 3u) == 0);
  EXPECT(capy_widget_rich_text_range_count(rt) == 3);
  EXPECT(capy_widget_rich_text_range(rt, 0u, &got) == 0 && got.start == 0u &&
         got.length == 5u && got.flags == CAPY_TEXT_STYLE_BOLD &&
         got.color == 0xFFFF0000u);
  EXPECT(capy_widget_rich_text_range(rt, 2u, &got) == 0 && got.start == 8u &&
         got.length == 4u &&
         got.flags == (CAPY_TEXT_STYLE_UNDERLINE | CAPY_TEXT_STYLE_BOLD));
  capy_widget_destroy(rt);
}

static void test_rich_text_range_out_of_range(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *rt;
  const struct capy_text_range ranges[2] = {
      {0u, 4u, CAPY_TEXT_STYLE_BOLD, 0u, 0u},
      {4u, 4u, CAPY_TEXT_STYLE_ITALIC, 0u, 0u}};
  struct capy_text_range got;
  capy_widget_context_init(&ctx, &allocator);
  rt = capy_widget_create(&ctx, CAPY_WIDGET_RICH_TEXT);
  EXPECT(rt != 0);
  EXPECT(capy_widget_rich_text_range(rt, 0u, &got) == -1); /* no data yet */
  EXPECT(capy_widget_set_rich_text_ranges(rt, ranges, 2u) == 0);
  EXPECT(capy_widget_rich_text_range(rt, 2u, &got) == -1); /* index == count */
  EXPECT(capy_widget_rich_text_range(rt, 99u, &got) == -1);
  capy_widget_destroy(rt);
}

static void test_rich_text_set_zero_count_clears(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *rt;
  const struct capy_text_range ranges[2] = {
      {0u, 3u, CAPY_TEXT_STYLE_BOLD, 0u, 0u},
      {3u, 3u, CAPY_TEXT_STYLE_ITALIC, 0u, 0u}};
  struct capy_text_range got;
  capy_widget_context_init(&ctx, &allocator);
  rt = capy_widget_create(&ctx, CAPY_WIDGET_RICH_TEXT);
  EXPECT(rt != 0);
  EXPECT(capy_widget_set_rich_text_ranges(rt, ranges, 2u) == 0);
  EXPECT(capy_widget_rich_text_range_count(rt) == 2);
  /* count 0 clears even when a (ignored) pointer is passed */
  EXPECT(capy_widget_set_rich_text_ranges(rt, ranges, 0u) == 0);
  EXPECT(capy_widget_rich_text_range_count(rt) == 0);
  EXPECT(capy_widget_rich_text_range(rt, 0u, &got) == -1);
  capy_widget_destroy(rt);
}

static void test_rich_text_set_null_ranges_rejected(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *rt;
  const struct capy_text_range ranges[1] = {{0u, 2u, CAPY_TEXT_STYLE_BOLD, 0u, 0u}};
  capy_widget_context_init(&ctx, &allocator);
  rt = capy_widget_create(&ctx, CAPY_WIDGET_RICH_TEXT);
  EXPECT(rt != 0);
  EXPECT(capy_widget_set_rich_text_ranges(rt, ranges, 1u) == 0); /* known-good */
  EXPECT(capy_widget_set_rich_text_ranges(rt, 0, 3u) == -1);     /* NULL + count>0 */
  /* fail-closed: previous run array intact */
  EXPECT(capy_widget_rich_text_range_count(rt) == 1);
  capy_widget_destroy(rt);
}

static void test_rich_text_clear(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *rt;
  const struct capy_text_range ranges[2] = {
      {0u, 2u, CAPY_TEXT_STYLE_BOLD, 0u, 0u},
      {2u, 2u, CAPY_TEXT_STYLE_ITALIC, 0u, 0u}};
  struct capy_text_range got;
  capy_widget_context_init(&ctx, &allocator);
  rt = capy_widget_create(&ctx, CAPY_WIDGET_RICH_TEXT);
  EXPECT(rt != 0);
  EXPECT(capy_widget_set_rich_text_ranges(rt, ranges, 2u) == 0);
  EXPECT(capy_widget_clear_rich_text_ranges(rt) == 0);
  EXPECT(capy_widget_rich_text_range_count(rt) == 0);
  EXPECT(capy_widget_rich_text_range(rt, 0u, &got) == -1);
  /* range_at on empty: returns 0 and zeroes the output */
  got.flags = 0x55u;
  EXPECT(capy_widget_rich_text_range_at(rt, 0u, &got) == 0 && got.start == 0u &&
         got.length == 0u && got.flags == 0u);
  capy_widget_destroy(rt);
}

static void test_rich_text_range_at_lookup(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *rt;
  const struct capy_text_range ranges[4] = {
      {0u, 5u, CAPY_TEXT_STYLE_BOLD, 0u, 0u},      /* [0,5)            */
      {5u, 5u, CAPY_TEXT_STYLE_ITALIC, 0u, 0u},    /* [5,10)           */
      {3u, 4u, CAPY_TEXT_STYLE_UNDERLINE, 0u, 0u}, /* [3,7) overlaps   */
      {20u, 0u, CAPY_TEXT_STYLE_BOLD, 0u, 0u}};    /* zero-len: covers nothing */
  struct capy_text_range got;
  capy_widget_context_init(&ctx, &allocator);
  rt = capy_widget_create(&ctx, CAPY_WIDGET_RICH_TEXT);
  EXPECT(rt != 0);
  EXPECT(capy_widget_set_rich_text_ranges(rt, ranges, 4u) == 0);
  /* offset only inside the first run */
  EXPECT(capy_widget_rich_text_range_at(rt, 1u, &got) == 1 &&
         got.flags == CAPY_TEXT_STYLE_BOLD);
  /* offset inside run 0 and run 2: later run (index 2) overrides */
  EXPECT(capy_widget_rich_text_range_at(rt, 4u, &got) == 1 &&
         got.flags == CAPY_TEXT_STYLE_UNDERLINE && got.start == 3u);
  /* offset only inside the second run */
  EXPECT(capy_widget_rich_text_range_at(rt, 9u, &got) == 1 &&
         got.flags == CAPY_TEXT_STYLE_ITALIC);
  /* gap: nothing covers offset 15 -> 0, output zeroed */
  got.flags = 0xAAu;
  EXPECT(capy_widget_rich_text_range_at(rt, 15u, &got) == 0 && got.start == 0u &&
         got.length == 0u && got.flags == 0u);
  /* zero-length run at 20 covers nothing */
  EXPECT(capy_widget_rich_text_range_at(rt, 20u, &got) == 0);
  capy_widget_destroy(rt);
}

static void test_rich_text_wrong_widget_type_rejected(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *btn;
  const struct capy_text_range ranges[1] = {{0u, 1u, CAPY_TEXT_STYLE_BOLD, 0u, 0u}};
  struct capy_text_range got;
  capy_widget_context_init(&ctx, &allocator);
  btn = capy_widget_create(&ctx, CAPY_WIDGET_BUTTON);
  EXPECT(btn != 0);
  EXPECT(capy_widget_set_rich_text_ranges(btn, ranges, 1u) == -1);
  EXPECT(capy_widget_clear_rich_text_ranges(btn) == -1);
  EXPECT(capy_widget_rich_text_range_count(btn) == -1);
  EXPECT(capy_widget_rich_text_range(btn, 0u, &got) == -1);
  EXPECT(capy_widget_rich_text_range_at(btn, 0u, &got) == -1);
  capy_widget_destroy(btn);
}

static void test_rich_text_null_guards(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *rt;
  const struct capy_text_range ranges[1] = {{0u, 1u, CAPY_TEXT_STYLE_BOLD, 0u, 0u}};
  struct capy_text_range got;
  capy_widget_context_init(&ctx, &allocator);
  rt = capy_widget_create(&ctx, CAPY_WIDGET_RICH_TEXT);
  EXPECT(rt != 0);
  EXPECT(capy_widget_set_rich_text_ranges(0, ranges, 1u) == -1);
  EXPECT(capy_widget_clear_rich_text_ranges(0) == -1);
  EXPECT(capy_widget_rich_text_range_count(0) == -1);
  EXPECT(capy_widget_rich_text_range(0, 0u, &got) == -1);
  EXPECT(capy_widget_rich_text_range_at(0, 0u, &got) == -1);
  EXPECT(capy_widget_set_rich_text_ranges(rt, ranges, 1u) == 0);
  EXPECT(capy_widget_rich_text_range(rt, 0u, 0) == -1);    /* NULL out_range */
  EXPECT(capy_widget_rich_text_range_at(rt, 0u, 0) == -1); /* NULL out_range */
  capy_widget_destroy(rt);
}

/* ── v2.21: advanced widget state — canvas draw callback (since 2.21.0) ───── */

struct canvas_probe {
  int calls;
  uint32_t seen_width;
  void *seen_user_data;
  int return_value;
};

static int canvas_probe_draw(struct capy_widget *w, struct capy_display_list *dl,
                             void *user_data) {
  struct canvas_probe *p = (struct canvas_probe *)user_data;
  if (p) {
    p->calls += 1;
    p->seen_width = w ? w->bounds.width : 0u;
    p->seen_user_data = user_data;
  }
  /* prove dl is usable from the callback: append one RECT op if there is room */
  if (dl && w && dl->count < dl->capacity) {
    dl->cmds[dl->count].op = CAPY_DL_RECT;
    dl->cmds[dl->count].rect = w->bounds;
    dl->count += 1u;
  }
  return p ? p->return_value : 0;
}

static void test_canvas_set_and_invoke(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *canvas;
  struct canvas_probe probe = {0, 0u, 0, 0};
  struct capy_dl_cmd cmd_buf[4];
  char text_buf[16];
  struct capy_display_list dl;
  capy_widget_context_init(&ctx, &allocator);
  canvas = capy_widget_create(&ctx, CAPY_WIDGET_CANVAS);
  EXPECT(canvas != 0);
  capy_widget_set_bounds(canvas, 0, 0, 50u, 30u);
  capy_display_list_init(&dl, cmd_buf, 4u, text_buf, 16u);
  EXPECT(capy_widget_set_canvas_draw(canvas, canvas_probe_draw, &probe) == 0);
  EXPECT(capy_widget_has_canvas_draw(canvas) == 1);
  EXPECT(capy_widget_canvas_draw(canvas, &dl) == 0);
  EXPECT(probe.calls == 1);
  EXPECT(probe.seen_width == 50u);
  EXPECT(probe.seen_user_data == &probe);
  /* the callback appended one RECT op carrying the widget bounds */
  EXPECT(dl.count == 1u);
  EXPECT(dl.cmds[0].op == CAPY_DL_RECT);
  EXPECT(dl.cmds[0].rect.width == 50u);
  capy_widget_destroy(canvas);
}

static void test_canvas_callback_failure_propagates(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *canvas;
  struct canvas_probe probe = {0, 0u, 0, 7}; /* callback returns non-zero */
  struct capy_dl_cmd cmd_buf[4];
  char text_buf[16];
  struct capy_display_list dl;
  capy_widget_context_init(&ctx, &allocator);
  canvas = capy_widget_create(&ctx, CAPY_WIDGET_CANVAS);
  EXPECT(canvas != 0);
  capy_widget_set_bounds(canvas, 0, 0, 10u, 10u);
  capy_display_list_init(&dl, cmd_buf, 4u, text_buf, 16u);
  EXPECT(capy_widget_set_canvas_draw(canvas, canvas_probe_draw, &probe) == 0);
  /* non-zero callback result is normalised to -1 */
  EXPECT(capy_widget_canvas_draw(canvas, &dl) == -1);
  EXPECT(probe.calls == 1);
  capy_widget_destroy(canvas);
}

static void test_canvas_clear(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *canvas;
  struct canvas_probe probe = {0, 0u, 0, 0};
  struct capy_dl_cmd cmd_buf[4];
  char text_buf[16];
  struct capy_display_list dl;
  void *ud = &probe;
  capy_widget_context_init(&ctx, &allocator);
  canvas = capy_widget_create(&ctx, CAPY_WIDGET_CANVAS);
  EXPECT(canvas != 0);
  capy_display_list_init(&dl, cmd_buf, 4u, text_buf, 16u);
  EXPECT(capy_widget_set_canvas_draw(canvas, canvas_probe_draw, &probe) == 0);
  EXPECT(capy_widget_has_canvas_draw(canvas) == 1);
  EXPECT(capy_widget_clear_canvas_draw(canvas) == 0);
  EXPECT(capy_widget_has_canvas_draw(canvas) == 0);
  /* no callback set -> invoke fails closed, user_data resets to NULL */
  EXPECT(capy_widget_canvas_draw(canvas, &dl) == -1);
  EXPECT(capy_widget_canvas_user_data(canvas, &ud) == 0 && ud == 0);
  capy_widget_destroy(canvas);
}

static void test_canvas_default_unset(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *canvas;
  struct capy_dl_cmd cmd_buf[4];
  char text_buf[16];
  struct capy_display_list dl;
  void *ud = &heap; /* non-NULL sentinel: getter must overwrite it to NULL */
  capy_widget_context_init(&ctx, &allocator);
  canvas = capy_widget_create(&ctx, CAPY_WIDGET_CANVAS);
  EXPECT(canvas != 0);
  capy_display_list_init(&dl, cmd_buf, 4u, text_buf, 16u);
  /* fresh canvas: no callback */
  EXPECT(capy_widget_has_canvas_draw(canvas) == 0);
  EXPECT(capy_widget_canvas_draw(canvas, &dl) == -1);
  EXPECT(capy_widget_canvas_user_data(canvas, &ud) == 0 && ud == 0);
  capy_widget_destroy(canvas);
}

static void test_canvas_set_null_fn_clears(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *canvas;
  struct canvas_probe probe = {0, 0u, 0, 0};
  void *ud = &probe;
  capy_widget_context_init(&ctx, &allocator);
  canvas = capy_widget_create(&ctx, CAPY_WIDGET_CANVAS);
  EXPECT(canvas != 0);
  EXPECT(capy_widget_set_canvas_draw(canvas, canvas_probe_draw, &probe) == 0);
  EXPECT(capy_widget_has_canvas_draw(canvas) == 1);
  /* NULL fn clears even when a (ignored) user_data is passed */
  EXPECT(capy_widget_set_canvas_draw(canvas, 0, &probe) == 0);
  EXPECT(capy_widget_has_canvas_draw(canvas) == 0);
  EXPECT(capy_widget_canvas_user_data(canvas, &ud) == 0 && ud == 0);
  capy_widget_destroy(canvas);
}

static void test_canvas_user_data_getter(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *canvas;
  int marker = 42;
  void *ud = 0;
  capy_widget_context_init(&ctx, &allocator);
  canvas = capy_widget_create(&ctx, CAPY_WIDGET_CANVAS);
  EXPECT(canvas != 0);
  EXPECT(capy_widget_set_canvas_draw(canvas, canvas_probe_draw, &marker) == 0);
  EXPECT(capy_widget_canvas_user_data(canvas, &ud) == 0 && ud == &marker);
  capy_widget_destroy(canvas);
}

static void test_canvas_wrong_widget_type_rejected(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *btn;
  struct capy_dl_cmd cmd_buf[4];
  char text_buf[16];
  struct capy_display_list dl;
  void *ud = 0;
  capy_widget_context_init(&ctx, &allocator);
  btn = capy_widget_create(&ctx, CAPY_WIDGET_BUTTON);
  EXPECT(btn != 0);
  capy_display_list_init(&dl, cmd_buf, 4u, text_buf, 16u);
  EXPECT(capy_widget_set_canvas_draw(btn, canvas_probe_draw, 0) == -1);
  EXPECT(capy_widget_clear_canvas_draw(btn) == -1);
  EXPECT(capy_widget_has_canvas_draw(btn) == -1);
  EXPECT(capy_widget_canvas_user_data(btn, &ud) == -1);
  EXPECT(capy_widget_canvas_draw(btn, &dl) == -1);
  capy_widget_destroy(btn);
}

static void test_canvas_null_guards(void) {
  struct test_heap heap = {{0}, 0u, 0u};
  struct capy_widget_allocator allocator = {test_alloc, test_free, &heap};
  struct capy_widget_context ctx;
  struct capy_widget *canvas;
  struct canvas_probe probe = {0, 0u, 0, 0};
  struct capy_dl_cmd cmd_buf[4];
  char text_buf[16];
  struct capy_display_list dl;
  void *ud = 0;
  capy_widget_context_init(&ctx, &allocator);
  canvas = capy_widget_create(&ctx, CAPY_WIDGET_CANVAS);
  EXPECT(canvas != 0);
  capy_display_list_init(&dl, cmd_buf, 4u, text_buf, 16u);
  EXPECT(capy_widget_set_canvas_draw(0, canvas_probe_draw, 0) == -1);
  EXPECT(capy_widget_clear_canvas_draw(0) == -1);
  EXPECT(capy_widget_has_canvas_draw(0) == -1);
  EXPECT(capy_widget_canvas_user_data(0, &ud) == -1);
  EXPECT(capy_widget_canvas_user_data(canvas, 0) == -1); /* NULL out */
  EXPECT(capy_widget_canvas_draw(0, &dl) == -1);
  EXPECT(capy_widget_set_canvas_draw(canvas, canvas_probe_draw, &probe) == 0);
  EXPECT(capy_widget_canvas_draw(canvas, 0) == -1); /* NULL dl */
  capy_widget_destroy(canvas);
}

/* ── v2.22: multi-touch gestures — pinch / rotate (since 2.22.0) ──────────── */

static void test_gesture_pinch_out(void) {
  struct capy_gesture_recognizer r;
  struct capy_widget_event ev;
  struct capy_touch_payload p;
  struct capy_gesture out;
  capy_gesture_recognizer_init(&r);
  gesture_make_touch(&ev, &p, CAPY_WIDGET_EVENT_TOUCH_BEGIN, 1u, 0, 0);
  EXPECT(capy_gesture_feed(&r, &ev, 1000u) == 0);
  gesture_make_touch(&ev, &p, CAPY_WIDGET_EVENT_TOUCH_BEGIN, 2u, 100, 0);
  EXPECT(capy_gesture_feed(&r, &ev, 1010u) == 0); /* two-finger session opens */
  /* finger 2 spreads: separation 100 -> 140, delta +40 >= 20 px */
  gesture_make_touch(&ev, &p, CAPY_WIDGET_EVENT_TOUCH_MOVE, 2u, 140, 0);
  EXPECT(capy_gesture_feed(&r, &ev, 1020u) == 1);
  EXPECT(capy_gesture_poll(&r, &out) == 1);
  EXPECT(out.kind == (uint8_t)CAPY_GESTURE_PINCH_OUT);
  EXPECT(out.magnitude == 40);
}

static void test_gesture_pinch_in(void) {
  struct capy_gesture_recognizer r;
  struct capy_widget_event ev;
  struct capy_touch_payload p;
  struct capy_gesture out;
  capy_gesture_recognizer_init(&r);
  gesture_make_touch(&ev, &p, CAPY_WIDGET_EVENT_TOUCH_BEGIN, 1u, 0, 0);
  EXPECT(capy_gesture_feed(&r, &ev, 0u) == 0);
  gesture_make_touch(&ev, &p, CAPY_WIDGET_EVENT_TOUCH_BEGIN, 2u, 100, 0);
  EXPECT(capy_gesture_feed(&r, &ev, 10u) == 0);
  /* finger 2 approaches: separation 100 -> 70, delta -30, |30| >= 20 px */
  gesture_make_touch(&ev, &p, CAPY_WIDGET_EVENT_TOUCH_MOVE, 2u, 70, 0);
  EXPECT(capy_gesture_feed(&r, &ev, 20u) == 1);
  EXPECT(capy_gesture_poll(&r, &out) == 1);
  EXPECT(out.kind == (uint8_t)CAPY_GESTURE_PINCH_IN);
  EXPECT(out.magnitude == -30);
}

static void test_gesture_rotate_cw(void) {
  struct capy_gesture_recognizer r;
  struct capy_widget_event ev;
  struct capy_touch_payload p;
  struct capy_gesture out;
  capy_gesture_recognizer_init(&r);
  /* baseline vector v0 = touch1 - touch2 = (10,0) */
  gesture_make_touch(&ev, &p, CAPY_WIDGET_EVENT_TOUCH_BEGIN, 1u, 10, 0);
  EXPECT(capy_gesture_feed(&r, &ev, 0u) == 0);
  gesture_make_touch(&ev, &p, CAPY_WIDGET_EVENT_TOUCH_BEGIN, 2u, 0, 0);
  EXPECT(capy_gesture_feed(&r, &ev, 10u) == 0);
  /* rotate finger 1 right->down (screen y-down clockwise): cur vector (0,10),
   * cross = 10*10 - 0*0 = +100 > 0 -> CW; separation stays 10 (<20) so pinch
   * does not pre-empt. */
  gesture_make_touch(&ev, &p, CAPY_WIDGET_EVENT_TOUCH_MOVE, 1u, 0, 10);
  EXPECT(capy_gesture_feed(&r, &ev, 20u) == 1);
  EXPECT(capy_gesture_poll(&r, &out) == 1);
  EXPECT(out.kind == (uint8_t)CAPY_GESTURE_ROTATE_CW);
}

static void test_gesture_rotate_ccw(void) {
  struct capy_gesture_recognizer r;
  struct capy_widget_event ev;
  struct capy_touch_payload p;
  struct capy_gesture out;
  capy_gesture_recognizer_init(&r);
  gesture_make_touch(&ev, &p, CAPY_WIDGET_EVENT_TOUCH_BEGIN, 1u, 10, 0);
  EXPECT(capy_gesture_feed(&r, &ev, 0u) == 0);
  gesture_make_touch(&ev, &p, CAPY_WIDGET_EVENT_TOUCH_BEGIN, 2u, 0, 0);
  EXPECT(capy_gesture_feed(&r, &ev, 10u) == 0);
  /* rotate finger 1 right->up: cur vector (0,-10), cross = 10*(-10) = -100 < 0
   * -> CCW. */
  gesture_make_touch(&ev, &p, CAPY_WIDGET_EVENT_TOUCH_MOVE, 1u, 0, -10);
  EXPECT(capy_gesture_feed(&r, &ev, 20u) == 1);
  EXPECT(capy_gesture_poll(&r, &out) == 1);
  EXPECT(out.kind == (uint8_t)CAPY_GESTURE_ROTATE_CCW);
}

static void test_gesture_rotate_below_threshold(void) {
  struct capy_gesture_recognizer r;
  struct capy_widget_event ev;
  struct capy_touch_payload p;
  struct capy_gesture out;
  capy_gesture_recognizer_init(&r);
  /* baseline v0 = (100,0), separation 100 */
  gesture_make_touch(&ev, &p, CAPY_WIDGET_EVENT_TOUCH_BEGIN, 1u, 100, 0);
  EXPECT(capy_gesture_feed(&r, &ev, 0u) == 0);
  gesture_make_touch(&ev, &p, CAPY_WIDGET_EVENT_TOUCH_BEGIN, 2u, 0, 0);
  EXPECT(capy_gesture_feed(&r, &ev, 10u) == 0);
  /* small rotation: cur (100,20), tan = 2000/10000 = 0.2 (~11 deg) < 27/100 */
  gesture_make_touch(&ev, &p, CAPY_WIDGET_EVENT_TOUCH_MOVE, 1u, 100, 20);
  EXPECT(capy_gesture_feed(&r, &ev, 20u) == 0);
  EXPECT(capy_gesture_poll(&r, &out) == 0);
  /* larger rotation: cur (100,40), tan = 4000/10000 = 0.4 (~22 deg) > 27/100 */
  gesture_make_touch(&ev, &p, CAPY_WIDGET_EVENT_TOUCH_MOVE, 1u, 100, 40);
  EXPECT(capy_gesture_feed(&r, &ev, 30u) == 1);
  EXPECT(capy_gesture_poll(&r, &out) == 1);
  EXPECT(out.kind == (uint8_t)CAPY_GESTURE_ROTATE_CW);
}

static void test_gesture_multi_touch_end_resets(void) {
  struct capy_gesture_recognizer r;
  struct capy_widget_event ev;
  struct capy_touch_payload p;
  struct capy_gesture out;
  capy_gesture_recognizer_init(&r);
  gesture_make_touch(&ev, &p, CAPY_WIDGET_EVENT_TOUCH_BEGIN, 1u, 0, 0);
  EXPECT(capy_gesture_feed(&r, &ev, 0u) == 0);
  gesture_make_touch(&ev, &p, CAPY_WIDGET_EVENT_TOUCH_BEGIN, 2u, 100, 0);
  EXPECT(capy_gesture_feed(&r, &ev, 10u) == 0);
  gesture_make_touch(&ev, &p, CAPY_WIDGET_EVENT_TOUCH_MOVE, 2u, 140, 0);
  EXPECT(capy_gesture_feed(&r, &ev, 20u) == 1); /* PINCH_OUT */
  EXPECT(capy_gesture_poll(&r, &out) == 1);
  /* lifting finger 2 ends the session with no further emit */
  gesture_make_touch(&ev, &p, CAPY_WIDGET_EVENT_TOUCH_END, 2u, 140, 0);
  EXPECT(capy_gesture_feed(&r, &ev, 30u) == 0);
  /* the still-down finger 1 lifting is now a no-op (no spurious TAP) */
  gesture_make_touch(&ev, &p, CAPY_WIDGET_EVENT_TOUCH_END, 1u, 0, 0);
  EXPECT(capy_gesture_feed(&r, &ev, 40u) == 0);
  EXPECT(capy_gesture_poll(&r, &out) == 0);
  /* a fresh single-finger tap works again after the session reset */
  gesture_make_touch(&ev, &p, CAPY_WIDGET_EVENT_TOUCH_BEGIN, 5u, 5, 5);
  EXPECT(capy_gesture_feed(&r, &ev, 100u) == 0);
  gesture_make_touch(&ev, &p, CAPY_WIDGET_EVENT_TOUCH_END, 5u, 5, 5);
  EXPECT(capy_gesture_feed(&r, &ev, 120u) == 1);
  EXPECT(capy_gesture_poll(&r, &out) == 1);
  EXPECT(out.kind == (uint8_t)CAPY_GESTURE_TAP);
}

static void test_gesture_single_touch_swipe_regression(void) {
  struct capy_gesture_recognizer r;
  struct capy_widget_event ev;
  struct capy_touch_payload p;
  struct capy_gesture out;
  capy_gesture_recognizer_init(&r);
  /* one finger only: the restructured feed must still produce a swipe */
  gesture_make_touch(&ev, &p, CAPY_WIDGET_EVENT_TOUCH_BEGIN, 1u, 100, 100);
  EXPECT(capy_gesture_feed(&r, &ev, 0u) == 0);
  gesture_make_touch(&ev, &p, CAPY_WIDGET_EVENT_TOUCH_END, 1u, 170, 100);
  EXPECT(capy_gesture_feed(&r, &ev, 30u) == 1);
  EXPECT(capy_gesture_poll(&r, &out) == 1);
  EXPECT(out.kind == (uint8_t)CAPY_GESTURE_SWIPE_RIGHT);
  EXPECT(out.magnitude == 70);
}

static void test_gesture_third_finger_ignored(void) {
  struct capy_gesture_recognizer r;
  struct capy_widget_event ev;
  struct capy_touch_payload p;
  struct capy_gesture out;
  capy_gesture_recognizer_init(&r);
  gesture_make_touch(&ev, &p, CAPY_WIDGET_EVENT_TOUCH_BEGIN, 1u, 0, 0);
  EXPECT(capy_gesture_feed(&r, &ev, 0u) == 0);
  gesture_make_touch(&ev, &p, CAPY_WIDGET_EVENT_TOUCH_BEGIN, 2u, 100, 0);
  EXPECT(capy_gesture_feed(&r, &ev, 10u) == 0);
  /* a third concurrent finger is ignored; the session keeps fingers 1 and 2 */
  gesture_make_touch(&ev, &p, CAPY_WIDGET_EVENT_TOUCH_BEGIN, 3u, 50, 50);
  EXPECT(capy_gesture_feed(&r, &ev, 15u) == 0);
  gesture_make_touch(&ev, &p, CAPY_WIDGET_EVENT_TOUCH_MOVE, 2u, 140, 0);
  EXPECT(capy_gesture_feed(&r, &ev, 20u) == 1);
  EXPECT(capy_gesture_poll(&r, &out) == 1);
  EXPECT(out.kind == (uint8_t)CAPY_GESTURE_PINCH_OUT);
}

int main(void) {
  test_create_find_and_click();
  test_disabled_widget_ignores_input();
  test_layout_stack_equal_grow();
  test_layout_grid_2x2();
  test_layout_idempotent();
  test_layout_min_max_constraints();
  test_displaylist_emit_simple_tree();
  test_displaylist_clip_balance();
  test_displaylist_overflow_rollback();
  test_displaylist_reset_and_text();
  test_focus_defaults_and_set();
  test_focus_traversal_order();
  test_focus_tab_index_priority();
  test_focus_skip_non_traversable();
  test_focus_prev_wrap();
  test_dispatch_key_tab_cycles();
  test_dispatch_key_enter_button();
  test_dispatch_key_space_checkbox();
  test_dispatch_key_escape_dialog();
  test_displaylist_focus_ring();
  test_text_edit_insert_basic();
  test_text_edit_insert_middle_utf8();
  test_text_edit_insert_overflow();
  test_text_edit_delete_selection();
  test_text_edit_backspace_utf8();
  test_displaylist_password_mode();
  test_text_edit_readonly_blocks_insert();
  test_text_edit_ime_stub();
  test_text_edit_utf8_invalid_rejected();
  test_text_edit_determinism();
  test_anim_sample_start_returns_from();
  test_anim_sample_end_returns_to();
  test_anim_sample_inactive_returns_from();
  test_anim_linear_midpoint();
  test_anim_easings_monotonic_and_endpoints();
  test_anim_now_rewound_stable();
  test_anim_zero_duration_returns_from();
  test_anim_multiple_independent();
  test_anim_determinism();
  test_widget_tick_walks_tree();
  test_theme_defaults_snapshot();
  test_theme_apply_stores_in_context();
  test_theme_resolve();
  test_theme_high_contrast_meets_wcag_aa();
  test_displaylist_uses_theme_when_token_set();
  test_displaylist_backward_compat_with_literal();
  test_displaylist_theme_change_preserves_structure();
  test_displaylist_diff_identical_zero_rects();
  test_displaylist_diff_button_color_change();
  test_displaylist_diff_coalesces_adjacent();
  test_displaylist_diff_overflow_returns_minus_one();
  test_displaylist_diff_determinism();
  test_displaylist_dirty_hint_op_not_emitted();
  test_text_metrics_fallback_deterministic();
  test_text_metrics_hook_called_when_set();
  test_text_metrics_hook_cleared_restores_fallback();
  test_text_metrics_hook_failure_falls_back();
  test_displaylist_emits_font_id();
  test_displaylist_font_id_default_zero();
  test_event_wheel_on_list_scrolls();
  test_event_wheel_clamps_to_range();
  test_event_wheel_on_panel_returns_unhandled();
  test_event_touch_single_routes_as_pointer();
  test_event_touch_uses_payload_position_over_event_xy();
  test_event_gamepad_no_crash();
  test_event_unknown_type_ignored();
  test_event_modifier_flags_distinct();
  test_a11y_snapshot_basic();
  test_a11y_snapshot_hierarchy();
  test_a11y_roles_cover_focusable_types();
  test_a11y_widget_id_stable_across_snapshots();
  test_a11y_state_flags_match_widget_state();
  test_a11y_overflow_partial_fill_returns_minus_one();
  test_a11y_label_untitled_fallback();
  test_locale_default_seeds_context();
  test_locale_set_null_restores_default();
  test_locale_plural_en_pt();
  test_locale_plural_ar();
  test_locale_plural_ru();
  test_locale_format_subset();
  test_locale_format_overflow_returns_minus_one();
  test_layout_rtl_mirrors_horizontal_stack();
  test_layout_rtl_vertical_stack_x_unchanged();
  test_theme_serialize_round_trip();
  test_theme_serialize_determinism();
  test_theme_serialize_overflow_returns_minus_one();
  test_theme_deserialize_rejects_future_version();
  test_theme_deserialize_rejects_malformed();
  test_theme_deserialize_ignores_unknown_keys();
  test_theme_deserialize_tolerates_whitespace_and_comments();
  test_pool_sequential_allocations();
  test_pool_overflow_returns_null();
  test_pool_reset_keeps_high_water();
  test_pool_alignment_16();
  test_pool_backs_widget_create();
  test_layout_measure_cache_short_circuit();
  test_invalidate_walks_up_parents();
  test_v1_freeze_markers();
  test_invalidate_bumps_dirty_version();
  test_invalidate_subtree_walks_down();
  test_dirty_version_accessor();
  test_frame_budget_round_trip();
  test_diff_incremental_matches_diff_identical();
  test_diff_incremental_matches_diff_trailing_change();
  test_diff_incremental_overflow_returns_minus_one();
  test_gpu_rect_to_single_quad();
  test_gpu_border_to_four_quads();
  test_gpu_clip_intersects_quads();
  test_gpu_overflow_returns_minus_one();
  test_gpu_unbalanced_clip_rejected();
  test_gpu_image_ref_carries_texture_and_uvs();
  test_gpu_translator_determinism();
  test_anim_track_sample_keyframes_ordered();
  test_anim_track_loop_infinite();
  test_anim_track_loop_finite_clamps();
  test_anim_spring_converges_with_damping();
  test_anim_spring_oscillates_undamped();
  test_anim_bezier_monotonic();
  test_anim_track_determinism();
  test_gesture_tap_emits_once();
  test_gesture_double_tap_within_gap();
  test_gesture_long_press_via_tick();
  test_gesture_swipe_four_directions();
  test_gesture_drag_emitted_at_threshold_cross();
  test_gesture_determinism();
  test_gesture_ignores_non_touch_and_extra_touch_ids();
  test_dpi_default_is_256();
  test_dpi_set_get_roundtrip();
  test_dpi_scale_dim_helper();
  test_dpi_scope_op_emitted_when_non_default();
  test_dpi_scope_op_omitted_at_default();
  test_dpi_independent_contexts();
  test_dpi_gpu_translator_skips_dpi_scope();
  test_dnd_set_draggable_attaches_payload();
  test_dnd_set_drop_target_attaches_filter();
  test_dnd_type_match_exact_wildcard_prefix();
  test_dnd_drop_invokes_callback_when_compatible();
  test_dnd_drop_silently_ignored_when_incompatible();
  test_dnd_event_types_routed_without_crash();
  test_dnd_dnd_accepts_helper();
  test_dnd_drop_deepest_first();
  test_slab_init_partitions_buffer();
  test_slab_alloc_returns_distinct_pointers_until_full();
  test_slab_free_returns_to_lifo_pool();
  test_slab_degenerate_init_returns_null_on_alloc();
  test_slab_null_guards();
  test_slab_alloc_pointer_alignment();
  test_slab_determinism_same_addresses();
  test_ime_set_preedit_marks_composing();
  test_ime_set_candidates_marks_selecting();
  test_ime_commit_inserts_and_clears();
  test_ime_cancel_preserves_textbox_content();
  test_ime_apis_are_noop_on_non_textbox();
  test_ime_event_types_routed_without_crash();
  test_ime_get_state_snapshot();
  test_transform_identity_is_diagonal();
  test_transform_scale_and_translate();
  test_transform_rotate_quadrants_exact();
  test_transform_multiply_identity_neutral();
  test_transform_multiply_composes();
  test_transform_widget_emits_push_pop();
  test_transform_gpu_translator_skips_transform_ops();
  test_state_serialize_header_and_magic();
  test_state_round_trip();
  test_state_determinism();
  test_state_rejects_bad_magic_version_checksum();
  test_state_capacity_overflow_no_corruption();
  test_state_text_dump();
  test_state_unknown_type_rejected();
  test_plugin_version_tag_macro();
  test_plugin_register_valid_descriptor();
  test_plugin_rejects_abi_mismatch();
  test_plugin_capacity_overflow_rejected();
  test_plugin_unregister_all_calls_destroy();
  test_plugin_null_guards();
  test_plugin_op_in_dl_skipped_by_gpu_translator();
  test_plugin_dl_schema_bumped_to_7();
  test_advanced_widget_tree_creates_and_lays_out();
  test_advanced_widget_table_in_panel_emits();
  test_advanced_widget_rich_text_basic();
  test_advanced_widget_canvas_creates();
  test_advanced_widget_chart_creates();
  test_advanced_widget_color_picker_focusable();
  test_advanced_widget_date_picker_respects_rtl_context();
  test_advanced_widget_autocomplete_roundtrips_serialization();
  test_virtual_set_source_stores_pointer();
  test_virtual_count_forwards_to_callback();
  test_virtual_get_item_decodes_index();
  test_virtual_null_source_returns_negative();
  test_virtual_helpers_null_guards();
  test_virtual_request_range_event_routes_without_crash();
  test_virtual_source_on_non_list_widget_tolerated();
  test_undo_init_partitions_caller_buffer();
  test_undo_push_undo_redo_roundtrip();
  test_undo_push_truncates_redo();
  test_undo_fifo_eviction_on_full_buffer();
  test_undo_on_empty_returns_negative();
  test_undo_null_guards_and_too_small_buffer();
  test_undo_determinism_same_sequence();
  test_theme_pack_validate_well_formed_zero();
  test_theme_pack_load_applies_tokens();
  test_theme_pack_bad_magic_rejected();
  test_theme_pack_future_version_rejected();
  test_theme_pack_checksum_mismatch_rejected();
  test_theme_pack_unknown_token_skipped();
  test_theme_pack_truncated_buffer_rejected();
  test_theme_pack_reserved_field_nonzero_rejected();
  test_inspector_root_only_returns_one_node();
  test_inspector_depth_first_preorder();
  test_inspector_text_pool_copies_label_text();
  test_inspector_capacity_overflow_fails_closed();
  test_inspector_text_pool_overflow_fails_closed();
  test_inspector_null_guards();
  test_perf_counters_snapshot_populates_fields();
  test_display_enum_modes_forwards_to_host();
  test_display_set_mode_caches_and_emits_event();
  test_display_set_mode_host_failure_no_cache();
  test_display_no_controller_returns_negative();
  test_display_set_mode_rejects_zero_dimensions();
  test_display_enum_modes_capacity_truncates();
  test_display_null_guards();
  test_user_list_forwards_to_host();
  test_user_create_appends_and_passes_password();
  test_user_create_rejects_empty_username();
  test_user_update_roundtrip();
  test_user_delete_rejects_root_uid_zero();
  test_user_set_avatar_clear_and_set();
  test_user_no_directory_returns_negative();
  test_contrast_white_on_black_is_21000();
  test_contrast_same_color_is_1000();
  test_contrast_alpha_ignored();
  test_contrast_ratio_in_bounds();
  test_audit_dry_run_returns_total();
  test_audit_high_contrast_all_pass_aa();
  test_audit_dark_high_contrast_all_pass_aaa();
  test_audit_null_and_capacity_overflow();
  test_desktop_layout_roundtrip();
  test_desktop_layout_empty_roundtrip();
  test_desktop_layout_bad_magic_rejected();
  test_desktop_layout_checksum_mismatch_rejected();
  test_desktop_layout_zero_cell_rejected();
  test_desktop_layout_invalid_sort_rejected();
  test_desktop_layout_short_buffer_reports_count();
  test_desktop_layout_null_guards();
  test_breadcrumb_full_fit_no_dropdown();
  test_breadcrumb_truncated_root_plus_tail();
  test_breadcrumb_min_two_with_narrow_width();
  test_breadcrumb_empty_input();
  test_breadcrumb_single_segment();
  test_breadcrumb_null_guards();
  test_breadcrumb_cap_too_small_for_full_fit();
  test_toolbar_group_taxonomy();
  test_icon_resolve_forwards_to_host();
  test_icon_resolve_fallback_no_provider();
  test_icon_resolve_fallback_host_failure();
  test_icon_thumbnail_request_forwards();
  test_icon_thumbnail_request_failure_modes();
  test_icon_thumbnail_ready_rejects_id_zero();
  test_icon_no_provider_thumbnail_request_returns_negative();
  test_icon_null_guards();
  test_wallpaper_roundtrip();
  test_wallpaper_no_slides_roundtrip();
  test_wallpaper_invalid_fit_rejected();
  test_wallpaper_slide_zero_id_rejected();
  test_wallpaper_bad_magic_and_checksum_rejected();
  test_wallpaper_short_buffer_reports_count();
  test_wallpaper_null_guards();
  test_login_layout_zero_or_one();
  test_login_layout_grid_range();
  test_login_layout_list_threshold();
  test_login_layout_deterministic();
  test_login_layout_taxonomy();
  test_login_power_taxonomy();
  test_date_set_valid_and_get();
  test_date_unset_get_returns_zero();
  test_date_invalid_rejected_fail_closed();
  test_date_clear_resets();
  test_date_wrong_widget_type_rejected();
  test_date_is_valid_calendar_predicate();
  test_date_null_guards();
  test_date_determinism();
  test_color_pack_channel_order();
  test_color_pack_extremes();
  test_color_set_and_get();
  test_color_unset_get_returns_zero();
  test_color_clear_resets();
  test_color_wrong_widget_type_rejected();
  test_color_null_guards();
  test_color_determinism();
  test_table_set_and_query();
  test_table_index_out_of_range();
  test_table_set_zero_count_clears();
  test_table_set_null_widths_rejected();
  test_table_clear();
  test_table_wrong_widget_type_rejected();
  test_table_null_guards();
  test_table_default_no_columns();
  test_autocomplete_set_and_query();
  test_autocomplete_item_out_of_range();
  test_autocomplete_set_zero_count_clears();
  test_autocomplete_set_null_items_rejected();
  test_autocomplete_clear();
  test_autocomplete_selection_clamp();
  test_autocomplete_wrong_widget_type_rejected();
  test_autocomplete_null_guards();
  test_tree_collapsed_default_expanded();
  test_tree_set_collapsed_toggle();
  test_tree_depth_set_and_get();
  test_tree_row_visible_no_ancestors();
  test_tree_row_visible_collapsed_ancestor();
  test_tree_row_visible_nested();
  test_tree_wrong_widget_type_rejected();
  test_tree_null_guards();
  test_chart_set_and_query();
  test_chart_value_out_of_range();
  test_chart_set_zero_count_clears();
  test_chart_set_null_values_rejected();
  test_chart_clear();
  test_chart_range_minmax();
  test_chart_wrong_widget_type_rejected();
  test_chart_null_guards();
  test_rich_text_set_and_query();
  test_rich_text_range_out_of_range();
  test_rich_text_set_zero_count_clears();
  test_rich_text_set_null_ranges_rejected();
  test_rich_text_clear();
  test_rich_text_range_at_lookup();
  test_rich_text_wrong_widget_type_rejected();
  test_rich_text_null_guards();
  test_canvas_set_and_invoke();
  test_canvas_callback_failure_propagates();
  test_canvas_clear();
  test_canvas_default_unset();
  test_canvas_set_null_fn_clears();
  test_canvas_user_data_getter();
  test_canvas_wrong_widget_type_rejected();
  test_canvas_null_guards();
  test_gesture_pinch_out();
  test_gesture_pinch_in();
  test_gesture_rotate_cw();
  test_gesture_rotate_ccw();
  test_gesture_rotate_below_threshold();
  test_gesture_multi_touch_end_resets();
  test_gesture_single_touch_swipe_regression();
  test_gesture_third_finger_ignored();
  return failures == 0 ? 0 : 1;
}
