#pragma once

#include "Headers.h"

struct VertexPositionNormalTangentTexture
{
	Vector3 position;
	Vector3 normal;
	Vector3 tangent;
	Vector2 texCoord;

	VertexPositionNormalTangentTexture(
		const Vector3 pos, const Vector3 normal,
		const Vector3 tan, const Vector2 uv
	) :position{ pos }, normal{ normal }, tangent{ tan }, texCoord{ uv }
	{

	}
	VertexPositionNormalTangentTexture() = default;
};