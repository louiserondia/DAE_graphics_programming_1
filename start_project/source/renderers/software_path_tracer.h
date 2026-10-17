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

namespace gfx
{
	//--- Software Path Tracer ---
	class SoftwarePathTracer final : public Renderer
	{
	public:
		//--- Construction / Destruction ---
		bool yo;

		SoftwarePathTracer(Context* const context);
		~SoftwarePathTracer() override;

		SoftwarePathTracer(const SoftwarePathTracer&) = delete;
		SoftwarePathTracer& operator=(const SoftwarePathTracer&) = delete;
		SoftwarePathTracer(SoftwarePathTracer&&) = delete;
		SoftwarePathTracer& operator=(SoftwarePathTracer&&) = delete;

		//--- Public Functions ---
		void Render() override;
		void RenderPerPixel();
		std::optional<RayHitRecord> BVHTraversal(size_t node_index,
			const Ray& ray,
			TriangleMesh* mesh,
			const std::optional<Matrix>& transform);
		bool SceneClosestHitTest(const Ray& ray, RayHitRecord& closest_hit, bool ignore_record = false) ; // remettre le const (retire pour yo)
		ShadingInput ConstructShadingInput(const RayHitRecord& hit);
		ColorRgba ConstructFinalColor(const VisualizationMode& vm, const RayHitRecord& hit, const ShadingInput& si);
		Vector4 GetRayDirection(int px, int py, float w, float h);

	};
}
#endif //SOFTWARE_PATH_TRACER_HEADER
