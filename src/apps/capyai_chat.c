/*
 * CapyAI graphical chat app.
 *
 * A desktop window with the capybara system logo, a scrolling conversation,
 * a text input line and three task-scoped permission buttons (Escrita /
 * Deletar / Sistema) that map to the executor's independent risk gates. Shares
 * the same brain as
 * the `capyai` terminal command (services/capyai.h + capy-ai-core built into
 * the kernel). Renders directly into the window surface (font + rect fills),
 * so it needs no widget/display-list bridge.
 *
 * When the CapyAI sibling core is absent at build time (no CAPYOS_HAVE_CAPYAI)
 * the app still opens and works as a UI, but submitting a message reports the
 * assistant module is unavailable.
 */
#include "apps/capyai_chat.h"

#include "gui/compositor.h"
#include "gui/desktop_runtime.h"
#include "gui/font.h"
#include "branding/capyos_icon_mask.h"

#include <stddef.h>
#include <stdint.h>

#ifdef CAPYOS_HAVE_CAPYAI
#include "services/capyai.h"
#include "services/capyai_async.h"
#include "services/capyai_system_actions.h"
#endif

#ifdef CAPYOS_CAPYAI_GUI_ASYNC_SMOKE
#include "drivers/serial/serial_com1.h"
#include "kernel/task.h"
#include "kernel/worker.h"
#endif

#define CHAT_HEADER_H 44
#define CHAT_BTN_TOP 50
#define CHAT_BTN_H 26
#define CHAT_LINE_H 16
#define CHAT_INPUT_H 26
#define CHAT_PAD 8

static struct capyai_chat_app g_chat;
static int g_chat_open = 0;
static uint64_t g_chat_generation = 0u;

#ifdef CAPYOS_HAVE_CAPYAI
/* This context is service-owned, not window-owned. It remains valid if the
 * user closes/reopens the chat while a command is running. The session copy
 * prevents the worker from retaining the foreground shell's stack pointer. */
struct capyai_chat_dispatch_context {
    struct session_context session;
    struct shell_context shell;
};

static struct capyai_chat_dispatch_context g_dispatch_context;
static int g_dispatch_context_in_use = 0;
static char *g_capture_buffer = NULL;
static size_t g_capture_capacity = 0u;
static size_t g_capture_length = 0u;
static int g_async_ready = 0;
#endif

#ifdef CAPYOS_CAPYAI_GUI_ASYNC_SMOKE
static volatile uint32_t g_smoke_frames = 0u;
static uint32_t g_smoke_submit_frame = 0u;
static size_t g_smoke_pool_count = 0u;
static int g_smoke_active = 0;
#endif

/* ── local helpers (no libc) ───────────────────────────────────────────── */

static void chat_strlcpy(char *dst, const char *src, size_t size) {
    size_t i = 0u;
    if (!dst || size == 0u) return;
    for (; src && src[i] && i + 1u < size; ++i) dst[i] = src[i];
    dst[i] = '\0';
}

static void chat_zero(void *p, size_t n) {
    uint8_t *b = (uint8_t *)p;
    for (size_t i = 0u; i < n; ++i) b[i] = 0;
}

static void chat_fill(struct gui_surface *s, int32_t x, int32_t y,
                      int32_t w, int32_t h, uint32_t color) {
    if (!s || !s->pixels) return;
    for (int32_t row = 0; row < h; ++row) {
        int32_t py = y + row;
        uint32_t *line;
        if (py < 0 || (uint32_t)py >= s->height) continue;
        line = (uint32_t *)((uint8_t *)s->pixels + (uint32_t)py * s->pitch);
        for (int32_t col = 0; col < w; ++col) {
            int32_t px = x + col;
            if (px < 0 || (uint32_t)px >= s->width) continue;
            line[px] = color;
        }
    }
}

