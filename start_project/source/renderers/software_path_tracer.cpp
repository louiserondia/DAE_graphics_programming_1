//==============================================================
//  Graphics Programming 1 (2026-2027)
//  Authors : Matthieu Delaere
//  Copyright (c) 2026 Matthieu Delaere. All rights reserved.
//==============================================================
#include <software_path_tracer.h>
#include <scenes.h>
#include <intersections.h>
#include <cmath>
#include "../core/utils.h" // loulou

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
		float w{ 1 - u - v };

		Vector3 normal{ v0.normal.value() * w + v1.normal.value() * u + v2.normal.value() * v };
		normal = has_transform ? scene_object.instance_transformation.value().TransformNormal(normal) : normal;

		si.world_normal = normal;
		break;
	}
	default:
		break;
	}
	return si;
}

// question: couldnt we optimise by no even testing if our current closest hit distance in the scene is smaller than the BVHNode ?

// either closest hit parameter or return if hit
std::optional<RayHitRecord> SoftwarePathTracer::BVHTraversal(
	size_t node_index,
	const Ray& ray,
	TriangleMesh* mesh,
	const std::optional<Matrix>& transform
)
{
	BVHNode& node{ mesh->bvh_nodes[node_index] };
	RayHitRecord closest_hit{};
	AABB aabb{ node.bounds };

	//if (transform.has_value()) {
	//	aabb.min = transform.value().TransformPoint(aabb.min);
	//	aabb.max = transform.value().TransformPoint(aabb.max);
	//}

	bool did_hit_AABB{ HitTestAABB(aabb, ray) };

	if (!did_hit_AABB)
		return std::nullopt;

	if (node.IsLeaf()) {
		bool did_hit{};
		RayHitRecord temp_hit{};
		for (size_t i{ node.first_tri }; i < node.first_tri + node.tri_count; i++)
		{
			Triangle triangle{};
			did_hit = false;

			auto mesh_index{ mesh->bvh_tri_indices[i] * 3 };
			uint32_t index0{ mesh->indices[mesh_index] };
			uint32_t index1{ mesh->indices[mesh_index + 1] };
			uint32_t index2{ mesh->indices[mesh_index + 2] };

			triangle.v0 = mesh->vertices[index0].position;
			triangle.v1 = mesh->vertices[index1].position;
			triangle.v2 = mesh->vertices[index2].position;

			triangle.normal = Vector3::Cross(triangle.v1 - triangle.v0, triangle.v2 - triangle.v0).Normalized();

			did_hit = HitTestTriangle(triangle, ray, temp_hit);
			if (did_hit) {

				if (temp_hit.t < closest_hit.t)
					//if (did_hit && temp_hit.t < closest_hit.t)
				{
					closest_hit = temp_hit;
					closest_hit.vertex_indices = { index0, index1, index2 };
				}
			}
		}

		if (closest_hit.t < ray.max) {
			return closest_hit;
		}
		return std::nullopt;
	}

	auto left_child_index{ node.left_child };

	auto left{ BVHTraversal(left_child_index, ray, mesh, transform) };
	auto right{ BVHTraversal(left_child_index + 1, ray, mesh, transform) };

	if (left.has_value() && right.has_value())
		return left.value().t < right.value().t ? left : right;
	else if (left.has_value())
		return left;
	else if (right.has_value())
		return right;
	else
		return std::nullopt;
}

