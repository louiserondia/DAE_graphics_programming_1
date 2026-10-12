//==============================================================
//  Graphics Programming 1 (2026-2027)
//  Authors : Matthieu Delaere
//  Copyright (c) 2026 Matthieu Delaere. All rights reserved.
//==============================================================
#include <primitives.h>
#include <numeric>
#include <iostream>
using namespace gfx;

//--- Primitives AABB Updates ---

void Sphere::UpdateAABB()
{
	//TODO
}

void Triangle::UpdateAABB()
{
	bounding_box = AABB{};
	bounding_box.value().ExpandToInclude(v0);
	bounding_box.value().ExpandToInclude(v1);
	bounding_box.value().ExpandToInclude(v2);

}

AABB GetAABB(const Vector3& v0, const Vector3& v1, const Vector3& v2)
{
	AABB aabb{};
	aabb.ExpandToInclude(v0);
	aabb.ExpandToInclude(v1);
	aabb.ExpandToInclude(v2);
	return aabb;
}

Vector4 Triangle::GetCentroid()
{
	return Vector4{ (v0.x + v1.x + v2.x) / 3, (v0.y + v1.y + v2.y) / 3, (v0.z + v1.z + v2.z) / 3, 1.f };
}


static void BVH(
	BVHNode& node,
	std::vector<BVHNode>& tree,
	std::vector<std::pair<Vector4, AABB>>& triangles,
	std::vector<uint32_t>& bvh_tri_indices)
{

	// 0. end condition
	if (node.IsLeaf())
	{
		std::cout << "tri count : " << node.tri_count << std::endl;
		return;
	}

	// 1. find longest side and cut in two
	const Vector3 size(node.bounds.GetSize());

	BVHNode left{};
	BVHNode right{};

	uint8_t biggest{
		size.x > size.y && size.x > size.z ? uint8_t(0) :
		size.y > size.x && size.y > size.z ? uint8_t(1) : uint8_t(2)
	};

	const float mid{ node.bounds.min[biggest] + (node.bounds.max[biggest] - node.bounds.min[biggest]) / 2 };

	// bvhvv_tri_indices is the array of indices of the triangles, sorted by node, 
	// we run through the indices of parent node and split them in two temporary arrays
	// then replace the values in the original
	// we also create the new aabb for the children, 
	// not just divided in 2 but adjsted to the new set of triangles

	std::vector<uint32_t> left_temp;
	std::vector<uint32_t> right_temp;

	left_temp.reserve(node.tri_count / 3);
	right_temp.reserve(node.tri_count / 3);

	for (uint32_t index{ node.first_tri }; index < node.first_tri + node.tri_count; index++)
	{
		uint32_t tri_index{ bvh_tri_indices[index] };

		if (triangles.at(tri_index).first[biggest] < mid) // is left child
		{
			left_temp.push_back(tri_index);
			left.bounds.ExpandToInclude(triangles.at(tri_index).second);
		}
		else
		{
			right_temp.push_back(tri_index);
			right.bounds.ExpandToInclude(triangles.at(tri_index).second);
		}
	}

	bvh_tri_indices.erase(bvh_tri_indices.begin() + node.first_tri, bvh_tri_indices.begin() + node.first_tri + node.tri_count);
	bvh_tri_indices.insert(bvh_tri_indices.begin() + node.first_tri, left_temp.begin(), left_temp.end());
	bvh_tri_indices.insert(bvh_tri_indices.begin() + node.first_tri, right_temp.begin(), right_temp.end());

	// 3. set children nodes' variables

	left.first_tri = node.first_tri;
	left.tri_count = uint32_t(left_temp.size());
	right.first_tri = uint32_t(node.first_tri + left_temp.size());
	right.tri_count = uint32_t(right_temp.size());


	// 4. add nodes to tree

	node.left_child = uint32_t(tree.size());

	tree.push_back(left);
	tree.push_back(right);

	// 5. call for children and clear temp vectors

	left_temp.clear();
	right_temp.clear();

	BVH(left, tree, triangles, bvh_tri_indices);
	BVH(right, tree, triangles, bvh_tri_indices);
}

void TriangleMesh::UpdateAABB()
{
	size_t count{ indices.size() / 3 };
	bvh_tri_indices.resize(count);
	std::iota(bvh_tri_indices.begin(), bvh_tri_indices.end(), 0);
	std::vector<std::pair<Vector4, AABB>> triangles(count);

	for (size_t i{}; i < indices.size(); i += 3)
	{
		Triangle t{
			vertices[indices[i]].position,
			vertices[indices[i + 1]].position,
			vertices[indices[i + 2]].position
		};

		t.UpdateAABB();
		triangles.at(i / 3) = std::make_pair(t.GetCentroid(), t.bounding_box.value());
		bounding_box.value().ExpandToInclude(triangles[i / 3].second);
	}

	// creer l arbre :  
	std::vector<BVHNode> tree;
	BVHNode root{};
	root.bounds = bounding_box.value();
	root.first_tri = 0;
	root.tri_count = uint32_t(count);

	tree.push_back(root);

	BVH(root, tree, triangles, bvh_tri_indices);
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

void AABB::ExpandToInclude(const Vector4& point)
{
	ExpandToInclude(Vector3{ point });
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