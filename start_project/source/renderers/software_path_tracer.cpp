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
{}

SoftwarePathTracer::~SoftwarePathTracer() = default;

// =============================================================================
// Public Functions
// =============================================================================

ShadingInput SoftwarePathTracer::ConstructShadingInput(const RayHitRecord& hit) {
	ShadingInput si{};
	Scene* scene{ context_->scene_manager->GetActiveScene() };
	SceneObject& scene_object{ scene->objects[hit.object_index] };
	Primitive* primitive = scene->primitives_factory.Get(scene_object.primitive_index);

	bool has_transform{};

	si.world_position = hit.ray.origin + hit.t * hit.ray.direction;
	if (scene_object.instance_transformation.has_value()) {
		si.world_position = scene_object.instance_transformation.value().TransformPoint(si.world_position);
		si.view_direction = scene_object.instance_transformation.value().TransformVector(si.view_direction);
		has_transform = true;
	}

	switch (primitive->type)
	{
	case PrimitiveType::kSphere:
	{
		Sphere* sphere{ static_cast<Sphere*>(primitive) };

		const Vector3 origin{ has_transform ? scene_object.instance_transformation.value()[3] : sphere->origin };
		si.world_normal = (si.world_position - origin) / sphere->radius;
		break;
	}
	case PrimitiveType::kPlane:
	{
		Plane* plane{ static_cast<Plane*>(primitive) };

		const Vector3 normal{ has_transform ? scene_object.instance_transformation.value().TransformNormal(plane->normal) : plane->normal };
		si.world_normal = normal;
		break;
	}
	case PrimitiveType::kTriangle:
	{
		Triangle* triangle{ static_cast<Triangle*>(primitive) };

		const Vector3 normal{ has_transform ? scene_object.instance_transformation.value().TransformNormal(triangle->normal) : triangle->normal };
		si.world_normal = normal;
		break;
	}
	case PrimitiveType::kTriangleMesh:
	{
		TriangleMesh* mesh{ static_cast<TriangleMesh*>(primitive) };

		auto vertices_index{ hit.vertex_indices.value() };
		Vector3 v0{ mesh->vertices[vertices_index[0]].position };
		Vector3 v1{ mesh->vertices[vertices_index[1]].position };
		Vector3 v2{ mesh->vertices[vertices_index[2]].position };

		Vector3 normal{ Vector3::Cross(v1 - v0, v2 - v0).Normalized() };
		normal = has_transform ? scene_object.instance_transformation.value().TransformNormal(normal) : normal;

		si.world_normal = normal;
		break;
	}
	default:
		break;
	}
	return si;
}

void SoftwarePathTracer::Render()
{
	assert(context_ && "Context not available!");

	Scene* scene{ context_->scene_manager->GetActiveScene() };

	const SurfaceInfo& surface_info = context_->surface_info;
	const uint32_t width{ surface_info.width }, height{ surface_info.height };
	const float aspect_ratio{ float(width) / float(height) };
	const float fov{ tanf(scene->camera.GetFovAngle() / 2) };

	Ray view_ray{ scene->camera.GetPosition() };
	Matrix viewMatrix{ scene->camera.GetView() };
	Matrix inverseViewMatrix{ viewMatrix.GetInverse() };

	//for (uint32_t py = uint32_t(height / 5); py < uint32_t(height * 4 / 5); ++py) // for performance
	//{
	//	for (uint32_t px = uint32_t(width / 4); px < uint32_t(width * 3 / 4); ++px)
	//	{
	for (uint32_t py{}; py < height; ++py)
	{
		for (uint32_t px{}; px < width; ++px)
		{
			const float x{ (2.f * ((px + 0.5f) / width) - 1.f) * aspect_ratio * fov };
			const float y{ (1.f - 2.f * ((py + 0.5f) / height)) * fov };

			Vector4 ray_direction{ float(x), float(y), 1.f, 0.f };
			view_ray.direction = inverseViewMatrix * ray_direction;

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
					const uint32_t idx{ closest_hit_record.object_index };
					final_color = { float(idx & 1), float((idx >> 1) & 1), float((idx >> 2) & 1) };
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
			surface_info.pixel_buffer[px + (py * width)] = SDL_MapRGB(
				surface_info.pixel_format_details, nullptr,
				uint8_t(final_color.r * 255),
				uint8_t(final_color.g * 255),
				uint8_t(final_color.b * 255));
		}
	}
}

