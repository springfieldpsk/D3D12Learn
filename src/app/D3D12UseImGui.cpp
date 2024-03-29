#include "D3D12UseImGui.h"
#include "defineLib.h"
#include "Win32Application.h"
#include "UIManager.h"
#include "imgui.h"
#include "backends/imgui_impl_dx12.h"
#include "backends/imgui_impl_win32.h"

D3D12UseImGui::D3D12UseImGui(UINT width, UINT height, std::wstring name):
    DXSample(width, height, name),
    _frameIndex(0),
    _viewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)),
    _scissorRect(0, 0, static_cast<LONG>(width), static_cast<LONG>(height)),
    _rtvDescriptorSize(0),
    _pCbvDataBegin(nullptr),
    _constantBufferData{},
    _fenceValue{}
{
    
}

void D3D12UseImGui::OnInit()
{
    LoadPipeline();
    LoadAssets();
    UIMangerCreateInfo createInfo = UIMangerCreateInfo(
        FrameCount, _device);
    UIManager::getInstance().OnInit(createInfo);
}

void D3D12UseImGui::LoadPipeline()
{
    UINT dxgiFactoryFlags = 0;

#if defined(_DEBUG)
    {
        ComPtr<ID3D12Debug> debugController;
        if(SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        {
            debugController->EnableDebugLayer();

            // 允许额外的调试层
            dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
        }
    }
#endif

    ComPtr<IDXGIFactory4> factory;
    MINI_ENGINE_THROW(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));

    if(_useWarpDevice)
    {
        ComPtr<IDXGIAdapter> warpAdapter;
        MINI_ENGINE_THROW(factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));

        MINI_ENGINE_THROW(D3D12CreateDevice(
            warpAdapter.Get(),
            _useFeatureLevel,
            IID_PPV_ARGS(&_device)));
    }
    else
    {
        ComPtr<IDXGIAdapter1> hardwareWarpAdapter;
        MINI_ENGINE_THROW(factory->EnumWarpAdapter(IID_PPV_ARGS(&hardwareWarpAdapter)));

        MINI_ENGINE_THROW(D3D12CreateDevice(
            hardwareWarpAdapter.Get(),
            _useFeatureLevel,
            IID_PPV_ARGS(&_device)));
    }

    // 描述并创建 commandQueue
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    MINI_ENGINE_THROW(_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&_commandQueue)));

    // 描述并创建交换链
    // DXGI_SWAP_EFFECT_DISCARD:
    // 这是默认值，它会在每次呈现后丢弃后缓冲区的内容。意味着每一帧都是新的、干净的帧。
    // DXGI_SWAP_EFFECT_SEQUENTIAL:
    // 与DISCARD类似，但是它会保存呈现的帧，这可能对某些类型的应用程序有用，如视频播放器。它会按顺序逐一呈现缓冲区的内容。
    // DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL:
    // 这种模式会在呈现后翻转（切换）到另一个缓冲区，而不是直接丢弃。这种模式对于需要更高效率的应用程序很有用，因为它可以避免重复渲染相同的场景。
    // DXGI_SWAP_EFFECT_FLIP_DISCARD:
    // 与FLIP_SEQUENTIAL类似，但是在翻转后，它会立即丢弃以前的缓冲区内容。这可以帮助节省内存，特别是在资源有限的设备上。
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = FrameCount;
    swapChainDesc.Width = _width;
    swapChainDesc.Height = _height;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;

    ComPtr<IDXGISwapChain1> swapChain;
    MINI_ENGINE_THROW(factory->CreateSwapChainForHwnd(
        _commandQueue.Get(),
        Win32Application::GetHwnd(),
        &swapChainDesc,
        nullptr,
        nullptr,
        &swapChain));

    // 该样例不支持全屏
    MINI_ENGINE_THROW(factory->MakeWindowAssociation(Win32Application::GetHwnd(), DXGI_MWA_NO_ALT_ENTER));

    MINI_ENGINE_THROW(swapChain.As(&_swapChain));
    _frameIndex = _swapChain->GetCurrentBackBufferIndex();

    // 创建描述符堆
    {
        // 描述并创建一个渲染对象描述符堆
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.NumDescriptors = FrameCount;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        MINI_ENGINE_THROW(_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&_rtvHeap)));

        // 获取描述符句柄的增量大小
        _rtvDescriptorSize = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        // 声明并构建一个常量缓冲区描述符堆
        D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc = {};
        cbvHeapDesc.NumDescriptors = 1;
        cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE; // 这个标志表明这个堆可以被绑定到命令列表，并且在着色器程序中可以访问这个堆中的描述符
        cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        MINI_ENGINE_THROW(_device->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&_cbvHeap)));
    }

    // 创建帧资源
    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(_rtvHeap->GetCPUDescriptorHandleForHeapStart());

        // 为每帧创建一个 RTV
        for(UINT i = 0 ; i < FrameCount; i ++)
        {
            MINI_ENGINE_THROW(_swapChain->GetBuffer(i, IID_PPV_ARGS(&_renderTarget[i])));
            _device->CreateRenderTargetView(_renderTarget[i].Get(), nullptr, rtvHandle);
            rtvHandle.Offset(1, _rtvDescriptorSize);
            // 创建命令列表分配器
            MINI_ENGINE_THROW(_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_commandAllocator[i])));
        }
    }
}

