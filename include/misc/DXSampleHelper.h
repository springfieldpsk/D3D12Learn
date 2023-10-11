#pragma once
#include <stdexcept>
#include <filesystem>
#include <tchar.h>

namespace fs = std::filesystem;

inline void GetAssetsPath(_Out_writes_(pathSize) TCHAR* path, uint32_t pathSize){
    if(path == nullptr)
    {
        throw std::exception();
    }
    wcscpy_s(path, pathSize, fs::current_path().wstring().c_str());
    TCHAR* lastSlash = _tcsstr(path, _T("\\build"));
    if(lastSlash)
    {
        *(lastSlash) = _T('\0');
    }
}
