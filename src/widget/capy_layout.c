#include "capy_layout.h"

#include "capy_widget.h"

static uint32_t capy_layout_clamp_u32(uint32_t v, uint32_t lo, uint32_t hi) {
  if (hi != 0u && v > hi) {
    v = hi;
  }
  if (v < lo) {
    v = lo;
  }
  return v;
}

void capy_widget_set_layout(struct capy_widget *widget,
                            const struct capy_layout_node *node) {
  if (!widget || !node) {
    return;
  }
  widget->layout = *node;
}

int capy_widget_measure(struct capy_widget *root, uint32_t avail_w,
                        uint32_t avail_h) {
  uint32_t w;
  uint32_t h;
  if (!root) {
    return -1;
  }
  w = capy_layout_clamp_u32(avail_w, root->layout.c.min_w, root->layout.c.max_w);
  h = capy_layout_clamp_u32(avail_h, root->layout.c.min_h, root->layout.c.max_h);
  root->bounds.width = w;
  root->bounds.height = h;
  return 0;
}

static void capy_layout_arrange_stack(struct capy_widget *root, int32_t ix,
                                      int32_t iy, int32_t iw, int32_t ih) {
  const struct capy_layout_constraints *c = &root->layout.c;
  uint32_t n = root->child_count;
  uint8_t axis = c->axis;
  uint32_t total_grow = 0u;
  uint32_t fixed = 0u;
  uint32_t total_gap;
  int32_t total_axis;
  int32_t remaining;
  int32_t cursor;
  uint32_t i;
  if (n == 0u) {
    return;
  }
  total_gap = (uint32_t)c->gap * (n - 1u);
  for (i = 0u; i < n; ++i) {
    struct capy_widget *ch = root->children[i];
    total_grow += ch->layout.grow;
    if (ch->layout.grow == 0u) {
      fixed += (axis == 1u) ? ch->bounds.height : ch->bounds.width;
    }
  }
  total_axis = (axis == 1u) ? ih : iw;
  remaining = total_axis - (int32_t)total_gap - (int32_t)fixed;
  if (remaining < 0) {
    remaining = 0;
  }
  cursor = (axis == 1u) ? iy : ix;
  for (i = 0u; i < n; ++i) {
    struct capy_widget *ch = root->children[i];
    uint32_t cross = (axis == 1u) ? (uint32_t)iw : (uint32_t)ih;
    uint32_t main_sz;
    if (ch->layout.grow > 0u && total_grow > 0u) {
      main_sz = (uint32_t)(((uint64_t)remaining * (uint64_t)ch->layout.grow) /
                           (uint64_t)total_grow);
    } else {
      main_sz = (axis == 1u) ? ch->bounds.height : ch->bounds.width;
    }
    if (axis == 1u) {
      ch->bounds.x = ix;
      ch->bounds.y = cursor;
      ch->bounds.width = cross;
      ch->bounds.height = main_sz;
    } else {
      ch->bounds.x = cursor;
      ch->bounds.y = iy;
      ch->bounds.width = main_sz;
      ch->bounds.height = cross;
    }
    cursor += (int32_t)main_sz + (int32_t)c->gap;
    capy_widget_arrange(ch);
  }
}

static void capy_layout_arrange_grid(struct capy_widget *root, int32_t ix,
                                     int32_t iy, int32_t iw, int32_t ih) {
  const struct capy_layout_constraints *c = &root->layout.c;
  uint32_t n = root->child_count;
  uint8_t cols;
  uint32_t rows;
  int32_t cell_w;
  int32_t cell_h;
  uint32_t i;
  if (n == 0u) {
    return;
  }
  cols = c->cols ? c->cols : 1u;
  rows = (n + cols - 1u) / cols;
  cell_w = (iw - (int32_t)c->gap * (int32_t)(cols - 1u)) / (int32_t)cols;
  cell_h = (rows > 0u)
               ? (ih - (int32_t)c->gap * (int32_t)(rows - 1u)) / (int32_t)rows
               : 0;
  if (cell_w < 0) {
    cell_w = 0;
  }
  if (cell_h < 0) {
    cell_h = 0;
  }
  for (i = 0u; i < n; ++i) {
    uint32_t r = i / cols;
    uint32_t col = i % cols;
    struct capy_widget *ch = root->children[i];
    ch->bounds.x = ix + (int32_t)col * (cell_w + (int32_t)c->gap);
    ch->bounds.y = iy + (int32_t)r * (cell_h + (int32_t)c->gap);
    ch->bounds.width = (uint32_t)cell_w;
    ch->bounds.height = (uint32_t)cell_h;
    capy_widget_arrange(ch);
  }
}

static void capy_layout_arrange_flow(struct capy_widget *root, int32_t ix,
                                     int32_t iy, int32_t iw, int32_t ih) {
  const struct capy_layout_constraints *c = &root->layout.c;
  uint32_t n = root->child_count;
  int32_t cur_x = ix;
  int32_t cur_y = iy;
  uint32_t line_h = 0u;
  uint32_t i;
  (void)ih;
  for (i = 0u; i < n; ++i) {
    struct capy_widget *ch = root->children[i];
    if (cur_x + (int32_t)ch->bounds.width > ix + iw && cur_x > ix) {
      cur_x = ix;
      cur_y += (int32_t)line_h + (int32_t)c->gap;
      line_h = 0u;
    }
    ch->bounds.x = cur_x;
    ch->bounds.y = cur_y;
    cur_x += (int32_t)ch->bounds.width + (int32_t)c->gap;
    if (ch->bounds.height > line_h) {
      line_h = ch->bounds.height;
    }
    capy_widget_arrange(ch);
  }
}

int capy_widget_arrange(struct capy_widget *root) {
  const struct capy_layout_constraints *c;
  int32_t ix;
  int32_t iy;
  int32_t iw;
  int32_t ih;
  uint32_t i;
  if (!root) {
    return -1;
  }
  c = &root->layout.c;
  ix = root->bounds.x + (int32_t)c->pad_l;
  iy = root->bounds.y + (int32_t)c->pad_t;
  iw = (int32_t)root->bounds.width - (int32_t)c->pad_l - (int32_t)c->pad_r;
  ih = (int32_t)root->bounds.height - (int32_t)c->pad_t - (int32_t)c->pad_b;
  if (iw < 0) {
    iw = 0;
  }
  if (ih < 0) {
    ih = 0;
  }
  switch (root->layout.kind) {
    case CAPY_LAYOUT_STACK:
      capy_layout_arrange_stack(root, ix, iy, iw, ih);
      break;
    case CAPY_LAYOUT_GRID:
      capy_layout_arrange_grid(root, ix, iy, iw, ih);
      break;
    case CAPY_LAYOUT_FLOW:
      capy_layout_arrange_flow(root, ix, iy, iw, ih);
      break;
    case CAPY_LAYOUT_NONE:
    default:
      for (i = 0u; i < root->child_count; ++i) {
        capy_widget_arrange(root->children[i]);
      }
      break;
  }
  return 0;
}
