import os, shutil
from conans import ConanFile
from conan.tools.cmake import CMakeToolchain, CMakeDeps, CMake


# helper function for deploy()
def copy(src, dst):
    if os.path.islink(src):
        if os.path.lexists(dst):
            os.unlink(dst)
        linkto = os.readlink(src)
        os.symlink(linkto, dst)
    else:
        shutil.copy(src, dst)

class Project(ConanFile):
    name = "coco-loop"
    description = "Event Loop for CoCo"
    url = "https://github.com/Jochen0x90h/coco-loop"
    license = "MIT"
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "platform": "ANY"}
    default_options = {
        "platform": None}
    generators = "CMakeDeps"
    exports_sources = "conanfile.py", "CMakeLists.txt", "coco/*", "test/*"
    requires = [
        "coco/0.4.0",
        "coco-devboards/0.3.0"
    ]


    # check if we are cross compiling
    def cross(self):
        if hasattr(self, "settings_build"):
           return self.settings.os != self.settings_build.os
        return False

    def configure(self):
        # pass platform option to dependencies
        self.options["coco"].platform = self.options.platform
        self.options["coco-devboards"].platform = self.options.platform

    keep_imports = True
    def imports(self):
        # copy dependent libraries into the build folder
        self.copy("*", src="@bindirs", dst="bin")
        self.copy("*", src="@libdirs", dst="lib")

    def generate(self):
        # generate "conan_toolchain.cmake"
        toolchain = CMakeToolchain(self)
        toolchain.variables["OS"] = self.settings.os
        toolchain.variables["PLATFORM"] = self.options.platform

        # cross compile to a platform if platform option is set
        # https://github.com/conan-io/conan/blob/develop/conan/tools/cmake/toolchain/blocks.py
        if self.cross():
            toolchain.blocks["generic_system"].values["cmake_system_name"] = "Generic"
            toolchain.blocks["generic_system"].values["cmake_system_processor"] = self.settings.arch
            toolchain.variables["CMAKE_TRY_COMPILE_TARGET_TYPE"] = "STATIC_LIBRARY"
            toolchain.variables["CMAKE_FIND_ROOT_PATH_MODE_PROGRAM"] = "NEVER"
            toolchain.variables["CMAKE_FIND_ROOT_PATH_MODE_LIBRARY"] = "ONLY"
            toolchain.variables["CMAKE_FIND_ROOT_PATH_MODE_INCLUDE"] = "ONLY"
            toolchain.variables["GENERATE_HEX"] = self.env.get("GENERATE_HEX", None)

        # bake CC and CXX from profile into toolchain
        cc = self.env.get("CC", None)
        if cc != None:
            toolchain.variables["CMAKE_C_COMPILER "] = cc
        cxx = self.env.get("CXX", None)
        if cxx != None:
            toolchain.variables["CMAKE_CXX_COMPILER "] = cxx

        toolchain.generate()

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

    def deploy(self):
        # install if CONAN_INSTALL_PREFIX env variable is set
        prefix = os.getenv("CONAN_INSTALL_PREFIX")
        if prefix == None:
            print("set CONAN_INSTALL_PREFIX env variable to install to local directory, e.g.")
            print("export CONAN_INSTALL_PREFIX=$HOME/.local")
        else:
            print(f"Installing {self.name} to {prefix}")

            # create destination directories if necessary
            dstBinPath = os.path.join(prefix, "bin")
            if not os.path.exists(dstBinPath):
                os.mkdir(dstBinPath)
            #print(f"dstBinPath: {dstBinPath}")
            dstLibPath = os.path.join(prefix, "lib")
            if not os.path.exists(dstLibPath):
                os.mkdir(dstLibPath)
            #print(f"dstLibPath: {dstLibPath}")

            # copy executables
            for bindir in self.cpp_info.bindirs:
                srcBinPath = os.path.join(self.cpp_info.rootpath, bindir)
                #print(f"srcBinPath {srcBinPath}")
                if os.path.isdir(srcBinPath):
                    files = os.listdir(srcBinPath)
                    for file in files:
                        print(f"install {file}")
                        src = os.path.join(srcBinPath, file)
                        dst = os.path.join(dstBinPath, file)
                        if os.path.isfile(src):
                            copy(src, dst)

            # copy libraries
            for libdir in self.cpp_info.libdirs:
                srcLibPath = os.path.join(self.cpp_info.rootpath, libdir)
                #print(f"srcLibPath {srcLibPath}")
                if os.path.isdir(srcLibPath):
                    files = os.listdir(srcLibPath)
                    for file in files:
                        print(f"install {file}")
                        src = os.path.join(srcLibPath, file)
                        dst = os.path.join(dstLibPath, file)
                        if os.path.isfile(src):
                            copy(src, dst)
