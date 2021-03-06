// Unique vertices finding part was reference to assimp implementation.
// See: https://github.com/assimp/assimp/blob/master/code/JoinVerticesProcess.cpp
// License: BSD-License

// Icosphere mesh creation refered to:
// http://blog.andreaskahler.com/2009/06/creating-icosphere-mesh-in-code.html

#include <algorithm>

#include "MathUtil.h"
#include "Log.h"
#include "Mesh.h"
#include "MeshUtil.h"

namespace fury
{
	std::shared_ptr<Mesh> MeshUtil::m_UnitQuad = nullptr;
	std::shared_ptr<Mesh> MeshUtil::m_UnitCube = nullptr;
	std::shared_ptr<Mesh> MeshUtil::m_UnitIcoSphere = nullptr;
	std::shared_ptr<Mesh> MeshUtil::m_UnitSphere = nullptr;
	std::shared_ptr<Mesh> MeshUtil::m_UnitCylinder = nullptr;
	std::shared_ptr<Mesh> MeshUtil::m_UnitCone = nullptr;

	std::shared_ptr<Mesh> MeshUtil::GetUnitCube() 
	{
		return m_UnitCube;
	}

	std::shared_ptr<Mesh> MeshUtil::GetUnitQuad() 
	{
		return m_UnitQuad;
	}

	std::shared_ptr<Mesh> MeshUtil::GetUnitSphere() 
	{
		return m_UnitSphere;
	}

	std::shared_ptr<Mesh> MeshUtil::GetUnitIcoSphere() 
	{
		return m_UnitIcoSphere;
	}

	std::shared_ptr<Mesh> MeshUtil::GetUnitCylinder() 
	{
		return m_UnitCylinder;
	}

	std::shared_ptr<Mesh> MeshUtil::GetUnitCone() 
	{
		return m_UnitCone;
	}

	// mesh creation

	std::shared_ptr<Mesh> MeshUtil::CreateQuad(const std::string &name, Vector4 min, Vector4 max)
	{
		Mesh::Ptr mesh = Mesh::Create(name);

		// 1----0
		// |    |
		// 2----3
		mesh->Positions.Data = { max.x, max.y, max.z, min.x, max.y, max.z, min.x, min.y, min.z, max.x, min.y, min.z };
		mesh->Indices.Data = { 0, 1, 2, 2, 3, 0 };
		mesh->UVs.Data = { 1, 1, 0, 1, 0, 0, 1, 0 };

		FURYD << mesh->GetName() << " [vtx: " << mesh->Positions.Data.size() / 3 << " tris: " << mesh->Indices.Data.size() / 3 << "]";

		mesh->CalculateAABB();

		return mesh;
	}

	std::shared_ptr<Mesh> MeshUtil::CreateCube(const std::string &name, Vector4 min, Vector4 max)
	{
		Mesh::Ptr mesh = Mesh::Create(name);
		mesh->Positions.Data = {
			// FTR, FTL, FBL, FBR
			max.x, max.y, max.z,
			min.x, max.y, max.z,
			min.x, min.y, max.z,
			max.x, min.y, max.z,
			// BTR, BTL, BBL, BBR
			max.x, max.y, min.z,
			min.x, max.y, min.z,
			min.x, min.y, min.z,
			max.x, min.y, min.z
		};
		mesh->Indices.Data = {
			// front
			//0, 1, 2, 2, 3, 0,
			0, 3, 2, 2, 1, 0,
			// back
			//4, 7, 6, 6, 5, 4,
			4, 5, 6, 6, 7, 4,
			// left
			//2, 1, 5, 5, 6, 2,
			2, 6, 5, 5, 1, 2,
			// right
			//4, 0, 3, 3, 7, 4,
			4, 7, 3, 3, 0, 4,
			// top
			//4, 5, 1, 1, 0, 4,
			4, 0, 1, 1, 5, 4,
			// bottom
			//2, 6, 7, 7, 3, 2
			2, 3, 7, 7, 6, 2
		};

		FURYD << mesh->GetName() << " [vtx: " << mesh->Positions.Data.size() / 3 << " tris: " << mesh->Indices.Data.size() / 3 << "]";

		mesh->CalculateAABB();

		return mesh;
	}

