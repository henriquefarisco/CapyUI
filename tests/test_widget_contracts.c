#include "capy_widget.h"
#include "capy_display_list.h"

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
  EXPECT(dl.version == 2u);
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
  EXPECT(light.version == 1u);
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
  return failures == 0 ? 0 : 1;
}
