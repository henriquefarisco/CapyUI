#ifndef CAPY_UI_WIDGET_H
#define CAPY_UI_WIDGET_H

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include "capy_layout.h"

#define CAPY_WIDGET_MAX_TEXT 256u
#define CAPY_WIDGET_MAX_CHILDREN 32u

/* ── ABI 1.0 freeze marker ────────────────────────────────────────────────
 *
 * `capy-ui-widget` is frozen at major 1 since `1.0.0`. Pre-1.0 (0.0 .. 0.15)
 * was strictly additive — no symbol is deprecated yet. Removal of any
 * pre-1.0 surface requires a major bump per
 * `docs/roadmap/contracts/deprecation-policy.md`.
 *
 * `CAPYUI_API_VERSION_MAJOR` / `_MINOR` / `_PATCH` follow the package
 * `VERSION` file. Consumers can compile-time gate on
 * `CAPYUI_API_VERSION_MAJOR >= 1` to know they are linking the stable ABI.
 *
 * `CAPYUI_API_DEPRECATED(msg)` is the canonical macro used to mark APIs
 * for retirement in future minors per the deprecation policy. It expands
 * to a compiler attribute on GCC/Clang/MSVC and to nothing elsewhere.
 * No symbol is currently annotated; the macro is published so downstream
 * call sites can compile against it without conditional guards. */
#define CAPYUI_API_VERSION_MAJOR 2
#define CAPYUI_API_VERSION_MINOR 22
#define CAPYUI_API_VERSION_PATCH 0

/* Composite ABI tag (since 2.0) used by plugins to declare the minimum host
 * version they require. Format: `(major << 16) | (minor << 8) | patch`.
 * Plugins set `capy_plugin_descriptor::capy_ui_abi_required` to a value
 * <= this constant; older hosts reject newer plugins. */
#define CAPYUI_API_VERSION_TAG \
    ((uint32_t)CAPYUI_API_VERSION_MAJOR << 16 | \
     (uint32_t)CAPYUI_API_VERSION_MINOR << 8 | \
     (uint32_t)CAPYUI_API_VERSION_PATCH)

/* Maximum plugin descriptors per `capy_widget_context` (since 2.0). The
 * static cap keeps registration zero-alloc; hosts that need more can
 * fork the limit at compile time. */
#define CAPY_MAX_PLUGINS 8u

#if defined(__GNUC__) || defined(__clang__)
#  define CAPYUI_API_DEPRECATED(msg) __attribute__((deprecated(msg)))
#elif defined(_MSC_VER)
#  define CAPYUI_API_DEPRECATED(msg) __declspec(deprecated(msg))
#else
#  define CAPYUI_API_DEPRECATED(msg)
#endif

/* Focus key codes (since 0.3) */
#define CAPY_KEY_TAB         9u
#define CAPY_KEY_ENTER       13u
#define CAPY_KEY_ESCAPE      27u
#define CAPY_KEY_SPACE       32u
#define CAPY_KEY_ARROW_UP    0x101u
#define CAPY_KEY_ARROW_DOWN  0x102u
#define CAPY_KEY_ARROW_LEFT  0x103u
#define CAPY_KEY_ARROW_RIGHT 0x104u

/* Event modifier flags (since 0.3; expanded in 0.10) */
#define CAPY_MOD_NONE     0x0u
#define CAPY_MOD_CTRL     0x1u
#define CAPY_MOD_ALT      0x2u
#define CAPY_MOD_SHIFT    0x4u
#define CAPY_MOD_META     0x8u
#define CAPY_MOD_CAPSLOCK 0x10u /* since 0.10 */
#define CAPY_MOD_NUMLOCK  0x20u /* since 0.10 */

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
  CAPY_WIDGET_TABS,
  /* Advanced widget types (since 2.1, aditivo). These reserve enum slots
   * and integrate with the generic widget pipeline (layout, emit, focus,
   * serialize, a11y) using the existing `capy_widget` fields. Per-widget
   * state (tree hierarchy, table columns, rich-text ranges, canvas draw
   * callback, chart datasets, color/date pickers, autocomplete
   * suggestions) lives in caller-owned structures referenced via
   * `user_data` until a dedicated 2.x minor lifts each into a tail-field
   * slot. Document this expectation in the slice doc. */
  CAPY_WIDGET_TREE,
  CAPY_WIDGET_TABLE,
  CAPY_WIDGET_RICH_TEXT,
  CAPY_WIDGET_CANVAS,
  CAPY_WIDGET_CHART,
  CAPY_WIDGET_COLOR_PICKER,
  CAPY_WIDGET_DATE_PICKER,
  CAPY_WIDGET_AUTOCOMPLETE
};

enum capy_widget_event_type {
  CAPY_WIDGET_EVENT_NONE = 0,
  CAPY_WIDGET_EVENT_POINTER_MOVE,
  CAPY_WIDGET_EVENT_POINTER_DOWN,
  CAPY_WIDGET_EVENT_POINTER_UP,
  CAPY_WIDGET_EVENT_KEY_DOWN,
  CAPY_WIDGET_EVENT_KEY_UP,
  /* Aditivo since 0.10 — ops futuras só podem ser appended at the tail. */
  CAPY_WIDGET_EVENT_WHEEL,
  CAPY_WIDGET_EVENT_TOUCH_BEGIN,
  CAPY_WIDGET_EVENT_TOUCH_MOVE,
  CAPY_WIDGET_EVENT_TOUCH_END,
  CAPY_WIDGET_EVENT_GAMEPAD,
  /* Drag and drop (since 1.6, aditivo). DRAG_BEGIN/MOVE/END are emitted by
   * the host dispatcher and consumed by it for visual preview; the widget
   * core treats them as no-ops to keep the routing trivial. DROP carries a
   * `const struct capy_dnd_payload *` in `event->payload` and is routed
   * deepest-first by `capy_widget_handle_event`; the deepest widget that
   * has a registered `on_drop` callback and whose accepted-type filter
   * matches the payload type wins and its callback fires. */
  CAPY_WIDGET_EVENT_DRAG_BEGIN,
  CAPY_WIDGET_EVENT_DRAG_MOVE,
  CAPY_WIDGET_EVENT_DRAG_END,
  CAPY_WIDGET_EVENT_DROP,
  /* Rich IME (since 1.8, aditivo). Pass-through in the widget core — the
   * host engine drives composition state through the four `capy_ime_*`
   * APIs; these event types let the dispatcher signal phase transitions
   * to the rest of the tree (notifications, re-render hints) without
   * coupling to the setter calls. */
  CAPY_WIDGET_EVENT_IME_COMPOSE_START,
  CAPY_WIDGET_EVENT_IME_PREEDIT_UPDATE,
  CAPY_WIDGET_EVENT_IME_COMMIT,
  CAPY_WIDGET_EVENT_IME_CANCEL,
  /* Virtualization (since 2.2, aditivo). Emitted by the host renderer
   * when a virtualized LIST/TREE/TABLE needs the next batch of items;
   * `event->x = start_index`, `event->y = end_index_exclusive`. Hosts
   * use this to prefetch data ahead of the scroll. The widget core is
   * pass-through (return 0); the host listens and pre-populates its
   * `capy_virtual_data_source` backing store. */
  CAPY_WIDGET_EVENT_VIRTUAL_REQUEST_RANGE,
  /* Display mode changed (since 2.6). Emitted by the widget core inside
   * `capy_display_set_mode` once the host callback returns successfully.
   * `event->x` carries `(width << 16) | height`; `event->y` carries
   * `(bpp << 16) | refresh_hz_q8`. Hosts use this to schedule a full
   * compositor re-damage and to persist the chosen mode. */
  CAPY_WIDGET_EVENT_DISPLAY_MODE_CHANGED,
  /* Thumbnail ready (since 2.11). Emitted by the widget core inside
   * `capy_icon_thumbnail_ready` after the host's async pipeline has
   * decoded and registered a thumbnail bitmap. `event->x` carries the
   * request id (from the matching `_thumbnail_request` call) and
   * `event->y` carries the resolved `image_id`. Hosts route this to
   * the requesting widget so it can swap from a placeholder icon. */
  CAPY_WIDGET_EVENT_THUMBNAIL_READY
};

struct capy_ui_rect {
  int32_t x;
  int32_t y;
  uint32_t width;
  uint32_t height;
};

/* Forward declaration for APIs that accept display lists while keeping the
 * widget header independent from capy_display_list.h. */
struct capy_display_list;

/* 2x3 affine transform in Q16.16 fixed-point (since 1.9). Maps a point
 * `(x, y)` to `(m00*x + m01*y + m02, m10*x + m11*y + m12)`. `m02`/`m12`
 * are integer-pixel translations; `m00`/`m01`/`m10`/`m11` are Q16.16. */
struct capy_dl_transform {
  int32_t m00;
  int32_t m01;
  int32_t m02;
  int32_t m10;
  int32_t m11;
  int32_t m12;
};

/* 2D point (since 0.10). Used by touch payload and any future surface that
 * needs a coordinate without size information. */
struct capy_ui_point {
  int32_t x;
  int32_t y;
};

/* Payload of CAPY_WIDGET_EVENT_WHEEL (since 0.10). delta_y > 0 means scroll
 * down; delta_x > 0 means scroll right. Units are host-defined notches. */
struct capy_wheel_payload {
  int16_t delta_x;
  int16_t delta_y;
};

/* Payload of CAPY_WIDGET_EVENT_TOUCH_* (since 0.10). touch_id is stable from
 * BEGIN through MOVE to END for the same finger. pressure_x256 is the
 * normalized pressure (0..256). */
struct capy_touch_payload {
  uint32_t touch_id;
  struct capy_ui_point pos;
  uint32_t pressure_x256;
};

/* Payload of CAPY_WIDGET_EVENT_DROP (since 1.6) and the value attached to a
 * widget via `capy_widget_set_draggable`. `type` is a NUL-terminated MIME-like
 * tag with a fixed 32-byte capacity to avoid host allocation (CapyUI's widget
 * core never allocates DnD memory). `data` is host-owned and must stay valid
 * for the duration of the drag; CapyUI does not copy or free it. `len` is
 * informational — the core never reads through `data`. */
struct capy_dnd_payload {
  char type[32];
  const void *data;
  uint32_t len;
};

/* Payload of CAPY_WIDGET_EVENT_GAMEPAD (since 0.10). button_mask bit i is
 * pressed when set. axis[0..3] are left-X, left-Y, right-X, right-Y in
 * signed normalized units (-32768..32767). */
struct capy_gamepad_payload {
  uint16_t button_mask;
  int16_t axis[4];
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
  /* Theme tokens (since 0.6; 0 = use literal *_color above) */
  uint8_t bg_token;
  uint8_t fg_token;
  uint8_t border_token;
  uint8_t reserved;
};

struct capy_widget_event {
  enum capy_widget_event_type type;
  int32_t x;
  int32_t y;
  uint32_t buttons;
  uint32_t keycode;
  uint32_t modifiers;
  /* Since 0.10: optional pointer to a payload struct whose concrete type
   * is determined by `type`:
   *   WHEEL          -> const struct capy_wheel_payload *
   *   TOUCH_BEGIN/MOVE/END -> const struct capy_touch_payload *
   *   GAMEPAD        -> const struct capy_gamepad_payload *
   * NULL for legacy POINTER/KEY events. CapyUI never writes through this
   * pointer — the host owns the buffer for the duration of the call. */
  const void *payload;
};

/* Text edit state (since 0.4, valid when type==TEXTBOX) */
struct capy_text_edit {
  uint16_t caret;
  uint16_t sel_start;
  uint16_t sel_end;
  uint8_t multiline;
  uint8_t readonly;
  uint8_t password;
  uint8_t reserved;
};

/* Animation easings (since 0.5; integer-only, no float) */
enum capy_anim_easing {
  CAPY_ANIM_LINEAR = 0,
  CAPY_ANIM_EASE_IN,
  CAPY_ANIM_EASE_OUT,
  CAPY_ANIM_EASE_IN_OUT
};

/* Animation state (since 0.5; deterministic ticks, no float, no clock) */
struct capy_anim {
  uint32_t start_tick;
  uint32_t duration_ticks;
  int32_t from;
  int32_t to;
  uint8_t easing;
  uint8_t active;
  uint16_t reserved;
};

/* Rich animation (since 1.3): keyframe-driven tracks, spring physics in
 * fixed-point, and a general cubic Bezier evaluator. All integer math; no
 * float anywhere in `src/widget/`. */
struct capy_anim_keyframe {
  uint32_t tick;       /* host-provided monotonic ms tick */
  int32_t value;
  uint8_t easing;      /* `capy_anim_easing` applied when leaving this kf */
  uint8_t reserved;
  uint16_t reserved2;
};

struct capy_anim_track {
  struct capy_anim_keyframe *keyframes; /* caller-owned, sorted by tick */
  uint32_t count;
  uint32_t capacity;
  int32_t loop;        /* 0 = once (clamp), -1 = infinite, N>0 = repeat N times */
};

/* Spring physics with all state in integer fixed-point. `position` and
 * `target` are plain integer units; `velocity` is Q24.8 (8 fractional
 * bits); `stiffness` and `damping` are Q8.8 dimensionless rate constants
 * (>= 0x100 means "stiffness >= 1.0"). `capy_spring_step` integrates with
 * 1 ms substeps (`dt_ms` substeps per call) so behaviour is stable across
 * a wide range of stiffness/damping without ever calling any float op. */
struct capy_spring {
  int32_t target;
  int32_t velocity;   /* fixed-point Q24.8 */
  int32_t position;
  uint16_t stiffness; /* fixed-point Q8.8 */
  uint16_t damping;   /* fixed-point Q8.8 */
};

/* Gesture engine (since 1.4; multi-touch since 2.22). Deterministic
 * recognizer for tap / double-tap / long-press / swipe / drag (single-touch,
 * since 1.4) plus pinch-in/out and rotate CW/CCW (two-finger, since 2.22).
 * The pinch/rotate enum values were reserved in 1.4 and are now emitted by
 * the recognizer when a second finger is tracked; all detection stays
 * integer-only (pinch = signed Chebyshev-separation delta; rotate direction =
 * integer cross-product sign, significance = scale-independent |cross|/dot
 * ratio). No float anywhere.
 *
 * The recognizer is a concrete struct that the caller embeds. Zero malloc;
 * thresholds are caller-tunable after `capy_gesture_recognizer_init` seeds
 * them with safe defaults. Touch events flow in via `capy_gesture_feed`
 * which expects the host to supply a monotonic `now` (ms) because
 * `capy_widget_event` carries no timestamp. `capy_gesture_tick` lets the
 * host drive long-press detection without a new touch event. */
enum capy_gesture_kind {
  CAPY_GESTURE_NONE = 0,
  CAPY_GESTURE_TAP,
  CAPY_GESTURE_DOUBLE_TAP,
  CAPY_GESTURE_LONG_PRESS,
  CAPY_GESTURE_SWIPE_LEFT,
  CAPY_GESTURE_SWIPE_RIGHT,
  CAPY_GESTURE_SWIPE_UP,
  CAPY_GESTURE_SWIPE_DOWN,
  CAPY_GESTURE_PINCH_IN,     /* two-finger, since 2.22 (fingers approach) */
  CAPY_GESTURE_PINCH_OUT,    /* two-finger, since 2.22 (fingers spread) */
  CAPY_GESTURE_ROTATE_CW,    /* two-finger, since 2.22 (clockwise, screen y-down) */
  CAPY_GESTURE_ROTATE_CCW,   /* two-finger, since 2.22 (counter-clockwise) */
  CAPY_GESTURE_DRAG
};

struct capy_gesture {
  uint8_t kind;            /* `capy_gesture_kind` */
  uint8_t reserved[3];
  struct capy_ui_point start;
  struct capy_ui_point end;
  int32_t magnitude;       /* swipe/drag: pixels; pinch (2.22): signed separation delta px (+spread/-approach); rotate (2.22): 0 (direction-only; deg*256 reserved) */
  uint32_t duration_ms;
};

