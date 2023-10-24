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
    configuration {}

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
    links{
        "d3d12",
        "dxgi",
        "d3dcompiler"
    }
end

function addCppEntryFile(fileName)
    files{
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
    flags { "Cpp20", "Unicode", "WinMain" }
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

project "examples03-Bundles"
    kind "WindowedApp"
    uuid (os.uuid("xamples03-Bundles"))

    files{
        path.join(SRC_DIR, "03Bundles.cpp"),
        path.join(SRC_DIR, "app/D3D12BundleApp.cpp"),
    }
    addGenerateMisc()
    defaultSettingForProject()
    addCppEntryFile("D3D12BundleApp")

project "examples04-ConstantBuffer"
    kind "WindowedApp"
    uuid (os.uuid("examples04"))

    files{
        path.join(SRC_DIR, "04CBuffer.cpp"),
        path.join(SRC_DIR, "app/D3D12CBuffer.cpp"),
    }
    addGenerateMisc()
    defaultSettingForProject()
    addCppEntryFile("D3D12CBuffer")

project "examples05-FrameBuffer"
    kind "WindowedApp"
    uuid (os.uuid("examples05-FrameBuffer"))

    files{
        path.join(SRC_DIR, "05FrameBuffer.cpp"),
        path.join(SRC_DIR, "app/D3D12FrameBuffer.cpp"),
    }
    addGenerateMisc()
    defaultSettingForProject()
    addCppEntryFile("D3D12FrameBuffer")

project "imgui"
    kind "StaticLib"
    uuid (os.uuid("imgui"))

    files{
        path.join(MODULE_DIR, "ThirdParty/imgui/imgui*"),
        path.join(MODULE_DIR, "ThirdParty/imgui/imconfig.h"),
        path.join(MODULE_DIR, "ThirdParty/imgui/misc/debuggers/imgui.natvis"),
        path.join(MODULE_DIR, "ThirdParty/imgui/misc/debuggers/imgui.natstepfilter"),
        path.join(MODULE_DIR, "ThirdParty/imgui/backends/imgui_impl_dx12.*"),
        path.join(MODULE_DIR, "ThirdParty/imgui/backends/imgui_impl_win32.*"),
    }

function addImguiSupport()
    includedirs({
        path.join(MODULE_DIR, "ThirdParty/imgui"),
    })
    links { "imgui" }
end

project "examples06-addImguiSupport"
    kind "WindowedApp"
    uuid (os.uuid("examples05"))

    files{
        path.join(SRC_DIR, "05FrameBuffer.cpp"),
        path.join(SRC_DIR, "app/D3D12FrameBuffer.cpp"),
    }
    addGenerateMisc()
    defaultSettingForProject()
    addCppEntryFile("D3D12FrameBuffer")
    addImguiSupport()