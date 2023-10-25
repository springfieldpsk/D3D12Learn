#pragma once

#include "stdafx.h"

class UISample
{
public:
    UISample(std::wstring name);
    virtual ~UISample();

    virtual void OnUpdate() = 0;
    
    const WCHAR* GetTitle() const   {return _title.c_str();}

protected:
    
private:
    std::wstring _title; // 窗口标题
};
