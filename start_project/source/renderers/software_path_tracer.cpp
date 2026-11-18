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
#include <samplers.h>

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
	context_->debug_params.max_samples = 32;

	Scene* scene{ context_->scene_manager->GetActiveScene() };
	const SurfaceInfo& surface_info = context_->surface_info;
	const int width{ int(surface_info.width) };
	const int height{ int(surface_info.height) };
	int total_pixels{ width * height };

	const Matrix& viewMatrix{ scene->camera.GetView() };
	const Matrix& inverseViewMatrix{ viewMatrix.GetInverse() };
	VisualizationMode vis_mode{ context_->debug_params.visualization_mode };
	SamplingStrategy sampling_strategy{ context_->debug_params.sampling_strategy };
	ToneMapOperator tone_map_operator{ context_->debug_params.tone_map_operator };

	if (scene->scene_changed || accumulation_buffer_.size() != total_pixels ||
		vis_mode != last_vis_mode_ || sampling_strategy != last_sampling_strategy_
		|| tone_map_operator != last_tone_map_operator)
	{
		context_->debug_params.accumulated_samples = 0;
		accumulation_buffer_.assign(total_pixels, ColorRgba{});
		last_vis_mode_ = vis_mode;
		last_sampling_strategy_ = sampling_strategy;
	}
	bool is_debug_view{ vis_mode != VisualizationMode::kNone && vis_mode != VisualizationMode::kDirectOnly };

	if (!is_debug_view && context_->debug_params.accumulated_samples >= context_->debug_params.max_samples)
		return;

#pragma omp parallel for schedule(dynamic, 16)
	for (int pixel_index{}; pixel_index < total_pixels; ++pixel_index)
	{
		int px{ GetCol(pixel_index, width) };
		int py{ GetRow(pixel_index, width) };
		uint32_t hash_state{ LowBias(pixel_index ^ (context_->debug_params.accumulated_samples * 2654435761u)) };
		//SeedRng(hash_state);
		Ray view_ray{ scene->camera.GetPosition() };
		RayHitRecord closest_hit_record{};
		ShadingInput shading_input{};

		// takes background color on miss
		ColorRgba final_color{ scene->background_color };

		view_ray.direction = inverseViewMatrix * GetRayDirection(pixel_index, px, py, float(width), float(height), hash_state);
		view_ray.direction.Normalize();


		if (!is_debug_view)
		{
			final_color = TracePath(view_ray, scene, hash_state);

			const float inv_frames{ 1.f / context_->debug_params.accumulated_samples };
			accumulation_buffer_[pixel_index] += final_color;
			final_color = accumulation_buffer_[pixel_index] * inv_frames;
			final_color = final_color.Tonemap(tone_map_operator);
			final_color = final_color.ToSrgb();
		}
		else if (SceneClosestHitTest(view_ray, closest_hit_record, false))
		{
			shading_input = ConstructShadingInput(closest_hit_record);
			final_color = ConstructFinalColor(vis_mode, closest_hit_record, shading_input, pixel_index);
		}

		final_color.Clamp();

		// Write to surface
		surface_info.pixel_buffer[px + (py * width)] = SDL_MapRGB(
			surface_info.pixel_format_details, nullptr,
			uint8_t(final_color.r * 255),
			uint8_t(final_color.g * 255),
			uint8_t(final_color.b * 255));
	}
	if (!is_debug_view)
		++context_->debug_params.accumulated_samples;
}

