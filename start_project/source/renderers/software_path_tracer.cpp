//==============================================================
//  Graphics Programming 1 (2026-2027)
//  Authors : Matthieu Delaere
//  Copyright (c) 2026 Matthieu Delaere. All rights reserved.
//==============================================================
#include <software_path_tracer.h>
#include <scenes.h>
#include <intersections.h>
#include <cmath>
#include <limits>
#include "../core/utils.h" // loulou
#include <variant>

using namespace gfx;
using Primitive = std::variant<Sphere, Plane, Triangle, TriangleMesh>;

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

	Scene* scene{ context_->scene_manager->GetActiveScene() };
	const SurfaceInfo& surface_info = context_->surface_info;
	const int width{ int(surface_info.width) };
	const int height{ int(surface_info.height) };

	const Matrix& viewMatrix{ scene->camera.GetView() };
	const Matrix& inverseViewMatrix{ viewMatrix.GetInverse() };
	VisualizationMode vismod{ context_->debug_params.visualization_mode };
	(void)vismod;

	Vector3 test{};
	Vector3 n{ Vector3{ 3, 15, 10 }.Normalized() };
	test = LocalToWorld(Vector3{ 0, 0, 1 }, n);


#pragma omp parallel for schedule(dynamic, 16)
	for (int py{}; py < height; ++py)
	{
		for (int px{}; px < width; ++px)
		{
			Ray view_ray{ scene->camera.GetPosition() };
			RayHitRecord closest_hit_record{};
			ShadingInput shading_input{};
			ColorRgba final_color{};

			view_ray.direction = inverseViewMatrix * GetRayDirection(px, py, float(width), float(height));
			view_ray.direction.Normalize();

			bool did_hit{ SceneClosestHitTest(view_ray, closest_hit_record, false) };
			if (did_hit)
			{
				shading_input = ConstructShadingInput(closest_hit_record);
				final_color = ConstructFinalColor(vismod, closest_hit_record, shading_input, GetIndex(py, px, width));
			}

			// Write to surface
			surface_info.pixel_buffer[px + (py * width)] = SDL_MapRGB(
				surface_info.pixel_format_details, nullptr,
				uint8_t(final_color.r * 255),
				uint8_t(final_color.g * 255),
				uint8_t(final_color.b * 255));
		}
	}
}

bool SoftwarePathTracer::SceneClosestHitTest(const Ray& ray, RayHitRecord& closest_hit, bool ignore_record) const
{
	Scene* scene{ context_->scene_manager->GetActiveScene() };
	const Factory<Primitive>& factory{ scene->primitives_factory };

	Ray final_ray{ ray };
	closest_hit.t = std::numeric_limits<float>::max(); // can go down?

	for (size_t i{}; i < scene->objects.size(); i++)
	{
		bool did_hit{};
		RayHitRecord temp_hit{};
		SceneObject& object{ scene->objects[i] };
		PrimitiveType primitive_type = factory.Get(object.primitive_index)->type;

		if (object.instance_transformation.has_value())
			final_ray = object.instance_transformation->TransformRay(ray);

		switch (primitive_type)
		{
		case PrimitiveType::kSphere:
		{
			Sphere* sphere{ factory.GetAs<Sphere>(object.primitive_index) };

			did_hit = HitTestSphere(*sphere, final_ray, temp_hit, ignore_record);
			break;
		}
		case PrimitiveType::kPlane:
		{
			Plane* plane{ factory.GetAs<Plane>(object.primitive_index) };

			did_hit = HitTestPlane(*plane, final_ray, temp_hit, ignore_record);
			break;
		}
		case PrimitiveType::kTriangle:
		{
			Triangle* triangle{ factory.GetAs<Triangle>(object.primitive_index) };

			did_hit = HitTestTriangle(*triangle, final_ray, temp_hit, ignore_record);
			break;
		}
		case PrimitiveType::kTriangleMesh:
		{
			TriangleMesh* mesh{ factory.GetAs<TriangleMesh>(object.primitive_index) };

			if (!mesh->bvh_nodes.size()) break;

			did_hit = BVHTraversal(0, final_ray, mesh, object.instance_transformation, ignore_record, temp_hit);

			// for shadow ray, we just want to know if sth hit, so return instantly
			if (did_hit && ignore_record) return true;

			break;
		}
		case PrimitiveType::kNone:
		default:
			break;
		}

		if (did_hit && ignore_record) return true;

		if (did_hit && temp_hit.t < closest_hit.t)
		{
			closest_hit = temp_hit;
			closest_hit.object_index = uint32_t(i);
			final_ray.max = closest_hit.t;
		}
	}

	if (closest_hit.t < ray.max) return true;
	return false;
}

