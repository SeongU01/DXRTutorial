#pragma once
#include "Headers.h"

struct ExportAssociation
{
	ExportAssociation(
		const WCHAR* exportNames[], const uint32_t exportCount,
		const D3D12_STATE_SUBOBJECT* pSubobjectToAssociate
	)
	{
		association.NumExports = exportCount;
		association.pExports = exportNames;
		association.pSubobjectToAssociate = pSubobjectToAssociate;
		subObject.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
		subObject.pDesc = &association;
	}

	D3D12_STATE_SUBOBJECT subObject{};
	D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION association{};
};