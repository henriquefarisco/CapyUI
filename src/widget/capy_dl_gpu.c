#include "capy_dl_gpu.h"

/* Internal clip stack frame. `valid == 0` means the frame collapsed to
 * zero area; any quad emitted while a zero-area frame is on top of the
 * stack is silently dropped (the host would clip it to nothing anyway). */
struct capy_dl_gpu_clip {
  int32_t x0;
  int32_t y0;
  int32_t x1;
  int32_t y1;
  uint8_t valid;
};

static int capy_gpu_emit_quad(struct capy_gpu_quad *out, uint32_t cap,
                              uint32_t *out_count,
                              const struct capy_dl_gpu_clip *clip,
                              int clip_depth, int32_t x, int32_t y,
                              uint32_t w, uint32_t h, uint32_t color,
                              uint32_t texture_id, int32_t uv_w,
                              int32_t uv_h) {
  int32_t x0 = x;
  int32_t y0 = y;
  int32_t x1 = x + (int32_t)w;
  int32_t y1 = y + (int32_t)h;
  if (w == 0u || h == 0u) {
    /* Empty quads are silently dropped; no buffer write, no count bump. */
    return 0;
  }
  if (clip_depth > 0) {
    const struct capy_dl_gpu_clip *top = &clip[clip_depth - 1];
    if (!top->valid) {
      return 0;
    }
    if (x0 < top->x0) {
      x0 = top->x0;
    }
    if (y0 < top->y0) {
      y0 = top->y0;
    }
    if (x1 > top->x1) {
      x1 = top->x1;
    }
    if (y1 > top->y1) {
      y1 = top->y1;
    }
    if (x0 >= x1 || y0 >= y1) {
      return 0;
    }
  }
  if (*out_count >= cap) {
    return -1;
  }
  out[*out_count].x0 = x0;
  out[*out_count].y0 = y0;
  out[*out_count].x1 = x1;
  out[*out_count].y1 = y1;
  out[*out_count].color = color;
  out[*out_count].texture_id = texture_id;
  out[*out_count].uv_x0 = 0;
  out[*out_count].uv_y0 = 0;
  out[*out_count].uv_x1 = uv_w;
  out[*out_count].uv_y1 = uv_h;
  ++(*out_count);
  return 0;
}

static int capy_gpu_emit_border(struct capy_gpu_quad *out, uint32_t cap,
                                uint32_t *out_count,
                                const struct capy_dl_gpu_clip *clip,
                                int clip_depth,
                                const struct capy_ui_rect *rect,
                                uint32_t border_width, uint32_t color) {
  /* Top strip. */
  if (border_width > rect->height) {
    border_width = rect->height;
  }
  if (border_width == 0u) {
    return 0;
  }
  if (capy_gpu_emit_quad(out, cap, out_count, clip, clip_depth, rect->x,
                         rect->y, rect->width, border_width, color, 0u, 0,
                         0) != 0) {
    return -1;
  }
  /* Bottom strip. */
  if (capy_gpu_emit_quad(out, cap, out_count, clip, clip_depth, rect->x,
                         rect->y + (int32_t)rect->height -
                             (int32_t)border_width,
                         rect->width, border_width, color, 0u, 0, 0) != 0) {
    return -1;
  }
  /* Left strip (between top and bottom). */
  if (rect->height > 2u * border_width) {
    uint32_t inner_h = rect->height - 2u * border_width;
    uint32_t side_w =
        (rect->width < border_width) ? rect->width : border_width;
    if (capy_gpu_emit_quad(out, cap, out_count, clip, clip_depth, rect->x,
                           rect->y + (int32_t)border_width, side_w, inner_h,
                           color, 0u, 0, 0) != 0) {
      return -1;
    }
    if (rect->width > border_width) {
      /* Right strip. */
      if (capy_gpu_emit_quad(out, cap, out_count, clip, clip_depth,
                             rect->x + (int32_t)rect->width -
                                 (int32_t)border_width,
                             rect->y + (int32_t)border_width, side_w,
                             inner_h, color, 0u, 0, 0) != 0) {
        return -1;
      }
    }
  }
  return 0;
}

