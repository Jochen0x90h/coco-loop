from conans import ConanFile
from conan.tools.cmake import CMake


class Project(ConanFile):
    name = "coco-loop"
    description = "Event Loop for CoCo"
    license = "MIT"
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "platform": [None, "ANY"]}
    default_options = {
        "platform": None}
    generators = "CMakeDeps", "CMakeToolchain"
    exports_sources = "conanfile.py", "CMakeLists.txt", "coco/*", "test/*"
    requires = [
        "coco/0.5.0"
    ]
    tool_requires = "coco-toolchain/0.1.0"


    # check if we are cross compiling
    def cross(self):
        if hasattr(self, "settings_build"):
            return self.settings.os != self.settings_build.os
        return False

    def requirements(self):
        if self.options.platform == "emu":
            self.requires("glfw/3.3.8")

    def build_requirements(self):
        self.test_requires("coco-devboards/0.4.0")

    def configure(self):
        # pass platform option to dependencies
        self.options["coco"].platform = self.options.platform
        self.options["coco-toolchain"].platform = self.options.platform
        self.options["coco-devboards"].platform = self.options.platform

    keep_imports = True
    def imports(self):
        # copy dependent libraries into the build folder
        self.copy("*", src="@bindirs", dst="bin")
        self.copy("*", src="@libdirs", dst="lib")

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

        # run unit tests if CONAN_RUN_TESTS environment variable is set to 1
        #if os.getenv("CONAN_RUN_TESTS") == "1" and not self.cross():
        #    cmake.test()

    def package(self):
        # install from build directory into package directory
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = [self.name]
        if self.settings.os == "Windows":
            self.cpp_info.system_libs = ["winmm"]
