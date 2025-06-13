#pragma once

#include <dxgi1_5.h>
#include <d3dcompiler.h>
#include <directxtk/SimpleMath.h>
#include <dxcapi.h>
#include <Windows.h>
#include <cstdint>
#include <string>
#include <wrl.h>
#include <fstream>
#include <sstream>
#include "dxcapi.use.h"

#include "D3DUtil.h"
#include "d3dx12.h"

#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxgi.lib")

using namespace Microsoft::WRL;
using namespace DirectX::SimpleMath;


#include "Util.h"