	std::shared_ptr<Mesh> MeshUtil::CreateIcoSphere(const std::string &name, float radius, int level)
	{
		Mesh::Ptr mesh = Mesh::Create(name);

		std::unordered_map<long long, unsigned int> middlePointIndexCache;
		unsigned int index = 0;

		auto AddVertex = [&mesh, &index, &radius](float x, float y, float z) -> unsigned int
		{
			float inverse_length = radius / std::sqrt(x * x + y * y + z * z);
			mesh->Positions.Data.insert(mesh->Positions.Data.end(), { x * inverse_length, y * inverse_length, z * inverse_length });
			return index++;
		};

		auto GetMidPoint = [&mesh, &middlePointIndexCache, &AddVertex](int p1, int p2) -> unsigned int
		{
			// first check if we have it already
			bool firstIsSmaller = p1 < p2;
			long long smallerIndex = firstIsSmaller ? p1 : p2;
			long long greaterIndex = firstIsSmaller ? p2 : p1;
			long long key = (smallerIndex << 32) + greaterIndex;

			auto it = middlePointIndexCache.find(key);
			if (it != middlePointIndexCache.end())
				return it->second;

			// not in cache, calculate it
			float p1x = mesh->Positions.Data[p1 * 3];
			float p1y = mesh->Positions.Data[p1 * 3 + 1];
			float p1z = mesh->Positions.Data[p1 * 3 + 2];
			float p2x = mesh->Positions.Data[p2 * 3];
			float p2y = mesh->Positions.Data[p2 * 3 + 1];
			float p2z = mesh->Positions.Data[p2 * 3 + 2];

			// add middle vertex makes sure point is on unit sphere
			unsigned int i = AddVertex((p1x + p2x) * 0.5f, (p1y + p2y) * 0.5f, (p1z + p2z) * 0.5f);

			// store it, return index
			middlePointIndexCache.emplace(key, i);
			return i;
		};

		// create 12 vertices of a icosahedron
		float t = (1 + std::sqrt(5.0f)) * 0.5f;

		AddVertex(-1, t, 0);
		AddVertex(1, t, 0);
		AddVertex(-1, -t, 0);
		AddVertex(1, -t, 0);

		AddVertex(0, -1, t);
		AddVertex(0, 1, t);
		AddVertex(0, -1, -t);
		AddVertex(0, 1, -t);

		AddVertex(t, 0, -1);
		AddVertex(t, 0, 1);
		AddVertex(-t, 0, -1);
		AddVertex(-t, 0, 1);

		// create 20 triangles of the icosahedron
		mesh->Indices.Data = {
			// 5 faces around point 0
			0, 11, 5, 0, 5, 1, 0, 1, 7, 0, 7, 10, 0, 10, 11,
			// 5 adjacent faces 
			1, 5, 9, 5, 11, 4, 11, 10, 2, 10, 7, 6, 7, 1, 8,
			// 5 faces around point 3
			3, 9, 4, 3, 4, 2, 3, 2, 6, 3, 6, 8, 3, 8, 9,
			// 5 adjacent faces 
			4, 9, 5, 2, 4, 11, 6, 2, 10, 8, 6, 7, 9, 8, 1
		};

		// refine triangles
		std::vector<unsigned int> newIndices;
		unsigned int oldSize = mesh->Indices.Data.size();

		for (int i = 0; i < level; i++)
		{
			// allocate memory
			newIndices.reserve(oldSize * 4);
			newIndices.erase(newIndices.begin(), newIndices.end());

			unsigned int numIndices = mesh->Indices.Data.size() / 3;
			for (unsigned int j = 0; j < numIndices; j++)
			{
				unsigned int index1 = mesh->Indices.Data[j * 3];
				unsigned int index2 = mesh->Indices.Data[j * 3 + 1];
				unsigned int index3 = mesh->Indices.Data[j * 3 + 2];

				unsigned int a = GetMidPoint(index1, index2);
				unsigned int b = GetMidPoint(index2, index3);
				unsigned int c = GetMidPoint(index3, index1);

				newIndices.insert(newIndices.end(), { index1, a, c, index2, b, a, index3, c, b, a, b, c });
			}

			oldSize = newIndices.size();
			mesh->Indices.Data = newIndices;
		}

		FURYD << mesh->GetName() << " [vtx: " << mesh->Positions.Data.size() / 3 << " tris: " << mesh->Indices.Data.size() / 3 << "]";

		mesh->CalculateAABB();

		return mesh;
	}

