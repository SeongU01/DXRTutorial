#pragma once
#include "Headers.h"
#include "D3DUtil.h"
class AccelerationStructure
{
public:
	static ComPtr<ID3D12Resource> CreateTriangleVB(ComPtr<ID3D12Device> pDevice);
	static ComPtr<ID3D12Resource> CreatePlaneVB(ComPtr<ID3D12Device> pDevice);
	static AccelerationStructureBuffers CreateBottomLevelAS(
		ComPtr<ID3D12Device> pDevice, ComPtr<ID3D12GraphicsCommandList> cmdList,
		ComPtr<ID3D12Resource> pVB[], const uint32_t vertexCount[], const uint32_t geometryCount
	);
	static void BuildTopLevelAS(
		ComPtr<ID3D12Device> pDevice,
		ComPtr<ID3D12GraphicsCommandList> cmdList,
		ComPtr<ID3D12Resource> pBottomLevelAS[2],
		uint64_t& tlasSize,
		float rotation,
		bool update,
		AccelerationStructureBuffers& buffers
	);
};