bool SoftwarePathTracer::SceneClosestHitTest(const Ray& ray, RayHitRecord& closest_hit, bool ignore_record) const
{
	Scene* scene{ context_->scene_manager->GetActiveScene() };
	const Factory<Primitive>& factory{ scene->primitives_factory };

	// t_max gets shorter for every hit so we avoid further intersections
	float t_max{ std::numeric_limits<float>::max() };

	bool any_hit{};
	for (size_t i{}; i < scene->objects.size(); i++)
	{
		bool did_hit{};
		SceneObject& object{ scene->objects[i] };
		PrimitiveType primitive_type = factory.Get(object.primitive_index)->type;

		Ray final_ray{ ray };
		if (object.instance_transformation.has_value())
			final_ray = object.instance_transformation->TransformRay(ray);
		final_ray.max = std::min(t_max, final_ray.max);

		switch (primitive_type)
		{
		case PrimitiveType::kSphere:
		{
			Sphere* sphere{ factory.GetAs<Sphere>(object.primitive_index) };

			did_hit = HitTestSphere(*sphere, final_ray, closest_hit, ignore_record);
			break;
		}
		case PrimitiveType::kPlane:
		{
			Plane* plane{ factory.GetAs<Plane>(object.primitive_index) };

			did_hit = HitTestPlane(*plane, final_ray, closest_hit, ignore_record);
			break;
		}
		case PrimitiveType::kTriangle:
		{
			Triangle* triangle{ factory.GetAs<Triangle>(object.primitive_index) };

			did_hit = HitTestTriangle(*triangle, final_ray, closest_hit, ignore_record);
			break;
		}
		case PrimitiveType::kTriangleMesh:
		{
			TriangleMesh* mesh{ factory.GetAs<TriangleMesh>(object.primitive_index) };

			if (mesh->bvh_nodes.empty()) break;

			Triangle triangle{};
			triangle.cull_mode = mesh->cull_mode;

			uint32_t bvh_stack[64];
			uint32_t stack_top{};
			bvh_stack[stack_top] = 0; // root
			++stack_top;

			while (stack_top > 0)
			{
				--stack_top;
				uint32_t node_index{ bvh_stack[stack_top] };
				const BVHNode& node{ mesh->bvh_nodes[node_index] };

				if (!HitTestAABB(node.bounds, final_ray)) continue;

				if (node.IsLeaf())
				{
					const size_t end{ node.first_tri + node.tri_count };
					for (size_t index_triangle{ node.first_tri }; index_triangle < end; ++index_triangle)
					{
						uint32_t mesh_index{ mesh->bvh_tri_indices[index_triangle] * 3 };
						uint32_t index0{ mesh->indices[mesh_index] };
						uint32_t index1{ mesh->indices[mesh_index + 1] };
						uint32_t index2{ mesh->indices[mesh_index + 2] };

						triangle.v0 = mesh->vertices[index0].position;
						triangle.v1 = mesh->vertices[index1].position;
						triangle.v2 = mesh->vertices[index2].position;

						if (!HitTestTriangle(triangle, final_ray, closest_hit, ignore_record)) continue;

						if (ignore_record) return true;

						did_hit = true;
						closest_hit.vertex_indices = { index0, index1, index2 };
						final_ray.max = closest_hit.t;
					}
				}
				else
				{
					bvh_stack[stack_top++] = node.left_child;
					bvh_stack[stack_top++] = node.left_child + 1;
				}
			}
			break;
		}
		case PrimitiveType::kNone:
		default:
			break;
		}

		if (!did_hit) continue;
		if (ignore_record) return true;

		if (primitive_type != PrimitiveType::kTriangleMesh)
			closest_hit.vertex_indices.reset();

		any_hit = true;
		closest_hit.object_index = uint32_t(i);
		t_max = closest_hit.t;
	}
	return any_hit;
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
		// maybe dyn cast bc faster and were certain?
		TriangleMesh* mesh{ factory.GetAs<TriangleMesh>(object.primitive_index) };

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

ColorRgba SoftwarePathTracer::ConstructFinalColor(const VisualizationMode& vismod, const RayHitRecord& hit, ShadingInput& si, [[maybe_unused]] uint32_t pixel_index)
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

		color = DirectIllumination(si, surface_prop, material, scene)/* * TraceAO(si, pixel_index)*/;
		//color = TraceAO(si, pixel_index);
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
		break;
	}
	case VisualizationMode::kMax:
	default:
		break;
	}

	return color;
}

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
		//result = ColorRgba::Red();
	}
	return result;
}


// Indirect Lighting

