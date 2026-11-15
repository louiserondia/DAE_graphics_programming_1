//==============================================================
//  Graphics Programming 1 (2026-2027)
//  Authors : Matthieu Delaere & Pieter-Jan Vandenberghe
//  Copyright (c) 2026 Matthieu Delaere. All rights reserved.
//==============================================================
// Stand-alone sanity test for HitTestVoxelGrid.
//
// Every ray starts at the camera position of the scene, so you can picture it
// as "standing where the camera is and looking in some direction".
// Rays are chosen to be easy to imagine: the center of the screen, straight
// ahead, straight down, aimed at an object's center, ...
//
// Run from the executable's directory (the bunny scene loads its .obj from
// "resources/..."). Exit code 0 = all checks passed.
//==============================================================

//--- Standard Includes ---
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

//--- Framework Includes ---
#include <intersections.h>
#include <scenes.h>

using namespace gfx;

namespace
{
	int g_checks{ 0 };
	int g_failures{ 0 };

	void Check(const bool condition, const std::string& what)
	{
		++g_checks;
		if (!condition)
		{
			++g_failures;
			std::printf("    FAILED: %s\n", what.c_str());
		}
	}

	bool Near(const float a, const float b, const float eps = 1e-3f) { return std::abs(a - b) <= eps; }
	bool Near(const Vector3& a, const Vector3& b, const float eps = 1e-3f)
	{
		return Near(a.x, b.x, eps) && Near(a.y, b.y, eps) && Near(a.z, b.z, eps);
	}

	std::string ToString(const Vector3& v)
	{
		char buffer[96];
		std::snprintf(buffer, sizeof(buffer), "(%.4f, %.4f, %.4f)", v.x, v.y, v.z);
		return buffer;
	}

	// Shoots the ray and prints what it hit, so a failure is easy to diagnose.
	bool Shoot(const char* name, const VoxelGrid& grid, const Vector3& origin, const Vector3& direction, RayHitRecord& hit)
	{
		const Ray ray{ .origin = origin, .direction = direction.Normalized() };
		hit = {};
		const bool did_hit{ HitTestVoxelGrid(grid, ray, hit) };

		std::printf("  %s\n", name);
		if (did_hit)
			std::printf("    hit: voxel=%u t=%.4f position=%s normal=%s steps=%u\n",
				hit.voxel, hit.t, ToString(hit.position).c_str(), ToString(hit.normal).c_str(), hit.step_count);
		else
			std::printf("    miss: steps=%u\n", hit.step_count);
		return did_hit;
	}

	// Common checks for every hit: t, position and origin must be consistent,
	// and the hit position must lie on the face of the voxel indicated by the normal.
	void CheckHitConsistency(const VoxelGrid& grid, const Vector3& origin, const Vector3& direction, const RayHitRecord& hit)
	{
		const Vector3 expected_position{ origin + direction.Normalized() * hit.t };
		Check(Near(hit.position, expected_position), "position == origin + direction * t");
		Check(Near(hit.normal.Magnitude(), 1.f), "normal has unit length");

		// The cell just "behind" the hit position (opposite the normal) is the voxel that was hit.
		const Vector3 inside{ hit.position - hit.normal * (grid.voxel_size * 0.5f) };
		const Vector3 cell{ (inside - grid.origin) / grid.voxel_size };
		const uint32_t x{ static_cast<uint32_t>(std::floor(cell.x)) };
		const uint32_t y{ static_cast<uint32_t>(std::floor(cell.y)) };
		const uint32_t z{ static_cast<uint32_t>(std::floor(cell.z)) };
		const bool in_grid{ cell.x >= 0.f && cell.y >= 0.f && cell.z >= 0.f && x < grid.size_x && y < grid.size_y && z < grid.size_z };
		Check(in_grid && grid.Get(x, y, z) == hit.voxel, "voxel behind the hit face matches hit.voxel");

		// And the cell in front of that face must be empty (or outside the grid).
		const Vector3 outside{ hit.position + hit.normal * (grid.voxel_size * 0.5f) };
		const Vector3 cell_out{ (outside - grid.origin) / grid.voxel_size };
		const bool out_in_grid{ cell_out.x >= 0.f && cell_out.y >= 0.f && cell_out.z >= 0.f &&
			cell_out.x < static_cast<float>(grid.size_x) && cell_out.y < static_cast<float>(grid.size_y) && cell_out.z < static_cast<float>(grid.size_z) };
		Check(!out_in_grid || grid.Get(static_cast<uint32_t>(cell_out.x), static_cast<uint32_t>(cell_out.y), static_cast<uint32_t>(cell_out.z)) == 0,
			"voxel in front of the hit face is empty");
	}

