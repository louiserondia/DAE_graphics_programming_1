//==============================================================
//  Graphics Programming 1 (2026-2027)
//  Authors : Matthieu Delaere
//  Copyright (c) 2026 Matthieu Delaere. All rights reserved.
//==============================================================
#ifndef MATERIALS_HEADER
#define MATERIALS_HEADER

//--- Framework Includes ---
#include <vector3.h>
#include <color.h>
#include <brdfs.h>

namespace gfx
{
	//--- Shading Input ---
	struct ShadingInput final
	{
		Vector3 view_direction{};
		Vector3 light_direction{};
		Vector3 world_normal{};
		Vector3 world_position{};
		Vector3 shadow_origin{};
		ColorRgba albedo{};
	};
}
#endif //MATERIALS_HEADER