ShadingInput SoftwarePathTracer::ConstructShadingInput(const RayHitRecord& hit) const
{
	ShadingInput si{};
	Scene* scene{ context_->scene_manager->GetActiveScene() };
	const Factory<Primitive>& factory{ scene->primitives_factory };
	const SceneObject& object{ scene->objects[hit.object_index] };
	PrimitiveType primitive_type = factory.Get(object.primitive_index)->type;

	bool has_transform{};
	const Vector3 objectspace_position{ hit.ray.origin + hit.t * hit.ray.direction };
	si.world_position = objectspace_position;
	si.view_direction = -hit.ray.direction.Normalized(); // from point to camera

	if (object.instance_transformation.has_value())
	{
		si.world_position = object.instance_transformation->TransformPoint(si.world_position);
		si.view_direction = object.instance_transformation->TransformVector(si.view_direction).Normalized();
		has_transform = true;
	}

	Vector3 objectspace_normal{};
	switch (primitive_type)
	{
	case PrimitiveType::kSphere:
	{
		Sphere* sphere{ factory.GetAs<Sphere>(object.primitive_index) };

		si.shadow_origin = objectspace_position;
		objectspace_normal = (objectspace_position - sphere->origin) / sphere->radius;
		break;
	}
	case PrimitiveType::kPlane:
	{
		Plane* plane{ factory.GetAs<Plane>(object.primitive_index) };

		si.shadow_origin = objectspace_position;
		objectspace_normal = plane->normal;
		break;
	}
	case PrimitiveType::kTriangle:
	{
		Triangle* triangle{ factory.GetAs<Triangle>(object.primitive_index) };

		si.shadow_origin = objectspace_position;
		objectspace_normal = triangle->normal;
		break;
	}
	case PrimitiveType::kTriangleMesh:
	{
		TriangleMesh* mesh{ factory.GetAs<TriangleMesh>(object.primitive_index) };
		//const std::array<float, 3> weights{ w, u, v };

		const auto& vertices_index{ *hit.vertex_indices };
		const Vector3 p0{ mesh->vertices[vertices_index[0]].position };
		const Vector3 p1{ mesh->vertices[vertices_index[1]].position };
		const Vector3 p2{ mesh->vertices[vertices_index[2]].position };
		const Vector3 n0{ *mesh->vertices[vertices_index[0]].normal };
		const Vector3 n1{ *mesh->vertices[vertices_index[1]].normal };
		const Vector3 n2{ *mesh->vertices[vertices_index[2]].normal };

		const float u{ hit.barycentric_coordinates->x };
		const float v{ hit.barycentric_coordinates->y };
		const float w{ 1.f - u - v };

		float dotw = std::min(0.f, Vector3::Dot(objectspace_position - p0, n0));
		float dotu = std::min(0.f, Vector3::Dot(objectspace_position - p1, n1));
		float dotv = std::min(0.f, Vector3::Dot(objectspace_position - p2, n2));

		si.shadow_origin = objectspace_position - (w * dotw * n0 + u * dotu * n1 + v * dotv * n2);
		objectspace_normal = n0 * w + n1 * u + n2 * v;

		if (mesh->vertices[vertices_index[0]].texture_coordinates.has_value())
		{
			const Vector2 tc0{ *mesh->vertices[vertices_index[0]].texture_coordinates };
			const Vector2 tc1{ *mesh->vertices[vertices_index[1]].texture_coordinates };
			const Vector2 tc2{ *mesh->vertices[vertices_index[2]].texture_coordinates };
			si.texture_coordinates = tc0 * w + tc1 * u + tc2 * v;
		}
		break;
	}
	default:
		break;
	}

	si.world_normal = objectspace_normal.Normalized();
	if (has_transform)
	{
		si.shadow_origin = object.instance_transformation->TransformPoint(si.shadow_origin);
		si.world_normal = object.instance_transformation->TransformNormal(si.world_normal);
	}
	si.world_tangent = Vector3::Cross(Vector3{ 0, 1, 0 }, si.world_normal).Normalized();
	si.world_bitangent = Vector3::Cross(si.world_normal, *si.world_tangent).Normalized();

	return si;
}

