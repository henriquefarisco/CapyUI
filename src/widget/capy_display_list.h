#ifndef CAPY_UI_DISPLAY_LIST_H
#define CAPY_UI_DISPLAY_LIST_H

#include <stddef.h>
#include <stdint.h>

#include "capy_widget.h"

#define CAPY_DISPLAY_LIST_SCHEMA_VERSION 7u

enum capy_dl_op {
  CAPY_DL_NONE = 0,
  CAPY_DL_RECT,
  CAPY_DL_BORDER,
  CAPY_DL_TEXT,
  CAPY_DL_CLIP_PUSH,
  CAPY_DL_CLIP_POP,
  CAPY_DL_IMAGE_REF,
  CAPY_DL_FOCUS_RING,
  /* Optional, since 0.8.0: explicit dirty rect hint that a producer can
   * embed inline so a compositor may skip its own diff pass. capy_widget_emit
   * itself never emits this op — it is reserved for higher-level producers
   * (e.g. the host compositor adapter) and capy_widget_diff, which writes
   * dirty rects into a caller-provided capy_ui_rect array rather than the
   * display-list itself. Consumers reading dl->version < 3 must ignore this
   * op when walking newer producers. */
  CAPY_DL_DIRTY_HINT,
  /* Since 1.5.0 (schema 5): DPI scope marker. Prepended once by
   * `capy_widget_emit` at the start of the display-list when
   * `dl->dpi_scale_x256 != 256` so a host compositor / GPU backend knows
   * which pixel scale to render the following ops at. Fields:
   *   `rect`     = scoped region (root bounds in 1.5; future minors may
   *                emit per-subtree scopes for multi-display windows).
   *   `image_id` = `dpi_scale_x256` (Q8.8 scale; e.g. 384 = 1.5×).
   *                Reuses the existing 32-bit slot to avoid growing the
   *                struct sizeof. The same value applies to font sizes.
   * Consumers reading `dl->version < 5` must ignore this op when walking
   * newer producers. */
  CAPY_DL_DPI_SCOPE,
  /* Since 1.9.0 (schema 6): 2D affine transform push/pop. `TRANSFORM_PUSH`
   * carries a `capy_dl_transform` matrix in `cmd->transform` and stacks
   * onto the renderer's transform stack; `TRANSFORM_POP` unwinds the top
   * of the stack. Producers must keep PUSH/POP balanced; consumers may
   * skip the pair (render axis-aligned) on backends that lack affine
   * support, accepting a visual-only degradation. Consumers reading
   * `dl->version < 6` must treat both ops as opaque and ignore them. */
  CAPY_DL_TRANSFORM_PUSH,
  CAPY_DL_TRANSFORM_POP,
  /* Since 2.0.0 (schema 7): plugin custom op. Carries an opaque 32-byte
   * payload in the existing `rect`/`color`/`text_offset`/`text_len`/
   * `border_width`/`font_size`/`font_id`/`image_id` slots (which togethe
   * sum to 32 bytes); the plugin renderer registered with the host
   * dispatcher decodes the bytes per its own schema. `image_id` doubles
   * as the plugin identifier (host maps it to a registered descriptor)
   * in the canonical mapping; alternative mappings are plugin-defined.
   * Consumers reading `dl->version < 7` must treat this op as opaque
   * and ignore it. `capy_widget_emit` never emits this op — it is
   * produced by host-side plugin code that walks the widget tree. */
  CAPY_DL_PLUGIN_OP
};

struct capy_dl_cmd {
  enum capy_dl_op op;
  struct capy_ui_rect rect;
  uint32_t color;
  uint16_t text_offset;
  uint16_t text_len;
  uint8_t border_width;
  uint8_t font_size;
  /* Since 0.9: font identifier referenced by the host text-metrics hook and
   * compositor font loader. Layout-compatible rename of the former 16-bit
   * reserved slot. `0` is the default font (host chooses); schema < 4
   * consumers see whatever was in `reserved` (zero for emit-produced cmds). */
  uint16_t font_id;
  uint32_t image_id;
  /* Since 1.9 (schema 6): affine transform payload, valid only fo
   * `CAPY_DL_TRANSFORM_PUSH` ops. Zero-filled for every other op (since
   * `capy_dl_push` memsets the whole cmd). Adds 24 bytes to
   * `sizeof(capy_dl_cmd)`; callers that stack-allocate arrays of cmds
   * just allocate the new size. */
  struct capy_dl_transform transform;
};

struct capy_display_list {
  struct capy_dl_cmd *cmds;
  uint32_t count;
  uint32_t capacity;
  char *text_pool;
  uint32_t text_used;
  uint32_t text_capacity;
  uint32_t version;
  /* Optional theme pointer (since 0.6). NULL means tokens resolve to literal
   * style.*_color fallbacks (backward-compat behaviour for pre-0.6 callers). */
  const struct capy_theme *theme;
  /* Since 1.5 (schema 5): caller-provided DPI scale in Q8.8 (256 = 1.0×,
   * 384 = 1.5×, 512 = 2.0×). `capy_display_list_init` seeds it to 256.
   * When non-default, `capy_widget_emit` prepends a `CAPY_DL_DPI_SCOPE`
   * op so the compositor / GPU backend can pick the correct rasterizer. */
  uint16_t dpi_scale_x256;
  uint16_t reserved_dpi;
};

void capy_display_list_init(struct capy_display_list *dl, void *cmd_buf,
                            uint32_t cmd_cap, char *text_buf,
                            uint32_t text_cap);
void capy_display_list_reset(struct capy_display_list *dl);
int capy_widget_emit(struct capy_widget *root, struct capy_display_list *dl);

/* Deterministic display-list diff (since 0.8.0).
 *
 * Walks `prev` and `next` in parallel by command index and writes the rect
 * of every differing command into `out_dirty`, plus the rects of any
 * trailing commands present only in one of the two lists. Consecutive
 * identical rects are coalesced into a single entry (stable: same input
 * always yields the same sequence of rects, byte-for-byte).
 *
 * Returns the number of rects written on success, or `-1` on overflow of
 * `out_cap`. On overflow `out_dirty` is partially filled up to capacity
 * (no rollback): the caller may treat the partial region as a conservative
 * upper bound and fall back to a full redraw.
 *
 * `prev == NULL` is treated as an empty list (all of `next` is dirty).
 * `next == NULL` is treated as an empty list (all of `prev` is dirty).
 * `out_dirty == NULL` with `out_cap > 0` returns `-1` without writing.
 * `out_cap == 0` returns `0` if both lists are empty/identical, otherwise
 * `-1` (overflow with nothing written).
 *
 * Complexity: O(max(prev->count, next->count)). Zero allocation. */
int capy_widget_diff(const struct capy_display_list *prev,
                     const struct capy_display_list *next,
                     struct capy_ui_rect *out_dirty, uint32_t out_cap);

#endif
