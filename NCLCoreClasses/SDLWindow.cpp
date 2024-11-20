#include "SDLWindow.h"

#include <SDL2/SDL.h>

namespace NCL::UnixCode {
    SDLWindow::SDLWindow(const WindowInitialisation& initData) : Window() {
        sdlWindow = SDL_CreateWindow(
            initData.windowTitle.c_str(),
            initData.windowPositionX, initData.windowPositionY,
            initData.width, initData.height,
            // TODO: Vulkan support
            SDL_WINDOW_OPENGL
        );

        init = sdlWindow != nullptr;
    }

    bool SDLWindow::InternalUpdate() {
        // TODO: Implement

        // TODO: Force quit
        return true;
    }
}
