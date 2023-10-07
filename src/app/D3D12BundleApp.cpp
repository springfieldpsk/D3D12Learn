#include "stdafx.h"
#include "D3D12BundleApp.h"

D3D12Bundles::D3D12Bundles(UINT width, UINT height, std::wstring name):
    DXSample(width, height, name),
    _frameIndex(0),
    _viewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)),
    _scissorRect(0, 0, static_cast<LONG>(width), static_cast<LONG>(height)),
    _rtvDescriptorSize(0)
{
    
}


void D3D12Bundles::OnInit()
{
    LoadPipeline();
    LoadAssets();
}

void D3D12Bundles::LoadPipeline()
{
    
}

void D3D12Bundles::LoadAssets()
{
    
}

void D3D12Bundles::OnUpdate()
{
    
}

void D3D12Bundles::OnRender()
{
    
}

void D3D12Bundles::OnDestroy()
{
    
}

void D3D12Bundles::PopulateCommandList()
{
    
}

void D3D12Bundles::WaitForPreviousFrame()
{
    
}
