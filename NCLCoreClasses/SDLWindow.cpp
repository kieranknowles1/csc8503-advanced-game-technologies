#include "SDLWindow.h"

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

        // TODO: Implement
        // mouse = new SDLMouse(sdlWindow);
        // keyboard = new SDLKeyboard(sdlWindow);
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

    bool SDLWindow::InternalUpdate() {
        // TODO: Implement

        // TODO: Force quit
        return true;
    }
}
