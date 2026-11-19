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
	struct BounceSample
	{
		Vector3 direction{};
		ColorRgba color_weight{};
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
		static ColorRgba EnvironmentRadiance(const Scene* scene, const Vector3& direction);
		ColorRgba ConstructFinalColor(const VisualizationMode& vm, const RayHitRecord& hit, ShadingInput& si, uint32_t pixel_index);
		ColorRgba DirectIllumination(
			const ShadingInput& shading_input,
			const SurfaceProperties& sp,
			const Material* material,
			const Scene* scene) const;

		ColorRgba TracePath(const Ray& ray, const Scene* scene, uint32_t& hash_state, int max_bounces = 8) const;
		Vector4 GetRayDirection(uint32_t pixel_index, int px, int py, float w, float h, uint32_t& hash_state);

		// AO

		uint32_t ao_sample_count{ 1 };
		float ao_radius{ 10.f };

		float TraceAO(const ShadingInput& si, uint32_t pixel_index) const;

		Vector2 GenerateSubpixelJitter(uint32_t pixel_index, uint32_t& hash_state) const;

	private:
		VisualizationMode last_vis_mode_{ VisualizationMode::kNone };
		SamplingStrategy last_sampling_strategy_{ SamplingStrategy::kCosine };
		ToneMapOperator last_tone_map_operator{ ToneMapOperator::kACESHill };
		std::vector<ColorRgba> accumulation_buffer_{};

	};
}

// if function not recognized, verify im in gfx namespace

#endif //SOFTWARE_PATH_TRACER_HEADER
