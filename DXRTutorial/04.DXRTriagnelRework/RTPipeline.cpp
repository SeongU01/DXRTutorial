#include "RTPipeline.h"

const WCHAR* RTPipeline::RayGenShader = L"RayGen";
const WCHAR* RTPipeline::MissShader = L"Miss";
const WCHAR* RTPipeline::TriangleChs = L"TriangleChs";
const WCHAR* RTPipeline::PlaneChs = L"PlaneChs";
const WCHAR* RTPipeline::TriHitGroup = L"TriHitGroup";
const WCHAR* RTPipeline::PlaneHitGroup = L"PlaneHitGroup";
const WCHAR* RTPipeline::ShadowChs = L"ShadowChs";
const WCHAR* RTPipeline::ShadowMiss = L"ShadowMiss";
const WCHAR* RTPipeline::ShadowHitGroup = L"ShadowHitGroup";

static dxc::DxcDllSupport gDxcDllHelper;

DxilLibrary RTPipeline::CreateDxilLibrary()
{
    // compile shader
    const ComPtr<IDxcBlob> rayGenshader = CompileLibrary(L"shaders/Shaders.hlsl", L"lib_6_3");
    const WCHAR* entryPoints[] = { RayGenShader,MissShader,PlaneChs,TriangleChs,ShadowMiss,ShadowChs };
    return DxilLibrary(rayGenshader, entryPoints, ARRAYSIZE(entryPoints));
}

ComPtr<IDxcBlob> RTPipeline::CompileLibrary(const WCHAR* filename, const WCHAR* targetString)
{
    // init helper
    FAILED_CHECK_BREAK(gDxcDllHelper.Initialize());
    ComPtr<IDxcCompiler> pCompiler;
    ComPtr<IDxcLibrary> pLibrary;
    FAILED_CHECK_BREAK(gDxcDllHelper.CreateInstance(CLSID_DxcCompiler, pCompiler.GetAddressOf()));
    FAILED_CHECK_BREAK(gDxcDllHelper.CreateInstance(CLSID_DxcLibrary, pLibrary.GetAddressOf()));
    
    // open and read the file
    const std::ifstream shaderFile(filename);
    if (shaderFile.good() == false)
    {
       // log.
    }
    std::stringstream strStream;
    strStream << shaderFile.rdbuf();
    const std::string shader = strStream.str();
    
    // create blob from the file
    ComPtr<IDxcBlobEncoding> ptextBlob;
    FAILED_CHECK_BREAK(
        pLibrary->CreateBlobWithEncodingFromPinned(
            LPBYTE(shader.c_str()),
            static_cast<uint32_t>(shader.size()),
            0, ptextBlob.GetAddressOf()
        )
    );
    
    // compile
    ComPtr<IDxcOperationResult> pResult;
    FAILED_CHECK_BREAK(
        pCompiler->Compile(
            ptextBlob.Get(),
            filename,
            L"",
            targetString,
            nullptr,
            0,
            nullptr,
            0,
            nullptr,
            pResult.GetAddressOf()
        )
    );
	ComPtr<IDxcBlobEncoding> pErrors;
	if (SUCCEEDED(pResult->GetErrorBuffer(&pErrors)) && pErrors && pErrors->GetBufferSize() > 1)
	{
		OutputDebugStringA((const char*)pErrors->GetBufferPointer());
        // log.
	}

    ComPtr<IDxcBlob> pBlob;
    FAILED_CHECK_BREAK(pResult->GetResult(pBlob.GetAddressOf()));
    return pBlob;
}

RTPipeline::RootSignatureDesc RTPipeline::CreateRayGenRootDesc()
{
    RootSignatureDesc desc;
    desc.range.resize(2);
    // output
    desc.range[0].BaseShaderRegister = 0;
    desc.range[0].NumDescriptors = 1;
    desc.range[0].RegisterSpace = 0;
    desc.range[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    desc.range[0].OffsetInDescriptorsFromTableStart = 0;

    //rtscene
    desc.range[1].BaseShaderRegister = 0;
    desc.range[1].NumDescriptors = 1;
    desc.range[1].RegisterSpace = 0;
    desc.range[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    desc.range[1].OffsetInDescriptorsFromTableStart = 1;
    
    desc.rootParams.resize(1);
    desc.rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    desc.rootParams[0].DescriptorTable.NumDescriptorRanges = 2;
    desc.rootParams[0].DescriptorTable.pDescriptorRanges = desc.range.data();

    desc.desc.NumParameters = 1;
    desc.desc.pParameters = desc.rootParams.data();
    desc.desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

    return desc;
}

RTPipeline::RootSignatureDesc RTPipeline::CreateTriangleHitRootDesc()
{
    RootSignatureDesc desc;
    desc.rootParams.resize(1);
    
    desc.rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    desc.rootParams[0].Descriptor.RegisterSpace = 0;
    desc.rootParams[0].Descriptor.ShaderRegister = 0;
    
    desc.desc.NumParameters = 1;
    desc.desc.pParameters = desc.rootParams.data();
    desc.desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

    return desc;
}

RTPipeline::RootSignatureDesc RTPipeline::CreatePlaneHitRootDesc()
{
    RootSignatureDesc desc;
    desc.range.resize(1);

	desc.range[0].BaseShaderRegister = 0;
	desc.range[0].NumDescriptors = 1;
	desc.range[0].RegisterSpace = 0;
	desc.range[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	desc.range[0].OffsetInDescriptorsFromTableStart = 0;

	desc.rootParams.resize(1);
	desc.rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	desc.rootParams[0].DescriptorTable.NumDescriptorRanges = 1;
	desc.rootParams[0].DescriptorTable.pDescriptorRanges = desc.range.data();

	desc.desc.NumParameters = 1;
	desc.desc.pParameters = desc.rootParams.data();
	desc.desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

	return desc;
}
