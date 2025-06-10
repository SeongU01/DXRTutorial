#include "Headers.h"
#include "Application.h"
#include "DXRTriangleInstanceAnimationApp.h"

_Use_decl_annotations_

int WINAPI WinMain(const HINSTANCE hInstance, HINSTANCE, LPSTR, const int nShowCmd)
{
	DXRTriangleInstanceAnimationApp sample(1280, 720, L"D3D12 Hello Triangle - Raytracing");
	return Application::Run(&sample, hInstance, nShowCmd);
}
