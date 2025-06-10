#include "DXRTriangleApp.h"
#include "Application.h"
#include "../DXRTutorial/nv_helpers_dx12/BottomLevelASGenerator.h"
#include "../DXRTutorial/nv_helpers_dx12/RaytracingPipelineGenerator.h"
#include "../DXRTutorial/nv_helpers_dx12/RootSignatureGenerator.h"

DXRTriangleApp::DXRTriangleApp(UINT width, UINT height, std::wstring name)
	:DXRSample{ width,height,name }, _frameIndex{ 0 }, _rtvDescriptorSize{ 0 }
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

void DXRTriangleApp::Initialize()
{
	LoadPipeline();
	_commandList->Reset(_commandAlloc.Get(), nullptr);
	LoadAssets();

	CheckDXRSupport();

	CreateAccelerationStructures();

	FAILED_CHECK_BREAK(_commandList->Close());

	CreateRaytracingPipeline();
	CreateRayTracingOutputBuffer();
	CreateShaderResourceHeap();
	CreateShaderBindingTable();
}

void DXRTriangleApp::Update(const float& dt)
{
	static bool prevKeyDown = false;
	bool currKeyDown = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;

	if (currKeyDown && !prevKeyDown)
	{
		_isRaster = !_isRaster;
	}

	prevKeyDown = currKeyDown;
}

void DXRTriangleApp::Render()
{
	WriteCommand();
	// Execute the command list.
	ID3D12CommandList* ppCommandLists[] = { _commandList.Get() };
	_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	Flip();
}

void DXRTriangleApp::Flip()
{
	_swapChain->Present(0, 0);
	GPUSync();
	_uploadBuffers.clear();
	_frameIndex = _swapChain->GetCurrentBackBufferIndex();

}

void DXRTriangleApp::Finalize()
{
}

void DXRTriangleApp::LoadPipeline()
{
	UINT flags = 0;
#ifdef _DEBUG
	ComPtr<ID3D12Debug> debugController;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(debugController.GetAddressOf()))))
	{
		debugController->EnableDebugLayer();
		flags |= DXGI_CREATE_FACTORY_DEBUG;
	}
#endif
	auto hwnd = Application::GetHwnd();
	assert(hwnd != nullptr); // ����� �� �� Ȯ���غ�����
	CreateDXRDeviceAndSwapChain(hwnd, D3D_FEATURE_LEVEL_12_1);
	CreateRTVHeapAndRTV();
}

void DXRTriangleApp::CreateDXRDeviceAndSwapChain(HWND hwnd, D3D_FEATURE_LEVEL feature)
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

void DXRTriangleApp::CreateCommandObject()
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

void DXRTriangleApp::CreateSyncObject()
{
	HRESULT hr = _device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(_fence.GetAddressOf()));
	_fenceValue = 1;
	_fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (FAILED(hr) || NULL == _fenceEvent)
		__debugbreak();
}

void DXRTriangleApp::CreateRTVHeapAndRTV()
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

void DXRTriangleApp::LoadAssets()
{
	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	// create empty root signature
	rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	ComPtr<ID3D10Blob> signature;
	ComPtr<ID3D10Blob> error;
	FAILED_CHECK_BREAK(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		signature.GetAddressOf(), error.GetAddressOf()
	));
	FAILED_CHECK_BREAK(_device->CreateRootSignature(0,
		signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(_rootSignature.GetAddressOf())));



	ComPtr<ID3DBlob> vertexShader;
	ComPtr<ID3DBlob> pixelShader;

