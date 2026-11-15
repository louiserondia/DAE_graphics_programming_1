//==============================================================
//  Graphics Programming 1 (2026-2027)
//  Authors : Matthieu Delaere
//  Copyright (c) 2026 Matthieu Delaere. All rights reserved.
//==============================================================
#include <asset_loader.h>
using namespace gfx;

//--- Standard Includes ---
#include <fstream>
#include <sstream>

bool AssetLoader::Load(const std::string& path, MeshData& mesh) const
{
	std::ifstream file(path);
	if (!file.is_open())
		return false;

	mesh = {};
	std::string line;
	while (std::getline(file, line))
	{
		std::istringstream stream(line);
		std::string token;
		stream >> token;

		if (token == "v")
		{
			Vector3 position{};
			stream >> position.x >> position.y >> position.z;
			mesh.positions.push_back(position);
		}
		else if (token == "f")
		{
			// Each vertex is "v", "v/vt", "v//vn" or "v/vt/vn"; only v is used.
			// Negative indices are relative to the end of the vertex list.
			std::vector<uint32_t> face{};
			std::string vertex;
			while (stream >> vertex)
			{
				const int32_t index = std::stoi(vertex.substr(0, vertex.find('/')));
				face.push_back(index > 0
					? static_cast<uint32_t>(index - 1)
					: static_cast<uint32_t>(static_cast<int32_t>(mesh.positions.size()) + index));
			}

			for (size_t i = 1; i + 1 < face.size(); ++i)
			{
				mesh.indices.push_back(face[0]);
				mesh.indices.push_back(face[i]);
				mesh.indices.push_back(face[i + 1]);
			}
		}
	}

	return !mesh.indices.empty();
}
