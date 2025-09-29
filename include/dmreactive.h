
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
#include "dmmoduleptr.h"
#include <functional>
#include <vector>
#include <memory>
#include <atomic>
#include <future>
#include <type_traits>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <any>
#include <typeindex>

//=============================================================================
// 前向声明
//=============================================================================
class IdmReactiveEvent;
class IdmReactiveObserver;
class IdmReactiveStream;
class IdmReactiveSubject;
class IdmReactiveOperator;
class IdmReactiveScheduler;
class IdmReactiveFactory;
class IdmReactiveCore;

//=============================================================================
// 智能指针别名
//=============================================================================
typedef DmModulePtr<IdmReactiveEvent> dmReactiveEventPtr;
typedef DmModulePtr<IdmReactiveObserver> dmReactiveObserverPtr;
typedef DmModulePtr<IdmReactiveStream> dmReactiveStreamPtr;
typedef DmModulePtr<IdmReactiveSubject> dmReactiveSubjectPtr;
typedef DmModulePtr<IdmReactiveOperator> dmReactiveOperatorPtr;
typedef DmModulePtr<IdmReactiveScheduler> dmReactiveSchedulerPtr;
typedef DmModulePtr<IdmReactiveFactory> dmReactiveFactoryPtr;
typedef DmModulePtr<IdmReactiveCore> dmReactiveCorePtr;

//=============================================================================
// 类型安全的事件包装
//=============================================================================
class IdmReactiveEvent {
public:
    enum Type {
        DMNEXT,
        DMERROR,
        DMCOMPLETE
    };
    
    virtual ~IdmReactiveEvent() {}
    virtual void DMAPI Release() = 0;
    
    virtual Type DMAPI GetType() const = 0;
    virtual std::any DMAPI GetValue() const = 0;
    virtual std::string DMAPI GetError() const = 0;
    virtual std::type_index DMAPI GetValueType() const = 0;
    
    // 类型安全的值获取
    template<typename T>
    T GetTypedValue() const {
        if (GetValueType() != std::type_index(typeid(T))) {
            throw std::bad_cast();
        }
        return std::any_cast<T>(GetValue());
    }
    
    template<typename T>
    bool TryGetTypedValue(T& out_value) const {
        if (GetValueType() != std::type_index(typeid(T))) {
            return false;
        }
        try {
            out_value = std::any_cast<T>(GetValue());
            return true;
        } catch (...) {
            return false;
        }
    }
};

//=============================================================================
// 观察者接口
//=============================================================================
class IdmReactiveObserver {
public:
    virtual ~IdmReactiveObserver() {}
    virtual void DMAPI Release() = 0;
    
    virtual void DMAPI OnEvent(IdmReactiveEvent* event) = 0;
    virtual std::type_index DMAPI GetExpectedType() const = 0;
    virtual bool DMAPI IsActive() const = 0;
    virtual void DMAPI SetActive(bool active) = 0;
};

//=============================================================================
// 响应式流接口
//=============================================================================
class IdmReactiveStream {
public:
    virtual ~IdmReactiveStream() {}
    virtual void DMAPI Release() = 0;
    
    // 订阅管理
    virtual void DMAPI Subscribe(IdmReactiveObserver* observer) = 0;
    virtual void DMAPI Unsubscribe(IdmReactiveObserver* observer) = 0;
    virtual void DMAPI UnsubscribeAll() = 0;
    virtual size_t DMAPI GetObserverCount() const = 0;
    
    // 流状态
    virtual bool DMAPI IsActive() const = 0;
    virtual bool DMAPI IsCompleted() const = 0;
    virtual bool DMAPI HasError() const = 0;
    virtual std::string DMAPI GetLastError() const = 0;
    virtual std::type_index DMAPI GetValueType() const = 0;
    
    // 流控制
    virtual void DMAPI Start() = 0;
    virtual void DMAPI Stop() = 0;
    virtual void DMAPI Reset() = 0;
    
