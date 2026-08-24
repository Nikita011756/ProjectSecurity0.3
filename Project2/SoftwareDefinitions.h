#pragma once
#include <commdlg.h>
#include <Windows.h>
#include <string>

#define OnSerialRefresh   2
#define OnConnectRequest  3
#define OnExitSoftware    4
#define OnClearField    5
#define OnMenuClearField   6
#define OnSaveFile        8
#define OnLoadFile        9
#define OnEncrypt         10
#define OnDencrypt        11
#define OnLockedFile          12
#define OnUnlockFile      13

#define ID_EDIT_LENGTH     14
#define ID_CHECK_LOWER     15
#define ID_CHECK_UPPER     16
#define ID_CHECK_DIGITS    17
#define ID_CHECK_SPECIAL   18
#define ID_BUTTON_GENERATE 19
#define ID_EDIT_RESULT     20

#define DlgIndexColorR    200
#define DlgIndexColorG    201
#define DlgIndexColorB    202

#define TextBufferSize    256

char Buffer[TextBufferSize];
unsigned num;
//int CharRead;

HWND hStaticControl;
HWND hEditControl;
HWND hNumberControl;
HWND Leght;

HMENU ComPortSubMenu;
HMENU ComPortListMenu;
HWND hLowerChk;
HWND hUpperChk;
HWND hDigitsChk;
HWND hSpecialChk;


char filename[260];
OPENFILENAMEA ofn;

volatile bool isConnected = false;
volatile bool isThreading = true;

int selectedPort = 1;
int targetBaudRate = 9600;

HFONT fontRectangle;
COLORREF fontColor;
int colorR, colorG, colorB;

HBRUSH brushRectangle;
RECT windowRectangle;
PAINTSTRUCT ps;

HANDLE connectedPort;
HANDLE readThread;

LRESULT CALLBACK SoftwareMainProcedure(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);
WNDCLASS NewWindowClass(HBRUSH BGColor, HCURSOR Cursor, HINSTANCE hInst, HICON Icon, LPCWSTR Name, WNDPROC Procedure);

void MainWndAddMenus(HWND hWnd);
void MainWndAddWidgets(HWND hWnd);
void SetOpenFileParams(HWND hWnd);
void SetWindowStatus(std::string status);
void SaveData(LPCSTR path);
void LoadData(LPCSTR path);
void Encrypt(HWND hEditControl);
void Dencrypt(HWND hEditControl);
void OnLockFile(HWND hWnd, HWND hEditControl);
void lockFileWithPassword(const std::wstring& filePath, const std::wstring& password);
void encryptFile(const std::wstring& inputFile, const std::wstring& outputFile);
void UnlockFile(HWND hWnd, HWND hEditControl);
void unlockFileWithPassword(const std::wstring& filePath, const std::wstring& password);
void lockFileWithPassword(const std::wstring& filePath, const std::wstring& password);
void OnGeneratePassword(HWND hWnd);
void generatePassword(int length, bool useLower, bool useUpper, bool useDigits, bool useSpecial);
//void LoadPassword(const std::wstring& filePath, std::wstring& password);

