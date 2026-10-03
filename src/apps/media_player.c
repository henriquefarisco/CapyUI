#include "apps/media_player.h"

#include "audio/audio_service.h"
#include "fs/vfs.h"
#include "gui/compositor.h"
#include "gui/font.h"
#include "gui/inline_prompt.h"
#include "lang/app_language.h"
#include "lang/localization.h"
#include "util/kstring.h"

#include <stdint.h>

#define MEDIA_PLAYER_APP_ID 0x4d504c59u

struct media_player_state {
    struct gui_window *window;
    char queue[MEDIA_PLAYER_QUEUE_MAX][MEDIA_PLAYER_PATH_MAX];
    size_t count;
    int selected;
    int playing_index;
    int auto_advance;
    uint64_t playback_id;
    uint32_t progress;
    uint16_t volume;
    uint16_t level_left, level_right;
    char status[96];
};

static struct media_player_state g_media;

static const char *mp_t(const char *pt, const char *en, const char *es) {
    return localization_select(app_current_language(), pt, en, es);
}

static void mp_fill(struct gui_surface *s, int32_t x, int32_t y,
                    uint32_t width, uint32_t height, uint32_t color) {
    uint32_t by;
    for (by = 0; by < height; ++by) {
        int32_t py = y + (int32_t)by;
        uint32_t bx;
        uint32_t *line;
        if (py < 0 || (uint32_t)py >= s->height) continue;
        line = (uint32_t *)((uint8_t *)s->pixels + (uint32_t)py * s->pitch);
        for (bx = 0; bx < width; ++bx) {
            int32_t px = x + (int32_t)bx;
            if (px >= 0 && (uint32_t)px < s->width) line[px] = color;
        }
    }
}

static void mp_button(struct gui_surface *s, const struct font *font,
                      int32_t x, int32_t y, int32_t width,
                      const char *label, uint32_t bg, uint32_t fg) {
    mp_fill(s, x, y, (uint32_t)width, 28u, bg);
    font_draw_string(s, font, x + 8, y + 7, label, fg);
}

static const char *mp_basename(const char *path) {
    const char *base = path;
    if (!path) return "";
    while (*path) {
        if (*path == '/') base = path + 1;
        ++path;
    }
    return base;
}

static void mp_paint(struct gui_window *win) {
    const struct gui_theme_palette *theme = compositor_theme();
    const struct font *font = font_default();
    struct audio_service_status audio;
    size_t i;
    if (!win || !font) return;
    mp_fill(&win->surface, 0, 0, win->surface.width, win->surface.height,
            theme->window_bg);
    font_draw_string(&win->surface, font, 14, 12,
                     mp_t("Lista de reproducao", "Playlist", "Lista de reproduccion"),
                     theme->text);
    for (i = 0; i < g_media.count; ++i) {
        int32_t y = 38 + (int32_t)i * 26;
        if ((int)i == g_media.selected)
            mp_fill(&win->surface, 10, y - 3, win->surface.width - 20u, 24u,
                    theme->accent_alt);
        font_draw_string(&win->surface, font, 16, y,
                         mp_basename(g_media.queue[i]), theme->text);
    }
    mp_button(&win->surface, font, 10, 262, 92,
              mp_t("Adicionar", "Add file", "Agregar"),
              theme->accent_alt, theme->text);
    mp_button(&win->surface, font, 108, 262, 72,
              mp_t("Tocar", "Play", "Tocar"), theme->accent_alt, theme->text);
    mp_button(&win->surface, font, 186, 262, 72,
              mp_t("Parar", "Stop", "Parar"), theme->accent_alt, theme->text);
    mp_button(&win->surface, font, 264, 262, 66, "Vol -",
              theme->accent_alt, theme->text);
    mp_button(&win->surface, font, 336, 262, 66, "Vol +",
              theme->accent_alt, theme->text);
    mp_button(&win->surface, font, 408, 262, 104,
              mp_t("Tom teste", "Test tone", "Tono test"),
              theme->accent_alt, theme->text);
    mp_fill(&win->surface, 14, 298, win->surface.width - 28u, 6u, theme->accent_alt);
    mp_fill(&win->surface, 14, 298,
            (win->surface.width - 28u) * g_media.progress / 100u, 6u, theme->accent);
    if (audio_service_get_status(&audio) == 0 && audio.playing &&
        audio.active_app_id == MEDIA_PLAYER_APP_ID &&
        audio.playback_id == g_media.playback_id)
        font_draw_string(&win->surface, font, 14, 310,
                         mp_t("Reproduzindo", "Playing", "Reproduciendo"),
                         theme->accent);
    font_draw_string(&win->surface, font, 14, 334, g_media.status,
                     theme->text_muted);
    if (win->surface.width > 56u) {
        uint32_t width = win->surface.width - 56u;
        uint16_t levels[2] = {g_media.level_left, g_media.level_right};
        for (unsigned ch = 0; ch < 2; ++ch) {
            int32_t y = 354 + (int32_t)ch * 16;
            uint32_t peak = levels[ch] > 32768u ? 32768u : levels[ch];
            font_draw_string(&win->surface, font, 14, y - 3, ch ? "R" : "L", theme->text_muted);
            mp_fill(&win->surface, 28, y, width, 6u, theme->accent_alt);
            mp_fill(&win->surface, 28, y, (uint32_t)((uint64_t)width * peak / 32768u), 6u, theme->accent);
        }
    }
}