/* Blit the capybara system logo mask into the surface, downsampled by `step`. */
static void chat_draw_capy(struct gui_surface *s, int32_t x0, int32_t y0,
                           uint32_t step, uint32_t color) {
    uint32_t oy, ox;
    if (step == 0u) step = 1u;
    for (oy = 0u; oy < CAPYOS_ICON_H / step; ++oy) {
        for (ox = 0u; ox < CAPYOS_ICON_W / step; ++ox) {
            uint32_t mx = ox * step;
            uint32_t my = oy * step;
            uint8_t byte = capyos_icon_mask[my * CAPYOS_ICON_STRIDE + (mx >> 3)];
            if (byte & (uint8_t)(0x80u >> (mx & 7u))) {
                chat_fill(s, x0 + (int32_t)ox, y0 + (int32_t)oy, 1, 1, color);
            }
        }
    }
}

/* Button geometry, shared by paint + mouse so hit-testing matches drawing. */
static void chat_button_rect(struct gui_window *win, int idx,
                             int32_t *bx, int32_t *by, int32_t *bw, int32_t *bh) {
    int32_t w = (int32_t)win->frame.width;
    int32_t avail = w - CHAT_PAD * 4;
    int32_t bwid = avail / 3;
    *by = CHAT_BTN_TOP;
    *bh = CHAT_BTN_H;
    *bw = bwid;
    *bx = CHAT_PAD + idx * (bwid + CHAT_PAD);
}

/* ── rendering ─────────────────────────────────────────────────────────── */

static void capyai_chat_paint(struct gui_window *win) {
    struct capyai_chat_app *app = (struct capyai_chat_app *)win->user_data;
    struct gui_surface *s = &win->surface;
    const struct font *f = font_default();
    const struct gui_theme_palette *th = compositor_theme();
    int32_t w = (int32_t)win->frame.width;
    int32_t h = (int32_t)win->frame.height;
    int32_t conv_top, conv_bottom, y, visible, first, i, idx;
    uint32_t bg, fg, accent, muted, border, hdr_text;
    if (!app || !s || !s->pixels || !f || !th) return;

    bg = th->window_bg;
    fg = th->text;
    accent = th->accent;
    muted = th->text_muted;
    border = th->window_border;
    hdr_text = th->accent_text;

    /* background */
    chat_fill(s, 0, 0, w, h, bg);

    /* header bar + capybara logo + title */
    chat_fill(s, 0, 0, w, CHAT_HEADER_H, accent);
    chat_draw_capy(s, CHAT_PAD, 6, 3u, hdr_text); /* 96/3 = 32 px */
    font_draw_string(s, f, CHAT_PAD + 40, 8, "CapyAI", hdr_text);
    font_draw_string(s, f, CHAT_PAD + 40, 24, "assistente do sistema", hdr_text);

    /* permission buttons */
    {
        static const char *const labels[3] = {"Escrita", "Deletar", "Sistema"};
        int on[3];
        on[0] = app->allow_write;
        on[1] = app->allow_delete;
        on[2] = app->allow_system;
        for (idx = 0; idx < 3; ++idx) {
            int32_t bx, by, bw, bh;
            chat_button_rect(win, idx, &bx, &by, &bw, &bh);
            if (on[idx]) {
                chat_fill(s, bx, by, bw, bh, accent);
                font_draw_string(s, f, bx + 8, by + 6, labels[idx], hdr_text);
            } else {
                chat_fill(s, bx, by, bw, bh, bg);
                chat_fill(s, bx, by, bw, 1, border);
                chat_fill(s, bx, by + bh - 1, bw, 1, border);
                chat_fill(s, bx, by, 1, bh, border);
                chat_fill(s, bx + bw - 1, by, 1, bh, border);
                font_draw_string(s, f, bx + 8, by + 6, labels[idx], muted);
            }
        }
    }

    /* conversation area */
    conv_top = CHAT_BTN_TOP + CHAT_BTN_H + CHAT_PAD;
    conv_bottom = h - CHAT_INPUT_H - CHAT_PAD;
    visible = (conv_bottom - conv_top) / CHAT_LINE_H;
    if (visible < 1) visible = 1;
    first = app->msg_count > visible ? app->msg_count - visible : 0;
    y = conv_top;
    for (i = first; i < app->msg_count; ++i) {
        const struct capyai_chat_msg *m = &app->msgs[i];
        uint32_t col = (m->role == CAPYAI_CHAT_ROLE_USER) ? accent
                     : (m->role == CAPYAI_CHAT_ROLE_SYSTEM) ? muted : fg;
        const char *prefix = (m->role == CAPYAI_CHAT_ROLE_USER) ? "voce: "
                           : (m->role == CAPYAI_CHAT_ROLE_SYSTEM) ? "" : "capyai: ";
        font_draw_string(s, f, CHAT_PAD, y, prefix, muted);
        font_draw_string(s, f, CHAT_PAD + (int32_t)(6 * 8), y, m->text, col);
        y += CHAT_LINE_H;
    }

    /* input line */
    {
        int32_t iy = h - CHAT_INPUT_H - 2;
        chat_fill(s, CHAT_PAD, iy, w - CHAT_PAD * 2, CHAT_INPUT_H, bg);
        chat_fill(s, CHAT_PAD, iy, w - CHAT_PAD * 2, 1, border);
        font_draw_string(s, f, CHAT_PAD + 2, iy + 5, "> ", accent);
        font_draw_string(s, f, CHAT_PAD + 2 + 16, iy + 5,
                         app->input_len ? app->input : "(digite e Enter)",
                         app->input_len ? fg : muted);
    }
}

