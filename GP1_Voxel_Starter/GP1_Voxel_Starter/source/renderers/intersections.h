//==============================================================
//  Graphics Programming 1 (2026-2027)
//  Authors : Matthieu Delaere & Pieter-Jan Vandenberghe & Louise Rondia ^^
//  Copyright (c) 2026 Matthieu Delaere. All rights reserved.
//==============================================================
#ifndef INTERSECTIONS_HEADER
#define INTERSECTIONS_HEADER

//--- Standard Includes ---
#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits>

//--- Framework Includes ---
#include <primitives.h>

#define TO_IMPLEMENT

namespace gfx
{
	//--- Intersection Tests ---
	// Slab test. On a hit, t_enter/t_exit hold the parametric interval of the ray inside the box,
	// clamped to [ray.min, ray.max].
	[[maybe_unused]]
	static bool HitTestAABB(const AABB& aabb, const Ray& ray, float& t_enter, float& t_exit)
	{
		// Running interval: per axis min/max maps to minss/maxss, no branches
		const float tx1{ (aabb.min.x - ray.origin.x) / ray.direction.x };
		const float tx2{ (aabb.max.x - ray.origin.x) / ray.direction.x };
		float t_min{ std::min(tx1, tx2) };
		float t_max{ std::max(tx1, tx2) };

		const float ty1{ (aabb.min.y - ray.origin.y) / ray.direction.y };
		const float ty2{ (aabb.max.y - ray.origin.y) / ray.direction.y };
		t_min = std::max(t_min, std::min(ty1, ty2));
		t_max = std::min(t_max, std::max(ty1, ty2));

		const float tz1{ (aabb.min.z - ray.origin.z) / ray.direction.z };
		const float tz2{ (aabb.max.z - ray.origin.z) / ray.direction.z };
		t_min = std::max(t_min, std::min(tz1, tz2));
		t_max = std::min(t_max, std::max(tz1, tz2));

		t_enter = std::max(t_min, ray.min);
		t_exit = std::min(t_max, ray.max);
		return t_max >= t_min && t_max >= ray.min && t_min <= ray.max;
	}

