#include "gui/desktop_runtime.h"
#include "gui/desktop.h"
#include "apps/capyai_chat.h"
#include "arch/x86_64/framebuffer_console.h"
#include "drivers/input/mouse.h"
#include "drivers/input/keyboard_layout.h"
#include "drivers/timer/pit.h"
#include "drivers/acpi/acpi.h"
#include "fs/buffer.h"
#include "fs/vfs.h"
#include "auth/session.h"
#include "arch/x86_64/kernel_shell_dispatch.h"
#include "arch/x86_64/kernel_runtime_control.h"
#include "kernel/scheduler.h"
#include "kernel/task.h"
#include "net/stack.h"
#include "services/capyai_system_actions.h"
#include <stddef.h>

static struct task *g_desktop_task = (struct task *)0;

static void desktop_net_yield(void) {
  struct mouse_state ms;
  /* Network operations may run on the CapyAI worker. Only the foreground
   * desktop task is allowed to touch the compositor; workers yield back and
   * let the next normal frame refresh the cursor. */
  if (task_current() != g_desktop_task) {
    task_yield();
    return;
  }
  mouse_get_state(&ms);
  compositor_render_cursor(ms.x, ms.y);
}

static void desktop_present_initial_frame(struct desktop_session *ds) {
  struct mouse_state ms;
  if (!ds) return;
  compositor_invalidate_all();
  compositor_render();
  mouse_get_state(&ms);
  compositor_render_cursor(ms.x, ms.y);
  ds->cursor_valid = 1;
  ds->cursor_x = ms.x;
  ds->cursor_y = ms.y;
}

static struct desktop_session g_desktop;
static int g_desktop_active = 0;
static struct shell_context *g_desktop_shell_ctx = NULL;
static volatile int g_desktop_shell_dispatch_busy = 0;
static int g_desktop_stop_pending = 0;
static int g_reboot_requested = 0;
static int g_shutdown_requested = 0;
static int g_capyai_launch_pending = 0;
static int g_desktop_gui_session_smoke_announced = 0;
static int g_desktop_mouse_events_smoke_announced = 0;
/* Blocked-state diagnostic emitted at most once per gate, only after the
 * boot has stabilized (DESKTOP_SMOKE_DIAG_DELAY_ITERATIONS main-loop
 * iterations). Gives the external smoke operator a serial breadcrumb
 * pointing at the first persistent blocker without spamming the log. */
#define DESKTOP_SMOKE_DIAG_DELAY_ITERATIONS 1000u
static int g_desktop_gui_session_smoke_diag_emitted = 0;
static int g_desktop_mouse_events_smoke_diag_emitted = 0;
static uint32_t g_desktop_smoke_diag_iter_count = 0;

int desktop_is_active(void) { return g_desktop_active; }

int kernel_desktop_dispatch_shell_command_scoped(
    struct shell_context *ctx, char *line, int *out_rc, int wait_if_busy,
    shell_output_write_fn write_cb, shell_output_putc_fn putc_cb,
    shell_output_clear_fn clear_cb) {
  struct session_context *previous_session = NULL;
  struct session_context *desktop_session = NULL;
  int handled = 0;
  if (!line) return DESKTOP_SHELL_DISPATCH_UNKNOWN;
  if (!ctx) ctx = g_desktop_shell_ctx;
  if (!ctx) return DESKTOP_SHELL_DISPATCH_UNKNOWN;
  desktop_session = shell_context_session(ctx);
  if (!desktop_session) return DESKTOP_SHELL_DISPATCH_UNKNOWN;

  while (__sync_lock_test_and_set(&g_desktop_shell_dispatch_busy, 1)) {
    if (!wait_if_busy) return DESKTOP_SHELL_DISPATCH_BUSY;
    task_yield();
  }

  previous_session = session_active();
  session_set_active(desktop_session);
  shell_set_output_callbacks(write_cb, putc_cb);
  shell_set_clear_callback(clear_cb);
  handled = x64_kernel_try_shell_command_result(ctx, 1, line, out_rc);
  shell_set_output_callbacks(NULL, NULL);
  shell_set_clear_callback(NULL);
  session_set_active(previous_session);
  __sync_lock_release(&g_desktop_shell_dispatch_busy);
  return handled ? DESKTOP_SHELL_DISPATCH_HANDLED
                 : DESKTOP_SHELL_DISPATCH_UNKNOWN;
}

