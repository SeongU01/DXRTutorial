#include "Application.h"
#include "DXRSample.h"
HWND Application::_hwnd = nullptr;
GameTimer* Application::_pTimer = nullptr;
int Application::Run(DXRSample* pSample, HINSTANCE hInstance, int cmdShow)
{
	_pTimer = GameTimer::GetInstance();
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
	_pTimer->Reset();
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
				_pTimer->Tick();
				CalculateFrameRate();
				float delta = _pTimer->DeltaTime();
				pSample->Update(delta);
				pSample->Render();
				pSample->Flip();
			}
		}
	pSample->Finalize();
	delete _pTimer;
	return static_cast<char>(msg.wParam);
}

void Application::CalculateFrameRate()
{
	static int frameCnt = 0;
	static float elapsed = 0.f;
	frameCnt++;
	if (GameTimer::GetInstance()->TotalTime() - elapsed >= 1.f)
	{
		float fps = (float)frameCnt;
		float mspf = 1000.f / fps;
		std::wstring fpsStr = std::to_wstring(fps);
		std::wstring mspfStr = std::to_wstring(mspf);
		std::wstring windowText =
			L"    fps: " + fpsStr + L"   mspf: " + mspfStr;

		SetWindowText(_hwnd,
			windowText.c_str());

		// Reset for next average.
		frameCnt = 0;
		elapsed += 1.0f;
	}
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
