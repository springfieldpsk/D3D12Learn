#include "DXSample.h"
#include "stdafx.h"
#include "Win32Application.h"

using namespace Microsoft::WRL;

DXSample::DXSample(UINT width, UINT height, std::wstring name) :
    _width(width),
    _height(height),
    _useWarpDevice(false),
    _title(name)
{
    WCHAR assetsPaths[MAX_PATH];
    GetAssetsPath(assetsPaths, _countof(assetsPaths));
    _assetsPath = assetsPaths;

    _aspectRatio = static_cast<float>(width) / static_cast<float>(height);
}

DXSample::~DXSample()
{
    
}

std::wstring DXSample::GetAssetFullPath(LPCWSTR assetName)
{
    return _assetsPath + assetName;
}

// _Use_decl_annotations_ 用于标注函数或方法使用在其声明处的注解
// 该函数用于获取设备中首个支持Dx12的显示适配器，若无法找到合适的适配器，则返回nullptr
_Use_decl_annotations_
void DXSample::GetHardwardAdapter(IDXGIFactory1* pFactory, IDXGIAdapter1** ppAdapter, bool requestHighPerformanceAdapter)
{
    *ppAdapter = nullptr;

    ComPtr<IDXGIAdapter1> adapter;
    ComPtr<IDXGIFactory6> factory6;

    if(SUCCEEDED(pFactory->QueryInterface(IID_PPV_ARGS(&factory6))))
    {
        // 枚举适配器，根据是否使用高性能GPU来选择使用的适配器
        for(UINT adapterIndex = 0;
            SUCCEEDED(factory6->EnumAdapterByGpuPreference(
                adapterIndex,
                requestHighPerformanceAdapter == true ? DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE : DXGI_GPU_PREFERENCE_UNSPECIFIED,
                IID_PPV_ARGS(&adapter)));
            ++adapterIndex)
        {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);

            // 跳过软件适配器
            if(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                continue;
            }

            //检查适配器是否符合标准 12.1 (使用DXR) 返回参数设置为nullptr 以避免创建设备导致重复释放
            // _uuidof 检索附加到表达式的 GUID
            if(SUCCEEDED(D3D12CreateDevice(adapter.Get(), _useFeatureLevel, _uuidof(ID3D12Device), nullptr)))
            {
                break;
            }
        }
    }

    // 没有实现 factory6 的情况下退化为 factory1 特性
    if(adapter.Get() == nullptr)
    {
        for (UINT adapterIndex = 0; SUCCEEDED(pFactory->EnumAdapters1(adapterIndex, &adapter)); ++adapterIndex)
        {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                continue;
            }
            
            if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), _useFeatureLevel, _uuidof(ID3D12Device), nullptr)))
            {
                break;
            }
        }
    }

    // 返回适配器
    *ppAdapter = adapter.Detach();
}

void DXSample::SetCustomWindowText(LPCWSTR text)
{
    std::wstring windowText = _title + L":" + text;
    SetWindowText(Win32Application::GetHwnd(), windowText.c_str());
}

_Use_decl_annotations_
void DXSample::ParseCommandLineArgs(WCHAR* argv[], int argc)
{
    for(int i = 1; i < argc; ++i )
    {
        // wcslen 用于计算宽字符串的长度
        // _wcsnicmp 用于对两个宽字符字符串进行大小写不敏感的比较
        if(_wcsnicmp(argv[i], L"-warp", wcslen(argv[i])) == 0 ||
           _wcsnicmp(argv[i], L"/warp", wcslen(argv[i])) == 0 )
        {
            _useWarpDevice = true;
            _title = _title + L": (WARP)";
        }
    }
}






