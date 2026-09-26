//==============================================================
//  Graphics Programming 1 (2026-2027)
//  Authors : Matthieu Delaere
//  Copyright (c) 2026 Matthieu Delaere. All rights reserved.
//==============================================================
#include <software_path_tracer.h>
#include <scenes.h>
#include <intersections.h>
#include <cmath>
using namespace gfx;

// =============================================================================
// Construction / Destruction
// =============================================================================
SoftwarePathTracer::SoftwarePathTracer(Context* const context)
	: Renderer(context)
{
}

SoftwarePathTracer::~SoftwarePathTracer() = default;

// =============================================================================
// Public Functions
// =============================================================================
void SoftwarePathTracer::Render()
{
	assert(context_ && "Context not available!");

	// DEMO CODE - TODO: remove!

	const SurfaceInfo& surface_info = context_->surface_info;
	const float aspectRatio{ float(surface_info.width) / float(surface_info.height) };

	for (uint32_t py = 0; py < surface_info.height; ++py)
	{
		for (uint32_t px = 0; px < surface_info.width; ++px)
		{

			const float x{ (2.f * ((px + 0.5f) / surface_info.width) - 1.f) * aspectRatio };
			const float y{ (1.f - 2.f * ((py + 0.5f) / surface_info.height)) };
			(void)aspectRatio;

			Vector3 ray_direction{ float(x), float(y), 1.f };
			ray_direction.Normalize();

			RayHitRecord closest_hit_record{};
			const Sphere test_sphere{ { 0.f, 0.f, 100.f }, 50.f };
			Ray view_ray{ Vector3{ 0.f, 0.f, 0.f }, ray_direction };
			bool did_hit{ HitTestSphere(test_sphere, view_ray, closest_hit_record) };

			ShadingInput shading_input{};
			shading_input.world_position = closest_hit_record.ray.origin;
			shading_input.world_normal = (closest_hit_record.ray.origin - test_sphere.origin)
				/ (closest_hit_record.ray.origin - test_sphere.origin).Magnitude();


			VisualizationMode vismod{ context_->debug_params.visualization_mode };
			ColorRgba final_color{};

			if (did_hit) {
				if (vismod == VisualizationMode::kDepth) {
					const float max_depth{ 100.f };
					const float scaled_t{ 1.f - std::clamp(closest_hit_record.t / max_depth, 0.f, 1.f) };
					final_color = { scaled_t, scaled_t, scaled_t };
				}
				else if (vismod == VisualizationMode::kNone) {
					final_color = { 1.f, 0.f, 0.f };
				}
				else if (vismod == VisualizationMode::kNormals) {
					const Vector3& n{ shading_input.world_normal };
					final_color = {
						(n.x + 1.f) * 0.5f,
						(n.y + 1.f) * 0.5f,
						(n.z + 1.f) * 0.5f
					};
				}
			}

			// Convert gradient value to color.
			final_color.MaxToOne();

			// Write to surface
			surface_info.pixel_buffer[px + (py * surface_info.width)] = SDL_MapRGB(
				surface_info.pixel_format_details, nullptr,
				static_cast<uint8_t>(final_color.r * 255),
				static_cast<uint8_t>(final_color.g * 255),
				static_cast<uint8_t>(final_color.b * 255));
		}
	}
}