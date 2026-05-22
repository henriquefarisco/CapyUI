#ifndef CAPY_UI_DL_GPU_H
#define CAPY_UI_DL_GPU_H

#include <stddef.h>
#include <stdint.h>

#include "capy_display_list.h"

/* GPU translation primitives (since 1.2.0).
 *
 * `capy_dl_to_quads` walks a `capy_display_list` produced by
 * `capy_widget_emit` and decomposes each renderable op into one or more
 * axis-aligned coloured quads. The translator is **strictly portable**:
 * no OpenGL/Vulkan/Metal symbol is referenced. Host CapyOS picks the
 * backend (CPU rasterizer, Mesa, lavapipe, VirGL, …) and consumes the
 * `capy_gpu_quad[]` output.
 *
 * Coordinates use the same `int32_t` units as `capy_ui_rect` from the
 * widget core. `texture_id == 0` means "no texture" — the quad is a
 * solid colour fill. `uv_*` coordinates are `int32_t` so backends can
 * choose normalisation. For solid-colour quads the `uv_*` fields are
 * zeroed.
 *
 * Op decomposition (deterministic, byte-for-byte stable):
 *   CAPY_DL_RECT       → 1 quad (cmd->rect, color).
 *   CAPY_DL_BORDER     → 4 quads (top, right, bottom, left strips with
 *                                  `cmd->border_width` thickness).
 *   CAPY_DL_TEXT       → 1 quad spanning the text rect with
 *                       `texture_id = 0` (host translates further with
 *                       its own glyph atlas keyed by font_id+text_offset+
 *                       text_len). UVs zeroed.
 *   CAPY_DL_FOCUS_RING → 4 quads (outer ring strips with
 *                       `cmd->border_width`). Same shape as BORDER.
 *   CAPY_DL_IMAGE_REF  → 1 quad with `texture_id = cmd->image_id` and
 *                       full-texture UVs (0,0)-(rect.w, rect.h).
 *   CAPY_DL_CLIP_PUSH  → no quad emitted. The translator tracks the
 *                       active clip rect internally and intersects every
 *                       subsequent emitted quad against the current
 *                       clip stack. A clip with zero area drops the
 *                       contained quads silently.
 *   CAPY_DL_CLIP_POP   → no quad emitted. Pops the clip stack.
 *   CAPY_DL_DIRTY_HINT → ignored (this op is reserved for higher-level
 *                       producers, never emitted by capy_widget_emit).
 *
 * Internal clip stack depth is bounded; if the depth exceeds the
 * compile-time limit the translator returns `-1` (corrupt DL: more
 * CLIP_PUSH than CLIP_POP, or deeper than the configured stack). An
 * unbalanced display-list (CLIP_POP without a matching push, or trailing
 * CLIP_PUSH at end of walk) is also rejected with `-1`.
 *
 * Returns:
 *   `0` on success, with `*out_count` set to the number of quads written
 *   into `out[]`.
 *   `-1` on overflow of `cap` (with `*out_count` set to the partial
 *   count already written; the buffer is not rolled back), `NULL dl` /
 *   `NULL out_count`, `NULL out` with `cap > 0`, malformed clip stack,
 *   or unknown op outside the schema 4 enum range.
 *
 * Deterministic: same `(dl)` → same `out[]` byte-for-byte, same
 * `*out_count`. */
#define CAPY_DL_GPU_CLIP_STACK_MAX 16u

struct capy_gpu_quad {
  int32_t x0;
  int32_t y0;
  int32_t x1;
  int32_t y1;
  uint32_t color;
  uint32_t texture_id;
  int32_t uv_x0;
  int32_t uv_y0;
  int32_t uv_x1;
  int32_t uv_y1;
};

int capy_dl_to_quads(const struct capy_display_list *dl,
                     struct capy_gpu_quad *out, uint32_t cap,
                     uint32_t *out_count);

#endif
