//==============================================================
//  Graphics Programming 1 (2026-2027)
//  Authors : Matthieu Delaere
//  Copyright (c) 2026 Matthieu Delaere. All rights reserved.
//==============================================================
#include <primitives.h>
using namespace gfx;

//--- Primitives AABB Updates ---

void Sphere::UpdateAABB()
{
	//TODO
}

void Triangle::UpdateAABB()
{
	bounding_box = AABB{};
	bounding_box.value().UpdateAABBPerVertex(v0);
	bounding_box.value().UpdateAABBPerVertex(v1);
	bounding_box.value().UpdateAABBPerVertex(v2);
}

void TriangleMesh::UpdateAABB()
{
	bounding_box = AABB{};
	for (const Vertex& vertex : vertices)
	{
		bounding_box.value().UpdateAABBPerVertex(vertex);
	}
}

//--- AABB ---

void AABB::UpdateAABBPerVertex(const Vector3& pos)
{
	min.x = std::min(pos.x, min.x);
	min.y = std::min(pos.y, min.y);
	min.z = std::min(pos.z, min.z);
					 
	max.x = std::max(pos.x, max.x);
	max.y = std::max(pos.y, max.y);
	max.z = std::max(pos.z, max.z);
}

void AABB::UpdateAABBPerVertex(const Vertex& vertex)
{
	min.x = std::min(vertex.position.x, min.x);
	min.y = std::min(vertex.position.y, min.y);
	min.z = std::min(vertex.position.z, min.z);

	max.x = std::max(vertex.position.x, max.x);
	max.y = std::max(vertex.position.y, max.y);
	max.z = std::max(vertex.position.z, max.z);
}

void AABB::ExpandToInclude(const Vector3& point)
{
	min = Vector3::Min(min, point);
	max = Vector3::Max(max, point);
}

void AABB::ExpandToInclude(const AABB& other)
{
	min = Vector3::Min(min, other.min);
	max = Vector3::Max(max, other.max);
}

bool AABB::Contains(const Vector3& point) const
{
	return point.x >= min.x && point.x <= max.x &&
		point.y >= min.y && point.y <= max.y &&
		point.z >= min.z && point.z <= max.z;
}

bool AABB::Overlaps(const AABB& other) const
{
	return min.x <= other.max.x && max.x >= other.min.x &&
		min.y <= other.max.y && max.y >= other.min.y &&
		min.z <= other.max.z && max.z >= other.min.z;
}

Vector3 AABB::GetCenter() const
{
	return { (min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f, (min.z + max.z) * 0.5f };
}

Vector3 AABB::GetSize() const
{
	return { max.x - min.x, max.y - min.y, max.z - min.z };
}

bool AABB::IsValid() const
{
	return min.x <= max.x && min.y <= max.y && min.z <= max.z;
}