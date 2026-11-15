//==============================================================
//  Graphics Programming 1 (2026-2027)
//  Authors : Matthieu Delaere
//  Copyright (c) 2026 Matthieu Delaere. All rights reserved.
//==============================================================
#ifndef VOXEL_RAY_TRACER_HEADER
#define VOXEL_RAY_TRACER_HEADER

//--- Standard Includes ---
#include <vector>

//--- Framework Includes ---
#include <renderer.h>

namespace gfx
{
	//--- Voxel Ray Tracer ---
	class VoxelRayTracer final : public Renderer
	{
		//--- Private Members ---
		std::vector<uint32_t> pixel_indices_ = {};

		//--- Private Functions ---
		bool SceneClosestHitTest(const Ray& ray,
			RayHitRecord& closest_hit, bool ignore_record = false) const;
		ShadingInput ConstructShadingInput(const RayHitRecord& hit) const;
		static ColorRgba CalculateRadiance(const Light* light, const Vector3& world_position);

		ColorRgba DirectIllumination(const ShadingInput& shading_input,
			const Scene* scene) const;

	public:
		//--- Construction / Destruction ---
		VoxelRayTracer(Context* const context) : Renderer(context) {}
		~VoxelRayTracer() override = default;

		VoxelRayTracer(const VoxelRayTracer&) = delete;
		VoxelRayTracer& operator=(const VoxelRayTracer&) = delete;
		VoxelRayTracer(VoxelRayTracer&&) = delete;
		VoxelRayTracer& operator=(VoxelRayTracer&&) = delete;

		//--- Public Functions ---
		void Render() override;
	};
}
#endif //VOXEL_RAY_TRACER_HEADER