    // 基础操作符工厂
    virtual IdmReactiveStream* DMAPI CreateMapStream(std::function<std::any(const std::any&)> mapper, std::type_index target_type) = 0;
    virtual IdmReactiveStream* DMAPI CreateFilterStream(std::function<bool(const std::any&)> predicate) = 0;
    virtual IdmReactiveStream* DMAPI CreateTakeStream(size_t count) = 0;
    virtual IdmReactiveStream* DMAPI CreateSkipStream(size_t count) = 0;
    virtual IdmReactiveStream* DMAPI CreateDebounceStream(uint32_t milliseconds) = 0;
    virtual IdmReactiveStream* DMAPI CreateThrottleStream(uint32_t milliseconds) = 0;
    virtual IdmReactiveStream* DMAPI CreateDistinctStream() = 0;
    virtual IdmReactiveStream* DMAPI CreateBufferStream(size_t size) = 0;
    
    // 组合操作
    virtual IdmReactiveStream* DMAPI CreateMergeStream(IdmReactiveStream* other) = 0;
    virtual IdmReactiveStream* DMAPI CreateConcatStream(IdmReactiveStream* other) = 0;
    virtual IdmReactiveStream* DMAPI CreateCombineLatestStream(IdmReactiveStream* other) = 0;
    virtual IdmReactiveStream* DMAPI CreateZipStream(IdmReactiveStream* other) = 0;
    
    // 调度器
    virtual IdmReactiveStream* DMAPI CreateObserveOnStream(IdmReactiveScheduler* scheduler) = 0;
    virtual IdmReactiveStream* DMAPI CreateSubscribeOnStream(IdmReactiveScheduler* scheduler) = 0;
    
    // 终端操作（返回Future包装的结果）
    virtual std::shared_ptr<std::future<std::any>> DMAPI GetFirst() = 0;
    virtual std::shared_ptr<std::future<std::any>> DMAPI GetLast() = 0;
    virtual std::shared_ptr<std::future<std::vector<std::any>>> DMAPI ToVector() = 0;
    virtual std::shared_ptr<std::future<bool>> DMAPI Any(std::function<bool(const std::any&)> predicate) = 0;
    virtual std::shared_ptr<std::future<bool>> DMAPI All(std::function<bool(const std::any&)> predicate) = 0;
    virtual std::shared_ptr<std::future<size_t>> DMAPI Count() = 0;
    
    // 统计信息
    virtual size_t DMAPI GetTotalEventCount() const = 0;
    virtual size_t DMAPI GetErrorEventCount() const = 0;
};

//=============================================================================
// 主题接口（既是观察者也是被观察者）
//=============================================================================
class IdmReactiveSubject : public IdmReactiveStream, public IdmReactiveObserver {
public:
    virtual ~IdmReactiveSubject() {}
    
    // 发射数据
    virtual void DMAPI EmitNext(const std::any& value, std::type_index value_type) = 0;
    virtual void DMAPI EmitError(const std::string& error) = 0;
    virtual void DMAPI EmitComplete() = 0;
    
    // 当前值（仅适用于BehaviorSubject）
    virtual bool DMAPI HasCurrentValue() const = 0;
    virtual std::any DMAPI GetCurrentValue() const = 0;
    virtual std::type_index DMAPI GetCurrentValueType() const = 0;
    
    // 缓冲区管理（仅适用于ReplaySubject）
    virtual void DMAPI SetBufferSize(size_t size) = 0;
    virtual size_t DMAPI GetBufferSize() const = 0;
    virtual void DMAPI ClearBuffer() = 0;
    
    // 主题类型
    enum SubjectType {
        SIMPLE,         // 普通主题
        BEHAVIOR,       // 行为主题（有初始值）
        REPLAY,         // 重播主题（有历史缓冲）
        ASYNC           // 异步主题（最后一个值）
    };
    
    virtual SubjectType DMAPI GetSubjectType() const = 0;
};

//=============================================================================
// 调度器接口
//=============================================================================
class IdmReactiveScheduler {
public:
    virtual ~IdmReactiveScheduler() {}
    virtual void DMAPI Release() = 0;
    
