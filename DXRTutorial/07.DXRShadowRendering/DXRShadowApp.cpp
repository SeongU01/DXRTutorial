#include "DXRShadowApp.h"
#include "Application.h"
#include "RTPipeline.h"
#include "HitProgram.h"
#include "LocalRootSignature.h"
#include "ExportAssociation.h"
#include "ShaderConfig.h"
#include "PipelineConfig.h"
#include "GlobalRootSignature.h"
#include "VertexPositionNormalTangentTexture.h"

std::vector<ComPtr<ID3D12Resource>> DXRShadowApp::gUploadBuffers{};

DXRShadowApp::DXRShadowApp(UINT width, UINT height, std::wstring name)
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

void DXRShadowApp::Initialize()
{
	LoadPipeline();
	_commandList->Reset(_commandAlloc.Get(), nullptr);
	CreateAccelerationStructures();
	CreateRTPipelineState();
	CreateShaderResource();
	CreateShaderTable();
}

void DXRShadowApp::Update(const float& dt)
{
}

void DXRShadowApp::Render()
{
	_commandAlloc->Reset();
	_commandList->Reset(_commandAlloc.Get(), nullptr);
	BeginFrame();
	auto br = CD3DX12_RESOURCE_BARRIER::Transition(
		_outputResource.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS
	);
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
	const size_t missOffset = 1 * _shaderTableEntrySize;
	raytraceDesc.MissShaderTable.StartAddress = _shaderTable->GetGPUVirtualAddress() + missOffset;
	raytraceDesc.MissShaderTable.StrideInBytes = _shaderTableEntrySize;
	raytraceDesc.MissShaderTable.SizeInBytes = _shaderTableEntrySize * 2;

	//hit
	const size_t hitOffset = 3 * _shaderTableEntrySize;
	raytraceDesc.HitGroupTable.StartAddress = _shaderTable->GetGPUVirtualAddress() + hitOffset;
	raytraceDesc.HitGroupTable.StrideInBytes = _shaderTableEntrySize;
	raytraceDesc.HitGroupTable.SizeInBytes = _shaderTableEntrySize * 1;

	//bind 
	_commandList->SetComputeRootSignature(_emptyRootsignature.Get());

	ComPtr<ID3D12GraphicsCommandList4> cmdList4;
	FAILED_CHECK_BREAK(_commandList->QueryInterface(cmdList4.GetAddressOf()));

	cmdList4->SetPipelineState1(_pipelineState.Get());
	cmdList4->DispatchRays(&raytraceDesc);

	br = CD3DX12_RESOURCE_BARRIER::Transition(
		_outputResource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_COPY_SOURCE
	);
	_commandList->ResourceBarrier(1, &br);

	br = CD3DX12_RESOURCE_BARRIER::Transition(
		_renderTargets[_frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_COPY_DEST
	);
	_commandList->ResourceBarrier(1, &br);
	_commandList->CopyResource(_renderTargets[_frameIndex].Get(), _outputResource.Get());

	EndFrame();
}
void DXRShadowApp::CheckDebug()
{
	_commandList->Close();
	ID3D12CommandList* commandLists[] = { _commandList.Get() };
	_commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);
	GPUSync();
	_commandList->Reset(_commandAlloc.Get(), nullptr);
}
void DXRShadowApp::Flip()
{
	ID3D12CommandList* commandLists[] = { _commandList.Get() };
	_commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);
	_swapChain->Present(0, 0);
	//// 디바이스 제거 사유 확인
	//HRESULT reason = _device->GetDeviceRemovedReason();
	//if (FAILED(reason))
	//{
	//	char reasonStr[64];
	//	sprintf_s(reasonStr, "DeviceRemovedReason: 0x%08X\n", reason);
	//	OutputDebugStringA(reasonStr);
	//}

	GPUSync();
	gUploadBuffers.clear();
}


void DXRShadowApp::Finalize()
{
}

uint32_t DXRShadowApp::BeginFrame()
{
	ID3D12DescriptorHeap* heaps[] = { _srvuavHeap.Get() };
	_commandList->SetDescriptorHeaps(_countof(heaps), heaps);
	_frameIndex = _swapChain->GetCurrentBackBufferIndex();
	return _frameIndex;
}

void DXRShadowApp::EndFrame()
{
	auto br = CD3DX12_RESOURCE_BARRIER::Transition(
		_renderTargets[_frameIndex].Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT);
	_commandList->ResourceBarrier(1, &br);
	_commandList->Close();
}