	// Traverse the grid with a 3D DDA (Amanatides & Woo) and report the first non-empty voxel.
	[[maybe_unused]]
	static bool HitTestVoxelGrid(const VoxelGrid& grid, const Ray& ray, RayHitRecord& hit_record, const bool ignore_hit_record = false)
	{
		uint32_t step_count{ 0 }; // How many cells we stepped through
		const float voxel_size{ grid.voxel_size }; // Size of a single cell in the grid

		float t_grid_enter{ 0.f }; // t value for closest point on ray inside the voxel grid
		float t_grid_exit{ 0.f };  // t value for farthest point on ray inside the voxel grid

		// Clip the ray against the grid bounds first, so traversal starts inside the grid.
		assert(grid.bounding_box.has_value() && "VoxelGrid has no bounding box, call UpdateAABB()!");
		const AABB& bounds{ grid.bounding_box.value() };
		if (!HitTestAABB(bounds, ray, t_grid_enter, t_grid_exit))
		{
			if (!ignore_hit_record)
				hit_record.step_count = step_count;
			return false;
		}

		// First point on ray that is inside the voxel grid (either on the edge, or inside if camera is in the grid)
		const Vector3 entry_point_world{ ray.origin + ray.direction * t_grid_enter };

		// Convert that point from world space to grid space (it is still a float! not a cell index!)
		const Vector3 entry_point_grid{ (entry_point_world - bounds.min) / voxel_size };

		// How many cells are there in each direction
		const int32_t grid_size_in_cells[3]{ static_cast<int32_t>(grid.size_x), static_cast<int32_t>(grid.size_y), static_cast<int32_t>(grid.size_z) };

		int32_t current_cell[3]{};	// Current cell (changes as we traverse to next neighbouring cell)
		[[maybe_unused]] int32_t step_direction[3]{};	// Direction in which you move to go to next cell (+1, -1 or 0)
		[[maybe_unused]] float t_next_grid_line[3]{};	// t value at which next grid line along that axis is reached
		[[maybe_unused]] float t_delta[3]{};			// delta t value needed to cross one whole cell on this axis

		// Tip: notice that data is kept in array with length 3 such that you can loop over these using indices (to avoid duplicate code).

		for (uint8_t axis{ 0 }; axis < 3; ++axis)
		{
			// Floor and clamp entry_point_grid to cell index
			current_cell[axis] = std::clamp(static_cast<int32_t>(std::floor(entry_point_grid[axis])), 0, grid_size_in_cells[axis] - 1);

			[[maybe_unused]] const float dir{ ray.direction[axis] };

			step_direction[axis] = dir > std::numeric_limits<float>::min() ?
				1 : dir < -std::numeric_limits<float>::min() ?
				-1 : 0;

			if (step_direction[axis] == 0)
			{
				t_next_grid_line[axis] = std::numeric_limits<float>::max();
				t_delta[axis] = std::numeric_limits<float>::max();
				continue;
			}

			t_delta[axis] = abs(voxel_size / dir);

			float pos_in_cell{entry_point_grid[axis] - current_cell[axis] };
			float entry_diff{ step_direction[axis] < 0.f && pos_in_cell > 0.f ? 1.f - pos_in_cell : pos_in_cell };
		
			t_next_grid_line[axis] = t_grid_enter + (t_delta[axis] - (entry_diff * t_delta[axis]));
		}

		//
		// Normal of the face via which a cell is hit.
		[[maybe_unused]] uint8_t entry_axis{ 0 };
		{
			float closest{ std::numeric_limits<float>::max() };
			for (uint8_t axis{ 0 }; axis < 3; ++axis)
			{
				// Only meaningful if starting outside the grid:
				// For one axis the distance between the grid line (either min or max) and entry_point_world will be zero
				const float distance{ std::min(std::abs(entry_point_world[axis] - bounds.min[axis]), std::abs(entry_point_world[axis] - bounds.max[axis])) };
				if (distance < closest) { closest = distance; entry_axis = axis; }
			}
		}

		float t_cell_entry{ t_grid_enter };

		// Walk the grid one cell at a time as long as we are inside the boundaries of the voxel grid
		// and as long as t_cell_entry is less than t_grid_exit (early exit for lighting)
		while (
			current_cell[0] >= 0 && current_cell[0] < grid_size_in_cells[0] &&
			current_cell[1] >= 0 && current_cell[1] < grid_size_in_cells[1] &&
			current_cell[2] >= 0 && current_cell[2] < grid_size_in_cells[2] &&
			t_cell_entry <= t_grid_exit
			)
		{
			++step_count;

			const auto voxel{ grid.Get(current_cell[0], current_cell[1], current_cell[2]) };

			if (voxel != 0) {
				if (!ignore_hit_record)
				{
					Vector3 normal{};
					normal[entry_axis] = float(-step_direction[entry_axis]);
					hit_record.t = t_cell_entry;
					hit_record.position = ray.origin + ray.direction * t_cell_entry;
					hit_record.normal = normal;
					hit_record.voxel = voxel;
					hit_record.step_count = step_count;
				}
				return true;
			}

			entry_axis = t_next_grid_line[0] < t_next_grid_line[1] && t_next_grid_line[0] < t_next_grid_line[2] ?
				0 : t_next_grid_line[1] < t_next_grid_line[2] ?
				1 : 2;

			t_cell_entry = t_next_grid_line[entry_axis];
			t_next_grid_line[entry_axis] += t_delta[entry_axis];
			current_cell[entry_axis] += step_direction[entry_axis];
		}

		if (!ignore_hit_record)
			hit_record.step_count = step_count;
		return false;
	}
}
#endif //INTERSECTIONS_HEADER