	std::shared_ptr<Mesh> MeshUtil::CreateSphere(const std::string &name, float radius, int segH, int segV)
	{
		if (segH < 2 || segV < 3)
		{
			FURYW << "SegH or SegV tooooo small!";
			return nullptr;
		}

		Mesh::Ptr mesh = Mesh::Create(name);

		float avgRadianH = MathUtil::PI / (segH - 1);
		float avgRadianV = MathUtil::PI * 2.0f / segV;

		float currentHeight = radius;
		float currentRadius = 0.0f;

		std::vector<unsigned int> previousRing;
		previousRing.reserve(segV * 3);

		std::vector<unsigned int> currentRing;
		currentRing.reserve(segV * 3);

		/*if (inclusive)
			radius = radius / std::cos(avgRadianH * 0.5f);*/

		unsigned int index = 0;
		auto AddVertex = [&currentRing, &mesh, &index](float x, float y, float z) -> unsigned int
		{
			mesh->Positions.Data.insert(mesh->Positions.Data.end(), { x, y, z });
			return index++;
		};

		for (int h = 0; h < segH; h++)
		{
			currentRadius = std::sin(h * avgRadianH) * radius;
			currentHeight = std::cos(h * avgRadianH) * radius;

			// fill current ring
			for (int v = 0; v < segV; v++)
			{
				float radian = avgRadianV * v;
				currentRing.push_back(AddVertex(
					cos(radian) * currentRadius, currentHeight, sin(radian) * currentRadius));
			}

			if (previousRing.size() > 0)
			{
				// has previous ring, we connect them to triangles.
				for (unsigned int i = 0; i < currentRing.size() - 1; i++)
				{
					mesh->Indices.Data.insert(mesh->Indices.Data.end(), {
						previousRing[i], previousRing[i + 1], currentRing[i + 1],
						currentRing[i + 1], currentRing[i], previousRing[i]
					});
				}
				mesh->Indices.Data.insert(mesh->Indices.Data.end(), {
					previousRing.back(), previousRing.front(), currentRing.front(),
					currentRing.front(), currentRing.back(), previousRing.back()
				});
			}
			else
			{
				// don't have previous ring, then we're on top.
				// close this ring.
				unsigned int center = AddVertex(0, currentHeight, 0);
				for (unsigned int i = 0; i < currentRing.size() - 1; i++)
				{
					mesh->Indices.Data.insert(mesh->Indices.Data.end(), { center, currentRing[i + 1], currentRing[i] });
				}
				mesh->Indices.Data.insert(mesh->Indices.Data.end(), { center, currentRing.front(), currentRing.back() });
			}

			previousRing = currentRing;
			currentRing.erase(currentRing.begin(), currentRing.end());
		}

		// close bottom ring
		unsigned int center = AddVertex(0, -radius, 0);
		for (unsigned int i = 0; i < previousRing.size() - 1; i++)
		{
			mesh->Indices.Data.insert(mesh->Indices.Data.end(), { center, previousRing[i], previousRing[i + 1] });
		}
		mesh->Indices.Data.insert(mesh->Indices.Data.end(), { center, previousRing.back(), previousRing.front() });

		FURYD << mesh->GetName() << " [vtx: " << mesh->Positions.Data.size() / 3 << " tris: " << mesh->Indices.Data.size() / 3 << "]";

		OptimizeMesh(mesh);

		mesh->CalculateAABB();

		return mesh;
	}

