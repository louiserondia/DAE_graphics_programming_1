//==============================================================
//  Graphics Programming 1 (2026-2027)
//  Authors : Matthieu Delaere
//  Copyright (c) 2026 Matthieu Delaere. All rights reserved.
//==============================================================
#ifndef SCENE_MANAGER_HEADER
#define SCENE_MANAGER_HEADER

//--- Standard Includes ---
#include <algorithm>
#include <memory>
#include <vector>

//--- Framework Includes ---
#include <camera.h>
#include <primitives.h>
#include <materials.h>

namespace gfx
{
	struct Scene
	{
		VoxelGrid voxel_grid { };
		Light light { };
		Camera camera { };
		bool scene_changed { true };

		Scene() = default;
		virtual ~Scene() = default;
		Scene(const Scene&) = delete;
		Scene& operator=(const Scene&) = delete;
		Scene(Scene&&) = delete;
		Scene& operator=(Scene&&) = delete;
		virtual void Setup() = 0;
		virtual bool Update(const double delta_time)
		{
			(void)delta_time; return false;
		}

		[[nodiscard]] uint64_t GetVoxelCount() const
		{
			return static_cast<uint64_t>(std::count_if(voxel_grid.voxels.begin(), voxel_grid.voxels.end(),
				[](const uint8_t v) { return v != 0; }));
		}
	};

	class SceneManager final
	{
		std::vector<std::unique_ptr<Scene>> scenes_{};
		uint32_t active_scene_index_{ std::numeric_limits<uint32_t>::max() };

	public:
		template<typename SceneType>
		bool CreateScene(const bool activate_scene = true)
		{
			static_assert(std::is_base_of_v<Scene, SceneType>, "Scene specialization given must derive from Scene");

			scenes_.emplace_back(std::make_unique<SceneType>());
			SceneType* scene = static_cast<SceneType*>(scenes_.back().get());
			scene->Setup();
			if (activate_scene)
				active_scene_index_ = static_cast<uint32_t>(scenes_.size() - 1);
			return true;
		}

		bool ActivateScene(const uint32_t index)
		{
			if (index >= static_cast<uint32_t>(scenes_.size()))
				return false;
			active_scene_index_ = index;
			scenes_[active_scene_index_]->scene_changed = true;
			return true;
		}

		void BeginFrame()
		{
			if (active_scene_index_ >= static_cast<uint32_t>(scenes_.size()))
				return;

			// Reset the dirty flag from the previous frame.
			// Input and Update during this frame will set it again if needed.
			scenes_[active_scene_index_]->scene_changed = false;
		}

		void UpdateActiveScene(const double delta_time, const bool update_scene = true)
		{
			if (active_scene_index_ >= static_cast<uint32_t>(scenes_.size()))
				return;

			Scene* scene = scenes_[active_scene_index_].get();

			// Propagate component-level changes into scene_changed.
			if (scene->camera.IsDirty())
			{
				scene->scene_changed = true;
				scene->camera.ClearDirty();
			}

			// Update the active scene. If it returns true, scene data changed.
			if (update_scene && scene->Update(delta_time))
				scene->scene_changed = true;

			// Afterward pre-cache the inverse view matrix (used by every pixel this frame).
			const Matrix& inverse = scene->camera.GetView().GetInverse();
			(void)inverse;
		}

		[[nodiscard]] Scene* GetActiveScene() const
		{
			if (active_scene_index_ >= static_cast<uint32_t>(scenes_.size()))
				return nullptr;
			return scenes_[active_scene_index_].get();
		}

		void NextScene()
		{
			if (!scenes_.empty())
			{
				active_scene_index_ = (active_scene_index_ + 1) % static_cast<uint32_t>(scenes_.size());
				scenes_[active_scene_index_]->scene_changed = true;
			}
		}

		void PreviousScene()
		{
			if (!scenes_.empty())
			{
				active_scene_index_ = (active_scene_index_ + static_cast<uint32_t>(scenes_.size()) - 1)
					% static_cast<uint32_t>(scenes_.size());
				scenes_[active_scene_index_]->scene_changed = true;
			}
		}
	};
}
#endif //SCENE_MANAGER_HEADER
