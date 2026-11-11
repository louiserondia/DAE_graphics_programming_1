#ifndef UTILS_H
#define UTILS_H

//#include <vector3.h>
//#include <matrix.h>

int GetRow(int index, int col) { return index / col; }
int GetCol(int index, int col) { return index % col; }
int GetIndex(int row, int col, int ncols) { return row * ncols + col; }

// lowbias32 hash by Chris Wellons
// nullprogram.com/blog/2018/07/31
uint32_t LowBias(uint32_t h)
{
	h ^= h >> 16; h *= 0x7feb352du;
	h ^= h >> 15; h *= 0x846ca68bu;
	h ^= h >> 16;
	return h;
}

float HashToFloat(const uint32_t h)
{
	return static_cast<float>(h >> 8) * (1.0f / 16444216.f);
}
namespace gfx
{
	Vector3 LocalToWorld(const Vector3& local_dir, const Vector3& normal)
	{
		Vector3 world_up{ 1.f, 0.f, 0.f};
		//if (Vector3::Dot(world_up, normal) > 0.999f) 
		if (abs(normal.x) > 0.9f)
			world_up = Vector3{  0.f, 1.f, 0.f };

		Vector3 tangent{ Vector3::Cross(normal, world_up).Normalized() };
		Vector3 bitangent{ Vector3::Cross(normal, tangent).Normalized() };
		Matrix ONB{ tangent, bitangent, normal, Vector3{} };
		return ONB.TransformVector(local_dir);
	}
}

#endif