	std::shared_ptr<Mesh> MeshUtil::CreateCylinder(const std::string &name, float topR, float bottomR, float height, int segH, int segV)
	{
		if (segH < 2 || segV < 3)
		{
			FURYW << "SegH or SegV tooooo small!";
			return nullptr;
		}

		Mesh::Ptr mesh = Mesh::Create(name);

		// allocate some space.
		mesh->Positions.Data.reserve((segV * segH + 2) * 3);
		mesh->Indices.Data.reserve((segV * segH * 2) * 3);

		float avgRadian = MathUtil::PI * 2.0f / segV;

		/*if (inclusive)
		{
			topR = topR / std::cos(avgRadian * 0.5f);
			bottomR = bottomR / std::cos(avgRadian * 0.5f);
		}*/

		float currentHeight = height / 2.0f;
		float currentRadius = topR;

		float heightStep = height / (segH - 1);
		float radiusSetp = (topR - bottomR) / (segH - 1);

		std::vector<unsigned int> previousRing;
		previousRing.reserve(segV * 3);

		std::vector<unsigned int> currentRing;
		currentRing.reserve(segV * 3);

		unsigned int index = 0;
		auto AddVertex = [&currentRing, &mesh, &index](float x, float y, float z) -> unsigned int
		{
			mesh->Positions.Data.insert(mesh->Positions.Data.end(), { x, y, z });
			return index++;
		};

		for (int h = 0; h < segH; h++)
		{
			// fill current ring
			for (int v = 0; v < segV; v++)
			{
				float radian = avgRadian * v;
				currentRing.push_back(AddVertex(
					cos(radian) * currentRadius, currentHeight, sin(radian) * currentRadius));
			}

			if (previousRing.size() > 0)
			{
				// has previous ring, we connect them to triangles.
				for (unsigned int i = 0; i < currentRing.size() - 1; i++)
				{
					mesh->Indices.Data.insert(mesh->Indices.Data.end(), {
						previousRing[i], previousRing[i + 1], currentRing[i + 1],
						currentRing[i + 1], currentRing[i], previousRing[i]
					});
				}
				mesh->Indices.Data.insert(mesh->Indices.Data.end(), {
					previousRing.back(), previousRing.front(), currentRing.front(),
					currentRing.front(), currentRing.back(), previousRing.back()
				});
			}
			else
			{
				// don't have previous ring, then we're on top.
				// close this ring.
				unsigned int center = AddVertex(0, currentHeight, 0);
				for (unsigned int i = 0; i < currentRing.size() - 1; i++)
				{
					mesh->Indices.Data.insert(mesh->Indices.Data.end(), { center, currentRing[i + 1], currentRing[i] });
				}
				mesh->Indices.Data.insert(mesh->Indices.Data.end(), { center, currentRing.front(), currentRing.back() });
			}

			previousRing = currentRing;
			currentRing.erase(currentRing.begin(), currentRing.end());
			currentHeight -= heightStep;
			currentRadius -= radiusSetp;
		}

		// close bottom ring
		unsigned int center = AddVertex(0, -height / 2.0f, 0);
		for (unsigned int i = 0; i < previousRing.size() - 1; i++)
		{
			mesh->Indices.Data.insert(mesh->Indices.Data.end(), { center, previousRing[i], previousRing[i + 1] });
		}
		mesh->Indices.Data.insert(mesh->Indices.Data.end(), { center, previousRing.back(), previousRing.front() });

		FURYD << mesh->GetName() << " [vtx: " << mesh->Positions.Data.size() / 3 << " tris: " << mesh->Indices.Data.size() / 3 << "]";

		if (topR == 0 || bottomR == 0)
			OptimizeMesh(mesh);

		mesh->CalculateAABB();

		return mesh;
	}

	void MeshUtil::TransformMesh(const std::shared_ptr<Mesh> &mesh, const Matrix4 &matrix, bool updateBuffer) 
	{
		unsigned int count = mesh->Positions.Data.size();
		if (count == 0) return;

		count = count / 3;

		bool hasNormal = mesh->Normals.Data.size() > 0;
		bool hasTangent = mesh->Tangents.Data.size() > 0;

		for (unsigned int i = 0; i < count; i++)
		{
			unsigned int j = i * 3;

			Vector4 pos(mesh->Positions.Data[j], mesh->Positions.Data[j + 1], mesh->Positions.Data[j + 2], 1);

			pos = matrix.Multiply(pos);
			mesh->Positions.Data[j] = pos.x;
			mesh->Positions.Data[j + 1] = pos.y;
			mesh->Positions.Data[j + 2] = pos.z;

			if (hasNormal)
			{
				Vector4 normal(mesh->Normals.Data[j], mesh->Normals.Data[j + 1], mesh->Normals.Data[j + 2], 0);

				normal = matrix.Multiply(normal).Normalized();
				mesh->Normals.Data[j] = normal.x;
				mesh->Normals.Data[j + 1] = normal.y;
				mesh->Normals.Data[j + 2] = normal.z;
			}

			if (hasTangent)
			{
				Vector4 tangent(mesh->Tangents.Data[j], mesh->Tangents.Data[j + 1], mesh->Tangents.Data[j + 2], 0);

				tangent = matrix.Multiply(tangent).Normalized();
				mesh->Tangents.Data[j] = tangent.x;
				mesh->Tangents.Data[j + 1] = tangent.y;
				mesh->Tangents.Data[j + 2] = tangent.z;
			}
		}

		if (updateBuffer)
		{
			mesh->Positions.UpdateBuffer(true);
			if (hasNormal)
				mesh->Normals.UpdateBuffer(true);
			if (hasTangent)
				mesh->Tangents.UpdateBuffer(true);
		}
	}

