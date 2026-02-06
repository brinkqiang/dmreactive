
# dmreactive

Copyright (c) 2013-2018 brinkqiang (brink.qiang@gmail.com)

[![dmreactive](https://img.shields.io/badge/brinkqiang-dmreactive-blue.svg?style=flat-square)](https://github.com/brinkqiang/dmreactive)
[![License](https://img.shields.io/badge/license-MIT-brightgreen.svg)](https://github.com/brinkqiang/dmreactive/blob/master/LICENSE)
[![blog](https://img.shields.io/badge/Author-Blog-7AD6FD.svg)](https://brinkqiang.github.io/)
[![Open Source Love](https://badges.frapsoft.com/os/v3/open-source.png)](https://github.com/brinkqiang)
[![GitHub stars](https://img.shields.io/github/stars/brinkqiang/dmreactive.svg?label=Stars)](https://github.com/brinkqiang/dmreactive) 
[![GitHub forks](https://img.shields.io/github/forks/brinkqiang/dmreactive.svg?label=Fork)](https://github.com/brinkqiang/dmreactive)

## Build status
| [Linux][lin-link] | [Mac][mac-link] | [Windows][win-link] |
| :---------------: | :----------------: | :-----------------: |
| ![lin-badge]      | ![mac-badge]       | ![win-badge]        |

[lin-badge]: https://github.com/brinkqiang/dmreactive/workflows/linux/badge.svg "linux build status"
[lin-link]:  https://github.com/brinkqiang/dmreactive/actions/workflows/linux.yml "linux build status"
[mac-badge]: https://github.com/brinkqiang/dmreactive/workflows/mac/badge.svg "mac build status"
[mac-link]:  https://github.com/brinkqiang/dmreactive/actions/workflows/mac.yml "mac build status"
[win-badge]: https://github.com/brinkqiang/dmreactive/workflows/win/badge.svg "win build status"
[win-link]:  https://github.com/brinkqiang/dmreactive/actions/workflows/win.yml "win build status"

## 简介

dmreactive 是一个基于现代C++的响应式编程库，提供轻量级、高性能的响应式数据流处理能力。该库采用模块化设计，支持信号槽机制、观察者模式和异步管道处理，适用于事件驱动编程、数据流处理和异步任务调度等场景。

## 特性

- 🚀 **响应式数据流**: 支持观察者模式，实现数据变更的自动传播
- ⚡ **信号槽机制**: 提供灵活的事件驱动编程支持
- 🔄 **异步管道**: 内置异步数据处理管道，支持数据转换和过滤
- 🛡️ **线程安全**: 使用原子操作保证多线程环境下的安全性
- 📦 **模块化设计**: 基于接口的模块化架构，易于扩展和维护
- 🎯 **零异常**: 遵循C++最佳实践，避免异常使用
- 🔧 **标准兼容**: 基于C++17标准，跨平台支持

## 依赖

- C++17
- CMake 3.12+
- Google Test (用于单元测试，可选)

## 构建

### Linux/macOS
```bash
git clone https://github.com/brinkqiang/dmreactive.git
cd dmreactive
mkdir build && cd build
cmake ..
make
```

### Windows
```bash
git clone https://github.com/brinkqiang/dmreactive.git
cd dmreactive
mkdir build && cd build
cmake -G "Visual Studio 16 2019" ..
cmake --build . --config Release
```

## 使用示例

### 基本响应式数据流
```cpp
#include "dmreactive.h"

int main() {
    dmreactivePtr module(dmreactiveGetModule());
    
    // 创建观察者
    auto observable = module->CreateObservable();
    observable->Subscribe([](const void* data) {
        if (data) {
            const std::string* str = static_cast<const std::string*>(data);
            std::cout << "Received: " << *str << std::endl;
        }
    });
    
    // 发送数据
    std::string message = "Hello Reactive!";
    module->NotifyObservers(observable.get(), &message);
    
    return 0;
}
```

### 信号槽机制
```cpp
#include "dmreactive.h"

int main() {
    dmreactivePtr module(dmreactiveGetModule());
    
    auto signal = module->CreateSignal();
    
    // 连接多个槽函数
    signal->Connect([]() { std::cout << "Slot 1 triggered" << std::endl; });
    signal->Connect([]() { std::cout << "Slot 2 triggered" << std::endl; });
    
    // 触发信号
    signal->Emit();
    
    return 0;
}
```

### 异步管道处理
```cpp
#include "dmreactive.h"

int main() {
    dmreactivePtr module(dmreactiveGetModule());
    
    auto pipeline = module->CreatePipeline();
    
    // 设置数据处理函数
    pipeline->SetProcessor([](void* data) -> void* {
        if (data) {
            int* value = static_cast<int*>(data);
            *value *= 2;  // 数据转换
            std::cout << "Processed value: " << *value << std::endl;
        }
        return data;
    });
    
    // 处理数据
    int data = 42;
    pipeline->Process(&data);
    
    return 0;
}
```

## API 文档

### 核心接口

#### Idmreactive
响应式编程库的主接口，提供组件创建功能。

```cpp
virtual dmobservablePtr CreateObservable();
virtual dmsignalPtr CreateSignal();  
virtual dmpipelinePtr CreatePipeline();
```

#### Idmobservable
响应式数据流接口，支持观察者订阅模式。

```cpp
virtual void Subscribe(const std::function<void(const void*)>& observer);
virtual void UnsubscribeAll();
```

#### Idmsignal
信号槽接口，支持事件驱动编程。

```cpp
virtual void Connect(const std::function<void()>& slot);
virtual void DisconnectAll();
virtual void Emit();
```

#### Idmpipeline
异步管道接口，支持数据流水线处理。

```cpp
virtual void Process(const void* data);
virtual void SetProcessor(const std::function<void*(void*)>& processor);
```

## 测试

项目包含完整的单元测试，使用 Google Test 框架：

```bash
cd build
make test
# 或
ctest
```

## 贡献

欢迎提交 Issue 和 Pull Request 来改进这个项目。

## 许可证

本项目采用 MIT 许可证 - 查看 [LICENSE](LICENSE) 文件了解详情。

## 致谢

感谢所有为这个项目做出贡献的开发者。

---

如果这个项目对您有帮助，请给我们一个 ⭐️ ！
