from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps, cmake_layout

class GP_Conan(ConanFile):
    name = "GP1_Voxel"
    version = "1.0.0"
    settings = "os", "compiler", "build_type", "arch"

    # Unused for now, but bootstrap.py always passes -o build_tests=..., which
    # fails when the option does not exist.
    options = {
        "build_tests" : [True, False],
    }
    default_options = {
        "build_tests": False
    }

    def requirements(self):
        # --- Core Dependencies ---
        self.requires("spdlog/1.17.0")
        self.requires("sdl/3.4.8")

    def configure(self):
        # --- Configurations ---
        for dep in ["spdlog","sdl"]:
            self.options[dep].shared = False

        # Linux display backend. Enable both, SDL picks at runtime.
        if self.settings.os == "Linux":
            self.options["sdl"].x11 = True
            self.options["sdl"].wayland = True

    def layout(self):
        cmake_layout(self)
        self.folders.build_folder_vars = ["settings.os", "settings.compiler", "settings.build_type"]

    def generate(self):
        tc = CMakeToolchain(self)
        tc.presets_prefix = f"conan-{str(self.settings.os).lower()}"
        tc.cache_variables["CMAKE_CONFIGURATION_TYPES"] = "Debug;Release"
        tc.cache_variables["CMAKE_INTERPROCEDURAL_OPTIMIZATION"] = "OFF"
        tc.generate()

        deps = CMakeDeps(self)
        deps.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