bool SoftwarePathTracer::SceneClosestHitTest(const Ray& ray, RayHitRecord& closest_hit, bool ignore_record)
{
	(void)ignore_record;
	Scene* scene{ context_->scene_manager->GetActiveScene() };
	const auto& factory{ scene->primitives_factory };

	Ray final_ray{ ray };
	closest_hit.t = std::numeric_limits<float>::max();

	for (size_t i{}; i < scene->objects.size(); i++)
	{
		yo = false;

		bool did_hit{};
		RayHitRecord temp_hit{};

		SceneObject& object{ scene->objects.at(i) };
		Primitive* primitive = factory.Get(object.primitive_index);

		if (object.instance_transformation.has_value())
			final_ray = object.instance_transformation.value().TransformRay(ray);

		switch (primitive->type)
		{
		case PrimitiveType::kSphere:
		{
			Sphere* sphere{ factory.GetAs<Sphere>(object.primitive_index) };

			did_hit = HitTestSphere(*sphere, final_ray, temp_hit);
			break;
		}
		case PrimitiveType::kPlane:
		{
			Plane* plane{ factory.GetAs<Plane>(object.primitive_index) };

			did_hit = HitTestPlane(*plane, final_ray, temp_hit);
			break;
		}
		case PrimitiveType::kTriangle:
		{
			Triangle* triangle{ factory.GetAs<Triangle>(object.primitive_index) };

			did_hit = HitTestTriangle(*triangle, final_ray, temp_hit);
			break;
		}
		case PrimitiveType::kTriangleMesh: // could be recursive and call as triangle ?
		{
			TriangleMesh* mesh{ static_cast<TriangleMesh*>(primitive) };

			if (!mesh->bvh_nodes.size()) break;

			auto mesh_hit{ BVHTraversal(0, ray, mesh, object.instance_transformation) };
			if (mesh_hit.has_value()) {
				did_hit = true;
				temp_hit = mesh_hit.value();
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

ColorRgba SoftwarePathTracer::ConstructFinalColor(const VisualizationMode& vm, const RayHitRecord& hit, const ShadingInput& si)
{
	ColorRgba color{};

	if (vm == VisualizationMode::kDepth) {
		const float max_depth{ 100.f };
		const float scaled_t{ 1.f - std::clamp(hit.t / max_depth, 0.f, 1.f) };
		color = { scaled_t, scaled_t, scaled_t };
	}
	else if (vm == VisualizationMode::kNone) {
		const uint32_t idx{ hit.object_index };
		color = { float(idx & 1), float((idx >> 1) & 1), float((idx >> 2) & 1) };
	}
	else if (vm == VisualizationMode::kNormals) {
		const Vector3& n{ si.world_normal };
		color = {
			(n.x + 1.f) * 0.5f,
			(n.y + 1.f) * 0.5f,
			(n.z + 1.f) * 0.5f
		};
	}

	//debug
	if (yo) return ColorRgba{ 1, 0, 0 };

	return color;
}

Vector4 SoftwarePathTracer::GetRayDirection(int px, int py, float w, float h)
{
	Scene* scene{ context_->scene_manager->GetActiveScene() };
	const float aspect_ratio{ w / h };
	const float fov{ tanf(scene->camera.GetFovAngle() / 2) };

	const float x{ (2.f * ((px + 0.5f) / w) - 1.f) * aspect_ratio * fov };
	const float y{ (1.f - 2.f * ((py + 0.5f) / h)) * fov };

	return Vector4{ x, y, 1.f, 0.f };
}

// question why do i have such low performance
void SoftwarePathTracer::Render()
{
	assert(context_ && "Context not available!");

	Scene* scene{ context_->scene_manager->GetActiveScene() };
	const SurfaceInfo& surface_info = context_->surface_info;
	const int width{ int(surface_info.width) };
	const int height{ int(surface_info.height) };

	Matrix viewMatrix{ scene->camera.GetView() };
	Matrix inverseViewMatrix{ viewMatrix.GetInverse() };

#pragma omp parallel for schedule(dynamic, 16)
	for (int py{}; py < height; ++py)
	{
		for (int px{}; px < width; ++px)
		{
			Ray view_ray{ scene->camera.GetPosition() };
			RayHitRecord closest_hit_record{};
			ShadingInput shading_input{};
			VisualizationMode vismod{ context_->debug_params.visualization_mode };
			ColorRgba final_color{};

			view_ray.direction = inverseViewMatrix * GetRayDirection(px, py, float(width), float(height));

			bool did_hit{ SceneClosestHitTest(view_ray, closest_hit_record) };
			if (did_hit) {
				shading_input = ConstructShadingInput(closest_hit_record);
				final_color = ConstructFinalColor(vismod, closest_hit_record, shading_input);
			}
			if (yo) final_color = { 1, 0, 0 };

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

