#include "stdafx.h"
#include "Win32Application.h"
#include "defineLib.h"

HWND Win32Application::_hwnd = nullptr;

int Win32Application::Run(DXSample* pSample, HINSTANCE hInstance, int nCmdShow)
{
    int argc;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLine(), &argc);
    pSample->ParseCommandLineArgs(argv, argc);

    // 释放argv占用的内存
    LocalFree(argv);

    WNDCLASSEX wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEX);
    // CS_HREDRAW 如果移动或大小调整更改了工作区的宽度，则重绘整个窗口
    // CS_VREDRAW 如果移动或大小调整更改了工作区的高度，则重新绘制整个窗口
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WindowProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);		//防止无聊的背景重绘
    wcex.lpszClassName = MINI_ENGINE_WND_CLASS_NAME;
    RegisterClassEx(&wcex);
    
    RECT rc = { 0, 0, static_cast<LONG>(pSample->GetWidth()), static_cast<LONG>(pSample->GetHeight()) };
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

    // 视口居中
    // GetSystemMetrics 获取屏幕信息
    //      SM_CXSCREEN   屏幕宽度
    //      SM_CYSCREEN   屏幕高度
    INT posX = (GetSystemMetrics(SM_CXSCREEN) - rc.right - rc.left) / 2;
    INT posY = (GetSystemMetrics(SM_CYSCREEN) - rc.bottom - rc.top) / 2;

    _hwnd = CreateWindow(wcex.lpszClassName
        , pSample->GetTitle()
        , WS_OVERLAPPEDWINDOW
        , posX
        , posY
        , rc.right - rc.left
        , rc.bottom - rc.top
        , nullptr
        , nullptr
        , hInstance
        , pSample);

    // 调用 Sample OnInit 句柄
    pSample->OnInit();

    ShowWindow(_hwnd, nCmdShow);

    // 主消息循环
    MSG msg = {};
    while (msg.message != WM_QUIT)
    {
        // 处理消息队列
        if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    pSample->OnDestroy();

    return static_cast<char>(msg.wParam);
}

LRESULT CALLBACK Win32Application::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    DXSample* pSample = reinterpret_cast<DXSample*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

    switch (message)
    {
    case WM_CREATE:
        {
            // 将 pSample 传递给 CreateWindow
            LPCREATESTRUCT pCreateStuct = reinterpret_cast<LPCREATESTRUCT>(lParam);
            SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreateStuct->lpCreateParams));
        }
        return 0;
    case WM_KEYDOWN:
        if(pSample)
        {
            pSample->OnKeyDown(static_cast<UINT8>(wParam));
        }
        return 0;
    case WM_KEYUP:
        if(pSample)
        {
            pSample->OnKeyUp(static_cast<UINT8>(wParam));
        }
        return 0;
    case WM_PAINT:
        if(pSample)
        {
            pSample->OnUpdate();
            pSample->OnRender();
        }
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}
