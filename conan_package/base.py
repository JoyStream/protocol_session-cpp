from conans import ConanFile, CMake, tools
import os

class ProtocolSessionBase(ConanFile):
    name = "ProtocolSession"
    version = "0.1.2"
    license = "(c) JoyStream Inc. 2016-2017"
    url = "https://github.com/JoyStream/protocol_session-conan.git"
    git_repo = "git@github.com:JoyStream/protocol_session-cpp.git"
    settings = "os", "compiler", "build_type", "arch"
    generators = "cmake"
    requires = "ProtocolStateMachine/0.1.1@joystream/stable"
    build_policy = "missing"

    def source(self):
        raise Exception("abstract base package was exported")

    def build(self):
        cmake = CMake(self.settings)
        self.run('cmake repo/sources %s' % (cmake.command_line))
        self.run("cmake --build . %s" % cmake.build_config)

    def package(self):
        self.copy("*.hpp", dst="include", src="repo/sources/include/")
        self.copy("*.cpp", dst="include", src="repo/sources/include/") #template defenitions
        self.copy("*.a", dst="lib", keep_path=False)
        self.copy("*.lib", dst="lib", keep_path=False)

    def package_info(self):
        self.cpp_info.libs = ["protocol_session"]
