//==============================================================
//  Graphics Programming 1 (2026-2027)
//  Authors : Matthieu Delaere
//  Copyright (c) 2026 Matthieu Delaere. All rights reserved.
//==============================================================
//--- Standard Includes ---
#include <iostream>
#include <iomanip>
#include <chrono>
#include <sstream>
#include <filesystem>
#include <format>

//--- Framework Includes ---
#include <context.h>
#include <renderer.h>
#include <voxel_ray_tracer.h>
#include <scenes.h>
using namespace gfx;

//--- Frame Limiter ---
class FrameLimiter final
{
	double target_ms_{ 0.0 };
	std::chrono::high_resolution_clock::time_point frame_start_ = {};

public:
	FrameLimiter(const double target_fps)
		: target_ms_(1000.0 / target_fps), frame_start_(std::chrono::high_resolution_clock::now())
	{}
	~FrameLimiter()
	{
		const auto frame_end{ std::chrono::high_resolution_clock::now() };
		const double frame_duration_ms{ std::chrono::duration<double, std::milli>(frame_end - frame_start_).count() };

		if (frame_duration_ms < target_ms_)
			SDL_Delay(static_cast<uint32_t>(target_ms_ - frame_duration_ms));
	}
	FrameLimiter(const FrameLimiter&) = delete;
	FrameLimiter& operator=(const FrameLimiter&) = delete;
	FrameLimiter(FrameLimiter&&) = delete;
	FrameLimiter& operator=(FrameLimiter&&) = delete;
};

//--- Helpers ---
namespace
{
	void SaveScreenshot(SDL_Surface* surface, const Logger& logger)
	{
		if (!surface)
		{
			logger.LogError("[Screenshot] No surface available.");
			return;
		}

		const auto now{ std::chrono::system_clock::now() };
		const auto time{ std::chrono::system_clock::to_time_t(now) };
		std::tm tm_buf{};
	#ifdef _WIN32
		localtime_s(&tm_buf, &time);
	#else
		localtime_r(&time, &tm_buf);
	#endif

		std::ostringstream filename;
		filename << "screenshots/screenshot_"
			<< std::put_time(&tm_buf, "%Y%m%d_%H%M%S") << ".bmp";

		std::filesystem::create_directories("screenshots");

		if (SDL_SaveBMP(surface, filename.str().c_str()))
			logger.LogInfo("[Screenshot] Saved: {}", filename.str());
		else
			logger.LogError("[Screenshot] Failed: {}", SDL_GetError());
	}

	void SetSurfaceSize(SDL_Window* window, SurfaceInfo& info)
	{
		int current_width{ 0 }, current_height{ 0 };
		SDL_GetWindowSize(window, &current_width, &current_height);
		info.width = static_cast<uint32_t>(current_width);
		info.height = static_cast<uint32_t>(current_height);
	}
}

