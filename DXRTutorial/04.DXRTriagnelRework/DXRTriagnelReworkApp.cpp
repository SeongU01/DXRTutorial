#include "DXRTriagnelReworkApp.h"
#include "Application.h"
#include "AccelerationStructure.h"
#include "RTPipeline.h"
#include "HitProgram.h"
#include "LocalRootSignature.h"
#include "ExportAssociation.h"
#include "ShaderConfig.h"
#include "PipelineConfig.h"
#include "GlobalRootSignature.h"
DXRTriagnelReworkApp::DXRTriagnelReworkApp(UINT width, UINT height, std::wstring name)
	:DXRSample{ width,height,name }
{
	_viewPort.TopLeftX = 0.f;
	_viewPort.TopLeftY = 0.f;
	_viewPort.Width = width;
	_viewPort.Height = height;
	_viewPort.MinDepth = 0.f;
	_viewPort.MaxDepth = 1.f;
	_scissorRect.top = 0;
	_scissorRect.left = 0;
	_scissorRect.right = width;
	_scissorRect.bottom = height;
}

void DXRTriagnelReworkApp::Initialize()
{
	LoadPipeline();
	_commandList->Reset(_commandAlloc.Get(), nullptr);
	CreateAccelerationStructures();
	CreateRTPipelineState();
	CreateShaderResource();
	CreateConstantBuffer();
	CreateShaderTable();
}
static float rotate = 0;
void DXRTriagnelReworkApp::Update(const float& dt)
{
}

void DXRTriagnelReworkApp::Render()
{
	_commandAlloc->Reset();
	_commandList->Reset(_commandAlloc.Get(), nullptr);
	const uint32_t rtvIndex = BeginFrame();
	AccelerationStructure::BuildTopLevelAS(
		_device, _commandList, _bottomLevelAS, _tlasSize, _rotation,
		true, _topLevelBuffers
	);
	_rotation += 0.005f;
	auto br = CD3DX12_RESOURCE_BARRIER::Transition(
		_outputResource.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	_commandList->ResourceBarrier(1, &br);


	D3D12_DISPATCH_RAYS_DESC raytraceDesc{};
	raytraceDesc.Width = _width;
	raytraceDesc.Height = _height;
	raytraceDesc.Depth = 1;


	//raygen
	raytraceDesc.RayGenerationShaderRecord.StartAddress =
		_shaderTable->GetGPUVirtualAddress() + 0 * _shaderTableEntrySize;
	raytraceDesc.RayGenerationShaderRecord.SizeInBytes = _shaderTableEntrySize;
	
	//miss
	const size_t missoffset = 1 * _shaderTableEntrySize;
	raytraceDesc.MissShaderTable.StartAddress = _shaderTable->GetGPUVirtualAddress() + missoffset;
	raytraceDesc.MissShaderTable.StrideInBytes = _shaderTableEntrySize;
	raytraceDesc.MissShaderTable.SizeInBytes = _shaderTableEntrySize * 2;//miss shader 2 entries
	
	//hit
	const size_t hitOffset = 3 * _shaderTableEntrySize;
	raytraceDesc.HitGroupTable.StartAddress = _shaderTable->GetGPUVirtualAddress() + hitOffset;
	raytraceDesc.HitGroupTable.StrideInBytes = _shaderTableEntrySize;
	raytraceDesc.HitGroupTable.SizeInBytes = _shaderTableEntrySize * 8;//hit shader 8 entries

	_commandList->SetComputeRootSignature(_rootSignature.Get());

	ComPtr<ID3D12GraphicsCommandList4> commandList4;

	FAILED_CHECK_BREAK(_commandList->QueryInterface(IID_PPV_ARGS(commandList4.GetAddressOf())));
	commandList4->SetPipelineState1(_pipelineState.Get());

	commandList4->DispatchRays(&raytraceDesc);

	// copy the result to the back-buffer
	br = CD3DX12_RESOURCE_BARRIER::Transition(
		_outputResource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
	_commandList->ResourceBarrier(1, &br);

	
	br = CD3DX12_RESOURCE_BARRIER::Transition(
		_renderTargets[_frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_DEST);
	_commandList->ResourceBarrier(1, &br);
	_commandList->CopyResource(_renderTargets[_frameIndex].Get(), _outputResource.Get());
	
	EndFrame();
}
void DXRTriagnelReworkApp::CheckDebug()
{
	_commandList->Close();
	ID3D12CommandList* commandLists[] = { _commandList.Get() };
	_commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);
	GPUSync();
	_commandList->Reset(_commandAlloc.Get(), nullptr);
}
void DXRTriagnelReworkApp::Flip()
{
	ID3D12CommandList* commandLists[] = { _commandList.Get() };
	_commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);
	_swapChain->Present(0, 0);
	// 디바이스 제거 사유 확인
	HRESULT reason = _device->GetDeviceRemovedReason();
	if (FAILED(reason))
	{
		char reasonStr[64];
		sprintf_s(reasonStr, "DeviceRemovedReason: 0x%08X\n", reason);
		OutputDebugStringA(reasonStr);
	}

	GPUSync();
}


void DXRTriagnelReworkApp::Finalize()
{
}

uint32_t DXRTriagnelReworkApp::BeginFrame()
{
	ID3D12DescriptorHeap* heaps[] = { _srvuavHeap.Get() };
	_commandList->SetDescriptorHeaps(_countof(heaps), heaps);
	_frameIndex = _swapChain->GetCurrentBackBufferIndex();
	return _frameIndex;
}

void DXRTriagnelReworkApp::EndFrame()
{
	auto br = CD3DX12_RESOURCE_BARRIER::Transition(
		_renderTargets[_frameIndex].Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT);
	_commandList->ResourceBarrier(1, &br);
	_commandList->Close();
}

void DXRTriagnelReworkApp::LoadPipeline()
{
#ifdef _DEBUG
	ComPtr<ID3D12Debug> debugController;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(debugController.GetAddressOf()))))
	{
		debugController->EnableDebugLayer();
	}

	//ComPtr<ID3D12Debug1> debugController1;
	//if (SUCCEEDED(debugController->QueryInterface(IID_PPV_ARGS(&debugController1))))
	//{
	//	debugController1->SetEnableGPUBasedValidation(TRUE); // GPU 기반 유효성 검사 (선택)
	//	debugController1->SetEnableSynchronizedCommandQueueValidation(TRUE); // 명령 큐 동기화 검사 (선택)
	//}

	//ComPtr<ID3D12DeviceRemovedExtendedDataSettings> dredSettings;
	//if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&dredSettings))))
	//{
	//	dredSettings->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
	//	dredSettings->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
	//}
