#include "D3DUtil.h"
#include "Util.h"


Microsoft::WRL::ComPtr<ID3D12Resource> d3dUtil::CreateDefaultBuffer(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, const void* initData, UINT64 byteSize, Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer)
{
	ComPtr<ID3D12Resource>  defaultBuffer;
	CD3DX12_HEAP_PROPERTIES heapPropertyDefault(D3D12_HEAP_TYPE_DEFAULT);
	CD3DX12_HEAP_PROPERTIES heapPropertyUPLOAD(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC   defaultBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(byteSize);
	CD3DX12_RESOURCE_DESC   uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(byteSize);
	// ���� �⺻ ���� �ڿ��� �����Ѵ�
	FAILED_CHECK_BREAK(device->CreateCommittedResource(&heapPropertyDefault, D3D12_HEAP_FLAG_NONE, &defaultBufferDesc,
		D3D12_RESOURCE_STATE_COMMON, nullptr,
		IID_PPV_ARGS(defaultBuffer.GetAddressOf())));

	// CPU �޸��� �ڷḦ �⺻ ���ۿ� ����
	// �ӽ� ���ε� ���� ����
	FAILED_CHECK_BREAK(device->CreateCommittedResource(&heapPropertyUPLOAD, D3D12_HEAP_FLAG_NONE, &uploadBufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(uploadBuffer.GetAddressOf())));

	// �⺻ ���ۿ� ������ �ڷḦ ����
	D3D12_SUBRESOURCE_DATA subResourceData = {};
	subResourceData.pData = initData;
	subResourceData.RowPitch = byteSize;
	subResourceData.SlicePitch = subResourceData.RowPitch;

	// �⺻ ���� �ڿ������� �ڷ� ���縦 ��û
	// ���������� �����ڸ�, �����Լ� UpdateSubResources�� CPU �޸𸮸�
	// �ӽ� ���ε� ���� �����ϰ�, ID3D12CommandList::CopySubresourceRegion��
	// �̿��ؼ� �ӽ� ���ε� ���� �ڷḦ mBuffer�� ����.
	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		defaultBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);

	cmdList->ResourceBarrier(1, &barrier);
	UpdateSubresources<1>(cmdList, defaultBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &subResourceData);
	barrier = CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_GENERIC_READ);
	cmdList->ResourceBarrier(1, &barrier);

	// Note: uploadBuffer has to be kept alive after the above function calls
	// because the command list has not been executed yet that performs the actual
	// copy. The caller can Release the uploadBuffer after it knows the copy has
	// been executed.

	return defaultBuffer;
}

void d3dUtil::CreateBuffer(UINT size, const D3D12_RESOURCE_FLAGS flags, const D3D12_RESOURCE_STATES initState,
	ComPtr<ID3D12Resource>& buffer,ID3D12Device* device)
{
	D3D12_HEAP_PROPERTIES hp = {};
	hp.Type = D3D12_HEAP_TYPE_DEFAULT;
	hp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	hp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	hp.CreationNodeMask = 0;
	hp.VisibleNodeMask = 0;

	// ���ۿ� ����� �ڿ� ���� ����
	D3D12_RESOURCE_DESC rd = {};
	rd.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER; // �ڿ� ���� : "����"
	rd.Alignment = 0;    // �⺻ ���� 64KB (D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT)
	rd.Width = size; // ������ �ڿ�(������)�� ũ��.
	rd.Height = 1;
	rd.DepthOrArraySize = 1;
	rd.MipLevels = 1;
	rd.Format = DXGI_FORMAT_UNKNOWN;
	rd.SampleDesc.Count = 1;
	rd.SampleDesc.Quality = 0;
	rd.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	rd.Flags = flags;

	// ���� ����.
	ID3D12Resource* pBuff = nullptr;

	FAILED_CHECK_BREAK(device->CreateCommittedResource(&hp, D3D12_HEAP_FLAG_NONE, &rd, initState, nullptr,
		IID_PPV_ARGS(buffer.GetAddressOf())));
}