    // 调度任务
    virtual void DMAPI Schedule(std::function<void()> task) = 0;
    virtual void DMAPI ScheduleDelayed(std::function<void()> task, uint32_t delay_ms) = 0;
    virtual void DMAPI SchedulePeriodic(std::function<void()> task, uint32_t period_ms) = 0;
    virtual void DMAPI ScheduleAt(std::function<void()> task, uint64_t timestamp_ms) = 0;
    
    // 调度器类型
    enum Type {
        IMMEDIATE,      // 立即执行
        THREAD_POOL,    // 线程池
        SINGLE_THREAD,  // 单线程
        MAIN_THREAD,    // 主线程
        IO_THREAD,      // IO线程
        COMPUTATION     // 计算线程
    };
    
    virtual Type DMAPI GetSchedulerType() const = 0;
    virtual bool DMAPI IsRunning() const = 0;
    virtual void DMAPI Start() = 0;
    virtual void DMAPI Stop() = 0;
    virtual void DMAPI Shutdown() = 0;
    
    // 线程池配置（仅适用于THREAD_POOL类型）
    virtual void DMAPI SetThreadCount(size_t count) = 0;
    virtual size_t DMAPI GetThreadCount() const = 0;
    virtual size_t DMAPI GetActiveThreadCount() const = 0;
    virtual size_t DMAPI GetQueuedTaskCount() const = 0;
};

//=============================================================================
// 操作符接口
//=============================================================================
class IdmReactiveOperator {
public:
    virtual ~IdmReactiveOperator() {}
    virtual void DMAPI Release() = 0;
    
    // 操作符类型
    enum Type {
        MAP,
        FILTER,
        TAKE,
        SKIP,
        DEBOUNCE,
        THROTTLE,
        DISTINCT,
        BUFFER,
        MERGE,
        CONCAT,
        COMBINE_LATEST,
        ZIP,
        SWITCH_MAP,
        FLAT_MAP,
        SCAN,
        REDUCE
    };
    
    virtual Type DMAPI GetOperatorType() const = 0;
    virtual std::string DMAPI GetOperatorName() const = 0;
    virtual IdmReactiveStream* DMAPI Apply(IdmReactiveStream* source) = 0;
    virtual bool DMAPI IsStateful() const = 0;
    virtual void DMAPI Reset() = 0;
};

//=============================================================================
// 响应式工厂接口
//=============================================================================
class IdmReactiveFactory {
public:
    virtual ~IdmReactiveFactory() {}
    virtual void DMAPI Release() = 0;
    
    // 创建事件
    virtual IdmReactiveEvent* DMAPI CreateNextEvent(const std::any& value, std::type_index value_type) = 0;
    virtual IdmReactiveEvent* DMAPI CreateErrorEvent(const std::string& error) = 0;
    virtual IdmReactiveEvent* DMAPI CreateCompleteEvent() = 0;
    
    // 创建流
    virtual IdmReactiveStream* DMAPI CreateStream(std::type_index value_type) = 0;
    virtual IdmReactiveStream* DMAPI CreateFromVector(const std::vector<std::any>& values, std::type_index value_type) = 0;
    virtual IdmReactiveStream* DMAPI CreateFromRange(int32_t start, int32_t end, int32_t step = 1) = 0;
    virtual IdmReactiveStream* DMAPI CreateInterval(uint32_t period_ms, std::type_index value_type) = 0;
    virtual IdmReactiveStream* DMAPI CreateTimer(uint32_t delay_ms, const std::any& value, std::type_index value_type) = 0;
    virtual IdmReactiveStream* DMAPI CreateEmpty(std::type_index value_type) = 0;
    virtual IdmReactiveStream* DMAPI CreateNever(std::type_index value_type) = 0;
    virtual IdmReactiveStream* DMAPI CreateError(const std::string& error, std::type_index value_type) = 0;
    virtual IdmReactiveStream* DMAPI CreateFromCallback(std::function<void(std::function<void(const std::any&, std::type_index)>)> callback, std::type_index value_type) = 0;
    