#endif
	auto hwnd = Application::GetHwnd();
	assert(hwnd != nullptr); // 디버그 시 꼭 확인해보세요
	CreateDXRDeviceAndSwapChain(hwnd, D3D_FEATURE_LEVEL_12_1);
	CreateRTVHeapAndRTV();
}

void DXRTriagnelReworkApp::CreateDXRDeviceAndSwapChain(HWND hwnd, D3D_FEATURE_LEVEL feature)
{
	ComPtr<IDXGIFactory4> factory;
	UINT flags = 0;
#ifdef _DEBUG
	flags |= DXGI_CREATE_FACTORY_DEBUG;
#endif //  _DEBUG
	FAILED_CHECK_BREAK(CreateDXGIFactory2(flags, IID_PPV_ARGS(factory.GetAddressOf())));
	ComPtr<IDXGIAdapter1> hardwareAdapter;
	GetHardwareAdapter(factory.Get(), hardwareAdapter.GetAddressOf());

	FAILED_CHECK_BREAK(D3D12CreateDevice(hardwareAdapter.Get(),
		D3D_FEATURE_LEVEL_12_1,
		IID_PPV_ARGS(_device.GetAddressOf())));

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	swapChainDesc.BufferCount = _frameCount;
	swapChainDesc.Width = _width;
	swapChainDesc.Height = _height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;

	CreateCommandObject();
	CreateSyncObject();
	_swapChain.Reset();

	ComPtr<IDXGISwapChain1> swapChain;
	FAILED_CHECK_BREAK(factory->CreateSwapChainForHwnd(
		_commandQueue.Get(),
		hwnd, &swapChainDesc, nullptr, nullptr,
		swapChain.GetAddressOf()));
	FAILED_CHECK_BREAK(swapChain->QueryInterface(IID_PPV_ARGS(_swapChain.GetAddressOf())));
	_frameIndex = _swapChain->GetCurrentBackBufferIndex();
}

