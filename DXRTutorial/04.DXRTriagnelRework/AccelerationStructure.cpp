#include "AccelerationStructure.h"

ComPtr<ID3D12Resource> AccelerationStructure::CreateTriangleVB(ComPtr<ID3D12Device> pDevice)
{
	Vector3 vertices[] =
	{
		Vector3(0,1,0),
		Vector3(0.866f,-0.5f,0),
		Vector3(-0.866f,-0.5f,0)
	};
	
	ComPtr<ID3D12Resource> buffer;
	d3dUtil::CreateUploadBuffer(
		sizeof(vertices), D3D12_RESOURCE_FLAG_NONE,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		buffer,pDevice.Get()
	);
	d3dUtil::UpdateBuffer(buffer, vertices, sizeof(vertices));

	return buffer;
}

ComPtr<ID3D12Resource> AccelerationStructure::CreatePlaneVB(ComPtr<ID3D12Device> pDevice)
{
	Vector3 vertices[] =
	{
		Vector3(-100, -1, -2),
		Vector3(100, -1, 100),
		Vector3(-100, -1, 100),

		Vector3(-100, -1, -2),
		Vector3(100, -1, -2),
		Vector3(100, -1, 100),
	};

	ComPtr<ID3D12Resource> buffer;
	d3dUtil::CreateUploadBuffer(
		sizeof(vertices), D3D12_RESOURCE_FLAG_NONE,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		buffer, pDevice.Get()
	);
	d3dUtil::UpdateBuffer(buffer, vertices, sizeof(vertices));

	return buffer;
}

AccelerationStructureBuffers AccelerationStructure::CreateBottomLevelAS(
	ComPtr<ID3D12Device> pDevice, ComPtr<ID3D12GraphicsCommandList> pCmdList,
	ComPtr<ID3D12Resource> pVB[], const uint32_t vertexCount[], const uint32_t geometryCount)
{
	std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geomDesc;
	geomDesc.resize(geometryCount);

	for (uint32_t i = 0; i < geometryCount; i++)
	{
		geomDesc[i].Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
		geomDesc[i].Triangles.VertexBuffer.StartAddress = pVB[i]->GetGPUVirtualAddress();
		geomDesc[i].Triangles.VertexBuffer.StrideInBytes = sizeof(Vector3);
		geomDesc[i].Triangles.VertexCount = vertexCount[i];
		geomDesc[i].Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
		geomDesc[i].Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
	}

	// Get the size requirements for the scratch and AS buffers
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs;
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
	inputs.NumDescs = geometryCount;
	inputs.pGeometryDescs = geomDesc.data();
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	ComPtr<ID3D12Device5> device5;
	FAILED_CHECK_BREAK(pDevice->QueryInterface(IID_PPV_ARGS(device5.GetAddressOf())));
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info;
	device5->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);

	// Create the buffers. They need to support UAV, and since we are going to immediately use them, we create them with an unordered-access state
	AccelerationStructureBuffers buffers;
	d3dUtil::CreateBuffer(
		info.ScratchDataSizeInBytes,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_COMMON,
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
	// Create the bottom-level AS
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC asDesc = {};
	asDesc.Inputs = inputs;
	asDesc.DestAccelerationStructureData = buffers.pResult->GetGPUVirtualAddress();
	asDesc.ScratchAccelerationStructureData = buffers.pScratch->GetGPUVirtualAddress();

	ComPtr<ID3D12GraphicsCommandList4> cmdList4;
	FAILED_CHECK_BREAK(pCmdList->QueryInterface(IID_PPV_ARGS(cmdList4.GetAddressOf())));

	cmdList4->BuildRaytracingAccelerationStructure(&asDesc, 0, nullptr);

	// We need to insert a UAV barrier before using the acceleration structures in a raytracing operation
	D3D12_RESOURCE_BARRIER uavBarrier = {};
	uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	uavBarrier.UAV.pResource = buffers.pResult.Get();
	pCmdList->ResourceBarrier(1, &uavBarrier);

	return buffers;
}



