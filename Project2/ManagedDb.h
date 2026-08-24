#pragma once

#include <string>

// Оголошення керованої функції для WinAPI-коду
int SavePasswordToDb(const std::wstring& filePath, const std::wstring& password);
bool VerifyPasswordForFile(const std::wstring& filePath, const std::wstring& password);