void D3D12UseImGui::LoadAssets()
{
    // 创建一个根签名对象包含一个有一个CBV描述符表
    {
        D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};

        // 声明支持的最高版本
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

        if(_device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData)))
        {
            // 若无法支持则向下兼容
            featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
        }

        // 描述符范围是一个连续的GPU描述符集合，这些描述符可以在着色器程序中被访问
        CD3DX12_DESCRIPTOR_RANGE1 ranges[1];
        // DirectX 12中，Root参数（Root Parameter）是用来定义着色器和管道状态之间交互所需的常量、描述符或者内存地址的。
        // Root参数可以是常量值，描述符表，描述符范围或者根签名的一部分。所以，Root参数的一种角色可以被视为是着色器的输入。
        CD3DX12_ROOT_PARAMETER1 rootParameters[1];

        // 在DirectX 12中，shader注册号和空间号是用来在HLSL着色器程序中标识和访问资源的。
        // 注册号(Register Number)：
        // 它在HLSL中用来标识资源。例如，如果你有一个常量缓冲区被绑定到寄存器b0，那么你可以在HLSL程序中通过cb0来访问这个常量缓冲区。
        // 空间号(Space Number)：
        // 当你有大量的资源需要在着色器中访问时，使用空间号可以帮助你组织这些资源。你可以把空间号看做是资源的额外维度。
        // 例如，你可以在空间0中有一个寄存器b0，同时在空间1中也有一个寄存器b0。这两个寄存器可以在HLSL中通过cb0(space0)和cb0(space1)来分别访问。
        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);

        // 初始化一个包含一个描述符表的Root参数
        // D3D12_SHADER_VISIBILITY_VERTEX 指定 Root 参数只在顶点着色器中可见
        rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_VERTEX);

        // 允许对布局进行输入，并拒绝对某些管道阶段的不必要访问。
        D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
        rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters,
            0, nullptr, rootSignatureFlags);
        
        ComPtr<ID3DBlob> signature;
        ComPtr<ID3DBlob> error;
        MINI_ENGINE_THROW(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error));
        MINI_ENGINE_THROW(_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(),
            IID_PPV_ARGS(&_rootSignature)));
    }

    // 创建 PSO 同时编译并加载shader
    {
        ComPtr<ID3DBlob> vertexShader;
        ComPtr<ID3DBlob> pixelShader;
#if defined(_DEBUG)
        UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
        UINT compileFlags = 0;
#endif

        MINI_ENGINE_THROW(D3DCompileFromFile(
            GetAssetFullPath(L"/Shader/shaders04.hlsl").c_str(),
            nullptr,
            nullptr,
            "VSMain",
            "vs_5_0",
            compileFlags,
            0,
            &vertexShader,
            nullptr));
        MINI_ENGINE_THROW(D3DCompileFromFile(
            GetAssetFullPath(L"/Shader/shaders04.hlsl").c_str(),
            nullptr,
            nullptr,
            "PSMain",
            "ps_5_0",
            compileFlags,
            0,
            &pixelShader,
            nullptr));

        // 定义顶点布局
        D3D12_INPUT_ELEMENT_DESC inputElementDesc[] = {
            {"POSITION", 0 , DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"COLOR", 0 , DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        };
        // 描述并创建 PSO
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout = {inputElementDesc, _countof(inputElementDesc)};
        psoDesc.pRootSignature = _rootSignature.Get();
        psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
        psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState.DepthEnable = FALSE;
        psoDesc.DepthStencilState.StencilEnable = FALSE;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.SampleDesc.Count = 1;
        MINI_ENGINE_THROW(_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&_pipelineState)));
    }

    // 使用当前帧的分配器创建一个新的 CommandList
    MINI_ENGINE_THROW(_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
        _commandAllocator[_frameIndex].Get(), _pipelineState.Get(), IID_PPV_ARGS(&_commandList)));

    // 创建 commandList后, commandList 将会处在录制状态, 因此在现在关闭
    _commandList->Close();

    // 创建 vertex Buffer
    {
        Vertex triangleVertices[] = {
            {{0.0f, 0.25f * _aspectRatio, 0.0f},{1.0f,0.0f,0.0f,1.0f}},
            {{0.25f, -0.25f * _aspectRatio, 0.0f},{0.0f,1.0f,0.0f,1.0f}},
            {{-0.25f, -0.25f * _aspectRatio, 0.0f},{0.0f,0.0f,1.0f,1.0f}},
        };

        const UINT vertexBufferSize = sizeof(triangleVertices);

        // 注意：不推荐使用上传堆来传输静态数据，比如vert buffers。每次GPU需要它时，上传堆都会被编组。
        // 请阅读默认堆的使用。这里使用上传堆是为了代码简单，因为实际上要传输的vert非常少。
        CD3DX12_HEAP_PROPERTIES uploadProperties(D3D12_HEAP_TYPE_UPLOAD);
        CD3DX12_RESOURCE_DESC uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
        MINI_ENGINE_THROW(_device->CreateCommittedResource(
            &uploadProperties,
            D3D12_HEAP_FLAG_NONE,
            &uploadBufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&_vertexBuffer)));
        
        // 将顶点数据复制到 顶点缓冲区
        UINT* pVertexDataBegin;
        CD3DX12_RANGE readRange(0, 0); // 由于不打算从CPU读取顶点数据，因此不设置范围
        MINI_ENGINE_THROW(_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
        memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
        _vertexBuffer->Unmap(0, nullptr);

        // 初始化顶点缓冲区视图
        _vertexBufferView.BufferLocation = _vertexBuffer->GetGPUVirtualAddress();
        _vertexBufferView.StrideInBytes = sizeof(Vertex);
        _vertexBufferView.SizeInBytes = vertexBufferSize;
    }

    // 创建常量缓冲区
    {
        const UINT constantBufferSize = sizeof(SceneConstantBuffer);
        CD3DX12_HEAP_PROPERTIES cbvHeapProperties(D3D12_HEAP_TYPE_UPLOAD);
        CD3DX12_RESOURCE_DESC cbvBuffer = CD3DX12_RESOURCE_DESC::Buffer(constantBufferSize);
        
        MINI_ENGINE_THROW(_device->CreateCommittedResource(
            &cbvHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &cbvBuffer,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&_constantBuffer)));

        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
        cbvDesc.BufferLocation = _constantBuffer->GetGPUVirtualAddress();
        cbvDesc.SizeInBytes = constantBufferSize;
        _device->CreateConstantBufferView(&cbvDesc,_cbvHeap->GetCPUDescriptorHandleForHeapStart());

        // 映射并初始化常量缓冲区，该映射将在应用结束时释放
        CD3DX12_RANGE readRange(0, 0);
        MINI_ENGINE_THROW(_constantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&_pCbvDataBegin)));
        memcpy(_pCbvDataBegin, &_constantBufferData, sizeof(_constantBufferData));
    }
    

    // 创建同步对象并等待资源被上传至 GPU
    {
        MINI_ENGINE_THROW(_device->CreateFence(_fenceValue[_frameIndex], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence)));
        _fenceValue[_frameIndex]++;

        // 创建事件句柄并用于帧同步
        _fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if(_fenceEvent == nullptr)
        {
            MINI_ENGINE_THROW(HRESULT_FROM_WIN32(GetLastError()));
        }
        
        // 等待命令列表执行;我们在主循环中重用了相同的命令列表，但是现在，我们只想等待设置完成后再继续。
        WaitForGPU();
    }
}

