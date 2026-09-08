#include <emscripten.h>
#include <emscripten/html5.h>
#include <SDL2/SDL.h>
#include <GLES/gl.h>

#include "client/renderer/gles.h"
#include <cstdio>
#include <cctype>
#include "platform/input/Mouse.h"
#include "platform/input/Multitouch.h"
#include "platform/input/Keyboard.h"
#include "util/Mth.h"
#include "AppPlatform_emscripten.h"
#include "NinecraftApp.h"
#define MAIN_CLASS NinecraftApp

static App* g_app = 0;
static SDL_Window* g_window = 0;
static SDL_GLContext g_glContext = 0;
AppContext appContext;

// Mouse state for delta tracking and drag-to-look
static int g_mouseX = 0, g_mouseY = 0;
static bool g_pointerLocked = false;

extern "C" EMSCRIPTEN_KEEPALIVE int emscripten_is_pointer_locked() {
    return g_pointerLocked ? 1 : 0;
}

// Screen dimensions – initialised from the actual browser window at startup
// and kept in sync on resize via emscripten_set_resize_callback.
static int g_screenW = 854, g_screenH = 480;

// Forward-declare so the resize callback can call app pointer.
static App* g_appPtr = nullptr;

static EM_BOOL onWindowResize(int /*eventType*/, const EmscriptenUiEvent* e, void* /*userData*/) {
    int newW = EM_ASM_INT({ return Math.round(window.innerWidth * (window.devicePixelRatio || 1)); });
    int newH = EM_ASM_INT({ return Math.round(window.innerHeight * (window.devicePixelRatio || 1)); });
    if (newW < 1 || newH < 1) return EM_FALSE;
    g_screenW = newW;
    g_screenH = newH;
    emscripten_set_canvas_element_size("#canvas", newW, newH);
    // Resize the SDL backing surface so OpenGL viewport matches the window
    if (g_window) SDL_SetWindowSize(g_window, newW, newH);
    // Tell the game its new logical dimensions
    if (g_appPtr) g_appPtr->setSize(newW, newH);
    return EM_FALSE;
}

// Convert SDL keycode to the game's expected keycode
// Game uses uppercase ASCII for letters, and specific codes for special keys
static int sdlKeyToGame(SDL_Keycode sym) {
    // Letters: SDL sends lowercase (97-122), game expects uppercase (65-90)
    if (sym >= SDLK_a && sym <= SDLK_z) {
        return sym - 32; // lowercase → uppercase ASCII
    }
    // Special keys
    switch (sym) {
        case SDLK_SPACE:    return 32;
        case SDLK_RETURN:   return 13;
        case SDLK_BACKSPACE:return 8;
        case SDLK_ESCAPE:   return 27;
        case SDLK_LSHIFT:
        case SDLK_RSHIFT:   return 10; // Keyboard::KEY_LSHIFT
        case SDLK_F1:       return 112;
        case SDLK_F2:       return 113;
        case SDLK_F3:       return 114;
        case SDLK_F4:       return 115;
        case SDLK_F5:       return 116;
        case SDLK_F6:       return 117;
        case SDLK_F7:       return 118;
        case SDLK_F8:       return 119;
        case SDLK_F9:       return 120;
        case SDLK_F10:      return 121;
        case SDLK_F11:      return 122;
        case SDLK_F12:      return 123;
        default:            return (int)sym;
    }
}

// Sync IDBFS saves to IndexedDB
extern "C" void EMSCRIPTEN_KEEPALIVE syncSaves() {
    EM_ASM({
        if (typeof FS !== 'undefined') {
            FS.syncfs(false, function(err) {
                if (err) console.error('FS.syncfs save error:', err);
            });
        }
    });
}

// Touch tracking for multitouch support
static SDL_FingerID s_touchSlots[8] = {-1, -1, -1, -1, -1, -1, -1, -1};
static bool s_touchActive[8] = {false, false, false, false, false, false, false, false};

static int getOrAllocTouchSlot(SDL_FingerID fingerId) {
    for (int i = 0; i < 8; ++i) {
        if (s_touchActive[i] && s_touchSlots[i] == fingerId) {
            return i;
        }
    }
    for (int i = 0; i < 8; ++i) {
        if (!s_touchActive[i]) {
            s_touchActive[i] = true;
            s_touchSlots[i] = fingerId;
            return i;
        }
    }
    return 0;
}

static int getTouchSlot(SDL_FingerID fingerId) {
    for (int i = 0; i < 8; ++i) {
        if (s_touchActive[i] && s_touchSlots[i] == fingerId) {
            return i;
        }
    }
    return 0;
}