	void MeshUtil::OptimizeMesh(const std::shared_ptr<Mesh> &mesh)
	{
		// structs && funcs for faster unique vertex finding.

		struct Vertex
		{
			Vector4 Position;

			Vector4 UV;

			Vector4 Normal;

			Vector4 Tangent;

			unsigned int IDs[4];

			float Weights[3];

			bool HasSameWeights(const float *another, float epsilon)
			{
				return !(std::abs(Weights[0] - another[0]) > epsilon ||
					std::abs(Weights[1] - another[1]) > epsilon ||
					std::abs(Weights[2] - another[2]) > epsilon);
			}

			bool HasSameIDs(const unsigned int *another)
			{
				return IDs[0] == another[0] && IDs[1] == another[1] &&
					IDs[2] == another[2] && IDs[3] == another[3];
			}

			void Empty()
			{
				Position.Zero();
				UV.Zero();
				Normal.Zero();
				Tangent.Zero();
				IDs[0] = IDs[1] = IDs[2] = IDs[3] = 0;
				Weights[0] = Weights[1] = Weights[2] = 0.0f;
			}
		};

		struct VtxEntry
		{
			// reference vertex's index
			int index;

			Vector4 position;

			// distance to storing plane
			float dist;

			VtxEntry(int index, Vector4 position, float dist)
				: index(index), position(position), dist(dist) {}

			inline bool operator < (const VtxEntry& other) const
			{
				return dist < other.dist;
			}
		};

		// read physical data.
		int verticesCount = mesh->Positions.Data.size() / 3;

		std::vector<VtxEntry> entries;
		entries.reserve(verticesCount);

		// dividePlane is for spatial sort.
		// we setup a non-usual plane as a reference plane.
		// we use vertex's distance to the plane to search near vertices faster.
		Vector4 dividePlane(0.6f, 0.7f, -0.4f, 1.0f);
		dividePlane.Normalize();

		bool hasNormal = mesh->Normals.Data.size() > 0;
		bool hasTangent = mesh->Tangents.Data.size() > 0;
		bool hasUV = mesh->UVs.Data.size() > 0;
		bool hasWeights = mesh->Weights.Data.size() > 0;
		bool hasIDs = mesh->IDs.Data.size() > 0;

		if ((hasWeights || hasIDs) && (!hasWeights || !hasIDs))
			ASSERT_MSG(false, "Error: Invalid Skin Info!");

		// create Vertex obj from mesh data.
		auto CreateVertex = [&](Vertex &vtx, int index)
		{
			int posIndex = index * 3;
			int uvIndex = index * 2;
			int weightIndex = index * 4;

			vtx.Empty();
			vtx.Position = Vector4(mesh->Positions.Data[posIndex], mesh->Positions.Data[posIndex + 1],
				mesh->Positions.Data[posIndex + 2]);

			if (hasNormal)
				vtx.Normal = Vector4(mesh->Normals.Data[posIndex], mesh->Normals.Data[posIndex + 1],
				mesh->Normals.Data[posIndex + 2]);
			if (hasTangent)
				vtx.Tangent = Vector4(mesh->Tangents.Data[posIndex], mesh->Tangents.Data[posIndex + 1],
				mesh->Tangents.Data[posIndex + 2]);
			if (hasUV)
				vtx.UV = Vector4(mesh->UVs.Data[uvIndex], mesh->UVs.Data[uvIndex + 1], 0.0f);

			if (hasWeights)
			{
				vtx.IDs[0] = mesh->IDs.Data[weightIndex];
				vtx.IDs[1] = mesh->IDs.Data[weightIndex + 1];
				vtx.IDs[2] = mesh->IDs.Data[weightIndex + 2];
				vtx.IDs[3] = mesh->IDs.Data[weightIndex + 3];
				vtx.Weights[0] = mesh->Weights.Data[posIndex];
				vtx.Weights[1] = mesh->Weights.Data[posIndex + 1];
				vtx.Weights[2] = mesh->Weights.Data[posIndex + 2];
			}
		};

		// find nearest points to a given position.
		auto FindNeighbors = [&dividePlane, &entries](Vector4 pos, float radius, std::vector<unsigned int> &output)
		{
			const float dist = dividePlane * pos;
			const float minDist = dist - radius, maxDist = dist + radius;

			// clear the array in this strange fashion because a simple clear() would also deallocate
			// the array which we want to avoid
			output.erase(output.begin(), output.end());

			// quick check for positions outside the range
			if (entries.size() == 0 || maxDist < entries.front().dist || minDist > entries.back().dist)
				return;

			// do a binary search for the minimal distance to start the iteration there
			unsigned int index = (unsigned int)entries.size() / 2;
			unsigned int binaryStepSize = (unsigned int)entries.size() / 4;
			while (binaryStepSize > 1)
			{
				if (entries[index].dist < minDist)
					index += binaryStepSize;
				else
					index -= binaryStepSize;

				binaryStepSize /= 2;
			}

			// depending on the direction of the last step we need to single step a bit back or forth
			// to find the actual beginning element of the range
			while (index > 0 && entries[index].dist > minDist)
				index--;
			while (index < (entries.size() - 1) && entries[index].dist < minDist)
				index++;

			// now start iterating from there until the first position lays outside of the distance range.
			// add all positions inside the distance range within the given radius to the result aray
			std::vector<VtxEntry>::const_iterator it = entries.begin() + index;
			const float pSquared = radius * radius;
			while (it->dist < maxDist)
			{
				if ((it->position - pos).SquareLength() < pSquared)
					output.push_back(it->index);
				++it;
				if (it == entries.end())
					break;
			}
		};

		for (int i = 0; i < verticesCount; i++)
		{
			int index = i * 3;
			Vector4 position(mesh->Positions.Data[index], mesh->Positions.Data[index + 1],
				mesh->Positions.Data[index + 2], 1.0f);

			// store data for unique vtx finding
			entries.push_back(VtxEntry(i, position, dividePlane * position));
		}

		// find unique vertices

		// spatial sort
		std::sort(entries.begin(), entries.end());

		// stores our unqiue vertices.
		std::vector<Vertex> uniqueVertices;
		uniqueVertices.reserve(verticesCount);

		// For each vertex the index of the vertex it was replaced by.
		// Since the maximal number of vertices is 2^31-1, the most significand bit can be used to mark
		//  whether a new vertex was created for the index (true) or if it was replaced by an existing
		//  unique vertex (false). This saves an additional std::vector<bool> and greatly enhances
		//  branching performance.
		std::vector<unsigned int> replaceIndices(verticesCount, 0xffffffff);

		// stores nearest vertices result to given vertex.
		std::vector<unsigned int> verticesFound;
		verticesFound.reserve(10);

		float epsilon = 1e-5f;
		float squareEpsilon = epsilon * epsilon;

		// iterate through mesh's vertices to find unique vertices.
		Vertex vtx;
		for (int i = 0; i < verticesCount; i++)
		{
			CreateVertex(vtx, i);

			// find nearest vertices to current vertex.
			FindNeighbors(vtx.Position, epsilon, verticesFound);
			unsigned int matchIndex = 0xffffffff;

			// iterate through nearest vertices to test if it's unique.
			for (unsigned int j = 0; j < verticesFound.size(); j++)
			{
				const unsigned int uidx = replaceIndices[verticesFound[j]];

				// check if flag bit is true, ie: already has a replacement?
				if (uidx & 0x80000000)
					continue;

				Vertex &vtxu = uniqueVertices[uidx];

				if (hasNormal && (vtxu.Normal - vtx.Normal).SquareLength() > squareEpsilon)
					continue;
				if (hasTangent && (vtxu.Tangent - vtx.Tangent).SquareLength() > squareEpsilon)
					continue;
				if (hasUV && (vtxu.UV - vtx.UV).SquareLength() > squareEpsilon)
					continue;
				if (hasWeights && !vtxu.HasSameWeights(vtx.Weights, epsilon) && !vtxu.HasSameIDs(vtx.IDs))
					continue;

				// found a unique one, update replacement data.
				matchIndex = uidx;
				break;
			}

			auto &pair = replaceIndices[i];

			// found a replacement vertex among the uniques?
			if (matchIndex != 0xffffffff)
			{
				// store where to found the matching unique vertex
				// set flag bit to 1
				replaceIndices[i] = matchIndex | 0x80000000;
			}
			else
			{
				// no unique vertex matches it upto now -> so add it
				replaceIndices[i] = (unsigned int)uniqueVertices.size();
				uniqueVertices.push_back(vtx);
			}
		}
		entries.clear();

		// move data to mesh
		const int vtxCount = uniqueVertices.size();
		const int vtxCount2 = vtxCount * 2;
		const int vtxCount3 = vtxCount * 3;
		const int vtxCount4 = vtxCount * 4;

		// pre allocate some space.
		mesh->Positions.Data.resize(vtxCount3);
		if (hasNormal)
			mesh->Normals.Data.resize(vtxCount3);
		if (hasTangent)
			mesh->Tangents.Data.resize(vtxCount3);
		if (hasUV)
			mesh->UVs.Data.resize(vtxCount2);
		if (hasWeights)
			mesh->IDs.Data.resize(vtxCount4);
		if (hasWeights)
			mesh->Weights.Data.resize(vtxCount3);

		// copy data to mesh.
		for (unsigned int i = 0; i < uniqueVertices.size(); i++)
		{
			Vertex &uVtx = uniqueVertices[i];
			int posIndex = i * 3;
			int uvIndex = i * 2;
			int weightIndex = i * 4;

			mesh->Positions.Data[posIndex] = uVtx.Position.x;
			mesh->Positions.Data[posIndex + 1] = uVtx.Position.y;
			mesh->Positions.Data[posIndex + 2] = uVtx.Position.z;

			if (hasNormal)
			{
				mesh->Normals.Data[posIndex] = uVtx.Normal.x;
				mesh->Normals.Data[posIndex + 1] = uVtx.Normal.y;
				mesh->Normals.Data[posIndex + 2] = uVtx.Normal.z;
			}

			if (hasTangent)
			{
				mesh->Tangents.Data[posIndex] = uVtx.Tangent.x;
				mesh->Tangents.Data[posIndex + 1] = uVtx.Tangent.y;
				mesh->Tangents.Data[posIndex + 2] = uVtx.Tangent.z;
			}

			if (hasUV)
			{
				mesh->UVs.Data[uvIndex] = uVtx.UV.x;
				mesh->UVs.Data[uvIndex + 1] = uVtx.UV.y;
			}

			if (hasWeights)
			{
				mesh->IDs.Data[weightIndex] = uVtx.IDs[0];
				mesh->IDs.Data[weightIndex + 1] = uVtx.IDs[1];
				mesh->IDs.Data[weightIndex + 2] = uVtx.IDs[2];
				mesh->IDs.Data[weightIndex + 3] = uVtx.IDs[3];
				mesh->Weights.Data[posIndex] = uVtx.Weights[0];
				mesh->Weights.Data[posIndex + 1] = uVtx.Weights[1];
				mesh->Weights.Data[posIndex + 2] = uVtx.Weights[2];
			}
		}

		// find correct indices.
		for (unsigned int i = 0; i < mesh->Indices.Data.size(); i++)
		{
			// set most significant bit to 0. ie: remove flag bit.
			mesh->Indices.Data[i] = replaceIndices[mesh->Indices.Data[i]] & ~0x80000000;
		}

		// correct submeshes
		unsigned int subMeshCount = mesh->GetSubMeshCount();
		for (unsigned int i = 0; i < subMeshCount; i++)
		{
			if (auto subMesh = mesh->GetSubMeshAt(i))
			{
				for (unsigned int j = 0; j < subMesh->Indices.Data.size(); j++)
				{
					subMesh->Indices.Data[j] = replaceIndices[subMesh->Indices.Data[j]] & ~0x80000000;
				}
			}
		}

		FURYD << mesh->GetName() << "[vtx: " << mesh->Positions.Data.size() / 3 << " tris: " << mesh->Indices.Data.size() / 3 << "]";
	}