struct capy_gesture_recognizer {
  /* Thresholds (caller-tunable after init). */
  uint32_t tap_max_duration_ms;
  uint32_t tap_max_distance_px;
  uint32_t double_tap_max_gap_ms;
  uint32_t long_press_min_ms;
  uint32_t swipe_min_distance_px;
  /* Internal single-touch state. */
  uint8_t active;
  uint8_t long_press_emitted;
  uint8_t drag_emitted;
  uint8_t has_pending;
  uint8_t has_last_tap;
  uint8_t reserved[3];
  uint32_t touch_id;
  struct capy_ui_point start_pos;
  struct capy_ui_point last_pos;
  uint32_t start_tick;
  uint32_t last_tap_end_tick;
  struct capy_ui_point last_tap_pos;
  struct capy_gesture pending;     /* output queue depth 1 */
  /* Multi-touch state (since 2.22, tail additive). Tracks a second finger so
   * the recognizer can emit PINCH_IN/OUT and ROTATE_CW/CCW. `pinch_min_distance_px`
   * is caller-tunable after init (seeded to 20). The two-finger session begins
   * when a second TOUCH_BEGIN with a distinct id arrives while one finger is
   * already down; it captures the baseline separation/vector, emits each of
   * pinch/rotate at most once (one-shot, like `drag_emitted`), and ends the
   * moment any finger lifts (full reset). While a session is active the
   * single-touch tap/swipe/drag/long-press path is suppressed. */
  uint32_t pinch_min_distance_px;
  uint8_t touch2_active;
  uint8_t pinch_emitted;
  uint8_t rotate_emitted;
  uint8_t reserved2;               /* must be 0 */
  uint32_t touch2_id;
  struct capy_ui_point touch2_pos; /* current position of the 2nd finger */
  struct capy_ui_point multi_v0;   /* baseline vector (touch1 - touch2) at session start */
  int32_t multi_start_dist;        /* baseline Chebyshev separation at session start */
};

/* Locale + RTL (since 0.13) */
enum capy_plural_rule {
  CAPY_PLURAL_EN = 0, /* one (n=1), other */
  CAPY_PLURAL_PT,     /* one (n=1), other */
  CAPY_PLURAL_AR,     /* zero, one, two, few, many, other (6 forms) */
  CAPY_PLURAL_RU,     /* one, few, many, other (4 forms, CLDR) */
  CAPY_PLURAL_OTHER   /* only "other" (catch-all) */
};

struct capy_locale {
  char tag[16];        /* BCP 47, NUL-terminated (e.g. "en-US", "pt-BR") */
  uint8_t rtl;         /* 1 means right-to-left layout */
  uint8_t plural_rule; /* one of enum capy_plural_rule */
  uint16_t reserved;
};

/* Theme tokens (since 0.6) */
enum capy_token {
  CAPY_TOKEN_NONE = 0,
  CAPY_TOKEN_BG_BASE,
  CAPY_TOKEN_BG_RAISED,
  CAPY_TOKEN_BG_SUNKEN,
  CAPY_TOKEN_FG_PRIMARY,
  CAPY_TOKEN_FG_MUTED,
  CAPY_TOKEN_FG_INVERSE,
  CAPY_TOKEN_ACCENT,
  CAPY_TOKEN_ACCENT_HOVER,
  CAPY_TOKEN_BORDER,
  CAPY_TOKEN_BORDER_FOCUS,
  CAPY_TOKEN_FOCUS_RING,
  CAPY_TOKEN_DANGER,
  CAPY_TOKEN_WARNING,
  CAPY_TOKEN_SUCCESS,
  CAPY_TOKEN_INFO,
  CAPY_TOKEN_DISABLED,
  CAPY_TOKEN_COUNT
};

#define CAPY_THEME_VARIANT_LIGHT               0u
#define CAPY_THEME_VARIANT_DARK                1u
#define CAPY_THEME_VARIANT_HIGH_CONTRAST       2u
#define CAPY_THEME_VARIANT_DARK_HIGH_CONTRAST  3u /* since 2.8 */

/* Theme state (since 0.6; standalone struct stored in widget context) */
struct capy_theme {
  uint32_t tokens[CAPY_TOKEN_COUNT];
  uint8_t variant;
  uint8_t high_contrast;
  uint16_t version;
};

struct capy_widget;

typedef void (*capy_widget_callback)(struct capy_widget *widget,
                                     void *user_data);
typedef void *(*capy_widget_alloc_fn)(size_t size, void *user_data);
typedef void (*capy_widget_free_fn)(void *ptr, void *user_data);

/* Host-provided text metrics hook (since 0.9).
 *
 * Called by `capy_widget_measure_text` (and indirectly by any layout pass
 * that needs to size text against a real font). Returns `0` on success and
 * fills `*out_width`/`*out_height` with the rendered extent in display
 * units. Returns `-1` on failure (the caller then uses the deterministic
 * fallback). The hook must be read-only on `text` and must not allocate
 * on the hot path. `font_id == 0` is always valid and means "default font". */
typedef int (*capy_text_metrics_fn)(uint16_t font_id, uint8_t font_size,
                                    const char *text, uint16_t len,
                                    uint32_t *out_width, uint32_t *out_height,
                                    void *user_data);

/* Host-provided canvas draw callback (since 2.21).
 *
 * Stored on a CANVAS widget and invoked by `capy_widget_canvas_draw` when the
 * host builds a frame. The callback appends ops to the caller-provided `dl`
 * (e.g. RECT/BORDER/TEXT within `w->bounds`) to render custom content. The
 * widget core never calls it during `capy_widget_emit`, so emit stays
 * self-contained and deterministic. Returns `0` on success, non-zero on
 * failure (CapyUI never interprets the value beyond zero/non-zero). The
 * callback must be deterministic for a given `(w, dl, user_data)` to preserve
 * the display-list determinism contract, must be read-only on `w`'s tree
 * structure, and must not allocate through CapyUI. */
typedef int (*capy_canvas_draw_fn)(struct capy_widget *w,
                                   struct capy_display_list *dl,
                                   void *user_data);

struct capy_widget_allocator {
  capy_widget_alloc_fn alloc;
  capy_widget_free_fn free;
  void *user_data;
};

/* Date picker value (since 2.14, aditivo). Embedded by value at the tail of
 * `capy_widget` and meaningful only when `type == CAPY_WIDGET_DATE_PICKER`.
 * A zeroed value (`year == 0 || month == 0 || day == 0`) is the "unset"
 * sentinel that every freshly-created widget starts in. Stored as a plain
 * proleptic-Gregorian calendar date; the widget core validates it (month
 * 1..12, day within the month including Feb 29 on leap years) and never
 * interprets time zones, locales or any clock. */
struct capy_date {
  uint16_t year;  /* 1..65535; 0 = unset */
  uint8_t month;  /* 1..12; 0 = unset */
  uint8_t day;    /* 1..31, calendar-validated; 0 = unset */
};

/* Rich-text style flags (since 2.20). Bit mask carried in
 * `capy_text_range.flags`; combine with bitwise OR. `NONE` (0) inherits the
 * widget's base style. New flags must append at higher bits (additive). */
#define CAPY_TEXT_STYLE_NONE          0x0u
#define CAPY_TEXT_STYLE_BOLD          0x1u
#define CAPY_TEXT_STYLE_ITALIC        0x2u
#define CAPY_TEXT_STYLE_UNDERLINE     0x4u
#define CAPY_TEXT_STYLE_STRIKETHROUGH 0x8u

/* Rich-text style range (since 2.20). A single attributed run over a
 * RICH_TEXT widget's text: characters in the half-open interval
 * `[start, start + length)` render with `flags` (a CAPY_TEXT_STYLE_* mask)
 * and `color` (0xAARRGGBB; 0 = inherit the widget/theme colour). Ranges are
 * caller-owned (see `capy_widget.rich_text_ranges`); the widget core never
 * interprets the underlying text, so `start`/`length` are in whatever unit
 * the host counts in (bytes or codepoints) — CapyUI only does bounds
 * arithmetic. A zero-`length` run covers nothing. `reserved` must be 0
 * (future use, e.g. a font_id channel). */
struct capy_text_range {
  uint32_t start;
  uint32_t length;
  uint16_t flags;
  uint16_t reserved;
  uint32_t color;
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
  struct capy_layout_node layout;
  struct capy_widget_allocator allocator;
  uint8_t focusable;
  int16_t tab_index;
  struct capy_text_edit text_edit;
  /* Since 0.9: font identifier propagated into CAPY_DL_TEXT cmds. `0` is the
   * default font (host chooses). Layout-compatible additive append. */
  uint16_t font_id;
  /* Since 0.15: measure-cache tracking. `layout_dirty` defaults to `1` after
   * `capy_widget_create` so the first measure always computes. `capy_widget_
   * invalidate` walks up to the root setting the flag. `layout_version` is a
   * monotonic counter incremented every time a real recompute runs. */
  uint8_t layout_dirty;
  uint32_t layout_version;
  /* Since 1.1: monotonic counter bumped by every `capy_widget_invalidate` or
   * `capy_widget_invalidate_subtree` call that touches this widget. Hosts
   * compare snapshots to detect whether a subtree was invalidated between
   * frames, independently of whether `measure` actually recomputed bounds. */
  uint32_t dirty_version;
  /* Drag and drop (since 1.6, aditivo, tail). All NULL/zero by default so
   * pre-1.6 widgets are neither draggable nor drop-targets and observe
   * identical behaviour. `drag_payload` makes the widget draggable; the
   * payload is caller-owned and must outlive the drag operation. The
   * drop side stores a caller-owned array of accepted type patterns
   * (wildcard, text-prefix, or exact `"text/plain"`) plus a callback fired by
   * `capy_widget_handle_event` when a DROP event lands. */
  const struct capy_dnd_payload *drag_payload;
  const char * const *drop_accepted_types;
  uint32_t drop_types_count;
  int (*on_drop)(struct capy_widget *target,
                 const struct capy_dnd_payload *payload, void *user_data);
  void *drop_user_data;
  /* Rich IME state (since 1.8, aditivo, tail). Valid only when
   * `type == CAPY_WIDGET_TEXTBOX`. All NULL/zero by default so pre-1.8
   * textboxes behave identically. The host IME engine drives this state
   * through the `capy_ime_*` APIs (set_preedit, set_candidates, commit,
   * cancel); CapyUI does not allocate or own any of the pointed-at
   * memory. `ime_composition_phase`: 0 = idle, 1 = composing, 2 =
   * selecting. */
  const char *ime_preedit;
  uint16_t ime_preedit_len;
  uint16_t ime_cursor_pos;
  const char *const *ime_candidates;
  uint16_t ime_candidate_count;
  uint16_t ime_selected_index;
  uint8_t ime_composition_phase;
  uint8_t ime_reserved[3];
  /* 2D affine transform (since 1.9, aditivo, tail). When non-NULL, the
   * widget's emit prepends a `CAPY_DL_TRANSFORM_PUSH` carrying this
   * matrix and appends a matching `CAPY_DL_TRANSFORM_POP`. The transform
   * pointer is caller-owned and must outlive the next `capy_widget_emit`.
   * NULL = identity (no push/pop emitted; output byte-equivalent to
   * pre-1.9 for the same widget tree). */
  const struct capy_dl_transform *transform;
  /* Virtual data source (since 2.2, aditivo, tail). Caller-owned
   * pointer. NULL = the widget renders all its children eagerly
   * (pre-2.2 behaviour). When non-NULL, the host renderer queries
   * `get_count` and `get_item` for the visible viewport range instead
   * of walking `children[]`. The widget core stores the pointer and
   * exposes accessor helpers; the actual viewport-only emit rewiring
   * lives in the host LIST/TREE/TABLE renderers and lands in a follow-up
   * fatia. */
  const struct capy_virtual_data_source *virtual_source;
  /* Date picker value (since 2.14, aditivo, tail). Valid only when
   * `type == CAPY_WIDGET_DATE_PICKER`. Zero-initialised by
   * `capy_widget_create` to the unset sentinel; mutated only through
   * `capy_widget_set_date` / `capy_widget_clear_date` and read through
   * `capy_widget_get_date`. This is the first "advanced widget" family
   * (enum-only since 2.1) lifted from caller-owned `user_data` into a
   * dedicated tail slot. Callers that kept the value in `user_data`
   * pre-2.14 are unaffected: the field simply stays unset. */
  struct capy_date date_value;
  /* Color picker value (since 2.15, aditivo, tail). Valid only when
   * `type == CAPY_WIDGET_COLOR_PICKER`. `picker_color` is 0xAARRGGBB (same
   * channel order as `capy_widget_style.*_color` and the theme tokens).
   * Because every 32-bit value is a valid colour there is no natural "unset"
   * sentinel, so `picker_color_set` (0 = unset, the create-time default;
   * 1 = a colour was stored) tracks presence. Mutated only through
   * `capy_widget_set_color` / `capy_widget_clear_color`; read through
   * `capy_widget_get_color`. Second advanced-widget family (enum-only since
   * 2.1) lifted from caller-owned `user_data` into a dedicated tail slot. */
  uint32_t picker_color;
  uint8_t picker_color_set;
  /* Table columns (since 2.16, aditivo, tail). Valid only when
   * `type == CAPY_WIDGET_TABLE`. `table_column_widths` is a caller-owned
   * array of `table_column_count` column widths in pixels; CapyUI never
   * copies, allocates or frees it — the pointer must outlive the widget's
   * use of it. NULL/0 (the create-time default) means "no column model
   * set". Third advanced-widget family lifted from caller-owned `user_data`
   * into a tail slot; mutated through `capy_widget_set_table_columns` /
   * `capy_widget_clear_table_columns`, read through
   * `capy_widget_table_column_count` / `capy_widget_table_column_width`. */
  const uint16_t *table_column_widths;
  uint16_t table_column_count;
  /* Autocomplete suggestions (since 2.17, aditivo, tail). Valid only when
   * `type == CAPY_WIDGET_AUTOCOMPLETE`. `autocomplete_items` is a caller-owned
   * array of `autocomplete_count` NUL-terminated C-strings (same caller-owned,
   * never-copied idiom as `drop_accepted_types` and the IME candidate list);
   * the pointer must outlive the widget's use. NULL/0 (create-time default)
   * means "no suggestion list". `autocomplete_selected` is the highlighted
   * entry index, or -1 for "none"; it is clamped against the live count on read
   * so a stale selection never points past the list. Fourth advanced-widget
   * family lifted from caller-owned `user_data` into tail slots; the first to
   * carry a mutable navigation cursor alongside a caller-owned model. */
  const char *const *autocomplete_items;
  uint16_t autocomplete_count;
  int32_t autocomplete_selected;
  /* Tree hierarchy state (since 2.18, aditivo, tail). Valid only when
   * `type == CAPY_WIDGET_TREE`. `tree_collapsed` is the per-node fold state
   * (1 = children hidden); the create-time default of 0 means "expanded", so
   * a fresh node shows its children. `tree_depth` is a caller-set indentation
   * level (0 = root row) for flattened/virtualized tree models where the
   * visual nesting need not match the widget tree. Fifth advanced-widget
   * family; the first whose read side walks the widget hierarchy
   * (`capy_widget_tree_row_visible` checks TREE ancestors). Mutated through
   * `capy_widget_set_tree_collapsed` / `capy_widget_set_tree_depth`. */
  uint8_t tree_collapsed;
  uint16_t tree_depth;
  /* Chart dataset (since 2.19, aditivo, tail). Valid only when
   * `type == CAPY_WIDGET_CHART`. `chart_values` is a caller-owned array of
   * `chart_count` signed data points (same caller-owned, never-copied idiom as
   * the table widths / autocomplete list); the pointer must outlive the
   * widget's use. NULL/0 (create-time default) means "no data". Sixth
   * advanced-widget family; the first carrying a numeric reduction
   * (`capy_widget_chart_range` scans the dataset for its signed min/max). */
  const int32_t *chart_values;
  uint16_t chart_count;
  /* Rich-text style ranges (since 2.20, aditivo, tail). Valid only when
   * `type == CAPY_WIDGET_RICH_TEXT`. `rich_text_ranges` is a caller-owned
   * array of `rich_text_range_count` attributed runs (same caller-owned,
   * never-copied idiom as the table widths / chart values); the pointer must
   * outlive the widget's use. NULL/0 (create-time default) means "no styling"
   * — the text renders in the widget's base style. Seventh advanced-widget
   * family; the first carrying a per-offset lookup
   * (`capy_widget_rich_text_range_at` finds the run covering a character
   * offset, last-match-wins). */
  const struct capy_text_range *rich_text_ranges;
  uint16_t rich_text_range_count;
  /* Canvas draw callback (since 2.21, aditivo, tail). Valid only when
   * `type == CAPY_WIDGET_CANVAS`. `canvas_draw` is a caller-owned host
   * callback (NULL = no custom drawing, the create-time default) and
   * `canvas_user_data` is the opaque pointer handed back to it. Eighth and
   * last advanced-widget family lifted into tail slots — the first carrying a
   * behaviour (a callback) rather than a data model; invoked through
   * `capy_widget_canvas_draw`, which the host calls while building a frame.
   * The widget core never invokes it during `capy_widget_emit`. */
  capy_canvas_draw_fn canvas_draw;
  void *canvas_user_data;
};