static void mp_set_status(const char *status) {
    kstrcpy(g_media.status, sizeof(g_media.status), status);
    if (g_media.window) compositor_invalidate(g_media.window->id);
}

int media_player_enqueue(const char *path) {
    size_t i;
    if (!path || !path[0] || kstrlen(path) >= MEDIA_PLAYER_PATH_MAX) return -1;
    for (i = 0; i < g_media.count; ++i) {
        if (kstreq(g_media.queue[i], path)) {
            g_media.selected = (int)i;
            return 0;
        }
    }
    if (g_media.count >= MEDIA_PLAYER_QUEUE_MAX) return -1;
    kstrcpy(g_media.queue[g_media.count], MEDIA_PLAYER_PATH_MAX, path);
    g_media.selected = (int)g_media.count;
    ++g_media.count;
    return 0;
}

size_t media_player_queue_count(void) { return g_media.count; }

static void mp_stop_owned(void) {
    struct audio_service_status audio;
    g_media.auto_advance = 0;
    g_media.level_left = g_media.level_right = 0;
    if (audio_service_get_status(&audio) == 0 && audio.playing &&
        audio.active_app_id == MEDIA_PLAYER_APP_ID &&
        audio.playback_id == g_media.playback_id)
        audio_service_stop();
}

static int mp_play_selected(void) {
    struct audio_service_status audio;
    g_media.auto_advance = 0;
    if (g_media.selected < 0 || (size_t)g_media.selected >= g_media.count)
        return -1;
    if (audio_service_play_wav_file(MEDIA_PLAYER_APP_ID,
                                    g_media.queue[g_media.selected]) != 0) {
        mp_stop_owned();
        mp_set_status(mp_t("Falha: use WAV/OGG 48 kHz", "Failed: use 48 kHz WAV/OGG",
                           "Fallo: use WAV/OGG 48 kHz"));
        return -1;
    }
    if (audio_service_get_status(&audio) != 0 || !audio.playing ||
        audio.active_app_id != MEDIA_PLAYER_APP_ID) return -1;
    g_media.playback_id = audio.playback_id;
    g_media.playing_index = g_media.selected;
    g_media.progress = 0;
    g_media.auto_advance = 1;
    mp_set_status(mp_t("Audio iniciado", "Audio started", "Audio iniciado"));
    return 0;
}