/* ── interaction ───────────────────────────────────────────────────────── */

static void chat_add_msg(struct capyai_chat_app *app, uint8_t role,
                         const char *text) {
    struct capyai_chat_msg *m;
    if (app->msg_count >= CAPYAI_CHAT_MAX_MSGS) {
        int i;
        for (i = 1; i < app->msg_count; ++i) app->msgs[i - 1] = app->msgs[i];
        app->msg_count--;
    }
    m = &app->msgs[app->msg_count++];
    m->role = role;
    chat_strlcpy(m->text, text, sizeof(m->text));
}

#ifdef CAPYOS_HAVE_CAPYAI
static void chat_capture_putc(char ch) {
    if (!g_capture_buffer || g_capture_capacity == 0u) return;
    if (g_capture_length + 1u >= g_capture_capacity) return;
    g_capture_buffer[g_capture_length++] = ch;
    g_capture_buffer[g_capture_length] = '\0';
}

static void chat_capture_write(const char *text) {
    size_t i;
    if (!text) return;
    for (i = 0u; text[i]; ++i) chat_capture_putc(text[i]);
}

static void chat_capture_clear(void) {
    g_capture_length = 0u;
    if (g_capture_buffer && g_capture_capacity > 0u) g_capture_buffer[0] = '\0';
}

static int chat_command_is_graphical(const char *line) {
    static const char *const names[] = {
        "desktop", "desktopstart", "desktop-start", "open-calculator",
        "open-files", "open-editor", "open-tasks", "open-capyai",
        "open-settings", "open-browser-graphical", "capyai", NULL,
    };
    size_t i;
    for (i = 0u; names[i]; ++i) {
        size_t j = 0u;
        while (line && line[j] && names[i][j] && line[j] == names[i][j]) ++j;
        if (!names[i][j] && (!line[j] || line[j] == ' ' || line[j] == '\t')) {
            return 1;
        }
    }
    return 0;
}

