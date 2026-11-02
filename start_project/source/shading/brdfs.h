//==============================================================
//  Graphics Programming 1 (2026-2027)
//  Authors : Matthieu Delaere
//  Copyright (c) 2026 Matthieu Delaere. All rights reserved.
//==============================================================
#ifndef BRDFS_HEADER
#define BRDFS_HEADER

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
	//--- BRDFs ---
	/**
	* \param kd Diffuse Reflection Coefficient
	* \param cd Diffuse Color
	*/
	[[maybe_unused]]
	static ColorRgba Lambert(const float kd, const ColorRgba& cd)
	{
		return kd * cd / float(std::numbers::pi);
	}

	[[maybe_unused]]
	static ColorRgba Lambert(const ColorRgba& kd, const ColorRgba& cd)
	{
		return kd * cd / float(std::numbers::pi);
	}

	/**
	* \brief BRDF Fresnel Function >> Schlick
	* \param h Normalized Half vector between View and Light directions
	* \param v Normalized View direction
	* \param f0 Base reflectivity of a surface based on IOR (Indices Of Refraction),
				this is different for Dielectrics (Non-Metal) and Conductors (Metal)
	*/
	[[maybe_unused]]
	static ColorRgba FresnelFunction_Schlick(const Vector3& h, const Vector3& v, const ColorRgba& f0)
	{
		return f0 + (ColorRgba::White() - f0) * float(pow(1 - Vector3::Dot(h, v), 5));
	}

	/**
	* \brief BRDF Normal Distribution >> Trowbridge-Reitz GGX (UE4 implemetation - squared(roughness))
	* \param n Surface normal
	* \param h Normalized half vector
	* \param roughness Roughness of the material
	*/
	[[maybe_unused]]
	static float NormalDistribution_GGX(const Vector3& n, const Vector3& h, const float roughness)
	{
		float alpha_square{ float(pow(roughness, 4)) };

		return alpha_square / 
			float(std::numbers::pi * pow(pow(Vector3::Dot(n, h), 2) * (alpha_square - 1) + 1, 2));
	}

	/**
	* \brief BRDF Geometry Function >> Schlick GGX (Direct Lighting + UE4 implementation - squared(roughness))
	* \param n Normal of the surface
	* \param v Normalized direction (view or light)
	* \param roughness Roughness of the material
	* \param is_indirect If true, should use indirect k remapping
	*/
	[[maybe_unused]]
	static float GeometryFunction_SchlickGGX(const Vector3& n, const Vector3& v, const float roughness,
		[[maybe_unused]] const bool is_indirect = false)
	{
		float dot_n_v{ Vector3::Dot(n, v) };
		float k{ is_indirect ?
			(roughness * roughness) / 2 :
			float(pow(roughness + 1, 2) / 8)
		};
		return dot_n_v / (dot_n_v * (1 - k) + k);
	}

	/**
	* \brief BRDF Geometry Function >> Smith (separable form)
	* \param n Normal of the surface
	* \param v Normalized view direction
	* \param l Normalized light direction
	* \param roughness Roughness of the material
	* \param is_indirect (see GeometryFunction_SchlickGGX).
	*/
	[[maybe_unused]]
	static float GeometryFunction_Smith(const Vector3& n, const Vector3& v, const Vector3& l, const float roughness,
		[[maybe_unused]] const bool is_indirect = false)
	{
		return GeometryFunction_SchlickGGX(n, v, roughness, is_indirect) * GeometryFunction_SchlickGGX(n, l, roughness, is_indirect);
	}

	[[maybe_unused]]
	static ColorRgba CookTorrence(
		const Vector3& n,
		const Vector3& v,
		const Vector3& l,
		const float roughness,
		const ColorRgba &f0,
		[[maybe_unused]] const bool is_indirect = false)
	{
		Vector3 h{ (v + l).Normalized() };

		auto D{ NormalDistribution_GGX(n, h, roughness) };
		auto F{ FresnelFunction_Schlick(h, v, f0) };
		auto G{ GeometryFunction_Smith(n, v, l, roughness, is_indirect) };

		float denominator{ (4 * std::max(Vector3::Dot(v, n), 0.f) * std::max(Vector3::Dot(l, n), 0.f)) };

		ColorRgba res{ (D * F * G) / std::max(denominator, 0.0001f) };
		if (res.r >= 0.9f && res.g >= 0.9f && res.b >= 0.9f) {
			//res = ColorRgba::Black();

		}
		return res;
	}
}
#endif