void media_player_poll(void) {
    struct audio_service_status audio;
    uint32_t progress;
    if (!g_media.window || audio_service_get_status(&audio) != 0) return;
    g_media.volume = audio.global_volume;
    uint16_t left = 0, right = 0;
    if (audio.playing && !audio.last_error && audio.active_app_id == MEDIA_PLAYER_APP_ID &&
        audio.playback_id == g_media.playback_id)
        (void)audio_service_get_app_levels(MEDIA_PLAYER_APP_ID, &left, &right);
    if (left != g_media.level_left || right != g_media.level_right) {
        g_media.level_left = left;
        g_media.level_right = right;
        compositor_invalidate(g_media.window->id);
    }
    if (!g_media.auto_advance) return;
    if (audio.playback_id != g_media.playback_id || audio.last_error ||
        (audio.playing && audio.active_app_id != MEDIA_PLAYER_APP_ID)) {
        g_media.auto_advance = 0;
        mp_set_status(mp_t("Reproducao interrompida", "Playback interrupted",
                           "Reproduccion interrumpida"));
        return;
    }
    /* Source frames are bounded by the service's 8 MiB PCM policy. */
    progress = !audio.source_frames ? 0u :
        (audio.played_frames >= audio.source_frames ? 100u :
         (uint32_t)(audio.played_frames * 100u / audio.source_frames));
    if (progress != g_media.progress) {
        g_media.progress = progress;
        compositor_invalidate(g_media.window->id);
    }
    if (audio.playing) return;
    g_media.auto_advance = 0; /* Consume EOF once, including the final item. */
    if (!audio.completed) {
        mp_set_status(mp_t("Parado", "Stopped", "Detenido"));
    } else if (g_media.playing_index >= 0 &&
               (size_t)(g_media.playing_index + 1) < g_media.count) {
        g_media.selected = g_media.playing_index + 1;
        (void)mp_play_selected();
    } else {
        mp_set_status(mp_t("Lista concluida", "Playlist completed", "Lista completada"));
    }
}

static void mp_prompt_submit(const char *text, void *ctx) {
    (void)ctx;
    if (media_player_enqueue(text) == 0)
        mp_set_status(mp_t("Arquivo adicionado", "File added", "Archivo agregado"));
    else
        mp_set_status(mp_t("Caminho ou lista invalida", "Invalid path or full queue",
                           "Ruta invalida o lista llena"));
}

static void mp_mouse(struct gui_window *win, int32_t x, int32_t y,
                     uint8_t buttons) {
    if (!win || !(buttons & 1u)) return;
    if (y >= 35 && y < 35 + (int32_t)g_media.count * 26) {
        g_media.selected = (y - 35) / 26;
    } else if (y >= 262 && y < 290) {
        if (x >= 10 && x < 102) {
            inline_prompt_show(mp_t("Caminho WAV/OGG:", "WAV/OGG path:", "Ruta WAV/OGG:"),
                               "", win->frame.x + 20, win->frame.y + 150,
                               mp_prompt_submit, 0);
        } else if (x >= 108 && x < 180) {
            (void)mp_play_selected();
        } else if (x >= 186 && x < 258) {
            mp_stop_owned();
            g_media.progress = 0;
            mp_set_status(mp_t("Parado", "Stopped", "Detenido"));
        } else if (x >= 264 && x < 330) {
            g_media.volume = g_media.volume >= 100u ? g_media.volume - 100u : 0u;
            (void)audio_service_set_global_volume(g_media.volume);
            mp_set_status(mp_t("Volume reduzido", "Volume lowered", "Volumen reducido"));
        } else if (x >= 336 && x < 402) {
            g_media.volume = g_media.volume <= 900u ? g_media.volume + 100u : 1000u;
            (void)audio_service_set_global_volume(g_media.volume);
            mp_set_status(mp_t("Volume aumentado", "Volume raised", "Volumen aumentado"));
        } else if (x >= 408 && x < 512) {
            struct audio_service_status audio;
            g_media.auto_advance = 0;
            g_media.progress = 0;
            if (audio_service_play_test_tone(MEDIA_PLAYER_APP_ID) == 0) {
                if (audio_service_get_status(&audio) == 0)
                    g_media.playback_id = audio.playback_id;
                mp_set_status(mp_t("Tom de teste ativo", "Test tone active",
                                   "Tono de prueba activo"));
            } else
                mp_set_status(mp_t("Dispositivo de audio indisponivel",
                                   "Audio device unavailable",
                                   "Dispositivo de audio no disponible"));
        }
    }
    compositor_invalidate(win->id);
}

