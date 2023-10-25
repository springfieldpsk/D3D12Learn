#pragma once

#include "stdafx.h"
#include "UISample.h"

using namespace Microsoft::WRL;

struct UIMangerCreateInfo
{
    UINT frameCnt;
    ComPtr<ID3D12Device> d3device;
    ComPtr<ID3D12DescriptorHeap> srvHeap;

    UIMangerCreateInfo(UINT _frameCnt, ComPtr<ID3D12Device> _device, ComPtr<ID3D12DescriptorHeap> _srcHeap) :
    frameCnt(_frameCnt),d3device(_device),srvHeap(_srcHeap)
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
    
    static UIManager& getInstance(){
        static UIManager inst;
        return inst;
    }
protected:
    
private:
    std::vector<ComPtr<UISample>> _uiArray;

};
