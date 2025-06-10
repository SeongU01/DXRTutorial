#pragma once
#include "DXRSample.h"
#include "GameTimer.h"
class Application
{
public:
	static int Run(DXRSample* pSample, HINSTANCE hInstance, int cmdShow);
	static HWND GetHwnd() { return _hwnd; }
	static void CalculateFrameRate();
protected:
	static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
private:
	static HWND _hwnd;
	static GameTimer* _pTimer;
};

