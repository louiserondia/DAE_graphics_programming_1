//==============================================================
//  Graphics Programming 1 (2026-2027)
//  Authors : Matthieu Delaere
//  Copyright (c) 2026 Matthieu Delaere. All rights reserved.
//==============================================================
#ifndef SOFTWARE_PATH_TRACER_HEADER
#define SOFTWARE_PATH_TRACER_HEADER

//--- Standard Includes ---

//--- Framework Includes ---
#include <renderer.h>
#include <vector3.h>

namespace gfx
{
	struct SurfaceSampleObjSpace
	{
		Vector3 normal{};
		Vector3 shadow_origin{};
	};

	struct HemisphereSample
	{
		Vector3 direction{};
		float pdf{}; // probablity density of that direction
	};
	//--- Software Path Tracer ---
	class SoftwarePathTracer final : public Renderer
	{

	public:
		//--- Construction / Destruction ---

		SoftwarePathTracer(Context* const context);
		~SoftwarePathTracer() override;

		SoftwarePathTracer(const SoftwarePathTracer&) = delete;
		SoftwarePathTracer& operator=(const SoftwarePathTracer&) = delete;
		SoftwarePathTracer(SoftwarePathTracer&&) = delete;
		SoftwarePathTracer& operator=(SoftwarePathTracer&&) = delete;

		//--- Public Functions ---
		void Render() override;
		void RenderPerPixel();
		bool BVHTraversal(size_t node_index,
			Ray& ray,
			const TriangleMesh* mesh,
			const std::optional<Matrix>& transform,
			bool ignore_record,
			RayHitRecord& hit) const;
		bool SceneClosestHitTest(const Ray& ray, RayHitRecord& closest_hit, bool ignore_record = false) const;
		ShadingInput ConstructShadingInput(const RayHitRecord& hit) const;
		ColorRgba CalculateRadiance(const Light* light, const Vector3& world_position) const;
		ColorRgba ConstructFinalColor(const VisualizationMode& vm, const RayHitRecord& hit, ShadingInput& si, uint32_t pixel_index);
		ColorRgba DirectIllumination(
			const ShadingInput& shading_input,
			const SurfaceProperties& sp,
			const Material* material,
			const Scene* scene) const;

		Vector4 GetRayDirection(uint32_t pixel_index, int px, int py, float w, float h);

		SurfaceSampleObjSpace ComputeNormalAndShadowOrigin(Sphere& sphere, const RayHitRecord& hit);
		SurfaceSampleObjSpace ComputeNormalAndShadowOrigin(Plane& plane, const RayHitRecord& hit);
		SurfaceSampleObjSpace ComputeNormalAndShadowOrigin(Triangle& triangle, const RayHitRecord& hit);
		SurfaceSampleObjSpace ComputeNormalAndShadowOrigin(TriangleMesh& mesh, const RayHitRecord& hit);

		// AO

		uint32_t ao_sample_count{ 1 };
		float ao_radius{ 10.f };

		float TraceAO(const ShadingInput& si, uint32_t pixel_index) const;
		static HemisphereSample UniformSampleHemisphere(const Vector3& normal, uint32_t seed);
		static HemisphereSample CosineSampleHemisphere(const Vector3& normal, uint32_t seed);

		Vector2 GenerateSubpixelJitter(uint32_t pixel_index) const;

	private:
		VisualizationMode last_vis_mode_{ VisualizationMode::kNone };
		SamplingStrategy last_sampling_strategy_{ SamplingStrategy::kCosine };
		std::vector<ColorRgba> accumulation_buffer_{};

	};
}

// if function not recognized, verify im in gfx namespace

#endif //SOFTWARE_PATH_TRACER_HEADER
