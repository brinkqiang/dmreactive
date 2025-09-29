
// Copyright (c) 2018 brinkqiang (brink.qiang@gmail.com)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

//=============================================================================
// dmreactive - 现代C++响应式编程库
// 支持响应式数据流、信号槽机制、异步管道处理
//=============================================================================

#ifndef __DMREACTIVE_H_INCLUDE__
#define __DMREACTIVE_H_INCLUDE__

#include "dmos.h"
#include <functional>
#include <vector>
#include <memory>
#include <atomic>
#include "dmmoduleptr.h"

class Idmreactive;
typedef DmUniquePtr<Idmreactive> dmreactivePtr;

// 响应式数据流接口
class Idmobservable {
public:
    virtual ~Idmobservable() {}
    virtual void DMAPI Release(void) = 0;
    virtual void DMAPI Subscribe(const std::function<void(const void*)>& observer) = 0;
    virtual void DMAPI UnsubscribeAll() = 0;
};

typedef DmUniquePtr<Idmobservable> dmobservablePtr;

// 信号槽接口
class Idmsignal {
public:
    virtual ~Idmsignal() {}
    virtual void DMAPI Release(void) = 0;
    virtual void DMAPI Connect(const std::function<void()>& slot) = 0;
    virtual void DMAPI DisconnectAll() = 0;
    virtual void DMAPI Emit() = 0;
};

typedef DmUniquePtr<Idmsignal> dmsignalPtr;

// 异步管道接口
class Idmpipeline {
public:
    virtual ~Idmpipeline() {}
    virtual void DMAPI Release(void) = 0;
    virtual void DMAPI Process(const void* data) = 0;
    virtual void DMAPI SetProcessor(const std::function<void*(void*)>& processor) = 0;
};

typedef DmUniquePtr<Idmpipeline> dmpipelinePtr;

// 主响应式模块接口
class Idmreactive {
public:
    virtual ~Idmreactive() {}
    virtual void DMAPI Release(void) = 0;
    
    // 响应式数据流
    virtual dmobservablePtr DMAPI CreateObservable() = 0;
    virtual void DMAPI NotifyObservers(Idmobservable* observable, const void* data) = 0;
    
    // 信号槽
    virtual dmsignalPtr DMAPI CreateSignal() = 0;
    
    // 异步管道
    virtual dmpipelinePtr DMAPI CreatePipeline() = 0;
    
    // 工具函数
    virtual dmobservablePtr DMAPI FromFunction(const std::function<void*(void*)>& func) = 0;
    virtual dmobservablePtr DMAPI FromContainer(const std::vector<void*>& container) = 0;
};

extern "C" DMEXPORT_DLL Idmreactive* DMAPI dmreactiveGetModule();
typedef Idmreactive* (DMAPI* PFN_dmreactiveGetModule)();

#endif // __DMREACTIVE_H_INCLUDE__

