#pragma once

#include <SDL2/SDL_events.h>

#include "Keyboard.h"

namespace NCL::UnixCode {
    class SDLKeyboard : public Keyboard {
    public:
        void handleEvent(const SDL_KeyboardEvent& event);
    private:
    };
}
