//==============================================================
//  Graphics Programming 1 (2026-2027)
//  Authors : Matthieu Delaere
//  Copyright (c) 2026 Matthieu Delaere. All rights reserved.
//==============================================================
#ifndef ASSET_LOADER_HEADER
#define ASSET_LOADER_HEADER

//--- Standard Includes ---
#include <cstdint>
#include <string>
#include <vector>

//--- Framework Includes ---
#include <vector3.h>

namespace gfx
{
	//--- Supporting Structs ---
	struct MeshData final
	{
		std::vector<Vector3> positions{};
		std::vector<uint32_t> indices{}; // Triangle list
	};

	//--- Class ----
	// Minimal Wavefront OBJ loader: only positions ("v") and faces ("f") are read.
	// Polygons are triangulated as a fan.
	class AssetLoader final
	{
	public:
		//--- Constructors & Destructor ---
		AssetLoader() = default;
		~AssetLoader() = default;
		AssetLoader(const AssetLoader&) = delete;
		AssetLoader& operator=(const AssetLoader&) = delete;
		AssetLoader(AssetLoader&&) = delete;
		AssetLoader& operator=(AssetLoader&&) = delete;

		//--- Functions ---
		bool Load(const std::string& path, MeshData& mesh) const;
	};
}
#endif //ASSET_LOADER_HEADER