void DXRShadowApp::LoadPipeline()
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

void DXRShadowApp::CreateDXRDeviceAndSwapChain(HWND hwnd, D3D_FEATURE_LEVEL feature)
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

void DXRShadowApp::CreateCommandObject()
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

void DXRShadowApp::CreateSyncObject()
{
	HRESULT hr = _device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(_fence.GetAddressOf()));
	_fenceValue = 1;
	_fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (FAILED(hr) || NULL == _fenceEvent)
		__debugbreak();
}

void DXRShadowApp::CreateRTVHeapAndRTV()
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

void DXRShadowApp::GPUSync()
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

void DXRShadowApp::CheckDXRSupport() const
{
	D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5{};
	FAILED_CHECK_BREAK(_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5)));
	if (options5.RaytracingTier < D3D12_RAYTRACING_TIER_1_0)
		throw std::runtime_error("DXR not supported on this device");
}

void DXRShadowApp::CreateAccelerationStructures()
{
	_pBottomLevelAS.push_back(AccelerationStructure::CreatePlaneBottomLevelAS(_device, _commandList));
	_pBottomLevelAS.push_back(AccelerationStructure::CreatePrimitiveBottomLevelAS(_device, _commandList));

	AccelerationStructure::BuiltTopLevelAS(_device, _commandList, _pBottomLevelAS, _tlasSize, _topLevelBuffers);

	FAILED_CHECK_BREAK(_commandList->Close());
	ID3D12CommandList* ppCommandList[] = { _commandList.Get() };
	_commandQueue->ExecuteCommandLists(_countof(ppCommandList), ppCommandList);
	GPUSync();
}
void DXRShadowApp::CreateRTPipelineState()
{
	// 12 object
	// 1 dxil library
	// 1 hit group
	// 2 raygen root-sig
	// 2 hit program root-sig
	// 2 miss shader root-sig
	// 2 shader config
	// 1 pipeline config
	// 1 global root-sig
	std::array<D3D12_STATE_SUBOBJECT, 12> subobjects{};
	uint32_t index = 0;

	DxilLibrary dxilLibrary = RTPipeline::CreateDxilLibrary();
	subobjects[index++] = dxilLibrary.stateSubobject;// 0 

	HitProgram hitProgram(nullptr, RTPipeline::ClosestHitShader, RTPipeline::HitGroup);
	subobjects[index++] = hitProgram.subObject;// 1

	LocalRootSignature rgsRootSignature(_device, RTPipeline::CreateRayGenRootDesc().desc);
	subobjects[index] = rgsRootSignature.subObject;// 2 

	const uint32_t rgsRootIndex = index++;
	ExportAssociation rgsRootAssociation(&RTPipeline::RayGenShader, 1, &(subobjects[rgsRootIndex]));
	subobjects[index++] = rgsRootAssociation.subObject;// 3

	LocalRootSignature hitRootSignature(_device, RTPipeline::CreateHitRootDesc().desc);
	subobjects[index] = hitRootSignature.subObject;// 4

	const uint32_t hitRootIndex = index++;
	ExportAssociation hitRootAssociation(&RTPipeline::ClosestHitShader, 1, &(subobjects[hitRootIndex]));
	subobjects[index++] = hitRootAssociation.subObject;// 5

	D3D12_ROOT_SIGNATURE_DESC emptyDesc{};
	emptyDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
	LocalRootSignature missRootSignature(_device, emptyDesc);
	subobjects[index] = missRootSignature.subObject;// 6

	const uint32_t missRootIndex = index++;
	const WCHAR* missRottShaders[] = {
		RTPipeline::MissShader,RTPipeline::ShadowMiss
	};
	ExportAssociation missRootAssociation(missRottShaders, _countof(missRottShaders), &(subobjects[missRootIndex]));
	subobjects[index++] = missRootAssociation.subObject;// 7

	// payload size float4+uint
	ShaderConfig shaderConfig(sizeof(float) * 2, sizeof(float) * (4+1));
	subobjects[index] = shaderConfig.subobject;// 8

	const uint32_t shaderConfigIndex = index++;
	const WCHAR* shaderExports[] = {
		RTPipeline::MissShader,RTPipeline::ClosestHitShader,RTPipeline::RayGenShader,RTPipeline::ShadowMiss
	};
	ExportAssociation configAssociation(shaderExports, _countof(shaderExports), &(subobjects[shaderConfigIndex]));
	subobjects[index++] = configAssociation.subObject;// 9

	PipelineConfig config(4+1);
	subobjects[index++] = config.subObject;// 10

	GlobalRootSignature root(_device, {});
	_emptyRootsignature = root.rootSingnature;
	subobjects[index++] = root.subObject;// 11

	D3D12_STATE_OBJECT_DESC desc;
	desc.NumSubobjects = index;
	desc.pSubobjects = subobjects.data();
	desc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;

	ComPtr<ID3D12Device5> device5;
	FAILED_CHECK_BREAK(_device->QueryInterface(device5.GetAddressOf()));
	FAILED_CHECK_BREAK(device5->CreateStateObject(&desc, IID_PPV_ARGS(_pipelineState.GetAddressOf())));
}