static int capyai_chat_dispatch(void *ctx, const char *command_line,
                                char *detail, size_t detail_size) {
    struct capyai_chat_dispatch_context *dispatch_ctx =
        (struct capyai_chat_dispatch_context *)ctx;
    char line[CAPYAI_COMMAND_MAX];
    int rc = 127;
    int status;
    if (!dispatch_ctx || !command_line || !detail || detail_size == 0u) {
        return 127;
    }
    /* The worker may use kernel services, but compositor/window mutations are
     * foreground-only. Reject every graphical shell entrypoint even if a
     * future model artifact emits one. */
    if (chat_command_is_graphical(command_line)) return 127;
#ifdef CAPYOS_CAPYAI_GUI_ASYNC_SMOKE
    /* Force the real worker to overlap at least three completed compositor
     * frames. A synchronous-on-pump regression deadlocks here and never emits
     * the ready marker, which is exactly what the QEMU gate must catch. */
    while (g_smoke_active &&
           (uint32_t)(g_smoke_frames - g_smoke_submit_frame) < 3u) {
        task_yield();
    }
#endif
    chat_strlcpy(line, command_line, sizeof(line));
    g_capture_buffer = detail;
    g_capture_capacity = detail_size;
    g_capture_length = 0u;
    detail[0] = '\0';
    status = kernel_desktop_dispatch_shell_command_scoped(
        &dispatch_ctx->shell, line, &rc, 1, chat_capture_write,
        chat_capture_putc, chat_capture_clear);
    g_capture_buffer = NULL;
    g_capture_capacity = 0u;
    g_capture_length = 0u;
    if (status != DESKTOP_SHELL_DISPATCH_HANDLED) return 127;
    return rc;
}

struct capyai_chat_typed_operation {
    struct shell_context *shell;
    const struct capy_ai_output *tool_call;
    char *detail;
    size_t detail_size;
};

static int chat_action_equal(const char *value, const char *literal) {
    size_t i = 0u;
    if (!value || !literal) return 0;
    while (value[i] && literal[i] && value[i] == literal[i]) ++i;
    return value[i] == '\0' && literal[i] == '\0';
}

static int capyai_chat_run_typed_operation(void *opaque) {
    struct capyai_chat_typed_operation *operation =
        (struct capyai_chat_typed_operation *)opaque;
    if (!operation || !operation->shell || !operation->tool_call) return 127;
    if (chat_action_equal(operation->tool_call->action, "file_create") ||
        chat_action_equal(operation->tool_call->action, "file_edit_text") ||
        chat_action_equal(operation->tool_call->action, "file_move")) {
        return capyai_native_file_dispatch(operation->shell,
                                           operation->tool_call,
                                           operation->detail,
                                           operation->detail_size);
    }
    return capyai_native_system_dispatch(operation->shell,
                                         operation->tool_call,
                                         operation->detail,
                                         operation->detail_size);
}

static int capyai_chat_typed_dispatch(void *ctx,
                                      const struct capy_ai_output *tool_call,
                                      char *detail, size_t detail_size) {
    struct capyai_chat_dispatch_context *dispatch_ctx =
        (struct capyai_chat_dispatch_context *)ctx;
    struct capyai_chat_typed_operation operation;
    int rc = 127;
    int status;
    if (!dispatch_ctx || !tool_call || !detail || detail_size == 0u) return 127;
    operation.shell = &dispatch_ctx->shell;
    operation.tool_call = tool_call;
    operation.detail = detail;
    operation.detail_size = detail_size;
    detail[0] = '\0';
    status = kernel_desktop_run_session_operation_scoped(
        &dispatch_ctx->shell, capyai_chat_run_typed_operation, &operation,
        1, &rc);
    return status == DESKTOP_SHELL_DISPATCH_HANDLED ? rc : 127;
}
#endif

