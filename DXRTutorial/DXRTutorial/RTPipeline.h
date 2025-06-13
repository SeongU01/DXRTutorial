#pragma once
#include "Headers.h"
#include "DxilLibrary.h"

class RTPipeline
{
public:
	RTPipeline() = default;
	
	struct RootSignatureDesc
	{
		D3D12_ROOT_SIGNATURE_DESC desc{};
		std::vector<D3D12_DESCRIPTOR_RANGE> range;
		std::vector<D3D12_ROOT_PARAMETER> rootParams;
	};
	static const WCHAR* RayGenShader;
	static const WCHAR* MissShader;
	static const WCHAR* TriangleChs;
	static const WCHAR* PlaneChs;
	static const WCHAR* TriHitGroup;
	static const WCHAR* PlaneHitGroup;
	static const WCHAR* ShadowChs;
	static const WCHAR* ShadowMiss;
	static const WCHAR* ShadowHitGroup;

	static DxilLibrary CreateDxilLibrary();
	static ComPtr<IDxcBlob> CompileLibrary(const WCHAR* filename,const WCHAR* targetString);
	
	static RootSignatureDesc CreateRayGenRootDesc();
	static RootSignatureDesc CreateTriangleHitRootDesc();
	static RootSignatureDesc CreatePlaneHitRootDesc();
	
};

