#include "stdafx.h"
#include "D3D12BundleApp.h"
#include "Win32Application.h"

#define MINI_ENGINE_03_TITLE L"D3D12 Bundles"

_Use_decl_annotations_
int WINAPI WinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    D3D12Bundles sample(1280, 720, MINI_ENGINE_03_TITLE);
    return Win32Application::Run(&sample, hInstance, nCmdShow);
}