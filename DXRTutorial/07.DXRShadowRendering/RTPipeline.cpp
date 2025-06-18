#include "RTPipeline.h"

const WCHAR* RTPipeline::RayGenShader = L"RayGen";
const WCHAR* RTPipeline::MissShader = L"Miss";
const WCHAR* RTPipeline::ClosestHitShader = L"Closesthit";
const WCHAR* RTPipeline::HitGroup = L"HitGroup";
const WCHAR* RTPipeline::ShadowMiss = L"ShadowMiss";

static dxc::DxcDllSupport gDxcDllHelper;

DxilLibrary RTPipeline::CreateDxilLibrary()
{
    // compile shader
    const ComPtr<IDxcBlob> rayGenshader = CompileLibrary(L"shaders/Shaders.hlsl", L"lib_6_3");
    const WCHAR* entryPoints[] = { RayGenShader,MissShader,ClosestHitShader,ShadowMiss};
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
    
    ComPtr<IDxcIncludeHandler> includeHandler;
    pLibrary->CreateIncludeHandler(includeHandler.GetAddressOf());

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
            includeHandler.Get(),
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
    desc.rootParams[0].DescriptorTable.NumDescriptorRanges = desc.range.size();
    desc.rootParams[0].DescriptorTable.pDescriptorRanges = desc.range.data();

    desc.desc.NumParameters = 1;
    desc.desc.pParameters = desc.rootParams.data();
    desc.desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

    return desc;
}

RTPipeline::RootSignatureDesc RTPipeline::CreateHitRootDesc()
{
    RootSignatureDesc desc;
    desc.range.resize(3);
    //rtscene
	desc.range[0].BaseShaderRegister = 0;
	desc.range[0].NumDescriptors = 1;
	desc.range[0].RegisterSpace = 0;
	desc.range[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	desc.range[0].OffsetInDescriptorsFromTableStart = 1;
    //indices
    desc.range[1].BaseShaderRegister = 1;
    desc.range[1].NumDescriptors = 1;
    desc.range[1].RegisterSpace = 0;
    desc.range[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    desc.range[1].OffsetInDescriptorsFromTableStart = 2;

    //vertices
	desc.range[2].BaseShaderRegister = 2;
	desc.range[2].NumDescriptors = 1;
	desc.range[2].RegisterSpace = 0;
	desc.range[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	desc.range[2].OffsetInDescriptorsFromTableStart = 3;

	desc.rootParams.resize(1);
	desc.rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    desc.rootParams[0].DescriptorTable.NumDescriptorRanges = desc.range.size();
	desc.rootParams[0].DescriptorTable.pDescriptorRanges = desc.range.data();

	// Create the desc
	desc.desc.NumParameters = 1;
	desc.desc.pParameters = desc.rootParams.data();
	desc.desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

	return desc;
}