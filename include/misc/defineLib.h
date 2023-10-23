#pragma once

constexpr const std::wstring_view MINI_ENGINE_WND_CLASS_NAME = L"Test Dx12 Class";
constexpr const std::wstring_view MINI_ENGINE_WND_TITLE = L"Test Dx12 Window";

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

inline void MINI_ENGINE_THROW(HRESULT hr)
{
    if (FAILED(hr))
    {
        throw MiniEngineExpression(hr);
    }
}// 抛出异常宏
