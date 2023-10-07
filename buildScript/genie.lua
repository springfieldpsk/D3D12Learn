MODULE_DIR = path.getabsolute("../")
SRC_DIR = path.join(MODULE_DIR, 'src')
BUILD_DIR = path.join(MODULE_DIR, 'build')
HEADERS_DIR = path.join(MODULE_DIR, 'include')

-- create a new option for sloution
-- newoption {
-- 	trigger = "with-webgpu",
-- 	description = "Enable webgpu experimental renderer.",
-- }

function defaultSettingForProject()
    includedirs{
        path.join(MODULE_DIR, "ThirdParty/D3DX12/include"),
        path.join(MODULE_DIR, "ThirdParty/stb/include"),
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

function addGenerateMisc()
    includedirs{
        path.join(HEADERS_DIR, "misc"),
        path.join(HEADERS_DIR, "app")
    }
    files{
        path.join(SRC_DIR, "misc/**.cpp"),
        path.join(HEADERS_DIR, "misc/**.h")
    }
    vpaths {
        ["Headers"] = { 
            path.join(HEADERS_DIR, "misc/**.h")
        }
    }
end

function addCppEntryFile(fileName)
    files{
        path.join(SRC_DIR, string.format("app/%s.cpp",fileName)),
        path.join(HEADERS_DIR, string.format("app/%s.h",fileName))
    }
    vpaths {
        ["Headers"] = { 
            path.join(HEADERS_DIR, string.format("app/%s.h",fileName))
        }
    }
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

project "examples03"
    kind "WindowedApp"
    uuid (os.uuid("examples03"))

    files{
        path.join(SRC_DIR, "03Bundles.cpp"),
    }
    addGenerateMisc()
    defaultSettingForProject()
    addCppEntryFile("D3D12BundleApp")