static void chat_submit(struct capyai_chat_app *app) {
    if (!app || app->input_len == 0) return;
    chat_add_msg(app, CAPYAI_CHAT_ROLE_USER, app->input);
#ifdef CAPYOS_HAVE_CAPYAI
    if (app->pending || capyai_async_busy()) {
        chat_add_msg(app, CAPYAI_CHAT_ROLE_SYSTEM,
                     "Aguarde o pedido anterior terminar.");
    } else {
        struct capyai_async_request_v2 request;
        int submit_rc;
        chat_zero(&request, sizeof(request));
        if (!g_async_ready && capyai_chat_runtime_init() != 0) {
            chat_add_msg(app, CAPYAI_CHAT_ROLE_SYSTEM,
                         "Servico CapyAI indisponivel nesta sessao.");
        } else if ((chat_zero(&g_dispatch_context,
                              sizeof(g_dispatch_context)),
                    kernel_desktop_shell_snapshot(
                       &g_dispatch_context.shell,
                       &g_dispatch_context.session)) != 0) {
            chat_add_msg(app, CAPYAI_CHAT_ROLE_SYSTEM,
                         "Sessao do terminal indisponivel.");
        } else {
            request.abi_version = CAPYAI_ASYNC_REQUEST_ABI_V2;
            request.struct_size = (uint32_t)sizeof(request);
            chat_strlcpy(request.base.intent, app->input,
                         sizeof(request.base.intent));
            request.base.perms.allow_write = app->allow_write;
            request.base.perms.allow_delete = app->allow_delete;
            request.base.perms.allow_system_change = app->allow_system;
            chat_strlcpy(request.base.session.last_file, app->last_file,
                         sizeof(request.base.session.last_file));
            request.base.session.turns = app->turns;
            request.base.dispatch = capyai_chat_dispatch;
            request.base.dispatch_ctx = &g_dispatch_context;
            request.typed_dispatch = capyai_chat_typed_dispatch;
            request.typed_dispatch_ctx = &g_dispatch_context;
            request.base.client_generation = app->generation;
            submit_rc = capyai_async_submit_v2(&request, &app->job_id);
            if (submit_rc == CAPYAI_ASYNC_OK) {
                g_dispatch_context_in_use = 1;
                app->pending = 1;
                chat_strlcpy(app->pending_intent, app->input,
                             sizeof(app->pending_intent));
                /* Grants are task-scoped. The request owns the copied values;
                 * the next task starts fail-closed and must be authorized
                 * explicitly again by the user. */
                app->allow_write = 0;
                app->allow_delete = 0;
                app->allow_system = 0;
                chat_add_msg(app, CAPYAI_CHAT_ROLE_SYSTEM, "Processando...");
            } else if (submit_rc == CAPYAI_ASYNC_ERR_BUSY) {
                chat_add_msg(app, CAPYAI_CHAT_ROLE_SYSTEM,
                             "Aguarde o pedido anterior terminar.");
            } else {
                app->job_id = 0u;
                chat_add_msg(app, CAPYAI_CHAT_ROLE_SYSTEM,
                             "Falha ao iniciar o worker CapyAI.");
            }
            if (submit_rc != CAPYAI_ASYNC_OK) {
                chat_zero(&g_dispatch_context, sizeof(g_dispatch_context));
            }
        }
    }
#else
    chat_add_msg(app, CAPYAI_CHAT_ROLE_SYSTEM,
                 "IA indisponivel nesta build (compile com ../CapyAI).");
#endif
    app->input[0] = '\0';
    app->input_len = 0;
}

int capyai_chat_runtime_init(void) {
#ifdef CAPYOS_HAVE_CAPYAI
    if (g_async_ready) return 0;
    if (capyai_async_init() != CAPYAI_ASYNC_OK) return -1;
    g_async_ready = 1;
    return 0;
#else
    return -1;
#endif
}

int capyai_chat_busy(void) {
#ifdef CAPYOS_HAVE_CAPYAI
    return capyai_async_busy();
#else
    return 0;
#endif
}

#ifdef CAPYOS_CAPYAI_GUI_ASYNC_SMOKE
static void capyai_chat_smoke_finish(int passed) {
    com1_puts(passed ? "[smoke] capyai-gui-async ready\n"
                     : "[smoke] capyai-gui-async FAIL\n");
    g_smoke_active = 0;
    desktop_stop();
}