int capy_dl_to_quads(const struct capy_display_list *dl,
                    struct capy_gpu_quad *out, uint32_t cap,
                    uint32_t *out_count) {
  struct capy_dl_gpu_clip stack[CAPY_DL_GPU_CLIP_STACK_MAX];
  int depth = 0;
  uint32_t i;
  if (!dl || !out_count) {
    return -1;
  }
  if (cap > 0u && !out) {
    return -1;
  }
  *out_count = 0u;
  for (i = 0u; i < dl->count; ++i) {
    const struct capy_dl_cmd *cmd = &dl->cmds[i];
    switch (cmd->op) {
      case CAPY_DL_RECT: {
        if (capy_gpu_emit_quad(out, cap, out_count, stack, depth, cmd->rect.x,
                               cmd->rect.y, cmd->rect.width, cmd->rect.height,
                               cmd->color, 0u, 0, 0) != 0) {
          return -1;
        }
        break;
      }
      case CAPY_DL_BORDER: {
        if (capy_gpu_emit_border(out, cap, out_count, stack, depth,
                                 &cmd->rect, (uint32_t)cmd->border_width,
                                 cmd->color) != 0) {
          return -1;
        }
        break;
      }
      case CAPY_DL_TEXT: {
        /* Single placeholder quad; the host translates further with its
         * own glyph atlas keyed by font_id + text_offset + text_len. */
        if (capy_gpu_emit_quad(out, cap, out_count, stack, depth, cmd->rect.x,
                               cmd->rect.y, cmd->rect.width, cmd->rect.height,
                               cmd->color, 0u, 0, 0) != 0) {
          return -1;
        }
        break;
      }
      case CAPY_DL_FOCUS_RING: {
        if (capy_gpu_emit_border(out, cap, out_count, stack, depth,
                                 &cmd->rect, (uint32_t)cmd->border_width,
                                 cmd->color) != 0) {
          return -1;
        }
        break;
      }
      case CAPY_DL_IMAGE_REF: {
        if (capy_gpu_emit_quad(out, cap, out_count, stack, depth, cmd->rect.x,
                               cmd->rect.y, cmd->rect.width, cmd->rect.height,
                               cmd->color, cmd->image_id,
                               (int32_t)cmd->rect.width,
                               (int32_t)cmd->rect.height) != 0) {
          return -1;
        }
        break;
      }
      case CAPY_DL_CLIP_PUSH: {
        struct capy_dl_gpu_clip frame;
        if ((uint32_t)depth >= CAPY_DL_GPU_CLIP_STACK_MAX) {
          return -1;
        }
        frame.x0 = cmd->rect.x;
        frame.y0 = cmd->rect.y;
        frame.x1 = cmd->rect.x + (int32_t)cmd->rect.width;
        frame.y1 = cmd->rect.y + (int32_t)cmd->rect.height;
        frame.valid = 1u;
        if (depth > 0) {
          const struct capy_dl_gpu_clip *parent = &stack[depth - 1];
          if (!parent->valid) {
            frame.valid = 0u;
          } else {
            if (frame.x0 < parent->x0) {
              frame.x0 = parent->x0;
            }
            if (frame.y0 < parent->y0) {
              frame.y0 = parent->y0;
            }
            if (frame.x1 > parent->x1) {
              frame.x1 = parent->x1;
            }
            if (frame.y1 > parent->y1) {
              frame.y1 = parent->y1;
            }
            if (frame.x0 >= frame.x1 || frame.y0 >= frame.y1) {
              frame.valid = 0u;
            }
          }
        }
        if (cmd->rect.width == 0u || cmd->rect.height == 0u) {
          frame.valid = 0u;
        }
        stack[depth++] = frame;
        break;
      }
      case CAPY_DL_CLIP_POP: {
        if (depth == 0) {
          return -1;
        }
        --depth;
        break;
      }
      case CAPY_DL_NONE:
      case CAPY_DL_DIRTY_HINT:
      case CAPY_DL_DPI_SCOPE:
      case CAPY_DL_TRANSFORM_PUSH:
      case CAPY_DL_TRANSFORM_POP:
      case CAPY_DL_PLUGIN_OP:
        /* No quad output for these. DIRTY_HINT is reserved for higher-level
         * producers; NONE should never appear in a well-formed DL but is
         * tolerated without emit. DPI_SCOPE (since 1.5) is metadata the host
         * backend reads to choose the rasterizer / viewport before consuming
         * quads — translator does not emit a quad for it. TRANSFORM_PUSH/POP
         * (since 1.9) carry an affine matrix the host backend feeds into its
         * own matrix stack; translator ignores them so quads remain in
         * widget-local axis-aligned coordinates. */
        break;
      default:
        return -1;
    }
  }
  if (depth != 0) {
    return -1;
  }
  return 0;
}
