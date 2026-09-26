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
		// a = dot(ray.direction, ray.direction) = |ray.direction|²
		// c = dot(vec_ray_sphere, vec_ray_sphere) - sphere.radius² = |vec_ray_sphere|² - sphere.radius²

		const Vector3 l{ sphere.origin - ray.origin };

		//const float a{ ray.direction.Magnitude() * ray.direction.Magnitude() };
		//const float b{ Vector3::Dot(2 * ray.direction, vec_ray_sphere) };
		//const float c{ vec_ray_sphere.Magnitude() * vec_ray_sphere.Magnitude() - sphere.radius * sphere.radius };
		//const float discri{ b * b - 4 * a * c };

		//if (discri <= 0.f) return false; // we dont take the tangent case

		//const float t0{ (-b - discri) / (2 * a) };
		//const float t1{ (-b + discri) / (2 * a) };

		const float tca{ Vector3::Dot(l, ray.direction) };
		const float od{ sqrtf(Vector3::Dot(l, l) - tca * tca) };
		const float thc{ sqrtf(sphere.radius * sphere.radius - od * od) };

		const float t0{ tca - thc };
		const float t1{ tca + thc };

		hit_record.ray = ray;

		if (t0 > ray.min && t0 < ray.max) {
			hit_record.t = t0;
			hit_record.ray.origin = ray.origin + t0 * ray.direction;
			return true;
		}
		if (t1 > ray.min && t1 < ray.max) // we're "inside" the sphere
		{
			hit_record.t = t1;
			hit_record.ray.origin = ray.origin + t1 * ray.direction;
			return true;
		}

		// what object index is the sphere, what are barycentric coord and what vertex indices ?

		return false;
	}

	[[maybe_unused]]
	static bool HitTestPlane(const Plane& plane, const Ray& ray,
		RayHitRecord& hit_record, const bool ignore_hit_record = false)
	{
		//TODO
		assert(false && "Not Implemented");
		(void)plane; (void)ray; (void)hit_record; (void)ignore_hit_record;
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