void capyai_chat_smoke_note_frame(void) {
    if (g_smoke_active && g_smoke_frames != UINT32_MAX) g_smoke_frames++;
}

int capyai_chat_smoke_start(void) {
    static const char intent[] = "listar arquivos";
    size_t i;
    if (capyai_chat_runtime_init() != 0 || capyai_chat_open() != 0) {
        capyai_chat_smoke_finish(0);
        return -1;
    }
    g_smoke_frames = 0u;
    g_smoke_submit_frame = 0u;
    g_smoke_pool_count = worker_pool_count();
    g_smoke_active = 1;
    for (i = 0u; intent[i] && i + 1u < sizeof(g_chat.input); ++i) {
        g_chat.input[i] = intent[i];
    }
    g_chat.input[i] = '\0';
    g_chat.input_len = (int)i;
    chat_submit(&g_chat);
    g_smoke_submit_frame = g_smoke_frames;
    if (!g_chat.pending || g_chat.job_id == 0u) {
        capyai_chat_smoke_finish(0);
        return -1;
    }
    com1_puts("[smoke] capyai-gui-async begin\n");
    return 0;
}
#endif

void capyai_chat_pump(void) {
#ifdef CAPYOS_HAVE_CAPYAI
    struct capyai_async_response response;
    uint64_t job_id;
    int poll_rc;
    if (!g_chat.pending || g_chat.job_id == 0u) {
        if (g_dispatch_context_in_use && !capyai_async_busy()) {
            chat_zero(&g_dispatch_context, sizeof(g_dispatch_context));
            g_dispatch_context_in_use = 0;
        }
        return;
    }
    job_id = g_chat.job_id;
    chat_zero(&response, sizeof(response));
    poll_rc = capyai_async_poll(job_id, &response);
    if (poll_rc == 0) return;

    chat_zero(&g_dispatch_context, sizeof(g_dispatch_context));
    g_dispatch_context_in_use = 0;

    g_chat.pending = 0;
    g_chat.job_id = 0u;
    g_chat.pending_intent[0] = '\0';
    if (poll_rc < 0) {
        if (g_chat_open && g_chat.window) {
            chat_add_msg(&g_chat, CAPYAI_CHAT_ROLE_SYSTEM,
                         "Resposta assincrona descartada com seguranca.");
            compositor_invalidate(g_chat.window->id);
        }
        return;
    }
    if (!g_chat_open || !g_chat.window ||
        response.client_generation != g_chat.generation) {
        return;
    }
    chat_strlcpy(g_chat.last_file, response.session.last_file,
                 sizeof(g_chat.last_file));
    g_chat.turns = response.session.turns;
    chat_add_msg(&g_chat,
                 response.status == CAPYAI_ASYNC_DONE
                     ? CAPYAI_CHAT_ROLE_ASSISTANT
                     : CAPYAI_CHAT_ROLE_SYSTEM,
                 response.summary);
    compositor_invalidate(g_chat.window->id);
#ifdef CAPYOS_CAPYAI_GUI_ASYNC_SMOKE
    if (g_smoke_active) {
        int passed = response.status == CAPYAI_ASYNC_DONE &&
                     (uint32_t)(g_smoke_frames - g_smoke_submit_frame) >= 3u;
        uint64_t previous_generation = g_chat.generation;
        int round;
        for (round = 0; passed && round < 4; ++round) {
            uint32_t window_id = g_chat.window ? g_chat.window->id : 0u;
            if (window_id == 0u) {
                passed = 0;
                break;
            }
            compositor_destroy_window(window_id);
            if (g_chat.window != NULL || capyai_chat_open() != 0 ||
                g_chat.generation <= previous_generation) {
                passed = 0;
                break;
            }
            previous_generation = g_chat.generation;
        }
        if (worker_pool_count() != g_smoke_pool_count) passed = 0;
        capyai_chat_smoke_finish(passed);
    }
#endif
#endif
}

