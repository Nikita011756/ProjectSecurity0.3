#include <Windows.h>
#include <string>
#include <iostream>
#include"ManagedDb.h"
#include <fstream> 
#include "resource.h"
#include "SoftwareDefinitions.h"
#include <cryptopp/aes.h>
#include <cryptopp/modes.h>
#include <cryptopp/filters.h>
#include <random>
#pragma comment(lib, "Gdi32.lib")
#pragma comment(lib, "Comdlg32.lib")


using namespace CryptoPP;
namespace fs = std::filesystem;

LRESULT CALLBACK SoftwareMainProcedure(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);
WNDCLASS NewWindowClass(HBRUSH BGColor, HCURSOR Cursor, HINSTANCE hInst, HICON Icon, LPCWSTR Name, WNDPROC Procedure);

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR args, int ncmdshow) {
	fontRectangle = CreateFontA(
		30, 10, 0, 0, FW_MEDIUM,
		FALSE, FALSE, FALSE, DEFAULT_CHARSET,
		OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
		FF_DECORATIVE, "MyFont"
	);

	WNDCLASS SoftWareMainClass = NewWindowClass((HBRUSH)COLOR_WINDOW, LoadCursor(NULL, IDC_ARROW),
		hInst, LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON1)), L"MainWndClass",
		SoftwareMainProcedure);

	if (!RegisterClassW(&SoftWareMainClass)) { return -1; }
	MSG SoftwareMainMessage = { 0 };

	CreateWindow(L"MainWndClass", L"Encrypt file and decrypt file ", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		100, 100, 750, 400, NULL, NULL, NULL, NULL);
	while (GetMessage(&SoftwareMainMessage, NULL, NULL, NULL)) {
		TranslateMessage(&SoftwareMainMessage);
		DispatchMessage(&SoftwareMainMessage);
	}
	TerminateThread(readThread, 0);
	return 0;


}

WNDCLASS NewWindowClass(HBRUSH BGColor, HCURSOR Cursor, HINSTANCE hInst,
	HICON Icon, LPCWSTR Name, WNDPROC Procedure) {
	WNDCLASS NWC = { 0 };

	NWC.hCursor = Cursor;
	NWC.hIcon = Icon;
	NWC.hInstance = hInst;
	NWC.lpszClassName = Name;
	NWC.hbrBackground = BGColor;
	NWC.lpfnWndProc = Procedure;

	return NWC;
}

void ExitSoftware(void) {
	isConnected = false;
	isThreading = false;
	CloseHandle(connectedPort);
	CloseHandle(readThread);
	PostQuitMessage(0);
}

LRESULT CALLBACK SoftwareMainProcedure(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp) {

	switch (msg) {
	case WM_COMMAND:

		switch (wp)
		{
		case OnLockedFile:
			OnLockFile(hWnd, hEditControl);
			break;
		case OnUnlockFile:
			UnlockFile(hWnd, hEditControl);
			break;
		case OnClearField:
			SetWindowTextA(hEditControl, "");
			break;
		case OnMenuClearField:
			SetWindowTextA(hEditControl, "");
			break;
		case OnExitSoftware:
			PostQuitMessage(0);
			break;
		case ID_BUTTON_GENERATE:
		{
			OnGeneratePassword(hWnd);
			break;
		}
		default: break;
		}
		break;

	case WM_CREATE:
		MainWndAddMenus(hWnd);
		MainWndAddWidgets(hWnd);
		SetOpenFileParams(hWnd);
		SendMessageA(hStaticControl, WM_SETFONT, (WPARAM)fontRectangle, TRUE);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default: return DefWindowProc(hWnd, msg, wp, lp);
		return 0;
	}
}
void OnGeneratePassword(HWND hWnd) {
	wchar_t lenBuffer[10];
	GetWindowTextW(GetDlgItem(hWnd, ID_EDIT_LENGTH), lenBuffer, 10);
	int length = _wtoi(lenBuffer);

	if (length <= 0) {
		MessageBoxW(hWnd, L"Введите положительную длину пароля.", L"Ошибка", MB_ICONERROR);
		return;
	}

	bool useLower = (SendMessage(hLowerChk, BM_GETCHECK, 0, 0) == BST_CHECKED);
	bool useUpper = (SendMessage(hUpperChk, BM_GETCHECK, 0, 0) == BST_CHECKED);
	bool useDigits = (SendMessage(hDigitsChk, BM_GETCHECK, 0, 0) == BST_CHECKED);
	bool useSpecial = (SendMessage(hSpecialChk, BM_GETCHECK, 0, 0) == BST_CHECKED);

	if (!useLower && !useUpper && !useDigits && !useSpecial) {
		MessageBoxW(hWnd, L"Выберите хотя бы один тип символов.", L"Ошибка", MB_ICONERROR);
		return;
	}

	generatePassword(length, useLower, useUpper, useDigits, useSpecial);
	
}
void MainWndAddMenus(HWND hWnd) {
	HMENU RootMenu = CreateMenu();
	HMENU SubMenu = CreateMenu();
	HMENU SubActionMenu = CreateMenu();

	ComPortSubMenu = CreateMenu();
	ComPortListMenu = CreateMenu();

	AppendMenu(SubMenu, MF_STRING, OnMenuClearField, L"Clear");
	AppendMenu(SubMenu, MF_SEPARATOR, NULL, NULL);
	AppendMenu(SubMenu, MF_STRING, OnExitSoftware, L"Exit");


	AppendMenu(RootMenu, MF_POPUP, (UINT_PTR)SubMenu, L"File");
	AppendMenu(RootMenu, MF_STRING, (UINT_PTR)SubMenu, L"Help");


	SetMenu(hWnd, RootMenu);
}

