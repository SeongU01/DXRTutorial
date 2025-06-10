#pragma once
#include "DXRSample.h"
#include "nv_helpers_dx12/TopLevelASGenerator.h"
#include "nv_helpers_dx12/ShaderBindingTableGenerator.h"

using namespace Microsoft::WRL;
using namespace DirectX;
class DXRTriagnelReworkApp : public DXRSample
{
public:
	DXRTriagnelReworkApp(UINT width, UINT height, std::wstring name);
	void Initialize() override;
	void Update(const float& dt) override;
	void Render() override;
	void Flip() override;
	void Finalize() override;
private:
	static const UINT _frameCount = 2;
	struct Vertex
	{
		Vector3 position;
		Vector4 color;
	};
	// com objects
	D3D12_VIEWPORT _viewPort;
	D3D12_RECT _scissorRect;
	ComPtr<IDXGISwapChain4> _swapChain;
	ComPtr<ID3D12Device> _device;
	ComPtr<ID3D12Resource>_renderTargets[_frameCount];
	ComPtr<ID3D12CommandAllocator>_commandAlloc;
	ComPtr<ID3D12CommandQueue> _commandQueue;
	ComPtr<ID3D12RootSignature>_rootSignature;
	ComPtr<ID3D12DescriptorHeap>_rtvHeap;
	ComPtr<ID3D12PipelineState> _pso;
	ComPtr<ID3D12GraphicsCommandList>_commandList;
	UINT _rtvDescriptorSize;


	// sync objects
	UINT _frameIndex;
	HANDLE _fenceEvent;
	ComPtr<ID3D12Fence> _fence;
	UINT64 _fenceValue;
private:
	void LoadPipeline();
	void CreateDXRDeviceAndSwapChain(HWND hwnd, D3D_FEATURE_LEVEL feature);
	void CreateCommandObject();
	void CreateSyncObject();
	void CreateRTVHeapAndRTV();
	void GPUSync();

	void CheckDXRSupport()const;

};