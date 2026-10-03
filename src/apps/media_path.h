#ifndef CAPYUI_MEDIA_PATH_H
#define CAPYUI_MEDIA_PATH_H
#include <stddef.h>

/* Routing hint only: the codec validates content, regardless of extension. */
static inline int media_path_is_audio(const char *path) {
    if (!path) return 0;
    size_t n = 0;
    while (path[n]) ++n;
    if (n < 4 || path[n-4] != '.') return 0;
    unsigned a = (unsigned char)path[n-3] | 32u;
    unsigned b = (unsigned char)path[n-2] | 32u;
    unsigned c = (unsigned char)path[n-1] | 32u;
    return (a == 'w' && b == 'a' && c == 'v') ||
           (a == 'o' && b == 'g' && c == 'g');
}
#endif
