//==============================================================
//  Graphics Programming 1 (2026-2027)
//  Authors : Matthieu Delaere & Pieter-Jan Vandenberghe
//  Copyright (c) 2026 Matthieu Delaere. All rights reserved.
//==============================================================
#include <primitives.h>
using namespace gfx;

//--- Standard Includes ---
#include <algorithm>
#include <cassert>
#include <cmath>

//--- Voxel Grid ---
void VoxelGrid::Set(const uint32_t x, const uint32_t y, const uint32_t z, const uint8_t value)
{
	assert(x < size_x && y < size_y && z < size_z && "Voxel coordinate out of range!");
	voxels[ToIndex(x, y, z)] = value;
}

uint8_t VoxelGrid::Get(const uint32_t x, const uint32_t y, const uint32_t z) const
{
	assert(x < size_x && y < size_y && z < size_z && "Voxel coordinate out of range!");
	return voxels[ToIndex(x, y, z)];
}

void VoxelGrid::Clear()
{
	std::fill(voxels.begin(), voxels.end(), static_cast<uint8_t>(0));
}

void VoxelGrid::UpdateAABB()
{
	const Vector3 extent{
		static_cast<float>(size_x) * voxel_size,
		static_cast<float>(size_y) * voxel_size,
		static_cast<float>(size_z) * voxel_size };
	bounding_box = AABB(origin, origin + extent);
}

void VoxelGrid::VoxelizeTriangle(const Vector3& v0, const Vector3& v1, const Vector3& v2, const uint8_t value)
{
	// Sample the triangle with a spacing of a quarter voxel and mark every voxel a sample lands in.
	// Simple and robust for a surface (shell) voxelization; the interior is not filled.
	const float longest_edge = std::max({ (v1 - v0).Magnitude(), (v2 - v0).Magnitude(), (v2 - v1).Magnitude() });
	const uint32_t steps = static_cast<uint32_t>(std::ceil(longest_edge / (voxel_size * 0.25f))) + 1;
	const float inv_steps = 1.f / static_cast<float>(steps);

	for (uint32_t i = 0; i <= steps; ++i)
	{
		for (uint32_t j = 0; j <= steps - i; ++j)
		{
			const float u = static_cast<float>(i) * inv_steps;
			const float v = static_cast<float>(j) * inv_steps;
			const Vector3 p = v0 + (v1 - v0) * u + (v2 - v0) * v;
			const Vector3 local = (p - origin) / voxel_size;
			if (local.x < 0.f || local.y < 0.f || local.z < 0.f)
				continue;

			const uint32_t x = static_cast<uint32_t>(local.x);
			const uint32_t y = static_cast<uint32_t>(local.y);
			const uint32_t z = static_cast<uint32_t>(local.z);
			if (x < size_x && y < size_y && z < size_z)
				voxels[ToIndex(x, y, z)] = value;
		}
	}
}
