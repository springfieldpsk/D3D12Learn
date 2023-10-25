#include "UIManager.h"
#include "imgui.h"
#include "backends/imgui_impl_dx12.h"
#include "backends/imgui_impl_win32.h"
#include "Win32Application.h"

void UIManager::OnInit(UIMangerCreateInfo createInfo)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(Win32Application::GetHwnd());
    ImGui_ImplDX12_Init(createInfo.d3device.Get(), createInfo.frameCnt,
        DXGI_FORMAT_R8G8B8A8_UNORM, createInfo.srvHeap.Get(),
        createInfo.srvHeap.Get()->GetCPUDescriptorHandleForHeapStart(),
        createInfo.srvHeap.Get()->GetGPUDescriptorHandleForHeapStart());
}

void UIManager::OnUpdate(UIMangerRuntimeInfo runtimeInfo)
{
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    
    for (auto element : _uiArray)
    {
        element->OnUpdate();
    }

    ImGui::Render();
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), runtimeInfo.useCommandList.Get());
}

void UIManager::OnDestroy()
{
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}