void AccelerationStructure::BuildTopLevelAS(ComPtr<ID3D12Device> pDevice,
	ComPtr<ID3D12GraphicsCommandList> pCmdList,
	ComPtr<ID3D12Resource> pBottomLevelAS[2],
	uint64_t& tlasSize, const float rotation, const bool update,
	AccelerationStructureBuffers& buffers)
{
	// First, get the size of the TLAS buffers and create them
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
	inputs.NumDescs = 4;
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	ComPtr<ID3D12Device5> device5;
	FAILED_CHECK_BREAK(pDevice->QueryInterface(IID_PPV_ARGS(device5.GetAddressOf())));
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info;
	device5->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);

	if (update)
	{
		// If this a request for an update, then the TLAS was already used in a DispatchRay() call. We need a UAV barrier to make sure the read operation ends before updating the buffer
		D3D12_RESOURCE_BARRIER uavBarrier = {};
		uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		uavBarrier.UAV.pResource = buffers.pResult.Get();
		pCmdList->ResourceBarrier(1, &uavBarrier);
	}
	else
	{
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
			sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * 4,
			D3D12_RESOURCE_FLAG_NONE,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			buffers.pInstanceDesc,
			pDevice.Get()
		);
		tlasSize = info.ResultDataMaxSizeInBytes;
	}

	// Map the instance desc buffer
	D3D12_RAYTRACING_INSTANCE_DESC* instanceDescs;
	buffers.pInstanceDesc->Map(0, nullptr, reinterpret_cast<void**>(&instanceDescs));
	ZeroMemory(instanceDescs, sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * 3);

	Matrix transformation[4];
	const Matrix rotationMat = Matrix::CreateRotationY(rotation);
	transformation[0] = Matrix::Identity;
	transformation[1] = rotationMat * Matrix::CreateTranslation(Vector3(-2, 0, 0));
	transformation[2] = rotationMat * Matrix::CreateTranslation(Vector3(2, 0, 0));
	transformation[3] = rotationMat * Matrix::CreateTranslation(Vector3(0, 2, 0));
	// The InstanceContributionToHitGroupIndex is set based on the shader-table layout specified in createShaderTable()
	// Create the desc for the triangle/plane instance
	instanceDescs[0].InstanceID = 0;
	instanceDescs[0].InstanceContributionToHitGroupIndex = 0;
	instanceDescs[0].Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
	memcpy(instanceDescs[0].Transform, &transformation[0], sizeof(instanceDescs[0].Transform));
	instanceDescs[0].AccelerationStructure = pBottomLevelAS[0]->GetGPUVirtualAddress();
	instanceDescs[0].InstanceMask = 0xFF;

	for (uint32_t i = 1; i < 4; i++)
	{
		instanceDescs[i].InstanceID = i;
		instanceDescs[i].InstanceContributionToHitGroupIndex = (i * 2) + 2;
		instanceDescs[i].Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
		Matrix m = transformation[i].Transpose();
		memcpy(instanceDescs[i].Transform, &m, sizeof(instanceDescs[i].Transform));
		instanceDescs[i].AccelerationStructure = pBottomLevelAS[1]->GetGPUVirtualAddress();
		instanceDescs[i].InstanceMask = 0xFF;
	}

	// Unmap
	buffers.pInstanceDesc->Unmap(0, nullptr);

	// Create the TLAS
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC asDesc = {};
	asDesc.Inputs = inputs;
	asDesc.Inputs.InstanceDescs = buffers.pInstanceDesc->GetGPUVirtualAddress();
	asDesc.DestAccelerationStructureData = buffers.pResult->GetGPUVirtualAddress();
	asDesc.ScratchAccelerationStructureData = buffers.pScratch->GetGPUVirtualAddress();

	// If this is an update operation, set the source buffer and the perform_update flag
	if (update)
	{
		asDesc.Inputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
		asDesc.SourceAccelerationStructureData = buffers.pResult->GetGPUVirtualAddress();
	}
	ComPtr<ID3D12GraphicsCommandList4> commandList4;
	FAILED_CHECK_BREAK(pCmdList->QueryInterface(IID_PPV_ARGS(commandList4.GetAddressOf())));
	commandList4->BuildRaytracingAccelerationStructure(&asDesc, 0, nullptr);

	// We need to insert a UAV barrier before using the acceleration structures in a raytracing operation
	D3D12_RESOURCE_BARRIER uavBarrier = {};
	uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	uavBarrier.UAV.pResource = buffers.pResult.Get();
	pCmdList->ResourceBarrier(1, &uavBarrier);
}