	// -------------------------------------------------------------------------
	// TerrainScene
	//   Grid: 64 x 32 x 64 voxels of 0.25 -> world box [0,16] x [0,8] x [0,16].
	//   Camera: (8, 7, -7.5), so in front of the grid (z < 0), centered on x,
	//           above the terrain, pitched down by 0.45 rad.
	//   Red sphere (voxel 4): center (8, 4, 8), radius 1.5 (world units).
	// -------------------------------------------------------------------------
	void TestTerrainScene()
	{
		std::printf("TerrainScene\n");
		TerrainScene scene{};
		scene.Setup();
		const VoxelGrid& grid{ scene.voxel_grid };
		const Vector3 eye{ scene.camera.GetPosition() };
		RayHitRecord hit{};

		// 1) Straight down: the camera hovers in front of the grid (z = -7.5),
		//    so looking down we never even enter the grid.
		{
			const Vector3 dir{ 0.f, -1.f, 0.f };
			const bool did_hit{ Shoot("straight down -> misses (camera is not above the grid)", grid, eye, dir, hit) };
			Check(!did_hit, "no hit");
			Check(hit.step_count == 0, "no cells visited");
		}

		// 2) Straight ahead (+z, horizontal) at height 7: everything in the scene is lower
		//    (pillar top = 6, sphere top = 5.5), so the ray flies over the whole grid.
		//    It enters at z = 0 and leaves at z = 16: 16 / 0.25 = 64 cells visited.
		{
			const Vector3 dir{ 0.f, 0.f, 1.f };
			const bool did_hit{ Shoot("straight ahead (horizontal) -> flies over everything", grid, eye, dir, hit) };
			Check(!did_hit, "no hit");
			Check(hit.step_count == 64, "visits all 64 cells along z");
		}

		// 3) Aimed at the center of the red sphere. Nothing is in between, so we hit the
		//    sphere's front face, which is one radius in front of the center: z = 8 - 1.5 = 6.5.
		//    The ray only drops slightly (from y = 7 to 4 over 15.5 in z), so it enters that
		//    voxel through its front (-z) face -> normal (0, 0, -1).
		{
			const Vector3 sphere_center{ 8.f, 4.f, 8.f };
			const Vector3 dir{ sphere_center - eye };
			const bool did_hit{ Shoot("aimed at the sphere center -> hits the red sphere", grid, eye, dir, hit) };
			Check(did_hit, "hit");
			Check(hit.voxel == 4, "voxel is red sphere (4)");
			Check(Near(hit.normal, Vector3{ 0.f, 0.f, -1.f }), "normal faces the camera (0, 0, -1)");
			Check(Near(hit.position.z, 6.5f), "hit on the sphere's front face (z = 8 - 1.5 = 6.5)");
			Check(Near(hit.position.x, 8.f), "hit stays in the plane x = 8");
			CheckHitConsistency(grid, eye, dir, hit);
		}

		// 4) Center of the screen (= camera forward). Looking down at 0.45 rad from (8, 7, -7.5):
		//    the ray stays in the plane x = 8, passes well below the sphere and lands on the
		//    terrain (grass on top), coming from above -> normal points up.
		//    Around z = 6.5 the terrain at x = 8 is only 1 voxel high, so it lands on y = 0.25.
		{
			const Vector3 dir{ scene.camera.GetForward() };
			const bool did_hit{ Shoot("center of screen -> lands on the terrain", grid, eye, dir, hit) };
			Check(did_hit, "hit");
			Check(hit.voxel == 1, "voxel is grass (1), the top layer of the terrain");
			Check(Near(hit.normal, Vector3{ 0.f, 1.f, 0.f }), "normal points up (0, 1, 0)");
			Check(Near(hit.position.x, 8.f), "hit stays in the plane x = 8");
			Check(Near(hit.position.y, 0.25f), "hit on top of the lowest voxel layer (y = 0.25)");
			CheckHitConsistency(grid, eye, dir, hit);
		}
	}