int kernel_desktop_run_session_operation_scoped(
    struct shell_context *ctx, desktop_session_operation_fn operation,
    void *operation_ctx, int wait_if_busy, int *out_rc) {
  struct session_context *previous_session = NULL;
  struct session_context *operation_session = NULL;
  int rc;
  if (!operation) return DESKTOP_SHELL_DISPATCH_UNKNOWN;
  if (!ctx) ctx = g_desktop_shell_ctx;
  if (!ctx) return DESKTOP_SHELL_DISPATCH_UNKNOWN;
  operation_session = shell_context_session(ctx);
  if (!operation_session) return DESKTOP_SHELL_DISPATCH_UNKNOWN;

  while (__sync_lock_test_and_set(&g_desktop_shell_dispatch_busy, 1)) {
    if (!wait_if_busy) return DESKTOP_SHELL_DISPATCH_BUSY;
    task_yield();
  }
  previous_session = session_active();
  session_set_active(operation_session);
  rc = operation(operation_ctx);
  session_set_active(previous_session);
  __sync_lock_release(&g_desktop_shell_dispatch_busy);
  if (out_rc) *out_rc = rc;
  return DESKTOP_SHELL_DISPATCH_HANDLED;
}

int kernel_desktop_dispatch_shell_command_result(char *line, int *out_rc) {
  return kernel_desktop_dispatch_shell_command_scoped(
      NULL, line, out_rc, 0, NULL, NULL, NULL) ==
         DESKTOP_SHELL_DISPATCH_HANDLED;
}

int kernel_desktop_dispatch_shell_command(char *line) {
  return kernel_desktop_dispatch_shell_command_result(line, (int *)0);
}

int desktop_launch_capyai(void) {
  struct gui_window *win;
  if (!g_desktop_active) {
    g_capyai_launch_pending = 1;
    return 0;
  }
  if (capyai_chat_open() != 0) {
    if (g_desktop.active_terminal) {
      terminal_write_string(g_desktop.active_terminal,
                            "\n[capyai] falha ao alocar a janela; "
                            "memoria grafica insuficiente.\n");
      compositor_invalidate(g_desktop.active_terminal->window->id);
    }
    return -1;
  }
  win = compositor_focused_window();
  if (win && win->id) {
    taskbar_add_window(&g_desktop.taskbar, win->id, "CapyAI");
    taskbar_set_focused(&g_desktop.taskbar, win->id);
  }
  return 0;
}

struct session_context *kernel_desktop_shell_session(void) {
  if (!g_desktop_shell_ctx) return NULL;
  return shell_context_session(g_desktop_shell_ctx);
}

int kernel_desktop_shell_snapshot(struct shell_context *out_ctx,
                                  struct session_context *out_session) {
  struct session_context *source;
  if (!out_ctx || !out_session || !g_desktop_shell_ctx) return -1;
  source = shell_context_session(g_desktop_shell_ctx);
  if (!source) return -1;
  *out_session = *source;
  /* The worker only needs the authorization principal, cwd and preferences.
   * Never retain password-verifier material in the service-owned snapshot. */
  for (size_t i = 0u; i < sizeof(out_session->user.salt); ++i)
    out_session->user.salt[i] = 0u;
  for (size_t i = 0u; i < sizeof(out_session->user.hash); ++i)
    out_session->user.hash[i] = 0u;
  out_session->user.algo_id = 0u;
  out_session->user.algo_t_cost = 0u;
  out_session->user.algo_m_cost = 0u;
  shell_context_init(out_ctx, out_session,
                     shell_context_settings(g_desktop_shell_ctx));
  return 0;
}

