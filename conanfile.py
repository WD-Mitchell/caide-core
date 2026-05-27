from conan import ConanFile
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout


class CaideCoreConan(ConanFile):
    name = "caide-core"
    version = "1.0.0"
    package_type = "library"
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "with_tests": [True, False],
    }
    default_options = {
        "shared": True,
        "fPIC": True,
        "with_tests": True,
    }
    exports_sources = "CMakeLists.txt", "cmake/*", "include/*", "src/*", "tests/*", "README.md", "LICENSE"

    def requirements(self):
        # Deliberately optional. The build falls back to stub mode if OpenCascade is absent.
        if self.conf.get("user.caide:with_opencascade", default=False):
            self.requires("opencascade/7.8.1")

    def layout(self):
        cmake_layout(self)

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        tc.variables["CAIDE_BUILD_TESTS"] = self.options.with_tests
        tc.variables["CAIDE_BUILD_SHARED"] = self.options.shared
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
        if self.options.with_tests:
            cmake.test()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = ["caide_core"]
