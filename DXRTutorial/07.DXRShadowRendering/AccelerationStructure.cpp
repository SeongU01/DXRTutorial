#include "AccelerationStructure.h"
#include "Primitive.h"
#include "DXRShadowApp.h"

std::vector<std::shared_ptr<AccelerationStructure::ShapeResource>> AccelerationStructure::_resources{};
std::shared_ptr<AccelerationStructure::ShapeResource> AccelerationStructure::CreatePrimitive(
	ComPtr<ID3D12Device> pDevice, ComPtr<ID3D12GraphicsCommandList> pCmdList, PrimitiveTiype type)
{
	Primitive::Shape shape;
	switch (type)
	{
	case SPEHRE:
		shape = Primitive::CreateSphere(2.f, 32);
		break;
	case CUBE:
		shape = Primitive::CreateCube(1.5f);
		break;
	case QUAD:
		shape = Primitive::CreateQuad(100);
		break;
	}

	auto primitiveData = std::make_shared<ShapeResource>();
	ComPtr<ID3D12Resource> vertexUpload;
	ComPtr<ID3D12Resource> indexUpload;


	primitiveData->vertexCount = static_cast<UINT>(shape.vertexData.size());
	primitiveData->vertexBuffer = d3dUtil::CreateDefaultBuffer(
		pDevice.Get(),
		pCmdList.Get(), shape.vertexData.data(),
		static_cast<UINT>(sizeof(VertexPositionNormalTangentTexture) * shape.vertexData.size()),
		vertexUpload
	);
	primitiveData->vertexBuffer->SetName(L"vertexBuffer");

	primitiveData->indexCount = static_cast<UINT>(shape.indexData.size());
	primitiveData->indexBuffer = d3dUtil::CreateDefaultBuffer(
		pDevice.Get(),
		pCmdList.Get(), shape.indexData.data(),
		static_cast<UINT>(sizeof(UINT) * shape.indexData.size()),
		indexUpload
	);
	primitiveData->indexBuffer->SetName(L"indexBuffer");

	vertexUpload->SetName(L"VeretexUpload");
	indexUpload->SetName(L"IndexUpload");

	DXRShadowApp::gUploadBuffers.push_back(vertexUpload);
	DXRShadowApp::gUploadBuffers.push_back(indexUpload);
	return primitiveData;
}

AccelerationStructureBuffers AccelerationStructure::CreatePlaneBottomLevelAS(ComPtr<ID3D12Device> pDevice, ComPtr<ID3D12GraphicsCommandList> pCmdList)
{
	auto planData = CreatePrimitive(pDevice, pCmdList, PrimitiveTiype::QUAD);
	if (!planData)
	{
		std::runtime_error("pland Data in null");
	}

	return CreateBottomLevelAS(pDevice, pCmdList, planData);
}

AccelerationStructureBuffers AccelerationStructure::CreatePrimitiveBottomLevelAS(ComPtr<ID3D12Device> pDevice, ComPtr<ID3D12GraphicsCommandList> pCmdList)
{
	auto primitive = CreatePrimitive(pDevice, pCmdList,PrimitiveTiype::SPEHRE);
	if (!primitive)
	{
		std::runtime_error("primitive Data is null");
	}

	return CreateBottomLevelAS(pDevice, pCmdList, primitive);
}

