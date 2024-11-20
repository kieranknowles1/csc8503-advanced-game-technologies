#pragma once

#include "Window.h"

namespace NCL::UnixCode {
    class SDLWindow : public Window {
public:
    SDLWindow(const WindowInitialisation& init);
protected:
    void* sdlWindow;

    bool InternalUpdate();
    };
}
