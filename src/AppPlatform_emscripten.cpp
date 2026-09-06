#include "AppPlatform_emscripten.h"
#include "util/Mth.h"

// Query the actual browser window / canvas size at call time so the game
// always knows the real render resolution instead of using a fixed value.
int AppPlatform_emscripten::getScreenWidth() {
    int w = EM_ASM_INT({ return window.innerWidth; });
    return (w > 0) ? w : 854;
}
int AppPlatform_emscripten::getScreenHeight() {
    int h = EM_ASM_INT({ return window.innerHeight; });
    return (h > 0) ? h : 480;
}

float AppPlatform_emscripten::getPixelsPerMillimeter() {
    // 1 CSS inch = 96 CSS pixels. 1 inch = 25.4 mm => ~3.7795 CSS pixels per mm
    // Physical pixels per mm = 3.7795 * window.devicePixelRatio
    double dpr = EM_ASM_DOUBLE({ return window.devicePixelRatio || 1.0; });
    return (float)(3.7795 * dpr);
}