void DXRShadowApp::CreateShaderTable()
{
	/*
	* entry 0 - ray-gen program
	* entry 1 - miss program
	* entry 2 - hit program
	*/
	_shaderTableEntrySize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
	_shaderTableEntrySize += 8;
	_shaderTableEntrySize = d3dUtil::AlignTo(_shaderTableEntrySize, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);
	const uint32_t shaderTableSize = _shaderTableEntrySize * 4;

	d3dUtil::CreateUploadBuffer(
		shaderTableSize,
		D3D12_RESOURCE_FLAG_NONE,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		_shaderTable,
		_device.Get()
	);

	uint8_t* pData;
	FAILED_CHECK_BREAK(_shaderTable->Map(0, nullptr, reinterpret_cast<void**>(&pData)));
	ComPtr<ID3D12StateObjectProperties> pRtsoProps;
	_pipelineState->QueryInterface(IID_PPV_ARGS(pRtsoProps.GetAddressOf()));

	// 0 ray gen program
	memcpy(pData, pRtsoProps->GetShaderIdentifier(RTPipeline::RayGenShader), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
	const uint64_t heapStart = _srvuavHeap->GetGPUDescriptorHandleForHeapStart().ptr;
	*reinterpret_cast<uint64_t*>(pData + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES) = heapStart;

	// 1 primary ray miss
	pData += _shaderTableEntrySize;
	memcpy(pData, pRtsoProps->GetShaderIdentifier(RTPipeline::MissShader), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

	// 2 miss program
	pData += _shaderTableEntrySize;
	memcpy(pData, pRtsoProps->GetShaderIdentifier(RTPipeline::ShadowMiss), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

	// 3 hit program
	pData += _shaderTableEntrySize;
	memcpy(pData, pRtsoProps->GetShaderIdentifier(RTPipeline::HitGroup), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
	const uint64_t heapStartHit = _srvuavHeap->GetGPUDescriptorHandleForHeapStart().ptr;
	*reinterpret_cast<uint64_t*>(pData + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES) = heapStartHit;
	_shaderTable->Unmap(0, nullptr);
}

void DXRShadowApp::CreateShaderResource()
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
	heapDesc.NumDescriptors = 4;
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

	AccelerationStructure::ShapeResource* primitives = AccelerationStructure::_resources[1].get();
	// Index SRV
	D3D12_SHADER_RESOURCE_VIEW_DESC indexSrvDesc{};
	indexSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	indexSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	indexSrvDesc.Format = DXGI_FORMAT_UNKNOWN;
	indexSrvDesc.Buffer.NumElements = static_cast<int>(primitives->indexCount);
	indexSrvDesc.Buffer.StructureByteStride = sizeof(UINT);
	srvHandle.ptr += _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	indexSRVHandle = srvHandle;
	_device->CreateShaderResourceView(primitives->indexBuffer.Get(), &indexSrvDesc, indexSRVHandle);

	// Vertex SRV
	D3D12_SHADER_RESOURCE_VIEW_DESC vertexSRVDesc = {};
	vertexSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	vertexSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	vertexSRVDesc.Format = DXGI_FORMAT_UNKNOWN;
	vertexSRVDesc.Buffer.NumElements = static_cast<int>(primitives->vertexCount);
	vertexSRVDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	vertexSRVDesc.Buffer.StructureByteStride = sizeof(VertexPositionNormalTangentTexture);

	srvHandle.ptr += _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	vertexSRVHandle = srvHandle;
	_device->CreateShaderResourceView(primitives->vertexBuffer.Get(), &vertexSRVDesc, vertexSRVHandle);
}