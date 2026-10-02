//==============================================================
//  Graphics Programming 1 (2026-2027)
//  Authors : Matthieu Delaere
//  Copyright (c) 2026 Matthieu Delaere. All rights reserved.
//==============================================================
#ifndef INTERSECTIONS_HEADER
#define INTERSECTIONS_HEADER

//--- Standard Includes ---
#include <cassert>

//--- Framework Includes ---
#include <primitives.h>
#include <matrix.h>

namespace gfx
{
	//--- Intersection Tests ---
	[[maybe_unused]]
	static bool HitTestSphere(const Sphere& sphere, const Ray& ray,
		RayHitRecord& hit_record, const bool ignore_hit_record = false)
	{
		(void)ignore_hit_record;

		const Vector3 sphere_to_ray{ ray.origin - sphere.origin };

		// ----- Analytical way -----
		
		const float a{ Vector3::Dot(ray.direction, ray.direction) };
		const float b{ Vector3::Dot(2 * ray.direction, sphere_to_ray) };
		const float c{ Vector3::Dot(sphere_to_ray, sphere_to_ray) - sphere.radius * sphere.radius };
		// c = |vec_ray_sphere|² - sphere.radius² , so is it cheaper to calclate dot of vector with itself than its magnitude squared ?

		const float discri{ b * b - 4 * a * c };

		if (discri <= 0.f) return false; // we dont take the tangent case

		const float t0{ (-b - sqrtf(discri)) / (2 * a) };
		const float t1{ (-b + sqrtf(discri)) / (2 * a) };


		// ----- Geometrical way -----
		
		//const float tca{ Vector3::Dot(dir_to_origin, ray.direction) };
		//const float od{ sqrtf(Vector3::Dot(dir_to_origin, dir_to_origin) - tca * tca) };
		//const float thc{ sqrtf(sphere.radius * sphere.radius - od * od) };

		//const float t0{ tca - thc };
		//const float t1{ tca + thc };

		hit_record.ray = ray;

		if (t0 > ray.min && t0 < ray.max) {
			hit_record.t = t0;
			return true;
		}
		if (t1 > ray.min && t1 < ray.max) // we're "inside" the sphere
		{
			hit_record.t = t1;
			return true;
		}

		// what object index is the sphere, what are barycentric coord and what vertex indices ?

		return false;
	}

	[[maybe_unused]]
	static bool HitTestPlane(const Plane& plane, const Ray& ray,
		RayHitRecord& hit_record, const bool ignore_hit_record = false)
	{
		(void)ignore_hit_record;
		const Vector3 ray_to_plane{ plane.origin - ray.origin };
		float division{ Vector3::Dot(ray.direction , plane.normal) };
		//division = division < FLT_EPSILON ? FLT_EPSILON : division; // doesnt work
		const float t{ Vector3::Dot(ray_to_plane, plane.normal) / division };
	

		if (t >= ray.min && t <= ray.max) {
			hit_record.ray = ray;
			hit_record.t = t;

			if (plane.half_extent.has_value()) // is finite
			{
				// with rotation
				const Vector3 point{ ray.origin + t * ray.direction };
				const Vector3 p{ point - plane.origin };
				const Vector3 b{ Vector3::Cross(plane.normal, plane.tangent) };
				const float t_prime{ Vector3::Dot(p, plane.tangent) };
				const float b_prime{ Vector3::Dot(p, b) };
				if (std::abs(t_prime) <= plane.half_extent.value().x
					&& std::abs(b_prime) <= plane.half_extent.value().y)
					return true;
				return false;
			}
			return true;
		}
		return false;
	}

	[[maybe_unused]]
	static bool HitTestTriangle(const Triangle& triangle, const Ray& ray,
		RayHitRecord& hit_record, const bool ignore_hit_record = false)
	{
		//TODO
		assert(false && "Not Implemented");
		(void)triangle; (void)ray; (void)hit_record; (void)ignore_hit_record;
		return false;
	}

	[[maybe_unused]]
	static bool HitTestAABB(const AABB& aabb, const Ray& ray)
	{
		//TODO
		assert(false && "Not Implemented");
		(void)aabb; (void)ray;
		return false;
	}
}
#endif //INTERSECTIONS_HEADER