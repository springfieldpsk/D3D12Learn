#pragma once
#include "stdafx.h"

class DXSample
{
public:
    DXSample(UINT width, UINT height, std::wstring name);
    virtual ~DXSample();

    virtual void OnInit() = 0;
    virtual void OnUpdate() = 0;
    virtual void OnRender() = 0;
    virtual void OnDestroy() = 0;

    virtual void OnKeyDown(UINT8 /*key*/) {}
    virtual void OnKeyUp(UINT8 /*key*/) {}

    UINT GetWidth() const           {return _width;}
    UINT GetHeight() const          {return _height;}
    const WCHAR* GetTitle() const   {return _title.c_str();}

    // _In_reads_(argc) 用于 Microsoft的静态分析工具 表明数组需要接受一个大小至少为argc的字符指针数组
    void ParseCommandLineArgs(_In_reads_(argc) WCHAR* argv[], int argc);
    
protected:
    std::wstring GetAssetFullPath(LPCWSTR assetName);

    // _In_宏通常用来标识一个函数参数是只读的（输入参数）
    // _Outptr_result_maybenull_ 宏表示函数可能返回一个空指针，或者函数可能将一个参数设置为空指针
    void GetHardwardAdapter(
        _In_ IDXGIFactory1* pFactory,
        _Outptr_result_maybenull_ IDXGIAdapter1** ppAdapter,
        bool requestHighPerformanceAdapter = false);

    void SetCustomWindowText(LPCWSTR text);
    
    // 视口参数
    UINT _width;
    UINT _height;
    float _aspectRatio;

    // 适配器参数
    bool _useWarpDevice;
    const static D3D_FEATURE_LEVEL _useFeatureLevel = D3D_FEATURE_LEVEL_12_0;
    
private:
    std::wstring _title; // 窗口标题
    std::wstring _assetsPath;


};