struct capy_widget_context {
  uint32_t next_id;
  struct capy_widget_allocator allocator;
  struct capy_theme theme;
  /* Since 0.9: optional text metrics hook used by capy_widget_measure_text.
   * NULL means "use the deterministic fallback". */
  capy_text_metrics_fn text_metrics_fn;
  void *text_metrics_user;
  /* Since 0.13: locale + RTL state. capy_widget_context_init seeds this with
   * `capy_locale_default()` ("en-US", LTR, EN plural rule). */
  struct capy_locale locale;
  /* Since 1.1: host-provided hint about the per-frame time budget in
   * microseconds. CapyUI itself does not enforce this — it merely stores the
   * value so hosts can query and degrade non-essential work cooperatively.
   * `0` means "no hint" (the default). */
  uint32_t frame_budget_microseconds;
  /* Since 1.5: monotonic DPI scale in Q8.8 (256 = 1.0×, 384 = 1.5×,
   * 512 = 2.0×). Seeded to `256` by `capy_widget_context_init`. The host
   * mirrors this onto `capy_display_list.dpi_scale_x256` before emit when
   * it wants the compositor to honour scaling. Multi-display hosts keep
   * one context per display so each tree carries an independent scale. */
  uint16_t dpi_scale_x256;
  uint16_t reserved_dpi;
  /* Plugin registry (since 2.0). `plugins` stores caller-owned
   * `capy_plugin_descriptor *` pointers; CapyUI does not copy the
   * descriptors themselves. `plugin_count` is the live length and
   * never exceeds `CAPY_MAX_PLUGINS`. The widget core never invokes
   * the plugin callbacks; the host dispatcher iterates the registry
   * and applies its own sandbox (timeout / signal handling). */
  const struct capy_plugin_descriptor *plugins[CAPY_MAX_PLUGINS];
  uint32_t plugin_count;
  /* Undo/redo stack (since 2.3, aditivo, tail). NULL until
   * `capy_undo_stack_init` is called on the context with a caller-owned
   * buffer. Once initialised, points into that buffer; CapyUI never
   * allocates the stack itself. */
  struct capy_undo_stack *undo_stack;
  /* Display mode controller (since 2.6, aditivo, tail). Caller-owned
   * pointer to a `capy_display_controller` carrying enum/set callbacks
   * and the cached current mode. NULL = no display control surface;
   * the three `capy_display_*` APIs degrade gracefully. */
  struct capy_display_controller *display_controller;
  /* User directory (since 2.7, aditivo, tail). Caller-owned pointer
   * to a `capy_user_directory` carrying user list/create/update/delete/
   * set_avatar callbacks. NULL = no user management surface; the five
   * `capy_user_*` APIs degrade to `-1`. */
  struct capy_user_directory *user_directory;
  /* Icon & thumbnail provider (since 2.11, aditivo, tail). Caller-owned
   * pointer to a `capy_icon_provider` carrying mime/extension resolve
   * + async thumbnail request callbacks. NULL = no provider; `icon_*`
   * APIs degrade to the deterministic `image_id=0` fallback. */
  struct capy_icon_provider *icon_provider;
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

/* Focus traversal (since 0.3) */
void capy_widget_set_focusable(struct capy_widget *widget, int focusable,
                               int16_t tab_index);
struct capy_widget *capy_widget_focus_next(struct capy_widget *root,
                                           struct capy_widget *current);
struct capy_widget *capy_widget_focus_prev(struct capy_widget *root,
                                           struct capy_widget *current);
void capy_widget_clear_focus(struct capy_widget *root);
struct capy_widget *capy_widget_get_focused(struct capy_widget *root);
int capy_widget_dispatch_key(struct capy_widget *root,
                             const struct capy_widget_event *event);

/* Text editing (since 0.4) */
int capy_textbox_insert(struct capy_widget *widget, const char *utf8,
                        uint16_t len);
int capy_textbox_delete(struct capy_widget *widget, int16_t count);
void capy_textbox_set_selection(struct capy_widget *widget, uint16_t start,
                                uint16_t end);
int capy_textbox_copy(struct capy_widget *widget, char *out, uint16_t out_cap);
int capy_widget_ime_compose(struct capy_widget *widget, const char *preedit,
                            uint16_t len);

/* Animation (since 0.5) */
void capy_anim_start(struct capy_anim *a, uint32_t now, uint32_t duration,
                     int32_t from, int32_t to, uint8_t easing);
int capy_anim_sample(const struct capy_anim *a, uint32_t now, int32_t *out);
void capy_widget_tick(struct capy_widget *root, uint32_t now);

/* Rich animation (since 1.3).
 *
 * `capy_anim_track_sample(track, now, &out)` returns `0` on success with
 * `*out` set to the interpolated value at `now`. Keyframes are assumed
 * sorted ascending by `tick` (callers own the array; CapyUI does not
 * mutate it). Loop policy:
 *   loop == 0  → play once, clamp before first and after last keyframe.
 *   loop == -1 → loop infinitely; `now` past the last keyframe wraps modulo
 *                the track duration.
 *   loop > 0  → loop `N` times then clamp to the last keyframe value.
 * Easing per segment uses the `easing` field of the **earlier** keyframe.
 * Returns `-1` on NULL args or `count == 0`. Determinism: same `(now,
 * track)` → same `*out`.
 *
 * `capy_spring_step(s, dt_ms)` advances the spring state by `dt_ms`
 * milliseconds using 1 ms symplectic Euler substeps. With `damping > 0`
 * the system converges to `target`; with `damping == 0` it oscillates
 * indefinitely. `dt_ms == 0` is a no-op. The integer integration uses
 * `int64_t` intermediates and clamps to `int32_t` for `velocity` and
 * `position`.
 *
 * `capy_anim_bezier_eval(p0, p1, p2, p3, t_q16)` evaluates a 1D cubic
 * Bezier curve B(t) = (1-t)^3·P0 + 3(1-t)^2·t·P1 + 3(1-t)·t^2·P2 + t^3·P3
 * with control points in the same units as the output. `t_q16` is
 * Q16.16 (clamped to [0, 1]). Implemented via de Casteljau using only
 * integer arithmetic. Monotonic for well-formed (monotonically ordered)
 * control points. */
int capy_anim_track_sample(const struct capy_anim_track *track, uint32_t now,
                           int32_t *out);
void capy_spring_step(struct capy_spring *s, uint32_t dt_ms);
int32_t capy_anim_bezier_eval(int32_t p0, int32_t p1, int32_t p2, int32_t p3,
                              int32_t t_q16);

/* Gesture engine (since 1.4).
 *
 * `capy_gesture_recognizer_init(r)` zero-initializes the state and seeds
 * the thresholds with sensible defaults (tap 200 ms / 10 px; double-tap
 * gap 300 ms; long-press 500 ms; swipe 50 px). Callers may overwrite the
 * threshold fields afterwards.
 *
 * `capy_gesture_feed(r, ev, now)` consumes a single widget event with the
 * host-provided monotonic `now`. Only TOUCH_BEGIN/MOVE/END are acted upon;
 * other event types return `0`. Returns `1` if a gesture was queued in
 * `r->pending`, `0` if the event only updated state, `-1` on NULL args.
 * A second BEGIN with a different `touch_id` while a touch is already active
 * opens a two-finger session (since 2.22): subsequent MOVEs on either finger
 * may queue PINCH_IN/OUT or ROTATE_CW/CCW (each at most once per session),
 * the single-touch path is suppressed, and any END ends the session and
 * resets the recognizer. A third concurrent finger is ignored.
 *
 * `capy_gesture_tick(r, now)` lets the host drive LONG_PRESS detection
 * without a new touch event. Returns `1` if LONG_PRESS was queued, `0`
 * otherwise, `-1` on NULL.
 *
 * `capy_gesture_poll(r, &out)` consumes the pending gesture (if any).
 * Returns `1` and copies into `out` on success, `0` if nothing pending,
 * `-1` on NULL. */
void capy_gesture_recognizer_init(struct capy_gesture_recognizer *r);
int capy_gesture_feed(struct capy_gesture_recognizer *r,
                      const struct capy_widget_event *ev, uint32_t now);
int capy_gesture_tick(struct capy_gesture_recognizer *r, uint32_t now);
int capy_gesture_poll(struct capy_gesture_recognizer *r,
                      struct capy_gesture *out);

/* Multi-display & DPI scaling (since 1.5).
 *
 * `capy_widget_set_dpi_scale(ctx, scale_x256)` stores the host-provided
 * Q8.8 scale on the context. Values below `1` are clamped up to `1` to
 * avoid degenerate dimension math (a real display always has scale > 0);
 * there is no upper clamp because hosts may want experimental >4× scales.
 * `capy_widget_get_dpi_scale(ctx)` returns the stored value (or `256` if
 * `ctx == NULL`).
 *
 * `capy_widget_dpi_scale_dim(scale_x256, value)` is a pure helper that
 * scales `value * scale_x256 / 256` using `int64_t` intermediates and
 * truncates toward zero. It is the canonical way for host code to scale
 * its own padding, gap, min/max constants deterministically; CapyUI's
 * internal `measure`/`arrange` does **not** auto-scale in 1.5, so existing
 * 0.x..1.4 layouts retain byte-identical output at `scale_x256 == 256`.
 * Deeper integration into the layout walker is reserved for a future
 * additive minor; the helper plus the `CAPY_DL_DPI_SCOPE` marker carried
 * on the display-list are enough to drive a DPI-aware compositor today.
 *
 * Determinism: pure integer arithmetic; same `(scale, value)` → same
 * return on every platform. */
void capy_widget_set_dpi_scale(struct capy_widget_context *ctx,
                               uint16_t scale_x256);
uint16_t capy_widget_get_dpi_scale(const struct capy_widget_context *ctx);
int32_t capy_widget_dpi_scale_dim(uint16_t scale_x256, int32_t value);

/* Drag and drop (since 1.6).
 *
 * Widget-side configuration (all setters NULL-safe on the widget argument):
 *  - `capy_widget_set_draggable(w, payload)` attaches a caller-owned payload
 *    pointer that turns `w` into a drag source. Pass `NULL` to clear.
 *  - `capy_widget_set_drop_target(w, types, count)` stores a caller-owned
 *    array of accepted type patterns ("*" wildcard, text-prefix, or
 *    exact "text/plain"). The array must outlive the widget; CapyUI does
 *    not copy it.
 *  - `capy_widget_set_on_drop(w, fn, user_data)` registers the callback
 *    fired when a DROP lands on this widget and the payload type matches.
 *    Callbacks return `1` to signal acceptance (event stops propagating)
 *    or `0` to keep bubbling.
 *
 * Matching helpers (case-insensitive ASCII; UTF-8 above 0x7F is compared
 * byte-for-byte):
 *  - `capy_dnd_type_matches(accepted, type)` returns `1` iff `type` matches
 *    the `accepted` pattern. `"*"` accepts anything; a prefix wildcard accepts
 *    anything starting with `"prefix/"`; otherwise exact match.
 *  - `capy_widget_dnd_accepts(w, payload)` returns `1` iff `w` is a drop
 *    target whose filter accepts `payload`.
 *
 * Event routing:
 *  - `CAPY_WIDGET_EVENT_DROP` carries `const struct capy_dnd_payload *` in
 *    `event->payload` and is routed deepest-first by
 *    `capy_widget_handle_event`. The first widget under `(event->x,
 *    event->y)` that accepts the payload AND has `on_drop` set wins.
 *  - `CAPY_WIDGET_EVENT_DRAG_BEGIN/MOVE/END` are passed through silently
 *    (no widget-core mutation); the host dispatcher owns the drag preview. */
void capy_widget_set_draggable(struct capy_widget *w,
                               const struct capy_dnd_payload *payload);
void capy_widget_set_drop_target(struct capy_widget *w,
                                 const char * const *accepted_types,
                                 uint32_t count);
void capy_widget_set_on_drop(
    struct capy_widget *w,
    int (*fn)(struct capy_widget *target,
              const struct capy_dnd_payload *payload, void *user_data),
    void *user_data);
int capy_dnd_type_matches(const char *accepted, const char *type);
int capy_widget_dnd_accepts(const struct capy_widget *w,
                            const struct capy_dnd_payload *payload);

/* Slab allocator (since 1.7).
 *
 * Caller-owned fixed-size-element allocator on top of a caller-provided
 * byte buffer. Zero internal malloc; deterministic LIFO reuse so identical
 * `(init, alloc/free sequence)` always returns identical pointer
 * addresses. The free-list lives inline in the freed elements (so
 * `element_size` must be >= `sizeof(void *)`); a degenerate `_init` with
 * a smaller element size or a NULL/zero buffer leaves the slab in a
 * state where every `_alloc` returns `NULL`.
 *
 *  - `capy_slab_init(s, buf, size, elem_size)` partitions `buf` into
 *    `floor(size / elem_size)` slots and resets the free-list and high
 *    water mark to zero.
 *  - `capy_slab_alloc(s)` first pops from the free-list (LIFO); if empty
 *    and `high_water < capacity`, advances `high_water` and returns the
 *    next slot; otherwise returns `NULL`.
 *  - `capy_slab_free(s, ptr)` pushes `ptr` onto the free-list. The caller
 *    is responsible for passing only pointers previously returned by
 *    `capy_slab_alloc` from the same slab; CapyUI does not validate the
 *    range to keep `free` O(1).
 *
 * Companion to the bump allocator (`capy_widget_pool`, since 0.15) — slab
 * is preferable for tight loops with frequent free/reuse, bump is
 * preferable for monotonically-growing arenas. Both are zero-alloc. */
struct capy_slab {
  void *buffer;
  uint32_t size;
  uint32_t element_size;
  uint32_t high_water;
  void *free_list;
};

void capy_slab_init(struct capy_slab *s, void *buf, uint32_t size,
                    uint32_t element_size);
void *capy_slab_alloc(struct capy_slab *s);
void capy_slab_free(struct capy_slab *s, void *ptr);

/* Rich IME (since 1.8).
 *
 * The widget core stores IME composition state (preedit text, candidate
 * list, cursor position, composition phase) on the textbox widget but
 * does NOT implement an IME engine itself. Host engines (mozc, anthy,
 * hangul-engine, libsamantha, etc.) drive this state through the four
 * setter APIs; CapyUI does not allocate, copy or free any of the
 * pointed-at memory. The caller must keep `text`, `final_text` and
 * `list` alive until the next `capy_ime_*` call clears or replaces them.
 *
 *  - `capy_ime_set_preedit(w, text, len, cursor)` stores the in-progress
 *    composition text and cursor position (byte offset within `text`).
 *    Transitions `ime_composition_phase` to `1` (composing). No-op when
 *    `w` is not a `CAPY_WIDGET_TEXTBOX`.
 *  - `capy_ime_set_candidates(w, list, count, selected)` stores the
 *    caller-owned candidate array. Transitions phase to `2` (selecting)
 *    when `count > 0`; with `count == 0` the candidates are cleared but
 *    the existing phase is preserved.
 *  - `capy_ime_commit(w, final_text, len)` inserts `final_text` into the
 *    textbox via `capy_textbox_insert` and clears the IME state
 *    (`ime_composition_phase = 0`, preedit and candidates cleared).
 *    Returns the result of `capy_textbox_insert` (`0` on success, `-1`
 *    on rejection / non-TEXTBOX).
 *  - `capy_ime_cancel(w)` clears the IME state without inserting; the
 *    underlying textbox content is untouched.
 *  - `capy_ime_get_state(w, out)` snapshots the IME fields into `out`
 *    for hosts that want to render a candidate window. Zero-fills `out`
 *    when `w` is not a TEXTBOX. NULL-safe on both arguments.
 *
 * Coexists with the pre-1.8 stub `capy_widget_ime_compose` (since 0.4),
 * which is left in place for callers that only need minimal preedit
 * forwarding without candidate handling. */
struct capy_ime_state {
  const char *preedit;
  uint16_t preedit_len;
  uint16_t cursor_pos;
  const char *const *candidates;
  uint16_t candidate_count;
  uint16_t selected_index;
  uint8_t composition_phase;
  uint8_t reserved[3];
};

void capy_ime_set_preedit(struct capy_widget *w, const char *text,
                          uint16_t len, uint16_t cursor);
void capy_ime_set_candidates(struct capy_widget *w, const char *const *list,
                             uint16_t count, uint16_t selected);
int capy_ime_commit(struct capy_widget *w, const char *final_text,
                    uint16_t len);
void capy_ime_cancel(struct capy_widget *w);
void capy_ime_get_state(const struct capy_widget *w,
                        struct capy_ime_state *out);

/* 2D transforms (since 1.9).
 *
 * `struct capy_dl_transform` is the affine matrix declared in
 * `capy_display_list.h`. CapyUI ships pure helpers to build common
 * matrices in Q16.16 fixed-point and a deterministic matrix multiply.
 *
 *  - `capy_transform_identity()` returns the identity matrix.
 *  - `capy_transform_scale(sx_q16, sy_q16)` builds a diagonal scale; the
 *    arguments are Q16.16 (`0x10000` = 1.0).
 *  - `capy_transform_translate(tx, ty)` builds a pure translation; `tx`
 *    and `ty` are integer pixels (NOT Q16.16), matching the `m02`/`m12`
 *    convention.
 *  - `capy_transform_rotate_quadrants(n)` builds an exact 90°×n
 *    rotation (n is reduced modulo 4). Useful for snap-to-orthogonal
 *    rotations without an FP library. Arbitrary-angle rotation is
 *    intentionally **not provided in 1.9**: hosts that need it must
 *    compute `sin`/`cos` themselves (via a host-side LUT or CORDIC) and
 *    populate the matrix directly. This keeps the widget core float-free
 *    and free of any `sin`/`cos` LUT.
 *  - `capy_transform_multiply(a, b)` returns `a * b`, computed in Q16.16
 *    with `int64_t` intermediates and saturating clamps to `INT32_MAX/MIN`.
 *    Identity is a left and right neutral for multiply.
 *  - `capy_widget_set_transform(w, t)` attaches a caller-owned matrix to
 *    a widget. `NULL` clears (back to identity — no PUSH/POP emitted).
 *
 * Hit-test compensation (`capy_widget_handle_event` reading event
 * coordinates through the inverse transform) is **deferred** to a
 * future minor. In 1.9 the widget bounds remain axis-aligned for hit
 * routing; the matrix is metadata for the renderer. */
struct capy_dl_transform capy_transform_identity(void);
struct capy_dl_transform capy_transform_scale(int32_t sx_q16, int32_t sy_q16);
struct capy_dl_transform capy_transform_translate(int32_t tx, int32_t ty);
struct capy_dl_transform capy_transform_rotate_quadrants(uint32_t n);
struct capy_dl_transform capy_transform_multiply(
    const struct capy_dl_transform *a, const struct capy_dl_transform *b);
void capy_widget_set_transform(struct capy_widget *w,
                               const struct capy_dl_transform *t);

/* Virtualization (since 2.2).
 *
 * Lets a LIST / TREE / TABLE widget pretend it owns millions of items
 * without materialising them as `capy_widget` children. The data source
 * is two caller-owned function pointers plus an opaque `user_data`
 * cookie; the widget core stores the bundle and provides typed
 * accessors. The actual viewport-only emit lives in the host renderer
 * (the widget core's `capy_widget_emit_recursive` continues to walk
 * `children[]` for backwards-compatible eager renders) and will land
 * in a follow-up fatia together with row recycling.
 *
 *  - `capy_widget_set_virtual_source(w, src)` stores `src` on the
 *    widget. `NULL` clears (widget reverts to eager rendering). The
 *    pointer is caller-owned and must outlive the next emit.
 *  - `capy_widget_virtual_count(w)` returns `src->get_count(user_data)`
 *    or `-1` when no source is attached / either pointer is NULL.
 *  - `capy_widget_virtual_get_item(w, index, out_text, cap)` forwards to
 *    `src->get_item(...)`; returns the callback's result on success or
 *    `-1` on NULL source / NULL `out_text` / `cap == 0`.
 *
 * Determinism: every call goes straight through to the caller-provided
 * function pointers. The widget core does no caching, no allocation and
 * no validation beyond NULL checks.
 *
 * Event `CAPY_WIDGET_EVENT_VIRTUAL_REQUEST_RANGE` is emitted by the host
 * renderer (not by the widget core) so the host can prefetch data;
 * `event->x` carries the start index, `event->y` the exclusive end.
 * `capy_widget_handle_event` returns `0` for this event — the host data
 * source is the active listener. */
typedef int (*capy_virtual_get_count_fn)(void *user_data);
typedef int (*capy_virtual_get_item_fn)(void *user_data, uint32_t index,
                                        char *out_text, uint16_t cap);

struct capy_virtual_data_source {
  capy_virtual_get_count_fn get_count;
  capy_virtual_get_item_fn get_item;
  void *user_data;
};

void capy_widget_set_virtual_source(
    struct capy_widget *w, const struct capy_virtual_data_source *src);
int capy_widget_virtual_count(const struct capy_widget *w);
int capy_widget_virtual_get_item(const struct capy_widget *w,
                                 uint32_t index, char *out_text,
                                 uint16_t cap);

/* Undo / redo (since 2.3).
 *
 * Deterministic per-context history of caller-defined actions. The
 * widget core stores entries verbatim — it never copies the redo/undo
 * payload bytes, never interprets the `action_id`, never advances any
 * clock. Hosts decide when to push, when to apply, and how to coalesce.
 *
 * Storage is a caller-owned byte buffer carved into:
 *   - a `capy_undo_stack` header at offset 0;
 *   - an array of `capy_undo_entry` slots immediately after the header.
 *
 * Capacity is derived from `buf_size`:
 *     capacity = (buf_size - sizeof(capy_undo_stack)) / sizeof(capy_undo_entry)
 * `capy_undo_stack_init` rejects buffers too small for one entry.
 *
 *  - `capy_undo_stack_init(ctx, buf, buf_size)` carves `buf`, attaches
 *    the stack to `ctx->undo_stack`, and returns 0 on success / -1 on
 *    NULL args or too-small buffer.
 *  - `capy_undo_push(ctx, action_id, redo, rlen, undo, ulen)` appends a
 *    new entry on top, discards any pending redo entries, and evicts
 *    the **oldest** entry FIFO-style when the slot array is full.
 *    Returns 0 on success, -1 if no stack is attached / NULL ctx.
 *    `action_id`, `redo_data` and `undo_data` are stored verbatim as
 *    pointers; the caller owns them and must keep them alive across
 *    later undo/redo calls.
 *  - `capy_undo_undo(ctx, out)` pops the top entry into `*out` (if
 *    `out` is non-NULL) and moves it onto the redo stack. Returns 0 on
 *    success, -1 on empty stack / NULL ctx.
 *  - `capy_undo_redo(ctx, out)` pops the next redo entry into `*out`
 *    (if non-NULL) and moves it back onto the live stack. Returns 0 on
 *    success, -1 on empty redo / NULL ctx.
 *  - `capy_undo_can_undo(ctx)` and `capy_undo_can_redo(ctx)` return 1
 *    when the respective stack is non-empty, 0 otherwise (also 0 for
 *    NULL ctx / no stack attached).
 *
 * Deferred for a follow-up additive minor:
 *  - **Coalescing.** The original slice plan merged successive entries
 *    with the same `action_id` inside a `coalesce_window_ms` (default
 *    500 ms). The current core stores entries 1:1; hosts that want to
 *    coalesce compare the new push against the previous entry before
 *    calling `_push`.
 *  - **Integrated text-edit hook.** TEXTBOX / RICH_TEXT mutations are
 *    not yet auto-registered. Hosts call `_push` themselves on insert/
 *    delete events. */
struct capy_undo_entry {
  const char *action_id;
  const void *redo_data;
  uint32_t redo_len;
  const void *undo_data;
  uint32_t undo_len;
  uint32_t timestamp_ms;
};

struct capy_undo_stack {
  struct capy_undo_entry *entries;
  uint32_t capacity;
  uint32_t count;
  uint32_t redo_count;
  uint32_t coalesce_window_ms;
};

int capy_undo_stack_init(struct capy_widget_context *ctx, void *buf,
                         uint32_t buf_size);
int capy_undo_push(struct capy_widget_context *ctx, const char *action_id,
                   const void *redo_data, uint32_t redo_len,
                   const void *undo_data, uint32_t undo_len);
int capy_undo_undo(struct capy_widget_context *ctx,
                   struct capy_undo_entry *out);
int capy_undo_redo(struct capy_widget_context *ctx,
                   struct capy_undo_entry *out);
int capy_undo_can_undo(const struct capy_widget_context *ctx);
int capy_undo_can_redo(const struct capy_widget_context *ctx);

/* State serialization (since 1.10).
 *
 * Canonical binary format with explicit little-endian byte order so blobs
 * are portable across hosts. Layout:
 *
 *   offset  size  field
 *   0       4     magic = "CUIS"
 *   4       2     format version (uint16, currently 1)
 *   6       2     reserved (must be 0)
 *   8       4     FNV-1a 32-bit checksum of bytes [16, end)
 *   12      4     node count (uint32)
 *   16+     ...   pre-order serialized nodes
 *
 * Per node (variable size depending on `text_len`):
 *
 *   1       type (uint8; `capy_widget_type` enum value)
 *   1       flags: bit0=visible, bit1=enabled, bit2=focused, bit3=focusable,
 *           bit4=checked
 *   2       tab_index (int16, little-endian)
 *   16      bounds: x, y (int32 LE), width, height (uint32 LE)
 *   2       text_len (uint16 LE)
 *   text_len bytes of UTF-8 text (no NUL)
 *   2       child_count (uint16 LE)
 *
 * What is preserved (since 1.10): widget type, bounds, visible/enabled/
 * focused/focusable/checked flags, tab_index, text, child tree structure.
 *
 * What is **not** preserved in 1.10 (deferred to a future minor):
 *   - text_edit caret/selection, password mask, readonly bit
 *   - active animations (`capy_anim`, tracks, springs)
 *   - theme tokens / live theme pointer
 *   - locale + RTL state
 *   - 2D transform pointer
 *   - drag-and-drop config
 *   - IME state
 *   - layout cache (`layout_dirty`, `layout_version`, `dirty_version`)
 *   - pool / slab allocator state
 *   - callback pointers (`on_click`, `on_change`, `on_submit`, `on_drop`)
 *     and `user_data` — hosts re-wire these after deserialize
 *   - font_id, image_id and any other external resource handles — hosts
 *     re-bind from their resource manager
 *
 * `capy_widget_serialize(root, out, cap, &out_len)` writes the binary
 * blob into `out`. Returns `0` on success with `*out_len` set; `-1` on
 * NULL args, capacity overflow (`*out_len` set to required size for the
 * portion already laid out), or tree depth/breadth issues.
 *
 * `capy_widget_deserialize(buf, len, ctx, &out_root)` validates magic,
 * version (only `1` accepted in 1.10) and checksum, then rebuilds the
 * tree using `ctx->allocator`. Returns `0` on success, `-1` on any
 * validation failure or allocation failure (no partial tree leak: any
 * widgets created before the failure are destroyed).
 *
 * `capy_widget_serialize_text(root, out, cap)` writes a single-line-per
 * widget indented dump (`[type=N bounds=(x,y,w,h) flags=ve text="..."
 * children=K]`) for debugging. Returns the byte length written (excluding
 * NUL terminator) on success, `-1` on overflow. */
#define CAPY_STATE_MAGIC0 'C'
#define CAPY_STATE_MAGIC1 'U'
#define CAPY_STATE_MAGIC2 'I'
#define CAPY_STATE_MAGIC3 'S'
#define CAPY_STATE_FORMAT_VERSION 1u
#define CAPY_STATE_HEADER_SIZE 16u

#define CAPY_STATE_FLAG_VISIBLE   0x01u
#define CAPY_STATE_FLAG_ENABLED   0x02u
#define CAPY_STATE_FLAG_FOCUSED   0x04u
#define CAPY_STATE_FLAG_FOCUSABLE 0x08u
#define CAPY_STATE_FLAG_CHECKED   0x10u

int capy_widget_serialize(const struct capy_widget *root, void *out,
                          uint32_t cap, uint32_t *out_len);
int capy_widget_deserialize(const void *buf, uint32_t len,
                            struct capy_widget_context *ctx,
                            struct capy_widget **out_root);
int capy_widget_serialize_text(const struct capy_widget *root, char *out,
                               uint32_t cap);

/* Plugin ABI (since 2.0).
 *
 * The plugin surface is intentionally small: callers register a
 * descriptor with `capy_plugin_register`; the host stores the descriptor
 * pointer + opaque `user_data` slot for sandbox isolation. The actual
 * plugin lifecycle (init, destroy, event routing, custom emit) is
 * declared here but driven by the host's dispatcher — the widget core
 * never invokes plugin callbacks on its own (forward-compat: a future
 * minor can wire automatic invocation under an opt-in flag).
 *
 * `capy_plugin_descriptor`:
 *  - `id` is a reverse-DNS string identifying the plugin (e.g.
 *    `"com.example.timeline"`); compared by pointer or value at the
 *    discretion of the host.
 *  - `version` is a free-form semver string for human display.
 *  - `capy_ui_abi_required` is the minimum host version the plugin
 *    needs, encoded as `(major << 16) | (minor << 8) | patch`. The host
 *    rejects descriptors with `capy_ui_abi_required >
 *    CAPYUI_API_VERSION_TAG`.
 *  - `allocator` is the per-plugin allocator the plugin promises to use
 *    for its own state. The widget core stores the pointer but never
 *    calls into it — it exists so the host can audit isolation.
 *  - `init`, `destroy`, `on_event`, `emit` are optional callbacks. NULL
 *    function pointers are tolerated; the host treats them as no-ops.
 *  - `timeout_microseconds` is a host-enforced hint. Callbacks that
 *    exceed it should be considered malfunctioning and de-registered.
 *
 * Sandbox guarantees the widget core enforces:
 *  - The descriptor's pointers (`init`, `destroy`, `on_event`, `emit`,
 *    `allocator`) are stored verbatim; the host invokes them under its
 *    own signal/timeout discipline. Plugin crashes never reach the
 *    widget core because the core does not invoke the callbacks.
 *  - The plugin allocator pointer is kept separate from the host's
 *    `capy_widget_context::allocator`. The core never mixes them.
 *
 * `capy_plugin_register(ctx, desc)` validates `desc` (non-NULL,
 * `capy_ui_abi_required` accepted, `init`/`destroy`/`on_event`/`emit`
 * may be NULL, descriptor pointer not already registered) and stores
 * the pointer in `ctx->plugins[ctx->plugin_count++]`. Returns the
 * 0-based slot index on success, `-1` on validation failure (no
 * mutation) or capacity overflow.
 *
 * `capy_plugin_unregister_all(ctx)` walks the registered descriptors,
 * invokes each non-NULL `destroy` callback once, and resets
 * `ctx->plugin_count = 0`. Safe to call multiple times. */
struct capy_plugin_context;

struct capy_plugin_descriptor {
  const char *id;
  const char *version;
  uint32_t capy_ui_abi_required;
  struct capy_widget_allocator allocator;
  uint32_t timeout_microseconds;
  int (*init)(struct capy_plugin_context *ctx);
  void (*destroy)(struct capy_plugin_context *ctx);
  int (*on_event)(struct capy_plugin_context *ctx,
                  struct capy_widget *target,
                  const struct capy_widget_event *ev);
  int (*emit)(struct capy_plugin_context *ctx,
              struct capy_widget *target, struct capy_display_list *dl);
};

/* Per-plugin sandbox handle. The widget core does not look inside it —
 * the host populates `user_data` with whatever state the plugin needs
 * and threads it back into the callbacks. */
struct capy_plugin_context {
  const struct capy_plugin_descriptor *descriptor;
  void *user_data;
};

int capy_plugin_register(struct capy_widget_context *ctx,
                         const struct capy_plugin_descriptor *desc);
void capy_plugin_unregister_all(struct capy_widget_context *ctx);

/* Theme (since 0.6) */
struct capy_theme capy_theme_default_light(void);
struct capy_theme capy_theme_default_dark(void);
struct capy_theme capy_theme_high_contrast(void);
void capy_theme_apply(struct capy_widget_context *ctx,
                      const struct capy_theme *theme);
uint32_t capy_theme_resolve(const struct capy_theme *theme, uint8_t token,
                            uint32_t fallback);

/* Theme serialization (since 0.14).
 *
 * Canonical line-oriented `key=value` ASCII format. One token per line in a
 * fixed order. Colours are written as `0xAARRGGBB` uppercase hex. Blank
 * lines and lines starting with `#` are ignored on deserialize. The format
 * version is independent of the per-instance `theme->version` field and is
 * exposed via `CAPY_THEME_FORMAT_VERSION`.
 *
 * `capy_theme_serialize(t, out, cap)` writes the canonical representation
 * of `t` into `out[]` and returns the number of bytes written (excluding
 * the terminating NUL) on success, or `-1` on overflow, NULL/zero
 * arguments, or invalid `variant`. Output is always NUL-terminated in
 * success. Same `theme` -> same byte sequence (deterministic).
 *
 * `capy_theme_deserialize(out, in, len)` parses `len` bytes of canonical
 * input and fills `out`. Returns `0` on success, `-1` on NULL arguments,
 * format-version greater than `CAPY_THEME_FORMAT_VERSION`, missing
 * `version=` line, malformed lines (no `=`, bad hex, invalid variant, bad
 * boolean), or empty key. Unknown keys (tokens added in future versions)
 * are silently ignored. Missing tokens stay zero in `out`; callers wanting
 * defaults should seed `out` with `capy_theme_default_light()` (or another
 * default) before calling. */
#define CAPY_THEME_FORMAT_VERSION 2u
int capy_theme_serialize(const struct capy_theme *t, char *out, uint32_t cap);
int capy_theme_deserialize(struct capy_theme *out, const char *in,
                           uint32_t len);

/* Theme packs (since 2.4).
 *
 * Compact binary bundle of colour tokens for offline distribution. Format
 * is little-endian throughout so blobs are portable across hosts.
 *
 *   offset  size  field
 *   0       4     magic = "CTHM"
 *   4       2     format version (uint16, currently 1)
 *   6       1     variant (CAPY_THEME_VARIANT_*; ignored if >= 16)
 *   7       1     high_contrast flag (0 or 1)
 *   8       4     FNV-1a 32-bit checksum of bytes [16, end)
 *   12      2     token entry count (uint16)
 *   14      2     reserved (must be 0)
 *   16+     ...   token entries (6 bytes each)
 *
 * Each entry is:
 *   uint8  token_id   (>=1, <CAPY_TOKEN_COUNT, otherwise the entry is skipped)
 *   uint8  reserved   (must be 0)
 *   uint32 colour_le  (0xAARRGGBB packed as little-endian bytes)
 *
 * Entry order is irrelevant; duplicate token_ids are tolerated (later
 * wins). Unknown token_ids (>= `CAPY_TOKEN_COUNT`) are silently skipped
 * so packs authored against a newer schema still load on older hosts.
 *
 *  - `capy_theme_pack_validate(buf, len)` returns 0 when `buf` carries a
 *    well-formed pack of `len` bytes, -1 otherwise. Validation checks
 *    magic, format version <= `CAPY_THEME_PACK_FORMAT_VERSION`, reserved
 *    fields zero, declared entry count fits in `len`, and FNV-1a checksum
 *    matches the body bytes.
 *  - `capy_theme_pack_load(buf, len, out)` validates then applies the
 *    pack onto `*out`. Tokens absent from the pack stay untouched; the
 *    caller seeds `*out` with `capy_theme_default_light()` (or another
 *    base) before calling. `variant`/`high_contrast` fields from the
 *    pack overwrite `out->variant`/`out->high_contrast`; `out->version`
 *    is left untouched so callers can stamp their own per-instance
 *    versioning. Returns 0 on success, -1 on validation failure or NULL
 *    arguments.
 *
 * Deferred for follow-up additive minors:
 *  - **Hot reload.** Hosts watch the source file and re-call `_load`.
 *    The widget core does not poll.
 *  - **Signature (Ed25519).** Hosts validate signatures externally; the
 *    core does not link any crypto. The format reserves no field for it
 *    yet — a future schema (`format_version=2`) may append a trailing
 *    signature block.
 *  - **Fonts / icons.** The 2.4 pack carries only colours. Font and icon
 *    refs ride the icon descriptor surface delivered by v2.11. */
#define CAPY_THEME_PACK_MAGIC0          0x43u /* 'C' */
#define CAPY_THEME_PACK_MAGIC1          0x54u /* 'T' */
#define CAPY_THEME_PACK_MAGIC2          0x48u /* 'H' */
#define CAPY_THEME_PACK_MAGIC3          0x4Du /* 'M' */
#define CAPY_THEME_PACK_FORMAT_VERSION  1u
#define CAPY_THEME_PACK_HEADER_SIZE     16u
#define CAPY_THEME_PACK_ENTRY_SIZE      6u

int capy_theme_pack_validate(const void *buf, uint32_t len);
int capy_theme_pack_load(const void *buf, uint32_t len,
                         struct capy_theme *out);

/* Devtools / inspector (since 2.5).
 *
 * Read-only introspection over a retained widget tree and a widget
 * context. Both APIs are zero-allocation and side-effect-free: they
 * write into caller-provided buffers and never mutate the inspected
 * tree or context. Hosts use this to drive inspector panels, headless
 * harnesses, snapshot tests and CI diff tools.
 *
 * Flatten layout: `capy_widget_inspect` walks the tree in deterministic
 * depth-first pre-order. The first node is always the root with
 * `parent_index == CAPY_INSPECTOR_NO_PARENT` and `depth == 0`. Each
 * subsequent node carries the index (in `out_nodes`) of its parent. The
 * traversal mirrors `capy_widget_emit_recursive`, so node ordering is
 * stable across runs and matches the display-list emission order.
 *
 * Text handling: each node's `text` is copied into the caller's
 * `out_text_pool` without a NUL terminator. The inspector reports the
 * `(text_offset, text_len)` slice; consumers can read those bytes
 * directly. Overflow rolls back both `*out_node_count` and
 * `*out_text_used` to their pre-call values and returns -1.
 *
 *  - `capy_widget_inspect(root, out_nodes, cap, out_text_pool, text_cap,
 *      out_node_count, out_text_used)` returns 0 on success, -1 on NULL
 *      `root`/`out_nodes`/`out_node_count`/`out_text_used`, capacity
 *      overflow, or text pool overflow. `out_text_pool` may be NULL only
 *      when `text_cap == 0`; in that case nodes with non-empty text
 *      cause overflow.
 *  - `capy_perf_counters_snapshot(ctx, root, out)` populates a small
 *      fixed-size struct with state useful for inspector overlays:
 *      widget count (counted from `root`, or 0 when `root` is NULL),
 *      plugin count (from `ctx->plugin_count`), undo / redo counts
 *      (from `ctx->undo_stack` when attached), DPI scale, frame budget,
 *      theme variant and high-contrast flag. Returns 0 on success, -1
 *      on NULL `ctx`/`out`.
 *
 * Deferred for follow-up additive minors:
 *  - **Display-list dump.** A textual `capy_devtools_dump_display_list`
 *    that emits a canonical line-oriented representation of each op.
 *  - **Event recording / replay.** A caller-buffer-backed ring of
 *    `(timestamp_ms, capy_widget_event)` records, with a `_replay` API
 *    that re-dispatches the stream into a fresh context. Builds on the
 *    v2.3 undo stack + v1.10 state serialization.
 *  - **Headless harness CLI.** Tooling that loads a serialized state,
 *    feeds a recorded event stream, captures the resulting display-list
 *    and compares it byte-for-byte against a golden. Lives outside the
 *    portable core.
 *  - **IDE protocol.** A simple textual wire format so editors can
 *    drive the inspector remotely. */
#define CAPY_INSPECTOR_NO_PARENT 0xFFFFFFFFu

#define CAPY_INSPECTOR_FLAG_VISIBLE          0x0001u
#define CAPY_INSPECTOR_FLAG_ENABLED          0x0002u
#define CAPY_INSPECTOR_FLAG_FOCUSED          0x0004u
#define CAPY_INSPECTOR_FLAG_FOCUSABLE        0x0008u
#define CAPY_INSPECTOR_FLAG_CHECKED          0x0010u
#define CAPY_INSPECTOR_FLAG_HOVERED          0x0020u
#define CAPY_INSPECTOR_FLAG_LAYOUT_DIRTY     0x0040u
#define CAPY_INSPECTOR_FLAG_HAS_TRANSFORM    0x0080u
#define CAPY_INSPECTOR_FLAG_HAS_VIRTUAL_SRC  0x0100u
#define CAPY_INSPECTOR_FLAG_HAS_DRAG_PAYLOAD 0x0200u
#define CAPY_INSPECTOR_FLAG_HAS_DROP_TARGET  0x0400u

struct capy_inspector_node {
  uint32_t id;
  uint16_t type;
  uint16_t depth;
  uint32_t parent_index;
  uint32_t child_count;
  int32_t x;
  int32_t y;
  int32_t w;
  int32_t h;
  uint32_t flags;
  int16_t tab_index;
  uint16_t font_id;
  uint16_t text_offset;
  uint16_t text_len;
  uint32_t layout_version;
  uint32_t dirty_version;
};

struct capy_perf_counters {
  uint32_t widget_count;
  uint32_t plugin_count;
  uint32_t undo_count;
  uint32_t undo_redo_count;
  uint32_t undo_capacity;
  uint16_t dpi_scale_x256;
  uint16_t reserved_dpi;
  uint32_t frame_budget_microseconds;
  uint16_t theme_variant;
  uint8_t theme_high_contrast;
  uint8_t reserved[1];
};

int capy_widget_inspect(const struct capy_widget *root,
                        struct capy_inspector_node *out_nodes,
                        uint32_t cap, char *out_text_pool,
                        uint32_t text_cap, uint32_t *out_node_count,
                        uint32_t *out_text_used);

int capy_perf_counters_snapshot(const struct capy_widget_context *ctx,
                                const struct capy_widget *root,
                                struct capy_perf_counters *out);

/* Display mode (since 2.6).
 *
 * Plumbing for runtime resolution + refresh-rate selection. CapyUI does
 * not touch the framebuffer, the kernel-level mode-setting interface,
 * or any video-card register. The host registers two callbacks via a
 * caller-owned `capy_display_controller`; the widget core just routes
 * calls and emits a `CAPY_WIDGET_EVENT_DISPLAY_MODE_CHANGED` event
 * after a successful mode change so other subsystems (compositor,
 * settings persistence) can react.
 *
 * Mode layout:
 *   uint16_t width            (pixels, > 0)
 *   uint16_t height           (pixels, > 0)
 *   uint16_t refresh_hz_q8    (Q8.0; 60 = 60 Hz, 144 = 144 Hz, 240 = 240 Hz)
 *   uint8_t  bpp              (24 or 32)
 *   uint8_t  flags            (CAPY_DISPLAY_MODE_FLAG_*)
 *
 * `refresh_hz_q8` is named for symmetry with future fractional refresh
 * rates (Q8.0 today; a follow-up minor may widen to Q24.8). `flags`
 * carries `PREFERRED` (host marks one mode as the default) and
 * `INTERLACED` (legacy CRT support).
 *
 *  - `capy_display_enum_modes(ctx, out, cap)` forwards to
 *      `controller->enum_modes`. Returns the number of modes written on
 *      success, `-1` on NULL ctx/out (when `cap > 0`), NULL controller
 *      callback, or whatever the host callback returns on error.
 *  - `capy_display_set_mode(ctx, mode)` forwards to
 *      `controller->set_mode`. On success it caches `*mode` into
 *      `controller->current_mode`, sets `controller->has_current = 1`,
 *      and emits `CAPY_WIDGET_EVENT_DISPLAY_MODE_CHANGED` through
 *      `capy_widget_handle_event` against `root` (if non-NULL). Returns
 *      `0` on success, `-1` on NULL/missing callback or host failure.
 *  - `capy_display_current_mode(ctx, out)` copies the cached
 *      `controller->current_mode` into `*out` when `has_current != 0`.
 *      Returns `0` on success, `-1` on NULL/missing current.
 *
 * Deferred for follow-up additive minors:
 *  - **Fractional refresh.** Promote `refresh_hz_q8` to Q24.8 + fractional
 *    bit when 119.88 Hz / 23.976 Hz / G-SYNC-class modes need exact
 *    representation.
 *  - **HDR + VRR metadata.** Reserved flags in `flags` field for HDR10,
 *    Dolby Vision, variable refresh range, colour gamut.
 *  - **Rollback / preview UI.** Settings → Display app shows "Apply"
 *    + "Revert in 15s" overlay; logic lives in the desktop session, not
 *    in the widget core.
 *  - **Persistence.** Last confirmed mode saved by the desktop session
 *    via `capy_widget_serialize` (1.10) into `/var/state/capyui/`. */
#define CAPY_DISPLAY_MODE_FLAG_PREFERRED   0x01u
#define CAPY_DISPLAY_MODE_FLAG_INTERLACED  0x02u

struct capy_display_mode {
  uint16_t width;
  uint16_t height;
  uint16_t refresh_hz_q8;
  uint8_t bpp;
  uint8_t flags;
};

typedef int (*capy_display_enum_modes_fn)(void *user_data,
                                          struct capy_display_mode *out,
                                          uint32_t cap);

typedef int (*capy_display_set_mode_fn)(void *user_data,
                                        const struct capy_display_mode *mode);

struct capy_display_controller {
  capy_display_enum_modes_fn enum_modes;
  capy_display_set_mode_fn set_mode;
  void *user_data;
  struct capy_display_mode current_mode;
  uint8_t has_current;
  uint8_t reserved[3];
};

void capy_widget_set_display_controller(
    struct capy_widget_context *ctx,
    struct capy_display_controller *controller);

int capy_display_enum_modes(struct capy_widget_context *ctx,
                            struct capy_display_mode *out, uint32_t cap);

int capy_display_set_mode(struct capy_widget_context *ctx,
                          struct capy_widget *root,
                          const struct capy_display_mode *mode);

int capy_display_current_mode(const struct capy_widget_context *ctx,
                              struct capy_display_mode *out);

/* User management UI (since 2.7).
 *
 * Plumbing for user account management. CapyUI never authenticates,
 * never hashes a password, never touches `/etc/passwd`-style backing
 * stores. The host registers five callbacks via a caller-owned
 * `capy_user_directory`; the widget core just routes calls and
 * publishes the canonical `capy_user_account` shape so the desktop
 * session's Settings → Users app and the v2.13 login screen consume
 * the same surface.
 *
 * Account layout:
 *   char     username[32]      (null-terminated)
 *   char     display_name[64]  (null-terminated, locale-aware)
 *   uint32_t uid               (host-assigned; 0 = root, reserved)
 *   uint8_t  is_admin
 *   uint8_t  is_locked
 *   uint16_t avatar_image_id   (host-owned; 0 = default silhouette)
 *   uint32_t last_login_ms     (monotonic ms; 0 = never)
 *
 *  - `capy_user_list(ctx, out, cap)` forwards to `directory->list`.
 *      Returns accounts written on success, `-1` on NULL ctx, missing
 *      callback, or `cap > 0 && out == NULL`.
 *  - `capy_user_create(ctx, account, password)` forwards to
 *      `directory->create`. `password` is passed through to the host
 *      verbatim; CapyUI never stores or hashes it. Returns `0` on
 *      success, `-1` on NULL args, missing callback, empty username,
 *      or host rejection.
 *  - `capy_user_update(ctx, account)` forwards to `directory->update`.
 *      The host looks up by `uid`. Returns `0` on success, `-1` on
 *      NULL args, missing callback, or host rejection.
 *  - `capy_user_delete(ctx, uid)` forwards to `directory->del`. The
 *      widget core rejects `uid == 0` (root is undeletable) before
 *      reaching the host. Returns `0` on success, `-1` on NULL ctx,
 *      missing callback, `uid == 0`, or host rejection.
 *  - `capy_user_set_avatar(ctx, uid, png, len)` forwards to
 *      `directory->set_avatar`. Bytes are caller-owned; CapyUI never
 *      copies them. `len == 0` with `png == NULL` clears the avatar.
 *      Returns `0` on success, `-1` on NULL ctx, missing callback,
 *      `len > 0 && png == NULL`, or host rejection.
 *
 * Deferred for follow-up additive minors:
 *  - **Authentication.** `capy_user_authenticate_fn` for the v2.13
 *    login screen (password + biometric + SSO). Today the desktop
 *    session calls a host hook directly.
 *  - **Groups + roles.** Multi-tenant ACLs beyond the binary
 *    `is_admin` flag.
 *  - **Account activity log.** Last login is exposed; full audit log
 *    lives outside the widget core. */
#define CAPY_USER_NAME_MAX           32u
#define CAPY_USER_DISPLAY_NAME_MAX   64u

struct capy_user_account {
  char username[CAPY_USER_NAME_MAX];
  char display_name[CAPY_USER_DISPLAY_NAME_MAX];
  uint32_t uid;
  uint8_t is_admin;
  uint8_t is_locked;
  uint16_t avatar_image_id;
  uint32_t last_login_ms;
};

typedef int (*capy_user_list_fn)(void *user_data,
                                 struct capy_user_account *out, uint32_t cap);
typedef int (*capy_user_create_fn)(void *user_data,
                                   const struct capy_user_account *account,
                                   const char *password);
typedef int (*capy_user_update_fn)(void *user_data,
                                   const struct capy_user_account *account);
typedef int (*capy_user_delete_fn)(void *user_data, uint32_t uid);
typedef int (*capy_user_set_avatar_fn)(void *user_data, uint32_t uid,
                                       const void *png, uint32_t len);

struct capy_user_directory {
  capy_user_list_fn list;
  capy_user_create_fn create;
  capy_user_update_fn update;
  capy_user_delete_fn del;
  capy_user_set_avatar_fn set_avatar;
  void *user_data;
};

void capy_widget_set_user_directory(struct capy_widget_context *ctx,
                                    struct capy_user_directory *directory);

int capy_user_list(struct capy_widget_context *ctx,
                   struct capy_user_account *out, uint32_t cap);
int capy_user_create(struct capy_widget_context *ctx,
                     const struct capy_user_account *account,
                     const char *password);
int capy_user_update(struct capy_widget_context *ctx,
                     const struct capy_user_account *account);
int capy_user_delete(struct capy_widget_context *ctx, uint32_t uid);
int capy_user_set_avatar(struct capy_widget_context *ctx, uint32_t uid,
                         const void *png, uint32_t len);

/* Contrast & accessibility theme refinement (since 2.8).
 *
 * Quantitative contrast metrics over the colour tokens. CapyUI computes
 * a WCAG-2.1-style relative luminance with a `gamma ≈ 2.0` integer
 * approximation: sRGB-byte → Q16.16 linear via `lin = c * c * 65536 /
 * 65025`, weighted as `L = (2126 * R + 7152 * G + 722 * B) / 10000`.
 * The contrast ratio is `(max(L1, L2) + 0.05) / (min(L1, L2) + 0.05)`,
 * scaled by 1000 so callers get an integer in `[1000, 21000]` covering
 * the `[1.0, 21.0]` WCAG range.
 *
 * The approximation differs slightly from the exact WCAG transform
 * (which uses `((c + 0.055) / 1.055) ^ 2.4` above the 0.03928 knee).
 * Differences are bounded — both methods agree on the AA / AAA pass /
 * fail decisions for the canonical token pairs that matter — and the
 * integer formulation is zero-float and deterministic across hosts.
 *
 *  - `capy_theme_contrast_ratio_x1000(fg, bg)` returns the ratio for
 *    two `0xAARRGGBB` colours, multiplied by 1000 and clamped to
 *    `[1000, 21000]`. Alpha is ignored. Returns `1000` (= 1.0) when
 *    both colours match.
 *  - `capy_theme_audit_wcag(theme, out, cap)` walks the canonical
 *    foreground / background pairs documented in
 *    `docs/roadmap/contracts/theme-tokens.md` (FG_PRIMARY/BG_BASE,
 *    FG_PRIMARY/BG_RAISED, FG_MUTED/BG_BASE, FG_INVERSE/ACCENT,
 *    BORDER_FOCUS/BG_BASE, FOCUS_RING/BG_BASE, DANGER/BG_BASE,
 *    WARNING/BG_BASE, SUCCESS/BG_BASE, INFO/BG_BASE, DISABLED/BG_BASE)
 *    and writes one `capy_contrast_finding` per pair. Returns the
 *    number of findings written on success, `-1` on NULL args, or the
 *    total pair count when `cap == 0` (allowing callers to size their
 *    buffer with a single dry-run call).
 *  - `capy_theme_default_dark_high_contrast()` is a fourth built-in
 *    factory: dark background, white foreground, paired so every
 *    canonical token pair lands at ≥ 7:1 (AAA). The variant byte is
 *    `CAPY_THEME_VARIANT_DARK_HIGH_CONTRAST = 3`. Useful as the
 *    counterpart to the existing light `capy_theme_high_contrast()`.
 *
 * Pass thresholds (per WCAG 2.1):
 *  - AA  = ratio ≥ 4.5 (i.e. `ratio_x1000 ≥ 4500`).
 *  - AAA = ratio ≥ 7.0 (i.e. `ratio_x1000 ≥ 7000`).
 *
 * Deferred for follow-up additive minors:
 *  - **WCAG 3 APCA**. The Accessible Perceptual Contrast Algorithm
 *    replaces the legacy ratio with a perceptual `Lc` score. Adding
 *    it is a new public API plus tables; not yet in scope.
 *  - **Large-text AA**. The `≥ 3.0` threshold for ≥ 24 px text would
 *    need a third `passes_*` bit and per-pair size metadata; deferred.
 *  - **Per-locale palette overrides**. Locale-aware theming lives in
 *    a future minor; the audit currently uses the active variant. */
#define CAPY_CONTRAST_AA_X1000   4500u
#define CAPY_CONTRAST_AAA_X1000  7000u

struct capy_contrast_finding {
  uint8_t fg_token;
  uint8_t bg_token;
  uint16_t ratio_x1000;
  uint8_t passes_aa;
  uint8_t passes_aaa;
  uint8_t reserved[2];
};

uint32_t capy_theme_contrast_ratio_x1000(uint32_t fg, uint32_t bg);
int capy_theme_audit_wcag(const struct capy_theme *theme,
                          struct capy_contrast_finding *out, uint32_t cap);
struct capy_theme capy_theme_default_dark_high_contrast(void);

/* Desktop icon arrangement (since 2.9).
 *
 * Plumbing primitives for the desktop session's icon layout: a small
 * `capy_desktop_layout` config (cell size, snap flag, auto-arrange
 * mode, sort key, flags), a per-icon `capy_desktop_icon_position`
 * record (id + free position + grid coordinates + pinned), and a
 * canonical binary blob `CDLA` that hosts persist into
 * `/var/state/capyui/desktop-layout.bin`.
 *
 * The widget core never opens a file, never drives the drag, never
 * decides where to place an icon — it just owns the struct shapes and
 * the round-trip serialization so desktop-session, login screen and
 * cross-host migration tooling consume the same surface.
 *
 *   Blob layout (little-endian, magic `"CDLA"`):
 *
 *   offset  size  field
 *   0       4     magic = "CDLA"
 *   4       2     format_version (uint16, currently 1)
 *   6       1     snap (0 or 1)
 *   7       1     auto_arrange (0 or 1)
 *   8       4     FNV-1a 32-bit checksum over bytes [24, end)
 *   12      2     cell_w (uint16, > 0)
 *   14      2     cell_h (uint16, > 0)
 *   16      1     sort_by (CAPY_DESKTOP_SORT_*)
 *   17      1     flags  (CAPY_DESKTOP_LAYOUT_FLAG_*)
 *   18      2     entry_count (uint16)
 *   20      4     reserved (must be 0)
 *   24+     ...   N entries of 16 bytes each:
 *                   uint32  icon_id
 *                   int16   x         (free position, pixels)
 *                   int16   y
 *                   int16   grid_x    (snapped column)
 *                   int16   grid_y    (snapped row)
 *                   uint8   pinned    (0 or 1)
 *                   uint8   reserved[3] (must be 0)
 *
 *  - `capy_desktop_layout_validate(buf, len)` returns `0` on a
 *    well-formed blob, `-1` otherwise. Checks magic, format_version
 *    `<= CAPY_DESKTOP_LAYOUT_FORMAT_VERSION`, reserved fields zero,
 *    `len == header + entry_count * entry_size`, FNV-1a body
 *    checksum, `cell_w > 0 && cell_h > 0`, `snap <= 1`,
 *    `auto_arrange <= 1`, `sort_by < CAPY_DESKTOP_SORT_COUNT`.
 *  - `capy_desktop_layout_load(buf, len, out_layout, out_positions,
 *      cap, out_count)` validates then writes the layout config into
 *      `*out_layout` and copies entries (capped by `cap`) into
 *      `out_positions`. `*out_count` always receives the *declared*
 *      entry count even when `cap` is smaller (callers detect a short
 *      buffer by `*out_count > cap`). Returns `0` on success, `-1` on
 *      validation failure or NULL args (the layout pointer must be
 *      non-NULL; positions buffer may be NULL only when `cap == 0`).
 *  - `capy_desktop_layout_serialize(layout, positions, count, out,
 *      cap)` writes a canonical blob into `out[]` and returns the
 *      number of bytes written, `-1` on NULL args, NULL positions
 *      with `count > 0`, invalid layout (zero cell, sort out of
 *      range, snap/auto > 1), or `cap` too small for `header + count
 *      * entry_size`. Output is byte-deterministic for identical
 *      inputs.
 *
 * Deferred for follow-up additive minors:
 *  - **Multi-monitor layout.** Today one blob holds one monitor's
 *    layout; multi-monitor needs an outer envelope keyed by
 *    `monitor_index` (from v2.6 display mode).
 *  - **Group folders.** Stacks of icons (macOS-style) live outside
 *    this surface; v2.11 icon system will model them.
 *  - **Locale-aware sorting.** `sort_by = NAME` uses byte order today;
 *    locale collation will arrive with the v0.13 locale surface.
 *  - **Drag hit-test in core.** The widget core does not own the
 *    interactive drag loop — the desktop session in `src/desktop/`
 *    does, and persists the result via this surface. */
#define CAPY_DESKTOP_LAYOUT_MAGIC0          0x43u /* 'C' */
#define CAPY_DESKTOP_LAYOUT_MAGIC1          0x44u /* 'D' */
#define CAPY_DESKTOP_LAYOUT_MAGIC2          0x4Cu /* 'L' */
#define CAPY_DESKTOP_LAYOUT_MAGIC3          0x41u /* 'A' */
#define CAPY_DESKTOP_LAYOUT_FORMAT_VERSION  1u
#define CAPY_DESKTOP_LAYOUT_HEADER_SIZE     24u
#define CAPY_DESKTOP_LAYOUT_ENTRY_SIZE      16u

#define CAPY_DESKTOP_SORT_MANUAL  0u
#define CAPY_DESKTOP_SORT_NAME    1u
#define CAPY_DESKTOP_SORT_TYPE    2u
#define CAPY_DESKTOP_SORT_MTIME   3u
#define CAPY_DESKTOP_SORT_SIZE    4u
#define CAPY_DESKTOP_SORT_COUNT   5u

#define CAPY_DESKTOP_LAYOUT_FLAG_ALIGN_RIGHT   0x01u
#define CAPY_DESKTOP_LAYOUT_FLAG_VERTICAL_FIRST 0x02u

struct capy_desktop_layout {
  uint16_t cell_w;
  uint16_t cell_h;
  uint8_t snap;
  uint8_t auto_arrange;
  uint8_t sort_by;
  uint8_t flags;
};

struct capy_desktop_icon_position {
  uint32_t icon_id;
  int16_t x;
  int16_t y;
  int16_t grid_x;
  int16_t grid_y;
  uint8_t pinned;
  uint8_t reserved[3];
};

int capy_desktop_layout_validate(const void *buf, uint32_t len);
int capy_desktop_layout_load(const void *buf, uint32_t len,
                             struct capy_desktop_layout *out_layout,
                             struct capy_desktop_icon_position *out_positions,
                             uint32_t cap, uint32_t *out_count);
int capy_desktop_layout_serialize(
    const struct capy_desktop_layout *layout,
    const struct capy_desktop_icon_position *positions, uint32_t count,
    void *out, uint32_t cap);

/* File manager UX plumbing (since 2.10).
 *
 * Plumbing primitives shared by the file-manager app and any future
 * shell that needs toolbar grouping or breadcrumb truncation. The
 * widget core does not own the file manager itself — that lives in
 * the desktop session (`src/desktop/` / `src/apps/`) — but the
 * taxonomy and the math are published here so multiple consumers
 * (file manager, save/open dialogs, location bars) stay consistent.
 *
 * Toolbar groups: a stable enum that callers can pin onto their
 * toolbar `capy_widget` instances. Groups render with a small visual
 * separator between them and collapse into an overflow `…` menu
 * when the viewport falls below `CAPY_FILE_MGR_COMPACT_THRESHOLD_PX`
 * (= 600 px). The widget core does not enforce the threshold; it
 * publishes the constant so all consumers agree.
 *
 * Breadcrumb truncation: `capy_breadcrumb_truncate` takes a list of
 * `capy_breadcrumb_segment` records, an `available_width_px` budget,
 * and a `segment_avg_px` estimate, and decides which segments stay
 * inline plus whether a `…` dropdown is needed between the root and
 * the tail. Output is byte-deterministic — same inputs always
 * produce the same `(out[], out_count, out_dropdown)`.
 *
 *  - When `in_count == 0`: `*out_count = 0`, `*out_dropdown = 0`.
 *  - When the full sequence fits (`in_count * segment_avg_px <=
 *      available_width_px`): copy all, `*out_dropdown = 0`.
 *  - Otherwise: `out[0]` is the root segment, the remaining slots are
 *      the tail of `in[]`, and `*out_dropdown = 1`. The number of
 *      inline slots is `max(2, available_width_px / segment_avg_px)`
 *      clamped to `min(cap, in_count)`. Callers render a `…` button
 *      between `out[0]` and `out[1]` when `*out_dropdown != 0`.
 *  - Returns `0` on success, `-1` on NULL `in`/`out`/`out_count`/
 *      `out_dropdown`, `cap == 0` with non-zero input, or
 *      `segment_avg_px == 0`.
 *
 * Deferred for follow-up additive minors:
 *  - **Variable-width segment measurement.** Today the algorithm uses
 *    one `segment_avg_px` for the whole bar. A future minor can add a
 *    callback that returns per-segment widths from the host's text
 *    metrics.
 *  - **Multi-line breadcrumb.** Wrapping the bar onto two lines when
 *    width is plentiful is a desktop-session policy decision.
 *  - **Locale-aware ellipsis.** RTL breadcrumb truncation may need a
 *    leading rather than middle `…`; will piggyback on v0.13 locale.
 *  - **Toolbar item plumbing.** The taxonomy enum is published, but
 *    per-item descriptors (icon_id + label + tooltip + shortcut) live
 *    in the desktop session today. */
#define CAPY_FILE_MGR_COMPACT_THRESHOLD_PX  600u

#define CAPY_TOOLBAR_GROUP_NAV     0u
#define CAPY_TOOLBAR_GROUP_VIEW    1u
#define CAPY_TOOLBAR_GROUP_ACTION  2u
#define CAPY_TOOLBAR_GROUP_LAYOUT  3u
#define CAPY_TOOLBAR_GROUP_COUNT   4u

struct capy_breadcrumb_segment {
  uint32_t id;
  uint16_t text_offset;
  uint16_t text_len;
  uint16_t icon_image_id;
  uint8_t is_clickable;
  uint8_t reserved;
};

int capy_breadcrumb_truncate(const struct capy_breadcrumb_segment *in,
                             uint32_t in_count, uint32_t available_width_px,
                             uint32_t segment_avg_px,
                             struct capy_breadcrumb_segment *out,
                             uint32_t cap, uint32_t *out_count,
                             uint8_t *out_dropdown);

/* Icon & thumbnail system (since 2.11).
 *
 * Plumbing for icon resolution and async thumbnail generation. The
 * widget core does not decode PNG/JPEG, does not maintain an icon
 * theme, does not touch the disk — the host registers two callbacks
 * via a caller-owned `capy_icon_provider`, and the widget core just
 * routes calls plus emits a `THUMBNAIL_READY` event when the host
 * signals completion.
 *
 *  - `capy_icon_resolve(ctx, mime_type, extension, out_icon_id)`
 *    forwards to `provider->resolve`. The host can match by MIME type
 *    (preferred), by extension (fallback), or both. CapyUI itself
 *    falls back deterministically to `image_id = 0` (the default
 *    silhouette) when there is no provider, when the host callback
 *    is NULL, or when the host returns `-1`. Returns `0` on success,
 *    `-1` on NULL `ctx` or `out_icon_id`.
 *  - `capy_icon_thumbnail_request(ctx, path, out_request_id)`
 *    forwards to `provider->thumbnail_request`. The host kicks off
 *    its async pipeline (decode + cache) and writes a non-zero
 *    request id into `*out_request_id`. Returns `0` on success, `-1`
 *    on NULL args, missing callback, or host failure.
 *  - `capy_icon_thumbnail_ready(ctx, root, request_id, image_id)`
 *    emits `CAPY_WIDGET_EVENT_THUMBNAIL_READY` on `root` (when
 *    non-NULL) so the requesting widget can swap from the placeholder
 *    icon to the decoded thumbnail. `event->x = (int32_t)request_id`,
 *    `event->y = (int32_t)image_id`. Returns `0` on success, `-1` on
 *    NULL ctx or `request_id == 0`.
 *
 *  - The 0-id `image_id` is reserved for the deterministic default
 *    silhouette. Hosts must assign image ids `>= 1` to real icons.
 *
 * Deferred for follow-up additive minors:
 *  - **Icon theme cascade.** Multiple providers chained (user theme
 *    \u2192 desktop default \u2192 fallback). Today one provider per ctx.
 *  - **Vector icons.** SVG/PDF rendering pipeline lives outside the
 *    widget core. The image_id contract assumes raster.
 *  - **Cancellation.** `capy_icon_thumbnail_cancel(request_id)` to
 *    abort in-flight host work for widgets that scrolled offscreen.
 *  - **Stack/group folder icons.** v2.9 deferred group folders; the
 *    icon descriptor will gain a `stack_count` field then. */
struct capy_icon_provider;

typedef int (*capy_icon_resolve_fn)(void *user_data, const char *mime_type,
                                    const char *extension,
                                    uint32_t *out_icon_id);
typedef int (*capy_icon_thumbnail_request_fn)(void *user_data,
                                              const char *path,
                                              uint32_t *out_request_id);

struct capy_icon_provider {
  capy_icon_resolve_fn resolve;
  capy_icon_thumbnail_request_fn thumbnail_request;
  void *user_data;
};

void capy_widget_set_icon_provider(struct capy_widget_context *ctx,
                                   struct capy_icon_provider *provider);
int capy_icon_resolve(struct capy_widget_context *ctx, const char *mime_type,
                      const char *extension, uint32_t *out_icon_id);
int capy_icon_thumbnail_request(struct capy_widget_context *ctx,
                                const char *path, uint32_t *out_request_id);
int capy_icon_thumbnail_ready(struct capy_widget_context *ctx,
                              struct capy_widget *root, uint32_t request_id,
                              uint32_t image_id);

/* Wallpaper management (since 2.12).
 *
 * Plumbing for the desktop session's wallpaper config: a `capy_wallpaper_config`
 * record (default image id + fit mode + flags + optional per-monitor index +
 * slideshow interval), optional `capy_wallpaper_slide` entries (slideshow
 * sequence with per-image dwell), and a canonical binary blob `CWLP` that the
 * host persists under `/var/state/capyui/wallpaper.bin` (global) or per-user.
 *
 * The widget core never reads pixels, never schedules the slideshow timer,
 * never composes the wallpaper into a render target. It only owns the
 * struct shapes plus the round-trip serialization so the desktop session,
 * the v2.13 login screen, the Settings → Personalization app and a future
 * cross-host migration tool consume the same format.
 *
 *   Blob layout (little-endian, magic `"CWLP"`):
 *
 *   offset  size  field
 *   0       4     magic = "CWLP"
 *   4       2     format_version (uint16, currently 1)
 *   6       1     fit_mode (CAPY_WALLPAPER_FIT_*)
 *   7       1     flags    (CAPY_WALLPAPER_FLAG_*)
 *   8       4     FNV-1a 32-bit checksum over bytes [24, end)
 *   12      4     default_image_id (uint32; 0 = solid colour fallback)
 *   16      2     slideshow_interval_sec (uint16; 0 = no slideshow)
 *   18      2     slide_count (uint16)
 *   20      2     monitor_index (uint16; 0 = global, >= 1 per-monitor)
 *   22      2     reserved (must be 0)
 *   24+     ...   N slides of 8 bytes each:
 *                   uint32  image_id        (>= 1)
 *                   uint16  duration_sec    (>= 1)
 *                   uint16  reserved        (must be 0)
 *
 *  - `capy_wallpaper_validate(buf, len)` returns `0` on a well-formed blob,
 *    `-1` otherwise. Checks: magic, `format_version <= 1`, reserved
 *    (header + per-entry) zero, `fit_mode < CAPY_WALLPAPER_FIT_COUNT`,
 *    `len == header + slide_count * entry_size`, FNV-1a body checksum,
 *    every slide `image_id > 0` and `duration_sec > 0`.
 *  - `capy_wallpaper_load(buf, len, out_config, out_slides, cap, out_count)`
 *    validates then writes the config into `*out_config` and copies up to
 *    `cap` slides into `out_slides`. `*out_count` always carries the
 *    declared `slide_count`, so callers detect a short buffer by
 *    `*out_count > cap`. Returns `0` on success, `-1` on validation failure
 *    or NULL args; `out_slides` may be NULL only when `cap == 0`.
 *  - `capy_wallpaper_serialize(config, slides, count, out, cap)` writes a
 *    canonical blob into `out[]` and returns the number of bytes written,
 *    `-1` on NULL args, NULL `slides` with `count > 0`, invalid
 *    `fit_mode`, slide with `image_id == 0` or `duration_sec == 0`, or
 *    `cap` too small. Output is byte-deterministic.
 *
 * Deferred for follow-up additive minors:
 *  - **Image format negotiation.** The blob carries opaque `image_id`s;
 *    decoding/scaling lives in the v2.11 icon provider.
 *  - **Live wallpaper / video.** Today only static stills + interval
 *    slideshow. A future minor can add a `LIVE_HANDLE` flag and a
 *    callback in the desktop session.
 *  - **Per-locale defaults.** A locale-aware factory will piggyback on
 *    the v0.13 locale surface.
 *  - **Transition effects.** Cross-fade / slide / dissolve between
 *    slides is a desktop-session animation policy. */
#define CAPY_WALLPAPER_MAGIC0           0x43u /* 'C' */
#define CAPY_WALLPAPER_MAGIC1           0x57u /* 'W' */
#define CAPY_WALLPAPER_MAGIC2           0x4Cu /* 'L' */
#define CAPY_WALLPAPER_MAGIC3           0x50u /* 'P' */
#define CAPY_WALLPAPER_FORMAT_VERSION   1u
#define CAPY_WALLPAPER_HEADER_SIZE      24u
#define CAPY_WALLPAPER_ENTRY_SIZE       8u

#define CAPY_WALLPAPER_FIT_STRETCH  0u
#define CAPY_WALLPAPER_FIT_FILL     1u
#define CAPY_WALLPAPER_FIT_FIT      2u
#define CAPY_WALLPAPER_FIT_CENTER   3u
#define CAPY_WALLPAPER_FIT_TILE     4u
#define CAPY_WALLPAPER_FIT_COUNT    5u

#define CAPY_WALLPAPER_FLAG_RANDOM      0x01u
#define CAPY_WALLPAPER_FLAG_PER_MONITOR 0x02u

struct capy_wallpaper_config {
  uint32_t default_image_id;
  uint16_t slideshow_interval_sec;
  uint16_t monitor_index;
  uint8_t fit_mode;
  uint8_t flags;
  uint16_t reserved;
};

struct capy_wallpaper_slide {
  uint32_t image_id;
  uint16_t duration_sec;
  uint16_t reserved;
};

int capy_wallpaper_validate(const void *buf, uint32_t len);
int capy_wallpaper_load(const void *buf, uint32_t len,
                        struct capy_wallpaper_config *out_config,
                        struct capy_wallpaper_slide *out_slides, uint32_t cap,
                        uint32_t *out_count);
int capy_wallpaper_serialize(const struct capy_wallpaper_config *config,
                             const struct capy_wallpaper_slide *slides,
                             uint32_t count, void *out, uint32_t cap);

/* Login screen (since 2.13).
 *
 * Plumbing primitives shared by the boot login surface, the lock
 * screen and the user switcher. The widget core never authenticates
 * (that lives in the v2.7 `capy_user_directory`), never composes the
 * background (v2.12 wallpaper), never resolves avatar bitmaps (v2.11
 * icon provider) — this slice publishes only the taxonomy + the
 * deterministic layout decision so all three consumers stay in sync.
 *
 * Layout rules (deterministic, integer-only):
 *  - `user_count == 0` or `1` → `CAPY_LOGIN_LAYOUT_SINGLE`. The host
 *    centres one avatar + name + password field. With zero users this
 *    is the first-boot "create root account" prompt.
 *  - `2 ≤ user_count ≤ 6` → `CAPY_LOGIN_LAYOUT_GRID`. The host draws
 *    a 2- to 3-column avatar grid.
 *  - `user_count ≥ 7` → `CAPY_LOGIN_LAYOUT_LIST`. The host shows a
 *    scrollable list with a search box on top.
 *
 * Power options: `CAPY_LOGIN_POWER_*` is a stable taxonomy of the
 * actions a login surface can offer (shutdown / reboot / sleep /
 * lock / logout). The widget core does not implement any of them;
 * the desktop session wires each constant to the appropriate host
 * call. Hosts may omit any subset on a per-policy basis (kiosk mode
 * usually hides shutdown + reboot).
 *
 * Deferred for follow-up additive minors:
 *  - **Biometric / SSO challenge UI.** A new `capy_login_challenge`
 *    enum + per-method input affordances. Today every method funnels
 *    through `capy_user_directory.authenticate` (v2.7 deferred).
 *  - **PIN keypad widget.** A 0..9 numeric pad is a desktop-session
 *    composite; widget core needs no new type for it.
 *  - **Locked-badge animation.** Visual treatment of the
 *    `capy_user_account.is_locked` bit lives in the desktop session.
 *  - **Resume from sleep wake-screen.** A separate surface that
 *    shares the same `capy_login_choose_layout` math. */
#define CAPY_LOGIN_LAYOUT_SINGLE   0u
#define CAPY_LOGIN_LAYOUT_GRID     1u
#define CAPY_LOGIN_LAYOUT_LIST     2u
#define CAPY_LOGIN_LAYOUT_COUNT    3u

#define CAPY_LOGIN_GRID_THRESHOLD  2u
#define CAPY_LOGIN_LIST_THRESHOLD  7u

#define CAPY_LOGIN_POWER_NONE      0u
#define CAPY_LOGIN_POWER_SHUTDOWN  1u
#define CAPY_LOGIN_POWER_REBOOT    2u
#define CAPY_LOGIN_POWER_SLEEP     3u
#define CAPY_LOGIN_POWER_LOCK      4u
#define CAPY_LOGIN_POWER_LOGOUT    5u
#define CAPY_LOGIN_POWER_COUNT     6u

uint8_t capy_login_choose_layout(uint32_t user_count);

/* Advanced widget state — date picker (since 2.14).
 *
 * Lifts the DATE_PICKER value (enum-only since 2.1) from caller-owned
 * `user_data` into a first-class tail field (`capy_widget.date_value`). The
 * widget core owns calendar validation and nothing else: no locale, no
 * formatting, no clock, no allocation. Every function is deterministic and
 * total. DL schema is unchanged (DATE_PICKER still emits via the generic
 * pipeline; this slice only adds state + validation).
 *
 *  - `capy_date_is_valid(year, month, day)` — pure proleptic-Gregorian
 *    calendar predicate (month 1..12, day within month incl. Feb 29 on leap
 *    years, year >= 1). Returns 1 for a real date, else 0. Hosts can call it
 *    to pre-validate before the setter.
 *  - `capy_widget_set_date(w, year, month, day)` — validates then stores.
 *    Returns 0 on success; -1 on NULL `w`, `w->type != CAPY_WIDGET_DATE_PICKER`,
 *    or an out-of-range date. On failure the stored value is left unchanged
 *    (fail-closed).
 *  - `capy_widget_clear_date(w)` — resets the value to the unset sentinel
 *    (all zero). Returns 0; -1 on NULL / wrong type.
 *  - `capy_widget_get_date(w, out)` — copies the stored value into `*out`.
 *    Returns 1 when a valid date is set, 0 when unset, -1 on NULL `w` /
 *    wrong type / NULL `out`. */
int capy_date_is_valid(uint16_t year, uint8_t month, uint8_t day);
int capy_widget_set_date(struct capy_widget *w, uint16_t year, uint8_t month,
                         uint8_t day);
int capy_widget_clear_date(struct capy_widget *w);
int capy_widget_get_date(const struct capy_widget *w, struct capy_date *out);

/* Advanced widget state — color picker (since 2.15).
 *
 * Second advanced-widget family (enum-only since 2.1) lifted into a tail
 * field. Mirrors the date-picker shape: total / deterministic / zero-alloc /
 * zero-float. Colours use the 0xAARRGGBB layout shared with
 * `capy_widget_style` and the theme tokens.
 *
 *  - `capy_color_pack(r, g, b, a)` — packs 8-bit channels into 0xAARRGGBB.
 *    Pure and total; each channel is cast to `uint32_t` before shifting so
 *    there is no signed overflow. The inverse is the established
 *    `(argb >> 16) & 0xFFu` (red) / `>> 8` (green) / `& 0xFFu` (blue).
 *  - `capy_widget_set_color(w, argb)` — stores `argb` and marks the value
 *    set. Returns 0; -1 on NULL `w` or `w->type != CAPY_WIDGET_COLOR_PICKER`.
 *    Every 32-bit value is a valid colour, so there is no value validation.
 *  - `capy_widget_clear_color(w)` — resets to the unset state (colour 0,
 *    flag 0). Returns 0; -1 on NULL / wrong type.
 *  - `capy_widget_get_color(w, out)` — copies the stored colour into `*out`
 *    (0 when unset). Returns 1 when a colour is set, 0 when unset, -1 on
 *    NULL `w` / wrong type / NULL `out`. */
uint32_t capy_color_pack(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
int capy_widget_set_color(struct capy_widget *w, uint32_t argb);
int capy_widget_clear_color(struct capy_widget *w);
int capy_widget_get_color(const struct capy_widget *w, uint32_t *out);

/* Advanced widget state — table columns (since 2.16).
 *
 * Third advanced-widget family (enum-only since 2.1) lifted into tail fields.
 * Unlike the scalar date/color pickers, the table stores a caller-owned
 * column-width array (zero-alloc: CapyUI never copies/frees it) plus a count,
 * and the accessors are bounds-checked / fail-closed. Widths are in pixels.
 *
 *  - `capy_widget_set_table_columns(w, widths, count)` — stores the model.
 *    `count == 0` clears (widths ignored). `count > 0` requires non-NULL
 *    `widths` (which must outlive the widget's use). Returns 0; -1 on NULL `w`,
 *    `w->type != CAPY_WIDGET_TABLE`, or `count > 0 && widths == NULL`. On
 *    failure the stored model is left unchanged (fail-closed).
 *  - `capy_widget_clear_table_columns(w)` — resets to no columns (NULL/0).
 *    Returns 0; -1 on NULL / wrong type.
 *  - `capy_widget_table_column_count(w)` — number of columns (0 when none),
 *    or -1 on NULL / wrong type.
 *  - `capy_widget_table_column_width(w, index, out_width)` — writes
 *    `widths[index]` into `*out_width`. Returns 0; -1 on NULL `w` / wrong type
 *    / NULL `out_width` / no model / `index >= count` (bounds-checked). */
int capy_widget_set_table_columns(struct capy_widget *w,
                                  const uint16_t *widths, uint16_t count);
int capy_widget_clear_table_columns(struct capy_widget *w);
int capy_widget_table_column_count(const struct capy_widget *w);
int capy_widget_table_column_width(const struct capy_widget *w, uint16_t index,
                                   uint16_t *out_width);

/* Advanced widget state — autocomplete suggestions (since 2.17).
 *
 * Fourth advanced-widget family lifted into tail fields. Like the table it
 * stores a caller-owned model (zero-alloc: the `const char *const *` array is
 * never copied/freed), but it adds a mutable selection cursor with clamp /
 * fail-closed semantics. The suggestion strings are caller-owned and must
 * outlive the widget's use.
 *
 *  - `capy_widget_set_autocomplete(w, items, count)` — stores the list and
 *    resets the selection to "none" (-1). `count == 0` clears (items ignored).
 *    `count > 0` requires non-NULL `items`. Returns 0; -1 on NULL `w`,
 *    `type != AUTOCOMPLETE`, or `count > 0 && items == NULL` (stored model left
 *    unchanged on failure).
 *  - `capy_widget_clear_autocomplete(w)` — resets to NULL/0 and selection -1.
 *    Returns 0; -1 on NULL / wrong type.
 *  - `capy_widget_autocomplete_count(w)` — number of suggestions (0 when none),
 *    or -1 on NULL / wrong type.
 *  - `capy_widget_autocomplete_item(w, index, out_item)` — writes
 *    `items[index]` into `*out_item`. Returns 0; -1 on NULL `w` / wrong type /
 *    NULL `out_item` / no list / `index >= count` (bounds-checked).
 *  - `capy_widget_set_autocomplete_selected(w, index)` — `index == -1` clears
 *    the selection; `0 <= index < count` selects. Returns 0; -1 on NULL `w` /
 *    wrong type / `index < -1` / `index >= count` (fail-closed).
 *  - `capy_widget_get_autocomplete_selected(w, out_index)` — writes the live
 *    selection (clamped against the current count; -1 when none) into
 *    `*out_index`. Returns 1 when an entry is selected, 0 when none, -1 on
 *    NULL `w` / wrong type / NULL `out_index`. */
int capy_widget_set_autocomplete(struct capy_widget *w,
                                 const char *const *items, uint16_t count);
int capy_widget_clear_autocomplete(struct capy_widget *w);
int capy_widget_autocomplete_count(const struct capy_widget *w);
int capy_widget_autocomplete_item(const struct capy_widget *w, uint16_t index,
                                  const char **out_item);
int capy_widget_set_autocomplete_selected(struct capy_widget *w, int32_t index);
int capy_widget_get_autocomplete_selected(const struct capy_widget *w,
                                          int32_t *out_index);

/* Advanced widget state — tree hierarchy (since 2.18).
 *
 * Fifth advanced-widget family. Per-node scalar state (fold flag + indent
 * depth) plus the track's first hierarchy-walking read: row visibility is
 * derived from the node's TREE ancestors. Total / deterministic / zero-alloc /
 * zero-float / fail-closed by widget type.
 *
 *  - `capy_widget_set_tree_collapsed(w, collapsed)` — sets the fold flag (any
 *    nonzero `collapsed` stores 1). Returns 0; -1 on NULL `w` / `type != TREE`.
 *  - `capy_widget_tree_is_collapsed(w)` — 1 collapsed, 0 expanded, or -1 on
 *    NULL / wrong type. A fresh node is expanded (0).
 *  - `capy_widget_set_tree_depth(w, depth)` — sets the indent level. Returns 0;
 *    -1 on NULL / wrong type.
 *  - `capy_widget_tree_depth(w)` — indent level (0 at root), or -1 on NULL /
 *    wrong type.
 *  - `capy_widget_tree_row_visible(w)` — walks the parent chain: returns 0 when
 *    any ancestor that is a TREE node is collapsed (the node's own collapse
 *    does not hide itself), 1 when no collapsed TREE ancestor is found, -1 on
 *    NULL `w` / `type != TREE`. Deterministic; non-TREE ancestors are skipped. */
int capy_widget_set_tree_collapsed(struct capy_widget *w, int collapsed);
int capy_widget_tree_is_collapsed(const struct capy_widget *w);
int capy_widget_set_tree_depth(struct capy_widget *w, uint16_t depth);
int capy_widget_tree_depth(const struct capy_widget *w);
int capy_widget_tree_row_visible(const struct capy_widget *w);

/* Advanced widget state — chart dataset (since 2.19).
 *
 * Sixth advanced-widget family. Caller-owned signed data array (zero-alloc:
 * the `const int32_t *` array is never copied/freed) plus a bounds-checked
 * accessor and a deterministic numeric reduction (signed min/max). Total /
 * deterministic / zero-alloc / zero-float / fail-closed by widget type.
 *
 *  - `capy_widget_set_chart_data(w, values, count)` — stores the dataset.
 *    `count == 0` clears (values ignored). `count > 0` requires non-NULL
 *    `values` (which must outlive the widget's use). Returns 0; -1 on NULL `w`,
 *    `type != CHART`, or `count > 0 && values == NULL` (state unchanged on fail).
 *  - `capy_widget_clear_chart_data(w)` — resets to NULL/0. 0; -1 NULL / type.
 *  - `capy_widget_chart_count(w)` — number of points (0 when none), or -1.
 *  - `capy_widget_chart_value(w, index, out_value)` — writes `values[index]`
 *    into `*out_value`. 0; -1 on NULL / wrong type / NULL `out_value` / no data
 *    / `index >= count` (bounds-checked).
 *  - `capy_widget_chart_range(w, out_min, out_max)` — single-pass signed
 *    min/max over the dataset. Returns 1 with the bounds written when data is
 *    present, 0 (and `*out_min = *out_max = 0`) when empty, -1 on NULL `w` /
 *    wrong type / NULL `out_min` / NULL `out_max`. */
int capy_widget_set_chart_data(struct capy_widget *w, const int32_t *values,
                               uint16_t count);
int capy_widget_clear_chart_data(struct capy_widget *w);
int capy_widget_chart_count(const struct capy_widget *w);
int capy_widget_chart_value(const struct capy_widget *w, uint16_t index,
                            int32_t *out_value);
int capy_widget_chart_range(const struct capy_widget *w, int32_t *out_min,
                            int32_t *out_max);

/* Advanced widget state — rich-text ranges (since 2.20).
 *
 * Seventh advanced-widget family. Caller-owned array of attributed style runs
 * (zero-alloc: the `const struct capy_text_range *` array is never
 * copied/freed) plus a bounds-checked accessor and a per-offset lookup. Total /
 * deterministic / zero-alloc / zero-float / fail-closed by widget type.
 *
 *  - `capy_widget_set_rich_text_ranges(w, ranges, count)` — stores the run
 *    array. `count == 0` clears (ranges ignored). `count > 0` requires non-NULL
 *    `ranges` (which must outlive the widget's use). Returns 0; -1 on NULL `w`,
 *    `type != RICH_TEXT`, or `count > 0 && ranges == NULL` (state unchanged on
 *    fail).
 *  - `capy_widget_clear_rich_text_ranges(w)` — resets to NULL/0. 0; -1 NULL /
 *    type.
 *  - `capy_widget_rich_text_range_count(w)` — number of runs (0 when none), or
 *    -1 on NULL / wrong type.
 *  - `capy_widget_rich_text_range(w, index, out_range)` — copies
 *    `ranges[index]` into `*out_range`. 0; -1 on NULL / wrong type / NULL
 *    `out_range` / no data / `index >= count` (bounds-checked).
 *  - `capy_widget_rich_text_range_at(w, offset, out_range)` — finds the run
 *    covering character `offset` (half-open `[start, start + length)`;
 *    zero-length runs cover nothing). When runs overlap, the **last** matching
 *    run wins (later entries override earlier — standard attributed-string
 *    layering). Returns 1 with the run written when a cover is found, 0 (and a
 *    zeroed `*out_range`) when none covers `offset`, -1 on NULL `w` / wrong
 *    type / NULL `out_range`. Deterministic single pass; overflow-safe. */
int capy_widget_set_rich_text_ranges(struct capy_widget *w,
                                     const struct capy_text_range *ranges,
                                     uint16_t count);
int capy_widget_clear_rich_text_ranges(struct capy_widget *w);
int capy_widget_rich_text_range_count(const struct capy_widget *w);
int capy_widget_rich_text_range(const struct capy_widget *w, uint16_t index,
                                struct capy_text_range *out_range);
int capy_widget_rich_text_range_at(const struct capy_widget *w, uint32_t offset,
                                   struct capy_text_range *out_range);

/* Advanced widget state — canvas draw callback (since 2.21).
 *
 * Eighth and final advanced-widget family — and the first carrying a
 * *behaviour* (a host callback) instead of a data model, which closes the
 * 8/8 advanced-widget state track opened in 2.1. The widget core stores the
 * callback and never invokes it during `capy_widget_emit` (emit stays
 * self-contained / deterministic); the host calls `capy_widget_canvas_draw`
 * while building a frame to let the canvas append its own display-list ops.
 * Zero-alloc / fail-closed by widget type.
 *
 *  - `capy_widget_set_canvas_draw(w, fn, user_data)` — stores the callback and
 *    its opaque `user_data`. `fn == NULL` clears (user_data reset too).
 *    Returns `0`, or `-1` on NULL `w` / `type != CANVAS`.
 *  - `capy_widget_clear_canvas_draw(w)` — resets callback + user_data to NULL.
 *    `0`, or `-1` on NULL / wrong type.
 *  - `capy_widget_has_canvas_draw(w)` — `1` when a callback is set, `0` when
 *    not, `-1` on NULL / wrong type.
 *  - `capy_widget_canvas_user_data(w, out_user_data)` — writes the stored
 *    user_data into `*out_user_data` (NULL when unset). `0`, or `-1` on NULL
 *    `w` / wrong type / NULL `out_user_data`.
 *  - `capy_widget_canvas_draw(w, dl)` — invokes the stored callback against
 *    `dl`, forwarding the stored user_data. Returns `0` when the callback ran
 *    and returned `0`; `-1` on NULL `w` / wrong type / NULL `dl` / no callback
 *    set / the callback returning non-zero. Deterministic iff the callback is. */
int capy_widget_set_canvas_draw(struct capy_widget *w, capy_canvas_draw_fn fn,
                                void *user_data);
int capy_widget_clear_canvas_draw(struct capy_widget *w);
int capy_widget_has_canvas_draw(const struct capy_widget *w);
int capy_widget_canvas_user_data(const struct capy_widget *w,
                                 void **out_user_data);
int capy_widget_canvas_draw(struct capy_widget *w, struct capy_display_list *dl);

/* Pool allocator + measure cache (since 0.15).
 *
 * `capy_widget_pool` is a bump allocator over a caller-supplied buffer.
 * Allocations align up to 16 bytes (covers `_Alignof(struct capy_widget)`
 * on all CapyUI targets). `free` is a no-op — the pool is bulk-managed via
 * `capy_widget_pool_reset`, which preserves `high_water` for diagnostics.
 *
 * `capy_widget_allocator_from_pool` returns an allocator descriptor that
 * can be passed straight to `capy_widget_context_init`, so a tree backed by
 * a pool is just `init(ctx, &pool_allocator); create(...)`.
 *
 * `capy_widget_invalidate(w)` walks from `w` up through `parent` pointers,
 * setting `layout_dirty = 1` on every ancestor (including `w` itself). The
 * next `capy_widget_measure` on any of those widgets will recompute. NULL
 * is a no-op. */
struct capy_widget_pool {
  void *buffer;
  uint32_t size;
  uint32_t used;
  uint32_t high_water;
};

void capy_widget_pool_init(struct capy_widget_pool *p, void *buf,
                           uint32_t size);
void capy_widget_pool_reset(struct capy_widget_pool *p);
struct capy_widget_allocator capy_widget_allocator_from_pool(
    struct capy_widget_pool *p);
void capy_widget_invalidate(struct capy_widget *widget);

/* Damage tracking + frame budget (since 1.1).
 *
 * `capy_widget_invalidate` (since 0.15) now also bumps `dirty_version` on
 * every widget it visits (additive change — existing callers still observe
 * `layout_dirty = 1` propagation). `capy_widget_invalidate_subtree` walks
 * **downward** from `widget`, setting `layout_dirty = 1` and bumping
 * `dirty_version` on `widget` and every descendant. NULL is a no-op.
 *
 * `capy_widget_dirty_version` returns the current monotonic counter for a
 * widget, or `0` for NULL. Hosts sample before/after to detect changes
 * independently of `capy_widget_measure` cache decisions.
 *
 * `capy_widget_frame_budget` stores a host hint about available time per
 * frame (in microseconds, 0 = no hint). CapyUI never enforces it — the
 * value is published in `ctx->frame_budget_microseconds` for cooperative
 * degradation in host-level loops. Passing `ctx == NULL` is a no-op.
 *
 * `capy_display_list_diff_incremental(prev, next, out_dirty, out_cap)` is
 * semantically equivalent to `capy_widget_diff` (since 0.8) but skips the
 * common **leading** prefix of the two display-lists before running the
 * positional inner diff. When most changes are at the end of the list
 * (e.g. only the last few widgets in a stack move), the cost is
 * proportional to the changed range plus the prefix scan instead of the
 * total `count`. Output sequence and overflow behaviour are byte-identical
 * to `capy_widget_diff` given the same `(prev, next)`. Suffix scanning is
 * deliberately not applied because it would re-align prev/next and drop
 * the trailing rects that the positional algorithm emits. */
void capy_widget_invalidate_subtree(struct capy_widget *widget);
uint32_t capy_widget_dirty_version(const struct capy_widget *widget);
void capy_widget_frame_budget(struct capy_widget_context *ctx,
                              uint32_t microseconds);
int capy_display_list_diff_incremental(const struct capy_display_list *prev,
                                       const struct capy_display_list *next,
                                       struct capy_ui_rect *out_dirty,
                                       uint32_t out_cap);

/* Accessibility tree (since 0.11).
 *
 * `capy_widget_a11y_snapshot` walks `root` in pre-order and writes one
 * `capy_a11y_node` per visited widget into `out[]`. Returns the number of
 * nodes written on success, or `-1` on overflow of `cap` (in which case
 * `out` is partially filled up to capacity) or NULL `root`.
 *
 * `widget_id` is a deterministic hash of the path (sequence of child
 * indices from the root). The same widget at the same position yields the
 * same `widget_id` across snapshots. Root nodes have `parent_id = 0`.
 *
 * `role` and `label` point into static string literals or directly into
 * `widget->text` — CapyUI never allocates here. `description` is NULL for
 * now (reserved for future aria-describedby integration).
 *
 * Snapshot does not mutate the widget tree. */
#define CAPY_A11Y_FOCUSED   0x1u
#define CAPY_A11Y_CHECKED   0x2u
#define CAPY_A11Y_DISABLED  0x4u
#define CAPY_A11Y_HIDDEN    0x8u
#define CAPY_A11Y_PRESSED   0x10u
#define CAPY_A11Y_EXPANDED  0x20u
#define CAPY_A11Y_SELECTED  0x40u
#define CAPY_A11Y_READONLY  0x80u

struct capy_a11y_node {
  uint32_t widget_id;
  const char *role;
  const char *label;
  const char *description;
  uint32_t state_flags;
  uint32_t parent_id;
};

int capy_widget_a11y_snapshot(struct capy_widget *root,
                              struct capy_a11y_node *out, uint32_t cap);

/* Locale + RTL (since 0.13).
 *
 * `capy_locale_default()` returns ("en-US", LTR, EN). The same default is
 * seeded into `capy_widget_context_init` so pre-0.13 callers see backward-
 * compatible behaviour without explicit setup.
 *
 * `capy_context_set_locale(ctx, l)` copies the locale into the context.
 * Passing `l == NULL` restores the default.
 *
 * `capy_locale_plural(l, n)` returns the index of the plural form for `n`
 * under the locale's plural rule. NULL locale falls back to EN rules.
 *
 * `capy_locale_format(l, out, out_cap, fmt, ...)` is a tiny printf subset
 * with no float specifiers (zero float in the core). Supported converters:
 *   %d  signed decimal (int promoted to int32_t)
 *   %u  unsigned decimal (uint32_t)
 *   %x  lowercase unsigned hex (uint32_t)
 *   %s  C string (NULL prints nothing)
 *   %%  literal '%'
 * Returns the number of characters written (excluding the terminating
 * NUL) on success, or `-1` on overflow / invalid arguments. The locale
 * argument is currently unused but reserved for future locale-aware digit
 * shaping; the API surface is stable. */
struct capy_locale capy_locale_default(void);
void capy_context_set_locale(struct capy_widget_context *ctx,
                             const struct capy_locale *l);
int capy_locale_plural(const struct capy_locale *l, uint32_t n);
int capy_locale_format(const struct capy_locale *l, char *out,
                       uint16_t out_cap, const char *fmt, ...);

/* Text metrics hook (since 0.9).
 *
 * `capy_widget_set_text_metrics_hook` stores the host callback in the
 * context. Passing `fn == NULL` clears the hook and restores the
 * deterministic fallback.
 *
 * `capy_widget_measure_text` returns `0` on success with
 * `*out_width`/`*out_height` populated, and `-1` on null arguments. When
 * the hook is set and returns `0`, the values come from the hook; when
 * the hook is null or returns `-1`, the deterministic fallback applies:
 *
 *   width  = (font_size > 0 ? font_size : 16) / 2 * len
 *   height = (font_size > 0 ? font_size : 16)
 *
 * `text == NULL` or `len == 0` yields `width = 0`, `height = font_size`
 * (or 16 when `font_size == 0`). The fallback never invokes the hook. */
void capy_widget_set_text_metrics_hook(struct capy_widget_context *ctx,
                                       capy_text_metrics_fn fn,
                                       void *user_data);
int capy_widget_measure_text(const struct capy_widget_context *ctx,
                             uint16_t font_id, uint8_t font_size,
                             const char *text, uint16_t len,
                             uint32_t *out_width, uint32_t *out_height);

#endif
