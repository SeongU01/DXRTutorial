#include "Primitive.h"
#include <cmath>

Primitive::Shape Primitive::CreateSphere(float diameter, int tessellation,
	bool uvHorizontalFlip, bool uvVerticalFlip)
{
	Shape returnSphereInfo;

	const int verticalSegments = tessellation;
	const int horizontalSegments = tessellation * 2;
	float uIncrement = 1.f / horizontalSegments;
	float vIncrement = 1.f / verticalSegments;
	const float radius = diameter / 2;

	uIncrement *= uvHorizontalFlip ? 1 : -1;
	vIncrement *= uvVerticalFlip ? 1 : -1;

	float u = uvHorizontalFlip ? 0 : 1;
	float v = uvVerticalFlip ? 0 : 1;

	// Start with a single vertex at the bottom of the sphere.
	for (int i = 0; i < horizontalSegments; i++)
	{
		u += uIncrement;

		VertexPositionNormalTangentTexture vertex(
			Vector3(0, -1, 0) * radius,
			Vector3(0, -1, 0),
			Vector3(0,0,0),
			Vector2(u, v)
		);

		//Add it
		returnSphereInfo.vertexData.push_back(vertex);
	}

	// Create rings of vertices at progressively higher latitudes.
	v = uvVerticalFlip ? 0 : 1;
	for (int i = 0; i < verticalSegments - 1; i++)
	{
		const float latitude = (((i + 1) * DirectX::XM_PI) 
			/ verticalSegments) - (DirectX::XM_PI / 2);
		u = uvHorizontalFlip ? 0 : 1;
		v += vIncrement;
		const float dy = sin(latitude);
		const float dxz = cos(latitude);

		// Create a single ring of vertices at this latitude.
		for (int j = 0; j <= horizontalSegments; j++)
		{
			const float longitude = j * DirectX::XM_PI * 2 / horizontalSegments;

			const float dx = cos(longitude) * dxz;
			const float dz = sin(longitude) * dxz;

			Vector3 normal(dx, dy, dz);
			normal.Normalize();
			const Vector2 texCoord(u, v);
			u += uIncrement;

			VertexPositionNormalTangentTexture vertex(
				normal * radius,
				normal,
				Vector3::Zero,
				texCoord
			);

			//Add it
			returnSphereInfo.vertexData.push_back(vertex);
		}
	}

	// Finish with a single vertex at the top of the sphere.
	v = uvVerticalFlip ? 1 : 0;
	u = uvHorizontalFlip ? 0 : 1;
	for (int i = 0; i < horizontalSegments; i++)
	{
		u += uIncrement;

		VertexPositionNormalTangentTexture vertex(
			Vector3(0, 1, 0) * radius,
			Vector3(0, 1, 0),
			Vector3::Zero,
			Vector2(u, v)
		);

		//Add it
		returnSphereInfo.vertexData.push_back(vertex);
	}

	// Create a fan connecting the bottom vertex to the bottom latitude ring.
	for (int i = 0; i < horizontalSegments; i++)
	{
		returnSphereInfo.indexData.push_back(static_cast<unsigned short>(i));

		returnSphereInfo.indexData.push_back(static_cast<unsigned short>(1 + i + horizontalSegments));

		returnSphereInfo.indexData.push_back(static_cast<unsigned short>(i + horizontalSegments));
	}

	// Fill the sphere body with triangles joining each pair of latitude rings.
	for (int i = 0; i < verticalSegments - 2; i++)
	{
		for (int j = 0; j < horizontalSegments; j++)
		{
			const int nextI = i + 1;
			const int nextJ = j + 1;
			const int num = horizontalSegments + 1;

			const int i1 = horizontalSegments + (i * num) + j;
			const int i2 = horizontalSegments + (i * num) + nextJ;
			const int i3 = horizontalSegments + (nextI * num) + j;
			const int i4 = i3 + 1;

			returnSphereInfo.indexData.push_back(static_cast<unsigned short>(i1));

			returnSphereInfo.indexData.push_back(static_cast<unsigned short>(i2));

			returnSphereInfo.indexData.push_back(static_cast<unsigned short>(i3));

			returnSphereInfo.indexData.push_back(static_cast<unsigned short>(i2));

			returnSphereInfo.indexData.push_back(static_cast<unsigned short>(i4));

			returnSphereInfo.indexData.push_back(static_cast<unsigned short>(i3));
		}
	}

	// Create a fan connecting the top vertex to the top latitude ring.
	for (int i = 0; i < horizontalSegments; i++)
	{
		returnSphereInfo.indexData.push_back(static_cast<unsigned short>(returnSphereInfo.vertexData.size() - 1 - i));

		returnSphereInfo.indexData.push_back(
			static_cast<unsigned short>(returnSphereInfo.vertexData.size() - horizontalSegments - 2 - i));

		returnSphereInfo.indexData.push_back(
			static_cast<unsigned short>(returnSphereInfo.vertexData.size() - horizontalSegments - 1 - i));
	}

	CalculateTangentSpace(returnSphereInfo);

	return returnSphereInfo;
}

