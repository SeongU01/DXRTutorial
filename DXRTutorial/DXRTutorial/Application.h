#pragma once
#include "DXRSample.h"

class Application
{
public:
	static int Run(DXRSample* pSample, HINSTANCE hInstance, int cmdShow);
	static HWND GetHwnd() { return _hwnd; }
protected:
	static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
private:
	static HWND _hwnd;
};

