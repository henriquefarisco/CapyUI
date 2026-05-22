/* CapyUI state serialization (since 1.10).
 *
 * Implements `capy_widget_serialize`, `capy_widget_deserialize` and
 * `capy_widget_serialize_text` over the canonical binary format
 * documented in `capy_widget.h`. Header layout:
 *
 *   [0..3]   magic "CUIS"
 *   [4..5]   format version (uint16 LE, currently 1)
 *   [6..7]   reserved (uint16 LE, must be 0)
 *   [8..11]  FNV-1a 32-bit checksum of bytes [16, end)
 *   [12..15] node count (uint32 LE)
 *
 * Per-node layout (variable size):
 *   uint8 type, uint8 flags, int16 tab_index,
 *   int32 x, int32 y, uint32 w, uint32 h,
 *   uint16 text_len, text bytes,
 *   uint16 child_count
 *
 * No internal allocation during serialize. Deserialize allocates new
 * widgets via the caller-provided `ctx->allocator` only; on any failure
 * after partial allocation we destroy the partial subtree before
 * returning. */

#include <stddef.h>
#include <stdint.h>

#include "capy_widget.h"

/* ── Little-endian byte primitives ─────────────────────────────────────── */

static void capy_state_write_u16(uint8_t *p, uint16_t v) {
  p[0] = (uint8_t)(v & 0xFFu);
  p[1] = (uint8_t)((v >> 8) & 0xFFu);
}

static void capy_state_write_u32(uint8_t *p, uint32_t v) {
  p[0] = (uint8_t)(v & 0xFFu);
  p[1] = (uint8_t)((v >> 8) & 0xFFu);
  p[2] = (uint8_t)((v >> 16) & 0xFFu);
  p[3] = (uint8_t)((v >> 24) & 0xFFu);
}

