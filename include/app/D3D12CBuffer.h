﻿#pragma once

#include "DXSample.h"

using namespace DirectX;

using Microsoft::WRL::ComPtr;

class D3D12CBuffer : public DXSample
{
public:
    D3D12CBuffer(UINT width, UINT height, std::wstring name);

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

    struct SceneConstantBuffer
    {
        XMFLOAT4 offset;
        float padding[60]; // 填充内存保证一个buffer占用256位
    };
    static_assert((sizeof(SceneConstantBuffer) % 256) == 0,  "Constant Buffer size must be 256-byte aligned");
    
    // 管线需要的内容
    CD3DX12_VIEWPORT _viewport;
    CD3DX12_RECT _scissorRect;
    ComPtr<IDXGISwapChain3> _swapChain;
    ComPtr<ID3D12Device> _device;
    ComPtr<ID3D12Resource> _renderTarget[FrameCount];
    ComPtr<ID3D12CommandAllocator> _commandAllocator;
    ComPtr<ID3D12CommandQueue> _commandQueue;
    ComPtr<ID3D12RootSignature> _rootSignature;
    ComPtr<ID3D12DescriptorHeap> _rtvHeap;
    ComPtr<ID3D12DescriptorHeap> _cbvHeap;
    ComPtr<ID3D12PipelineState> _pipelineState;
    ComPtr<ID3D12GraphicsCommandList> _commandList;
    UINT _rtvDescriptorSize;

    // 应用资源
    ComPtr<ID3D12Resource> _vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW _vertexBufferView;
    ComPtr<ID3D12Resource> _constantBuffer;
    SceneConstantBuffer _constantBufferData;
    UINT8* _pCbvDataBegin;
    
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
