#include "stdafx.h"
#include "D3D12UseImGui.h"
#include "Win32Application.h"

constexpr std::wstring_view MINI_ENGINE_06_TITLE = L"D3D12 Use ImGui";

_Use_decl_annotations_
int WINAPI WinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    D3D12UseImGui sample(1280, 720, MINI_ENGINE_06_TITLE.data());
    return Win32Application::Run(&sample, hInstance, nCmdShow);
}