// question: couldnt we optimise by not even testing if our current closest hit distance in the scene is smaller than the BVHNode ?

ColorRgba SoftwarePathTracer::DirectIllumination(
	const ShadingInput& shading_input,
	const SurfaceProperties& sp,
	const Material* material,
	const Scene* scene) const
{
	ColorRgba result{};
	ShadingInput si{ shading_input };

	for (const Light* light : scene->lights_factory.GetAll())
	{
		Vector3 light_dir{ light->type == LightType::kDirectional ? -light->direction : light->origin - si.world_position };
		const float light_distance{ light_dir.Normalize() };
		si.light_direction = light_dir;

		// lambert's cosine law : lights below horizon dont contribute
		const float cos_theta{ Vector3::Dot(si.world_normal, light_dir) };
		if (cos_theta <= 0) continue;

		const Ray shadow_ray{
			.origin = si.shadow_origin,
			.direction = light_dir,
			.min = 0.001f,
			.max = light->type == LightType::kDirectional ? std::numeric_limits<float>::max() : light_distance
		};
		RayHitRecord shadow_hit{};

		if (SceneClosestHitTest(shadow_ray, shadow_hit, true)) // light hits another object before reaching our point
			continue;

		const ColorRgba brdf{ material->Shade(si, sp) };
		result += cos_theta * brdf * CalculateRadiance(light, si.world_position);
	}
	return result;
}


ColorRgba SoftwarePathTracer::ConstructFinalColor(const VisualizationMode& vismod, const RayHitRecord& hit, ShadingInput& si, uint32_t pixel_index)
{
	Scene* scene{ context_->scene_manager->GetActiveScene() };
	const SceneObject& scene_object{ scene->objects[hit.object_index] };
	const Material* material{ scene->materials_factory.Get(scene_object.material_index) };
	const SurfaceProperties surface_prop{ material->GetSurfaceProperties(si, scene->textures_factory) };
	ColorRgba color{};

	// either overwrites with normal texture or keeps the geometric normal
	si.world_normal = surface_prop.shading_normal;

	switch (vismod)
	{
	case VisualizationMode::kNone:
	{
		//color = DirectIllumination(si, surface_prop, material, scene);

		//color = color.Tonemap(context_->debug_params.tone_map_operator);
		//color = color.ToSrgb();
		const float ao{ TraceAO(si, pixel_index) };
		color = Vector3{ ao, ao, ao };

		break;
	}
	case VisualizationMode::kNormals:
	{
		const Vector3& n{ si.world_normal };
		color = {
			(n.x + 1.f) * 0.5f,
			(n.y + 1.f) * 0.5f,
			(n.z + 1.f) * 0.5f
		};
		break;
	}
	case VisualizationMode::kAlbedo:
		color = surface_prop.albedo;
		break;
	case VisualizationMode::kMetallicRoughness:
		// red AO, green roughness, blue metallic
		color = ColorRgba{ 0.f, surface_prop.roughness, surface_prop.metallic };
		break;
	case VisualizationMode::kDepth:
	{
		const float max_depth{ 100.f };
		const float scaled_t{ 1.f - std::clamp(hit.t / max_depth, 0.f, 1.f) };
		color = { scaled_t, scaled_t, scaled_t };
		break;
	}
	case VisualizationMode::kUV:
	{
		// use checker board texture
		// color = ColorRgba{hit.uv.x, hit.uv.y, 0.0f};
		break;
	}
	case VisualizationMode::kBounceCount:
	case VisualizationMode::kDirectOnly:
	{
		color = DirectIllumination(si, surface_prop, material, scene);
		color = color.Tonemap(context_->debug_params.tone_map_operator);
		color = color.ToSrgb();
	}
	case VisualizationMode::kMax:
	default:
		break;
	}

	color.Clamp();
	return color;
}

