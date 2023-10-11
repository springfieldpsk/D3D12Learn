#pragma once

#define MINI_ENGINE_WND_CLASS_NAME L"Test Dx12 Class"
#define MINI_ENGINE_WND_TITLE L"Test Dx12 Window"

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