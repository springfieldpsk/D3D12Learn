#include "UIManager.h"
#include "imgui.h"
#include "defineLib.h"
#include "backends/imgui_impl_dx12.h"
#include "backends/imgui_impl_win32.h"
#include "Win32Application.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

//refers : https://www.cnblogs.com/timefiles/p/17632348.html
void UIManager::OnInit(UIMangerCreateInfo createInfo)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    // 开启 docking 与 viewPorts 特性，需要将分支切换至docking
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    // 设置 imgui 字体为微软雅黑
    ImFont* font = io.Fonts->AddFontFromFileTTF(
        "C:/Windows/Fonts/msyh.ttc",
        30,                                   // 字体大小
        nullptr,
        io.Fonts->GetGlyphRangesChineseFull() // 设定字符获取范围
    );

    MINI_ENGINE_THROW(font != nullptr);
    
    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // 创建一个用于imgui渲染的资源描述符堆
    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
    srvHeapDesc.NumDescriptors = 1;
    srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE; // 这个标志表明这个堆可以被绑定到命令列表，并且在着色器程序中可以访问这个堆中的描述符
    srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    MINI_ENGINE_THROW(createInfo.d3device.Get()->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&_srvHeap)));
    
    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(Win32Application::GetHwnd());
    ImGui_ImplDX12_Init(createInfo.d3device.Get(), createInfo.frameCnt,
        DXGI_FORMAT_R8G8B8A8_UNORM, _srvHeap.Get(),
        _srvHeap->GetCPUDescriptorHandleForHeapStart(),
        _srvHeap->GetGPUDescriptorHandleForHeapStart());

    // 当viewport被启用时，我们调整windowwround /WindowBg，使平台窗口看起来与常规窗口相同。
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }
    
    _uiArray.push_back([]()
    {
        ImGuiIO& io = ImGui::GetIO();
        ImGui::Begin("Hello, world!");                       
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        
        ImGui::End();
    });
    
    _uiArray.push_back([]()
    {
        ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_Once);
        ImGui::Begin("Main Window");
        ImGui::BeginChild("Child Window 1", ImVec2(200, 200), true);
        ImGui::EndChild();
        ImGui::BeginChild("Child Window 2", ImVec2(200, 200), true);
        ImGui::EndChild();
        ImGui::End();
    });
}

void UIManager::OnUpdate(UIMangerRuntimeInfo runtimeInfo)
{
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ID3D12DescriptorHeap* imguiHeaps[] = {_srvHeap.Get()};
    runtimeInfo.useCommandList.Get()->SetDescriptorHeaps(_countof(imguiHeaps), imguiHeaps);
    
    for(auto func : _uiArray)
    {
        func();
    }
    
    ImGui::Render();
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), runtimeInfo.useCommandList.Get());

    // Update and Render additional Platform Windows
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault(nullptr, (void*)runtimeInfo.useCommandList.Get());
    }
}

void UIManager::OnDestroy()
{
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

LRESULT UIManager::UIProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;
    return false;
}