    // 创建主题
    virtual IdmReactiveSubject* DMAPI CreateSubject(std::type_index value_type) = 0;
    virtual IdmReactiveSubject* DMAPI CreateBehaviorSubject(const std::any& initial_value, std::type_index value_type) = 0;
    virtual IdmReactiveSubject* DMAPI CreateReplaySubject(size_t buffer_size, std::type_index value_type) = 0;
    virtual IdmReactiveSubject* DMAPI CreateAsyncSubject(std::type_index value_type) = 0;
    
    // 创建调度器
    virtual IdmReactiveScheduler* DMAPI CreateScheduler(IdmReactiveScheduler::Type type) = 0;
    virtual IdmReactiveScheduler* DMAPI CreateThreadPoolScheduler(size_t thread_count) = 0;
    
    // 创建操作符
    virtual IdmReactiveOperator* DMAPI CreateMapOperator(std::function<std::any(const std::any&)> mapper, std::type_index target_type) = 0;
    virtual IdmReactiveOperator* DMAPI CreateFilterOperator(std::function<bool(const std::any&)> predicate) = 0;
    virtual IdmReactiveOperator* DMAPI CreateTakeOperator(size_t count) = 0;
    virtual IdmReactiveOperator* DMAPI CreateSkipOperator(size_t count) = 0;
    virtual IdmReactiveOperator* DMAPI CreateDebounceOperator(uint32_t milliseconds) = 0;
    virtual IdmReactiveOperator* DMAPI CreateThrottleOperator(uint32_t milliseconds) = 0;
    virtual IdmReactiveOperator* DMAPI CreateDistinctOperator() = 0;
    virtual IdmReactiveOperator* DMAPI CreateBufferOperator(size_t size) = 0;
    
    // 组合操作
    virtual IdmReactiveStream* DMAPI MergeStreams(const std::vector<IdmReactiveStream*>& streams) = 0;
    virtual IdmReactiveStream* DMAPI ConcatStreams(const std::vector<IdmReactiveStream*>& streams) = 0;
    virtual IdmReactiveStream* DMAPI ZipStreams(const std::vector<IdmReactiveStream*>& streams) = 0;
    virtual IdmReactiveStream* DMAPI RaceStreams(const std::vector<IdmReactiveStream*>& streams) = 0;
    virtual IdmReactiveStream* DMAPI CombineLatestStreams(const std::vector<IdmReactiveStream*>& streams) = 0;
};

//=============================================================================
// 响应式核心接口
//=============================================================================
class IdmReactiveCore {
public:
    virtual ~IdmReactiveCore() {}
    virtual void DMAPI Release() = 0;
    
    // 工厂访问
    virtual IdmReactiveFactory* DMAPI GetFactory() = 0;
    
    // 全局调度器管理
    virtual void DMAPI SetDefaultScheduler(IdmReactiveScheduler* scheduler) = 0;
    virtual IdmReactiveScheduler* DMAPI GetDefaultScheduler() = 0;
    virtual IdmReactiveScheduler* DMAPI GetIOScheduler() = 0;
    virtual IdmReactiveScheduler* DMAPI GetComputationScheduler() = 0;
    virtual IdmReactiveScheduler* DMAPI GetMainThreadScheduler() = 0;
    
    // 全局配置
    virtual void DMAPI SetGlobalErrorHandler(std::function<void(const std::string&)> handler) = 0;
    virtual void DMAPI SetLogLevel(int level) = 0;
    virtual void DMAPI SetMaxBufferSize(size_t size) = 0;
    virtual void DMAPI SetDefaultDebounceTime(uint32_t milliseconds) = 0;
    virtual void DMAPI SetDefaultThrottleTime(uint32_t milliseconds) = 0;
    