void D3D12UseImGui::OnUpdate()
{
    const float translationSpeed = 0.005f;
    const float offsetBounds = 1.25f;

    _constantBufferData.offset.x += translationSpeed;
    if(_constantBufferData.offset.x > offsetBounds)
    {
        _constantBufferData.offset.x = -offsetBounds;
    }
    memcpy(_pCbvDataBegin, &_constantBufferData, sizeof(_constantBufferData));
}

void D3D12UseImGui::OnRender()
{
    // 将所有我们需要的渲染指令保存在渲染队列中
    PopulateCommandList();

    // 运行渲染队列
    ID3D12CommandList* ppCommandLists [] = {_commandList.Get()};
    _commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // 交换链交换
    MINI_ENGINE_THROW(_swapChain->Present(1, 0));

    MoveToNextFrame();
}

void D3D12UseImGui::OnDestroy()
{
    WaitForGPU();
    UIManager::getInstance().OnDestroy();
    CloseHandle(_fenceEvent);
}

void D3D12UseImGui::PopulateCommandList()
{
    // 命令列表分配器只能在相关的命令列表在GPU上完成执行时重置;应用程序应该使用栅栏来确定GPU的执行进度。
    MINI_ENGINE_THROW(_commandAllocator[_frameIndex]->Reset());
    
    // 但是，当ExecuteCommandList()在一个特定的命令列表上被调用时，该命令列表可以在任何时候被重置，并且必须在重新记录之前重置。
    MINI_ENGINE_THROW(_commandList->Reset(_commandAllocator[_frameIndex].Get(), _pipelineState.Get()));

    // 设定需要的状态
    _commandList->SetGraphicsRootSignature(_rootSignature.Get());

    ID3D12DescriptorHeap* ppHeaps[] = {_cbvHeap.Get()};
    _commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
    _commandList->SetGraphicsRootDescriptorTable(0, _cbvHeap->GetGPUDescriptorHandleForHeapStart());
    
    _commandList->RSSetViewports(1, &_viewport);
    _commandList->RSSetScissorRects(1, &_scissorRect);

    CD3DX12_RESOURCE_BARRIER preTransition = CD3DX12_RESOURCE_BARRIER::Transition(_renderTarget[_frameIndex].Get(),
        D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    
    _commandList->ResourceBarrier(1, &preTransition);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(_rtvHeap->GetCPUDescriptorHandleForHeapStart(), _frameIndex, _rtvDescriptorSize);

    // 设置呈现目标和深度模具的 CPU 描述符句柄
    _commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

    // 录制命令
    const float clearColor[] = {0.0f, 0.2f, 0.4f, 1.0f};
    _commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    _commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    _commandList->IASetVertexBuffers(0, 1, &_vertexBufferView);
    _commandList->DrawInstanced(3, 1, 0, 0);
    
    UIMangerRuntimeInfo runtime_info = UIMangerRuntimeInfo(_commandList);
    UIManager::getInstance().OnUpdate(runtime_info);
    
    CD3DX12_RESOURCE_BARRIER afterTransition = CD3DX12_RESOURCE_BARRIER::Transition(_renderTarget[_frameIndex].Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    
    _commandList->ResourceBarrier(1, &afterTransition);

    MINI_ENGINE_THROW(_commandList->Close());
}

void D3D12UseImGui::WaitForGPU()
{
    MINI_ENGINE_THROW(_commandQueue->Signal(_fence.Get(), _fenceValue[_frameIndex]));

    // 等待围栏值到达
    MINI_ENGINE_THROW(_fence->SetEventOnCompletion(_fenceValue[_frameIndex], _fenceEvent));
    WaitForSingleObjectEx(_fenceEvent, INFINITE, FALSE);

    // 递增当前帧的围栏值
    _fenceValue[_frameIndex] ++;
}

void D3D12UseImGui::MoveToNextFrame()
{
    const UINT64 currentFenceValue = _fenceValue[_frameIndex];
    MINI_ENGINE_THROW(_commandQueue->Signal(_fence.Get(), currentFenceValue));

    // 更新当前帧号
    _frameIndex = _swapChain->GetCurrentBackBufferIndex();

    // 当下一帧还没有准备好渲染，等待下一帧
    if(_fence->GetCompletedValue() < currentFenceValue)
    {
        MINI_ENGINE_THROW(_fence->SetEventOnCompletion(currentFenceValue, _fenceEvent));
        WaitForSingleObjectEx(_fenceEvent, INFINITE, FALSE);
    }

    // 更新下一帧的围栏值
    _fenceValue[_frameIndex] = currentFenceValue + 1;
}

