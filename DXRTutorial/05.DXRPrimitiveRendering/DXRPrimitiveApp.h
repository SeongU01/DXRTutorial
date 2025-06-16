#pragma once
#include "DXRSample.h"
#include "nv_helpers_dx12/TopLevelASGenerator.h"
#include "nv_helpers_dx12/ShaderBindingTableGenerator.h"
#include "AccelerationStructure.h"

using namespace Microsoft::WRL;
using namespace DirectX;
class DXRPrimitiveApp : public DXRSample
{
public:
	DXRPrimitiveApp(UINT width, UINT height, std::wstring name);
	void Initialize() override;
	void Update(const float& dt) override;
	void Render() override;
	void Flip() override;
	void Finalize() override;

	static std::vector<ComPtr<ID3D12Resource>> gUploadBuffers;

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
private:
	uint32_t BeginFrame();
	void EndFrame();
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
	void CheckDebug();
		
	void CheckDXRSupport()const;

	//DXR Function
	void CreateAccelerationStructures();
	ComPtr<ID3D12Resource> _bottomLevelAS;
	AccelerationStructureBuffers _topLevelBuffers;
	uint64_t _tlasSize = 0;
	
	void CreateRTPipelineState();
	ComPtr<ID3D12StateObject> _pipelineState;
	ComPtr<ID3D12RootSignature> _emptyRootsignature;
	
	void CreateShaderTable();
	ComPtr<ID3D12Resource> _shaderTable;
	uint32_t _shaderTableEntrySize = 0;

	void CreateShaderResource();
	ComPtr<ID3D12Resource> _outputResource;
	ComPtr<ID3D12DescriptorHeap> _srvuavHeap;


	// app resource
	std::vector<std::shared_ptr<AccelerationStructure::ShapeResource>> _resources;
};