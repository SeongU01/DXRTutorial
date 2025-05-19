#include "DXRSample.h"
#include "Application.h"
DXRSample::DXRSample(UINT width, UINT height, std::wstring name):
	_width{ width }, 
	_height{ height }, 
	_useWarpDevice{false},
	_title{name}
{
	_aspectRatio = static_cast<float>(width) / static_cast<float>(height);
}

_Use_decl_annotations_

void DXRSample::ParseCommnadLineArgs(WCHAR* argv[], int argc)
{
	for (int i = 1; i < argc; ++i)
	{
		if (_wcsnicmp(argv[i], L"-warp", wcslen(argv[i])) == 0 ||
			_wcsnicmp(argv[i], L"/warp", wcslen(argv[i])) == 0)
		{
			_useWarpDevice = true;
			_title = _title + L" (WARP)";
		}
	}
}

_Use_decl_annotations_

void DXRSample::GetHardwareAdapter(IDXGIFactory2* pFactory, IDXGIAdapter1** ppAdapter)
{
	ComPtr<IDXGIAdapter1> pAdapter;
	*ppAdapter = nullptr;
	for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != pFactory->EnumAdapters1(adapterIndex, &pAdapter); ++adapterIndex)
	{
		DXGI_ADAPTER_DESC1 desc;
		pAdapter->GetDesc1(&desc);
		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
		{
			continue;
		}
		
		if (SUCCEEDED(D3D12CreateDevice(pAdapter.Get(), D3D_FEATURE_LEVEL_12_1, _uuidof(ID3D12Device), nullptr)))
		{
			break;
		}
	}
	*ppAdapter = pAdapter.Detach();
}

void DXRSample::SetCustomWindowText(LPWSTR text) const
{
	const std::wstring windowText = _title + L": " + text;
	SetWindowText(Application::GetHwnd(), windowText.c_str());
}