AccelerationStructureBuffers AccelerationStructure::CreateBottomLevelAS(
	ComPtr<ID3D12Device> pDevice,
	ComPtr<ID3D12GraphicsCommandList> pCmdList,
	std::shared_ptr<AccelerationStructure::ShapeResource> resource
)
{
	_resources.push_back(resource);
	std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geomDesc{};
	geomDesc.resize(1);

	geomDesc[0].Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
	geomDesc[0].Triangles.VertexBuffer = D3D12_GPU_VIRTUAL_ADDRESS_AND_STRIDE();
	geomDesc[0].Triangles.VertexBuffer.StartAddress = resource->vertexBuffer->GetGPUVirtualAddress();
	geomDesc[0].Triangles.VertexBuffer.StrideInBytes = sizeof(VertexPositionNormalTangentTexture);
	geomDesc[0].Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
	geomDesc[0].Triangles.VertexCount = resource->vertexCount;
	geomDesc[0].Triangles.IndexBuffer = resource->indexBuffer->GetGPUVirtualAddress();
	geomDesc[0].Triangles.IndexCount = resource->indexCount;
	geomDesc[0].Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
	geomDesc[0].Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
	

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

void AccelerationStructure::BuiltTopLevelAS(ComPtr<ID3D12Device> pDevice, ComPtr<ID3D12GraphicsCommandList> pCmdList, std::vector<AccelerationStructureBuffers> pBottomLevelAS, uint64_t& tlasSize, AccelerationStructureBuffers& buffers)
{
	static const int instances = 2;
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs{};
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
	inputs.NumDescs = instances;
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
		sizeof(D3D12_RAYTRACING_INSTANCE_DESC)*instances,
		D3D12_RESOURCE_FLAG_NONE,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		buffers.pInstanceDesc,
		pDevice.Get()
	);
	tlasSize = info.ResultDataMaxSizeInBytes;

	D3D12_RAYTRACING_INSTANCE_DESC* instanceDescs;
	buffers.pInstanceDesc->Map(0, nullptr, reinterpret_cast<void**>(&instanceDescs));
	ZeroMemory(instanceDescs, sizeof(D3D12_RAYTRACING_INSTANCE_DESC));
	// The transformation matrices for the instances
	Matrix transformation[instances];
	transformation[0] = Matrix::CreateTranslation(Vector3(0,-1,0));
	transformation[1] = Matrix::Identity;

	instanceDescs[0].InstanceID = 0; // This value will be exposed to the shader via InstanceID()
	instanceDescs[0].InstanceContributionToHitGroupIndex = 0;
	// This is the offset inside the shader-table. We only have a single geometry, so the offset 0
	instanceDescs[0].Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
	Matrix mat = transformation[0].Transpose(); 
	memcpy(instanceDescs[0].Transform, &mat, sizeof(instanceDescs[0].Transform));
	instanceDescs[0].AccelerationStructure = pBottomLevelAS[0].pResult->GetGPUVirtualAddress();
	instanceDescs[0].InstanceMask = 0xFF;

	for (int i = 1; i < instances; i++)
	{
		instanceDescs[i].InstanceID = i;
		instanceDescs[i].InstanceContributionToHitGroupIndex = 0;
		instanceDescs[i].Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
		mat = transformation[i].Transpose();
		memcpy(instanceDescs[i].Transform, &mat, sizeof(instanceDescs[i].Transform));
		instanceDescs[i].AccelerationStructure = pBottomLevelAS[i].pResult->GetGPUVirtualAddress();
		instanceDescs[i].InstanceMask = 0xFF;
	}
	buffers.pInstanceDesc->Unmap(0, nullptr);

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC asDesc{};
	asDesc.Inputs = inputs;
	asDesc.Inputs.InstanceDescs = buffers.pInstanceDesc->GetGPUVirtualAddress();
	asDesc.DestAccelerationStructureData = buffers.pResult->GetGPUVirtualAddress();
	asDesc.ScratchAccelerationStructureData = buffers.pScratch->GetGPUVirtualAddress();

	ComPtr<ID3D12GraphicsCommandList4> cmdList4;
	FAILED_CHECK_BREAK(pCmdList->QueryInterface(cmdList4.GetAddressOf()));
	cmdList4->BuildRaytracingAccelerationStructure(&asDesc, 0, nullptr);

	auto br = CD3DX12_RESOURCE_BARRIER::UAV(buffers.pResult.Get());
	pCmdList->ResourceBarrier(1, &br);
}