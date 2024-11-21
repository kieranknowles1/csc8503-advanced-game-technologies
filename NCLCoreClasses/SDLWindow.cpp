#include "SDLWindow.h"

#include <SDL2/SDL_events.h>

namespace NCL::UnixCode {
    SDLWindow::SDLWindow(const WindowInitialisation& initData) : Window() {
        // Use a borderless window. Most modern games do this
        // for seamless alt-tabbing
        int fullScreenFlag = initData.fullScreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0;
        sdlWindow = SDL_CreateWindow(
            initData.windowTitle.c_str(),
            initData.windowPositionX, initData.windowPositionY,
            initData.width, initData.height,
            // TODO: Vulkan support
            SDL_WINDOW_OPENGL | fullScreenFlag
        );

        init = sdlWindow != nullptr;

        // TODO: The base class should set this
        size = Vector2i(initData.width, initData.height);
        defaultSize = size;

        sdlKeyboard = new SDLKeyboard();
        sdlMouse = new SDLMouse();
        keyboard = sdlKeyboard;
        mouse = sdlMouse;
    }

    void SDLWindow::LockMouseToWindow(bool lock)
    {
        // TODO: Implement
    }

    void SDLWindow::ShowOSPointer(bool show)
    {
        // TODO: Implement
    }

    void SDLWindow::SetWindowPosition(int x, int y)
    {
        SDL_SetWindowPosition(sdlWindow, x, y);
    }

    void SDLWindow::SetFullScreen(bool state)
    {
        // TODO: Implement
    }

    void SDLWindow::SetConsolePosition(int x, int y)
    {
        // TODO: Implement
    }

    void SDLWindow::ShowConsole(bool state)
    {
        // TODO: Implement
    }

    void SDLWindow::UpdateTitle()
    {
        // TODO: Implement
    }

    // Handle any events that have been triggered
    // Returns false if quit was requested
    bool SDLWindow::InternalUpdate() {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    return false;
                case SDL_KEYDOWN:
                case SDL_KEYUP:
                    sdlKeyboard->handleEvent(event.key);
                    break;
                case SDL_MOUSEMOTION:
                    sdlMouse->handleEvent(event.motion);
                    break;
                case SDL_MOUSEBUTTONDOWN:
                case SDL_MOUSEBUTTONUP:
                    sdlMouse->handleEvent(event.button);
                    break;
            }
        };

        return true;
    }
}