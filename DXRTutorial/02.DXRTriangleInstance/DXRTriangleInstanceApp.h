#pragma once
#include "DXRSample.h"
#include "nv_helpers_dx12/TopLevelASGenerator.h"
#include "nv_helpers_dx12/ShaderBindingTableGenerator.h"

using namespace Microsoft::WRL;
using namespace DirectX;
class DXRTriangleInstanceApp : public DXRSample
{
public:
	DXRTriangleInstanceApp(UINT width, UINT height, std::wstring name);
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

	// app resource
	ComPtr<ID3D12Resource> _vertexBuffer;

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
	void LoadAssets();
	void WriteCommand() const;
	void GPUSync();

	void CheckDXRSupport()const;

private:
	// DXR
	struct AccelerationStructureBuffers
	{
		ComPtr<ID3D12Resource> scratch;
		ComPtr<ID3D12Resource> result;
		ComPtr<ID3D12Resource> instanceDesc;
	};

	ComPtr<ID3D12Resource> _bottomLevelAS;
	nv_helpers_dx12::TopLevelASGenerator _topLevelASGenerator;
	AccelerationStructureBuffers _topLevelASBuffers;
	std::vector<std::pair<ComPtr<ID3D12Resource>, Matrix>> _instances;

	/**
	* @brief dCreate the acceleration structure of and instance
	* @param vVertexBuffers pair of BLAS and transform
	* @return
	*/
	AccelerationStructureBuffers CreateBottomLevelAS(
		std::vector<std::pair<ComPtr<ID3D12Resource>, uint32_t>>vVertexBuffers
	)const;

	/**
	 * @brief Create the main acceleration structure that holds all instances of the scene
	 * @param instances
	 */
	void CreateTopLevelAS(
		const std::vector<std::pair<ComPtr<ID3D12Resource>, Matrix>>& instances
	);

	// Create all acceleration structures bottom and top
	void CreateAccelerationStructures();
	// DXR
	ComPtr<ID3D12RootSignature> CreateRayGenSignature() const;
	ComPtr<ID3D12RootSignature> CreateMissSignature() const;
	ComPtr<ID3D12RootSignature> CreateHitSignature() const;

	void CreateRaytracingPipeline();
	// DXR shader
	ComPtr<IDxcBlob> _rayGenLibrary;
	ComPtr<IDxcBlob> _hitLibrary;
	ComPtr<IDxcBlob> _missLibrary;

	ComPtr<ID3D12RootSignature> _rayGenSignature;
	ComPtr<ID3D12RootSignature> _hitSignature;
	ComPtr<ID3D12RootSignature> _missSignature;

	// rt pipeline state
	ComPtr<ID3D12StateObject> _rtStateObject;
	// rt pipeline state properties, retain shader identifiers 
	// to use in the shader binding table
	ComPtr<ID3D12StateObjectProperties> _rtStateObjectProps;
	// DXR
	void CreateRayTracingOutputBuffer();
	void CreateShaderResourceHeap();
	ComPtr<ID3D12Resource> _outputResource;
	ComPtr<ID3D12DescriptorHeap> _srvUavHeap;

	// DXR
	void CreateShaderBindingTable();
	nv_helpers_dx12::ShaderBindingTableGenerator _sbtHelper;
	ComPtr<ID3D12Resource> _sbtStorage;

	// #DXR Extra: Perspective Camera
	void CreateCameraBuffer();
	void UpdateCameraBuffer() const;
	ComPtr<ID3D12Resource> _cameraBuffer;
	ComPtr<ID3D12DescriptorHeap> _constHeap;
	uint32_t _cameraBufferSize = 0;
	DirectX::SimpleMath::Vector4 eye{1.5f,1.5f ,1.5f ,0.f};

	// #DXR Extra: Per-Instance Data
	void CreatePlaneVB();
	ComPtr<ID3D12Resource> _planeBuffer;

	// #DXR Extra: Per-Instance Data
	void CreatePerInstanceConstantBuffers();
	std::vector<ComPtr<ID3D12Resource>> _perInstanceConstantBuffers;
};