#pragma once
#include "Headers.h"

class DXRSample
{
public:
	DXRSample(UINT width, UINT height, std::wstring name);
	virtual ~DXRSample() = default;

	virtual void Initialize() = 0;
	virtual void Update() = 0;
	virtual void Render() = 0;
	virtual void Flip() = 0;
	virtual void Finalize() = 0;
	
	virtual void OnKeyDown(UINT8) {}
	virtual void OnKeyUp(UINT8) {}
	UINT GetWidth() const { return _width; }
	UINT GetHeight() const { return _height; }
	const WCHAR* GetTitle()const { return _title.c_str(); }
	void ParseCommnadLineArgs(_In_reads_(argc)WCHAR* argv[],int argc);
protected:
	static void GetHardwareAdapter(_In_ IDXGIFactory2* pFactory, _Outptr_result_maybenull_ IDXGIAdapter1** ppAdapter);
	void SetCustomWindowText(LPWSTR text) const;

	UINT _width;
	UINT _height;
	float _aspectRatio;

	bool _useWarpDevice;

	std::vector<ComPtr<ID3D12Resource>> _uploadBuffers;
private:
	std::wstring _title;
};