ColorRgba SoftwarePathTracer::TracePath(const Ray& ray, const Scene* scene, uint32_t& hash_state, int max_bounces) const
{
	ColorRgba accumulated_color{};
	ColorRgba throughput{ ColorRgba::White() };
	Ray current_ray{ ray };
	int bounce{};

	for (; bounce < max_bounces; ++bounce)
	{
		RayHitRecord hit{};
		if (!SceneClosestHitTest(current_ray, hit))
		{
			// ray left the scene
			accumulated_color += scene->background_color * throughput;
			break;
		}

		ShadingInput si{ ConstructShadingInput(hit) };
		const SceneObject& scene_object{ scene->objects[hit.object_index] };
		const Material* material{ scene->materials_factory.Get(scene_object.material_index) };
		const SurfaceProperties sp{ material->GetSurfaceProperties(si, scene->textures_factory) };

		accumulated_color += throughput * DirectIllumination(si, sp, material, scene);

		bool use_uniform{ context_->debug_params.sampling_strategy == SamplingStrategy::kUniform };
		HemisphereSample sample{ use_uniform ?
			SampleUniformHemisphere(si.world_normal, hash_state)
			: SampleCosineHemisphere(si.world_normal, hash_state) };

		float cos_theta{ Vector3::Dot(sample.direction, si.world_normal) };
		// below surface or pdf impossible
		if (cos_theta < 0.f || sample.pdf <= 0.f)
			break;

		si.light_direction = sample.direction;
		const ColorRgba brdf{ material->Shade(si, sp) };

		BounceSample bounce_sample{};
		bounce_sample.direction = sample.direction;
		bounce_sample.color_weight = brdf * cos_theta / sample.pdf;

		throughput *= bounce_sample.color_weight;

		current_ray = Ray{
			.origin = si.shadow_origin,
			.direction = sample.direction,
			.min = 0.001f,
			.max = std::numeric_limits<float>::max()
		};

		if (context_->debug_params.visualization_mode != VisualizationMode::kNone) break;
	}
	return accumulated_color;
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

Vector4 SoftwarePathTracer::GetRayDirection(uint32_t pixel_index, int px, int py, float w, float h, uint32_t& hash_state)
{
	Scene* scene{ context_->scene_manager->GetActiveScene() };
	const float aspect_ratio{ w / h };
	const float fov{ tanf((scene->camera.GetFovAngle() * float(std::numbers::pi) / 180.f) / 2) };

	const Vector2 jitter{ GenerateSubpixelJitter(pixel_index, hash_state) };

	const float x{ (2.f * ((float(px) + jitter.x) / w) - 1.f) * aspect_ratio * fov };
	const float y{ (1.f - 2.f * ((float(py) + jitter.y) / h)) * fov };

	return Vector4{ x, y, 1.f, 0.f };
}

float gfx::SoftwarePathTracer::TraceAO(const ShadingInput& si, uint32_t pixel_index) const
{
	float sum{};
	for (uint32_t index{}; index < ao_sample_count; ++index)
	{
		// multipliers need to be odd and coprime together
		RayHitRecord hit{};
		uint32_t ao_seed{ pixel_index * 1973u + index * 6133u + context_->debug_params.accumulated_samples * 2654435761u };

		bool use_uniform{ context_->debug_params.sampling_strategy == SamplingStrategy::kUniform };

		HemisphereSample sample{ use_uniform ?
			SampleUniformHemisphere(si.world_normal, ao_seed) :
			SampleCosineHemisphere(si.world_normal, ao_seed) };

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

Vector2 gfx::SoftwarePathTracer::GenerateSubpixelJitter(uint32_t pixel_index, uint32_t& hash_state) const
{
	(void)pixel_index;
	float jx{ NextFloat(hash_state) };
	float jy{ NextFloat(hash_state) };
	//const uint32_t frame{ context_->debug_params.accumulated_samples };
	//const uint32_t frame_hash{ LowBias(frame) };
	//uint32_t h{ pixel_index ^ frame_hash };
	//float jx{ HashToFloat(h) };
	//h = LowBias(h);

	return Vector2(jx, jy);
	//return Vector2(jx, HashToFloat(h));
}