static void mp_close(struct gui_window *win) {
    (void)win;
    mp_stop_owned();
    g_media.progress = 0;
    g_media.window = 0;
}

void media_player_open(void) {
    struct audio_service_status audio;
    if (g_media.window) {
        compositor_show_window(g_media.window->id);
        compositor_focus_window(g_media.window->id);
        return;
    }
    if (audio_service_get_status(&audio) == 0)
        g_media.volume = audio.global_volume;
    if (g_media.count == 0) {
        static const char *const presets[] = {
            "/Music/Capy Acoustic.ogg", "/Music/Capy Opera.ogg", "/Music/Capy Sound.ogg"
        };
        g_media.selected = -1;
        for (size_t i = 0; i < sizeof(presets) / sizeof(presets[0]); ++i) {
            struct vfs_stat st;
            if (vfs_stat_path(presets[i], &st) == VFS_OK && st.mode == VFS_MODE_FILE)
                (void)media_player_enqueue(presets[i]);
        }
        if (g_media.count) g_media.selected = 0;
    }
    g_media.window = compositor_create_window("Media Player", 120, 80, 540, 390);
    if (!g_media.window) return;
    g_media.window->user_data = &g_media;
    g_media.window->on_paint = mp_paint;
    g_media.window->on_mouse = mp_mouse;
    g_media.window->on_close = mp_close;
    mp_set_status(mp_t("Adicione WAV/OGG ou use o tom de teste",
                       "Add WAV/OGG or use the test tone",
                       "Agregue WAV/OGG o use el tono de prueba"));
    compositor_show_window(g_media.window->id);
    compositor_focus_window(g_media.window->id);
}

int media_player_open_path(const char *path) {
    media_player_open();
    if (!g_media.window || media_player_enqueue(path) != 0) return -1;
    compositor_invalidate(g_media.window->id);
    return mp_play_selected();
}

int media_player_smoke_roundtrip(void) {
    struct media_player_state saved = g_media;
    static const char *extra[] = {
        "/home/c.wav", "/home/d.wav", "/home/e.wav",
        "/home/f.wav", "/home/g.wav", "/home/h.wav"
    };
    size_t i;
    int result = 0;
    kmemzero(&g_media, sizeof(g_media));
    g_media.selected = -1;
    if (media_player_enqueue("/home/a.wav") != 0 ||
        media_player_enqueue("/home/b.wav") != 0 ||
        media_player_enqueue("/home/a.wav") != 0 ||
        g_media.count != 2u || g_media.selected != 0 ||
        !kstreq(mp_basename(g_media.queue[1]), "b.wav")) result = 1;
    if (media_player_enqueue(0) == 0 || media_player_enqueue("") == 0)
        result = 1;
    for (i = 0; i < sizeof(extra) / sizeof(extra[0]); ++i) {
        if (media_player_enqueue(extra[i]) != 0) result = 1;
    }
    if (media_player_enqueue("/home/a.wav") != 0 ||
        media_player_enqueue("/home/new.wav") == 0 ||
        g_media.count != MEDIA_PLAYER_QUEUE_MAX) result = 1;
    g_media = saved;
    return result;
}

#ifdef CAPYOS_MEDIA_PLAYER_SMOKE
#include "arch/x86_64/timebase.h"
#include "drivers/serial/serial_com1.h"
#include "gui/desktop_runtime.h"
#include "kernel/log/klog.h"
static uint64_t mp_smoke_started;
static unsigned mp_smoke_frames, mp_smoke_progress, mp_smoke_levels;
static int mp_smoke_result = -1;

