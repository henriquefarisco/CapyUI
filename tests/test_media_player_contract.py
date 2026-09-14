#!/usr/bin/env python3
"""Check source completeness and wiring, not audio/runtime behavior.

The behavioral player test lives in the CapyOS consumer's audio-selftest.
An optional source-root argument allows checking an exported source tree without
trusting existing compiled objects or requiring Git metadata in that export.
"""

from pathlib import Path
import sys

from test_desktop_logout_contract import function_body


ROOT = Path(__file__).resolve().parents[1]


def check_contract(root: Path) -> None:
    paths = {
        "player": "src/apps/media_player.c",
        "apps": "src/apps/apps_smoke.c",
        "files": "src/apps/file_manager.c",
        "desktop": "src/desktop/desktop.c",
        "runtime": "src/desktop/desktop_runtime.c",
    }
    sources = {}
    for name, relative in paths.items():
        path = root / relative
        assert path.is_file(), f"missing Media Player integration source: {relative}"
        sources[name] = path.read_text(encoding="utf-8")

    for signature in (
        "void media_player_open(",
        "int media_player_open_path(",
        "int media_player_enqueue(",
        "void media_player_poll(",
        "int media_player_smoke_roundtrip(",
    ):
        function_body(sources["player"], signature)

    launch = function_body(sources["desktop"], "static void menu_action_media_player(")
    assert "media_player_open()" in launch, "launcher must open the owned player"
    menu = function_body(sources["desktop"], "static void desktop_menu_apps(")
    assert "menu_action_media_player" in menu, "player must be registered in launcher"
    files = function_body(sources["files"], "void fm_open_entry(")
    assert "media_player_open_path(path)" in files, "WAV files must reach the player"
    apps = function_body(sources["apps"], "int apps_smoke_roundtrip_run(")
    assert "media_player_smoke_roundtrip()" in apps, "apps smoke must cover the player"
    runtime = function_body(sources["runtime"], "int desktop_runtime_start(")
    worker = runtime.find("audio_service_start_worker()")
    audio_poll = runtime.find("audio_service_poll();")
    player_poll = runtime.find("media_player_poll();")
    frame = runtime.find("desktop_run_frame(")
    assert 0 <= worker < audio_poll < player_poll < frame, (
        "desktop must start audio and poll service/player before rendering"
    )


if __name__ == "__main__":
    assert len(sys.argv) <= 2, "usage: test_media_player_contract.py [source-root]"
    check_contract(Path(sys.argv[1]) if len(sys.argv) == 2 else ROOT)
    print("[tests] Media Player sources and desktop wiring OK")
