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
		return (cd * kd) / static_cast<float>(std::numbers::pi);
	}

	[[maybe_unused]]
	static ColorRgba Lambert(const ColorRgba& kd, const ColorRgba& cd)
	{
		return (cd * kd) / static_cast<float>(std::numbers::pi);
	}
}
#endif
