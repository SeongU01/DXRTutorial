//***************************************************************************************
// d3dUtil.h by Frank Luna (C) 2015 All Rights Reserved.
//
// General helper code.
//***************************************************************************************

#pragma once

#include "DDSTextureLoader.h"
#include "MathHelper.h"
#include "d3dx12.h"
#include "functional"
#include <dxcapi.h>
#include <D3Dcompiler.h>
#include <DirectXCollision.h>
#include <DirectXColors.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <windows.h>
#include <wrl.h>
extern const int gNumFrameResources;
using namespace Microsoft::WRL;

inline void d3dSetDebugName(IDXGIObject* obj, const char* name)
{
  if (obj)
  {
    obj->SetPrivateData(WKPDID_D3DDebugObjectName, lstrlenA(name), name);
  }
}
inline void d3dSetDebugName(ID3D12Device* obj, const char* name)
{
  if (obj)
  {
    obj->SetPrivateData(WKPDID_D3DDebugObjectName, lstrlenA(name), name);
  }
}
inline void d3dSetDebugName(ID3D12DeviceChild* obj, const char* name)
{
  if (obj)
  {
    obj->SetPrivateData(WKPDID_D3DDebugObjectName, lstrlenA(name), name);
  }
}

inline std::wstring AnsiToWString(const std::string& str)
{
  WCHAR buffer[512];
  MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
  return std::wstring(buffer);
}

/*
#if defined(_DEBUG)
    #ifndef Assert
    #define Assert(x, description)                                  \
    {                                                               \
        static bool ignoreAssert = false;                           \
        if(!ignoreAssert && !(x))                                   \
        {                                                           \
            Debug::AssertResult result = Debug::ShowAssertDialog(   \
            (L#x), description, AnsiToWString(__FILE__), __LINE__); \
        if(result == Debug::AssertIgnore)                           \
        {                                                           \
            ignoreAssert = true;                                    \
        }                                                           \
                    else if(result == Debug::AssertBreak)           \
        {                                                           \
            __debugbreak();                                         \
        }                                                           \
        }                                                           \
    }
    #endif
#else
    #ifndef Assert
    #define Assert(x, description)
    #endif
#endif
    */

class d3dUtil
{
public:
    static UINT CalcConstantBufferByteSize(UINT byteSize)
    {
        // Constant buffers must be a multiple of the minimum hardware
        // allocation size (usually 256 bytes).  So round up to nearest
        // multiple of 256.  We do this by adding 255 and then masking off
        // the lower 2 bytes which store all bits < 256.
        // Example: Suppose byteSize = 300.
        // (300 + 255) & ~255
        // 555 & ~255
        // 0x022B & ~0x00ff
        // 0x022B & 0xff00
        // 0x0200
        // 512
        return (byteSize + 255) & ~255;
    }

    static Microsoft::WRL::ComPtr<ID3D12Resource> CreateDefaultBuffer(
        ID3D12Device* device, ID3D12GraphicsCommandList* cmdList,
        const void* initData, UINT64 byteSize,
        Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer);

    static void CreateBuffer(UINT size, const D3D12_RESOURCE_FLAGS flags, const D3D12_RESOURCE_STATES initState,
        ComPtr<ID3D12Resource>& buffer,ID3D12Device* device);
	static void CreateUploadBuffer(UINT size, const D3D12_RESOURCE_FLAGS flags, const D3D12_RESOURCE_STATES initState,
		ComPtr<ID3D12Resource>& buffer, ID3D12Device* device);

    static IDxcBlob* CompileShaderLibrary(LPCWSTR fileName);

    static HRESULT UpdateBuffer(const ComPtr<ID3D12Resource>& buffer, void* data, UINT size);
};