bool SoftwarePathTracer::BVHTraversal(
	size_t node_index,
	Ray& ray,
	const TriangleMesh* mesh,
	const std::optional<Matrix>& transform,
	bool ignore_record,
	RayHitRecord& best_hit) const
{
	const BVHNode& node{ mesh->bvh_nodes[node_index] };
	const AABB& aabb{ node.bounds };

	if (!HitTestAABB(aabb, ray))
		return false;

	bool did_hit{};
	if (node.IsLeaf()) {
		RayHitRecord closest_hit{};
		const size_t end{ node.first_tri + node.tri_count };
		for (size_t i{ node.first_tri }; i < end; i++)
		{
			uint32_t mesh_index{ mesh->bvh_tri_indices[i] * 3 };
			uint32_t index0{ mesh->indices[mesh_index] };
			uint32_t index1{ mesh->indices[mesh_index + 1] };
			uint32_t index2{ mesh->indices[mesh_index + 2] };

			Triangle triangle{
				mesh->vertices[index0].position,
				mesh->vertices[index1].position,
				mesh->vertices[index2].position
			};

			if (!HitTestTriangle(triangle, ray, closest_hit, ignore_record)) continue;

			if (ignore_record) return true;

			if (closest_hit.t < best_hit.t)
			{
				did_hit = true;
				closest_hit.vertex_indices = { index0, index1, index2 };
				best_hit = closest_hit;
				ray.max = closest_hit.t;
			}
		}
		return did_hit;
	}

	auto left_child_index{ node.left_child };

	// visit front child then back
	bool left{ BVHTraversal(left_child_index, ray, mesh, transform, ignore_record, best_hit) };
	bool right{ BVHTraversal(left_child_index + 1, ray, mesh, transform, ignore_record, best_hit) };

	return left || right;
}

