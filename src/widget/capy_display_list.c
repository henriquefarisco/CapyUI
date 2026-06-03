#include "capy_display_list.h"

static void capy_dl_zero(void *ptr, size_t n) {
  unsigned char *p = (unsigned char *)ptr;
  while (n--) {
    *p++ = 0u;
  }
}

void capy_display_list_init(struct capy_display_list *dl, void *cmd_buf,
                            uint32_t cmd_cap, char *text_buf,
                            uint32_t text_cap) {
  if (!dl) {
    return;
  }
  dl->cmds = (struct capy_dl_cmd *)cmd_buf;
  dl->count = 0u;
  dl->capacity = cmd_cap;
  dl->text_pool = text_buf;
  dl->text_used = 0u;
  dl->text_capacity = text_cap;
  dl->version = CAPY_DISPLAY_LIST_SCHEMA_VERSION;
  dl->theme = 0;
  dl->dpi_scale_x256 = 256u;
  dl->reserved_dpi = 0u;
}

void capy_display_list_reset(struct capy_display_list *dl) {
  if (!dl) {
    return;
  }
  dl->count = 0u;
  dl->text_used = 0u;
}

static struct capy_dl_cmd *capy_dl_push(struct capy_display_list *dl) {
  struct capy_dl_cmd *cmd;
  if (!dl->cmds || dl->count >= dl->capacity) {
    return 0;
  }
  cmd = &dl->cmds[dl->count++];
  capy_dl_zero(cmd, sizeof(*cmd));
  return cmd;
}

static int capy_dl_copy_text(struct capy_display_list *dl, const char *src,
                             uint16_t *out_offset, uint16_t *out_len) {
  uint32_t srclen = 0u;
  uint32_t i;
  *out_offset = 0u;
  *out_len = 0u;
  if (!src || src[0] == '\0') {
    return 0;
  }
  while (src[srclen] && srclen < 0xFFFFu) {
    ++srclen;
  }
  if (!dl->text_pool || dl->text_used + srclen > dl->text_capacity) {
    return -1;
  }
  *out_offset = (uint16_t)dl->text_used;
  *out_len = (uint16_t)srclen;
  for (i = 0u; i < srclen; ++i) {
    dl->text_pool[dl->text_used + i] = src[i];
  }
  dl->text_used += srclen;
  return 0;
}