Primitive::Shape Primitive::CreateCube(float size, bool uvHorizontalFlip,
	bool uvVerticalFlip, float uTileFactor,
	float vTileFactor)
{
	Shape returnSphereInfo;

	const float uCoordMin = uvHorizontalFlip ? uTileFactor : 0;
	const float uCoordMax = uvHorizontalFlip ? 0 : uTileFactor;
	const float vCoordMin = uvVerticalFlip ? vTileFactor : 0;
	const float vCoordMax = uvVerticalFlip ? 0 : vTileFactor;

	const Vector3 normals[] = {
		Vector3(0, 0, 1),
		Vector3(0, 0, -1),
		Vector3(1, 0, 0),
		Vector3(-1, 0, 0),
		Vector3(0, 1, 0),
		Vector3(0, -1, 0),
	};

	const Vector2 texCoord[] = {
		Vector2(uCoordMax, vCoordMax), Vector2(uCoordMin, vCoordMax), Vector2(uCoordMin, vCoordMin),
		Vector2(uCoordMax, vCoordMin),
		Vector2(uCoordMin, vCoordMin), Vector2(uCoordMax, vCoordMin), Vector2(uCoordMax, vCoordMax),
		Vector2(uCoordMin, vCoordMax),
		Vector2(uCoordMax, vCoordMin), Vector2(uCoordMax, vCoordMax), Vector2(uCoordMin, vCoordMax),
		Vector2(uCoordMin, vCoordMin),
		Vector2(uCoordMax, vCoordMin), Vector2(uCoordMax, vCoordMax), Vector2(uCoordMin, vCoordMax),
		Vector2(uCoordMin, vCoordMin),
		Vector2(uCoordMin, vCoordMax), Vector2(uCoordMin, vCoordMin), Vector2(uCoordMax, vCoordMin),
		Vector2(uCoordMax, vCoordMax),
		Vector2(uCoordMax, vCoordMin), Vector2(uCoordMax, vCoordMax), Vector2(uCoordMin, vCoordMax),
		Vector2(uCoordMin, vCoordMin),
	};

	const Vector3 tangents[] = {
		Vector3(1, 0, 0),
		Vector3(-1, 0, 0),
		Vector3(0, 0, -1),
		Vector3(0, 0, 1),
		Vector3(1, 0, 0),
		Vector3(1, 0, 0),
	};

	// Create each face in turn.
	for (int i = 0, j = 0; i < _countof(normals); i++, j += 4)
	{
		Vector3 normal = normals[i];
		Vector3 tangent = tangents[i];

		// Get two vectors perpendicular to the face normal and to each other.
		Vector3 side1 = Vector3(normal.y, normal.z, normal.x);
		Vector3 side2 = normal.Cross(side1);

		const int vertexCount = returnSphereInfo.vertexData.size();
		// Six indices (two triangles) per face.
		returnSphereInfo.indexData.push_back(static_cast<unsigned short>(vertexCount + 0));

		returnSphereInfo.indexData.push_back(static_cast<unsigned short>(vertexCount + 1));

		returnSphereInfo.indexData.push_back(static_cast<unsigned short>(vertexCount + 3));

		returnSphereInfo.indexData.push_back(static_cast<unsigned short>(vertexCount + 1));

		returnSphereInfo.indexData.push_back(static_cast<unsigned short>(vertexCount + 2));

		returnSphereInfo.indexData.push_back(static_cast<unsigned short>(vertexCount + 3));

		// 0   3
		// 1   2
		const float sideOverTwo = size * 0.5f;

		// Four vertices per face.
		returnSphereInfo.vertexData.emplace_back(
			(normal - side1 - side2) * sideOverTwo,
			normal,
			tangent,
			texCoord[j]
		);

		returnSphereInfo.vertexData.emplace_back(
			(normal - side1 + side2) * sideOverTwo,
			normal,
			tangent,
			texCoord[j + 1]
		);

		returnSphereInfo.vertexData.emplace_back(
			(normal + side1 + side2) * sideOverTwo,
			normal,
			tangent,
			texCoord[j + 2]
		);

		returnSphereInfo.vertexData.emplace_back(
			(normal + side1 - side2) * sideOverTwo,
			normal,
			tangent,
			texCoord[j + 3]
		);
	}

	CalculateTangentSpace(returnSphereInfo);

	return returnSphereInfo;
}

