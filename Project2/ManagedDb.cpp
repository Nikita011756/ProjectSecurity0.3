// ManagedDb.cpp
#include "ManagedDb.h"

#using <System.dll>
#using <System.Data.dll>

#include <msclr/marshal_cppstd.h>
#include <shlwapi.h>     // Для PathCombine
#include <atlbase.h>
#include <openssl/sha.h>
#include <openssl/md5.h>
#include <sstream>
#include <iomanip>
#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "Ole32.lib")


using namespace System;
using namespace System::Data;
using namespace System::Data::OleDb;
using namespace System::IO;

using namespace msclr::interop;


// Функція хешування паролю через SHA256 (unmanaged C++ логіка)
std::string HashPasswordMixed(const std::string& password)
{
    const std::string SALT = "SALT1234";

    // MD5
    unsigned char md5Hash[MD5_DIGEST_LENGTH];
    MD5((const unsigned char*)password.c_str(), password.length(), md5Hash);

    std::stringstream md5Stream;
    for (int i = 0; i < MD5_DIGEST_LENGTH; ++i)
        md5Stream << std::hex << std::setw(2) << std::setfill('0') << (int)md5Hash[i];
    std::string md5Hex = md5Stream.str();

    // SHA256
    unsigned char sha256Hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, password.c_str(), password.size());
    SHA256_Final(sha256Hash, &sha256);

    std::stringstream shaStream;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i)
        shaStream << std::hex << std::setw(2) << std::setfill('0') << (int)sha256Hash[i];
    std::string shaHex = shaStream.str();

    // Combine and hash again
    std::string combined = md5Hex + SALT + shaHex;

    unsigned char finalHash[SHA256_DIGEST_LENGTH];
    SHA256_CTX finalSha;
    SHA256_Init(&finalSha);
    SHA256_Update(&finalSha, combined.c_str(), combined.size());
    SHA256_Final(finalHash, &finalSha);

    std::stringstream finalStream;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i)
        finalStream << std::hex << std::setw(2) << std::setfill('0') << (int)finalHash[i];

    return finalStream.str();
}
std::wstring GetDatabasePath() {
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);

    // Удаляем имя exe
    PathRemoveFileSpecW(exePath);

    // Добавляем "Password.accdb" к пути
    wchar_t dbPath[MAX_PATH];
    PathCombineW(dbPath, exePath, L"Password.accdb");

    return std::wstring(dbPath);
}
// Основна функція — збереження хешу пароля в базу Access
int SavePasswordToDb(const std::wstring& filePath, const std::wstring& password)
{
    // Отримати ім’я файлу без шляху
    std::wstring originalFileName = filePath.substr(filePath.find_last_of(L"\\/") + 1);
    std::wstring lockedFileName = originalFileName + L".locked";  // Використаємо тільки після перевірки

    // Підготовка хешу паролю
    std::string nativePassword(password.begin(), password.end());
    std::string hash = HashPasswordMixed(nativePassword);

    String^ sqlFileNameOrinal = gcnew String(originalFileName.c_str());
    String^ sqlFileName = gcnew String(lockedFileName.c_str()); // тільки для бази
    String^ sqlHash = gcnew String(hash.c_str());

    // Підготовка підключення до бази
    std::wstring dbPath = GetDatabasePath();
    System::String^ connStr = gcnew System::String((L"Provider=Microsoft.ACE.OLEDB.16.0;Data Source=" + dbPath).c_str());

    String^ checkQuery = "SELECT COUNT(*) FROM Passwords WHERE FileName = ?";
    String^ insertQuery = "INSERT INTO Passwords (FileName, PasswordHash) VALUES (?, ?)";

    OleDbConnection^ conn = gcnew OleDbConnection(connStr);

    try {
        conn->Open();

        // Перевірка, чи файл вже зашифрований
        OleDbCommand^ checkCmd = gcnew OleDbCommand(checkQuery, conn);
        checkCmd->Parameters->AddWithValue("?", sqlFileNameOrinal);
        int count = Convert::ToInt32(checkCmd->ExecuteScalar());
      
        if (count > 0) {
            MessageBoxW(NULL, L"Файл уже зашифрований. Повторне збереження заблоковано.", L"Увага", MB_ICONWARNING);
            conn->Close();
            return 1;
        }

        // Тепер виконуємо вставку
        OleDbCommand^ insertCmd = gcnew OleDbCommand(insertQuery, conn);
        insertCmd->Parameters->AddWithValue("?", sqlFileName);
        insertCmd->Parameters->AddWithValue("?", sqlHash);

        int result = insertCmd->ExecuteNonQuery();
        if (result < 1) {
            MessageBoxW(NULL, L"Помилка збереження: запис не додано.", L"Збереження пароля", MB_ICONERROR);
        }
        else {
            std::wstring message = L"Пароль успішно збережено:\nФайл: " + lockedFileName + L"\nХеш: " + std::wstring(hash.begin(), hash.end());
            MessageBoxW(NULL, message.c_str(), L"Збереження в базу", MB_OK | MB_ICONINFORMATION);
        }
    }
    catch (Exception^ ex) {
        std::wstring errorMessage = L"Помилка доступу до бази:\n" + marshal_as<std::wstring>(ex->Message);
        MessageBoxW(NULL, errorMessage.c_str(), L"База даних", MB_ICONERROR);
    }
    finally {
        if (conn->State == ConnectionState::Open)
            conn->Close();
    }

    return 0;
}


