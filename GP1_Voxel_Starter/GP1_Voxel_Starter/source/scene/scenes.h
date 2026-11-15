//==============================================================
//  Graphics Programming 1 (2026-2027)
//  Authors : Matthieu Delaere & Pieter-Jan Vandenberghe
//  Copyright (c) 2026 Matthieu Delaere. All rights reserved.
//==============================================================
#ifndef SCENES_HEADER
#define SCENES_HEADER

//--- Standard Includes ---
#include <cmath>
#include <numbers>

//--- Framework Includes ---
#include <scene_manager.h>
#include <asset_loader.h>

namespace gfx
{
	// =========================================================================
	// VOXEL RAY TRACER SCENES
	// =========================================================================
	// -------------------------------------------------------------------------
	// TerrainScene: rolling terrain with a floating sphere and a pillar.
	// -------------------------------------------------------------------------
	struct TerrainScene final : public Scene
	{
		void Setup() override
		{
			// Camera Setup
			camera.SetPosition({ 8.f, 7.f, -7.5f });
			camera.Pitch(-0.45f);

			// Light Setup
			light = Light{
				.direction = Vector3{ -0.5f, -1.f, -0.3f }.Normalized(),
				.color = ColorRgba::White(),
				.intensity = 6.f,
				.type = LightType::kDirectional };

			// Voxel Grid Setup (64 x 32 x 64 voxels of 0.25 units)
			voxel_grid = VoxelGrid(64, 32, 64, Vector3::Zero(), .25f);
			voxel_grid.palette[1] = { 0.30f, 0.55f, 0.20f }; // grass
			voxel_grid.palette[2] = { 0.45f, 0.32f, 0.20f }; // dirt
			voxel_grid.palette[3] = { 0.50f, 0.50f, 0.52f }; // stone
			voxel_grid.palette[4] = { 0.85f, 0.25f, 0.20f }; // red
			voxel_grid.palette[5] = { 0.20f, 0.40f, 0.85f }; // blue

			// Rolling terrain
			for (uint32_t z = 0; z < voxel_grid.size_z; ++z)
			{
				for (uint32_t x = 0; x < voxel_grid.size_x; ++x)
				{
					const float fx = static_cast<float>(x);
					const float fz = static_cast<float>(z);
					const float height_f = 4.f + 2.5f * std::sin(fx * 0.15f) + 2.5f * std::cos(fz * 0.11f);
					const uint32_t height = std::min(static_cast<uint32_t>(std::max(height_f, 1.f)), voxel_grid.size_y);
					for (uint32_t y = 0; y < height; ++y)
					{
						uint8_t value = 3;
						if (y + 1 == height) value = 1;
						else if (y + 4 >= height) value = 2;
						voxel_grid.Set(x, y, z, value);
					}
				}
			}

			// A sphere and a pillar floating above the terrain (in voxel coordinates)
			const Vector3 center{ static_cast<float>(voxel_grid.size_x) * 0.5f, 16.f, static_cast<float>(voxel_grid.size_z) * 0.5f };
			constexpr float radius{ 6.f };
			for (uint32_t z = 0; z < voxel_grid.size_z; ++z)
				for (uint32_t y = 0; y < voxel_grid.size_y; ++y)
					for (uint32_t x = 0; x < voxel_grid.size_x; ++x)
					{
						const Vector3 p{ static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f, static_cast<float>(z) + 0.5f };
						if ((p - center).SqrMagnitude() <= radius * radius)
							voxel_grid.Set(x, y, z, 4);
					}

			for (uint32_t y = 0; y < std::min(voxel_grid.size_y, 24u); ++y)
				for (uint32_t z = 8; z < 11; ++z)
					for (uint32_t x = 8; x < 11; ++x)
						voxel_grid.Set(x, y, z, 5);
		}
	};

	// -------------------------------------------------------------------------
	// BunnyScene: voxelized bunny mesh on a floor.
	// -------------------------------------------------------------------------
	struct BunnyScene final : public Scene
	{
		float light_time{ 0.f };
		bool rotating_light{ false };

		void Setup() override
		{
			// Camera setup
			camera.SetPosition({ 0.f, 2.4f, -7.f });
			camera.SetFovAngle(45.f);
			camera.Pitch(-0.15f);

			// Light setup
			light = Light{
				.direction = Vector3{ 0.5f, -1.f, 0.5f }.Normalized(),
				.color = ColorRgba::White(),
				.intensity = 6.f,
				.type = LightType::kDirectional };

			// Voxel grid setup (64 x 48 x 64 voxels of 0.1 units, centered on x/z)
			constexpr float voxel_size{ 0.1f };
			voxel_grid = VoxelGrid(64, 48, 64, Vector3(-3.2f, 0.f, -3.2f), voxel_size);
			constexpr uint8_t kFloor{ 1 }, kBunny{ 2 };
			voxel_grid.palette[kFloor] = { 0.2f, 0.7f, 0.2f };
			voxel_grid.palette[kBunny] = { 0.7f, 0.7f, 0.7f };

			// Floor
			for (uint32_t z = 0; z < voxel_grid.size_z; ++z)
				for (uint32_t x = 0; x < voxel_grid.size_x; ++x)
					voxel_grid.Set(x, 0, z, kFloor);

			// Load bunny mesh and voxelize it
			MeshData mesh{};
			AssetLoader loader{};
			if (!loader.Load("resources/models/bunny/lowpoly_bunny.obj", mesh))
				return;

			const Matrix transformation =
				Matrix::CreateRotationY(static_cast<float>(std::numbers::pi)) * Matrix::CreateScale(2.f, 2.f, 2.f);
			float min_y{ std::numeric_limits<float>::max() };
			for (Vector3& position : mesh.positions)
			{
				position = transformation.TransformPoint(position);
				min_y = std::min(min_y, position.y);
			}

			// Place the bunny on the floor.
			const Vector3 offset{ 0.f, voxel_size - min_y, 0.f };
			for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3)
			{
				voxel_grid.VoxelizeTriangle(
					mesh.positions[mesh.indices[i + 0]] + offset,
					mesh.positions[mesh.indices[i + 1]] + offset,
					mesh.positions[mesh.indices[i + 2]] + offset,
					kBunny);
			}
		}

		bool Update(const double delta_time) override
		{
			if (!rotating_light)
				return false;

			light_time += static_cast<float>(delta_time) * 0.5f;
			light.direction = Vector3{ std::cos(light_time) * 0.5f, -1.f, std::sin(light_time) * 0.5f }.Normalized();
			return true;
		}
	};
}
#endif //SCENES_HEADER
