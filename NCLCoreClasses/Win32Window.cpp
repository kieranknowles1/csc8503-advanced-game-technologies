#include "Win32Window.h"
#ifdef _WIN32
#include "Windowsx.h"
#include <fcntl.h>
#include <stdio.h>
#include <io.h>
using namespace NCL;
using namespace Win32Code;

#define WINDOWCLASS "WindowClass"

Win32Window::Win32Window(const WindowInitialisation& winInitInfo) {
	forceQuit		= false;
	init			= false;
	mouseLeftWindow	= false;
	lockMouse		= false;
	showMouse		= true;
	active			= true;

	windowTitle = winInitInfo.windowTitle;
	fullScreen	= winInitInfo.fullScreen;

	size = Vector2i(winInitInfo.width, winInitInfo.height);
	defaultSize = size;

	position.x = fullScreen ? 0 : winInitInfo.windowPositionX;
	position.y = fullScreen ? 0 : winInitInfo.windowPositionY;

	windowInstance = GetModuleHandle(NULL);

	WNDCLASSEX windowClass;
	ZeroMemory(&windowClass, sizeof(WNDCLASSEX));

	if (!GetClassInfoEx(windowInstance, WINDOWCLASS, &windowClass))	{
		windowClass.cbSize		= sizeof(WNDCLASSEX);
	    windowClass.style		= CS_HREDRAW | CS_VREDRAW;
		windowClass.lpfnWndProc	= (WNDPROC)WindowProc;
		windowClass.hInstance	= windowInstance;
		windowClass.hCursor		= LoadCursor(NULL, IDC_ARROW);
		windowClass.hbrBackground = (HBRUSH)COLOR_WINDOW;
		windowClass.lpszClassName = WINDOWCLASS;

		if(!RegisterClassEx(&windowClass)) {
			std::cout << __FUNCTION__ << " Failed to register class!\n";
			return;
		}
	}

	windowHandle = CreateWindowEx(NULL,
		WINDOWCLASS,							// name of the window class
		winInitInfo.windowTitle.c_str(),		// title of the window
		fullScreen ? WS_POPUP|WS_VISIBLE : WS_OVERLAPPEDWINDOW|WS_POPUP|WS_VISIBLE|WS_SYSMENU|WS_MAXIMIZEBOX|WS_MINIMIZEBOX,    // window style
		winInitInfo.windowPositionX,			// x-position of the window
		winInitInfo.windowPositionY,			// y-position of the window
		fullScreen ? GetSystemMetrics(SM_CXSCREEN) : winInitInfo.width, // If fullscreen, use the size of the screen
		fullScreen ? GetSystemMetrics(SM_CYSCREEN) : winInitInfo.height,				// height of the window
        NULL,				// No parent window!
        NULL,				// No Menus!
		windowInstance,		// application handle
        NULL);				// No multiple windows!

 	if(!windowHandle) {
		std::cout << __FUNCTION__ << " Failed to create window!\n";
		return;
	}

	winMouse	= new Win32Mouse(windowHandle);
	winKeyboard = new Win32Keyboard(windowHandle);

	keyboard	= winKeyboard;
	mouse		= winMouse;
	winMouse->SetAbsolutePositionBounds(size);

	winMouse->Wake();
	winKeyboard->Wake();

	LockMouseToWindow(lockMouse);
	ShowOSPointer(showMouse);

	init		= true;
	maximised	= false;
	minimised	= false;
}

Win32Window::~Win32Window(void)	{
	init = false;
}

bool	Win32Window::InternalUpdate() {
	MSG		msg;

	POINT pt;
	GetCursorPos(&pt);
	ScreenToClient(windowHandle, &pt);
	winMouse->SetAbsolutePosition(Vector2((float)pt.x, (float)pt.y));

	while(PeekMessage(&msg,windowHandle,0,0,PM_REMOVE)) {
		CheckMessages(msg);
	}

	return !forceQuit;
}

void	Win32Window::UpdateTitle()	{
	SetWindowText(windowHandle, windowTitle.c_str());
}

void	Win32Window::SetFullScreen(bool fullScreen) {
	throw std::runtime_error("Not implemented"); // Need different logic for borderless fullscreen
}

void Win32Window::CheckMessages(MSG &msg)	{
	Win32Window* thisWindow = (Win32Window*)window;
	switch (msg.message)	{				// Is There A Message Waiting?
		case (WM_QUIT):
		case (WM_CLOSE): {					// Have We Received A Quit Message?
			thisWindow->ShowOSPointer(true);
			thisWindow->LockMouseToWindow(false);
			forceQuit = true;
		}break;
		case (WM_INPUT): {
			UINT dwSize;
			GetRawInputData((HRAWINPUT)msg.lParam, RID_INPUT, NULL, &dwSize,sizeof(RAWINPUTHEADER));

			BYTE* lpb = new BYTE[dwSize];

			GetRawInputData((HRAWINPUT)msg.lParam, RID_INPUT, lpb, &dwSize,sizeof(RAWINPUTHEADER));
			RAWINPUT* raw = (RAWINPUT*)lpb;

			if (keyboard && raw->header.dwType == RIM_TYPEKEYBOARD && active) {
				thisWindow->winKeyboard->UpdateRAW(raw);
			}

			if (mouse && raw->header.dwType == RIM_TYPEMOUSE && active) {
				thisWindow->winMouse->UpdateRAW(raw);
			}

			delete lpb;
		}break;

		default: {								// If Not, Deal With Window Messages
			TranslateMessage(&msg);				// Translate The Message
			DispatchMessage(&msg);				// Dispatch The Message
		}
	}
}