#ifdef _DEBUG
	// Enable better shader debugging with the graphics debugging tools.
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif

	const std::wstring shader_path = L"shaders/shaders.hlsl";

	FAILED_CHECK_BREAK(D3DCompileFromFile(shader_path.c_str(),
		nullptr, nullptr, "vs_main", "vs_5_1",
		compileFlags, 0, &vertexShader, nullptr));
	FAILED_CHECK_BREAK(D3DCompileFromFile(shader_path.c_str(),
		nullptr, nullptr, "ps_main", "ps_5_1",
		compileFlags, 0, &pixelShader, nullptr));

	// Define the vertex input layout.
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
		{
			"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
		},
		{
			"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
		}
	};

	// Describe and create the graphics pipeline state object (PSO).
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
	psoDesc.pRootSignature = _rootSignature.Get();
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState.DepthEnable = FALSE;
	psoDesc.DepthStencilState.StencilEnable = FALSE;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc.Count = 1;
	FAILED_CHECK_BREAK(_device->CreateGraphicsPipelineState(
		&psoDesc, IID_PPV_ARGS(&_pso)));

	Vertex triangleVertices[] = {
			{{0.0f, 0.25f * _aspectRatio, 0.0f}, {1.0f, 1.0f, 0.0f, 1.0f}},
			{{0.25f, -0.25f * _aspectRatio, 0.0f}, {0.0f, 1.0f, 1.0f, 1.0f}},
			{{-0.25f, -0.25f * _aspectRatio, 0.0f}, {1.0f, 0.0f, 1.0f, 1.0f}}
	};

	const UINT vertexBufferSize = sizeof(triangleVertices);
	ComPtr<ID3D12Resource> uploadBuffer;
	_vertexBuffer = d3dUtil::CreateDefaultBuffer(_device.Get(), _commandList.Get(), triangleVertices
		, vertexBufferSize, uploadBuffer);
	_uploadBuffers.push_back(uploadBuffer);

	_vertexBufferView.BufferLocation = _vertexBuffer->GetGPUVirtualAddress();
	_vertexBufferView.StrideInBytes = sizeof(Vertex);
	_vertexBufferView.SizeInBytes = vertexBufferSize;

	_commandList->Close();
	ID3D12CommandList* ppCommandLists[] = { _commandList.Get() };
	_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	GPUSync();
	FAILED_CHECK_BREAK(_commandList->Reset(_commandAlloc.Get(), _pso.Get()));
}

