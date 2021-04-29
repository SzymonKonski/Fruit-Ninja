#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "UxTheme.lib")
#pragma comment( lib, "Msimg32" )
#include "framework.h"
#include "commctrl.h"
#include "Uxtheme.h"
#include "FruitNinja.h"
#include <cstdlib>
#include <string>
#include <time.h>
#include <windows.h>


#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;
WCHAR szTitle[MAX_LOADSTRING];
WCHAR szWindowClass[MAX_LOADSTRING];
WCHAR szProgressClass[MAX_LOADSTRING];

ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);


void CreateIniFile();
void CheckMenu();
void ResizeWindow(HWND hWnd);
void ReadFromIni();
void DrawBalls(HWND hWnd, int X[], int Y[], COLORREF C[], HDC memHDC, HBRUSH brush);
void UpdateBalls( int X[], int Y[], double VX[], double VY[], int elapseTime);
void ResteBalls();
int GenerateBall();
void ProgressBar(HWND hWnd, HDC memHDC, HBRUSH gree_brush, int maxCounter, int counter, HBRUSH white_brush);
void ResetGame(HWND hWnd, int width, int height, double vel);
void SetGreenScreen(HWND hWnd, HBRUSH green_brush, HDC memHDC);


#define BALLS_NUM 80 
int ballsX[BALLS_NUM], ballsY[BALLS_NUM]; 
double ballsVX[BALLS_NUM], ballsVY[BALLS_NUM]; 
bool dostepne[BALLS_NUM];
bool falling[BALLS_NUM];
COLORREF ballsC[BALLS_NUM];    
int radius = 25;

