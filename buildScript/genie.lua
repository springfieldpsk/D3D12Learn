MODULE_DIR = path.getabsolute("../")
SRC_DIR = path.join(MODULE_DIR, 'src')
BUILD_DIR = path.join(MODULE_DIR, 'build')

-- create a new option for sloution
-- newoption {
-- 	trigger = "with-webgpu",
-- 	description = "Enable webgpu experimental renderer.",
-- }

function defaultSettingForProject()
    includedirs{
        path.join(MODULE_DIR, "ThirdParty"),
    }

    configuration "Debug"
        defines     { "_DEBUG", "_WINDOWS" }
        flags       { "Symbols" }
        targetdir   (path.join(BUILD_DIR,'bin/debug'))  

    configuration "Release"
        defines     { "NDEBUG", "_WINDOWS" }
        flags       { "OptimizeSize" }
        targetdir   (path.join(BUILD_DIR,'bin/Release'))   

end

solution "d3d12Learn"
    configurations {
        "Release",
        "Debug"
    }
    platforms {
        "x32",
        "x64"
    }
    flags { "Cpp17", "Unicode", "WinMain" }
    location (BUILD_DIR)
    language "C++"
    startproject "example-00-helloworld"


project "examples01"
    uuid (os.uuid("examples01"))
    kind "WindowedApp"
    files{
        path.join(SRC_DIR, "01CreateTriangle.cpp")
    }

    defaultSettingForProject()

project "examples02"
    kind "WindowedApp"
    uuid (os.uuid("examples02"))

    files{
        path.join(SRC_DIR, "02LoadImage.cpp")
    }

    defaultSettingForProject()