static uint16_t capy_state_read_u16(const uint8_t *p) {
  return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static uint32_t capy_state_read_u32(const uint8_t *p) {
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
         ((uint32_t)p[3] << 24);
}

/* FNV-1a 32-bit. Deterministic, zero-state, no division — perfect for
 * blob integrity checks. */
static uint32_t capy_state_fnv1a(const uint8_t *data, uint32_t len) {
  uint32_t hash = 0x811C9DC5u;
  uint32_t i;
  for (i = 0u; i < len; ++i) {
    hash ^= (uint32_t)data[i];
    hash *= 0x01000193u;
  }
  return hash;
}

static uint32_t capy_state_node_packed_flags(const struct capy_widget *w) {
  uint32_t f = 0u;
  if (w->visible) {
    f |= CAPY_STATE_FLAG_VISIBLE;
  }
  if (w->enabled) {
    f |= CAPY_STATE_FLAG_ENABLED;
  }
  if (w->focused) {
    f |= CAPY_STATE_FLAG_FOCUSED;
  }
  if (w->focusable) {
    f |= CAPY_STATE_FLAG_FOCUSABLE;
  }
  if (w->checked) {
    f |= CAPY_STATE_FLAG_CHECKED;
  }
  return f;
}

/* ── Tree walks for sizing / counting ──────────────────────────────────── */

static uint32_t capy_state_text_byte_count(const char *text) {
  uint32_t n = 0u;
  if (!text) {
    return 0u;
  }
  while (text[n] != '\0' && n < CAPY_WIDGET_MAX_TEXT) {
    ++n;
  }
  return n;
}

static int capy_state_count_nodes(const struct capy_widget *w,
                                  uint32_t *out_count) {
  uint32_t i;
  if (!w) {
    return 0;
  }
  ++(*out_count);
  for (i = 0u; i < w->child_count; ++i) {
    if (capy_state_count_nodes(w->children[i], out_count) != 0) {
      return -1;
    }
  }
  return 0;
}

/* ── Serialize ─────────────────────────────────────────────────────────── */

static int capy_state_serialize_node(const struct capy_widget *w,
                                     uint8_t *buf, uint32_t cap,
                                     uint32_t *cursor) {
  uint32_t text_len;
  uint32_t i;
  uint8_t *p;
  if (!w) {
    return -1;
  }
  text_len = capy_state_text_byte_count(w->text);
  /* Fixed per-node overhead: 1+1+2+16+2 + text_len + 2 = 24 + text_len. */
  if ((uint64_t)*cursor + 24u + text_len > (uint64_t)cap) {
    return -1;
  }
  p = buf + *cursor;
  p[0] = (uint8_t)w->type;
  p[1] = (uint8_t)capy_state_node_packed_flags(w);
  capy_state_write_u16(&p[2], (uint16_t)w->tab_index);
  capy_state_write_u32(&p[4], (uint32_t)w->bounds.x);
  capy_state_write_u32(&p[8], (uint32_t)w->bounds.y);
  capy_state_write_u32(&p[12], w->bounds.width);
  capy_state_write_u32(&p[16], w->bounds.height);
  capy_state_write_u16(&p[20], (uint16_t)text_len);
  for (i = 0u; i < text_len; ++i) {
    p[22u + i] = (uint8_t)w->text[i];
  }
  capy_state_write_u16(&p[22u + text_len], (uint16_t)w->child_count);
  *cursor += 24u + text_len;
  for (i = 0u; i < w->child_count; ++i) {
    if (capy_state_serialize_node(w->children[i], buf, cap, cursor) != 0) {
      return -1;
    }
  }
  return 0;
}

int capy_widget_serialize(const struct capy_widget *root, void *out,
                          uint32_t cap, uint32_t *out_len) {
  uint8_t *buf;
  uint32_t cursor;
  uint32_t node_count = 0u;
  uint32_t checksum;
  if (!root || !out || !out_len) {
    return -1;
  }
  if (cap < CAPY_STATE_HEADER_SIZE) {
    *out_len = 0u;
    return -1;
  }
  buf = (uint8_t *)out;
  /* Header magic + version + reserved. Checksum and node count filled
   * after the body is written. */
  buf[0] = (uint8_t)CAPY_STATE_MAGIC0;
  buf[1] = (uint8_t)CAPY_STATE_MAGIC1;
  buf[2] = (uint8_t)CAPY_STATE_MAGIC2;
  buf[3] = (uint8_t)CAPY_STATE_MAGIC3;
  capy_state_write_u16(&buf[4], (uint16_t)CAPY_STATE_FORMAT_VERSION);
  capy_state_write_u16(&buf[6], 0u);
  /* Reserve checksum + node_count slots. */
  capy_state_write_u32(&buf[8], 0u);
  capy_state_write_u32(&buf[12], 0u);
  cursor = CAPY_STATE_HEADER_SIZE;
  if (capy_state_count_nodes(root, &node_count) != 0) {
    *out_len = 0u;
    return -1;
  }
  if (capy_state_serialize_node(root, buf, cap, &cursor) != 0) {
    *out_len = cursor;
    return -1;
  }
  capy_state_write_u32(&buf[12], node_count);
  checksum = capy_state_fnv1a(buf + CAPY_STATE_HEADER_SIZE,
                              cursor - CAPY_STATE_HEADER_SIZE);
  capy_state_write_u32(&buf[8], checksum);
  *out_len = cursor;
  return 0;
}

/* ── Deserialize ───────────────────────────────────────────────────────── */

static int capy_state_deserialize_node(const uint8_t *buf, uint32_t len,
                                       uint32_t *cursor,
                                       struct capy_widget_context *ctx,
                                       struct capy_widget **out_node) {
  uint32_t pos;
  uint32_t text_len;
  uint32_t child_count;
  uint32_t i;
  struct capy_widget *node;
  uint8_t flags;
  *out_node = 0;
  pos = *cursor;
  if (pos + 24u > len) {
    return -1;
  }
  /* Read the fixed prefix. */
  {
    uint8_t type_byte = buf[pos];
    /* Accept all widget types up to the most recently appended one.
     * Since 2.1: `CAPY_WIDGET_AUTOCOMPLETE` is the tail entry; future
     * additive bumps move this ceiling forward. Unknown (out-of-range)
     * type bytes are still rejected fail-closed. */
    if (type_byte > (uint8_t)CAPY_WIDGET_AUTOCOMPLETE) {
      return -1;
    }
    node = capy_widget_create(ctx, (enum capy_widget_type)type_byte);
    if (!node) {
      return -1;
    }
  }
  flags = buf[pos + 1u];
  node->visible = (flags & CAPY_STATE_FLAG_VISIBLE) ? 1u : 0u;
  node->enabled = (flags & CAPY_STATE_FLAG_ENABLED) ? 1u : 0u;
  node->focused = (flags & CAPY_STATE_FLAG_FOCUSED) ? 1u : 0u;
  node->focusable = (flags & CAPY_STATE_FLAG_FOCUSABLE) ? 1u : 0u;
  node->checked = (flags & CAPY_STATE_FLAG_CHECKED) ? 1u : 0u;
  node->tab_index = (int16_t)capy_state_read_u16(&buf[pos + 2u]);
  node->bounds.x = (int32_t)capy_state_read_u32(&buf[pos + 4u]);
  node->bounds.y = (int32_t)capy_state_read_u32(&buf[pos + 8u]);
  node->bounds.width = capy_state_read_u32(&buf[pos + 12u]);
  node->bounds.height = capy_state_read_u32(&buf[pos + 16u]);
  text_len = capy_state_read_u16(&buf[pos + 20u]);
  if (text_len > (uint32_t)(CAPY_WIDGET_MAX_TEXT - 1u)) {
    capy_widget_destroy(node);
    return -1;
  }
  if (pos + 22u + text_len + 2u > len) {
    capy_widget_destroy(node);
    return -1;
  }
  for (i = 0u; i < text_len; ++i) {
    node->text[i] = (char)buf[pos + 22u + i];
  }
  node->text[text_len] = '\0';
  child_count = capy_state_read_u16(&buf[pos + 22u + text_len]);
  if (child_count > CAPY_WIDGET_MAX_CHILDREN) {
    capy_widget_destroy(node);
    return -1;
  }
  *cursor = pos + 24u + text_len;
  for (i = 0u; i < child_count; ++i) {
    struct capy_widget *child = 0;
    if (capy_state_deserialize_node(buf, len, cursor, ctx, &child) != 0) {
      capy_widget_destroy(node); /* recursive destroy via parent pointer chain */
      return -1;
    }
    if (capy_widget_add_child(node, child) != 0) {
      capy_widget_destroy(child);
      capy_widget_destroy(node);
      return -1;
    }
  }
  *out_node = node;
  return 0;
}

int capy_widget_deserialize(const void *buf, uint32_t len,
                            struct capy_widget_context *ctx,
                            struct capy_widget **out_root) {
  const uint8_t *p;
  uint16_t version;
  uint16_t reserved;
  uint32_t stored_checksum;
  uint32_t computed_checksum;
  uint32_t node_count;
  uint32_t cursor;
  struct capy_widget *root = 0;
  if (!buf || !ctx || !out_root) {
    return -1;
  }
  *out_root = 0;
  if (len < CAPY_STATE_HEADER_SIZE) {
    return -1;
  }
  p = (const uint8_t *)buf;
  if (p[0] != (uint8_t)CAPY_STATE_MAGIC0 ||
      p[1] != (uint8_t)CAPY_STATE_MAGIC1 ||
      p[2] != (uint8_t)CAPY_STATE_MAGIC2 ||
      p[3] != (uint8_t)CAPY_STATE_MAGIC3) {
    return -1;
  }
  version = capy_state_read_u16(&p[4]);
  reserved = capy_state_read_u16(&p[6]);
  if (version != CAPY_STATE_FORMAT_VERSION || reserved != 0u) {
    return -1;
  }
  stored_checksum = capy_state_read_u32(&p[8]);
  node_count = capy_state_read_u32(&p[12]);
  computed_checksum =
      capy_state_fnv1a(p + CAPY_STATE_HEADER_SIZE, len - CAPY_STATE_HEADER_SIZE);
  if (stored_checksum != computed_checksum) {
    return -1;
  }
  if (node_count == 0u) {
    return -1;
  }
  cursor = CAPY_STATE_HEADER_SIZE;
  if (capy_state_deserialize_node(p, len, &cursor, ctx, &root) != 0) {
    return -1;
  }
  if (!root) {
    return -1;
  }
  /* The pre-order walk should have consumed exactly the body. Trailing
   * bytes indicate corruption or a future format extension we don't
   * understand yet — reject conservatively. */
  if (cursor != len) {
    capy_widget_destroy(root);
    return -1;
  }
  *out_root = root;
  return 0;
}

/* ── Text dump (debug) ─────────────────────────────────────────────────── */

static int capy_state_text_append(char *out, uint32_t cap, uint32_t *pos,
                                  const char *src) {
  while (*src) {
    if (*pos + 1u >= cap) {
      return -1;
    }
    out[(*pos)++] = *src++;
  }
  return 0;
}

static int capy_state_text_append_u32(char *out, uint32_t cap, uint32_t *pos,
                                      uint32_t v) {
  /* `uint32_t` fits in 10 decimal digits. */
  char tmp[11];
  uint32_t n = 0u;
  uint32_t i;
  if (v == 0u) {
    tmp[n++] = '0';
  } else {
    while (v > 0u) {
      tmp[n++] = (char)('0' + (v % 10u));
      v /= 10u;
    }
  }
  for (i = 0u; i < n; ++i) {
    if (*pos + 1u >= cap) {
      return -1;
    }
    out[(*pos)++] = tmp[n - 1u - i];
  }
  return 0;
}

static int capy_state_text_append_i32(char *out, uint32_t cap, uint32_t *pos,
                                      int32_t v) {
  if (v < 0) {
    if (*pos + 1u >= cap) {
      return -1;
    }
    out[(*pos)++] = '-';
    /* Negate carefully: INT32_MIN's negation overflows; cast through uint32_t. */
    return capy_state_text_append_u32(
        out, cap, pos, (uint32_t)(-(int64_t)v));
  }
  return capy_state_text_append_u32(out, cap, pos, (uint32_t)v);
}

static int capy_state_text_dump_node(const struct capy_widget *w, char *out,
                                     uint32_t cap, uint32_t *pos,
                                     uint32_t depth) {
  uint32_t i;
  uint32_t text_len;
  if (!w) {
    return 0;
  }
  for (i = 0u; i < depth; ++i) {
    if (*pos + 2u >= cap) {
      return -1;
    }
    out[(*pos)++] = ' ';
    out[(*pos)++] = ' ';
  }
  if (capy_state_text_append(out, cap, pos, "[type=") != 0) return -1;
  if (capy_state_text_append_u32(out, cap, pos, (uint32_t)w->type) != 0)
    return -1;
  if (capy_state_text_append(out, cap, pos, " bounds=(") != 0) return -1;
  if (capy_state_text_append_i32(out, cap, pos, w->bounds.x) != 0) return -1;
  if (capy_state_text_append(out, cap, pos, ",") != 0) return -1;
  if (capy_state_text_append_i32(out, cap, pos, w->bounds.y) != 0) return -1;
  if (capy_state_text_append(out, cap, pos, ",") != 0) return -1;
  if (capy_state_text_append_u32(out, cap, pos, w->bounds.width) != 0)
    return -1;
  if (capy_state_text_append(out, cap, pos, ",") != 0) return -1;
  if (capy_state_text_append_u32(out, cap, pos, w->bounds.height) != 0)
    return -1;
  if (capy_state_text_append(out, cap, pos, ") flags=") != 0) return -1;
  if (w->visible) {
    if (*pos + 1u >= cap) return -1;
    out[(*pos)++] = 'v';
  }
  if (w->enabled) {
    if (*pos + 1u >= cap) return -1;
    out[(*pos)++] = 'e';
  }
  if (w->focused) {
    if (*pos + 1u >= cap) return -1;
    out[(*pos)++] = 'f';
  }
  if (w->focusable) {
    if (*pos + 1u >= cap) return -1;
    out[(*pos)++] = 'F';
  }
  if (w->checked) {
    if (*pos + 1u >= cap) return -1;
    out[(*pos)++] = 'c';
  }
  if (capy_state_text_append(out, cap, pos, " text=\"") != 0) return -1;
  text_len = capy_state_text_byte_count(w->text);
  for (i = 0u; i < text_len; ++i) {
    if (*pos + 1u >= cap) return -1;
    out[(*pos)++] = w->text[i];
  }
  if (capy_state_text_append(out, cap, pos, "\" children=") != 0) return -1;
  if (capy_state_text_append_u32(out, cap, pos, w->child_count) != 0)
    return -1;
  if (capy_state_text_append(out, cap, pos, "]\n") != 0) return -1;
  for (i = 0u; i < w->child_count; ++i) {
    if (capy_state_text_dump_node(w->children[i], out, cap, pos, depth + 1u) !=
        0) {
      return -1;
    }
  }
  return 0;
}

int capy_widget_serialize_text(const struct capy_widget *root, char *out,
                               uint32_t cap) {
  uint32_t pos = 0u;
  if (!root || !out || cap == 0u) {
    return -1;
  }
  if (capy_state_text_dump_node(root, out, cap, &pos, 0u) != 0) {
    return -1;
  }
  if (pos >= cap) {
    return -1;
  }
  out[pos] = '\0';
  return (int)pos;
}
