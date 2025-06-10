#include "DXRTriagnelReworkApp.h"
#include "Application.h"
#include "../DXRTutorial/nv_helpers_dx12/BottomLevelASGenerator.h"
#include "../DXRTutorial/nv_helpers_dx12/RaytracingPipelineGenerator.h"
#include "../DXRTutorial/nv_helpers_dx12/RootSignatureGenerator.h"

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
}
static float rotate = 0;
void DXRTriagnelReworkApp::Update(const float& dt)
{
}

void DXRTriagnelReworkApp::Render()
{
}

void DXRTriagnelReworkApp::Flip()
{
}

void DXRTriagnelReworkApp::Finalize()
{
}

void DXRTriagnelReworkApp::LoadPipeline()
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