int kernel_desktop_shell_should_logout(void) {
  if (!g_desktop_shell_ctx) return 0;
  return shell_context_should_logout(g_desktop_shell_ctx);
}

int kernel_desktop_shell_should_stop(void) {
  if (!g_desktop_shell_ctx) return 0;
  return !shell_context_running(g_desktop_shell_ctx) ||
         shell_context_should_logout(g_desktop_shell_ctx);
}

void desktop_stop(void) {
  if (g_desktop_active) {
    if (capyai_chat_busy()) {
      g_desktop_stop_pending = 1;
    } else {
      g_desktop_active = 0;
    }
  }
}

int desktop_request_logout(void) {
  /* desktop_stop() alone returns control to the command dispatcher with the
   * authenticated shell still alive, exposing the text shell. Marking the
   * owning shell context for logout makes login_runtime reset the session and
   * render the login screen as soon as desktop_runtime_start() returns. */
  if (!g_desktop_active || !g_desktop_shell_ctx) return -1;
  shell_request_logout(g_desktop_shell_ctx);
  desktop_stop();
  return 0;
}

int desktop_open_terminal_window(void) {
  if (!g_desktop_active) return -1;
  desktop_open_terminal(&g_desktop);
  return compositor_find_window_by_title("Terminal") ? 0 : -1;
}

static void sync_and_flush_desktop(void) {
  struct super_block *root = vfs_root();
  if (root && root->bdev)
    buffer_cache_sync(root->bdev);
}

void kernel_request_reboot(void) {
  g_reboot_requested = 1;
}

void kernel_request_shutdown(void) {
  g_shutdown_requested = 1;
}

static inline void desktop_idle_wait_cpu(void) {
  uint64_t start = pit_ticks();
  uint32_t spins = 0;
  while (pit_ticks() == start && spins++ < 1024u) {
    if (mouse_pending()) break;
    __asm__ volatile("pause");
  }
}

static inline void desktop_frame_delay(void) {
  if (scheduler_can_sleep_current()) {
    task_sleep(1);
    return;
  }
  if (!mouse_pending()) desktop_idle_wait_cpu();
}

static void desktop_scheduler_entry(void *arg) {
  (void)arg;
  for (;;) task_yield();
}

static int desktop_scheduler_adopt_current(void) {
  if (task_current()) {
    g_desktop_task = task_current();
    scheduler_set_running(1);
    return 0;
  }
  if (!g_desktop_task) {
    g_desktop_task = task_create_kernel("desktop-main",
                                        desktop_scheduler_entry,
                                        (void *)0);
    if (!g_desktop_task) return -1;
    scheduler_add(g_desktop_task);
  }
  g_desktop_task->state = TASK_STATE_RUNNING;
  task_set_current(g_desktop_task);
  scheduler_set_running(1);
  return 0;
}

static void desktop_gui_session_smoke_emit_once(void) {
  struct desktop_session_smoke_readiness readiness;
  struct desktop_gui_session_smoke_gate gate;
  if (g_desktop_gui_session_smoke_announced) return;
  if (desktop_session_smoke_readiness_snapshot(&g_desktop, &readiness) != 1) {
    return;
  }
  if (desktop_gui_session_smoke_gate_from_readiness(&readiness, &gate) != 1) {
    return;
  }
  if (!gate.smoke_ready) return;
  fbcon_print(desktop_gui_session_smoke_marker());
  fbcon_print("\n");
  g_desktop_gui_session_smoke_announced = 1;
}

static void desktop_mouse_events_smoke_emit_once(void) {
  struct desktop_session_smoke_readiness readiness;
  struct desktop_mouse_events_smoke_gate gate;
  if (g_desktop_mouse_events_smoke_announced) return;
  if (desktop_session_smoke_readiness_snapshot(&g_desktop, &readiness) != 1) {
    return;
  }
  if (desktop_mouse_events_smoke_gate_from_readiness(&readiness, &gate) != 1) {
    return;
  }
  if (!gate.smoke_ready) return;
  fbcon_print(desktop_mouse_events_smoke_marker());
  fbcon_print("\n");
  g_desktop_mouse_events_smoke_announced = 1;
}

