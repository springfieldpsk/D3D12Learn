#include <SDKDDKVer.h>      //用于根据您希望程序支持的操作系统从Windows头控制将哪些函数、常量等包含到代码中。copy的注释，知道和Windows相关就得了。删了都不报错
#define WIN32_LEAN_AND_MEAN // 从 Windows 头中排除极少使用的资料
#include <windows.h>
#include <tchar.h>          
#include <strsafe.h>
#include <wrl.h>		    //添加WTL支持 方便使用COM，就是COM只能指针需要的
#include <dxgi1_6.h>
#include <DirectXMath.h>
// for d3d12
#include <d3d12.h>
#include <d3d12shader.h>
#include <d3dcompiler.h>

using namespace Microsoft;
using namespace Microsoft::WRL;
using namespace DirectX;

// linker
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "d3dcompiler.lib")

#if defined(_DEBUG)
#include <dxgidebug.h>
#endif

#include "D3DX12/include/d3dx12.h"
#include "stb/include/stb_image.h"

#define MINI_ENGINE_WND_CLASS_NAME "Test Dx12 Class"
#define MINI_ENGINE_WND_TITLE "Test Dx12 Window"
#define MINI_ENGINE_WND_CLASS_NAME_L L"Test Dx12 Class"
#define MINI_ENGINE_WND_TITLE_L L"Test Dx12 Window"


#define MINI_ENGINE_THROW(hr) if (FAILED(hr)){ throw MiniEngineExpression(hr); } // 抛出异常宏

// 定义抛错类
class MiniEngineExpression
{
public:
    MiniEngineExpression(HRESULT hr) : hr_(hr) {}

    HRESULT Error() const
    {
        return hr_;
    }
private:
    const HRESULT hr_;
};