int media_player_smoke_start(void) {
    mp_smoke_started = x64_timebase_ticks_100hz();
    mp_smoke_frames = mp_smoke_progress = mp_smoke_levels = 0;
    mp_smoke_result = -1;
    com1_puts("[smoke] media-player-playlist starting\n");
    if (audio_service_smoke_preemption() != 0) {
        com1_puts("[smoke] media-player-playlist FAIL preemption-SIMD\n");
        desktop_stop();
        return -1;
    }
    com1_puts("[smoke] media-player-playlist preemption-SIMD ready\n");
#ifdef CAPYOS_BUILTIN_MUSIC_SMOKE
    media_player_open();
    if (!g_media.window || g_media.count != 3 || mp_play_selected() != 0) {
        com1_puts("[smoke] media-player-playlist FAIL presets\n");
        desktop_stop();
        return -1;
    }
#else
#ifdef CAPYOS_AUDIO_OGG_SMOKE
    const char *first_track = "/audio-smoke.ogg";
#else
    const char *first_track = "/audio-smoke.wav";
#endif
    media_player_open();
    /* Fixture smoke owns its queue; preset discovery is tested separately. */
    g_media.count = 0;
    g_media.selected = -1;
    if (media_player_open_path(first_track) != 0 ||
        media_player_enqueue("/audio-smoke-next.wav") != 0) {
        com1_puts("[smoke] media-player-playlist FAIL start\n");
        desktop_stop();
        return -1;
    }
#endif
    if (audio_service_smoke_guarded_playback() != 0) {
        com1_puts("[smoke] media-player-playlist FAIL guarded-DMA\n");
        mp_stop_owned();
        desktop_stop();
        return -1;
    }
    com1_puts("[smoke] media-player-playlist guarded-DMA ready\n");
    return 0;
}

void media_player_smoke_note_frame(void) {
    struct audio_service_status audio = {0};
#ifdef CAPYOS_BUILTIN_MUSIC_SMOKE
    const int tracks = 3;
    const uint64_t timeout = 60000u;
#else
    const int tracks = 2;
    const uint64_t timeout = 1500u;
#endif
    const unsigned all_tracks = (1u << tracks) - 1u;
    ++mp_smoke_frames;
    if (g_media.playing_index >= 0 && g_media.playing_index < tracks &&
        g_media.level_left > 0 && g_media.level_right > 0 &&
        g_media.level_left <= 32768u && g_media.level_right <= 32768u)
        mp_smoke_levels |= 1u << g_media.playing_index;
    if (g_media.playing_index >= 0 && g_media.playing_index < tracks &&
        g_media.progress > 0 && g_media.progress < 100)
        mp_smoke_progress |= 1u << g_media.playing_index;
    if (audio_service_get_status(&audio) == 0 && !audio.last_error &&
        audio.completed && !g_media.auto_advance && g_media.playing_index == tracks - 1 &&
        g_media.progress == 100 && mp_smoke_progress == all_tracks && mp_smoke_levels == all_tracks &&
        !g_media.level_left && !g_media.level_right && mp_smoke_frames >= 4u) {
        mp_smoke_result = 0;
#ifdef CAPYOS_BUILTIN_MUSIC_SMOKE
        com1_puts("[smoke] media-player-playlist rendered-three-presets\n");
#else
        com1_puts("[smoke] media-player-playlist rendered-two-tracks\n");
#endif
        desktop_stop();
    } else if (audio.last_error || !g_media.auto_advance ||
               x64_timebase_ticks_100hz() - mp_smoke_started > timeout) {
        com1_puts("[smoke] media-player-playlist FAIL playback-or-progress\n");
        klog_dump(com1_puts);
        mp_stop_owned();
        desktop_stop();
    }
}

int media_player_smoke_result(void) { return mp_smoke_result; }
#endif
