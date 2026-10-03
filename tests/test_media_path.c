#include "../src/apps/media_path.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
    assert(media_path_is_audio("/home/song.wav"));
    assert(media_path_is_audio("/home/song.WAV"));
    assert(media_path_is_audio("/home/song.ogg"));
    assert(media_path_is_audio("/home/song.OgG"));
    assert(!media_path_is_audio(NULL));
    assert(!media_path_is_audio(""));
    assert(!media_path_is_audio("ogg"));
    assert(!media_path_is_audio("/home/song.ogg.txt"));
    assert(!media_path_is_audio("/home/song.ogx"));
    assert(!media_path_is_audio("/home/song.mp3"));
    puts("[media-path] WAV/OGG dispatch and negative routes: PASS");
    return 0;
}