	void MeshUtil::CalculateNormal(const std::shared_ptr<Mesh> &mesh) 
	{
		mesh->Normals.Data.resize(mesh->Positions.Data.size());

		for (auto &value : mesh->Normals.Data)
			value = 0.0f;

		unsigned int numTriangles = mesh->Indices.Data.size() / 3;
		unsigned int numVertices = mesh->Positions.Data.size() / 3;

		auto GetPositionAt = [&mesh](unsigned int index) -> Vector4
		{
			unsigned int j = index * 3;
			return Vector4(mesh->Positions.Data[j], mesh->Positions.Data[j + 1], mesh->Positions.Data[j + 2]);
		};

		auto SetNormalAt = [&mesh](unsigned int index, Vector4 normal)
		{
			unsigned int j = index * 3;
			mesh->Normals.Data[j] += normal.x;
			mesh->Normals.Data[j + 1] += normal.y;
			mesh->Normals.Data[j + 2] += normal.z;
		};

		for (unsigned int i = 0; i < numTriangles; i++)
		{
			unsigned int j = i * 3;
			unsigned int ia = mesh->Indices.Data[j];
			unsigned int ib = mesh->Indices.Data[j + 1];
			unsigned int ic = mesh->Indices.Data[j + 2];

			Vector4 a = GetPositionAt(ia);
			Vector4 b = GetPositionAt(ib);
			Vector4 c = GetPositionAt(ic);

			Vector4 e0 = b - a;
			Vector4 e1 = c - b;
			Vector4 normal = e0.CrossProduct(e1);

			SetNormalAt(ia, normal);
			SetNormalAt(ib, normal);
			SetNormalAt(ic, normal);
		}

		for (unsigned int i = 0; i < numVertices; i++)
		{
			unsigned int j = i * 3;

			float &x = mesh->Normals.Data[j];
			float &y = mesh->Normals.Data[j + 1];
			float &z = mesh->Normals.Data[j + 2];

			Vector4 normal = Vector4(x, y, z).Normalized();

			x = normal.x;
			y = normal.y;
			z = normal.z;
		}
	}

