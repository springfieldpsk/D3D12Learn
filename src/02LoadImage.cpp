// ReSharper disable CppMsExtAddressOfClassRValue
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
#include <stdexcept>

using namespace Microsoft;
using namespace Microsoft::WRL;
using namespace DirectX;

// linker
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "d3dcompiler.lib")

#if defined(DEBUG)
#include <dxgidebug.h>
#endif

#include "D3DX12/include/d3dx12.h"

#define STB_IMAGE_IMPLEMENTATION
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
    XMFLOAT3 vertexPos; // pos
    XMFLOAT2 vertexUV; // Texcoord
};

const UINT nTexturePixelSize = 4;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

std::vector<UINT8> GenerateTexture(UINT nTextureW, UINT nTextureH)
{
    // 使用黑白纹理填充图片数据
    const UINT rowPitch = nTextureW * nTexturePixelSize;
    const UINT cellPitch = rowPitch >> 3;
    const UINT cellHeight = nTextureW >> 3;
    const UINT textureSize = rowPitch * nTextureH;

    std::vector<UINT8> data(textureSize);
    UINT8* pData = &data[0];

    for(UINT n = 0 ; n < textureSize; n += nTexturePixelSize)
    {
        UINT x = n % rowPitch;
        UINT y = n / rowPitch;
        UINT i = x / cellPitch;
        UINT j = y / cellHeight;

        if(i % 2 == j % 2)
        {
            pData[n] = 0x00;        // R
            pData[n + 1] = 0x00;    // G
            pData[n + 2] = 0x00;    // B
            pData[n + 3] = 0xff;    // A
        }
        else
        {
            pData[n] = 0xff;        // R
            pData[n + 1] = 0xff;    // G
            pData[n + 2] = 0xff;    // B
            pData[n + 3] = 0xff;    // A
        }
    }
    return data;
}

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
    const UINT nFrameBackBufCount = 3u;

    int iWidth = 800;
    int iHeight = 600;
    UINT nFrameIndex = 0;

    UINT nTextureW = 0u;
    UINT nTextureH = 0u;
    UINT nBPP	   = 0u;
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
    ComPtr<ID3D12DescriptorHeap>         pISRVHeap;
    ComPtr<ID3D12Resource>               pIARenderTargets[nFrameBackBufCount];
    ComPtr<ID3D12Resource>               pITexture;
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
        // 创建SRV堆
        D3D12_DESCRIPTOR_HEAP_DESC stSRVHeapDesc = {};
        stSRVHeapDesc.NumDescriptors = 1;
        stSRVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        stSRVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

        MINI_ENGINE_THROW(pID3DDevice->CreateDescriptorHeap(&stSRVHeapDesc, IID_PPV_ARGS(&pISRVHeap)));
        
        // 创建空的默认根签名对象
        // 根签名目的是集中管理在此前D3D11中在各个Slot中存储的资源
        // 根签名描述了渲染管线需要的资源存储在易失性存储中的数据格式
        // MSDN : 根签名是一个绑定约定，由应用程序定义，着色器使用它来定位他们需要访问的资源。
        // 目标是 PSO 的搭建
        // 根签名描述了常量、常量缓冲区（CBV）、资源（SRC，纹理）、无序访问缓冲（UAV、随机读写缓冲）、采样器等寄存器存储规划的结构体
        {
            // 创建根描述符
            D3D12_FEATURE_DATA_ROOT_SIGNATURE stFeatureData = {};
            // 确认 1.1 版本根签名是否受硬件支持
            stFeatureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

            // CheckFeatureSupport 用于检测高级特性的支持性
            if(FAILED(pID3DDevice->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &stFeatureData, sizeof(stFeatureData))))
            {
                stFeatureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
            }

            // 在 GPU 执行 SetGraphicsRootDescriptorTable后，我们不修改命令列表中的 SRV，因此可以使用默认 Rang 行为
            // CD3DX12_DESCRIPTOR_RANGE1 与 CD3DX12_ROOT_PARAMETER1 联合定义了渲染管线需要一个 SRV 资源
            CD3DX12_DESCRIPTOR_RANGE1 stDSPRanges[1];
            // baseShaderRegister 参数指定了 Texture 资源的寄存器参数 与 HLSL 代码中相互对应
            stDSPRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE);

            CD3DX12_ROOT_PARAMETER1 stRootParameters[1];
            stRootParameters[0].InitAsDescriptorTable(1, &stDSPRanges[0], D3D12_SHADER_VISIBILITY_PIXEL);

            // D3D12_STATIC_SAMPLER_DESC 定义了一个静态采样器对象 相当于一个默认参数 固化在根签名中
            D3D12_STATIC_SAMPLER_DESC stSampleDesc = {};
            stSampleDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
            stSampleDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
            stSampleDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
            stSampleDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
            stSampleDesc.MipLODBias = 0;
            stSampleDesc.MaxAnisotropy = 0;
            stSampleDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
            stSampleDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
            stSampleDesc.MinLOD = 0.0f;
            stSampleDesc.MaxLOD = D3D12_FLOAT32_MAX;
            // ShaderRegister 参数指定了 采样器 资源的寄存器参数 与 HLSL 代码中相互对应
            stSampleDesc.ShaderRegister = 0;
            stSampleDesc.RegisterSpace = 0;
            stSampleDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

            CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC stRootSigDesc;
            // 初始化时可以定义静态采样器数组，用于传入多个静态采样器
            stRootSigDesc.Init_1_1(_countof(stRootParameters), stRootParameters, 1, &stSampleDesc,
                D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
            
            ComPtr<ID3DBlob> pISignatureBlob;
            ComPtr<ID3DBlob> pIErrorBlob;

            // 这里使用 定义结构体再编译生成根签名的方式创建
            // 同时也已通过脚本化来定义一个根签名
            MINI_ENGINE_THROW(D3DX12SerializeVersionedRootSignature(
                &stRootSigDesc,
                stFeatureData.HighestVersion,
                &pISignatureBlob,
                &pIErrorBlob));

            MINI_ENGINE_THROW(pID3DDevice->CreateRootSignature(0,
                pISignatureBlob->GetBufferPointer(),
                pISignatureBlob->GetBufferSize(),
                IID_PPV_ARGS(&pIRootSignature)));
        }
        
        // 编译Shader创建渲染管线对象
        {
            ComPtr<ID3DBlob> pIBlobVertexShader;
            ComPtr<ID3DBlob> pIBlobPixelShader;

#if defined(DEBUG)
            // 调试状态下 打开 Shader 编译的调试标志 不优化
            UINT nCompileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
            UINT nCompileFlags = 0;
#endif
            TCHAR miniEngineFileName[MAX_PATH] = {};
            StringCchPrintf(miniEngineFileName,
                MAX_PATH,
                _T("%s\\Shader\\shaders02.hlsl"),
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
                    DXGI_FORMAT_R32G32B32_FLOAT,
                    0,
                    0,
                    D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                    0
                },
                {
                    "TEXCOORD",
                    0,
                    DXGI_FORMAT_R32G32_FLOAT,
                    0,
                    12,
                    D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                    0
                }
            };

            // 定义渲染管线状态描述结构，创建渲染管线对象
            D3D12_GRAPHICS_PIPELINE_STATE_DESC stPSODesc = {};
            stPSODesc.InputLayout = { stInputElementDescs, _countof(stInputElementDescs)};
            stPSODesc.pRootSignature = pIRootSignature.Get();
            stPSODesc.VS = CD3DX12_SHADER_BYTECODE(pIBlobVertexShader.Get());
            stPSODesc.PS = CD3DX12_SHADER_BYTECODE(pIBlobPixelShader.Get());
            stPSODesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
            stPSODesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
            stPSODesc.DepthStencilState.DepthEnable = FALSE;
            stPSODesc.DepthStencilState.StencilEnable = FALSE;
            stPSODesc.SampleMask = UINT_MAX;
            stPSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
            stPSODesc.NumRenderTargets = 1;
            stPSODesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
            stPSODesc.SampleDesc.Count = 1;
            
            MINI_ENGINE_THROW(pID3DDevice->CreateGraphicsPipelineState(&stPSODesc, IID_PPV_ARGS(&pIPipelineState)));
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
        // 创建顶点缓冲
        // 定义正方形的数据结构
        MINIENGINE_VERTEX stTriangleVertices[] = {
            { { -0.25f * fAspectRatio, -0.25f * fAspectRatio, 0.0f}, { 0.0f, 1.0f } },	// Bottom left.
            { { -0.25f* fAspectRatio, 0.25f * fAspectRatio, 0.0f}, { 0.0f, 0.0f } },	// Top left.
            { { 0.25f* fAspectRatio, -0.25f * fAspectRatio, 0.0f }, { 1.0f, 1.0f } },	// Bottom right.
            { { 0.25f* fAspectRatio, 0.25f * fAspectRatio, 0.0f}, { 1.0f, 0.0f } },		// Top right.
        };

        const UINT nVertexBufferSize = sizeof(stTriangleVertices);
        
        // 向设备 GPU 提交资源 (DX12 中不多的同步函数)
        MINI_ENGINE_THROW(pID3DDevice->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD)
            ,D3D12_HEAP_FLAG_NONE
            ,&CD3DX12_RESOURCE_DESC::Buffer(nVertexBufferSize)
            ,D3D12_RESOURCE_STATE_GENERIC_READ
            ,nullptr
            ,IID_PPV_ARGS(&pIVertexBuffer)));

        UINT8* pVertexDataBegin = nullptr;
        CD3DX12_RANGE stReadRange(0, 0);

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
        
        // 加载 2D 纹理
        {
            nTextureW = 1024u;
            nTextureH = 1024u;
            
            ComPtr<ID3D12Resource> pITexure2DUpload;

            // 使用 CD3DX12_RESOURCE_DESC::Tex2D 代替以下结构体
            // D3D12_RESOURCE_DESC stTexDesc = {};
            // stTexDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            // stTexDesc.MipLevels = 1;
            // stTexDesc.Format    = emRenderTargetFormat;
            // stTexDesc.Width     = nTextureW;
            // stTexDesc.Height    = nTextureH;
            // stTexDesc.Flags     = D3D12_RESOURCE_FLAG_NONE;
            // stTexDesc.DepthOrArraySize   = 1;
            // stTexDesc.SampleDesc.Count   = 1;
            // stTexDesc.SampleDesc.Quality = 0;

            // 创建默认堆上的资源，类型是Texture2D，GPU对默认堆的读取速度最快
            // 由于纹理资源一般是不易变资源，因此使用上传堆复制到默认堆上
            // 在传统的D3D11及以前的D3D接口中，这些过程都被封装了，我们只能指定创建时的类型为默认堆
            MINI_ENGINE_THROW(pID3DDevice->CreateCommittedResource(
                &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
                D3D12_HEAP_FLAG_NONE,
                &CD3DX12_RESOURCE_DESC::Tex2D(emRenderTargetFormat, nTextureW, nTextureH),
                D3D12_RESOURCE_STATE_COPY_DEST,
                nullptr,
                IID_PPV_ARGS(&pITexture)));

            // 使用 GetRequiredIntermediateSize 获取上传 Buffer 大小
            const UINT64 n64UploadBufferSize = GetRequiredIntermediateSize(pITexture.Get(), 0, 1);

            // 创建上传纹理的 Buffer 资源, 由于上传堆对于 GPU 访问效率很差，因此对于默认不变数据采用上传堆复制到默认堆上
            MINI_ENGINE_THROW(pID3DDevice->CreateCommittedResource(
                &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
                D3D12_HEAP_FLAG_NONE,
                &CD3DX12_RESOURCE_DESC::Buffer(n64UploadBufferSize),
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&pITexure2DUpload)));
            
            // 从图片中读取数据 也可以尝试使用黑白纹理填充 见 line601
            INT nTextureChannel = 0u;
            TCHAR miniEngineFileName[MAX_PATH] = {};
            StringCchPrintf(miniEngineFileName,
                MAX_PATH,
                _T("%s\\res\\testImg.png"),
                miniEngineAppPath);
    
            char texturePath[MAX_PATH];
            WideCharToMultiByte(CP_ACP, 0, miniEngineFileName, -1,
                texturePath, sizeof(texturePath), NULL, NULL);
            
            stbi_uc* pixels = stbi_load(texturePath, reinterpret_cast<int*>(&nTextureW), reinterpret_cast<int*>(&nTextureH)
                , &nTextureChannel, STBI_rgb_alpha);
            
            if (!pixels) {
                throw std::runtime_error("failed to load texture image!");
            }

            UINT imageSize = nTextureW * nTextureH * nTexturePixelSize;
            std::vector<UINT8> picTexutre(imageSize);
            memcpy(picTexutre.data(), pixels, imageSize);
            stbi_image_free(pixels);
            
            // 使用黑白纹理填充图片数据
            // std::vector<UINT8> picTexutre = GenerateTexture(nTextureW, nTextureH);

            D3D12_SUBRESOURCE_DATA textureData = {};
            textureData.pData = &picTexutre[0];
            textureData.RowPitch = nTextureW * nTexturePixelSize;
            textureData.SlicePitch = textureData.RowPitch * nTextureH;
            
            // // UpdateSubresources是一个DirectX 12的API，用于复制数据到 GPU 的资源中，如纹理、缓冲区等。这个函数将数据从 CPU 内存复制到 GPU 内存。
            UpdateSubresources(pICommandList.Get(), pITexture.Get(), pITexure2DUpload.Get()
                , 0 , 0 , 1, &textureData);
            pICommandList->ResourceBarrier(1,
                &CD3DX12_RESOURCE_BARRIER::Transition(
                    pITexture.Get(),
                    D3D12_RESOURCE_STATE_COPY_DEST,
                    D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
            
            // 创建 SRV 描述符
            D3D12_SHADER_RESOURCE_VIEW_DESC stSRVDesc = {};
            stSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            stSRVDesc.Format = emRenderTargetFormat;
            stSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            stSRVDesc.Texture2D.MipLevels = 1;
            pID3DDevice->CreateShaderResourceView(pITexture.Get(), &stSRVDesc, pISRVHeap->GetCPUDescriptorHandleForHeapStart());

            // 执行命令列表并等待纹理资源上传完成
            MINI_ENGINE_THROW(pICommandList->Close());
            ID3D12CommandList* ppCommandList[] = {pICommandList.Get()};
            pICommandQueue->ExecuteCommandLists(_countof(ppCommandList), ppCommandList);
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

            // 等待纹理资源正式复制完成
            const UINT64 fence = n64FenceValue;
            MINI_ENGINE_THROW(pICommandQueue->Signal(pIFence.Get(), fence));
            n64FenceValue++;

            // 查看命令是否真正执行到围栏标记，没有就利用事件等待
            if(pIFence->GetCompletedValue() < fence)
            {
                MINI_ENGINE_THROW(pIFence->SetEventOnCompletion(fence, hFenceEvent));
                WaitForSingleObject(hFenceEvent, INFINITE);
            }
        }

        // 先 Reset 命令分配器，因为执行了一次纹理复制
        MINI_ENGINE_THROW(pICommandAllocator->Reset());

        // Reset 命令列表，并重新指定命令分配器和 PSO
        MINI_ENGINE_THROW(pICommandList->Reset(pICommandAllocator.Get(), pIPipelineState.Get()));
        
        // 创建定时器对象，以便创建高效的消息循环
        HANDLE phWait = CreateWaitableTimer(NULL, FALSE, NULL);
        LARGE_INTEGER largeWaitTime = {};
        largeWaitTime.QuadPart = -1i64; // 1s 后开始计时
        SetWaitableTimer(phWait, &largeWaitTime, 40, NULL, NULL, 0); // 40ms周期的定时器
        
        DWORD dwRet = 0;
        BOOL bExit = FALSE;
        // 开始消息循环
        while (!bExit)
        {
            dwRet = ::MsgWaitForMultipleObjects(1, &phWait, FALSE, INFINITE, QS_ALLINPUT);

            switch (dwRet - WAIT_OBJECT_0)
            {
            case WAIT_TIMEOUT:
                {
                    
                }
                break;
            case 0:
                {
                    pICommandList->SetGraphicsRootSignature(pIRootSignature.Get());
                    ID3D12DescriptorHeap* ppHeaps[] = {pISRVHeap.Get()};

                    pICommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
                    pICommandList->SetGraphicsRootDescriptorTable(0, pISRVHeap->GetGPUDescriptorHandleForHeapStart());
                    pICommandList->RSSetViewports(1, &stViewPort);
                    pICommandList->RSSetScissorRects(1, &stScissorRect);

                    pICommandList->ResourceBarrier(1 ,&CD3DX12_RESOURCE_BARRIER::Transition(
                        pIARenderTargets[nFrameIndex].Get(),
                        D3D12_RESOURCE_STATE_PRESENT,
                        D3D12_RESOURCE_STATE_RENDER_TARGET));
                    
                    CD3DX12_CPU_DESCRIPTOR_HANDLE stRTVHandle(pIRTVHeap->GetCPUDescriptorHandleForHeapStart(),
                        nFrameIndex,
                        nRTVDescriptorSize);

                    // 设置渲染目标
                    pICommandList->OMSetRenderTargets(1, &stRTVHandle, FALSE, nullptr);
                    
                    pICommandList->ClearRenderTargetView(stRTVHandle, fClearColor, 0, nullptr);
                    // 设定输入到图形管线的图元类型
                    pICommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
                    pICommandList->IASetVertexBuffers(0, 1, &stVertexBufferView);

                    // Draw Call
                    pICommandList->DrawInstanced(_countof(stTriangleVertices), 1, 0, 0);

                    // 屏障等待绘制完成
                    pICommandList->ResourceBarrier(1 ,&CD3DX12_RESOURCE_BARRIER::Transition(
                           pIARenderTargets[nFrameIndex].Get(),
                           D3D12_RESOURCE_STATE_RENDER_TARGET,
                           D3D12_RESOURCE_STATE_PRESENT));

                    MINI_ENGINE_THROW(pICommandList->Close());

                    // 执行命令队列
                    ID3D12CommandList* ppCommandList[] = {pICommandList.Get()};
                    pICommandQueue->ExecuteCommandLists(_countof(ppCommandList), ppCommandList);

                    // 提交画面
                    MINI_ENGINE_THROW(pISwapChain3->Present(1, 0));

                    // 围栏同步CPU与GPU执行
                    const UINT64 fence = n64FenceValue;
                    MINI_ENGINE_THROW(pICommandQueue->Signal(pIFence.Get(), fence));
                    n64FenceValue++;
                    
                    // 查看命令是否真正执行到围栏标记，没有就利用事件等待
                    if(pIFence->GetCompletedValue() < fence)
                    {
                        MINI_ENGINE_THROW(pIFence->SetEventOnCompletion(fence, hFenceEvent));
                        WaitForSingleObject(hFenceEvent, INFINITE);
                    }

                    // 一帧渲染完成
                    nFrameIndex = pISwapChain3->GetCurrentBackBufferIndex();

                    // 重置命令分配器
                    MINI_ENGINE_THROW(pICommandAllocator->Reset());
                    MINI_ENGINE_THROW(pICommandList->Reset(pICommandAllocator.Get(), pIPipelineState.Get()));
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