double velocity;
int x_pos, y_pos;
int g_width, g_height;
HMENU hMenu;
int block_size = 50;
int progress_margin = 20;
int nBalls; 
int timeStep = 30;
int timeStepThrow;
int wndWidth;
int wndHeight;
int score=0;


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    srand(time(NULL));
    InitCommonControls();
    
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_FruitNinja, szWindowClass, MAX_LOADSTRING);
    LoadStringW(hInstance, IDS_BarClass, szProgressClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);
    g_width = 8, g_height = 6;
    ReadFromIni();


    if (!InitInstance(hInstance, nCmdShow))
        return FALSE;
    
    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_FruitNinja));
    MSG msg;

    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int)msg.wParam;
}


ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
    wcex.hCursor = LoadCursor(hInstance, MAKEINTRESOURCE(IDC_CURSOR1));
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_FruitNinja);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_ICON1));

    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance;

    RECT rect, wr = { 0,0,g_width * block_size,g_height * block_size + progress_margin };

    AdjustWindowRect(&wr, WS_CAPTION | WS_SYSMENU, TRUE);
    int winWidth = wr.right - wr.left;
    int winHeight = wr.bottom - wr.top;

    int x = (GetSystemMetrics(SM_CXSCREEN) - wr.right + wr.left) / 2;
    int y = (GetSystemMetrics(SM_CYSCREEN) - wr.bottom + wr.top) / 2;

    x_pos = x;
    y_pos = y;

    wndWidth = winWidth;
    wndHeight = winHeight;

    ResteBalls();

    hMenu = LoadMenu(hInst, MAKEINTRESOURCE(IDC_FruitNinja));
    HWND hWnd = CreateWindowExW(WS_EX_TOPMOST, szWindowClass, szTitle,
        WS_CAPTION | WS_SYSMENU, x, y, winWidth, winHeight, nullptr, hMenu, hInstance, nullptr);

    CheckMenu();

    if (!hWnd) 
        return FALSE;

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int throw_time;
    static  HBRUSH black_brush = CreateSolidBrush(RGB(0, 0, 0));
    static HBRUSH white_brush = (HBRUSH)CreateSolidBrush(RGB(255, 255, 255));
    static HBRUSH green_brush = (HBRUSH)CreateSolidBrush(RGB(0, 255, 0));
    static HDC memHDC = NULL;
    static HBITMAP offOldBitmap = NULL;
    static HBITMAP offBitmap = NULL;
    static int counter = 0;
    const int maxCounter = 30000;
    int black = 1;
    static HFONT hFont = CreateFont(48,0,0,0,0,0,0,0,0,0,0,0,0,L"Roboto Th");
    

    switch (message)
    {
        case WM_ERASEBKGND:
        break;

        case WM_MOUSEMOVE:
        {
            SetWindowLong(hWnd, GWL_EXSTYLE,
            GetWindowLong(hWnd, GWL_EXSTYLE) | WS_EX_LAYERED);
            SetLayeredWindowAttributes(hWnd, 0, 255, LWA_ALPHA);
        }
        break;

        case WM_CREATE:
        {
            HDC hdc = GetDC(hWnd);
            memHDC = CreateCompatibleDC(hdc);
            ReleaseDC(hWnd, hdc);
            SetTimer(hWnd, TRANSPARENT_TIMER, 3000, NULL);
            SetTimer(hWnd, PAINT_TIMER, timeStep, NULL);
            SetTimer(hWnd, THROW_TIMER, 1000, NULL);
        }
        break;

        case WM_WINDOWPOSCHANGING:
        {
            WINDOWPOS* pos = (WINDOWPOS*)lParam;
            pos->x = x_pos;
            pos->y = y_pos;
        }
        break;

        case WM_SIZE:
        {
            counter = 0;
            SetTimer(hWnd, PROGRESS_TIMER, 100, (TIMERPROC)NULL);
            wndWidth = LOWORD(lParam);
            wndHeight = HIWORD(lParam);
            HDC hdc = GetDC(hWnd);
            if (offOldBitmap != NULL) {
                SelectObject(memHDC, offOldBitmap);
            }
            if (offBitmap != NULL) {
                DeleteObject(offBitmap);

            }
            offBitmap = CreateCompatibleBitmap(hdc, wndWidth, wndHeight);
            offOldBitmap = (HBITMAP)SelectObject(memHDC, offBitmap);
            ReleaseDC(hWnd, hdc);
        }
        break;

        case WM_COMMAND:
        {
            HMENU menu = GetSystemMenu(hWnd, FALSE);

            if (message == SC_MOVE)
                return 0;

            int wmId = LOWORD(wParam);

            switch (wmId)
            {
            case ID_GAME_NEWGAME:
                counter = 0;
                ResetGame(hWnd, g_width, g_height, velocity);

                break;

            case ID_BOARD_SMALL:
                counter = 0;
                timeStepThrow = 2000;
                ResetGame(hWnd, 8, 6, 1.6);
                break;

            case ID_BOARD_MEDIUM:
                counter = 0;
                timeStepThrow = 1000;
                ResetGame(hWnd, 12, 10, 2.3);

                break;

            case ID_BOARD_BIG:
                counter = 0;
                timeStepThrow = 500;
                ResetGame(hWnd, 16, 12, 2.8);

                break;

            case ID_GAME_EXIT:
                PostQuitMessage(0);
                break;

            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        RECT rc;
        GetClientRect(hWnd, &rc);

        int black = 1;
        for (int i = 0; i < g_height; i++)
        {
            for (int j = 0; j < g_width; j++)
            {
                if (black == 1)
                {
                    SelectObject(memHDC, black_brush);
                    Rectangle(memHDC, j * block_size, i * block_size, j * block_size + block_size, i * block_size + block_size);
                    black = 0;
                }

                else
                {
                    SelectObject(memHDC, white_brush);
                    Rectangle(memHDC, j * block_size, i * block_size, j * block_size + block_size, i * block_size + block_size);
                    black = 1;
                }
            }

            if (black == 0)
                black = 1;
            else
                black = 0;
        }
        
        std::wstring score_text = std::to_wstring(score);
        SelectObject(memHDC, hFont);
        SetTextColor(memHDC, RGB(0, 255, 0));
        SetBkMode(memHDC,  TRANSPARENT);
        TextOut(memHDC, rc.right - rc.left - 30, 0, score_text.c_str(), score_text.length());

        DrawBalls(hWnd, ballsX, ballsY, ballsC, memHDC, white_brush);

        BitBlt(hdc, 0, 0, rc.right - rc.left, rc.bottom - rc.top - progress_margin, memHDC, 0, 0, SRCCOPY);
        EndPaint(hWnd, &ps);
    }
    break;

    case WM_TIMER:
    {
        switch (wParam)
        {
            case TRANSPARENT_TIMER:
                SetWindowLong(hWnd, GWL_EXSTYLE,
                GetWindowLong(hWnd, GWL_EXSTYLE) | WS_EX_LAYERED);
                SetLayeredWindowAttributes(hWnd, 0, (255 * 50) / 100, LWA_ALPHA);
            break;

            case PROGRESS_TIMER:

                counter += 100;
                if (counter == 30000)
                {
                    KillTimer(hWnd, PROGRESS_TIMER);
                    KillTimer(hWnd, PAINT_TIMER);
                    KillTimer(hWnd, THROW_TIMER);
                    SetGreenScreen(hWnd, green_brush, memHDC);
                }
                else
                {
                    HBRUSH white_brush2 = (HBRUSH)SelectObject(memHDC, (HBRUSH)GetStockObject(WHITE_BRUSH));
                    ProgressBar(hWnd, memHDC, green_brush, maxCounter ,counter, white_brush2);
                }
            break;

            case PAINT_TIMER:
                UpdateBalls(ballsX, ballsY, ballsVX, ballsVY, timeStep / 10);
                InvalidateRect(hWnd, NULL, TRUE);
            break;

            case THROW_TIMER:
                GenerateBall();
            break;
        }
    }
    break;

    case WM_DESTROY:
    {
        KillTimer(hWnd, PROGRESS_TIMER);
        KillTimer(hWnd, PAINT_TIMER);
        KillTimer(hWnd, THROW_TIMER);
        HBRUSH oldBrush = (HBRUSH)SelectObject(memHDC, &green_brush);
        DeleteObject(green_brush);
        DeleteObject(white_brush);
        DeleteObject(black_brush);
        DeleteObject(hFont);

        if (offOldBitmap != NULL)
            SelectObject(memHDC, offOldBitmap);
        if (memHDC != NULL)
            DeleteDC(memHDC);
        if (offBitmap != NULL)
            DeleteObject(offBitmap);

        PostQuitMessage(0);
    }
    break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

void SetGreenScreen(HWND hWnd, HBRUSH green_brush, HDC memHDC)
{
    HDC hdc = GetDC(hWnd);
    RECT rc;
    GetClientRect(hWnd, &rc);
    int w = rc.right - rc.left;
    int h = rc.bottom - rc.top;

    BITMAPINFO bmi;
    ZeroMemory(&bmi, sizeof(BITMAPINFO));

    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = w;
    bmi.bmiHeader.biHeight = h;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    bmi.bmiHeader.biSizeImage = w * h * 4;

    HBITMAP hbitmap = CreateCompatibleBitmap(memHDC, w, h);
    SelectObject(memHDC, hbitmap);
    FillRect(memHDC, &rc, green_brush);

    BLENDFUNCTION bf;
    bf.BlendOp = AC_SRC_OVER;
    bf.BlendFlags = 0;
    bf.SourceConstantAlpha = 100;
    bf.AlphaFormat = 0;

    if (!AlphaBlend(hdc, rc.left, rc.top, w, h, memHDC, rc.left, rc.top, w, h, bf))
        return;

    ReleaseDC(hWnd, hdc);
    DeleteObject(hbitmap);
}

int GenerateBall()
{    
    if (nBalls >= BALLS_NUM)
        return 0;

    int index=0;
    for (int i = 0; i < BALLS_NUM; i++)
    {
        if (dostepne[i] == true)
        {
            index = i;
            break;
        }
    }

    dostepne[index] = false;
    ballsX[index] =  30 +( rand() % (wndWidth - 30 +1));
    ballsY[index] = wndHeight-40;

    int a = rand() % 2;
    if(a == 0)
        ballsVX[index] = 0.2;    
    else 
        ballsVX[index] = -0.2;

    ballsVY[index] = velocity;
    ballsC[index] = RGB(rand() % 256, rand() % 256, rand() % 256);    
    
    nBalls++;
    return 1;

}

void DrawBalls(HWND hWnd,  int X[], int Y[], COLORREF C[], HDC memHDC, HBRUSH white_brush)
{
    HDC hdc = GetDC(hWnd);

    HBRUSH brush;
    for (int i = 0;i < BALLS_NUM; i++)
    {
        if (dostepne[i] == false && nBalls >0)
        {
            brush = CreateSolidBrush(C[i]);
            SelectObject(memHDC, brush);
            Ellipse(memHDC, X[i] - radius, Y[i] - radius, X[i] + radius, Y[i] + radius);
            DeleteObject(brush);
        }
    }
    ReleaseDC(hWnd, hdc);
}

void UpdateBalls( int X[], int Y[], double VX[], double VY[], int elapseTime)
{
    if (nBalls > BALLS_NUM)
        return;

    for (int i = 0; i < BALLS_NUM; i++)
    {
        if (dostepne[i] == false && nBalls > 0)
        {
            if ((Y[i] + radius) >= wndHeight && falling[i] == false)
            {
                VY[i] = -abs(VY[i]);
                falling[i] = true;
            }
            else if ((Y[i] + radius) >= wndHeight && falling[i] == true)
            {
                dostepne[i] = true;
                falling[i] = false;
            }
        }
    }

    for (int i = 0; i < BALLS_NUM; i++)
    {
        if (dostepne[i] == false)
        {
            VY[i] += 0.03;


            X[i] += VX[i] * elapseTime;
            Y[i] += VY[i] * elapseTime;
        }
    }

}

void CreateIniFile()
{
    switch (g_width)
    {
    case 8:
        WritePrivateProfileString(TEXT("GAME"), TEXT("SIZE"), TEXT("1"), TEXT(".//FruitNinja.ini"));
        break;

    case 12:
        WritePrivateProfileString(TEXT("GAME"), TEXT("SIZE"), TEXT("2"), TEXT(".//FruitNinja.ini"));
        break;

    case 16:
        WritePrivateProfileString(TEXT("GAME"), TEXT("SIZE"), TEXT("3"), TEXT(".//FruitNinja.ini"));
        break;

    default:
        break;
    }
}

void ResizeWindow(HWND hWnd)
{
    RECT rect, wr = { 0,0,g_width * block_size,g_height * block_size + progress_margin };

    AdjustWindowRect(&wr, WS_CAPTION | WS_SYSMENU, TRUE);
    int winWidth = wr.right - wr.left;
    int winHeight = wr.bottom - wr.top;

    int x = (GetSystemMetrics(SM_CXSCREEN) - wr.right + wr.left) / 2;
    int y = (GetSystemMetrics(SM_CYSCREEN) - wr.bottom + wr.top) / 2;

    x_pos = x;
    y_pos = y;
    wndWidth = winWidth;
    wndHeight = winHeight;
   

    SetWindowPos(hWnd, NULL, x, y, winWidth, winHeight, NULL);
    UpdateWindow(hWnd);
    CheckMenu();
}

void CheckMenu()
{
    switch (g_width)
    {
    case 8:
        CheckMenuItem(hMenu, ID_BOARD_SMALL, MF_CHECKED);
        CheckMenuItem(hMenu, ID_BOARD_MEDIUM, MF_UNCHECKED);
        CheckMenuItem(hMenu, ID_BOARD_BIG, MF_UNCHECKED);
        break;

    case 12:
        CheckMenuItem(hMenu, ID_BOARD_SMALL, MF_UNCHECKED);
        CheckMenuItem(hMenu, ID_BOARD_MEDIUM, MF_CHECKED);
        CheckMenuItem(hMenu, ID_BOARD_BIG, MF_UNCHECKED);
        break;

    case 16:
        CheckMenuItem(hMenu, ID_BOARD_SMALL, MF_UNCHECKED);
        CheckMenuItem(hMenu, ID_BOARD_MEDIUM, MF_UNCHECKED);
        CheckMenuItem(hMenu, ID_BOARD_BIG, MF_CHECKED);
        break;

    default:
        break;
    }
}

void ReadFromIni()
{
    TCHAR   inBuf[2];
    int size = GetPrivateProfileString(TEXT("GAME"), TEXT("SIZE"), TEXT("0"), inBuf, 2, TEXT(".//FruitNinja.ini"));

    switch (inBuf[0])
    {
    case'1':
        g_width = 8;
        g_height = 6;
        timeStepThrow = 2000;
        velocity = 1.8;
        break;

    case'2':
        g_width = 12;
        g_height = 10;
        timeStepThrow = 1000;
        velocity = 2.3;
        break;

    case'3':
        g_width = 16;
        g_height = 12;
        timeStepThrow = 500;
        velocity = 2.8;
        break;

    default:
        g_width = 8;
        g_height = 6;
        break;
    }
}

void ResetGame(HWND hWnd, int width, int height, double vel)
{
    g_width = width;
    g_height = height;
    velocity = vel;

    ResteBalls();
    ResizeWindow(hWnd);
    
    SetTimer(hWnd, PROGRESS_TIMER, 100, (TIMERPROC)NULL);
    SetTimer(hWnd, PAINT_TIMER, timeStep, NULL);
    SetTimer(hWnd, THROW_TIMER, timeStepThrow, NULL);

    CreateIniFile();
}

void ResteBalls()
{
    nBalls = 0;

    for (int i = 0; i < BALLS_NUM; i++)
    {
        dostepne[i] = true;
        falling[i] = false;
    }
}

void ProgressBar(HWND hWnd, HDC memHDC, HBRUSH gree_brush, int maxCounter, int counter, HBRUSH white_brush)
{
    HDC hdc = GetDC(hWnd);
    RECT rc;
    GetClientRect(hWnd, &rc);

    int width = rc.right - rc.left;
    int progress_size = counter * width / maxCounter;
    
    Rectangle(memHDC, rc.left, rc.bottom - progress_margin, width, rc.bottom);
    SelectObject(memHDC, gree_brush);

    
    Rectangle(memHDC, rc.left, rc.bottom - progress_margin, progress_size, rc.bottom);

    BitBlt(hdc, 0, 0, rc.right, rc.bottom, memHDC, 0, 0, SRCCOPY);
    SelectObject(memHDC, white_brush);
    ReleaseDC(hWnd, hdc);
}