// 定义顶点格式
struct MINIENGINE_VERTEX
{
    XMFLOAT4 position;
    XMFLOAT4 color;
};

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
    const UINT nFrameBackBufCount = 3u;

    int iWidth = 800;
    int iHeight = 600;
    UINT nFrameIndex = 0;

    DXGI_FORMAT emRenderTargetFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    const float fClearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    UINT nDXGIFactoryFlags = 0u;
    UINT nRTVDescriptorSize = 0u;

    HWND hWnd = nullptr;
    MSG msg = {};
    TCHAR miniEngineAppPath[MAX_PATH] = {};
    
    float fAspectRatio = 3.0f;

    D3D12_VERTEX_BUFFER_VIEW stVertexBufferView = {};

    UINT64 n64FenceValue = 0ui64;
    HANDLE hFenceEvent = nullptr;

    CD3DX12_VIEWPORT stViewPort(0.0f, 0.0f, static_cast<float>(iWidth), static_cast<float>(iHeight));
    CD3DX12_RECT stScissorRect(0.0f, 0.0f, static_cast<float>(iWidth), static_cast<float>(iHeight));

    D3D_FEATURE_LEVEL					emFeatureLevel = D3D_FEATURE_LEVEL_12_1;
    
    ComPtr<IDXGIFactory5>                pIDXGIFactory5;
    ComPtr<IDXGIAdapter1>                pIAdapter;
    ComPtr<ID3D12Device4>                pID3DDevice;
    ComPtr<ID3D12CommandQueue>           pICommandQueue;
    ComPtr<IDXGISwapChain1>              pISwapChain1;
    ComPtr<IDXGISwapChain3>              pISwapChain3;
    ComPtr<ID3D12DescriptorHeap>         pIRTVHeap;
    ComPtr<ID3D12Resource>               pIARenderTargets[nFrameBackBufCount];
    ComPtr<ID3D12CommandAllocator>       pICommandAllocator;
    ComPtr<ID3D12RootSignature>          pIRootSignature;
    ComPtr<ID3D12PipelineState>          pIPipelineState;
    ComPtr<ID3D12GraphicsCommandList>    pICommandList;
    ComPtr<ID3D12Resource>               pIVertexBuffer;
    ComPtr<ID3D12Fence>                  pIFence;

    try
    {
        // 获取当前工作目录 方便使用相对路径获取资源文件
        {
            // 获取当前运行目录
            if(::GetModuleFileName(nullptr, miniEngineAppPath, MAX_PATH))
            {
                // HRESULT_FROM_WIN32 将 系统错误代码 映射到 HRESULT 值
                // GetLastError 检索调用线程的最后错误代码值
                MINI_ENGINE_THROW(HRESULT_FROM_WIN32(GetLastError()));
            }
            
            // _tcsstr 查找一个字符串中的子串
            TCHAR* lastSlash = _tcsstr(miniEngineAppPath, _T("\\build"));
            if(lastSlash)
            {
                *(lastSlash) = _T('\0');
            }
        }
        
        // 创建窗口
        {
            WNDCLASSEX wcex = {};
            wcex.cbSize = sizeof(WNDCLASSEX);
            wcex.style = CS_GLOBALCLASS;
            wcex.lpfnWndProc = WndProc;
            wcex.cbClsExtra = 0;
            wcex.cbWndExtra = 0;
            wcex.hInstance = hInstance;
            wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
            wcex.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);		//防止无聊的背景重绘
            wcex.lpszClassName = MINI_ENGINE_WND_CLASS_NAME_L;
            RegisterClassEx(&wcex);

            DWORD dwWndStyle = WS_OVERLAPPED | WS_SYSMENU; // 先不使用 WS_OVERLAPPEDWINDOW 避免处理OnSize
            RECT rc = { 0, 0, iWidth, iHeight };
            AdjustWindowRect(&rc, dwWndStyle, FALSE);

            // 视口居中
            // GetSystemMetrics 获取屏幕信息
            //      SM_CXSCREEN   屏幕宽度
            //      SM_CYSCREEN   屏幕高度
            INT posX = (GetSystemMetrics(SM_CXSCREEN) - rc.right - rc.left) / 2;
            INT posY = (GetSystemMetrics(SM_CYSCREEN) - rc.bottom - rc.top) / 2;

            hWnd = CreateWindowW(MINI_ENGINE_WND_CLASS_NAME_L
                , MINI_ENGINE_WND_TITLE_L
                , dwWndStyle
                , posX
                , posY
                , rc.right - rc.left
                , rc.bottom - rc.top
                , nullptr
                , nullptr
                , hInstance
                , nullptr);

            if(!hWnd)
            {
                return FALSE;
            }

            ShowWindow(hWnd, nCmdShow);
            UpdateWindow(hWnd);
        }

        // 创建 DXGI Factory 对象
        {
            // IID_PPV_ARGS 接受COM指针 返回一个包含接口IID和接口指针的元组
            MINI_ENGINE_THROW(CreateDXGIFactory2(nDXGIFactoryFlags, IID_PPV_ARGS(&pIDXGIFactory5)));
            // 关闭 ALT + ENTER 切换全屏 避免涉及 OnSize
            MINI_ENGINE_THROW(pIDXGIFactory5->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER));
        }

        // 枚举显示适配器 选择合适的适配器创建 D3D2 设备对象接口
        {
            DXGI_ADAPTER_DESC1 stAdapterDesc = {};
            for(UINT i = 0; pIDXGIFactory5->EnumAdapters1(i, &pIAdapter); ++i)
            {
                pIAdapter->GetDesc1(&stAdapterDesc);

                // 跳过虚拟显示适配器
                if(stAdapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                {
                    continue;
                }

                //检查适配器是否符合标准 12.1 (使用DXR) 返回参数设置为nullptr 以避免创建设备导致重复释放
                // _uuidof 检索附加到表达式的 GUID
                if(SUCCEEDED(D3D12CreateDevice(pIAdapter.Get(), emFeatureLevel, _uuidof(ID3D12Device), nullptr)))
                {
                    break;
                }
            }

            // 创建设备
            MINI_ENGINE_THROW(D3D12CreateDevice(pIAdapter.Get(), emFeatureLevel, IID_PPV_ARGS(&pID3DDevice)));

            TCHAR pszWndTitle[MAX_PATH] = {};
            MINI_ENGINE_THROW(pIAdapter->GetDesc1(&stAdapterDesc));
            // 获取窗口标题
            ::GetWindowText(hWnd, pszWndTitle, MAX_PATH);

            // 格式化窗口标题
            StringCchPrintf(pszWndTitle, MAX_PATH, _T("%s (GPU:%s)"), pszWndTitle
                , stAdapterDesc.Description);
            ::SetWindowText(hWnd, pszWndTitle);
        }

        // 创建命令队列
        {
            D3D12_COMMAND_QUEUE_DESC stQueueDesc = {};
            // D3D12 中 命令队列分成多种类型，以支持不同的GPU操作模式，
            // 这里使用 D3D12_COMMAND_LIST_TYPE_DIRECT，代表支持所有模式的命令队列
            stQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
            MINI_ENGINE_THROW(pID3DDevice->CreateCommandQueue(&stQueueDesc, IID_PPV_ARGS(&pICommandQueue)))
        }

        // 创建交换链
        {
            DXGI_SWAP_CHAIN_DESC1 stSwapChainDesc = {};
            stSwapChainDesc.BufferCount = nFrameBackBufCount;
            stSwapChainDesc.Width = iWidth;
            stSwapChainDesc.Height = iHeight;
            stSwapChainDesc.Format = emRenderTargetFormat;
            stSwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            stSwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
            stSwapChainDesc.SampleDesc.Count = 1;

            // 交换链的创建需要命令队列参与
            MINI_ENGINE_THROW(pIDXGIFactory5->CreateSwapChainForHwnd(
                pICommandQueue.Get(),
                hWnd,
                &stSwapChainDesc,
                nullptr,
                nullptr,
                &pISwapChain1));

            // 将低版本的交换链通过As转换为高版本
            MINI_ENGINE_THROW(pISwapChain1.As(&pISwapChain3));

            // 使用高版本的交换链获取下一个缓冲区的序号
            nFrameIndex = pISwapChain3->GetCurrentBackBufferIndex();

            // 创建RTV描述符堆 (可以理解为固定大小元素的固定大小显存池）
            {
                D3D12_DESCRIPTOR_HEAP_DESC stRTVHeapDesc = {};
                stRTVHeapDesc.NumDescriptors = nFrameBackBufCount;
                stRTVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
                stRTVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

                MINI_ENGINE_THROW(pID3DDevice->CreateDescriptorHeap(&stRTVHeapDesc, IID_PPV_ARGS(&pIRTVHeap)));

                // 获取描述符堆大小
                nRTVDescriptorSize = pID3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
            }

            // 创建RTV描述符
            {
                CD3DX12_CPU_DESCRIPTOR_HANDLE stRTVHandle(pIRTVHeap->GetCPUDescriptorHandleForHeapStart());
                for(UINT i = 0; i < nFrameBackBufCount; ++i)
                {
                    MINI_ENGINE_THROW(pISwapChain3->GetBuffer(i, IID_PPV_ARGS(&pIARenderTargets[i])));
                    pID3DDevice->CreateRenderTargetView(pIARenderTargets[i].Get(), nullptr, stRTVHandle);
                    stRTVHandle.Offset(1, nRTVDescriptorSize);
                }
            }
        }

        // 创建空的默认根签名对象
        // 根签名目的是集中管理在此前D3D11中在各个Slot中存储的资源
        // 根签名描述了渲染管线需要的资源存储在易失性存储中的数据格式
        // MSDN : 根签名是一个绑定约定，由应用程序定义，着色器使用它来定位他们需要访问的资源。
        // 目标是 PSO 的搭建
        // 根签名描述了常量、常量缓冲区（CBV）、资源（SRC，纹理）、无序访问缓冲（UAV、随机读写缓冲）、采样器等寄存器存储规划的结构体
        {
            // 可以使用 d3dx12.h 中的 CD3DX12_ROOT_SIGNATURE_DESC 代替
            // CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
            // rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
            D3D12_ROOT_SIGNATURE_DESC stRootSigDesc = {
            0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
            };

            ComPtr<ID3DBlob> pISignatureBlob;
            ComPtr<ID3DBlob> pIErrorBlob;

            // 这里使用 定义结构体再编译生成根签名的方式创建
            // 同时也已通过脚本化来定义一个根签名
            MINI_ENGINE_THROW(D3D12SerializeRootSignature(
                &stRootSigDesc,
                D3D_ROOT_SIGNATURE_VERSION_1,
                &pISignatureBlob,
                &pIErrorBlob));

            MINI_ENGINE_THROW(pID3DDevice->CreateRootSignature(0,
                pISignatureBlob->GetBufferPointer(),
                pISignatureBlob->GetBufferSize(),
                IID_PPV_ARGS(&pIRootSignature)));
        }
        
        {
            // 创建命令列表分配器
            MINI_ENGINE_THROW(pID3DDevice->CreateCommandAllocator(
                D3D12_COMMAND_LIST_TYPE_DIRECT,
                IID_PPV_ARGS(&pICommandAllocator)));

            // 创建命令列表 同时与PSO绑定
            MINI_ENGINE_THROW(pID3DDevice->CreateCommandList(
                0,
                D3D12_COMMAND_LIST_TYPE_DIRECT,
                pICommandAllocator.Get(),
                pIPipelineState.Get(),
                IID_PPV_ARGS(&pICommandList)));
        }
        // 编译Shader创建渲染管线对象
        {
            ComPtr<ID3DBlob> pIBlobVertexShader;
            ComPtr<ID3DBlob> pIBlobPixelShader;

#if defined(_DEBUG)
            // 调试状态下 打开 Shader 编译的调试标志 不优化
            UINT nCompileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
            UINT nCompileFlags = 0;
#endif
            TCHAR miniEngineFileName[MAX_PATH] = {};
            StringCchPrintf(miniEngineFileName,
                MAX_PATH,
                _T("%s\\Shader\\shaders.hlsl"),
                miniEngineAppPath);

            MINI_ENGINE_THROW(D3DCompileFromFile(miniEngineFileName,
                nullptr,
                nullptr,
                "VSMain",
                "vs_5_0",
                nCompileFlags,
                0,
                &pIBlobVertexShader,
                nullptr));

            MINI_ENGINE_THROW(D3DCompileFromFile(miniEngineFileName,
                nullptr,
                nullptr,
                "PSMain",
                "ps_5_0",
                nCompileFlags,
                0,
                &pIBlobPixelShader,
                nullptr));

            // 定义顶点格式
            D3D12_INPUT_ELEMENT_DESC stInputElementDescs [] = {
                {
                    "POSITION",
                    0,
                    DXGI_FORMAT_R32G32B32A32_FLOAT,
                    0,
                    0,
                    D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                    0
                },
                {
                    "COLOR",
                    0,
                    DXGI_FORMAT_R32G32B32A32_FLOAT,
                    0,
                    16,
                    D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                    0
                }
            };

            // 定义渲染管线状态描述结构，创建渲染管线对象
            D3D12_GRAPHICS_PIPELINE_STATE_DESC stPSODesc = {};
            stPSODesc.InputLayout = { stInputElementDescs, _countof(stInputElementDescs)};
            stPSODesc.pRootSignature = pIRootSignature.Get();
            stPSODesc.VS.pShaderBytecode = pIBlobVertexShader->GetBufferPointer();
            stPSODesc.VS.BytecodeLength = pIBlobVertexShader->GetBufferSize();
            stPSODesc.PS.pShaderBytecode = pIBlobPixelShader->GetBufferPointer();
            stPSODesc.PS.BytecodeLength = pIBlobPixelShader->GetBufferSize();

            stPSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
            stPSODesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;

            stPSODesc.BlendState.AlphaToCoverageEnable = FALSE;
            stPSODesc.BlendState.IndependentBlendEnable = FALSE;
            stPSODesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

            stPSODesc.DepthStencilState.DepthEnable = FALSE;
            stPSODesc.DepthStencilState.StencilEnable = FALSE;

            stPSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

            stPSODesc.NumRenderTargets = 1;
            stPSODesc.RTVFormats[0] = emRenderTargetFormat;

            stPSODesc.SampleMask = UINT_MAX;
            stPSODesc.SampleDesc.Count = 1;
            
            MINI_ENGINE_THROW(pID3DDevice->CreateGraphicsPipelineState(&stPSODesc, IID_PPV_ARGS(&pIPipelineState)));
        }

        // 创建顶点缓冲
        {
            // 定义三角形的数据结构，每个顶点色使用三原色之一
            MINIENGINE_VERTEX stTriangleVertices[] = {
                {{ 0.0f, 0.25f * fAspectRatio, 0.0f, 1.0f},{1.0f, 0.0f, 0.0f, 1.0f}},
                {{ 0.25f * fAspectRatio, -0.25f * fAspectRatio, 0.0f, 1.0f},{1.0f, 0.0f, 0.0f, 1.0f}},
                {{ -0.25f * fAspectRatio, -0.25f * fAspectRatio, 0.0f, 1.0f},{1.0f, 0.0f, 0.0f, 1.0f}}
            };

            const UINT nVertexBufferSize = sizeof(stTriangleVertices);

            // CPU 内存上传至缓存
            D3D12_HEAP_PROPERTIES stHeapProp = { D3D12_HEAP_TYPE_UPLOAD };

            // 描述顶点资源描述符
            D3D12_RESOURCE_DESC stResSesc = {};
            stResSesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            stResSesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            stResSesc.Flags = D3D12_RESOURCE_FLAG_NONE;
            stResSesc.Format = DXGI_FORMAT_UNKNOWN;
            stResSesc.Width = nVertexBufferSize;
            stResSesc.Height = 1;
            stResSesc.DepthOrArraySize = 1;
            stResSesc.MipLevels = 1;
            stResSesc.SampleDesc.Count = 1;
            stResSesc.SampleDesc.Quality = 0;

            // 向设备 GPU 提交资源 (DX12 中不多的同步函数)
            MINI_ENGINE_THROW(pID3DDevice->CreateCommittedResource(&stHeapProp
                , D3D12_HEAP_FLAG_NONE
                ,&stResSesc
                ,D3D12_RESOURCE_STATE_GENERIC_READ
                ,nullptr
                ,IID_PPV_ARGS(&pIVertexBuffer)));

            UINT8* pVertexDataBegin = nullptr;
            D3D12_RANGE stReadRange = {0,0};

            // Map 获取指向资源中指定子资源的 CPU 指针，但可能不会向应用程序透露指针值。
            // 映射 还会在必要时使 CPU 缓存失效，以便 CPU 读取到此地址反映 GPU 所做的任何修改。
            MINI_ENGINE_THROW(pIVertexBuffer->Map(
                0,
                &stReadRange,
                reinterpret_cast<void**>(&pVertexDataBegin)));

            memcpy(pVertexDataBegin, stTriangleVertices, sizeof(stTriangleVertices));

            // 使指向资源中指定子资源的 CPU 指针失效。
            pIVertexBuffer->Unmap(0 ,nullptr);

            // 通过结构体对象描述资源视图 使得GPU明确当前资源为 Vertex Buffer 目的是 脚本化实现
            stVertexBufferView.BufferLocation = pIVertexBuffer->GetGPUVirtualAddress();
            stVertexBufferView.StrideInBytes = sizeof(MINIENGINE_VERTEX);
            stVertexBufferView.SizeInBytes = nVertexBufferSize;
        }

        // 创建同步对象 Fence 等待异步渲染完成
        {
            MINI_ENGINE_THROW(pID3DDevice->CreateFence(0,
                D3D12_FENCE_FLAG_NONE,
                IID_PPV_ARGS(&pIFence)));

            n64FenceValue = 1;

            // 创建一个 Event 同步对象，用于等待围栏事件通知
            hFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
            if(hFenceEvent == nullptr)
            {
                MINI_ENGINE_THROW(HRESULT_FROM_WIN32(GetLastError()));
            }
        }

        // 填充资源屏障结构
        D3D12_RESOURCE_BARRIER stBeginResBarrier = {};
        stBeginResBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        stBeginResBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        stBeginResBarrier.Transition.pResource = pIARenderTargets[nFrameIndex].Get();
        stBeginResBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        stBeginResBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        stBeginResBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        D3D12_RESOURCE_BARRIER stEndResBarrier = {};
        stEndResBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        stEndResBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        stEndResBarrier.Transition.pResource = pIARenderTargets[nFrameIndex].Get();
        stEndResBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        stEndResBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        stEndResBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        D3D12_CPU_DESCRIPTOR_HANDLE stRTVHandle = pIRTVHeap->GetCPUDescriptorHandleForHeapStart();
        DWORD dwRet = 0;
        BOOL bExit = FALSE;

        MINI_ENGINE_THROW(pICommandList->Close());
        SetEvent(hFenceEvent);

        // 开始消息循环
        while (!bExit)
        {
            dwRet = ::MsgWaitForMultipleObjects(1, &hFenceEvent, FALSE, INFINITE, QS_ALLINPUT);

            switch (dwRet - WAIT_OBJECT_0)
            {
            case 0:
                {
                    // 获取新的缓冲区号，当 Present 完成时后缓冲号更新
                    nFrameIndex = pISwapChain3->GetCurrentBackBufferIndex();

                    // Reset 命令分配器
                    MINI_ENGINE_THROW(pICommandAllocator->Reset());

                    // Reset 命令列表以重新指定命令分配器与PSO对象
                    MINI_ENGINE_THROW(pICommandList->Reset(pICommandAllocator.Get(), pIPipelineState.Get()));

                    // 开始记录命令
                    pICommandList->SetGraphicsRootSignature(pIRootSignature.Get());
                    pICommandList->SetPipelineState(pIPipelineState.Get());
                    pICommandList->RSSetViewports(1, &stViewPort);
                    pICommandList->RSSetScissorRects(1, &stScissorRect);

                    // 通过资源屏障判定后缓冲已经切换完毕 准备渲染
                    stBeginResBarrier.Transition.pResource = pIARenderTargets[nFrameIndex].Get();
                    pICommandList->ResourceBarrier(1, &stBeginResBarrier);

                    stRTVHandle = pIRTVHeap->GetCPUDescriptorHandleForHeapStart();
                    stRTVHandle.ptr += nFrameIndex * nRTVDescriptorSize;

                    // 设置渲染目标
                    pICommandList->OMSetRenderTargets(1, &stRTVHandle, FALSE, nullptr);

                    pICommandList->ClearRenderTargetView(stRTVHandle, fClearColor, 0, nullptr);
                    pICommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
                    pICommandList->IASetVertexBuffers(0, 1, &stVertexBufferView);

                    // DrawCall
                    pICommandList->DrawInstanced(3, 1, 0, 0);

                    stEndResBarrier.Transition.pResource = pIARenderTargets[nFrameIndex].Get();
                    pICommandList->ResourceBarrier(1, &stEndResBarrier);

                    // 关闭命令队列， 准备执行
                    MINI_ENGINE_THROW(pICommandList->Close());

                    // 执行命令列表
                    ID3D12CommandList* ppCommandList[] = {pICommandList.Get()};
                    pICommandQueue->ExecuteCommandLists(_countof(ppCommandList), ppCommandList);

                    // 提交画面
                    MINI_ENGINE_THROW(pISwapChain3->Present(1, 0));

                    // 开始同步 GPU 与 CPU 执行，先记录围栏标记值
                    const UINT64 n64CurrentFenceValue = n64FenceValue;
                    MINI_ENGINE_THROW(pICommandQueue->Signal(pIFence.Get(), n64CurrentFenceValue));
                    n64FenceValue ++ ;
                    MINI_ENGINE_THROW(pIFence->SetEventOnCompletion(n64CurrentFenceValue, hFenceEvent));
                }
                break;
            case 1:
                {
                    // 处理消息
                    while (::PeekMessage(&msg, NULL, 0, 0 ,PM_REMOVE))
                    {
                        if(WM_QUIT != msg.message)
                        {
                            ::TranslateMessage(&msg);
                            ::DispatchMessage(&msg);
                        }
                        else
                        {
                            bExit = TRUE;
                        }
                    }
                }
                break;
            case WAIT_TIMEOUT:
                {
                    
                }
                break;
            default:
                break;
            }
        }
    }
    catch (MiniEngineExpression &e)
    {
        e;
    }
    return 0;
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
