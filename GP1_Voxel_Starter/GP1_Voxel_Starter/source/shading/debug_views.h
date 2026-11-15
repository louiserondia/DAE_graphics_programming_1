//==============================================================
//  Graphics Programming 1 (2026-2027)
//  Authors : Matthieu Delaere
//  Copyright (c) 2026 Matthieu Delaere. All rights reserved.
//==============================================================
#ifndef DEBUG_VIEWS_HEADER
#define DEBUG_VIEWS_HEADER

//--- Standard Includes ---
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

//--- Framework Includes ---
#include <color.h>

namespace gfx
{
	//--- Enums ---
	enum class VisualizationMode : uint8_t
	{
		kNone,
		kNormals,
		kDepth,
		kStepCount,
		kMax
	};

	//--- Debug Parameters ---
	struct DebugParams final
	{
		VisualizationMode visualization_mode{ VisualizationMode::kNone };
		ToneMapOperator tone_map_operator{ ToneMapOperator::kACESHill };
		bool update_active_scene{ true };
		bool print_timer{ false };
	};

	//--- Key Bindings ---
	struct KeyBinding final
	{
		int key;
		const char* description;
		std::function<void()> action;
	};

	// Forward declarations for users
	struct Context;
	class Logger;
	std::vector<KeyBinding> RegisterKeyBindings(Context& context);
	void PrintKeyBindings(const std::vector<KeyBinding>& bindings, const Logger& logger);

	//--- String Conversion Helpers ---
	inline std::string VisualizationModeToString(const VisualizationMode& mode)
	{
		switch (mode)
		{
		case VisualizationMode::kNone:
			return "None";
		case VisualizationMode::kNormals:
			return "Normals";
		case VisualizationMode::kDepth:
			return "Depth";
		case VisualizationMode::kStepCount:
			return "Step Count";
		case VisualizationMode::kMax:
			return "";
		}
		return "";
	}

	inline std::string ToneMapOperatorToString(const ToneMapOperator& op)
	{
		switch (op)
		{
		case ToneMapOperator::kNone:
			return "None";
		case ToneMapOperator::kReinhard:
			return "Reinhard";
		case ToneMapOperator::kACESNarkowicz:
			return "ACES (Narkowicz)";
		case ToneMapOperator::kACESHill:
			return "ACES (Hill)";
		case ToneMapOperator::kMax:
			return "";
		}
		return "";
	}
}
#endif //DEBUG_VIEWS_HEADER
