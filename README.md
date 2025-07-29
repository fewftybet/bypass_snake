# fscan_bypass 免杀工具

一个用于对 fscan 等 EXE 文件进行免杀处理的工具集，通过将 EXE 转换为 Shellcode 并进行 XOR 加密，配合自定义加载器实现内存加载执行，提升绕过检测的能力。

## 绕过效果
若文件失效，通过对cpp文件进行混淆即可重新免杀。

截止时间为2025年7月29日
<img width="2879" height="1799" alt="db709506c3951f8989cc9a682bcc294" src="https://github.com/user-attachments/assets/13bb76a4-aa6c-4979-96da-20ccecb19b46" />
<img width="2879" height="1799" alt="4bdb0d0a21dbbdee2e294073b8e6099" src="https://github.com/user-attachments/assets/4dec1e61-cd3e-4036-9a4d-af6be2578328" />

## 火绒
<img width="2879" height="1799" alt="6340e8cf5ba2f4476afce01b07dcc1a" src="https://github.com/user-attachments/assets/e291efe3-6a58-4572-86f6-54ac29d673dd" />

## 腾讯电脑管家
<img width="2879" height="1799" alt="3fd2f6e36c55cea835517a33c458f65" src="https://github.com/user-attachments/assets/93d1aad7-91c6-4796-9b73-efffefdf8f7f" />

## 360安全卫士
<img width="2879" height="1799" alt="2cd7bf38a18133d443bff0921cfd7c1" src="https://github.com/user-attachments/assets/80933059-c271-4d8c-af27-9e9411e6d237" />

## 联想电脑管家
<img width="2879" height="1627" alt="40ec712ffd9ccef14e6e26a132e2586" src="https://github.com/user-attachments/assets/cdfc4d24-4f49-4f17-a4b3-7db09a1ebc1e" />


## 项目简介

本工具集提供了一套完整的fscan.exe（64位）免杀处理流程，主要包括：

- 将原始 EXE 文件转换为 Shellcode
- 使用 XOR 算法对 Shellcode 进行加密
- 通过加载器在内存中解密并执行 Shellcode

通过内存加载的方式避免直接运行 EXE 文件，结合加密处理提高免杀效果，适用于安全测试和研究场景。

## 工具组成

- 一键转Shellcode工具.bat：自动化处理脚本，集成转换和加密流程
- `EXEToshellcode/`：包含 EXE 转 Shellcode 的转换工具
- `xor/`：包含 XOR 加密脚本（xor.py）
- loader.cpp：加密 Shellcode 的加载器源码，用于解密并执行
- output.bin：fscan.exe转换并加密后的shellcode

## 使用方法

### 前提条件

- Windows 操作系统
- Python  环境（用于运行加密脚本）
- Visual Studio 编译器

### 操作步骤

1. **准备工作**：将需要处理的 fscan.exe工具准备好，并放在bat脚本同级目录下

2. **执行转换加密**：

   - 运行一键转Shellcode工具.bat
   - 按照提示将 EXE 文件拖入窗口或输入文件路径
   - 输入 XOR 加密密钥（默认值为 170，即 0xAA）
   - 处理完成后，会在原 EXE 文件目录生成`output.bin`（加密后的 Shellcode）

3. **编译加载器**：

   使用Visual Studio进行编译（若编译失败可直接利用已经仓库中编译好的loader.exe）

4. **执行 Shellcode**：

   - 将编译好的`loader.exe`与`output.bin`放在同一目录
   - 确保loader.cpp中定义的`xorKey`与加密时使用的密钥一致
   - 运行`loader.exe`即可在内存中解密并执行 Shellcode

## 核心代码说明

### XOR 加密脚本（xor.py）

实现对 Shellcode 的 XOR 单字节加密，支持自定义密钥（0-255）：

```python
def xor_data(data: bytes, key: int) -> bytes:
    """对数据进行 XOR 加密/解密（key为单字节0-255）"""
    return bytes([b ^ key for b in data])
```

### 加载器（loader.cpp）

关键功能：

```
- 读取加密的 Shellcode 文件（`output.bin`）
- 在内存中分配可执行区域
- 使用相同密钥解密 Shellcode
- 刷新指令缓存并执行 Shellcode
```

核心解密函数：

```cpp
// 禁用优化确保解密函数不被编译器优化掉
#pragma optimize("", off)
void xor_decrypt(BYTE* data, DWORD size, BYTE key) {
    for (DWORD i = 0; i < size; i++) {
        data[i] ^= key;
    }
}
#pragma optimize("", on)
```

## 注意事项

1. 加密密钥必须保持一致：一键转Shellcode工具.bat中使用的密钥需与loader.cpp中定义的`xorKey`相同
2. 本工具仅用于合法的安全测试和学习研究，请勿用于未授权的渗透测试
3. 不同环境下的免杀效果可能存在差异，建议结合实际场景进行调整
4. 使用前请确保已获得目标系统的测试授权，遵守相关法律法规