void DXRTriagnelReworkApp::CreateCommandObject()
{
	D3D12_COMMAND_QUEUE_DESC desc
	{
		.Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
		.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
		.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
		.NodeMask = 0,
	};

	FAILED_CHECK_BREAK(_device->CreateCommandQueue(&desc, IID_PPV_ARGS(_commandQueue.GetAddressOf())));
	FAILED_CHECK_BREAK(_device->CreateCommandAllocator(desc.Type, IID_PPV_ARGS(_commandAlloc.GetAddressOf())));
	FAILED_CHECK_BREAK(_device->CreateCommandList(desc.NodeMask, desc.Type, _commandAlloc.Get(), nullptr,
		IID_PPV_ARGS(_commandList.GetAddressOf())));
	_commandList->Close();
}

void DXRTriagnelReworkApp::CreateSyncObject()
{
	HRESULT hr = _device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(_fence.GetAddressOf()));
	_fenceValue = 1;
	_fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (FAILED(hr) || NULL == _fenceEvent)
		__debugbreak();
}

void DXRTriagnelReworkApp::CreateRTVHeapAndRTV()
{
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc
	{
		.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
		.NumDescriptors = _frameCount,
		.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE
	};
	FAILED_CHECK_BREAK(_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(_rtvHeap.GetAddressOf())));
	_rtvDescriptorSize = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = _rtvHeap->GetCPUDescriptorHandleForHeapStart();
	for (UINT i = 0; i < _frameCount; ++i)
	{
		FAILED_CHECK_BREAK(_swapChain->GetBuffer(i, IID_PPV_ARGS(_renderTargets[i].GetAddressOf())));
		_device->CreateRenderTargetView(_renderTargets[i].Get(), nullptr, rtvHandle);
		rtvHandle.ptr += _rtvDescriptorSize;
	}
}

void DXRTriagnelReworkApp::GPUSync()
{
	const UINT64 fence = _fenceValue;
	_commandQueue->Signal(_fence.Get(), fence);
	_fenceValue++;
	if (_fence->GetCompletedValue() < fence)
	{
		_fence->SetEventOnCompletion(fence, _fenceEvent);
		::WaitForSingleObject(_fenceEvent, INFINITE);
	}
}

void DXRTriagnelReworkApp::CheckDXRSupport() const
{
	D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5{};
	FAILED_CHECK_BREAK(_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5)));
	if (options5.RaytracingTier < D3D12_RAYTRACING_TIER_1_0)
		throw std::runtime_error("DXR not supported on this device");
}

void DXRTriagnelReworkApp::CreateAccelerationStructures()
{
	_vertexBuffers[0] = AccelerationStructure::CreateTriangleVB(_device);
	_vertexBuffers[1] = AccelerationStructure::CreatePlaneVB(_device);
	AccelerationStructureBuffers bottomLevelBuffers[2];

	const uint32_t vertexCnt[] = { 3,6 };
	bottomLevelBuffers[0] = AccelerationStructure::CreateBottomLevelAS(
		_device, _commandList, _vertexBuffers, vertexCnt, 2
	);
	_bottomLevelAS[0] = bottomLevelBuffers[0].pResult;

	bottomLevelBuffers[1] = AccelerationStructure::CreateBottomLevelAS(
		_device, _commandList, _vertexBuffers, vertexCnt, 1);
	_bottomLevelAS[1] = bottomLevelBuffers[1].pResult;
	
	AccelerationStructure::BuildTopLevelAS(
		_device,
		_commandList,
		_bottomLevelAS,
		_tlasSize,
		false,
		false,
		_topLevelBuffers
	);
	FAILED_CHECK_BREAK(_commandList->Close());
	ID3D12CommandList* ppCommandLists[] = { _commandList.Get() };
	_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	GPUSync();
}