LRESULT CALLBACK Win32Window::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)	{
	Win32Window* thisWindow = (Win32Window*)window;

	bool applyResize = false;

    switch(message)	 {
        case(WM_DESTROY):	{
			thisWindow->ShowOSPointer(true);
			thisWindow->LockMouseToWindow(false);

			PostQuitMessage(0);
			thisWindow->forceQuit = true;
		} break;
		case (WM_ACTIVATE): {
			if(LOWORD(wParam) == WA_INACTIVE)	{
				thisWindow->active = false;
				ReleaseCapture();
				ClipCursor(NULL);
				if (thisWindow->init && mouse && keyboard) {
					thisWindow->winMouse->Sleep();
					thisWindow->winKeyboard->Sleep();
				}
			}
			else {
				thisWindow->active = true;
				if(thisWindow->init) {
					thisWindow->winMouse->Wake();
					thisWindow->winKeyboard->Wake();

					if(thisWindow->lockMouse) {
						thisWindow->LockMouseToWindow(true);
					}
				}
			}
			return 0;
		}break;
		case (WM_SYSCOMMAND): {
			if (wParam == SC_RESTORE) {
				if (thisWindow->minimised) {
					ShowWindow(thisWindow->windowHandle, SW_RESTORE);
					if (thisWindow->init) {
						thisWindow->winMouse->SetAbsolutePositionBounds(thisWindow->size);
						thisWindow->LockMouseToWindow(thisWindow->lockMouse);
					}
				}
			}
		}break;
		case (WM_LBUTTONDOWN): {
			if(thisWindow->init && thisWindow->lockMouse) {
				thisWindow->LockMouseToWindow(true);
			}
		}break;
		case (WM_MOUSEMOVE): {
			TRACKMOUSEEVENT tme;
			tme.cbSize = sizeof(TRACKMOUSEEVENT);
			tme.dwFlags = TME_LEAVE;
			tme.hwndTrack = thisWindow->windowHandle;
			TrackMouseEvent(&tme);

			if (mouse) {
				Win32Mouse*realMouse = (Win32Mouse*)mouse;
				realMouse->UpdateWindowPosition(
					Vector2i(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam))
				);
			}

			if(thisWindow->mouseLeftWindow) {
				thisWindow->mouseLeftWindow = false;
				if (thisWindow->init) {
					thisWindow->winMouse->Wake();
					thisWindow->winKeyboard->Wake();
				}
			}
		}break;
		case(WM_MOUSELEAVE):{
			thisWindow->mouseLeftWindow = true;
			if (thisWindow->init) {
				thisWindow->winMouse->Sleep();
				thisWindow->winKeyboard->Sleep();
			}
		}break;
		case(WM_SIZE): {
			float newX = (float)LOWORD(lParam);
			float newY = (float)HIWORD(lParam);
			if (newX > 0 && newY > 0 && (newX != thisWindow->size.x || newY != thisWindow->size.y)) {
				thisWindow->size.x = (float)LOWORD(lParam);
				thisWindow->size.y = (float)HIWORD(lParam);
			}
			if (wParam == SIZE_MINIMIZED) {
				thisWindow->minimised = true;
				//applyResize = true;
			}
			if (wParam == SIZE_MAXIMIZED) {
				thisWindow->minimised = false;
				thisWindow->maximised = true;
				applyResize = true;
			}
			else if (wParam == SIZE_RESTORED && thisWindow->maximised) {
				thisWindow->maximised = false;
				applyResize = true;
			}
			else if (wParam == SIZE_RESTORED && thisWindow->minimised) {
				thisWindow->minimised = false;
				applyResize = true;
			}
		}break;
		case(WM_ENTERSIZEMOVE): {
		}break;
		case(WM_EXITSIZEMOVE): {
			applyResize = true;
		}break;
    }

	if (applyResize) {
		if (thisWindow->eventHandler) {
			thisWindow->eventHandler(NCL::WindowEvent::Resize, thisWindow->size.x, thisWindow->size.y);
		}

		if (thisWindow->init) {
			thisWindow->winMouse->SetAbsolutePositionBounds(thisWindow->size);
			thisWindow->LockMouseToWindow(thisWindow->lockMouse);
		}
	}

    return DefWindowProc (hWnd, message, wParam, lParam);
}

void	Win32Window::LockMouseToWindow(bool lock)	{
	lockMouse = lock;
	if(lock) {
		RECT		windowRect;
		GetWindowRect (windowHandle, &windowRect);

		SetCapture(windowHandle);
		ClipCursor(&windowRect);
	}
	else{
		ReleaseCapture();
		ClipCursor(NULL);
	}
}

void	Win32Window::ShowOSPointer(bool show)	{
	if(show == showMouse) {
		return;	//ShowCursor does weird things, due to being a counter internally...
	}

	showMouse = show;
	if(show) {
		ShowCursor(1);
	}
	else{
		ShowCursor(0);
	}
}

void	Win32Window::SetConsolePosition(int x, int y)	{
	HWND consoleWindow = GetConsoleWindow();

	SetWindowPos(consoleWindow, 0, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

	SetActiveWindow(windowHandle);
}

void	Win32Window::SetWindowPosition(int x, int y) {
	SetWindowPos(windowHandle, 0, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

	SetActiveWindow(windowHandle);
}

void	Win32Window::ShowConsole(bool state)				{
	HWND consoleWindow = GetConsoleWindow();

	ShowWindow(consoleWindow, state ? SW_RESTORE : SW_HIDE);

	SetActiveWindow(windowHandle);
}

#endif //_WIN32
