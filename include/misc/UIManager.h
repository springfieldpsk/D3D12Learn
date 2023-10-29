#pragma once

#include "stdafx.h"
#include <functional>

using namespace Microsoft::WRL;

struct UIMangerCreateInfo
{
    UINT frameCnt;
    ComPtr<ID3D12Device> d3device;

    UIMangerCreateInfo(UINT _frameCnt, ComPtr<ID3D12Device> _device) :
    frameCnt(_frameCnt),d3device(_device)
    {
        
    }
};

struct UIMangerRuntimeInfo
{
    ComPtr<ID3D12GraphicsCommandList> useCommandList;

    UIMangerRuntimeInfo(ComPtr<ID3D12GraphicsCommandList> _useCommandList):useCommandList(_useCommandList)
    {
        
    }
};

class UIManager
{
public:
    UIManager(){}
    virtual ~UIManager(){}

    virtual void OnInit(UIMangerCreateInfo createInfo);
    virtual void OnUpdate(UIMangerRuntimeInfo runtimeInfo);
    virtual void OnDestroy();

    LRESULT UIProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    
    static UIManager& getInstance(){
        static UIManager inst;
        return inst;
    }
private:
    std::vector<std::function<void()> > _uiArray;
    ComPtr<ID3D12DescriptorHeap> _srvHeap;
};
