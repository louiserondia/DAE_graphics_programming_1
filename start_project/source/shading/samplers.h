//==============================================================
//  Graphics Programming 1 (2026-2027)
//  Authors : Matthieu Delaere & Louise Rondia
//  Copyright (c) 2026 Matthieu Delaere. All rights reserved.
//==============================================================
#ifndef SAMPLERS_HEADER
#define SAMPLERS_HEADER

//--- Standard Includes ---
#include <cassert>
#include <numbers>

//--- Framework Includes ---
#include <vector2.h>
#include <vector3.h>
#include <color.h>
#include <materials.h>

namespace gfx
{

	struct HemisphereSample
	{
		Vector3 direction{};
		float pdf{}; // probablity density of that direction
	};

	//static HemisphereSample UniformSampleHemisphere(const Vector3& normal, uint32_t seed);
	//static HemisphereSample CosineSampleHemisphere(const Vector3& normal, uint32_t seed);

	// lowbias32 hash by Chris Wellons
	// nullprogram.com/blog/2018/07/31
	[[maybe_unused]]
	static uint32_t LowBias(uint32_t h)
	{
		h ^= h >> 16; h *= 0x7feb352du;
		h ^= h >> 15; h *= 0x846ca68bu;
		h ^= h >> 16;
		return h;
	}

	[[maybe_unused]]
	static float HashToFloat(const uint32_t h)
	{
		return static_cast<float>(h >> 8) * (1.0f / 16444216.f);
	}

	[[maybe_unused]]
	static float NextFloat(uint32_t& h)
	{
		h = LowBias(h);
		return HashToFloat(h);
	}

	[[maybe_unused]]
	static Vector3 LocalToWorld(const Vector3& local_dir, const Vector3& normal)
	{
		Vector3 world_up{ 1.f, 0.f, 0.f };
		//if (Vector3::Dot(world_up, normal) > 0.999f) 
		if (abs(normal.x) > 0.9f)
			world_up = Vector3{ 0.f, 1.f, 0.f };

		Vector3 tangent{ Vector3::Cross(normal, world_up).Normalized() };
		Vector3 bitangent{ Vector3::Cross(normal, tangent).Normalized() };
		Matrix ONB{ tangent, bitangent, normal, Vector3{} };
		return ONB.TransformVector(local_dir);
	}

	[[maybe_unused]]
	static float PdfUniformHemisphere(const Vector3& normal, const Vector3& direction)
	{
		const float pi{ static_cast<float>(std::numbers::pi) };
		const float dot{ Vector3::Dot(normal, direction) };
		
		if (dot < 0.f) return 0.f;
		return 1 / (2 * pi);
	}

	[[maybe_unused]]
	static float PdfCosineHemisphere(const Vector3& normal, const Vector3& direction)
	{
		const float pi{ static_cast<float>(std::numbers::pi) };
		const float dot{ Vector3::Dot(normal, direction) };

		if (dot < 0.f) return 0.f;
		return dot / pi;
	}

	[[maybe_unused]]
	static HemisphereSample SampleUniformHemisphere(const Vector3& normal, uint32_t& hash_state)
	{
		const float u1{ NextFloat(hash_state) };
		const float u2{ NextFloat(hash_state) };

		const float sin_theta{ sqrt((1 - u2 * u2)) };
		const float pi{ static_cast<float>(std::numbers::pi) };
		const float phi{ 2 * pi * u1 };

		const Vector3 local_dir{ sin_theta * cos(phi), sin_theta * sin(phi), u2 };
		const Vector3 direction{ LocalToWorld(local_dir, normal) };

		return HemisphereSample{
			.direction = direction,
			.pdf = PdfUniformHemisphere(normal, direction) };
	} // question says in the slide to not reduce ourselves, but in the slide ao formula looks more opti by simplifying p

	// faire fonction pour f (weight)

	[[maybe_unused]]
	static HemisphereSample SampleCosineHemisphere(const Vector3& normal, uint32_t& hash_state)
	{
		const float u1{ NextFloat(hash_state) };
		const float u2{ NextFloat(hash_state) };

		const float pi{ static_cast<float>(std::numbers::pi) };
		const float sin_theta{ sqrt((1 - u2 * u2)) };
		const float phi{ 2 * pi * u1 };

		// verify in .pdf if cos_theta (u2) should be sqrt(u2) and if sin_theta is correct
		Vector3 local_dir{ sin_theta * cos(phi), sin_theta * sin(phi), u2 };
		const Vector3 direction{ LocalToWorld(local_dir, normal) };

		return HemisphereSample{
			.direction = direction,
			.pdf = PdfCosineHemisphere(normal, direction) };
	}
}

#endif