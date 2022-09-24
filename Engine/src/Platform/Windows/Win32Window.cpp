#include "stdafx.h"
#include "Win32Window.h"

#include "Platform/DirectX/DirectX11Context.h"

#include "Events/ApplicationEvent.h"
#include "Events/KeyEvent.h"
#include "Events/MouseEvent.h"

#include "Core/Application.h"
#include "Core/Settings.h"
#include "Core/Input.h"
#include "Core/MouseButtonCodes.h"
#include <windowsx.h>

#include "Win32KeyCodes.h"

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK Win32Window::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
		return true;

	WindowData* data = (WindowData*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	if (data == nullptr)
		return DefWindowProc(hWnd, message, wParam, lParam);

	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_SIZE:
	{
		unsigned int width = LOWORD(lParam);
		unsigned int height = HIWORD(lParam);

		data->width = width;
		data->height = height;

		WindowResizeEvent event(width, height);
		data->eventCallback(event);

		Application::GetWindow().GetContext()->ResizeBuffers(width, height);

		if ((data->maximized && wParam != SIZE_MAXIMIZED) || (!data->maximized && wParam != SIZE_RESTORED))
		{
			switch (wParam)
			{
			case SIZE_MAXIMIZED:
			{
				data->maximized = true;

				WindowMaximizedEvent event(data->maximized);
				data->eventCallback(event);
				break;
			}
			case SIZE_RESTORED:
			{
				data->maximized = false;

				WindowMaximizedEvent event(data->maximized);
				data->eventCallback(event);
				break;
			}
			default:
				break;
			}
		}
		break;
	}
	case WM_CLOSE:
	{
		WindowCloseEvent event;
		data->eventCallback(event);

		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	case WM_SETFOCUS:
	{
		WindowFocusEvent event;
		data->eventCallback(event);
		break;
	}
	case WM_KILLFOCUS:
	{
		WindowFocusLostEvent event;
		data->eventCallback(event);
		break;
	}
	case WM_MOVE:
	{
		int posX = LOWORD(lParam);
		int posY = HIWORD(lParam);

		data->posX = posX;
		data->posY = posY;

		WindowMoveEvent event(posX, posY);
		data->eventCallback(event);

		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	case WM_MOUSEMOVE:
	{
		int xPos = GET_X_LPARAM(lParam);
		int yPos = GET_Y_LPARAM(lParam);

		MouseMotionEvent event((float)xPos, (float)yPos);
		data->eventCallback(event);
		break;
	}
	case WM_LBUTTONDOWN:
	{
		MouseButtonPressedEvent event(MOUSE_BUTTON_LEFT);
		data->eventCallback(event);
		break;
	}
	case WM_LBUTTONUP:
	{
		MouseButtonReleasedEvent event(MOUSE_BUTTON_LEFT);
		data->eventCallback(event);
		break;
	}
	case WM_RBUTTONDOWN:
	{
		MouseButtonPressedEvent event(MOUSE_BUTTON_RIGHT);
		data->eventCallback(event);
		break;
	}
	case WM_RBUTTONUP:
	{
		MouseButtonReleasedEvent event(MOUSE_BUTTON_RIGHT);
		data->eventCallback(event);
		break;
	}
	case WM_MBUTTONDOWN:
	{
		MouseButtonPressedEvent event(MOUSE_BUTTON_MIDDLE);
		data->eventCallback(event);
		break;
	}
	case WM_MBUTTONUP:
	{
		MouseButtonReleasedEvent event(MOUSE_BUTTON_MIDDLE);
		data->eventCallback(event);
		break;
	}
	case WM_MOUSEHWHEEL:
	{
		int delta = std::clamp((int)GET_WHEEL_DELTA_WPARAM(wParam), -1, 1);

		MouseWheelEvent event((float)delta, 0);
		data->eventCallback(event);
		break;
	}
	case WM_MOUSEWHEEL:
	{
		int delta = std::clamp((int)GET_WHEEL_DELTA_WPARAM(wParam), -1, 1);

		MouseWheelEvent event(0, (float)delta);
		data->eventCallback(event);
		break;
	}
	case WM_KEYDOWN:
	{
		KeyPressedEvent event(VirtualCodesToKeyCodes((int)wParam), 0);
		data->eventCallback(event);
		break;
	}
	case WM_KEYUP:
	{
		KeyReleasedEvent event(VirtualCodesToKeyCodes((int)wParam));
		data->eventCallback(event);
		break;
	}
	case WM_CHAR:
	{
		KeyTypedEvent event((int)wParam);
		data->eventCallback(event);
		break;
	}

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}

Win32Window::Win32Window(const WindowProps& props)
{
	PROFILE_FUNCTION();
	if (FAILED(Init(props)))
	{
		CORE_ASSERT(false, "Win32 Window failed to initialise");
	}
}

Win32Window::~Win32Window()
{
	Shutdown();
}

void Win32Window::OnUpdate()
{
	MessageLoop();
	m_Context->SwapBuffers();
}

void Win32Window::SetVSync(bool enabled)
{
	m_Data.vSync = enabled;
	Settings::SetBool("Display", "V-Sync", enabled);
	m_Context->SetSwapInterval(enabled ? 1 : 0);
}

bool Win32Window::IsVSync() const
{
	return m_Data.vSync;
}

void Win32Window::SetIcon(const std::filesystem::path& path)
{
	//TODO: change icon
}

void Win32Window::SetCursor(Cursors cursorType)
{
	//TODO: set cursor
}

void Win32Window::SetTitle(const char* title)
{
	SetWindowTextA(m_Window, title);
}

void Win32Window::DisableCursor()
{
	//TODO: disable cursor
}

void Win32Window::EnableCursor()
{
	//TODO: enable cursor
}

void Win32Window::SetCursorPosition(double xpos, double ypos)
{
}

void Win32Window::SetWindowMode(WindowMode mode, unsigned int width, unsigned int height)
{
	//TODO: change window mode
}

void Win32Window::MaximizeWindow()
{
	ShowWindow(m_Window, SW_MAXIMIZE);
}

void Win32Window::RestoreWindow()
{
	ShowWindow(m_Window, SW_RESTORE);
}

HRESULT Win32Window::Init(const WindowProps& props)
{
	Input::SetInput(RendererAPI::GetAPI());
	m_Instance = GetModuleHandle(0);

	m_Data.title = props.title;
	m_Data.width = Settings::GetInt("Display", "Window_Width");
	m_Data.height = Settings::GetInt("Display", "Window_Height");
	m_Data.posX = Settings::GetInt("Display", "Window_Position_X");
	m_Data.posY = Settings::GetInt("Display", "Window_Position_Y");
	m_Data.mode = WindowMode::WINDOWED;

	//Register class
	WNDCLASSEX wcex;
	ZeroMemory(&wcex, sizeof(wcex));
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW | CS_CLASSDC;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = m_Instance;
	wcex.hIcon = nullptr;
	//wcex.hIcon = LoadIcon(m_Instance, (LPCTSTR)IDI_ICON1)//TODO:: generate the resource file and include to here to get the Icon.ico
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW); // TODO: make a cursor management class to allow the switching of the cursor from the default arrow
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = nullptr;
	wcex.lpszClassName = L"EngineWindowClass";
	wcex.hIconSm = nullptr;
	//wcex.hIconSm = LoadIcon(wcex.hInstance, (LPSTR)IDI_ICON1);
	if (!RegisterClassEx(&wcex))
		return E_FAIL;

	int len;
	int slength = (int)m_Data.title.length() + 1;
	len = MultiByteToWideChar(CP_ACP, 0, m_Data.title.c_str(), slength, 0, 0);
	wchar_t* buf = new wchar_t[len];
	MultiByteToWideChar(CP_ACP, 0, m_Data.title.c_str(), slength, buf, len);
	std::wstring wTitle(buf);
	delete[] buf;

	//create the window
	RECT rc = { 0,0, (LONG)m_Data.width, (LONG)m_Data.height };
	AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
	m_Window = CreateWindow(wcex.lpszClassName, wTitle.c_str(), WS_OVERLAPPEDWINDOW,
		m_Data.posX, m_Data.posY, rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, m_Instance, nullptr);

	if (!m_Window)
		return E_FAIL;


	ENGINE_INFO("Creating Window {0} {1} {2}", m_Data.title, m_Data.width, m_Data.height);

	ShowWindow(m_Window, SW_SHOWNORMAL);
	SetWindowLongPtr(m_Window, GWLP_USERDATA, (LONG_PTR)&m_Data);

	++s_Win32WindowCount;

	m_Context = CreateRef<DirectX11Context>(m_Window);

	m_Context->Init();

	return S_OK;
}

void Win32Window::Shutdown()
{
	m_Context.reset();
	--s_Win32WindowCount;
}

void Win32Window::MessageLoop()
{
	MSG msg = { 0 };

	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		if (msg.message == WM_QUIT)
		{
			return;
		}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
	}
}