static void releaseTouchSlot(SDL_FingerID fingerId) {
    for (int i = 0; i < 8; ++i) {
        if (s_touchActive[i] && s_touchSlots[i] == fingerId) {
            s_touchActive[i] = false;
            s_touchSlots[i] = -1;
            return;
        }
    }
}

void main_loop() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                emscripten_cancel_main_loop();
                return;

            case SDL_MOUSEBUTTONDOWN: {
                int button = (event.button.button == SDL_BUTTON_RIGHT)
                    ? MouseAction::ACTION_RIGHT : MouseAction::ACTION_LEFT;
                int gx = event.button.x;
                int gy = event.button.y;
                Mouse::feed(button, MouseAction::DATA_DOWN, gx, gy);
                if (!g_pointerLocked) {
                    Multitouch::feed(MouseAction::ACTION_LEFT, MouseAction::DATA_DOWN, gx, gy, 0);
                }
                g_mouseX = gx;
                g_mouseY = gy;
                break;
            }

            case SDL_MOUSEBUTTONUP: {
                int button = (event.button.button == SDL_BUTTON_RIGHT)
                    ? MouseAction::ACTION_RIGHT : MouseAction::ACTION_LEFT;
                int gx = event.button.x;
                int gy = event.button.y;
                Mouse::feed(button, MouseAction::DATA_UP, gx, gy);
                if (!g_pointerLocked) {
                    Multitouch::feed(MouseAction::ACTION_LEFT, MouseAction::DATA_UP, gx, gy, 0);
                }
                break;
            }

            case SDL_MOUSEMOTION: {
                int gx = event.motion.x;
                int gy = event.motion.y;

                // Mouse delta: use relative motion when pointer locked, else compute manually
                int dx, dy;
                if (g_pointerLocked) {
                    dx = event.motion.xrel;
                    dy = event.motion.yrel;
                } else {
                    dx = gx - g_mouseX;
                    dy = gy - g_mouseY;
                }
                g_mouseX = gx;
                g_mouseY = gy;

                // Feed position + delta into mouse (for MouseTurnInput camera)
                Mouse::feed(MouseAction::ACTION_MOVE, 0, gx, gy, (short)dx, (short)dy);

                // If left button held, also update the touch pointer (for in-game dragging)
                if (event.motion.state & SDL_BUTTON_LMASK) {
                    if (!g_pointerLocked) Multitouch::feed(MouseAction::ACTION_MOVE, 0, gx, gy, 0);
                }
                break;
            }

            case SDL_MOUSEWHEEL: {
                // event.wheel.y is positive for scrolling up (away from user), negative for scrolling down
                Mouse::feed(MouseAction::ACTION_WHEEL, 0, g_mouseX, g_mouseY, 0, (short)event.wheel.y);
                break;
            }

            case SDL_KEYDOWN: {
                int gameKey = sdlKeyToGame(event.key.keysym.sym);
                if (gameKey >= 0 && gameKey < 256)
                    Keyboard::feed((unsigned char)gameKey, 1);
                break;
            }

            case SDL_KEYUP: {
                int gameKey = sdlKeyToGame(event.key.keysym.sym);
                if (gameKey >= 0 && gameKey < 256)
                    Keyboard::feed((unsigned char)gameKey, 0);
                break;
            }

            // --- Multitouch Support ---
            case SDL_FINGERDOWN: {
                int slot = getOrAllocTouchSlot(event.tfinger.fingerId);
                int gx = (int)(event.tfinger.x * g_screenW);
                int gy = (int)(event.tfinger.y * g_screenH);
                Multitouch::feed(MouseAction::ACTION_LEFT, MouseAction::DATA_DOWN, (short)gx, (short)gy, (char)slot);
                if (slot == 0 || !Mouse::isButtonDown(MouseAction::ACTION_LEFT)) {
                    Mouse::feed(MouseAction::ACTION_LEFT, MouseAction::DATA_DOWN, (short)gx, (short)gy);
                }
                g_mouseX = gx;
                g_mouseY = gy;
                break;
            }

            case SDL_FINGERUP: {
                int slot = getTouchSlot(event.tfinger.fingerId);
                int gx = (int)(event.tfinger.x * g_screenW);
                int gy = (int)(event.tfinger.y * g_screenH);
                Multitouch::feed(MouseAction::ACTION_LEFT, MouseAction::DATA_UP, (short)gx, (short)gy, (char)slot);
                if (slot == 0 || !Multitouch::isPointerDown(0)) {
                    Mouse::feed(MouseAction::ACTION_LEFT, MouseAction::DATA_UP, (short)gx, (short)gy);
                }
                releaseTouchSlot(event.tfinger.fingerId);
                break;
            }

            case SDL_FINGERMOTION: {
                int slot = getTouchSlot(event.tfinger.fingerId);
                int gx = (int)(event.tfinger.x * g_screenW);
                int gy = (int)(event.tfinger.y * g_screenH);
                short dx = (short)(event.tfinger.dx * g_screenW);
                short dy = (short)(event.tfinger.dy * g_screenH);
                Multitouch::feed(MouseAction::ACTION_MOVE, 0, (short)gx, (short)gy, (char)slot);
                if (slot == 0) {
                    Mouse::feed(MouseAction::ACTION_MOVE, 0, (short)gx, (short)gy, dx, dy);
                    g_mouseX = gx;
                    g_mouseY = gy;
                }
                break;
            }
        }
    }

    // Check pointer lock state
    g_pointerLocked = EM_ASM_INT({
        return (document.pointerLockElement != null) ? 1 : 0;
    }) != 0;

    if (g_app && g_app->wantToQuit()) {
        emscripten_cancel_main_loop();
        return;
    }

    if (g_app) {
        g_app->update();
    }
    SDL_GL_SwapWindow(g_window);
}

