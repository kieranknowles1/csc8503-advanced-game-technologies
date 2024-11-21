#include "SDLKeyboard.h"

namespace NCL::UnixCode {
    void SDLKeyboard::handleEvent(const SDL_KeyboardEvent &event)
    {
        bool down = event.state == SDL_PRESSED;
        SDL_Keycode key = event.keysym.sym;
        KeyCodes::Type keyCode = convertKey(key);

        keyStates[keyCode] = down;
    }

    KeyCodes::Type SDLKeyboard::convertKey(SDL_Keycode key) {
        // SDL2 uses lowercase letters for keycodes
        // Windows uses uppercase
        if (key >= SDLK_a && key <= SDLK_z) {
            return (KeyCodes::Type)(key - ('a' - 'A'));
        }

        // Most other keys are the same
        return (KeyCodes::Type)key;
    }
}
