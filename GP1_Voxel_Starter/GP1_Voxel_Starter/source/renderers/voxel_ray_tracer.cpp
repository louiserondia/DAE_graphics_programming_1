//==============================================================
//  Graphics Programming 1 (2026-2027)
//  Authors : Matthieu Delaere
//  Copyright (c) 2026 Matthieu Delaere. All rights reserved.
//==============================================================
#include <voxel_ray_tracer.h>
#include <intersections.h>
using namespace gfx;

//--- Standard Includes ---
#include <algorithm>
#include <cassert>
#include <cmath>
#include <execution>
#include <numbers>
#include <numeric>

// Local Defines
#define PARALLEL_EXECUTION_ENABLED 1

// =============================================================================
// Private Functions
// =============================================================================
bool VoxelRayTracer::SceneClosestHitTest(const Ray& ray,
	RayHitRecord& closest_hit, const bool ignore_record) const
{
	const Scene* active_scene = context_->scene_manager->GetActiveScene();
	if (!active_scene) return false;

	if (!ignore_record)
		closest_hit = {};
	return HitTestVoxelGrid(active_scene->voxel_grid, ray, closest_hit, ignore_record);
}

ShadingInput VoxelRayTracer::ConstructShadingInput(const RayHitRecord& hit) const
{
	const Scene* const active_scene = context_->scene_manager->GetActiveScene();
	const VoxelGrid& grid = active_scene->voxel_grid;

	ShadingInput shading_input = {};
	shading_input.world_position = hit.position;
	shading_input.world_normal = hit.normal;
	shading_input.view_direction = (Vector3(active_scene->camera.GetPosition()) - hit.position).Normalized();
	shading_input.shadow_origin = hit.position + hit.normal * (grid.voxel_size * 1e-3f);
	shading_input.albedo = grid.palette[hit.voxel];
	return shading_input;
}

ColorRgba VoxelRayTracer::CalculateRadiance(const Light* light,
	const Vector3& world_position)
{
	(void)world_position;
	switch (light->type)
	{
	case LightType::kDirectional:
		return light->color * light->intensity;
	case LightType::kNone:
	default:
		return ColorRgba::Black();
	}
}

ColorRgba VoxelRayTracer::DirectIllumination(const ShadingInput& shading_input,
	const Scene* scene) const
{
	ColorRgba result = ColorRgba::Black();
	ShadingInput input = shading_input;

	const Light* light = &scene->light;
	if (light->type == LightType::kNone)
		return result;

	Vector3 light_dir = -light->direction;
	light_dir.Normalize();

	const float cos_theta = Vector3::Dot(input.world_normal, light_dir);
	if (cos_theta <= 0.f)
		return result;

	// Shadow test: is the light visible from the hit point?
	const Ray shadow_ray = {
		.origin = input.shadow_origin,
		.direction = light_dir,
		.min = 0.001f,
		.max = std::numeric_limits<float>::max()
	};
	RayHitRecord shadow_hit = {};
	if (SceneClosestHitTest(shadow_ray, shadow_hit, true))
		return result;

	// Lambert BRDF
	const ColorRgba radiance = CalculateRadiance(light, input.world_position);
	input.light_direction = light_dir;
	const ColorRgba brdf = Lambert(1.f, input.albedo);
	result += radiance * cos_theta * brdf;

	return result;
}