//--- Main ---
int main(int, char**)
{
	// Specify settings.
	constexpr uint32_t width{ 640 };
	constexpr uint32_t height{ 480 };
	const char* window_title{ "Example Exercise: Voxel Ray Tracer" };

	// Create context and construct objects.
	Context context{};
	context.logger = std::make_unique<Logger>(LoggerType::kConsole | LoggerType::kFile);
	context.leak_detector = std::make_unique<LeakDetector>(
		context.logger->IsLoggerTypeEnabled(LoggerType::kFile) ? context.logger->GetReportFilename() : "");
	context.timer = std::make_unique<Timer>();
	context.scene_manager = std::make_unique<SceneManager>();

	// Specify ID of leaks here (if any).
	//LeakDetector::BreakOnAllocationId(id);

	// Create SDL window and acquire surface.
	#ifdef GFX_PLATFORM_LINUX // VM issues with broken Wayland support, force X11 for now.
		SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "x11");
	#endif
	if (!SDL_Init(SDL_INIT_VIDEO))
	{
		context.logger->LogCritical("SDL_Init failed: {}", SDL_GetError());
		return 1;
	}
	std::unique_ptr<SDL_Window, void(*)(SDL_Window*)> window = { nullptr, SDL_DestroyWindow };
	window.reset(SDL_CreateWindow(window_title, width, height, 0));
	if (!window)
	{
		context.logger->LogCritical("SDL_CreateWindow failed: {}", SDL_GetError());
		SDL_Quit();
		return 1;
	}

	// Set options - disable resizing for now.
	SDL_SetWindowResizable(window.get(), false);

	// Get the surface information (CPU pixel buffer).
	SDL_Surface* surface = SDL_GetWindowSurface(window.get());
	context.surface_info.pixel_format_details = SDL_GetPixelFormatDetails(surface->format);
	context.surface_info.pixel_buffer = static_cast<uint32_t*>(surface->pixels);

	// Create renderer.
	std::unique_ptr<Renderer> renderer = std::make_unique<VoxelRayTracer>(&context);

	// Create scenes.
	context.scene_manager->CreateScene<TerrainScene>();
	context.scene_manager->CreateScene<BunnyScene>();

	// Register key bindings and add screenshot key binding.
	std::vector<KeyBinding> key_bindings = RegisterKeyBindings(context);
	key_bindings.push_back({ SDL_SCANCODE_F12, "F12: Save Screenshot",
		[&surface, &context]()
	{
		SaveScreenshot(surface, *context.logger);
	}});

	// Print info.
	PrintKeyBindings(key_bindings, *context.logger);

	// Tick once before the loop to reset the baseline, avoiding a large initial delta time spike.
	context.timer->Tick();
	bool is_looping{ true };
	while (is_looping)
	{
		// Limit the entire frame.
		FrameLimiter limiter(120.0);

		// Tick the timer at the start of the frame so all systems use the same delta time.
		context.timer->Tick();

		// Reset per-frame flags before input and update.
		context.scene_manager->BeginFrame();

		// Get active scene, if any.
		Scene* active_scene = context.scene_manager->GetActiveScene();

		//--------- Get input events ---------
		SDL_Event e;
		while (SDL_PollEvent(&e))
		{
			switch (e.type)
			{
			case SDL_EVENT_QUIT:
				is_looping = false;
				break;
			case SDL_EVENT_KEY_UP:
				for (const auto& binding : key_bindings)
				{
					if (e.key.scancode == binding.key)
					{
						binding.action();
						break;
					}
				}
				break;
			default:
				break;
			}
		}

		//----------- UE5-style camera controls -----------
		if (active_scene)
		{
			const float delta_time{ static_cast<float>(context.timer->GetDeltaTime()) / 1000.f };
			constexpr float mouse_sensitivity{ 0.003f };
			constexpr float movement_speed{ 3.0f };
			constexpr float rotation_speed{ 1.0f };
			constexpr float speed_boost_multiplier{ 3.0f };

			// Get input states.
			const bool* keyboard_state = SDL_GetKeyboardState(nullptr);
			float mouse_x{ 0.f }, mouse_y{ 0.f };
			const SDL_MouseButtonFlags mouse_state = SDL_GetRelativeMouseState(&mouse_x, &mouse_y);

			// Calculate current speeds (with speed boost if shift is held).
			const bool speed_boost{ keyboard_state[SDL_SCANCODE_LSHIFT] || keyboard_state[SDL_SCANCODE_RSHIFT] };
			const float current_movement_speed{ movement_speed * (speed_boost ? speed_boost_multiplier : 1.0f) * delta_time };
			constexpr float current_rotation_speed{ rotation_speed * mouse_sensitivity };

			// Right Mouse Button: Look around (pitch/yaw).
			if (mouse_state & SDL_BUTTON_RMASK)
			{
				active_scene->camera.Pitch(-mouse_y * current_rotation_speed);
				active_scene->camera.Yaw(-mouse_x * current_rotation_speed);

				// Right Mouse + WASD: Fly around with look.
				if (keyboard_state[SDL_SCANCODE_W])
					active_scene->camera.MoveForward(current_movement_speed);
				if (keyboard_state[SDL_SCANCODE_S])
					active_scene->camera.MoveForward(-current_movement_speed);
				if (keyboard_state[SDL_SCANCODE_A])
					active_scene->camera.MoveRight(-current_movement_speed);
				if (keyboard_state[SDL_SCANCODE_D])
					active_scene->camera.MoveRight(current_movement_speed);
				if (keyboard_state[SDL_SCANCODE_Q])
					active_scene->camera.MoveUp(-current_movement_speed);
				if (keyboard_state[SDL_SCANCODE_E])
					active_scene->camera.MoveUp(current_movement_speed);
			}
			// Middle Mouse Button: Pan (move up/down/left/right).
			else if (mouse_state & SDL_BUTTON_MMASK)
			{
				active_scene->camera.MoveRight(-mouse_x * current_movement_speed * 0.1f);
				active_scene->camera.MoveUp(mouse_y * current_movement_speed * 0.1f);
			}
			// Left Mouse: Move forward/backward and turn
			else if (mouse_state & SDL_BUTTON_LMASK)
			{
				active_scene->camera.MoveForward(-mouse_y * current_movement_speed * 0.1f);
				active_scene->camera.Yaw(mouse_x * current_rotation_speed);
			}
		}

		//--------- Update ---------
		if (context.debug_params.print_timer)
			context.timer->PrintInformation(2.f, context.logger.get());
		context.scene_manager->UpdateActiveScene(
			context.timer->GetDeltaTime() / 1000.0,
			context.debug_params.update_active_scene);

		//--------- Render ---------
		// Update surface dimensions each frame (handles window resize if enabled later).
		SetSurfaceSize(window.get(), context.surface_info);
		renderer->Render();

		// Update window title with stats.
		{
			const double dt_ms{ context.timer->GetDeltaTime() };
			const double fps{ (dt_ms > 0.0) ? 1000.0 / dt_ms : 0.0 };
			std::string title = std::format("{} | FPS: {:.0f} | {:.1f}ms", window_title, fps, dt_ms);
			if (active_scene)
				title += std::format(" | Grid: {}x{}x{} | Voxels: {}", active_scene->voxel_grid.size_x,
					active_scene->voxel_grid.size_y, active_scene->voxel_grid.size_z, active_scene->GetVoxelCount());
			SDL_SetWindowTitle(window.get(), title.c_str());
		}
		SDL_UpdateWindowSurface(window.get());
	}

	renderer.reset();
	window.reset();
	SDL_Quit();
	return 0;
}