#include <Windows.h>
#include <msclr/marshal_cppstd.h>
using namespace msclr::interop;

bool VerifyPasswordForFile(const std::wstring& filePath, const std::wstring& password)
{
    try {
        // Отримуємо ім’я файлу без шляху
        std::wstring fileName = filePath.substr(filePath.find_last_of(L"\\/") + 1);

        // Конвертуємо в System::String^
        System::String^ fileNameManaged = gcnew System::String(fileName.c_str());
        std::wstring dbPath = GetDatabasePath();
        System::String^ connStr = gcnew System::String((L"Provider=Microsoft.ACE.OLEDB.16.0;Data Source=" + dbPath).c_str());
       

        // SQL-запит до бази
        System::String^ query = "SELECT PasswordHash FROM Passwords WHERE FileName = ?";
        System::Data::OleDb::OleDbConnection^ conn = gcnew System::Data::OleDb::OleDbConnection(connStr);
        System::Data::OleDb::OleDbCommand^ cmd = gcnew System::Data::OleDb::OleDbCommand(query, conn);
        cmd->Parameters->AddWithValue("?", fileNameManaged);

        conn->Open();
        System::Data::OleDb::OleDbDataReader^ reader = cmd->ExecuteReader();

        if (!reader->Read()) {
            conn->Close();
            MessageBoxW(NULL, L"Для цього файлу не знайдено запис паролю у базі.", L"Увага", MB_ICONWARNING);
            return false;
        }

        // Отримуємо хеш з бази
        System::String^ dbHashed = reader->GetString(0);
        std::string dbHash = marshal_as<std::string>(dbHashed);
        std::wstring wideInputStr(dbHash.begin(), dbHash.end());
       
        conn->Close();

        // Обчислюємо хеш введеного пароля
        std::string inputStr(password.begin(), password.end());
        std::string inputHash = HashPasswordMixed(inputStr);
        

        if (inputHash == dbHash) {
            MessageBoxW(NULL, L"Пароль вірний. Доступ дозволено.", L"Успіх", MB_ICONINFORMATION);

            // Видаляємо запис з бази
            System::String^ deleteQuery = "DELETE FROM Passwords WHERE FileName = ?";
            System::Data::OleDb::OleDbCommand^ deleteCmd = gcnew System::Data::OleDb::OleDbCommand(deleteQuery, conn);
            deleteCmd->Parameters->AddWithValue("?", fileNameManaged);

            conn->Open();
            int rowsAffected = deleteCmd->ExecuteNonQuery();
            conn->Close();

            if (rowsAffected < 1) {
                MessageBoxW(NULL, L"Не вдалося видалити запис з бази після перевірки.", L"Попередження", MB_ICONWARNING);
            }

            return true;
        }
        else {
            MessageBoxW(NULL, L"Невірний пароль. Спробуйте ще раз.", L"Помилка", MB_ICONERROR);
            return false;
        }
    }
    catch (System::Exception^ ex) {
        std::wstring message = L"Помилка під час перевірки пароля:\n" + marshal_as<std::wstring>(ex->Message);
        MessageBoxW(NULL, message.c_str(), L"Помилка", MB_ICONERROR);
        return false;
    }
}