int main(int argc, char* argv[]) {
    // Disable SDL's generation of mouse events for touches to let Multitouch handle it exclusively
    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
    SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "0");

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        printf("SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    // Request OpenGL ES 2.0 context (WebGL requires ES 2.0; LEGACY_GL_EMULATION handles ES 1.x on top)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);

    // Read the actual browser window size so the game renders at the real physical resolution
    int screenW = EM_ASM_INT({ return Math.round(window.innerWidth * (window.devicePixelRatio || 1)); });
    int screenH = EM_ASM_INT({ return Math.round(window.innerHeight * (window.devicePixelRatio || 1)); });
    if (screenW < 1) screenW = 854;
    if (screenH < 1) screenH = 480;
    g_screenW = screenW;
    g_screenH = screenH;

    emscripten_set_canvas_element_size("#canvas", screenW, screenH);

    g_window = SDL_CreateWindow(
        "Minecraft",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        screenW, screenH,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN
    );
    if (!g_window) {
        printf("SDL_CreateWindow failed: %s\n", SDL_GetError());
        return 1;
    }

    g_glContext = SDL_GL_CreateContext(g_window);
    if (!g_glContext) {
        printf("SDL_GL_CreateContext failed: %s\n", SDL_GetError());
        return 1;
    }
    SDL_GL_MakeCurrent(g_window, g_glContext);

    // Mount IDBFS in background so worlds persist across page reloads
    EM_ASM({
        try {
            if (!FS.analyzePath('/games').exists) {
                FS.mkdir('/games', 0777);
            }
        } catch (e) {
            console.warn('mkdir /games warning:', e);
        }
        try {
            FS.mount(IDBFS, {}, '/games');
            FS.syncfs(true, function(err) {
                if (err) console.warn('FS.syncfs load warning:', err);
                else console.log('IDBFS synced');
            });
        } catch (e) {
            console.warn('IDBFS mount warning:', e);
        }
    });

    appContext.platform = new AppPlatform_emscripten();
    glInit();

    App* app = new MAIN_CLASS();
    g_app = app;
    g_appPtr = app;  // keep pointer for resize callback
    ((MAIN_CLASS*)g_app)->externalStoragePath = ".";
    ((MAIN_CLASS*)g_app)->externalCacheStoragePath = ".";
    g_app->init(appContext);
    g_app->setSize(screenW, screenH);

    // Register an Emscripten resize callback so the game tracks browser window changes
    emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, EM_FALSE, onWindowResize);

    // Resume Web Audio on first user interaction if suspended
    EM_ASM({
        var unlockAudio = function() {
            if (typeof AL !== 'undefined' && AL.currentCtx && AL.currentCtx.audioCtx && AL.currentCtx.audioCtx.state === 'suspended') {
                AL.currentCtx.audioCtx.resume();
            }
        };
        window.addEventListener('click', unlockAudio, { passive: true });
        window.addEventListener('touchstart', unlockAudio, { passive: true });
        window.addEventListener('keydown', unlockAudio, { passive: true });
    });

    // Hook up emscripten loop (simulate_infinite_loop = 0 so it returns cleanly)
    emscripten_set_main_loop(main_loop, 0, 0);

    return 0;
}
