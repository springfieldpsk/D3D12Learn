#pragma once

#include "DXSample.h"

using namespace DirectX;

// ComPtr 被用来管理CPU资源的生命周期时，对CPU资源关联的GPU资源是不敏感的，注意要避免破坏可能依旧被GPU使用的对象
using Microsoft::WRL::ComPtr;

class D3D12Bundles : public DXSample
{
public:
    D3D12Bundles(UINT width, UINT height, std::wstring name);

    virtual void OnInit();
    virtual void OnUpdate();
    virtual void OnRender();
    virtual void OnDestroy();

private:
    static const UINT FrameCount = 2;

    struct Vertex
    {
        XMFLOAT3 position;
        XMFLOAT4 color;
    };

    // 管线需要的内容
    CD3DX12_VIEWPORT _viewport;
    CD3DX12_RECT _scissorRect;
    ComPtr<IDXGISwapChain3> _swapChain;
    ComPtr<ID3D12Device> _device;
    ComPtr<ID3D12Resource> _renderTarget[FrameCount];
    ComPtr<ID3D12CommandAllocator> _commandAllocator;
    ComPtr<ID3D12CommandAllocator> _bundleAllocator;
    ComPtr<ID3D12CommandQueue> _commandQueue;
    ComPtr<ID3D12RootSignature> _rootSignature;
    ComPtr<ID3D12DescriptorHeap> _rtvHeap;
    ComPtr<ID3D12GraphicsCommandList> _commandList;
    ComPtr<ID3D12GraphicsCommandList> _bundle;
    UINT _rtvDescriptorSize;

    // 应用资源
    ComPtr<ID3D12Resource> _vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW _vertexBufferView;

    // 同步对象
    UINT _frameIndex;
    HANDLE _fenceEvent;
    ComPtr<ID3D12Fence> _fence;
    UINT64 _fenceValue;

    void LoadPipeline();
    void LoadAssets();
    void PopulateCommandList();
    void WaitForPreviousFrame();
};