/*
 * Blocked-state diagnostics for external smoke operators.
 *
 * Each gate has a matching "blocked diagnostic" emitter that runs on the
 * main loop alongside the ready marker emitter. When the boot has
 * passed DESKTOP_SMOKE_DIAG_DELAY_ITERATIONS iterations and the gate
 * is still not ready, we emit exactly ONE line of the form:
 *
 *   [smoke-diag] gui-session blocked: <first_blocker_name>
 *   [smoke-diag] mouse-events blocked: <first_blocker_name>
 *
 * The `[smoke-diag]` prefix is deliberately distinct from `[smoke]` so
 * the official smoke parser's substring matching on the mandatory
 * markers cannot collide. This is observability only and does not
 * change any gate logic or change which markers are mandatory.
 *
 * Bounded total messages: at most 2 (one per gate) over the entire
 * desktop runtime lifetime, so the serial log is never flooded.
 */
static void desktop_gui_session_smoke_emit_blocked_diagnostic_once(void) {
  struct desktop_session_smoke_readiness readiness;
  struct desktop_gui_session_smoke_gate gate;
  if (g_desktop_gui_session_smoke_announced) return;
  if (g_desktop_gui_session_smoke_diag_emitted) return;
  if (g_desktop_smoke_diag_iter_count < DESKTOP_SMOKE_DIAG_DELAY_ITERATIONS) return;
  if (desktop_session_smoke_readiness_snapshot(&g_desktop, &readiness) != 1) return;
  if (desktop_gui_session_smoke_gate_from_readiness(&readiness, &gate) != 1) return;
  if (gate.smoke_ready) return;
  fbcon_print("[smoke-diag] gui-session blocked: ");
  fbcon_print(gate.first_blocker_name ? gate.first_blocker_name : "unknown");
  fbcon_print("\n");
  g_desktop_gui_session_smoke_diag_emitted = 1;
}

static void desktop_mouse_events_smoke_emit_blocked_diagnostic_once(void) {
  struct desktop_session_smoke_readiness readiness;
  struct desktop_mouse_events_smoke_gate gate;
  if (g_desktop_mouse_events_smoke_announced) return;
  if (g_desktop_mouse_events_smoke_diag_emitted) return;
  if (g_desktop_smoke_diag_iter_count < DESKTOP_SMOKE_DIAG_DELAY_ITERATIONS) return;
  if (desktop_session_smoke_readiness_snapshot(&g_desktop, &readiness) != 1) return;
  if (desktop_mouse_events_smoke_gate_from_readiness(&readiness, &gate) != 1) return;
  if (gate.smoke_ready) return;
  fbcon_print("[smoke-diag] mouse-events blocked: ");
  fbcon_print(gate.first_blocker_name ? gate.first_blocker_name : "unknown");
  fbcon_print("\n");
  g_desktop_mouse_events_smoke_diag_emitted = 1;
}

