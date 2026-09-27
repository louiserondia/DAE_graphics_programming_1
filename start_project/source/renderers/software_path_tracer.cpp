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

ShadingInput ConstructShadingInput(const RayHitRecord& hit) {
	ShadingInput si{};

	si.world_position = hit.ray.origin;
	si.world_normal = hit.ray.direction;
	return si;
}

void SoftwarePathTracer::Render()
{
	assert(context_ && "Context not available!");

	// DEMO CODE - TODO: remove!

	Scene* scene{ context_->scene_manager->GetActiveScene() };

	const SurfaceInfo& surface_info = context_->surface_info;
	const float aspectRatio{ float(surface_info.width) / float(surface_info.height) };
	Ray view_ray{};

	for (uint32_t py = 0; py < surface_info.height; ++py)
	{
		for (uint32_t px = 0; px < surface_info.width; ++px)
		{
			const float x{ (2.f * ((px + 0.5f) / surface_info.width) - 1.f) * aspectRatio };
			const float y{ (1.f - 2.f * ((py + 0.5f) / surface_info.height)) };

			Vector3 ray_direction{ float(x), float(y), 1.f };
			ray_direction.Normalize();
			view_ray.direction = ray_direction;

			RayHitRecord closest_hit_record{};
			bool did_hit{ scene->SceneClosestHitTest(view_ray, closest_hit_record) };

			ShadingInput shading_input{};
			VisualizationMode vismod{ context_->debug_params.visualization_mode };
			ColorRgba final_color{};

			if (did_hit) {
				shading_input = ConstructShadingInput(closest_hit_record);

				if (vismod == VisualizationMode::kDepth) {
					const float max_depth{ 100.f };
					const float scaled_t{ 1.f - std::clamp(closest_hit_record.t / max_depth, 0.f, 1.f) };
					final_color = { scaled_t, scaled_t, scaled_t };
				}
				else if (vismod == VisualizationMode::kNone) {
					if (!closest_hit_record.object_index)
						final_color = ColorRgba{ 1.f, 0.f, 0.f, 0.01f };
					if (closest_hit_record.object_index == 1)
						final_color = ColorRgba{ 0.f, 1.f, 0.f, 0.f };
					if (closest_hit_record.object_index == 2)
						final_color = ColorRgba{ 0.f, 0.f ,1.f, 0.01f };
					if (closest_hit_record.object_index == 3)
						final_color = ColorRgba{ 1.f, 1.f, 0.f, 0.01f };
					if (closest_hit_record.object_index == 4)
						final_color = ColorRgba{ 1.f, 0.f, 1.f, 0.01f };
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