void DXRTriagnelReworkApp::CreateRTPipelineState()
{
	// Need 16 subobjects:
//  1 for DXIL library    
//  3 for the hit-groups (triangle hit group, plane hit-group, shadow-hit group)
//  2 for RayGen root-signature (root-signature and the subobject association)
//  2 for triangle hit-program root-signature (root-signature and the subobject association)
//  2 for the plane-hit root-signature (root-signature and the subobject association)
//  2 for shadow-program and miss root-signature (root-signature and the subobject association)
//  2 for shader config (shared between all programs. 1 for the config, 1 for association)
//  1 for pipeline config
//  1 for the global root signature
	std::array<D3D12_STATE_SUBOBJECT, 16> subObjects{};
	uint32_t index = 0;
	
	// dxil library 
	DxilLibrary dxilLib = RTPipeline::CreateDxilLibrary();
	subObjects[index++] = dxilLib.stateSubobject; // 0: library

	// hit program
	HitProgram triHitProgram(nullptr, RTPipeline::TriangleChs, RTPipeline::TriHitGroup);
	subObjects[index++] = triHitProgram.subObject; // 1: triangle hit group

	HitProgram planeHitProgram(nullptr, RTPipeline::PlaneChs, RTPipeline::PlaneHitGroup);
	subObjects[index++] = planeHitProgram.subObject; // 2: plane hit group
	
	HitProgram shadowHitProgram(nullptr, RTPipeline::ShadowChs, RTPipeline::ShadowHitGroup);
	subObjects[index++] = shadowHitProgram.subObject; // 3: shadow hit group 

	// root signature
	LocalRootSignature rgsRootSignature(
		_device, RTPipeline::CreateRayGenRootDesc().desc
	);
	subObjects[index] = rgsRootSignature.subObject; // 4: raygen rootsig
	
	uint32_t rgsRootIndex = index++;
	ExportAssociation rgsRootAssociation(&RTPipeline::RayGenShader, 1, &(subObjects[rgsRootIndex]));
	subObjects[index++] = rgsRootAssociation.subObject; // 5: Associate Root sig to root sig desc



	LocalRootSignature triHitRootSignature(
		_device, RTPipeline::CreateTriangleHitRootDesc().desc
	);
	subObjects[index] = triHitRootSignature.subObject; // 6: triangle hit root sig
	
	uint32_t triRootIndex = index++;
	ExportAssociation triHitRootAssociation(&RTPipeline::TriangleChs, 1, &(subObjects[triRootIndex]));
	subObjects[index++] = triHitRootAssociation.subObject; // 7: associate tri root sig to tri hit group



	LocalRootSignature planeHitRootSignature(
		_device, RTPipeline::CreatePlaneHitRootDesc().desc
	);
	subObjects[index] = planeHitRootSignature.subObject; // 8: plane hit root sig

	uint32_t planeRootIndex = index++;
	ExportAssociation planeHitRootAssociation(&RTPipeline::PlaneHitGroup, 1, &(subObjects[planeRootIndex]));
	subObjects[index++] = planeHitRootAssociation.subObject;// 9: associate plane hit root sig to plane hit group


	D3D12_ROOT_SIGNATURE_DESC emptyDesc{};
	emptyDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
	LocalRootSignature emptyRootSignature(_device, emptyDesc);
	subObjects[index] = emptyRootSignature.subObject; // 10: empty root sig for plane hit group and miss
	
	uint32_t emptyRootIndex = index++;
	const WCHAR* emptyRootExport[]{
		RTPipeline::MissShader,RTPipeline::ShadowChs,RTPipeline::ShadowMiss
	};
	ExportAssociation emptyRootAssociation(emptyRootExport, _countof(emptyRootExport), &(subObjects[emptyRootIndex]));
	subObjects[index++] = emptyRootAssociation.subObject;// 11: associate empty root sig to plane hit group and miss shader

	// bind the payload size to all programs
	ShaderConfig primaryShaderConfig(sizeof(float) * 2, sizeof(float) * 3);
	subObjects[index] = primaryShaderConfig.subobject; // 12
	uint32_t primaryShaderConfigIndex = index++;
	const WCHAR* primaryShaderExports[] = {
		RTPipeline::RayGenShader,
		RTPipeline::MissShader,
		RTPipeline::TriangleChs,
		RTPipeline::PlaneChs,
		RTPipeline::ShadowMiss,
		RTPipeline::ShadowChs
	};
	ExportAssociation primaryConfigAssociation(
		primaryShaderExports,
		_countof(primaryShaderExports),
		&(subObjects[primaryShaderConfigIndex])
	);
	subObjects[index++] = primaryConfigAssociation.subObject; // 13: associate shader config to all programs
	
	// create the pipeline config
	PipelineConfig config(2);
	subObjects[index++] = config.subObject;// 14

	// create global root signature and store the empty signature
	GlobalRootSignature root(_device, {});
	_emptyRootsignature = root.rootSingnature;
	subObjects[index++] = root.subObject;// 15

	//create the state
	D3D12_STATE_OBJECT_DESC desc;
	desc.NumSubobjects = index;// 16
	desc.pSubobjects = subObjects.data();
	desc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;

	ComPtr<ID3D12Device5> device5;
	FAILED_CHECK_BREAK(_device->QueryInterface(device5.GetAddressOf()));
	FAILED_CHECK_BREAK(device5->CreateStateObject(&desc, IID_PPV_ARGS(_pipelineState.GetAddressOf())));
}

