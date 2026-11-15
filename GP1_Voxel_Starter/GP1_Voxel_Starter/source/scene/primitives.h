//==============================================================
//  Graphics Programming 1 (2026-2027)
//  Authors : Matthieu Delaere & Pieter-Jan Vandenberghe
//  Copyright (c) 2026 Matthieu Delaere. All rights reserved.
//==============================================================
#ifndef PRIMITIVES_HEADER
#define PRIMITIVES_HEADER

//--- Standard Includes ---
#include <array>
#include <cstdint>
#include <limits>
#include <optional>
#include <vector>

//--- Framework Includes ---
#include <vector3.h>
#include <color.h>

namespace gfx
{
	//--- Ray Tracing Support Structs ---
	struct Ray final
	{
		Vector3 origin{};
		Vector3 direction{};
		float min{ 1e-4f };
		float max{ std::numeric_limits<float>::max() };
	};

	struct RayHitRecord final
	{
		float t{ std::numeric_limits<float>::max() };
		Vector3 position{};
		Vector3 normal{};
		uint8_t voxel{ 0 };
		uint32_t step_count{ 0 };
	};

	//--- General Supporting Primitives ---
	struct AABB final
	{
		Vector3 min{ std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max() };
		Vector3 max{ std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest() };

		AABB() = default;
		AABB(const Vector3& min_point, const Vector3& max_point) : min(min_point), max(max_point) {}
	};

	//--- Lights ---
	enum class LightType : uint8_t
	{
		kNone,
		kDirectional
	};

	struct Light final
	{
		Vector3 direction{ 0.f, -1.f, 0.f }; // Direction the light travels in.
		ColorRgba color{ ColorRgba::White() };
		float intensity{ 1.f };
		LightType type{ LightType::kNone };
	};

	//--- Dense Voxel Grid ---
	// Every voxel is one byte: 0 = empty, any other value indexes the color palette.
	// The grid is axis aligned, starts at origin and each voxel is voxel_size wide.
	struct VoxelGrid final
	{
		std::vector<uint8_t> voxels{};
		std::array<ColorRgba, 256> palette{};
		uint32_t size_x{ 0 };
		uint32_t size_y{ 0 };
		uint32_t size_z{ 0 };
		Vector3 origin{ 0.f, 0.f, 0.f };
		float voxel_size{ 1.f };
		std::optional<AABB> bounding_box{};

		VoxelGrid() { palette.fill(ColorRgba::Magenta()); }
		VoxelGrid(const uint32_t sx, const uint32_t sy, const uint32_t sz,
			const Vector3& o = Vector3::Zero(), const float vs = 1.f) :
			voxels(static_cast<uint64_t>(sx) * sy * sz, 0),
			size_x(sx), size_y(sy), size_z(sz),
			origin(o), voxel_size(vs)
		{
			palette.fill(ColorRgba::Magenta());
			UpdateAABB();
		}

		void Set(const uint32_t x, const uint32_t y, const uint32_t z, const uint8_t value);
		[[nodiscard]] uint8_t Get(const uint32_t x, const uint32_t y, const uint32_t z) const;
		void Clear();
		void UpdateAABB();

		// Marks every voxel the (world space) triangle passes through with value.
		void VoxelizeTriangle(const Vector3& v0, const Vector3& v1, const Vector3& v2, const uint8_t value);

		[[nodiscard]] uint64_t ToIndex(const uint32_t x, const uint32_t y, const uint32_t z) const
		{ return x + static_cast<uint64_t>(size_x) * (y + static_cast<uint64_t>(size_y) * z); }
	};
}
#endif //PRIMITIVES_HEADER
