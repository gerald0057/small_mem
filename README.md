# Small Memory Manager

![License](https://img.shields.io/badge/license-Apache--2.0-blue) ![Build Status](https://img.shields.io/badge/build-passing-brightgreen)

A lightweight memory management library for embedded systems or environments with constrained resources, based on [RT-Thread](https://github.com/RT-Thread/rt-thread/blob/master/src/mem.c).

## Features

- **Small footprint**: Minimal memory overhead for management structures
- **Deterministic behavior**: Fixed-time operations for real-time systems
- **Memory pool management**: Manages a fixed memory region efficiently
- **Standard-compatible**: Provides familiar `alloc`, `realloc`, and `free` interfaces
- **Portable**: Easily adaptable to different platforms through porting layer

## API Overview

```c
/* Initialize memory manager with a memory region */
smem_t smem_init(void *begin_addr, size_t size);

/* Allocate memory block */
void *smem_alloc(smem_t m, size_t size);

/* Reallocate memory block */
void *smem_realloc(smem_t m, void *rmem, size_t newsize);

/* Free allocated memory */
void smem_free(void *rmem);
```

## Getting Started

### Prerequisites

- CMake (version 3.10 or higher)
- C compiler (GCC, Clang, or equivalent)
- [Optional] Google Test (for running tests)

### Building

```bash
cd /path/to/small_mem/small_mem
mkdir build && cd build
cmake ..
cmake --build .
```

### Installation

```bash
sudo cmake --install .
```

### Running Tests

```bash
cd /path/to/small_mem
mkdir build && cd build
cmake ..
cmake --build .
./run_unit_tests
```

## Usage Example

```c
#include "smem.h"

#define MEMORY_SIZE 1024

int main() {
    uint8_t memory_pool[MEMORY_SIZE];
    smem_t mem = smem_init(memory_pool, MEMORY_SIZE);
    
    int* arr = (int*)smem_alloc(mem, 10 * sizeof(int));
    // ... use the memory ...
    smem_free(arr);
    
    return 0;
}
```

## Configuration

Configuration options can be set in `smem_port.h`:
- Memory alignment requirements
- Debugging options
- Platform-specific overrides

## License

Apache License 2.0 - see [LICENSE](LICENSE) file for details

This project is based on [RT-Thread] (SPDX-License-Identifier: Apache-2.0).

---

# Small Mem 小型内存管理器

![License](https://img.shields.io/badge/license-Apache--2.0-blue) ![Build Status](https://img.shields.io/badge/build-passing-brightgreen)

面向嵌入式系统或资源受限环境的轻量级内存管理库，基于[RT-Thread](https://github.com/RT-Thread/rt-thread/blob/master/src/mem.c).

## 特性

- **小内存占用**: 管理结构内存开销极小
- **确定性行为**: 固定时间操作，适合实时系统
- **内存池管理**: 高效管理固定内存区域
- **标准兼容**: 提供熟悉的 `alloc`、`realloc` 和 `free` 接口
- **可移植**: 通过移植层轻松适配不同平台

## API 概述

```c
/* 用内存区域初始化内存管理器 */
smem_t smem_init(void *begin_addr, size_t size);

/* 分配内存块 */
void *smem_alloc(smem_t m, size_t size);

/* 重新分配内存块 */
void *smem_realloc(smem_t m, void *rmem, size_t newsize);

/* 释放已分配内存 */
void smem_free(void *rmem);
```

## 快速开始

### 前提条件

- CMake (3.10 或更高版本)
- C 编译器 (GCC, Clang 或等效工具)
- [可选] Google Test (用于运行测试)

### 构建

```bash
cd /path/to/small_mem/small_mem
mkdir build && cd build
cmake ..
cmake --build .
```

### 安装

```bash
sudo cmake --install .
```

### 运行测试

```bash
cd /path/to/small_mem
mkdir build && cd build
cmake ..
cmake --build .
./run_unit_tests
```

## 使用示例

```c
#include "smem.h"

#define MEMORY_SIZE 1024

int main() {
    uint8_t memory_pool[MEMORY_SIZE];
    smem_t mem = smem_init(memory_pool, MEMORY_SIZE);
    
    int* arr = (int*)smem_alloc(mem, 10 * sizeof(int));
    // ... 使用内存 ...
    smem_free(arr);
    
    return 0;
}
```

## 配置

配置选项可在 `smem_port.h` 中设置:
- 内存对齐要求
- 调试选项
- 平台特定重写

## 许可证

Apache 许可证 2.0 - 详见 [LICENSE](LICENSE) 文件

本项目基于 [RT-Thread] (SPDX-License-Identifier: Apache-2.0)。