void DXRTriagnelReworkApp::CreateShaderTable()
{
	/** The shader-table layout is as follows:
	Entry 0 - Ray-gen program
	Entry 1 - Miss program for the primary ray
	Entry 2 - Miss program for the shadow ray
	Entries 3,4 - Hit programs for triangle 0 (primary followed by shadow)
	Entries 5,6 - Hit programs for the plane (primary followed by shadow)
	Entries 7,8 - Hit programs for triangle 1 (primary followed by shadow)
	Entries 9,10 - Hit programs for triangle 2 (primary followed by shadow)
	Entries 11,12 - Hit programs for triangle 3 (primary followed by shadow)
	All entries in the shader-table must have the same size, so we will choose it base on the largest required entry.
	The triangle primary-ray hit program requires the largest entry - sizeof(program identifier) + 8 bytes for the constant-buffer root descriptor.
	The entry size must be aligned up to D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT
*/
	_shaderTableEntrySize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
	_shaderTableEntrySize += 8;
	_shaderTableEntrySize = d3dUtil::AlignTo(_shaderTableEntrySize, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);
	const uint32_t shaderTableSize = _shaderTableEntrySize * 13;

	d3dUtil::CreateUploadBuffer(
		shaderTableSize, D3D12_RESOURCE_FLAG_NONE,
		D3D12_RESOURCE_STATE_GENERIC_READ, _shaderTable, _device.Get()
	);

	uint8_t* pData;
	FAILED_CHECK_BREAK(_shaderTable->Map(0, nullptr, reinterpret_cast<void**>(&pData)));
	ComPtr<ID3D12StateObjectProperties> pRtsoProps;
	FAILED_CHECK_BREAK(_pipelineState->QueryInterface(IID_PPV_ARGS(pRtsoProps.GetAddressOf())));

	const uint64_t heapStart = _srvuavHeap->GetGPUDescriptorHandleForHeapStart().ptr;;
	const uint32_t descIncrSize = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);


	d3dUtil::WriteShaderTableEntry(
		pData, 0, _shaderTableEntrySize,
		pRtsoProps->GetShaderIdentifier(RTPipeline::RayGenShader), &heapStart,
		sizeof(uint64_t)
	);

	d3dUtil::WriteShaderTableEntry(
		pData, 1, _shaderTableEntrySize,
		pRtsoProps->GetShaderIdentifier(RTPipeline::MissShader), nullptr, 0
	);
	d3dUtil::WriteShaderTableEntry(
		pData, 2, _shaderTableEntrySize,
		pRtsoProps->GetShaderIdentifier(RTPipeline::ShadowMiss), nullptr, 0
	);

	D3D12_GPU_VIRTUAL_ADDRESS cbAddr0 = _constantResource[0]->GetGPUVirtualAddress();
	d3dUtil::WriteShaderTableEntry(
		pData, 3, _shaderTableEntrySize,
		pRtsoProps->GetShaderIdentifier(RTPipeline::TriHitGroup), &cbAddr0, sizeof(cbAddr0)
	);
	d3dUtil::WriteShaderTableEntry(
		pData, 4, _shaderTableEntrySize,
		pRtsoProps->GetShaderIdentifier(RTPipeline::ShadowHitGroup), nullptr, 0
	);

	uint64_t srvAddr = heapStart + descIncrSize;
	d3dUtil::WriteShaderTableEntry(
		pData, 5, _shaderTableEntrySize,
		pRtsoProps->GetShaderIdentifier(RTPipeline::PlaneHitGroup), &srvAddr, sizeof(srvAddr)
	);
	d3dUtil::WriteShaderTableEntry(
		pData, 6, _shaderTableEntrySize,
		pRtsoProps->GetShaderIdentifier(RTPipeline::ShadowHitGroup), nullptr, 0
	);

	D3D12_GPU_VIRTUAL_ADDRESS cbAddr1 = _constantResource[1]->GetGPUVirtualAddress();
	d3dUtil::WriteShaderTableEntry(
		pData, 7, _shaderTableEntrySize,
		pRtsoProps->GetShaderIdentifier(RTPipeline::TriHitGroup), &cbAddr1, sizeof(cbAddr1)
	);
	d3dUtil::WriteShaderTableEntry(
		pData, 8, _shaderTableEntrySize,
		pRtsoProps->GetShaderIdentifier(RTPipeline::ShadowHitGroup), nullptr, 0
	);

	D3D12_GPU_VIRTUAL_ADDRESS cbAddr2 = _constantResource[2]->GetGPUVirtualAddress();
	d3dUtil::WriteShaderTableEntry(
		pData, 9, _shaderTableEntrySize,
		pRtsoProps->GetShaderIdentifier(RTPipeline::TriHitGroup), &cbAddr2, sizeof(cbAddr2)
	);
	d3dUtil::WriteShaderTableEntry(
		pData, 10, _shaderTableEntrySize,
		pRtsoProps->GetShaderIdentifier(RTPipeline::ShadowHitGroup), nullptr, 0
	);

	D3D12_GPU_VIRTUAL_ADDRESS cbAddr3 = _constantResource[3]->GetGPUVirtualAddress();
	d3dUtil::WriteShaderTableEntry(
		pData, 11, _shaderTableEntrySize,
		pRtsoProps->GetShaderIdentifier(RTPipeline::TriHitGroup), &cbAddr3, sizeof(cbAddr3)
	);
	d3dUtil::WriteShaderTableEntry(
		pData, 12, _shaderTableEntrySize,
		pRtsoProps->GetShaderIdentifier(RTPipeline::ShadowHitGroup), nullptr, 0
	);

	_shaderTable->Unmap(0, nullptr);
}