int desktop_runtime_start(struct shell_context *ctx) {
  struct session_context *previous_session = session_active();
  if (g_desktop_active) { fbcon_print("Desktop already running.\n"); return 0; }
  uint32_t *fb = kernel_desktop_get_fb();
  uint32_t w = kernel_desktop_get_width();
  uint32_t h = kernel_desktop_get_height();
  uint32_t pitch = kernel_desktop_get_pitch();
  if (!fb || w == 0 || h == 0) { fbcon_print("Error: no framebuffer.\n"); return -1; }

  mouse_ps2_init();
  g_desktop_gui_session_smoke_announced = 0;
  g_desktop_mouse_events_smoke_announced = 0;
  g_desktop_gui_session_smoke_diag_emitted = 0;
  g_desktop_mouse_events_smoke_diag_emitted = 0;
  g_desktop_smoke_diag_iter_count = 0;
  g_desktop_stop_pending = 0;
  g_desktop_shell_ctx = ctx;
  if (ctx && shell_context_session(ctx)) {
    session_set_active(shell_context_session(ctx));
  }
  desktop_init(&g_desktop, fb, w, h, pitch, ctx ? ctx->settings : NULL);
  desktop_open_terminal(&g_desktop);
  if (desktop_scheduler_adopt_current() != 0) {
    fbcon_print("Error: desktop scheduler unavailable.\n");
    desktop_shutdown(&g_desktop);
    g_desktop_shell_ctx = NULL;
    session_set_active(previous_session);
    return -1;
  }
  if (ctx && shell_context_session(ctx)) {
    session_set_active(shell_context_session(ctx));
  }
  (void)capyai_chat_runtime_init();
  net_stack_set_yield_hook(desktop_net_yield);
  scheduler_preempt_disable();
  desktop_present_initial_frame(&g_desktop);
  scheduler_preempt_enable();
  fbcon_set_visual_muted(1);
  fbcon_print("[desktop] session started\n");
  g_desktop_active = 1;
  if (g_capyai_launch_pending) {
    g_capyai_launch_pending = 0;
    (void)desktop_launch_capyai();
  }
#ifdef CAPYOS_CAPYAI_GUI_ASYNC_SMOKE
  (void)capyai_chat_smoke_start();
#endif

  /* Small state machine to distinguish a bare ESC press (exit desktop)
   * from a VT100 arrow-key escape sequence (ESC [ A/B/C/D).
   * escape_state: 0 = idle, 1 = saw ESC, 2 = saw ESC+[ */
  int escape_state = 0;

  while (g_desktop_active) {
    char ch = 0;
    int had_activity = 0;

    /* Etapa 7 / Slice 7.5 (alpha.309/CapyUI 2.23.1): cada frame do desktop
     * e uma unidade atomica de escalonamento. Com o scheduler preemptivo
     * default (policy PRIORITY, APIC 100Hz) e um processo grafico ring-3
     * vivo (capygfx via kernel_spawn_capygfx_desktop), o tick podia
     * preemptar esta task NO MEIO de desktop_run_frame/compositor_render;
     * o capygfx entao mutava a tabela de janelas/surfaces/damage por
     * syscall (ou o reaper de zumbis do tick destruia janelas em contexto
     * de IRQ) e, ao retomar a varredura, o compositor lia estado
     * inconsistente -- o travamento de campo do VMware ("tela toda azul":
     * so o wallpaper pintado, frame nunca completa). O guard adia APENAS
     * preempcao por quantum + reaping durante o corpo do frame; os pontos
     * voluntarios (task_yield/task_sleep logo abaixo) continuam sendo os
     * unicos lugares onde o capygfx roda -- sempre entre frames, com o
     * compositor quiescente. IRQs continuam habilitados (input nao perde
     * eventos; so a troca de contexto e adiada). */
    scheduler_preempt_disable();

    x64_kernel_runtime_poll_background();

    while (kernel_input_trygetc(&ch)) {
      had_activity = 1;
      if (escape_state == 2) {
        escape_state = 0;
        switch (ch) {
          case 'A': desktop_handle_input(&g_desktop, KEY_UP, 0); break;
          case 'B': desktop_handle_input(&g_desktop, KEY_DOWN, 0); break;
          case 'C': desktop_handle_input(&g_desktop, KEY_RIGHT, 0); break;
          case 'D': desktop_handle_input(&g_desktop, KEY_LEFT, 0); break;
          default: break;
        }
        continue;
      }
      if (escape_state == 1) {
        escape_state = 0;
        if (ch == '[') { escape_state = 2; continue; }
        /* Non-sequence char after ESC: dispatch as normal input */
        desktop_handle_input(&g_desktop, (uint32_t)(uint8_t)ch, ch);
        continue;
      }
      if ((uint8_t)ch == KEY_SUPER) {
        desktop_handle_input(&g_desktop, KEY_SUPER, 0);
        continue;
      }
      if ((uint8_t)ch == KEY_TTY_FALLBACK) {
        fbcon_print("[desktop] CTRL+ALT+F1 fallback requested\n");
        desktop_stop();
        break;
      }
      if (ch == 0x1B) { escape_state = 1; continue; }
      desktop_handle_input(&g_desktop, (uint32_t)(uint8_t)ch, ch);
    }
    if (escape_state == 1) {
      /* Give arrow-key escape sequences a brief window to arrive.
       * PS/2 scan codes and serial VT100 sequences may split the
       * ESC [ <letter> triplet across poll cycles. */
      int esc_resolved = 0;
      for (int retry = 0; retry < 64 && !esc_resolved; retry++) {
        if (kernel_input_trygetc(&ch)) {
          if (ch == '[') { escape_state = 2; esc_resolved = 1; }
          else { escape_state = 0; esc_resolved = 1; }
        }
        if (!esc_resolved) __asm__ volatile("pause");
      }
      if (!esc_resolved) {
        /* Bare ESC with no follow-up: ignore it (user can close
         * the desktop via the terminal 'exit' command). */
        escape_state = 0;
      }
    }
    if (g_desktop_active) {
      /* App open/close requested by a CapyAI worker is executed only here,
       * on the desktop task and inside the compositor's frame guard. This
       * keeps workers away from mutable window state while still allowing
       * them to wait for a bounded, observable result. */
      if (capyai_system_actions_pump() > 0) had_activity = 1;
      if (desktop_run_frame(&g_desktop)) had_activity = 1;
#ifdef CAPYOS_CAPYAI_GUI_ASYNC_SMOKE
      capyai_chat_smoke_note_frame();
#endif
      if (g_desktop_smoke_diag_iter_count < DESKTOP_SMOKE_DIAG_DELAY_ITERATIONS) {
        g_desktop_smoke_diag_iter_count++;
      }
      desktop_gui_session_smoke_emit_once();
      desktop_mouse_events_smoke_emit_once();
      desktop_gui_session_smoke_emit_blocked_diagnostic_once();
      desktop_mouse_events_smoke_emit_blocked_diagnostic_once();
    }
    /* Fim da secao atomica do frame: daqui ate o topo do proximo frame o
     * scheduler volta a poder preemptar/reapear -- task_yield e o ponto
     * canonico onde o capygfx (e qualquer outra task) roda. */
    scheduler_preempt_enable();
    /* CapyAI submission happens inside on_key, but execution is deliberately
     * deferred until after the frame guard is released. */
    capyai_chat_pump();
    if (g_desktop_stop_pending && !capyai_chat_busy()) {
      g_desktop_stop_pending = 0;
      g_desktop_active = 0;
    }
    if (!g_desktop_active) break;
    task_yield();
    if (!had_activity && !mouse_pending()) desktop_frame_delay();
  }

  desktop_shutdown(&g_desktop);
  net_stack_set_yield_hook((void *)0);
  fbcon_set_visual_muted(0);
  fbcon_print("[desktop] session stopped\n");
  g_desktop_active = 0;
  g_desktop_stop_pending = 0;
  g_desktop_shell_ctx = NULL;
  session_set_active(previous_session);

  fbcon_clear_view();

  if (g_reboot_requested) {
    g_reboot_requested = 0;
    sync_and_flush_desktop();
    fbcon_print("Rebooting...\n");
    acpi_reboot();
  }
  if (g_shutdown_requested) {
    g_shutdown_requested = 0;
    sync_and_flush_desktop();
    fbcon_print("Shutting down...\n");
    acpi_shutdown();
  }

  return 0;
}

#ifdef CAPYOS_CAPYAI_GUI_ASYNC_SMOKE
int desktop_capyai_gui_async_smoke_run(void) {
  struct session_context session;
  struct shell_context shell;
  session_reset(&session);
  shell_context_init(&shell, &session, NULL);
  return desktop_runtime_start(&shell);
}
#endif
