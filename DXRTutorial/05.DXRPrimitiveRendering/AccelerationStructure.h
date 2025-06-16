#pragma once

#include "Headers.h"

class AccelerationStructure
{
public:
	struct ShapeResource
	{
		ComPtr<ID3D12Resource> vertexBuffer;
		UINT vertexCount;
		ComPtr<ID3D12Resource> indexBuffer;
		UINT indexCount;
	};
	static std::shared_ptr<ShapeResource> CreatePrimitive(
		ComPtr<ID3D12Device> pDevice,
		ComPtr<ID3D12GraphicsCommandList> pCmdList
	);
	static AccelerationStructureBuffers CreateBottomLevelAS(
		ComPtr<ID3D12Device> pDevice,
		ComPtr<ID3D12GraphicsCommandList> pCmdList,
		std::vector<std::shared_ptr<ShapeResource>>& resources
	);
	static void BuiltTopLevelAS(
		ComPtr<ID3D12Device> pDevice,
		ComPtr<ID3D12GraphicsCommandList>pCmdList,
		ComPtr<ID3D12Resource> pBottomLevelAS,
		uint64_t& tlasSize,
		AccelerationStructureBuffers& buffers
	);
};