ColorRgba SoftwarePathTracer::CalculateRadiance(const Light* light, const Vector3& world_position) const
{
	switch (light->type)
	{
	case LightType::kPoint:
	{
		const float dist_square{ (world_position - light->origin).SqrMagnitude() };

		return light->color * (light->intensity / dist_square);
	}
	case LightType::kDirectional:
		return light->color * light->intensity;
	default:
		return ColorRgba{};
	}
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

SurfaceSampleObjSpace SoftwarePathTracer::ComputeNormalAndShadowOrigin(Sphere& sphere, const RayHitRecord& hit)
{
	const Vector3 position{ hit.ray.origin + hit.t * hit.ray.direction };

	Vector3 normal{ (position - sphere.origin) / sphere.radius };
	return SurfaceSampleObjSpace{ normal, position };
}

SurfaceSampleObjSpace SoftwarePathTracer::ComputeNormalAndShadowOrigin(Plane& plane, const RayHitRecord& hit)
{
	const Vector3 position{ hit.ray.origin + hit.t * hit.ray.direction };

	return SurfaceSampleObjSpace{ plane.normal, position };
}

SurfaceSampleObjSpace SoftwarePathTracer::ComputeNormalAndShadowOrigin(Triangle& triangle, const RayHitRecord& hit)
{
	const Vector3 position{ hit.ray.origin + hit.t * hit.ray.direction };

	return SurfaceSampleObjSpace{ triangle.normal, position };
}

SurfaceSampleObjSpace SoftwarePathTracer::ComputeNormalAndShadowOrigin(TriangleMesh& mesh, const RayHitRecord& hit)
{
	const Vector3 position{ hit.ray.origin + hit.t * hit.ray.direction };

	const auto& vertices_index{ *hit.vertex_indices };
	const Vector3 p0{ mesh.vertices[vertices_index[0]].position };
	const Vector3 p1{ mesh.vertices[vertices_index[1]].position };
	const Vector3 p2{ mesh.vertices[vertices_index[2]].position };
	const Vector3 n0{ *mesh.vertices[vertices_index[0]].normal };
	const Vector3 n1{ *mesh.vertices[vertices_index[1]].normal };
	const Vector3 n2{ *mesh.vertices[vertices_index[2]].normal };

	const float u{ hit.barycentric_coordinates->x };
	const float v{ hit.barycentric_coordinates->y };
	const float w{ 1.f - u - v };

	float dotw = std::min(0.f, Vector3::Dot(position - p0, n0));
	float dotu = std::min(0.f, Vector3::Dot(position - p1, n1));
	float dotv = std::min(0.f, Vector3::Dot(position - p2, n2));

	Vector3 normal{ n0 * w + n1 * u + n2 * v };
	Vector3 shadow_origin{ position - (w * dotw * n0 + u * dotu * n1 + v * dotv * n2) };
	return SurfaceSampleObjSpace{ normal, shadow_origin };
}

float gfx::SoftwarePathTracer::TraceAO(const ShadingInput& si, uint32_t pixel_index) const
{
	float sum{};
	for (uint32_t index{}; index < ao_sample_count; ++index)
	{
		// multipliers need to be odd and coprime together
		RayHitRecord hit{};
		const uint32_t ao_seed{ pixel_index * 1973u + index * 6133u };

		bool use_uniform{ context_->debug_params.sampling_strategy == SamplingStrategy::kUniform };

		HemisphereSample sample{ use_uniform ?
			UniformSampleHemisphere(si.world_normal, ao_seed) :
			CosineSampleHemisphere(si.world_normal, ao_seed)};

		Ray ao_ray{
			.origin = si.shadow_origin,
			.direction = sample.direction,
			.min = 0.001f,
			.max = ao_radius
		};

		if (SceneClosestHitTest(ao_ray, hit, true)) continue;

		const float pi{ static_cast<float>(std::numbers::pi) };
		const float cos_theta{ std::max(0.f, Vector3::Dot(sample.direction, si.world_normal)) };
		const float f{ cos_theta / pi };

		if (sample.pdf > 0)
			sum += f / sample.pdf;
	}

	return sum /= float(ao_sample_count);
}

HemisphereSample gfx::SoftwarePathTracer::UniformSampleHemisphere(const Vector3& normal, uint32_t seed)
{
	uint32_t h{ LowBias(seed) };
	const float u1{ HashToFloat(h) };
	h = LowBias(h);
	const float u2{ HashToFloat(h) };

	const float sin_theta{ sqrt((1 - u2 * u2)) };
	const float pi{ static_cast<float>(std::numbers::pi) };
	const float phi{ 2 * pi * u1 };

	Vector3 local_dir{ sin_theta * cos(phi), sin_theta * sin(phi), u2 };

	return HemisphereSample{
		.direction = LocalToWorld(local_dir, normal),
		.pdf = 1 / (2 * pi) };
}

// faire fonction pour f (weight)

HemisphereSample gfx::SoftwarePathTracer::CosineSampleHemisphere(const Vector3& normal, uint32_t seed)
{
	uint32_t h{ LowBias(seed) };
	const float u1{ HashToFloat(h) };
	h = LowBias(h);
	const float u2{ HashToFloat(h) };

	const float sin_theta{ sqrt((1 - u2 * u2)) };
	const float pi{ static_cast<float>(std::numbers::pi) };
	const float phi{ 2 * pi * u1 };

	Vector3 local_dir{ sin_theta * cos(phi), sin_theta * sin(phi), u2 };

	return HemisphereSample{
		.direction = LocalToWorld(local_dir, normal),
		.pdf = u2 / pi };
}
