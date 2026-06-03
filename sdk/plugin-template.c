/* Minimal CapyUI plugin template (since 2.0).
 *
 * Build:
 *   cc -std=c11 -Wall -Wextra -Werror -pedantic -Isrc/widget \
 *      sdk/plugin-template.c -c -o build/plugin-template.o
 *
 * The template is intentionally functional — every callback is a
 * documented no-op, every field is filled in. Copy the file, rename the
 * `id`, swap the callbacks, and ship. */

#include "capy_widget.h"

/* Per-plugin allocator. In a real plugin you would point this at you
 * own arena (e.g. a `capy_widget_pool` from `0.15` or a `capy_slab` from
 * `1.7`). The widget core stores the pointer but never calls into it. */
static void *template_plugin_alloc(uint32_t bytes, void *user_data) {
  (void)bytes;
  (void)user_data;
  return 0;
}

static void template_plugin_free(void *ptr, void *user_data) {
  (void)ptr;
  (void)user_data;
}

static int template_plugin_init(struct capy_plugin_context *pc) {
  /* Host hands the descriptor back via `pc->descriptor` and an opaque
   * `user_data` slot that the host itself populates. Return 0 on
   * success; a non-zero return signals init failure to the host. */
  (void)pc;
  return 0;
}

static void template_plugin_destroy(struct capy_plugin_context *pc) {
  /* Invoked exactly once by `capy_plugin_unregister_all`. Release any
   * resources you allocated via the per-plugin allocator. */
  (void)pc;
}

static int template_plugin_on_event(struct capy_plugin_context *pc,
                                    struct capy_widget *target,
                                    const struct capy_widget_event *ev) {
  /* Return 1 to claim the event (host stops further routing), 0 to
   * let the event continue bubbling. The widget core never calls this
   * directly in 2.0; the host dispatcher decides when to invoke. */
  (void)pc;
  (void)target;
  (void)ev;
  return 0;
}

static int template_plugin_emit(struct capy_plugin_context *pc,
                                struct capy_widget *target,
                                struct capy_display_list *dl) {
  /* Append `CAPY_DL_PLUGIN_OP` (schema 7) commands here. The 32-byte
   * payload reuses the standard cmd fields; `image_id` should carry
   * your plugin id so the host can dispatch the op back to you on the
   * consumer side. Return 0 on success, -1 on overflow / failure. */
  (void)pc;
  (void)target;
  (void)dl;
  return 0;
}

const struct capy_plugin_descriptor template_plugin_descriptor = {
    .id = "com.example.template",
    .version = "0.1.0",
    .capy_ui_abi_required = CAPYUI_API_VERSION_TAG,
    .allocator = {template_plugin_alloc, template_plugin_free, 0},
    .timeout_microseconds = 1000u, /* 1 ms soft deadline per callback */
    .init = template_plugin_init,
    .destroy = template_plugin_destroy,
    .on_event = template_plugin_on_event,
    .emit = template_plugin_emit,
};

/* Hosts typically register the descriptor at startup:
 *
 *   int slot = capy_plugin_register(&widget_ctx, &template_plugin_descriptor);
 *   if (slot < 0) { fail; }
 *
 * and clean up at shutdown:
 *
 *   capy_plugin_unregister_all(&widget_ctx);
 */