	// -------------------------------------------------------------------------
	// BunnyScene
	//   Grid: 64 x 48 x 64 voxels of 0.1, origin (-3.2, 0, -3.2)
	//         -> world box [-3.2,3.2] x [0,4.8] x [-3.2,3.2].
	//   Floor (voxel 1): bottom layer, y in [0, 0.1].
	//   Bunny (voxel 2): standing on the floor around x = z = 0, height ~3.2.
	//   Camera: (0, 2.4, -7), in front of the grid, looking along +z, pitched down by 0.15 rad.
	// -------------------------------------------------------------------------
	void TestBunnyScene()
	{
		std::printf("BunnyScene\n");
		BunnyScene scene{};
		scene.Setup();
		const VoxelGrid& grid{ scene.voxel_grid };
		const Vector3 eye{ scene.camera.GetPosition() };
		RayHitRecord hit{};

		// The bunny is loaded from disk; without it the expectations below are meaningless.
		const bool bunny_loaded{ std::count(grid.voxels.begin(), grid.voxels.end(), static_cast<uint8_t>(2)) > 0 };
		Check(bunny_loaded, "bunny mesh loaded (run from the executable directory so resources/ is found)");
		if (!bunny_loaded)
			return;

		// 1) Straight down: the camera is in front of the grid (z = -7), so we miss.
		{
			const Vector3 dir{ 0.f, -1.f, 0.f };
			const bool did_hit{ Shoot("straight down -> misses (camera is not above the grid)", grid, eye, dir, hit) };
			Check(!did_hit, "no hit");
			Check(hit.step_count == 0, "no cells visited");
		}

		// 2) Straight up: also outside the grid -> miss.
		{
			const Vector3 dir{ 0.f, 1.f, 0.f };
			const bool did_hit{ Shoot("straight up -> misses", grid, eye, dir, hit) };
			Check(!did_hit, "no hit");
		}

		// 3) Aimed at a floor point in front of the bunny: (0, 0.1, -2.95) is on the top face
		//    of the floor layer. Nothing is in between, so we hit exactly that point,
		//    coming from above -> normal (0, 1, 0), distance = |target - eye|.
		{
			const Vector3 target{ 0.f, 0.1f, -2.95f };
			const Vector3 dir{ target - eye };
			const bool did_hit{ Shoot("aimed at the floor in front of the bunny -> hits the floor", grid, eye, dir, hit) };
			Check(did_hit, "hit");
			Check(hit.voxel == 1, "voxel is floor (1)");
			Check(Near(hit.normal, Vector3{ 0.f, 1.f, 0.f }), "normal points up (0, 1, 0)");
			Check(Near(hit.position, target), "hit position is the targeted floor point");
			Check(Near(hit.t, dir.Magnitude()), "t is the distance from camera to target");
			CheckHitConsistency(grid, eye, dir, hit);
		}

		// 4) Center of the screen (= camera forward): we look at the bunny, which stands in the
		//    middle of the screen, so we hit it from the front (camera side).
		{
			const Vector3 dir{ scene.camera.GetForward() };
			const bool did_hit{ Shoot("center of screen -> hits the bunny", grid, eye, dir, hit) };
			Check(did_hit, "hit");
			Check(hit.voxel == 2, "voxel is bunny (2)");
			Check(Near(hit.normal, Vector3{ 0.f, 0.f, -1.f }), "normal faces the camera (0, 0, -1)");
			Check(Near(hit.position.x, 0.f), "hit stays in the plane x = 0");
			CheckHitConsistency(grid, eye, dir, hit);
		}

		// 5) Straight ahead (+z, horizontal) at height 2.4: the ray passes over the bunny's head
		//    (the ears reach higher, and they are not on this ray's path), so it crosses
		//    the whole grid: 6.4 / 0.1 = 64 cells.
		{
			const Vector3 dir{ 0.f, 0.f, 1.f };
			const bool did_hit{ Shoot("straight ahead (horizontal) -> passes over the bunny's head", grid, eye, dir, hit) };
			Check(!did_hit, "no hit");
			Check(hit.step_count == 64, "visits all 64 cells along z");
		}
	}
}

int main()
{
	TestTerrainScene();
	TestBunnyScene();

	std::printf("\n%d / %d checks passed\n", g_checks - g_failures, g_checks);
	return g_failures == 0 ? 0 : 1;
}