void DXRTriagnelReworkApp::CreateShaderResource()
{
	D3D12_RESOURCE_DESC resDesc{};
	resDesc.DepthOrArraySize = 1;
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	resDesc.Width = _width;
	resDesc.Height = _height;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN; 
	resDesc.MipLevels = 1;
	resDesc.SampleDesc.Count = 1;
	FAILED_CHECK_BREAK(
		_device->CreateCommittedResource(
			&DefaultHeapProps,
			D3D12_HEAP_FLAG_NONE,
			&resDesc,
			D3D12_RESOURCE_STATE_COPY_SOURCE,
			nullptr,
			IID_PPV_ARGS(_outputResource.GetAddressOf())
		)
	);
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
	heapDesc.NumDescriptors = 2;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(_srvuavHeap.GetAddressOf()));

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	_device->CreateUnorderedAccessView(
		_outputResource.Get(),
		nullptr,
		&uavDesc,
		_srvuavHeap->GetCPUDescriptorHandleForHeapStart()
	);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.RaytracingAccelerationStructure.Location = _topLevelBuffers.pResult->GetGPUVirtualAddress();
	D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = _srvuavHeap->GetCPUDescriptorHandleForHeapStart();
	srvHandle.ptr += _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	_device->CreateShaderResourceView(nullptr, &srvDesc, srvHandle);
}

void DXRTriagnelReworkApp::CreateConstantBuffer()
{
	Vector4 bufferData[] = {
		Vector4(1.f,0.f,0.f,1.f),
		Vector4(1.f,1.f,0.f,1.f),
		Vector4(1.f,0.f,1.f,1.f),

		Vector4(0.f,1.f,0.f,1.f),
		Vector4(0.f,1.f,1.f,1.f),
		Vector4(1.f,1.f,0.f,1.f),

		Vector4(0.f,0.f,1.f,1.f),
		Vector4(1.f,0.f,1.f,1.f),
		Vector4(0.f,1.f,1.f,1.f),

		Vector4(0.f,0.f,1.f,1.f),
		Vector4(1.f,0.f,1.f,1.f),
		Vector4(0.f,1.f,1.f,1.f),
	};
	for (uint32_t i = 0; i < 4; ++i)
	{
		const uint32_t bufferSize = sizeof(Vector4)*3;
		d3dUtil::CreateUploadBuffer(
			bufferSize, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ,
			_constantResource[i], _device.Get()
		);
		d3dUtil::UpdateBuffer(_constantResource[i], &bufferData[i * 3], sizeof(bufferData));
	}
}
