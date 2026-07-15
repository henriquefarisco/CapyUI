#!/usr/bin/env python3
"""Static cross-repo wiring contract for graphical desktop logout."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def function_body(source: str, signature: str) -> str:
    start = source.find(signature)
    if start < 0:
        raise AssertionError(f"missing function: {signature}")
    opening = source.find("{", start)
    if opening < 0:
        raise AssertionError(f"missing function body: {signature}")
    depth = 0
    for index in range(opening, len(source)):
        if source[index] == "{":
            depth += 1
        elif source[index] == "}":
            depth -= 1
            if depth == 0:
                return source[opening + 1 : index]
    raise AssertionError(f"unterminated function body: {signature}")


def main() -> None:
    desktop = (ROOT / "src/desktop/desktop.c").read_text(encoding="utf-8")
    runtime = (ROOT / "src/desktop/desktop_runtime.c").read_text(
        encoding="utf-8"
    )
    menu_logout = function_body(desktop, "static void menu_action_logout(")
    assert "desktop_request_logout()" in menu_logout
    assert "desktop_stop()" not in menu_logout

    request_logout = function_body(runtime, "int desktop_request_logout(")
    guard = request_logout.find("!g_desktop_active || !g_desktop_shell_ctx")
    mark = request_logout.find("shell_request_logout(g_desktop_shell_ctx)")
    stop = request_logout.find("desktop_stop();")
    assert guard >= 0, "logout must fail closed without a live desktop context"
    assert guard < mark < stop, "logout must mark the session before stopping GUI"
    assert "return -1" in request_logout
    assert "return 0" in request_logout

    runtime_start = function_body(runtime, "int desktop_runtime_start(")
    adopt = runtime_start.find("desktop_scheduler_adopt_current()")
    rebind = runtime_start.find(
        "session_set_active(shell_context_session(ctx))", adopt
    )
    preempt_off = runtime_start.find("scheduler_preempt_disable()", rebind)
    initial_frame = runtime_start.find(
        "desktop_present_initial_frame", preempt_off
    )
    preempt_on = runtime_start.find("scheduler_preempt_enable()", initial_frame)
    assert adopt >= 0 and rebind > adopt, "adopted desktop task must receive session"
    assert preempt_off >= 0 and preempt_off < initial_frame < preempt_on, (
        "initial frame must be guarded"
    )
    adopt_failure = runtime_start[
        runtime_start.find("if (desktop_scheduler_adopt_current()") : rebind
    ]
    assert "desktop_shutdown(&g_desktop)" in adopt_failure
    assert "session_set_active(previous_session)" in adopt_failure

    scoped_shell = function_body(
        runtime, "int kernel_desktop_dispatch_shell_command_scoped("
    )
    shell_lookup = scoped_shell.find("desktop_session = shell_context_session(ctx)")
    shell_guard = scoped_shell.find("if (!desktop_session)", shell_lookup)
    shell_bind = scoped_shell.find("session_set_active(desktop_session)", shell_guard)
    assert 0 <= shell_lookup < shell_guard < shell_bind
    assert "ctx == g_desktop_shell_ctx" not in scoped_shell

    scoped_typed = function_body(
        runtime, "int kernel_desktop_run_session_operation_scoped("
    )
    typed_lookup = scoped_typed.find("operation_session = shell_context_session(ctx)")
    typed_guard = scoped_typed.find("if (!operation_session)", typed_lookup)
    typed_bind = scoped_typed.find("session_set_active(operation_session)", typed_guard)
    assert 0 <= typed_lookup < typed_guard < typed_bind

    print("[tests] desktop logout contract OK")


if __name__ == "__main__":
    main()
