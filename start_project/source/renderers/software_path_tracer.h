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
		ColorRgba ConstructFinalColor(const VisualizationMode& vm, const RayHitRecord& hit, ShadingInput& si);
		ColorRgba DirectIllumination(
			const ShadingInput& shading_input,
			const SurfaceProperties& sp,
			const Material* material,
			const Scene* scene) const;

		Vector4 GetRayDirection(int px, int py, float w, float h);

		SurfaceSampleObjSpace ComputeNormalAndShadowOrigin(Sphere& sphere, const RayHitRecord& hit);
		SurfaceSampleObjSpace ComputeNormalAndShadowOrigin(Plane& plane, const RayHitRecord& hit);
		SurfaceSampleObjSpace ComputeNormalAndShadowOrigin(Triangle& triangle, const RayHitRecord& hit);
		SurfaceSampleObjSpace ComputeNormalAndShadowOrigin(TriangleMesh& mesh, const RayHitRecord& hit);


	};

	//uint32_t LowBias(uint32_t h);
	//float HashToFloat(const uint32_t h);

}
#endif //SOFTWARE_PATH_TRACER_HEADER
