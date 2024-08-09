#include <windows.h>
#include <sys/stat.h>
#include <sys/types.h>
#include<fileapi.h>
#include <stdio.h>
#include <stdlib.h>
#include<time.h>
#include "Source.h"
#pragma warning(disable : 4996) //_CRT_SECURE_NO_WARNINGS

// Declare global variables
NOTIFYICONDATA nid;  //  contains information about the tray icon.
const wchar_t* separatChar = L"\\";  // TODO Change the name
wchar_t* CbSD;
HWND g_hwndNextViewer;
 
void CbSD_Init() {
    CbSD = calloc(FILENAME_MAX, sizeof(wchar_t)); // clipboard save directory;
    CbSD[0] = L'\0'; // add a null terminator to the beginning of the buffer
    wchar_t* d = L"C:\\Users\\basha\\clipboard_saves";
    wcscat(CbSD, d);
}
inline void conStrInt(wchar_t* str, int number) {
    wchar_t numStr[5];  // allocate a buffer to hold the string representation of the number 
    swprintf(numStr, 5, L"%d", number);  // convert the number to a wide string
    wcscat(str, numStr);  // copy the wide string to the output buffer
};
inline int isDirEx(wchar_t* path) {
    DWORD result = GetFileAttributes(path);
    return (result != INVALID_FILE_ATTRIBUTES) && (result & FILE_ATTRIBUTE_DIRECTORY);
}
void dirInit() {

    time_t t = time(NULL);
    struct tm curr_time = *localtime(&t); //  converts the time_t value into a local time representation

    int year = curr_time.tm_year + 1900;
    int month = curr_time.tm_mon + 1;
    int day = curr_time.tm_mday;

    int time_array[] = { year, month, day };


    for (unsigned int i = 0; i < 3; i++) {
        wcscat(CbSD, separatChar);
        conStrInt(CbSD, time_array[i]);

        if (!isDirEx(CbSD))
            CreateDirectory(CbSD, NULL);
    }
}
int GetPixelDataOffsetForPackedDIB(const BITMAPINFOHEADER* BitmapInfoHeader)
{
    int OffsetExtra = 0;

    if (BitmapInfoHeader->biSize == sizeof(BITMAPINFOHEADER) /* 40 */)
    {
        // This is the common BITMAPINFOHEADER type. In this case, there may be bit masks following the BITMAPINFOHEADER
        // and before the actual pixel bits (does not apply if bitmap has <= 8 bpp)
        if (BitmapInfoHeader->biBitCount > 8)
        {
            if (BitmapInfoHeader->biCompression == BI_BITFIELDS)
            {
                OffsetExtra += 3 * sizeof(RGBQUAD);
            }
            else if (BitmapInfoHeader->biCompression == 6 /* BI_ALPHABITFIELDS */)
            {
                // Not widely supported, but technically a valid DIB format.
                // You *can* get this in the clipboard, although neither GDI nor stb_image will like it.
                OffsetExtra += 4 * sizeof(RGBQUAD);
            }
        }
    }

    if (BitmapInfoHeader->biClrUsed > 0)
    {
        // We have no choice but to trust this value.
        OffsetExtra += BitmapInfoHeader->biClrUsed * sizeof(RGBQUAD);
    }
    else
    {
        // In this case, the color table contains the maximum number for the current bit count (0 if > 8bpp)
        if (BitmapInfoHeader->biBitCount <= 8)
        {
            // 1bpp: 2
            // 4bpp: 16
            // 8bpp: 256
            OffsetExtra += sizeof(RGBQUAD) << BitmapInfoHeader->biBitCount;
        }
    }

    return BitmapInfoHeader->biSize + OffsetExtra;
}
void imageProcess()
{
   


    HGLOBAL ClipboardDataHandle = (HGLOBAL)GetClipboardData(CF_DIB);


    void* pDIBData = GlobalLock(ClipboardDataHandle);  // using [GlobalLock] to allocates specified location to write on it 

    BITMAPINFOHEADER* BitmapInfoHeader = (BITMAPINFOHEADER*)(pDIBData);
    SIZE_T ClipboardDataSize = GlobalSize(ClipboardDataHandle);
    int PixelDataOffset = GetPixelDataOffsetForPackedDIB(BitmapInfoHeader);
    //
    size_t TotalBitmapFileSize = sizeof(BITMAPFILEHEADER) + ClipboardDataSize;
    wprintf(L"BITMAPINFOHEADER size:          %u\r\n", BitmapInfoHeader->biSize);
    wprintf(L"Format:                         %hubpp, Compression %u\r\n", BitmapInfoHeader->biBitCount, BitmapInfoHeader->biCompression);
    wprintf(L"Pixel data offset within DIB:   %u\r\n", PixelDataOffset);
    wprintf(L"Total DIB size:                 %zu\r\n", ClipboardDataSize);
    wprintf(L"Total bitmap file size:         %zu\r\n", TotalBitmapFileSize);

    BITMAPFILEHEADER BitmapFileHeader = { 0 };
    BitmapFileHeader.bfType = 0x4D42;
    BitmapFileHeader.bfSize = (DWORD)TotalBitmapFileSize; // Will fail if bitmap size is nonstandard >4GB
    BitmapFileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + PixelDataOffset;

    const char* fileName = L"C:/Users/basha/Pictures/test2.jpg";
    HANDLE FileHandle = CreateFileW(fileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (FileHandle != INVALID_HANDLE_VALUE)
    {

        DWORD dummy = 0;
        int Success = 1;
        Success &= WriteFile(FileHandle, &BitmapFileHeader, sizeof(BITMAPFILEHEADER), &dummy, NULL);
        Success &= WriteFile(FileHandle, BitmapInfoHeader, (DWORD)ClipboardDataSize, &dummy, NULL);
        Success &= CloseHandle(FileHandle);
        if (Success)
        {
            wprintf(L"File saved.\r\n");
        }
        GlobalUnlock(ClipboardDataHandle);
    }
}
void getClipboardData(void* lastDataPointer)

{
    HANDLE h;
    if (!OpenClipboard(NULL))
    {
        printf("Can't open clipboard");
        return;
    }
    if (IsClipboardFormatAvailable(CF_TEXT))
    {
        HANDLE h = GetClipboardData(CF_TEXT);
       
       
        printf("The pointer is: %p", (char*)h);
        // saveTextToFile();
  
    }
    else
    {
        if (IsClipboardFormatAvailable(CF_DIB))

        {
            HANDLE h = GetClipboardData(CF_DIB);
            if ((void*)h != lastDataPointer) {
                lastDataPointer = (void*) h;
                imageProcess();
            }
            else {
                printf("Same Image\n");
            }
        }
        CloseClipboard();

    }
    CloseClipboard();
}
 

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static int clipboardChanged = 0;

    switch (uMsg)
    {
 

    case WM_DRAWCLIPBOARD:
    {
        if (!clipboardChanged)
        {
            // Clipboard contents have changed
            printf("Clipboard data has been changed\n");
            clipboardChanged = 1;
        }

        // Pass the message on to the next viewer
        if (g_hwndNextViewer != NULL)
        {
            SendMessage(g_hwndNextViewer, uMsg, wParam, lParam);
        }
        break;
    }



 

        // Handle tray icon messages
    case WM_USER:
        switch (lParam)
        {
            // Handle left-click on icon
        case WM_LBUTTONDOWN:
            MessageBox(NULL, L"Left-click on tray icon", L"Tray Icon", MB_OK);
            break;
            // Handle right-click on icon
        case WM_RBUTTONDOWN:
            MessageBox(NULL, L"Right-click on tray icon", L"Tray Icon", MB_OK);
            break;
        }
        break;

    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void RegisterClipboardViewer(HWND hwnd)
{
    g_hwndNextViewer = SetClipboardViewer(hwnd);
}
int main()
{
    CbSD_Init();
    dirInit();
    // Set up tray icon data
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = NULL;
    nid.uID = 0;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_USER;
    nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    strcpy_s(nid.szTip, sizeof(nid.szTip), L"My Tray Icon");

    // Create window
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = L"MyWindowClass";
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName = NULL;
    RegisterClass(&wc);
    HWND hwnd = CreateWindow(wc.lpszClassName, L"My Window", WS_OVERLAPPEDWINDOW, 0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL);

    // Add tray icon
    nid.hWnd = hwnd;
    Shell_NotifyIcon(NIM_ADD, &nid);

    // Register clipboard viewer
    g_hwndNextViewer = SetClipboardViewer(hwnd);

    // Run message loop
    MSG msg;
    void* lastDataPointer;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);

    }
    // Unregister clipboard viewer
    ChangeClipboardChain(hwnd, g_hwndNextViewer);

    // Clean up
    Shell_NotifyIcon(NIM_DELETE, &nid);
    DestroyWindow(hwnd);
    UnregisterClass(wc.lpszClassName, wc.hInstance);

    free(CbSD);

    return 0;
}