static void capyai_chat_window_key(struct gui_window *win, uint32_t keycode,
                                   uint8_t mods) {
    struct capyai_chat_app *app = (struct capyai_chat_app *)win->user_data;
    char ch;
    (void)mods;
    if (!app) return;
    ch = (keycode < 0x80u) ? (char)keycode : 0;
    if (ch == '\n' || ch == '\r') {
        chat_submit(app);
    } else if (ch == 0x08 || ch == 0x7F) { /* backspace / delete */
        if (app->input_len > 0) app->input[--app->input_len] = '\0';
    } else if (ch >= 32 && ch < 127) {
        if (app->input_len + 1 < CAPYAI_CHAT_INPUT_MAX) {
            app->input[app->input_len++] = ch;
            app->input[app->input_len] = '\0';
        }
    } else {
        return;
    }
    compositor_invalidate(win->id);
}

static void capyai_chat_window_mouse(struct gui_window *win, int32_t x,
                                     int32_t y, uint8_t btns) {
    struct capyai_chat_app *app = (struct capyai_chat_app *)win->user_data;
    int idx;
    if (!app) return;
    if (!(btns & 1u)) return; /* left button only */
    for (idx = 0; idx < 3; ++idx) {
        int32_t bx, by, bw, bh;
        chat_button_rect(win, idx, &bx, &by, &bw, &bh);
        if (x >= bx && x < bx + bw && y >= by && y < by + bh) {
            if (idx == 0) app->allow_write = !app->allow_write;
            else if (idx == 1) app->allow_delete = !app->allow_delete;
            else app->allow_system = !app->allow_system;
            compositor_invalidate(win->id);
            return;
        }
    }
}

static void capyai_chat_on_close(struct gui_window *win) {
    (void)win;
#ifdef CAPYOS_HAVE_CAPYAI
    if (g_chat.job_id != 0u) {
        (void)capyai_async_detach(g_chat.job_id);
    }
#endif
    g_chat.window = NULL;
    g_chat_open = 0;
    g_chat.pending = 0;
    g_chat.job_id = 0u;
    g_chat.generation = 0u;
}

int capyai_chat_open(void) {
    uint8_t scale = compositor_ui_scale();
    if (g_chat_open && g_chat.window) {
        compositor_show_window(g_chat.window->id);
        compositor_focus_window(g_chat.window->id);
        return 0;
    }
    chat_zero(&g_chat, sizeof(g_chat));
    g_chat_generation++;
    if (g_chat_generation == 0u) g_chat_generation++;
    g_chat.generation = g_chat_generation;
    g_chat.window = compositor_create_window("CapyAI", 140, 90,
                                             520u + 120u * (scale - 1u),
                                             400u + 100u * (scale - 1u));
    if (!g_chat.window) return -1;
    g_chat.window->user_data = &g_chat;
    g_chat.window->on_paint = capyai_chat_paint;
    g_chat.window->on_key = capyai_chat_window_key;
    g_chat.window->on_mouse = capyai_chat_window_mouse;
    g_chat.window->on_close = capyai_chat_on_close;
    chat_add_msg(&g_chat, CAPYAI_CHAT_ROLE_SYSTEM,
                 "Ola! Escreva um pedido e tecle Enter.");
    chat_add_msg(&g_chat, CAPYAI_CHAT_ROLE_SYSTEM,
                 "Permissoes valem somente para o proximo pedido.");
    if (capyai_chat_runtime_init() != 0) {
        chat_add_msg(&g_chat, CAPYAI_CHAT_ROLE_SYSTEM,
                     "Worker assincrono indisponivel nesta build.");
    }
    compositor_show_window(g_chat.window->id);
    compositor_focus_window(g_chat.window->id);
    g_chat_open = 1;
    return 0;
}
