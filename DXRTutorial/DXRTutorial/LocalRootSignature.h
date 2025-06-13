#pragma once
#include "Headers.h"

struct LocalRootSignature
{
	LocalRootSignature(const ComPtr<ID3D12Device> device, const D3D12_ROOT_SIGNATURE_DESC& desc)
	{
		rootSingnature = d3dUtil::CreateRoogSignature(device.Get(), desc);
		pInterface = rootSingnature.Get();
		subObject.pDesc = &pInterface;
		subObject.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
	}
	ComPtr<ID3D12RootSignature> rootSingnature;
	ID3D12RootSignature* pInterface = nullptr;
	D3D12_STATE_SUBOBJECT subObject{};
};