void d3dUtil::CreateUploadBuffer(UINT size, const D3D12_RESOURCE_FLAGS flags, const D3D12_RESOURCE_STATES initState,
	ComPtr<ID3D12Resource>& buffer, ID3D12Device* device)
{
	D3D12_HEAP_PROPERTIES hp = {};
	hp.Type = D3D12_HEAP_TYPE_UPLOAD;
	hp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	hp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	hp.CreationNodeMask = 0;
	hp.VisibleNodeMask = 0;

	// ���ۿ� ����� �ڿ� ���� ����
	D3D12_RESOURCE_DESC rd = {};
	rd.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER; // �ڿ� ���� : "����"
	rd.Alignment = 0;    // �⺻ ���� 64KB (D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT)
	rd.Width = size; // ������ �ڿ�(������)�� ũ��.
	rd.Height = 1;
	rd.DepthOrArraySize = 1;
	rd.MipLevels = 1;
	rd.Format = DXGI_FORMAT_UNKNOWN;
	rd.SampleDesc.Count = 1;
	rd.SampleDesc.Quality = 0;
	rd.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	rd.Flags = flags;

	// ���� ����.
	ID3D12Resource* pBuff = nullptr;

	FAILED_CHECK_BREAK(device->CreateCommittedResource(&hp, D3D12_HEAP_FLAG_NONE, &rd, initState, nullptr,
		IID_PPV_ARGS(buffer.GetAddressOf())));
}

IDxcBlob* d3dUtil::CompileShaderLibrary(LPCWSTR fileName)
{
	static IDxcCompiler* pCompiler = nullptr;
	static IDxcLibrary* pLibrary = nullptr;
	static IDxcIncludeHandler* dxcIncludeHandler;

	HRESULT hr;

	// Initialize the DXC compiler and compiler helper
	if (!pCompiler)
	{
		FAILED_CHECK_BREAK(DxcCreateInstance(CLSID_DxcCompiler, __uuidof(IDxcCompiler), reinterpret_cast<void**>(&pCompiler)));
		FAILED_CHECK_BREAK(DxcCreateInstance(CLSID_DxcLibrary, __uuidof(IDxcLibrary), reinterpret_cast<void**>(&pLibrary)));
		FAILED_CHECK_BREAK(pLibrary->CreateIncludeHandler(&dxcIncludeHandler));
	}
	// Open and read the file
	std::ifstream shaderFile(fileName);
	if (shaderFile.good() == false)
	{
		throw std::logic_error("Cannot find shader file");
	}
	std::stringstream strStream;
	strStream << shaderFile.rdbuf();
	std::string sShader = strStream.str();

	// Create blob from the string
	IDxcBlobEncoding* pTextBlob;
	FAILED_CHECK_BREAK(pLibrary->CreateBlobWithEncodingFromPinned(
		LPBYTE(sShader.c_str()), static_cast<uint32_t>(sShader.size()), 0, &pTextBlob));

	// Compile
	IDxcOperationResult* pResult;
	FAILED_CHECK_BREAK(pCompiler->Compile(pTextBlob, fileName, L"", L"lib_6_3", nullptr, 0, nullptr, 0,
		dxcIncludeHandler, &pResult));

	// Verify the result
	HRESULT resultCode;
	FAILED_CHECK_BREAK(pResult->GetStatus(&resultCode));
	if (FAILED(resultCode))
	{
		IDxcBlobEncoding* pError;
		hr = pResult->GetErrorBuffer(&pError);
		if (FAILED(hr))
		{
			throw std::logic_error("Failed to get shader compiler error");
		}

		// Convert error blob to a string
		std::vector<char> infoLog(pError->GetBufferSize() + 1);
		memcpy(infoLog.data(), pError->GetBufferPointer(), pError->GetBufferSize());
		infoLog[pError->GetBufferSize()] = 0;

		std::string errorMsg = "Shader Compiler Error:\n";
		errorMsg.append(infoLog.data());

		MessageBoxA(nullptr, errorMsg.c_str(), "Error!", MB_OK);
		throw std::logic_error("Failed compile shader");
	}

	IDxcBlob* pBlob;
	FAILED_CHECK_BREAK(pResult->GetResult(&pBlob));
	return pBlob;
}

HRESULT d3dUtil::UpdateBuffer(const ComPtr<ID3D12Resource>& buffer, void* data, UINT size)
{
	HRESULT hr = S_OK;
	if (nullptr == data)
		return hr;

	uint8_t* temp = nullptr;
	hr = buffer->Map(0, nullptr, (void**)&temp);

	if (FAILED(hr) || nullptr == temp)
	{
		__debugbreak();
		return hr;
	}

	memcpy(temp, data, size);
	buffer->Unmap(0, nullptr);

	return hr;
}
