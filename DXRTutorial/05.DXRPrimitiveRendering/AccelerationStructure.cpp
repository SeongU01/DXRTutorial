#include "AccelerationStructure.h"
#include "Primitive.h"
#include "DXRPrimitiveApp.h"

std::shared_ptr<AccelerationStructure::ShapeResource>AccelerationStructure::CreatePrimitive(
	ComPtr<ID3D12Device> pDevice,
	ComPtr<ID3D12GraphicsCommandList> pCmdList
)
{
	Primitive::Shape shape = Primitive::CreateSphere(2.f, 32);

	auto resource = std::make_shared<ShapeResource>();

	ComPtr<ID3D12Resource> vertexUpload;
	ComPtr<ID3D12Resource> indexUpload;


	resource->vertexCount = static_cast<UINT>(shape.vertexData.size());
	resource->vertexBuffer = d3dUtil::CreateDefaultBuffer(
		pDevice.Get(),
		pCmdList.Get(), shape.vertexData.data(),
		static_cast<UINT>(sizeof(VertexPositionNormalTangentTexture) * shape.vertexData.size()),
		vertexUpload
	);

	resource->indexCount = static_cast<UINT>(shape.indexData.size());
	resource->indexBuffer = d3dUtil::CreateDefaultBuffer(
		pDevice.Get(),
		pCmdList.Get(), shape.indexData.data(),
		static_cast<UINT>(sizeof(UINT) * shape.indexData.size()),
		indexUpload
	);
	vertexUpload->SetName(L"VeretexUpload");
	indexUpload->SetName(L"IndexUpload");

	DXRPrimitiveApp::gUploadBuffers.push_back(vertexUpload);
	DXRPrimitiveApp::gUploadBuffers.push_back(indexUpload);
	return resource;
}

AccelerationStructureBuffers AccelerationStructure::CreateBottomLevelAS(
	ComPtr<ID3D12Device> pDevice,
	ComPtr<ID3D12GraphicsCommandList> pCmdList,
	std::vector<std::shared_ptr<ShapeResource>>& resources
)
{
	std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geomDesc{};
	geomDesc.resize(1);

	auto shape = CreatePrimitive(pDevice, pCmdList);
	resources.push_back(shape);
	for (auto& desc : geomDesc)
	{
		desc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
		desc.Triangles.VertexBuffer = D3D12_GPU_VIRTUAL_ADDRESS_AND_STRIDE();
		desc.Triangles.VertexBuffer.StartAddress = shape->vertexBuffer->GetGPUVirtualAddress();
		desc.Triangles.VertexBuffer.StrideInBytes = sizeof(VertexPositionNormalTangentTexture);
		desc.Triangles.VertexCount = shape->vertexCount;
		desc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
		desc.Triangles.IndexBuffer = shape->indexBuffer->GetGPUVirtualAddress();
		desc.Triangles.IndexCount = shape->indexCount;
		desc.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
		desc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
	}

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs{};
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
	inputs.NumDescs = geomDesc.size();
	inputs.pGeometryDescs = geomDesc.data();
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;

	ComPtr<ID3D12Device5> device5;
	pDevice->QueryInterface(IID_PPV_ARGS(device5.GetAddressOf()));

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info;
	device5->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);

	AccelerationStructureBuffers buffers;
	d3dUtil::CreateBuffer(
		info.ScratchDataSizeInBytes,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		buffers.pScratch,
		pDevice.Get()
	);
	d3dUtil::CreateBuffer(
		info.ResultDataMaxSizeInBytes,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
		buffers.pResult,
		pDevice.Get()
	);

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC asDesc{};
	asDesc.Inputs = inputs;
	asDesc.DestAccelerationStructureData = buffers.pResult->GetGPUVirtualAddress();
	asDesc.ScratchAccelerationStructureData = buffers.pScratch->GetGPUVirtualAddress();

	ComPtr<ID3D12GraphicsCommandList4> cmdList4;
	pCmdList->QueryInterface(IID_PPV_ARGS(cmdList4.GetAddressOf()));
	
	cmdList4->BuildRaytracingAccelerationStructure(&asDesc, 0, nullptr);

	auto br = CD3DX12_RESOURCE_BARRIER::UAV(buffers.pResult.Get());
	pCmdList->ResourceBarrier(1, &br);
	return buffers;
}

void AccelerationStructure::BuiltTopLevelAS(ComPtr<ID3D12Device> pDevice, ComPtr<ID3D12GraphicsCommandList> pCmdList, ComPtr<ID3D12Resource> pBottomLevelAS, uint64_t& tlasSize, AccelerationStructureBuffers& buffers)
{
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs{};
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
	inputs.NumDescs = 1;
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

	ComPtr<ID3D12Device5> device5;
	pDevice->QueryInterface(IID_PPV_ARGS(device5.GetAddressOf()));
	
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info{};
	device5->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);
	
	d3dUtil::CreateBuffer(
		info.ScratchDataSizeInBytes,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		buffers.pScratch,
		pDevice.Get()
	);
	d3dUtil::CreateBuffer(
		info.ResultDataMaxSizeInBytes,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
		buffers.pResult,
		pDevice.Get()
	);
	d3dUtil::CreateUploadBuffer(
		sizeof(D3D12_RAYTRACING_INSTANCE_DESC),
		D3D12_RESOURCE_FLAG_NONE,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		buffers.pInstanceDesc,
		pDevice.Get()
	);
	tlasSize = info.ResultDataMaxSizeInBytes;

	D3D12_RAYTRACING_INSTANCE_DESC* instanceDesc;
	buffers.pInstanceDesc->Map(0, nullptr, reinterpret_cast<void**>(&instanceDesc));
	ZeroMemory(instanceDesc, sizeof(D3D12_RAYTRACING_INSTANCE_DESC));
	instanceDesc[0].InstanceID = 0;
	instanceDesc[0].Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
	instanceDesc[0].InstanceContributionToHitGroupIndex = 0;

	Matrix transform = Matrix::Identity;
	memcpy(instanceDesc[0].Transform, &transform, sizeof(instanceDesc[0].Transform));
	instanceDesc[0].AccelerationStructure = pBottomLevelAS->GetGPUVirtualAddress();
	instanceDesc[0].InstanceMask = 0xFF;
	buffers.pInstanceDesc->Unmap(0, nullptr);

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC asDesc{};
	asDesc.Inputs = inputs;
	asDesc.Inputs.InstanceDescs = buffers.pInstanceDesc->GetGPUVirtualAddress();
	asDesc.DestAccelerationStructureData = buffers.pResult->GetGPUVirtualAddress();
	asDesc.ScratchAccelerationStructureData = buffers.pScratch->GetGPUVirtualAddress();

	ComPtr<ID3D12GraphicsCommandList4> cmdList4;
	pCmdList->QueryInterface(IID_PPV_ARGS(cmdList4.GetAddressOf()));

	cmdList4->BuildRaytracingAccelerationStructure(&asDesc, 0, nullptr);

	auto br = CD3DX12_RESOURCE_BARRIER::UAV(buffers.pResult.Get());
	pCmdList->ResourceBarrier(1, &br);
}