void MainWndAddWidgets(HWND hWnd) {
	hStaticControl = CreateWindowA("Static", "Пароль для входу", WS_VISIBLE | WS_CHILD | ES_CENTER, 300, 50, 120, 30, hWnd, NULL, NULL, NULL);
	hEditControl = CreateWindowA("edit", "", WS_VISIBLE | WS_CHILD | ES_MULTILINE, 225, 75, 250, 20, hWnd, NULL, NULL, NULL);
	windowRectangle = { 5 + 470, 40, 5, 40 + 120 };

	// Метка "Длина пароля"
	CreateWindowW(L"STATIC", L"Длина пароля:", WS_VISIBLE | WS_CHILD,
		290, 250, 100, 20, hWnd, NULL, NULL, NULL);

	// Чекбоксы
	hLowerChk = CreateWindowW(L"BUTTON", L"Рядкові літери", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
		150, 220, 150, 20, hWnd, (HMENU)ID_CHECK_LOWER, NULL, NULL);

	hUpperChk = CreateWindowW(L"BUTTON", L"Великі літери", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
		290, 220, 150, 20, hWnd, (HMENU)ID_CHECK_UPPER, NULL, NULL);

	hDigitsChk = CreateWindowW(L"BUTTON", L"Цифри", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
		440, 220, 100, 20, hWnd, (HMENU)ID_CHECK_DIGITS, NULL, NULL);

	hSpecialChk = CreateWindowW(L"BUTTON", L"Символи", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
		520, 220, 100, 20, hWnd, (HMENU)ID_CHECK_SPECIAL, NULL, NULL);

	// Поле для ввода длины
	CreateWindowW(L"EDIT", L"12", WS_VISIBLE | WS_CHILD | WS_BORDER,
		390, 250, 50, 20, hWnd, (HMENU)ID_EDIT_LENGTH, NULL, NULL);

	// Кнопка генерации
	CreateWindowW(L"BUTTON", L"Згенерувати", WS_VISIBLE | WS_CHILD,
		280, 280, 160, 30, hWnd, (HMENU)ID_BUTTON_GENERATE, NULL, NULL);


	CreateWindowA("button", "Очистити", WS_VISIBLE | WS_CHILD | ES_CENTER, 290, 150, 120, 30, hWnd, (HMENU)OnClearField, NULL, NULL);
	//CreateWindowA("button", "Encrypt", WS_VISIBLE | WS_CHILD | ES_CENTER, 195, 5, 60, 30, hWnd, (HMENU)OnEncrypt, NULL, NULL);
	//CreateWindowA("button", "Dencrypt", WS_VISIBLE | WS_CHILD | ES_CENTER, 260, 5, 60, 30, hWnd, (HMENU)OnDencrypt, NULL, NULL);
	CreateWindowA("button", "Зашифрувати", WS_VISIBLE | WS_CHILD | ES_CENTER, 230, 100, 110, 30, hWnd, (HMENU)OnLockedFile, NULL, NULL);
	CreateWindowA("button", "Розшифрувати", WS_VISIBLE | WS_CHILD | ES_CENTER, 370, 100, 110, 30, hWnd, (HMENU)OnUnlockFile, NULL, NULL);
}
void SaveData(LPCSTR path) {
	HANDLE FileToSave = CreateFileA(
		path,
		GENERIC_WRITE,
		0,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	int saveLenth = GetWindowTextLength(hEditControl) + 1;
	char* data = new char[saveLenth];

	saveLenth = GetWindowTextA(hEditControl, data, saveLenth);

	DWORD bytesIterated;
	WriteFile(FileToSave, data, saveLenth, &bytesIterated, NULL);

	CloseHandle(FileToSave);
	delete[] data;
}



// Шифрование файла с использованием Windows API
void encryptFile(const std::wstring& inputFile, const std::wstring& outputFile)
{
	// AES ключ і IV (ініціалізаційний вектор)
	byte key[AES::DEFAULT_KEYLENGTH] = { 0x00 }; // Можна згенерувати на основі паролю або задати статично
	byte iv[AES::BLOCKSIZE] = { 0x00 };

	// Відкриваємо вхідний файл
	std::ifstream inFile(inputFile, std::ios::binary);
	if (!inFile.is_open()) {
		throw std::runtime_error("Не вдалося відкрити файл для читання");
	}

	// Читаємо вхідні дані в буфер
	std::vector<byte> buffer((std::istreambuf_iterator<char>(inFile)),
		std::istreambuf_iterator<char>());
	inFile.close();

	// Буфер для зашифрованих даних
	std::string encryptedData;

	// AES CBC режим
	CBC_Mode<AES>::Encryption encryption;
	encryption.SetKeyWithIV(key, sizeof(key), iv);

	// Шифрування
	StringSource ss(buffer.data(), buffer.size(), true,
		new StreamTransformationFilter(encryption,
			new StringSink(encryptedData)
		)
	);

	// Запис у вихідний файл
	std::ofstream outFile(outputFile, std::ios::binary);
	if (!outFile.is_open()) {
		throw std::runtime_error("Не вдалося відкрити файл для запису");
	}

	outFile.write(encryptedData.c_str(), encryptedData.size());
	outFile.close();
}

void EncryptPassword(const std::wstring& password, const std::wstring& filePath) {
	std::ofstream outFile(filePath, std::ios::out | std::ios::binary);

	if (!outFile.is_open()) {
		throw std::runtime_error("Не удалось открыть файл для записи.");
	}

	std::wstring alphabet = L"abcdefghijklmnopqrstuvwxyz";
	for (wchar_t buffer : password) {
		bool isEncrypted = false;
		for (int i = 0; i < alphabet.length(); ++i) {
			if (towlower(buffer) == alphabet[i]) {
				wchar_t newChar = alphabet[(i + 3) % 26];  // Сдвиг на 3 символа вперед
				if (iswupper(buffer)) {
					newChar = towupper(newChar);  // Учитываем регистр
				}
				outFile.put(newChar);
				isEncrypted = true;
				break;
			}
		}
		if (!isEncrypted) {
			outFile.put(buffer);  // Если символ не найден в алфавите, записываем его без изменений
		}
	}

	outFile.close();
}

void DecryptPassword(const std::wstring& filePath, std::wstring& decryptedPassword) {
	std::wifstream inFile(filePath, std::ios::in | std::ios::binary);

	if (!inFile.is_open()) {
		throw std::runtime_error("Не удалось открыть файл для чтения.");
	}

	std::wstring alphabet = L"abcdefghijklmnopqrstuvwxyz";
	wchar_t buffer;

	while (inFile.get(buffer)) {
		bool isDecrypted = false;
		for (int i = 0; i < alphabet.length(); ++i) {
			if (towlower(buffer) == alphabet[i]) {
				wchar_t newChar = alphabet[(i - 3) % 26];  // Сдвиг на 3 символа назад
				if (iswupper(buffer)) {
					newChar = towupper(newChar);  // Учитываем регистр
				}
				decryptedPassword.push_back(newChar);
				isDecrypted = true;
				break;
			}
		}
		if (!isDecrypted) {
			decryptedPassword.push_back(buffer);  // Если символ не найден в алфавите, добавляем его без изменений
		}
	}

	inFile.close();

}



/*void LoadPassword(const std::wstring& filePath, std::wstring& password) {
	// Убираем суффикс ".locked" из имени файла, если он есть
	std::wstring passwordFilePath = filePath;

	size_t lockedPos = passwordFilePath.rfind(L".locked");
	if (lockedPos != std::wstring::npos) {
		passwordFilePath = passwordFilePath.substr(0, lockedPos);  // Оставляем имя без ".locked"
	}

	passwordFilePath += L".password";  // Формируем имя файла с паролем, добавляем ".password"

	DecryptPassword(passwordFilePath, password);  // Читаем и расшифровываем из файла
}*/
void generatePassword(int length, bool useLower, bool useUpper, bool useDigits, bool useSpecial) {
	std::string chars;
	if (useLower)   chars += "abcdefghijklmnopqrstuvwxyz";
	if (useUpper)   chars += "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	if (useDigits)  chars += "0123456789";
	if (useSpecial) chars += "!@#$%^&*()-_=+[]{}|;:,.<>?";

	std::string nope = " ";
	if (chars.empty()) SetWindowTextA(hEditControl, nope.c_str());;

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dist(0, chars.size() - 1);

	std::string password;
	for (int i = 0; i < length; ++i) {
		password += chars[dist(gen)];
	}

	SetWindowTextA(hEditControl, password.c_str());
}
void lockFileWithPassword(const std::wstring& filePath, const std::wstring& password) {
	if (GetFileAttributesW(filePath.c_str()) == INVALID_FILE_ATTRIBUTES) {
		throw std::runtime_error("Исходный файл не найден");
	}

	std::wstring encryptedFile = filePath + L".locked";
	if (GetFileAttributesW(encryptedFile.c_str()) != INVALID_FILE_ATTRIBUTES) {
		throw std::runtime_error("Зашифрованный файл уже существует");
	}

	try {
		encryptFile(filePath, encryptedFile); // Шифруем файл

		if (!DeleteFileW(filePath.c_str())) { // Удаляем исходный файл после шифрования
			throw std::runtime_error("Не удалось удалить исходный файл");
		}

		MessageBoxW(NULL, (L"Файл успешно заблокирован. Результат: " + encryptedFile).c_str(), L"Успех", MB_ICONINFORMATION);
	}
	catch (const std::exception& e) {
		if (GetFileAttributesW(encryptedFile.c_str()) != INVALID_FILE_ATTRIBUTES) {
			DeleteFileW(encryptedFile.c_str()); // Убираем зашифрованный файл в случае ошибки
		}
		throw; // Перекидываем исключение дальше
	}
}


// Блокировка файла с использованием Windows API

// Обработчик кнопки "Заблокировать файл" с использованием Windows API
void OnLockFile(HWND hWnd, HWND hEditControl) {
	OPENFILENAMEW ofn;
	wchar_t filePath[MAX_PATH] = { 0 };

	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFilter = L"All Files\0*.*\0";
	ofn.lpstrFile = filePath;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	if (GetOpenFileNameW(&ofn)) {
		wchar_t password[256];
		GetWindowTextW(hEditControl, password, sizeof(password) / sizeof(password[0]));

		if (wcslen(password) == 0) {
			MessageBoxW(hWnd, L"Пароль не может быть пустым.", L"Ошибка", MB_ICONERROR);
			return;
		}

		try {
			if (SavePasswordToDb(filePath, password) == 1) {
				MessageBoxW(hWnd, L"Файл уже зашифрований.", L"Ошибка", MB_ICONERROR);
				return;
			}
			  // Сохраняем зашифрованный пароль
		 lockFileWithPassword(filePath, password);  // Блокируем файл с паролем
			MessageBoxW(hWnd, L"Файл успешно заблокирован.", L"Информация", MB_ICONINFORMATION);
		}
		catch (const std::exception& e) {
			std::wstring errorMessage = L"Ошибка: " + std::wstring(e.what(), e.what() + strlen(e.what()));
			MessageBoxW(hWnd, errorMessage.c_str(), L"Ошибка блокировки файла", MB_ICONERROR);
		}
	}
	else {
		MessageBoxW(hWnd, L"Выбор файла отменён.", L"Информация", MB_ICONINFORMATION);
	}
}
void decryptFile(const std::wstring& inputFile, const std::wstring& outputFile)
{
	// AES ключ і IV — повинні збігатися з тими, що використовувалися для шифрування
	byte key[AES::DEFAULT_KEYLENGTH] = { 0x00 }; // Замінити або згенерувати з пароля
	byte iv[AES::BLOCKSIZE] = { 0x00 };

	// Відкриваємо зашифрований файл
	std::ifstream inFile(inputFile, std::ios::binary);
	if (!inFile.is_open()) {
		throw std::runtime_error("Не вдалося відкрити файл для читання");
	}

	// Читаємо вхідні зашифровані дані
	std::vector<byte> encryptedBuffer((std::istreambuf_iterator<char>(inFile)),
		std::istreambuf_iterator<char>());
	inFile.close();

	// Буфер для розшифрованих даних
	std::string decryptedData;

	// AES CBC режим
	CBC_Mode<AES>::Decryption decryption;
	decryption.SetKeyWithIV(key, sizeof(key), iv);

	// Розшифрування
	try {
		StringSource ss(encryptedBuffer.data(), encryptedBuffer.size(), true,
			new StreamTransformationFilter(decryption,
				new StringSink(decryptedData)
			)
		);
	}
	catch (const Exception& e) {
		throw std::runtime_error(std::string("Помилка розшифрування: ") + e.what());
	}

	// Запис розшифрованого виводу
	std::ofstream outFile(outputFile, std::ios::binary);
	if (!outFile.is_open()) {
		throw std::runtime_error("Не вдалося відкрити файл для запису");
	}

	outFile.write(decryptedData.c_str(), decryptedData.size());
	outFile.close();
}
void unlockFileWithPassword(const std::wstring& filePath, const std::wstring& password) {
	// Проверяем существует ли зашифрованный файл
	if (GetFileAttributesW(filePath.c_str()) == INVALID_FILE_ATTRIBUTES) {
		throw std::runtime_error("Зашифрованный файл не найден");
	}

	std::wstring decryptedFile = filePath.substr(0, filePath.find_last_of(L'.'));
	if (GetFileAttributesW(decryptedFile.c_str()) != INVALID_FILE_ATTRIBUTES) {
		throw std::runtime_error("Файл уже был разблокирован");
	}

	try {
		// Пытаемся расшифровать файл
		decryptFile(filePath, decryptedFile);

		if (!DeleteFileW(filePath.c_str())) { // Удаляем зашифрованный файл после расшифровки
			throw std::runtime_error("Не удалось удалить зашифрованный файл");
		}

		// Удаляем файл с паролем
		std::wstring passwordFilePath = decryptedFile + L".password";
		if (GetFileAttributesW(passwordFilePath.c_str()) != INVALID_FILE_ATTRIBUTES) {
			if (!DeleteFileW(passwordFilePath.c_str())) {
				throw std::runtime_error("Не удалось удалить файл с паролем");
			}
		}

		MessageBoxW(NULL, (L"Файл успешно разблокирован. Результат: " + decryptedFile).c_str(), L"Успех", MB_ICONINFORMATION);
	}
	catch (const std::exception& e) {
		if (GetFileAttributesW(decryptedFile.c_str()) != INVALID_FILE_ATTRIBUTES) {
			DeleteFileW(decryptedFile.c_str()); // Убираем расшифрованный файл в случае ошибки
		}
		throw; // Перекидываем исключение дальше
	}
}
void UnlockFile(HWND hWnd, HWND hEditControl) {
	setlocale(LC_ALL, "");
	OPENFILENAMEW ofn;
	wchar_t filePath[MAX_PATH] = { 0 };

	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFilter = L"All Files\0*.*\0";
	ofn.lpstrFile = filePath;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	if (GetOpenFileNameW(&ofn)) {
		wchar_t password[256];
		GetWindowTextW(hEditControl, password, sizeof(password) / sizeof(password[0]));

		if (wcslen(password) == 0) {
			MessageBoxW(hWnd, L"Пароль не может быть пустым.", L"Ошибка", MB_ICONERROR);
			return;
		}

		try {
			std::wstring savedPassword;
			  // Читаем и расшифровываем сохранённый пароль
			if (VerifyPasswordForFile(filePath, password) == true) {
				unlockFileWithPassword(filePath, password);  // Разблокировка файла с паролем
				MessageBoxW(hWnd, L"Файл успешно разблокирован.", L"Информация", MB_ICONINFORMATION);
			}
			else {
				MessageBoxW(hWnd, L"Файл не зашифрований", L"Ошибка", MB_ICONERROR);
			}
		}
		catch (const std::exception& e) {
			std::wstring errorMessage = L"Ошибка: " + std::wstring(e.what(), e.what() + strlen(e.what()));
			MessageBoxW(hWnd, errorMessage.c_str(), L"Ошибка разблокировки файла", MB_ICONERROR);
		}
	}
	else {
		MessageBoxW(hWnd, L"Выбор файла отменён.", L"Информация", MB_ICONINFORMATION);
	}
}

void LoadData(LPCSTR path) {
	HANDLE FileToLoad = CreateFileA(
		path,
		GENERIC_READ,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	DWORD bytesIterated;
	ReadFile(FileToLoad, Buffer, TextBufferSize, &bytesIterated, NULL);

	SetWindowTextA(hEditControl, Buffer);

	CloseHandle(FileToLoad);
}
void SetOpenFileParams(HWND hWnd) {
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFile = filename;
	ofn.nMaxFile = sizeof(filename);
	ofn.lpstrFilter = "*.txt";
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = "C:/Users/Nikita/source/repos/Project/Project/output.txt";
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

}

void Encrypt(HWND hEditControl) {
	// Получаем длину текста из hEditControl
	int textLength = GetWindowTextLengthA(hEditControl);
	if (textLength == 0) {
		MessageBoxA(NULL, "No text to encrypt!", "Error", MB_ICONERROR);
		return;
	}

	// Создаём буфер для хранения текста
	char* buffer = new char[textLength + 1];
	GetWindowTextA(hEditControl, buffer, textLength + 1);

	// Шифруем текст
	std::string result = buffer;
	std::string alphabet = "abcdefghijklmnopqrstuvwxyz";
	for (char& c : result) {
		auto pos = alphabet.find(tolower(c));
		if (pos != std::string::npos) {
			c = alphabet[(pos + 3) % alphabet.size()];
		}
	}

	// Устанавливаем зашифрованный текст обратно в hEditControl
	SetWindowTextA(hEditControl, result.c_str());

	// Освобождаем выделенную память
	delete[] buffer;
}
void Dencrypt(HWND hEditControl) {
	// Получаем длину текста из hEditControl
	int textLength = GetWindowTextLengthA(hEditControl);
	if (textLength == 0) {
		MessageBoxA(NULL, "No text to encrypt!", "Error", MB_ICONERROR);
		return;
	}

	// Создаём буфер для хранения текста
	char* buffer = new char[textLength + 1];
	GetWindowTextA(hEditControl, buffer, textLength + 1);

	// Шифруем текст
	std::string result = buffer;
	std::string alphabet = "abcdefghijklmnopqrstuvwxyz";
	for (char& c : result) {
		auto pos = alphabet.find(tolower(c));
		if (pos != std::string::npos) {
			c = alphabet[(pos - 3 + alphabet.size()) % alphabet.size()];
		}
	}

	// Устанавливаем зашифрованный текст обратно в hEditControl
	SetWindowTextA(hEditControl, result.c_str());

	// Освобождаем выделенную память
	delete[] buffer;
}

void SetWindowStatus(std::string status) {
	SetWindowTextA(hStaticControl, status.c_str());

}