static int capy_widget_emit_recursive(struct capy_widget *w,
                                      struct capy_display_list *dl) {
  struct capy_dl_cmd *cmd;
  uint16_t text_offset = 0u;
  /* v1.9: prepend CAPY_DL_TRANSFORM_PUSH when this widget carries a
   * non-NULL transform; the matching POP is appended after CLIP_POP at
   * the end of this function. Identity (NULL) emits nothing → preserves
   * byte-equivalent output with pre-1.9 for untouched widget trees. */
  uint16_t text_len = 0u;
  uint32_t i;
  if (!w || !w->visible) {
    return 0;
  }
  if (w->transform) {
    cmd = capy_dl_push(dl);
    if (!cmd) {
      return -1;
    }
    cmd->op = CAPY_DL_TRANSFORM_PUSH;
    cmd->transform = *w->transform;
  }
  cmd = capy_dl_push(dl);
  if (!cmd) {
    return -1;
  }
  cmd->op = CAPY_DL_CLIP_PUSH;
  cmd->rect = w->bounds;
  cmd = capy_dl_push(dl);
  if (!cmd) {
    return -1;
  }
  cmd->op = CAPY_DL_RECT;
  cmd->rect = w->bounds;
  cmd->color = capy_theme_resolve(dl->theme, w->style.bg_token,
                                  w->style.bg_color);
  if (w->style.border_width > 0u) {
    cmd = capy_dl_push(dl);
    if (!cmd) {
      return -1;
    }
    cmd->op = CAPY_DL_BORDER;
    cmd->rect = w->bounds;
    cmd->color = capy_theme_resolve(dl->theme, w->style.border_token,
                                    w->style.border_color);
    cmd->border_width = w->style.border_width;
  }
  if (w->focused) {
    cmd = capy_dl_push(dl);
    if (!cmd) {
      return -1;
    }
    cmd->op = CAPY_DL_FOCUS_RING;
    cmd->rect = w->bounds;
    cmd->color = w->style.active_color;
    cmd->border_width = (w->style.border_width > 0u)
                            ? (uint8_t)(w->style.border_width + 1u)
                            : 2u;
  }
  if (w->text[0] != '\0') {
    int copy_rc;
    if (w->type == CAPY_WIDGET_TEXTBOX && w->text_edit.password) {
      uint32_t k;
      uint16_t char_count = 0u;
      for (k = 0u; w->text[k] != '\0' && k < 0xFFFFu; ++k) {
        if (((uint8_t)w->text[k] & 0xC0u) != 0x80u) {
          ++char_count;
        }
      }
      if (char_count == 0u) {
        text_offset = 0u;
        text_len = 0u;
        copy_rc = 0;
      } else if (!dl->text_pool ||
                 dl->text_used + char_count > dl->text_capacity) {
        copy_rc = -1;
      } else {
        text_offset = (uint16_t)dl->text_used;
        text_len = char_count;
        for (k = 0u; k < char_count; ++k) {
          dl->text_pool[dl->text_used + k] = '*';
        }
        dl->text_used += char_count;
        copy_rc = 0;
      }
    } else {
      copy_rc = capy_dl_copy_text(dl, w->text, &text_offset, &text_len);
    }
    if (copy_rc != 0) {
      return -1;
    }
    cmd = capy_dl_push(dl);
    if (!cmd) {
      return -1;
    }
    cmd->op = CAPY_DL_TEXT;
    cmd->rect = w->bounds;
    cmd->color = capy_theme_resolve(dl->theme, w->style.fg_token,
                                    w->style.text_color);
    cmd->text_offset = text_offset;
    cmd->text_len = text_len;
    cmd->font_size = w->style.font_size;
    /* Since 0.9 (DL schema 4): forward the widget's font_id reference so the
     * host compositor / font loader resolves the actual glyph set. `0` is the
     * default font and is always valid. */
    cmd->font_id = w->font_id;
  }
  if (w->type == CAPY_WIDGET_TEXTBOX && w->focused &&
      !w->text_edit.readonly && !w->text_edit.password) {
    cmd = capy_dl_push(dl);
    if (!cmd) {
      return -1;
    }
    cmd->op = CAPY_DL_RECT;
    cmd->color = capy_theme_resolve(dl->theme, w->style.fg_token,
                                    w->style.text_color);
    {
      uint32_t font_w =
          (w->style.font_size > 0u) ? (uint32_t)(w->style.font_size / 2u) : 8u;
      uint32_t caret_x =
          (uint32_t)w->text_edit.caret * (font_w > 0u ? font_w : 1u);
      uint32_t caret_h =
          (w->style.font_size > 0u) ? (uint32_t)w->style.font_size : 16u;
      cmd->rect.x = w->bounds.x + (int32_t)caret_x;
      cmd->rect.y = w->bounds.y + 2;
      cmd->rect.width = 1u;
      cmd->rect.height = caret_h;
    }
  }
  for (i = 0u; i < w->child_count; ++i) {
    if (capy_widget_emit_recursive(w->children[i], dl) != 0) {
      return -1;
    }
  }
  cmd = capy_dl_push(dl);
  if (!cmd) {
    return -1;
  }
  cmd->op = CAPY_DL_CLIP_POP;
  if (w->transform) {
    cmd = capy_dl_push(dl);
    if (!cmd) {
      return -1;
    }
    cmd->op = CAPY_DL_TRANSFORM_POP;
  }
  return 0;
}

int capy_widget_emit(struct capy_widget *root, struct capy_display_list *dl) {
  uint32_t snap_count;
  uint32_t snap_text;
  if (!root || !dl) {
    return -1;
  }
  snap_count = dl->count;
  snap_text = dl->text_used;
  /* Since 1.5: prepend a DPI_SCOPE marker when the host has set a
   * non-default scale on the display list. The scope rect covers the
   * root widget bounds; future minors may emit per-subtree scopes fo
   * windows on different displays. `image_id` carries the scale (reuses
   * the existing 32-bit slot to avoid bumping struct sizeof). */
  if (dl->dpi_scale_x256 != 256u) {
    struct capy_dl_cmd *dpi_cmd = capy_dl_push(dl);
    if (!dpi_cmd) {
      dl->count = snap_count;
      dl->text_used = snap_text;
      return -1;
    }
    dpi_cmd->op = CAPY_DL_DPI_SCOPE;
    dpi_cmd->rect = root->bounds;
    dpi_cmd->image_id = (uint32_t)dl->dpi_scale_x256;
  }
  if (capy_widget_emit_recursive(root, dl) != 0) {
    dl->count = snap_count;
    dl->text_used = snap_text;
    return -1;
  }
  return 0;
}

/* Byte-equality comparison of two display-list commands.
 * Safe because capy_dl_push zero-fills every command on allocation, so
 * unused padding/reserved bytes are always 0 on both sides. */
static int capy_dl_cmd_equal(const struct capy_dl_cmd *a,
                             const struct capy_dl_cmd *b) {
  const unsigned char *pa = (const unsigned char *)a;
  const unsigned char *pb = (const unsigned char *)b;
  size_t i;
  for (i = 0u; i < sizeof(struct capy_dl_cmd); ++i) {
    if (pa[i] != pb[i]) {
      return 0;
    }
  }
  return 1;
}

static int capy_dl_rect_equal(const struct capy_ui_rect *a,
                              const struct capy_ui_rect *b) {
  return a->x == b->x && a->y == b->y && a->width == b->width &&
         a->height == b->height;
}

