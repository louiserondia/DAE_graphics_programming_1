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
		auto v0{ mesh->vertices[vertices_index[0]] };
		auto v1{ mesh->vertices[vertices_index[1]] };
		auto v2{ mesh->vertices[vertices_index[2]] };

		const float u{ hit.barycentric_coordinates.value().x };
		const float v{ hit.barycentric_coordinates.value().y };
		float w{1 - u - v };
	
		Vector3 normal{ v0.normal.value() * w + v1.normal.value() * u + v2.normal.value() * v };
		//Vector3 normal{ Vector3::Cross(v1 - v0, v2 - v0).Normalized() };
		normal = has_transform ? scene_object.instance_transformation.value().TransformNormal(normal) : normal;

		si.world_normal = normal;
		break;
	}
	default:
		break;
	}
	return si;
}

bool SoftwarePathTracer::SceneClosestHitTest(const Ray & ray, RayHitRecord & closest_hit, bool ignore_record) const
{
	(void)ignore_record;
	RayHitRecord temp_hit{};
	bool did_hit{};
	closest_hit.t = std::numeric_limits<float>::max();
	Ray final_ray{ ray };
	Scene* scene{ context_->scene_manager->GetActiveScene() };

	for (size_t i{}; i < scene->objects.size(); i++)
	{
		did_hit = false;
		temp_hit = RayHitRecord{}; // could only reset t ?
		Primitive* primitive = scene->primitives_factory.Get(scene->objects.at(i).primitive_index);

		if (scene->objects.at(i).instance_transformation.has_value())
			final_ray = scene->objects.at(i).instance_transformation.value().TransformRay(ray);

		switch (primitive->type)
		{
		case PrimitiveType::kSphere:
		{
			//Sphere* sphere{ static_cast<Sphere*>(primitive) };
			Sphere* sphere{ scene->primitives_factory.GetAs<Sphere>(scene->objects.at(i).primitive_index) };

			did_hit = HitTestSphere(*sphere, final_ray, temp_hit);
			break;
		}
		case PrimitiveType::kPlane:
		{
			//Plane* plane{ static_cast<Plane*>(primitive) };
			Plane* plane{ scene->primitives_factory.GetAs<Plane>(scene->objects.at(i).primitive_index) };

			did_hit = HitTestPlane(*plane, final_ray, temp_hit);
			break;
		}
		case PrimitiveType::kTriangle:
		{
			//Triangle* triangle{ static_cast<Triangle*>(primitive) };
			Triangle* triangle{ scene->primitives_factory.GetAs<Triangle>(scene->objects.at(i).primitive_index) };

			did_hit = HitTestTriangle(*triangle, final_ray, temp_hit);
			break;
		}
		case PrimitiveType::kTriangleMesh:
		{
			TriangleMesh* mesh{ static_cast<TriangleMesh*>(primitive) };
			bool mesh_did_hit{};
			RayHitRecord mesh_temp_hit{};

			for (size_t j{}; j < mesh->indices.size(); j += 3)
			{
				mesh_did_hit = false;
				mesh_temp_hit = RayHitRecord{}; // could only reset t ?

				Triangle triangle{};
				uint32_t index0{ mesh->indices[j] };
				uint32_t index1{ mesh->indices[j + 1] };
				uint32_t index2{ mesh->indices[j + 2] };

				triangle.v0 = mesh->vertices[index0].position;
				triangle.v1 = mesh->vertices[index1].position;
				triangle.v2 = mesh->vertices[index2].position;
				triangle.normal = Vector3::Cross(triangle.v1 - triangle.v0, triangle.v2 - triangle.v0).Normalized();

				mesh_did_hit = HitTestTriangle(triangle, final_ray, mesh_temp_hit);
				if (mesh_did_hit && mesh_temp_hit.t < temp_hit.t)
				{
					did_hit = true;
					temp_hit = mesh_temp_hit;
					temp_hit.vertex_indices = { index0, index1, index2 };
				}
			}
			break;
		}
		case PrimitiveType::kNone:
		default:
			break;
		}

		if (did_hit and temp_hit.t < closest_hit.t)
		{
			closest_hit = temp_hit;
			closest_hit.object_index = uint32_t(i);
		}
	}
	if (closest_hit.t < ray.max)
	{
		return true;
	}

	return false;
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

	//for (uint32_t py = uint32_t(height / 2); py < uint32_t(height); ++py) // for performance
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
			bool did_hit{ SceneClosestHitTest(view_ray, closest_hit_record) };

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