void DXRTriangleApp::WriteCommand() const
{
	FAILED_CHECK_BREAK(_commandAlloc->Reset());
	FAILED_CHECK_BREAK(_commandList->Reset(_commandAlloc.Get(), _pso.Get()));
	_commandList->SetGraphicsRootSignature(_rootSignature.Get());
	_commandList->RSSetViewports(1, &_viewPort);
	_commandList->RSSetScissorRects(1, &_scissorRect);
	auto br = CD3DX12_RESOURCE_BARRIER::Transition(
		_renderTargets[_frameIndex].Get(),
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);
	_commandList->ResourceBarrier(1, &br);
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(
		_rtvHeap->GetCPUDescriptorHandleForHeapStart(), _frameIndex, _rtvDescriptorSize
	);
	_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
 	if (_isRaster)
	{
		const float clearColor[] = { 0.4f,0.2f,0.f,1.f };
		_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
		_commandList->IASetVertexBuffers(0, 1, &_vertexBufferView);
		_commandList->DrawInstanced(3, 1, 0, 0);
	}
	else
	{
		std::vector<ID3D12DescriptorHeap*> heaps = { _srvUavHeap.Get() };
		_commandList->SetDescriptorHeaps(static_cast<UINT>(heaps.size()),heaps.data());
		auto br = CD3DX12_RESOURCE_BARRIER::Transition(
			_outputResource.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS
		);
		_commandList->ResourceBarrier(1, &br);
		
		D3D12_DISPATCH_RAYS_DESC desc{};
		
		const uint32_t rayGenerationSectionSizeInBytes =
			_sbtHelper.GetRayGenSectionSize();
		desc.RayGenerationShaderRecord.StartAddress= 
			_sbtStorage->GetGPUVirtualAddress();
		desc.RayGenerationShaderRecord.SizeInBytes =
			rayGenerationSectionSizeInBytes;

		const uint32_t missSectionSizeInBytes = 
			_sbtHelper.GetMissSectionSize();
		desc.MissShaderTable.StartAddress =
			_sbtStorage->GetGPUVirtualAddress() + rayGenerationSectionSizeInBytes;
		desc.MissShaderTable.SizeInBytes = missSectionSizeInBytes;
		desc.MissShaderTable.StrideInBytes = _sbtHelper.GetMissEntrySize();

		const uint32_t hitGroupSectionSize =
			_sbtHelper.GetHitGroupSectionSize();
		desc.HitGroupTable.StartAddress =
			_sbtStorage->GetGPUVirtualAddress() + rayGenerationSectionSizeInBytes + missSectionSizeInBytes;
		desc.HitGroupTable.SizeInBytes = hitGroupSectionSize;
		desc.HitGroupTable.StrideInBytes = _sbtHelper.GetHitGroupEntrySize();
		desc.Width = GetWidth();
		desc.Height = GetHeight();
		desc.Depth = 1;

		ComPtr<ID3D12GraphicsCommandList4> commandList4;
		_commandList->QueryInterface(IID_PPV_ARGS(commandList4.GetAddressOf()));
		commandList4->SetPipelineState1(_rtStateObject.Get());
		commandList4->DispatchRays(&desc);
		br = CD3DX12_RESOURCE_BARRIER::Transition(
			_outputResource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			D3D12_RESOURCE_STATE_COPY_SOURCE);
		_commandList->ResourceBarrier(1, &br);
		br = CD3DX12_RESOURCE_BARRIER::Transition(
			_renderTargets[_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET
			, D3D12_RESOURCE_STATE_COPY_DEST);
		_commandList->ResourceBarrier(1, &br);
		_commandList->CopyResource(_renderTargets[_frameIndex].Get(), _outputResource.Get());

		br = CD3DX12_RESOURCE_BARRIER::Transition(
			_renderTargets[_frameIndex].Get(), D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_RENDER_TARGET);
		_commandList->ResourceBarrier(1, &br);
	}
	br = CD3DX12_RESOURCE_BARRIER::Transition(
		_renderTargets[_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	_commandList->ResourceBarrier(1, &br);
	FAILED_CHECK_BREAK(_commandList->Close());
}

void DXRTriangleApp::GPUSync()
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

void DXRTriangleApp::CheckDXRSupport() const
{
	D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5{};
	FAILED_CHECK_BREAK(_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5)));
	if (options5.RaytracingTier < D3D12_RAYTRACING_TIER_1_0)
		throw std::runtime_error("DXR not supported on this device");
}

DXRTriangleApp::AccelerationStructureBuffers
DXRTriangleApp::CreateBottomLevelAS(std::vector<std::pair<ComPtr<ID3D12Resource>, uint32_t>> vVertexBuffers) const
{
	ComPtr<ID3D12Device5> device5;
	FAILED_CHECK_BREAK(_device->QueryInterface(IID_PPV_ARGS(device5.GetAddressOf())));
	nv_helpers_dx12::BottomLevelASGenerator bottomLevelAS;
	for (const auto& buffers : vVertexBuffers)
	{
		bottomLevelAS.AddVertexBuffer(buffers.first.Get(), 0, buffers.second, sizeof(Vertex), nullptr, 0);
	}
	UINT64 scratchSizeInBytes = 0;
	UINT64 resultSizeInBytes = 0;
	bottomLevelAS.ComputeASBufferSizes(device5.Get(), false, &scratchSizeInBytes, &resultSizeInBytes);
	AccelerationStructureBuffers buffers;
	d3dUtil::CreateBuffer(scratchSizeInBytes,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON,
		buffers.scratch, _device.Get()
	);
	d3dUtil::CreateBuffer(resultSizeInBytes,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
		buffers.result, _device.Get()
	);
	ComPtr<ID3D12GraphicsCommandList4> commandList4;
	FAILED_CHECK_BREAK(_commandList->QueryInterface(IID_PPV_ARGS(commandList4.GetAddressOf())));
	bottomLevelAS.Generate(commandList4.Get(), buffers.scratch.Get(), buffers.result.Get(), false, nullptr);
	return buffers;
}

void DXRTriangleApp::CreateTopLevelAS(const std::vector<std::pair<ComPtr<ID3D12Resource>, Matrix>>& instances)
{
	ComPtr<ID3D12Device5> device5;
	FAILED_CHECK_BREAK(_device->QueryInterface(IID_PPV_ARGS(device5.GetAddressOf())));
	for (size_t i = 0; i < instances.size(); ++i)
	{
		_topLevelASGenerator.AddInstance(instances[i].first.Get()
			, instances[i].second, static_cast<UINT>(i)
			, static_cast<UINT>(0));
	}
	UINT64 scratchSize, resultSize, instanceDescsSize;
	_topLevelASGenerator.ComputeASBufferSizes(device5.Get(), true, &scratchSize, &resultSize, &instanceDescsSize);
	d3dUtil::CreateBuffer(scratchSize,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_COMMON,
		_topLevelASBuffers.scratch, _device.Get());
	d3dUtil::CreateBuffer(resultSize,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
		_topLevelASBuffers.result, _device.Get());
	d3dUtil::CreateUploadBuffer(instanceDescsSize,
		D3D12_RESOURCE_FLAG_NONE,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		_topLevelASBuffers.instanceDesc, _device.Get());

	ComPtr<ID3D12GraphicsCommandList4> commandList4;
	FAILED_CHECK_BREAK(_commandList->QueryInterface(IID_PPV_ARGS(commandList4.GetAddressOf())));

	_topLevelASGenerator.Generate(commandList4.Get(),
		_topLevelASBuffers.scratch.Get(),
		_topLevelASBuffers.result.Get(),
		_topLevelASBuffers.instanceDesc.Get()
	);
}

void DXRTriangleApp::CreateAccelerationStructures()
{
	//bulid bottom as rp, the triangle vertex buffer
	AccelerationStructureBuffers bottomLevelBuffers = CreateBottomLevelAS({{ _vertexBuffer.Get(),3 }});
	_instances = { {bottomLevelBuffers.result,DirectX::XMMatrixIdentity()}};
	CreateTopLevelAS(_instances);
	_commandList->Close();
	ID3D12CommandList* ppCommnadList[] = {_commandList.Get()};
	_commandQueue->ExecuteCommandLists(1, ppCommnadList);
	GPUSync();
	FAILED_CHECK_BREAK(_commandList->Reset(_commandAlloc.Get(), _pso.Get()));
	_bottomLevelAS = bottomLevelBuffers.result;
}

ComPtr<ID3D12RootSignature> DXRTriangleApp::CreateRayGenSignature() const
{
	nv_helpers_dx12::RootSignatureGenerator rsc;
	rsc.AddHeapRangesParameter(
		{
			{ 0,1,0,D3D12_DESCRIPTOR_RANGE_TYPE_UAV,0 },
			{ 0,1,0,D3D12_DESCRIPTOR_RANGE_TYPE_SRV,1 }
		}
	);
	return rsc.Generate(_device.Get(), true);
}

ComPtr<ID3D12RootSignature> DXRTriangleApp::CreateMissSignature() const
{
	nv_helpers_dx12::RootSignatureGenerator rsc;
	return rsc.Generate(_device.Get(), true);
}

ComPtr<ID3D12RootSignature> DXRTriangleApp::CreateHitSignature() const
{
	nv_helpers_dx12::RootSignatureGenerator rsc;
	rsc.AddRootParameter(D3D12_ROOT_PARAMETER_TYPE_SRV);
	return rsc.Generate(_device.Get(), true);
}
void DXRTriangleApp::CreateRaytracingPipeline()
{
	ComPtr<ID3D12Device5> device5;
	FAILED_CHECK_BREAK(_device->QueryInterface(IID_PPV_ARGS(device5.GetAddressOf())));
	nv_helpers_dx12::RayTracingPipelineGenerator pipeline(device5.Get());

	_rayGenLibrary = d3dUtil::CompileShaderLibrary(L"shaders/RayGen.hlsl");
	_missLibrary = d3dUtil::CompileShaderLibrary(L"shaders/Miss.hlsl");
	_hitLibrary = d3dUtil::CompileShaderLibrary(L"shaders/Hit.hlsl");

	// DLL�� �����ϰ�, �� ���̺귯���� export�� �ɺ�(���̴� �Լ� �̸�)�� ��������� ����ؾ� �մϴ�.
	// �Ʒ����� ����� �̸��� HLSL �ڵ忡�� [shader("xxx")] �������� ���ǵ˴ϴ�.
	pipeline.AddLibrary(_rayGenLibrary.Get(), { L"RayGen" });
	pipeline.AddLibrary(_missLibrary.Get(), { L"Miss" });
	pipeline.AddLibrary(_hitLibrary.Get(), { L"ClosestHit" });

	// ��� DX12 ���̴��� ������ �Ķ���Ϳ� ���۸� �����ϴ� ��Ʈ �ñ״�ó�� �ʿ��մϴ�.
	_rayGenSignature = CreateRayGenSignature();
	_missSignature = CreateMissSignature();
	_hitSignature = CreateHitSignature();

	// �� 3������ ���̴��� ���� �׽�Ʈ�� ���� �� �ֽ��ϴ�:
	// - ���ͼ��� ���̴�: ��ﰢ�� ����ü�� ��� �ڽ��� ���� �� ȣ��˴ϴ�.
	//   (�� Ʃ�丮�󿡼��� �ٷ��� ����)
	// - �ִ���Ʈ ���̴�: �������� �浹���� ȣ��Ǹ�, ���� �׽�Ʈ ���� ���� ������ ������ �� �ֽ��ϴ�.
	// - Ŭ���Ʈ��Ʈ ���̴�: ���� �������� ���� ����� �浹 �������� ȣ��˴ϴ�.
	// �� 3���� ���̴��� �ϳ��� ��Ʈ �׷����� ���Դϴ�.

	// �ﰢ�� ����ü�� ���, ���ͼ��� ���̴��� DXR�� ����Ǿ� �ֽ��ϴ�.
	// �⺻������ ��� �ִ� �ִ���Ʈ ���̴��� ���ǵǾ� �����Ƿ�,
	// �ܼ��� ��쿡�� Ŭ���Ʈ��Ʈ ���̴��� ���Ե� ��Ʈ �׷��� ���� �� �ֽ��ϴ�.
	// ������ export�� �̸��� �����ϹǷ�, �ܼ��� �̸������� ���̴��� ������ �� �ֽ��ϴ�.

	// �ﰢ���� ��Ʈ �׷�: �� ���������� �ܼ��� ���� ������ �����մϴ�.
	pipeline.AddHitGroup(L"HitGroup", L"ClosestHit");

	// �Ʒ� ���ǿ����� �� ���̴��� ��Ʈ �ñ״�ó�� �����մϴ�.
	// �Ϻ� ���̴��� ������ ��Ʈ �ñ״�ó�� �����Ѵٴ� ���� ��������� ������ �� �ֽ��ϴ�
	// (��: Miss ���̴��� ShadowMiss ���̴�).
	// ��Ʈ ���̴��� ���� ��Ʈ �׷� �̸��� ����Ͽ� �����ϸ�,
	// �̴� ������ ���ͼ���, �ִ���Ʈ, Ŭ���Ʈ��Ʈ ���̴��� ���� ��Ʈ �ñ״�ó�� �������� �ǹ��մϴ�.
	pipeline.AddRootSignatureAssociation(_rayGenSignature.Get(), { L"RayGen" });
	pipeline.AddRootSignatureAssociation(_missSignature.Get(), { L"Miss" });
	pipeline.AddRootSignatureAssociation(_hitSignature.Get(), { L"HitGroup" });

	// ���̷ε� ũ��� ���̴� �� ���޵Ǵ� ������(HitInfo ����ü ��)�� �ִ� ũ�⸦ �����մϴ�.
	// �� ���� �ʹ� ũ�� ������ ���ʿ��� �޸� ����� ĳ�� ������ �ʷ��ϹǷ� ������ �۰� �����ϴ� ���� �߿��մϴ�.
	pipeline.SetMaxPayloadSize(4 * sizeof(float)); // RGB + �Ÿ�

	// ǥ�鿡 ��Ʈ�� ���, DXR�� ���� �Ӽ��� �����մϴ�.
	// �� ���ÿ����� �ﰢ���� ������ �� ���� �������� ���ǵ� u, v ����ġ�� �̿���
	// �ٸ���Ʈ�� ��ǥ�� ����մϴ�.
	// ��: float3 barycentrics = float3(1.f - u - v, u, v);
	pipeline.SetMaxAttributeSize(2 * sizeof(float)); // �ٸ���Ʈ�� ��ǥ(u, v)

	// ����Ʈ���̽� ���� �� ���� ��Ʈ �������� ���ο� ���̸� ��� ��ø�� TraceRay ȣ���� �����մϴ�.
	// �� ���ÿ����� 1���� ���̸� �����ϹǷ� ��� ���̴� 1�Դϴ�.
	// �� ���̴� ������ ���� ������ ���� �����ϴ� ���� �����ϴ�.
	// Path tracing�� ���� �ݺ������� ��źȭ�� �� �ֽ��ϴ�.
	pipeline.SetMaxRecursionDepth(1);

	// ������������ GPU���� ������ �� �ֵ��� �������մϴ�.
	_rtStateObject = pipeline.Generate();

	// �� StateObject�� ���� ���߿� �̸����� ���̴� �����Ϳ� ������ �� �ְ� �Ӽ� �������̽��� ĳ�����մϴ�.
	FAILED_CHECK_BREAK(_rtStateObject->QueryInterface(IID_PPV_ARGS(&_rtStateObjectProps)));
}

void DXRTriangleApp::CreateRayTracingOutputBuffer()
{
	D3D12_RESOURCE_DESC desc{};
	desc.DepthOrArraySize = 1;
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	desc.Width = GetWidth();
	desc.Height = GetHeight();
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	desc.MipLevels = 1;
	desc.SampleDesc.Count = 1;
	
	D3D12_HEAP_PROPERTIES hp;
	hp.Type = D3D12_HEAP_TYPE_DEFAULT;
	hp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	hp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	hp.CreationNodeMask = 0;
	hp.VisibleNodeMask = 0;
	FAILED_CHECK_BREAK(_device->CreateCommittedResource(
		&hp, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COPY_SOURCE, nullptr,
		IID_PPV_ARGS(_outputResource.GetAddressOf())
	));
}

void DXRTriangleApp::CreateShaderBindingTable()
{
	_sbtHelper.Reset();
	const D3D12_GPU_DESCRIPTOR_HANDLE srvUavHeapHandle =
		_srvUavHeap->GetGPUDescriptorHandleForHeapStart();
	auto heapPointer = reinterpret_cast<UINT64*>(srvUavHeapHandle.ptr);
	_sbtHelper.AddRayGenerationProgram(L"RayGen", { heapPointer });
	_sbtHelper.AddMissProgram(L"Miss", {});
	_sbtHelper.AddHitGroup(L"HitGroup", { reinterpret_cast<void*>(_vertexBuffer->GetGPUVirtualAddress()) });
	const uint32_t sbtSize = _sbtHelper.ComputeSBTSize();

	d3dUtil::CreateUploadBuffer(sbtSize, D3D12_RESOURCE_FLAG_NONE,
		D3D12_RESOURCE_STATE_GENERIC_READ, _sbtStorage, _device.Get());

	if (!_sbtStorage)
	{
		throw std::logic_error("could not alloc the shader binding table");
	}
	_sbtHelper.Generate(_sbtStorage.Get(), _rtStateObjectProps.Get());
}

void DXRTriangleApp::CreateShaderResourceHeap()
{
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
	heapDesc.NumDescriptors = 2;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	
	FAILED_CHECK_BREAK(_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(_srvUavHeap.GetAddressOf())));
	D3D12_CPU_DESCRIPTOR_HANDLE srvHandle =
		_srvUavHeap->GetCPUDescriptorHandleForHeapStart();
	
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	_device->CreateUnorderedAccessView(_outputResource.Get(), nullptr, &uavDesc, srvHandle);

	srvHandle.ptr += _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.RaytracingAccelerationStructure.Location =
		_topLevelASBuffers.result->GetGPUVirtualAddress();
	_device->CreateShaderResourceView(nullptr, &srvDesc, srvHandle);

}