void Primitive::CalculateTangentSpace(Shape& shape)
{
	const int vertexCount = shape.vertexData.size();
	const int triangleCount = shape.indexData.size() / 3;

	Vector3* tan1 = new Vector3[vertexCount * 2];
	Vector3* tan2 = tan1 + vertexCount;

	VertexPositionNormalTangentTexture a1, a2, a3;
	Vector3 v1, v2, v3;
	Vector2 w1, w2, w3;

	for (int a = 0; a < triangleCount; a++)
	{
		const unsigned short i1 = shape.indexData[(a * 3) + 0];
		const unsigned short i2 = shape.indexData[(a * 3) + 1];
		const unsigned short i3 = shape.indexData[(a * 3) + 2];

		a1 = shape.vertexData[i1];
		a2 = shape.vertexData[i2];
		a3 = shape.vertexData[i3];

		v1 = a1.position;
		v2 = a2.position;
		v3 = a3.position;

		w1 = a1.texCoord;
		w2 = a2.texCoord;
		w3 = a3.texCoord;

		float x1 = v2.x - v1.x;
		float x2 = v3.x - v1.x;
		float y1 = v2.y - v1.y;
		float y2 = v3.y - v1.y;
		float z1 = v2.z - v1.z;
		float z2 = v3.z - v1.z;

		float s1 = w2.x - w1.x;
		float s2 = w3.x - w1.x;
		float t1 = w2.y - w1.y;
		float t2 = w3.y - w1.y;

		const float r = 1.0F / ((s1 * t2) - (s2 * t1));
		Vector3 sdir(((t2 * x1) - (t1 * x2)) * r, ((t2 * y1) - (t1 * y2)) * r, ((t2 * z1) - (t1 * z2)) * r);
		Vector3 tdir(((s1 * x2) - (s2 * x1)) * r, ((s1 * y2) - (s2 * y1)) * r, ((s1 * z2) - (s2 * z1)) * r);

		tan1[i1] += sdir;
		tan1[i2] += sdir;
		tan1[i3] += sdir;

		tan2[i1] += tdir;
		tan2[i2] += tdir;
		tan2[i3] += tdir;
	}

	for (int a = 0; a < vertexCount; a++)
	{
		VertexPositionNormalTangentTexture vertex = shape.vertexData[a];

		const Vector3 n = vertex.normal;
		const Vector3 t = tan1[a];

		// Gram-Schmidt orthogonalize
		vertex.tangent = t - (n * n.Dot(t));
		vertex.tangent.Normalize();

		shape.vertexData[a] = vertex;
	}
}


Primitive::Shape Primitive::CreateQuad(const int size,
	const bool uvHorizontalFlip,
	const bool uvVerticalFlip,
	const float uTileFactor,
	const float vTileFactor)
{
	Shape returnSphereInfo;

	returnSphereInfo.vertexData =
	{
		VertexPositionNormalTangentTexture(Vector3(-size, 0, -size),
													Vector3(0, 1, 0),
													Vector3(0,0,0),
													Vector2(0,0)),
		VertexPositionNormalTangentTexture(Vector3(size, 0, -size),
													Vector3(0, 1, 0),
													Vector3(0,0,0),
													Vector2(0,0)),
		VertexPositionNormalTangentTexture(Vector3(size, 0, size),
													Vector3(0, 1, 0),
													Vector3(0,0,0),
													Vector2(0,0)),
		VertexPositionNormalTangentTexture(Vector3(-size, 0, size),
													Vector3(0, 1, 0),
													Vector3(0,0,0),
													Vector2(0,0)),
	};

	returnSphereInfo.indexData = { 0, 1, 2, 0, 2, 3 };

	return returnSphereInfo;
}