	void MeshUtil::CalculateTangent(const std::shared_ptr<Mesh> &mesh) 
	{
		if (mesh->Normals.Data.size() == 0 || mesh->UVs.Data.size() == 0)
		{
			FURYW << "Normal and UV data is required.";
			return;
		}

		mesh->Tangents.Data.resize(mesh->Positions.Data.size());

		for (auto &value : mesh->Tangents.Data)
			value = 0.0f;

		unsigned int numTriangles = mesh->Indices.Data.size() / 3;
		unsigned int numVertices = mesh->Positions.Data.size() / 3;

		auto GetPositionAt = [&mesh](unsigned int index) -> Vector4
		{
			unsigned int j = index * 3;
			return Vector4(mesh->Positions.Data[j], mesh->Positions.Data[j + 1], mesh->Positions.Data[j + 2]);
		};

		auto GetUVAt = [&mesh](unsigned int index) -> Vector4
		{
			unsigned int j = index * 3;
			return Vector4(mesh->UVs.Data[j], mesh->UVs.Data[j + 1], 0, 0);
		};

		auto SetTangentAt = [&mesh](unsigned int index, Vector4 tangent)
		{
			unsigned int j = index * 3;
			mesh->Tangents.Data[j] += tangent.x;
			mesh->Tangents.Data[j + 1] += tangent.y;
			mesh->Tangents.Data[j + 2] += tangent.z;
		};

		for (unsigned int i = 0; i < numTriangles; i++)
		{
			unsigned int j = i * 3;
			unsigned int ia = mesh->Indices.Data[j];
			unsigned int ib = mesh->Indices.Data[j + 1];
			unsigned int ic = mesh->Indices.Data[j + 2];

			Vector4 p0 = GetPositionAt(ia);
			Vector4 p1 = GetPositionAt(ib);
			Vector4 p2 = GetPositionAt(ic);

			Vector4 uv0 = GetUVAt(ia);
			Vector4 uv1 = GetUVAt(ib);
			Vector4 uv2 = GetUVAt(ic);

			Vector4 dp0 = p1 - p0;
			Vector4 dp1 = p2 - p1;

			Vector4 duv0 = uv1 - uv0;
			Vector4 duv1 = uv2 - uv1;

			float cross = (duv0.x * duv1.y - duv0.y * duv1.x);
			float r = 0.0f;
			if (cross != 0.0f) r = 1.0f / cross;

			Vector4 tangent = (dp0 * duv1.y - dp1 * duv0.y) * r;

			SetTangentAt(ia, tangent);
			SetTangentAt(ib, tangent);
			SetTangentAt(ic, tangent);
		}

		for (unsigned int i = 0; i < numVertices; i++)
		{
			unsigned int j = i * 3;

			float &x = mesh->Tangents.Data[j];
			float &y = mesh->Tangents.Data[j + 1];
			float &z = mesh->Tangents.Data[j + 2];

			Vector4 tangent = Vector4(x, y, z).Normalized();

			x = tangent.x;
			y = tangent.y;
			z = tangent.z;
		}
	}
}