// =============================================================================
// Public Functions
// =============================================================================
void VoxelRayTracer::Render()
{
	assert(context_ && "Context not available!");

	// Early exit when no scene
	Scene* const active_scene = context_->scene_manager->GetActiveScene();
	if (!active_scene) return;

	// Get surface info
	const SurfaceInfo& surface_info = context_->surface_info;
	const uint32_t total_pixels = surface_info.width * surface_info.height;
	if (pixel_indices_.size() != total_pixels)
	{
		pixel_indices_.resize(total_pixels);
		std::iota(pixel_indices_.begin(), pixel_indices_.end(), 0u);
	}

	// Camera setup
	const Vector3 camera_position = active_scene->camera.GetPosition();
	const Matrix& camera_view_inv_matrix = active_scene->camera.GetView().GetInverse();
	const float aspect_ratio = static_cast<float>(surface_info.width) /
		static_cast<float>(surface_info.height);
	const float fov = std::tan((active_scene->camera.GetFovAngle()
		* static_cast<float>(std::numbers::pi) / 180.f) / 2.f);
	const VoxelGrid& grid = active_scene->voxel_grid;

	// Pixel loop
	const auto per_pixel = [&](const uint32_t pixel_index)
		{
			// Setup variables.
			ColorRgba final_color = ColorRgba::Black();
			const uint32_t px = pixel_index % surface_info.width;
			const uint32_t py = pixel_index / surface_info.width;

			// Calculate pixel coordinate for ray.
			const float x = (2.f * ((static_cast<float>(px) + 0.5f) / static_cast<float>(surface_info.width)) - 1.f)
				* aspect_ratio * fov;
			const float y = (1.f - 2.f * ((static_cast<float>(py) + 0.5f) / static_cast<float>(surface_info.height))) * fov;

			// Create ray direction and normalize.
			Vector3 ray_direction = camera_view_inv_matrix.TransformVector({ x, y, 1.f });
			ray_direction.Normalize();

			// Intersection test, then shared debug visualization.
			RayHitRecord closest_hit_record = {};
			ShadingInput shading_input = {};
			bool did_hit = false;

			const Ray view_ray = { .origin = camera_position, .direction = ray_direction };
			did_hit = SceneClosestHitTest(view_ray, closest_hit_record);
			if (did_hit)
				shading_input = ConstructShadingInput(closest_hit_record);

			// Shading and debug visualization
			const auto vis_mode = context_->debug_params.visualization_mode;
			if (vis_mode == VisualizationMode::kStepCount)
			{
				// Also meaningful for missed rays: shows the traversal cost per pixel.
				const float max_steps = static_cast<float>(grid.size_x + grid.size_y + grid.size_z);
				const float s = std::min(static_cast<float>(closest_hit_record.step_count) / max_steps, 1.f);
				final_color = { s, s, s };
			}
			else if (!did_hit)
			{
				// Simple sky gradient.
				const float t = 0.5f * (ray_direction.y + 1.f);
				final_color = ColorRgba::Lerp({ 1.f, 1.f, 1.f }, { 0.5f, 0.7f, 1.f }, t);
			}
			else if (vis_mode == VisualizationMode::kNone)
			{
				final_color = DirectIllumination(shading_input, active_scene);
			}
			else if (vis_mode == VisualizationMode::kDepth)
			{
				const float max_depth = 25.f;
				const float scaled_t = 1.f - std::clamp(closest_hit_record.t / max_depth, 0.f, 1.f);
				final_color = { scaled_t, scaled_t, scaled_t };
			}
			else if (vis_mode == VisualizationMode::kNormals)
			{
				const Vector3& n = shading_input.world_normal;
				final_color = {
					(n.x + 1.f) * 0.5f,
					(n.y + 1.f) * 0.5f,
					(n.z + 1.f) * 0.5f };
			}

			// Tone mapping: HDR -> LDR (skip for debug views)
			if (vis_mode == VisualizationMode::kNone)
			{
				final_color = final_color.Tonemap(context_->debug_params.tone_map_operator);
				final_color = final_color.ToSrgb();
			}

			// Write to surface. Clamp first: not every tone map operator is bounded
			// (kNone passes HDR straight through), and casting an out-of-range float
			// to uint8_t is undefined behaviour.
			final_color.Clamp();
			surface_info.pixel_buffer[px + (py * surface_info.width)] = SDL_MapRGB(
				surface_info.pixel_format_details, nullptr,
				static_cast<uint8_t>(final_color.r * 255),
				static_cast<uint8_t>(final_color.g * 255),
				static_cast<uint8_t>(final_color.b * 255));
		};

	// Execute loop
#if PARALLEL_EXECUTION_ENABLED == 1
	std::for_each(std::execution::par_unseq, pixel_indices_.begin(), pixel_indices_.end(), per_pixel);
#else
	std::for_each(pixel_indices_.begin(), pixel_indices_.end(), per_pixel);
#endif
}
