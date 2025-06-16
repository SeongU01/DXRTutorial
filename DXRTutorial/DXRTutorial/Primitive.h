#pragma once

#include "Headers.h"
#include "VertexPositionNormalTangentTexture.h"

class Primitive
{
public:
	struct Shape
	{
		std::vector<VertexPositionNormalTangentTexture> vertexData;
		std::vector<UINT> indexData;
	};
	static Shape CreateSphere(float diameter, int tessellation, bool uvHorizontalFlip = false,
		bool uvVerticalFlip = false);

	static Shape CreateCube(float size, bool uvHorizontalFlip = false, bool uvVerticalFlip = false,
		float uTileFactor = 1, float vTileFactor = 1);
private:
	static void CalculateTangentSpace(Shape& shape);
};