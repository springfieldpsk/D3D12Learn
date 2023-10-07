#pragma once
#include <intsafe.h>
#include <stdexcept>
#include <filesystem>
#include "DXSample.h"

namespace fs = std::filesystem;

inline void GetAssetsPath(_Out_writes_(pathSize) WCHAR* path, UINT pathSize){
    if(path == nullptr)
    {
        throw std::exception();
    }
    
    wcscpy_s(path, pathSize, fs::current_path().wstring().c_str());
}
