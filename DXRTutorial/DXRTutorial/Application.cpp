#include "Application.h"
#include "DXRSample.h"
HWND Application::_hwnd = nullptr;

int Application::Run(DXRSample* pSample, HINSTANCE hInstance, int cmdShow)
{
	int argc;
	LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	pSample->ParseCommnadLineArgs(argv, argc);
	LocalFree(argv);
	
	WNDCLASSEX windowClass = { 0 };
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = WindowProc;
	windowClass.hInstance = hInstance;
	windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
	windowClass.lpszClassName = L"DXSampleClass";
	RegisterClassEx(&windowClass);

	RECT windowRect = {
	0, 0, static_cast<LONG>(pSample->GetWidth()),
	static_cast<LONG>(pSample->GetHeight())
	};
	AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);
	_hwnd = CreateWindow(windowClass.lpszClassName, pSample->GetTitle(),
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
		windowRect.right - windowRect.left,
		windowRect.bottom - windowRect.top,
		NULL, // We have no parent window.
		NULL, // We aren't using menus.
		hInstance,pSample);
	pSample->Initialize();

	ShowWindow(_hwnd, cmdShow);

	MSG msg = {};

		while (true)
		{
			if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
			{
				if (WM_QUIT == msg.message)
					break;

				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else
			{
				pSample->Update();
				pSample->Render();
			}
		}
	pSample->Finalize();
	return static_cast<char>(msg.wParam);
}

LRESULT Application::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	DXRSample* pSample =
		reinterpret_cast<DXRSample*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

	switch (message)
	{
	case WM_CREATE:
	{
		// Save the DXSample* passed in to CreateWindow.
		LPCREATESTRUCT pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
		SetWindowLongPtr(hWnd, GWLP_USERDATA,
			reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
	}
	return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	// Handle any messages the switch statement didn't.
	return DefWindowProc(hWnd, message, wParam, lParam);
}
