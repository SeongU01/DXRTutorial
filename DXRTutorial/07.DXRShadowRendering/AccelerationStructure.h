#pragma once

#include "Headers.h"

class AccelerationStructure
{
public:
	enum PrimitiveTiype
	{
		SPEHRE,
		CUBE,
		QUAD
	};
	struct ShapeResource
	{
		ComPtr<ID3D12Resource> vertexBuffer;
		UINT vertexCount;
		ComPtr<ID3D12Resource> indexBuffer;
		UINT indexCount;
	};
	static std::shared_ptr<ShapeResource> CreatePrimitive(
		ComPtr<ID3D12Device> pDevice,
		ComPtr<ID3D12GraphicsCommandList> pCmdList,
		PrimitiveTiype type
	);
	static AccelerationStructureBuffers CreatePlaneBottomLevelAS(
		ComPtr<ID3D12Device> pDevice,
		ComPtr<ID3D12GraphicsCommandList> pCmdList
	);
	static AccelerationStructureBuffers CreatePrimitiveBottomLevelAS(
		ComPtr<ID3D12Device> pDevice,
		ComPtr<ID3D12GraphicsCommandList> pCmdList
	);
	static void BuiltTopLevelAS(
		ComPtr<ID3D12Device> pDevice,
		ComPtr<ID3D12GraphicsCommandList>pCmdList,
		std::vector<AccelerationStructureBuffers>pBottomLevelAS,
		uint64_t& tlasSize,
		AccelerationStructureBuffers& buffers
	);
	// app resource
	static std::vector<std::shared_ptr<AccelerationStructure::ShapeResource>> _resources;
private:
	static AccelerationStructureBuffers CreateBottomLevelAS(
		ComPtr<ID3D12Device> pDevice,
		ComPtr<ID3D12GraphicsCommandList> pCmdList,
		std::shared_ptr<ShapeResource> resource
	);

};