    // 统计信息
    virtual size_t DMAPI GetActiveStreamCount() const = 0;
    virtual size_t DMAPI GetActiveObserverCount() const = 0;
    virtual size_t DMAPI GetActiveSubjectCount() const = 0;
    virtual size_t DMAPI GetTotalEventsProcessed() const = 0;
    virtual size_t DMAPI GetTotalErrorsOccurred() const = 0;
    
    // 性能监控
    virtual double DMAPI GetAverageEventProcessingTime() const = 0;
    virtual size_t DMAPI GetPeakMemoryUsage() const = 0;
    virtual size_t DMAPI GetCurrentMemoryUsage() const = 0;
    
    // 生命周期管理
    virtual void DMAPI Initialize() = 0;
    virtual void DMAPI Shutdown() = 0;
    virtual bool DMAPI IsInitialized() const = 0;
    virtual void DMAPI GarbageCollect() = 0;
    
    // 调试支持
    virtual void DMAPI EnableDebugMode(bool enable) = 0;
    virtual bool DMAPI IsDebugModeEnabled() const = 0;
    virtual void DMAPI DumpStreamGraph(const std::string& filename) = 0;
    virtual void DMAPI DumpStatistics(const std::string& filename) = 0;
    virtual std::string DMAPI GetDebugInfo() const = 0;
    
    // 类型注册（用于序列化等高级功能）
    virtual void DMAPI RegisterType(std::type_index type, const std::string& type_name) = 0;
    virtual std::string DMAPI GetTypeName(std::type_index type) const = 0;
    virtual bool DMAPI IsTypeRegistered(std::type_index type) const = 0;
};

//=============================================================================
// 工厂函数声明
//=============================================================================
DMEXPORT_DLL dmReactiveCorePtr DMAPI dmReactiveGetCore();
typedef IdmReactiveCore* (DMAPI* PFN_dmReactiveGetCore)();

//=============================================================================
// 便利宏定义
//=============================================================================
#define DM_REACTIVE_SUBSCRIBE(stream, observer) (stream)->Subscribe(observer)
#define DM_REACTIVE_EMIT_NEXT(subject, value, type) (subject)->EmitNext(std::any(value), std::type_index(typeid(type)))
#define DM_REACTIVE_EMIT_ERROR(subject, error) (subject)->EmitError(error)
#define DM_REACTIVE_EMIT_COMPLETE(subject) (subject)->EmitComplete()

// 类型安全的模板包装宏
#define DM_REACTIVE_CREATE_STREAM(factory, T) (factory)->CreateStream(std::type_index(typeid(T)))
#define DM_REACTIVE_CREATE_SUBJECT(factory, T) (factory)->CreateSubject(std::type_index(typeid(T)))
#define DM_REACTIVE_CREATE_BEHAVIOR_SUBJECT(factory, initial_value, T) (factory)->CreateBehaviorSubject(std::any(initial_value), std::type_index(typeid(T)))

//=============================================================================
// 类型traits支持
//=============================================================================
template<typename T>
struct ReactiveTypeTraits {
    static std::type_index GetTypeIndex() { return std::type_index(typeid(T)); }
    static std::any ToAny(const T& value) { return std::any(value); }
    static T FromAny(const std::any& value) { return std::any_cast<T>(value); }
    static bool TryFromAny(const std::any& value, T& output) {
        try {
            output = std::any_cast<T>(value);
            return true;
        } catch (...) {
            return false;
        }
    }
};

//=============================================================================
// 辅助函数模板
//=============================================================================
template<typename T>
inline std::function<std::any(const std::any&)> MakeReactiveMapper(std::function<T(const T&)> mapper) {
    return [mapper](const std::any& input) -> std::any {
        T typed_input = std::any_cast<T>(input);
        T result = mapper(typed_input);
        return std::any(result);
    };
}

template<typename T>
inline std::function<bool(const std::any&)> MakeReactivePredicate(std::function<bool(const T&)> predicate) {
    return [predicate](const std::any& input) -> bool {
        T typed_input = std::any_cast<T>(input);
        return predicate(typed_input);
    };
}

#endif // __DMREACTIVE_H_INCLUDE__