/* Append `r` to out_dirty[] coalescing if equal to last entry.
 * Returns 0 on success, -1 on overflow. */
static int capy_dl_dirty_append(struct capy_ui_rect *out, uint32_t cap,
                                uint32_t *count, const struct capy_ui_rect *r) {
  if (*count > 0u && capy_dl_rect_equal(&out[*count - 1u], r)) {
    return 0;
  }
  if (*count >= cap) {
    return -1;
  }
  out[*count] = *r;
  ++(*count);
  return 0;
}

int capy_widget_diff(const struct capy_display_list *prev,
                     const struct capy_display_list *next,
                     struct capy_ui_rect *out_dirty, uint32_t out_cap) {
  uint32_t prev_count = prev ? prev->count : 0u;
  uint32_t next_count = next ? next->count : 0u;
  uint32_t common;
  uint32_t i;
  uint32_t written = 0u;
  if (out_cap > 0u && !out_dirty) {
    return -1;
  }
  if (prev_count == 0u && next_count == 0u) {
    return 0;
  }
  common = (prev_count < next_count) ? prev_count : next_count;
  for (i = 0u; i < common; ++i) {
    const struct capy_dl_cmd *pa = &prev->cmds[i];
    const struct capy_dl_cmd *pb = &next->cmds[i];
    if (capy_dl_cmd_equal(pa, pb)) {
      continue;
    }
    if (capy_dl_dirty_append(out_dirty, out_cap, &written, &pb->rect) != 0) {
      return -1;
    }
    if (!capy_dl_rect_equal(&pa->rect, &pb->rect)) {
      if (capy_dl_dirty_append(out_dirty, out_cap, &written, &pa->rect) != 0) {
        return -1;
      }
    }
  }
  for (i = common; i < next_count; ++i) {
    const struct capy_ui_rect *r = &next->cmds[i].rect;
    if (capy_dl_dirty_append(out_dirty, out_cap, &written, r) != 0) {
      return -1;
    }
  }
  for (i = common; i < prev_count; ++i) {
    const struct capy_ui_rect *r = &prev->cmds[i].rect;
    if (capy_dl_dirty_append(out_dirty, out_cap, &written, r) != 0) {
      return -1;
    }
  }
  return (int)written;
}

/* Since 1.1: equivalent to capy_widget_diff but with O(prefix) common-range
 * skipping. The output is byte-identical because the skipped prefix cannot
 * contribute rects (equal cmds) and the inner positional walk over the
 * remaining range follows the exact same algorithm as capy_widget_diff.
 *
 * A symmetric suffix skip would change observable behaviour: capy_widget_diff
 * is positional (it pairs prev[i] with next[i]) and emits trailing rects
 * unconditionally when the two lists differ in length. A suffix-aware skip
 * would implicitly re-align the two lists and drop those trailing rects.
 * To keep byte-equivalence with capy_widget_diff under the contract, only
 * the leading common prefix is short-circuited here. */
int capy_display_list_diff_incremental(const struct capy_display_list *prev,
                                       const struct capy_display_list *next,
                                       struct capy_ui_rect *out_dirty,
                                       uint32_t out_cap) {
  uint32_t prev_count = prev ? prev->count : 0u;
  uint32_t next_count = next ? next->count : 0u;
  uint32_t prefix = 0u;
  uint32_t common;
  uint32_t i;
  uint32_t written = 0u;
  if (out_cap > 0u && !out_dirty) {
    return -1;
  }
  if (prev_count == 0u && next_count == 0u) {
    return 0;
  }
  common = (prev_count < next_count) ? prev_count : next_count;
  while (prefix < common &&
         capy_dl_cmd_equal(&prev->cmds[prefix], &next->cmds[prefix])) {
    ++prefix;
  }
  /* From `prefix` onwards we replay the exact positional diff logic. */
  for (i = prefix; i < common; ++i) {
    const struct capy_dl_cmd *pa = &prev->cmds[i];
    const struct capy_dl_cmd *pb = &next->cmds[i];
    if (capy_dl_cmd_equal(pa, pb)) {
      continue;
    }
    if (capy_dl_dirty_append(out_dirty, out_cap, &written, &pb->rect) != 0) {
      return -1;
    }
    if (!capy_dl_rect_equal(&pa->rect, &pb->rect)) {
      if (capy_dl_dirty_append(out_dirty, out_cap, &written, &pa->rect) != 0) {
        return -1;
      }
    }
  }
  for (i = common; i < next_count; ++i) {
    const struct capy_ui_rect *r = &next->cmds[i].rect;
    if (capy_dl_dirty_append(out_dirty, out_cap, &written, r) != 0) {
      return -1;
    }
  }
  for (i = common; i < prev_count; ++i) {
    const struct capy_ui_rect *r = &prev->cmds[i].rect;
    if (capy_dl_dirty_append(out_dirty, out_cap, &written, r) != 0) {
      return -1;
    }
  }
  return (int)written;
}
