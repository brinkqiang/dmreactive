
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
#ifndef __LIBDMREACTIVE_IMPL_H_INCLUDE__
#define __LIBDMREACTIVE_IMPL_H_INCLUDE__

#include "dmreactive.h"
#include <mutex>
#include <list>
#include <map>
#include <set>
#include <deque>
#include <condition_variable>

//=============================================================================
// 辅助类和前向声明
//=============================================================================
class DmReactiveStreamImpl;
class DmReactiveFactoryImpl;

// 线程安全的观察者列表
class ObserverList {
public:
    void Add(IdmReactiveObserver* observer);
    void Remove(IdmReactiveObserver* observer);
    void Clear();
    std::vector<IdmReactiveObserver*> GetSnapshot() const;
    size_t GetCount() const;

private:
    mutable std::mutex m_mutex;
    std::list<IdmReactiveObserver*> m_observers;
};

//=============================================================================
// 实现类声明
//=============================================================================

class DmReactiveEventImpl : public IdmReactiveEvent {
public:
    DmReactiveEventImpl(Type type, const std::any& value, std::type_index value_type, const std::string& error);
    virtual ~DmReactiveEventImpl() {}

    virtual void DMAPI Release() override;
    virtual Type DMAPI GetType() const override;
    virtual std::any DMAPI GetValue() const override;
    virtual std::string DMAPI GetError() const override;
    virtual std::type_index DMAPI GetValueType() const override;

private:
    Type m_type;
    std::any m_value;
    std::type_index m_value_type;
    std::string m_error;
};

class DmReactiveObserverImpl : public IdmReactiveObserver {
public:
    DmReactiveObserverImpl(std::type_index expected_type, std::function<void(IdmReactiveEvent*)> on_event);
    virtual ~DmReactiveObserverImpl() {}

    virtual void DMAPI Release() override;
    virtual void DMAPI OnEvent(IdmReactiveEvent* event) override;
    virtual std::type_index DMAPI GetExpectedType() const override;
    virtual bool DMAPI IsActive() const override;
    virtual void DMAPI SetActive(bool active) override;

private:
    std::type_index m_expected_type;
    std::function<void(IdmReactiveEvent*)> m_on_event;
    std::atomic<bool> m_is_active;
};

class DmReactiveStreamImpl : public IdmReactiveStream {
public:
    DmReactiveStreamImpl(std::type_index value_type);
    virtual ~DmReactiveStreamImpl();

    virtual void DMAPI Release() override;
    virtual void DMAPI Subscribe(IdmReactiveObserver* observer) override;
    virtual void DMAPI Unsubscribe(IdmReactiveObserver* observer) override;
    virtual void DMAPI UnsubscribeAll() override;
    virtual size_t DMAPI GetObserverCount() const override;
    virtual bool DMAPI IsActive() const override;
    virtual bool DMAPI IsCompleted() const override;
    virtual bool DMAPI HasError() const override;
    virtual std::string DMAPI GetLastError() const override;
    virtual std::type_index DMAPI GetValueType() const override;
    virtual void DMAPI Start() override;
    virtual void DMAPI Stop() override;
    virtual void DMAPI Reset() override;

    virtual IdmReactiveStream* DMAPI CreateMapStream(std::function<std::any(const std::any&)> mapper, std::type_index target_type) override;
    virtual IdmReactiveStream* DMAPI CreateFilterStream(std::function<bool(const std::any&)> predicate) override;
    virtual IdmReactiveStream* DMAPI CreateTakeStream(size_t count) override;
    virtual IdmReactiveStream* DMAPI CreateSkipStream(size_t count) override;
    virtual IdmReactiveStream* DMAPI CreateDebounceStream(uint32_t milliseconds) override;
    virtual IdmReactiveStream* DMAPI CreateThrottleStream(uint32_t milliseconds) override;
    virtual IdmReactiveStream* DMAPI CreateDistinctStream() override;
    virtual IdmReactiveStream* DMAPI CreateBufferStream(size_t size) override;
    virtual IdmReactiveStream* DMAPI CreateMergeStream(IdmReactiveStream* other) override;
    virtual IdmReactiveStream* DMAPI CreateConcatStream(IdmReactiveStream* other) override;
    virtual IdmReactiveStream* DMAPI CreateCombineLatestStream(IdmReactiveStream* other) override;
    virtual IdmReactiveStream* DMAPI CreateZipStream(IdmReactiveStream* other) override;
    virtual IdmReactiveStream* DMAPI CreateObserveOnStream(IdmReactiveScheduler* scheduler) override;
    virtual IdmReactiveStream* DMAPI CreateSubscribeOnStream(IdmReactiveScheduler* scheduler) override;

    virtual std::shared_ptr<std::future<std::any>> DMAPI GetFirst() override;
    virtual std::shared_ptr<std::future<std::any>> DMAPI GetLast() override;
    virtual std::shared_ptr<std::future<std::vector<std::any>>> DMAPI ToVector() override;
    virtual std::shared_ptr<std::future<bool>> DMAPI Any(std::function<bool(const std::any&)> predicate) override;
    virtual std::shared_ptr<std::future<bool>> DMAPI All(std::function<bool(const std::any&)> predicate) override;
    virtual std::shared_ptr<std::future<size_t>> DMAPI Count() override;

    virtual size_t DMAPI GetTotalEventCount() const override;
    virtual size_t DMAPI GetErrorEventCount() const override;

    virtual void NotifyObservers(IdmReactiveEvent* event);

protected:
    std::type_index m_value_type;
    ObserverList m_observers;
    std::atomic<bool> m_is_active{true};
    std::atomic<bool> m_is_completed{false};
    std::atomic<bool> m_has_error{false};
    std::string m_last_error;
    std::atomic<size_t> m_total_events{0};
    std::atomic<size_t> m_error_events{0};
    mutable std::mutex m_state_mutex;
};

class DmReactiveSubjectImpl : public IdmReactiveSubject {
public:
    DmReactiveSubjectImpl(std::type_index value_type, SubjectType subject_type);
    virtual ~DmReactiveSubjectImpl() {}

    // IdmReactiveStream
    virtual void DMAPI Release() override;
    virtual void DMAPI Subscribe(IdmReactiveObserver* observer) override;
    virtual void DMAPI Unsubscribe(IdmReactiveObserver* observer) override;
    virtual void DMAPI UnsubscribeAll() override;
    virtual size_t DMAPI GetObserverCount() const override;
    virtual bool DMAPI IsActive() const override;
    virtual bool DMAPI IsCompleted() const override;
    virtual bool DMAPI HasError() const override;
    virtual std::string DMAPI GetLastError() const override;
    virtual std::type_index DMAPI GetValueType() const override;
    virtual void DMAPI Start() override;
    virtual void DMAPI Stop() override;
    virtual void DMAPI Reset() override;
    virtual IdmReactiveStream* DMAPI CreateMapStream(std::function<std::any(const std::any&)> mapper, std::type_index target_type) override;
    virtual IdmReactiveStream* DMAPI CreateFilterStream(std::function<bool(const std::any&)> predicate) override;
    virtual IdmReactiveStream* DMAPI CreateTakeStream(size_t count) override;
    virtual IdmReactiveStream* DMAPI CreateSkipStream(size_t count) override;
    virtual IdmReactiveStream* DMAPI CreateDebounceStream(uint32_t milliseconds) override;
    virtual IdmReactiveStream* DMAPI CreateThrottleStream(uint32_t milliseconds) override;
    virtual IdmReactiveStream* DMAPI CreateDistinctStream() override;
    virtual IdmReactiveStream* DMAPI CreateBufferStream(size_t size) override;
    virtual IdmReactiveStream* DMAPI CreateMergeStream(IdmReactiveStream* other) override;
    virtual IdmReactiveStream* DMAPI CreateConcatStream(IdmReactiveStream* other) override;
    virtual IdmReactiveStream* DMAPI CreateCombineLatestStream(IdmReactiveStream* other) override;
    virtual IdmReactiveStream* DMAPI CreateZipStream(IdmReactiveStream* other) override;
    virtual IdmReactiveStream* DMAPI CreateObserveOnStream(IdmReactiveScheduler* scheduler) override;
    virtual IdmReactiveStream* DMAPI CreateSubscribeOnStream(IdmReactiveScheduler* scheduler) override;
    virtual std::shared_ptr<std::future<std::any>> DMAPI GetFirst() override;
    virtual std::shared_ptr<std::future<std::any>> DMAPI GetLast() override;
    virtual std::shared_ptr<std::future<std::vector<std::any>>> DMAPI ToVector() override;
    virtual std::shared_ptr<std::future<bool>> DMAPI Any(std::function<bool(const std::any&)> predicate) override;
    virtual std::shared_ptr<std::future<bool>> DMAPI All(std::function<bool(const std::any&)> predicate) override;
    virtual std::shared_ptr<std::future<size_t>> DMAPI Count() override;
    virtual size_t DMAPI GetTotalEventCount() const override;
    virtual size_t DMAPI GetErrorEventCount() const override;

    // IdmReactiveObserver
    virtual void DMAPI OnEvent(IdmReactiveEvent* event) override;
    virtual std::type_index DMAPI GetExpectedType() const override;
    virtual void DMAPI SetActive(bool active) override; // <<< 这是缺失的函数

    // IdmReactiveSubject
    virtual void DMAPI EmitNext(const std::any& value, std::type_index value_type) override;
    virtual void DMAPI EmitError(const std::string& error) override;
    virtual void DMAPI EmitComplete() override;
    virtual bool DMAPI HasCurrentValue() const override;
    virtual std::any DMAPI GetCurrentValue() const override;
    virtual std::type_index DMAPI GetCurrentValueType() const override;
    virtual void DMAPI SetBufferSize(size_t size) override;
    virtual size_t DMAPI GetBufferSize() const override;
    virtual void DMAPI ClearBuffer() override;
    virtual SubjectType DMAPI GetSubjectType() const override;

private:
    DmReactiveStreamImpl m_stream_impl;
    SubjectType m_subject_type;
};

class DmThreadPoolSchedulerImpl : public IdmReactiveScheduler {
public:
    DmThreadPoolSchedulerImpl(size_t thread_count);
    virtual ~DmThreadPoolSchedulerImpl();

    virtual void DMAPI Release() override;
    virtual void DMAPI Schedule(std::function<void()> task) override;
    virtual void DMAPI ScheduleDelayed(std::function<void()> task, uint32_t delay_ms) override;
    virtual void DMAPI SchedulePeriodic(std::function<void()> task, uint32_t period_ms) override;
    virtual void DMAPI ScheduleAt(std::function<void()> task, uint64_t timestamp_ms) override;
    virtual Type DMAPI GetSchedulerType() const override { return THREAD_POOL; }
    virtual bool DMAPI IsRunning() const override;
    virtual void DMAPI Start() override;
    virtual void DMAPI Stop() override;
    virtual void DMAPI Shutdown() override;
    virtual void DMAPI SetThreadCount(size_t count) override;
    virtual size_t DMAPI GetThreadCount() const override;
    virtual size_t DMAPI GetActiveThreadCount() const override;
    virtual size_t DMAPI GetQueuedTaskCount() const override;

private:
    void WorkerThread();
    std::vector<std::thread> m_threads;
    std::queue<std::function<void()>> m_tasks;
    mutable std::mutex m_mutex;
    std::condition_variable m_condition;
    std::atomic<bool> m_running{false};
    std::atomic<size_t> m_active_threads{0};
};

class DmReactiveFactoryImpl : public IdmReactiveFactory {
public:
    virtual ~DmReactiveFactoryImpl() {}
    virtual void DMAPI Release() override;
    virtual IdmReactiveEvent* DMAPI CreateNextEvent(const std::any& value, std::type_index value_type) override;
    virtual IdmReactiveEvent* DMAPI CreateErrorEvent(const std::string& error) override;
    virtual IdmReactiveEvent* DMAPI CreateCompleteEvent() override;
    virtual IdmReactiveStream* DMAPI CreateStream(std::type_index value_type) override;
    virtual IdmReactiveStream* DMAPI CreateFromVector(const std::vector<std::any>& values, std::type_index value_type) override;
    virtual IdmReactiveStream* DMAPI CreateFromRange(int32_t start, int32_t end, int32_t step = 1) override;
    virtual IdmReactiveStream* DMAPI CreateInterval(uint32_t period_ms, std::type_index value_type) override;
    virtual IdmReactiveStream* DMAPI CreateTimer(uint32_t delay_ms, const std::any& value, std::type_index value_type) override;
    virtual IdmReactiveStream* DMAPI CreateEmpty(std::type_index value_type) override;
    virtual IdmReactiveStream* DMAPI CreateNever(std::type_index value_type) override;
    virtual IdmReactiveStream* DMAPI CreateError(const std::string& error, std::type_index value_type) override;
    virtual IdmReactiveStream* DMAPI CreateFromCallback(std::function<void(std::function<void(const std::any&, std::type_index)>)> callback, std::type_index value_type) override;
    virtual IdmReactiveSubject* DMAPI CreateSubject(std::type_index value_type) override;
    virtual IdmReactiveSubject* DMAPI CreateBehaviorSubject(const std::any& initial_value, std::type_index value_type) override;
    virtual IdmReactiveSubject* DMAPI CreateReplaySubject(size_t buffer_size, std::type_index value_type) override;
    virtual IdmReactiveSubject* DMAPI CreateAsyncSubject(std::type_index value_type) override;
    virtual IdmReactiveScheduler* DMAPI CreateScheduler(IdmReactiveScheduler::Type type) override;
    virtual IdmReactiveScheduler* DMAPI CreateThreadPoolScheduler(size_t thread_count) override;
    virtual IdmReactiveOperator* DMAPI CreateMapOperator(std::function<std::any(const std::any&)> mapper, std::type_index target_type) override;
    virtual IdmReactiveOperator* DMAPI CreateFilterOperator(std::function<bool(const std::any&)> predicate) override;
    virtual IdmReactiveOperator* DMAPI CreateTakeOperator(size_t count) override;
    virtual IdmReactiveOperator* DMAPI CreateSkipOperator(size_t count) override;
    virtual IdmReactiveOperator* DMAPI CreateDebounceOperator(uint32_t milliseconds) override;
    virtual IdmReactiveOperator* DMAPI CreateThrottleOperator(uint32_t milliseconds) override;
    virtual IdmReactiveOperator* DMAPI CreateDistinctOperator() override;
    virtual IdmReactiveOperator* DMAPI CreateBufferOperator(size_t size) override;
    virtual IdmReactiveStream* DMAPI MergeStreams(const std::vector<IdmReactiveStream*>& streams) override;
    virtual IdmReactiveStream* DMAPI ConcatStreams(const std::vector<IdmReactiveStream*>& streams) override;
    virtual IdmReactiveStream* DMAPI ZipStreams(const std::vector<IdmReactiveStream*>& streams) override;
    virtual IdmReactiveStream* DMAPI RaceStreams(const std::vector<IdmReactiveStream*>& streams) override;
    virtual IdmReactiveStream* DMAPI CombineLatestStreams(const std::vector<IdmReactiveStream*>& streams) override;
};

class DmReactiveCoreImpl : public IdmReactiveCore {
public:
    DmReactiveCoreImpl();
    virtual ~DmReactiveCoreImpl();

    virtual void DMAPI Release() override;
    virtual IdmReactiveFactory* DMAPI GetFactory() override;
    virtual void DMAPI Initialize() override;
    virtual void DMAPI Shutdown() override;
    virtual bool DMAPI IsInitialized() const override;
    virtual void DMAPI SetDefaultScheduler(IdmReactiveScheduler* scheduler) override;
    virtual IdmReactiveScheduler* DMAPI GetDefaultScheduler() override;
    virtual IdmReactiveScheduler* DMAPI GetIOScheduler() override;
    virtual IdmReactiveScheduler* DMAPI GetComputationScheduler() override;
    virtual IdmReactiveScheduler* DMAPI GetMainThreadScheduler() override;
    virtual void DMAPI SetGlobalErrorHandler(std::function<void(const std::string&)> handler) override;
    virtual void DMAPI SetLogLevel(int level) override;
    virtual void DMAPI SetMaxBufferSize(size_t size) override;
    virtual void DMAPI SetDefaultDebounceTime(uint32_t milliseconds) override;
    virtual void DMAPI SetDefaultThrottleTime(uint32_t milliseconds) override;
    virtual size_t DMAPI GetActiveStreamCount() const override;
    virtual size_t DMAPI GetActiveObserverCount() const override;
    virtual size_t DMAPI GetActiveSubjectCount() const override;
    virtual size_t DMAPI GetTotalEventsProcessed() const override;
    virtual size_t DMAPI GetTotalErrorsOccurred() const override;
    virtual double DMAPI GetAverageEventProcessingTime() const override;
    virtual size_t DMAPI GetPeakMemoryUsage() const override;
    virtual size_t DMAPI GetCurrentMemoryUsage() const override;
    virtual void DMAPI GarbageCollect() override;
    virtual void DMAPI EnableDebugMode(bool enable) override;
    virtual bool DMAPI IsDebugModeEnabled() const override;
    virtual void DMAPI DumpStreamGraph(const std::string& filename) override;
    virtual void DMAPI DumpStatistics(const std::string& filename) override;
    virtual std::string DMAPI GetDebugInfo() const override;
    virtual void DMAPI RegisterType(std::type_index type, const std::string& type_name) override;
    virtual std::string DMAPI GetTypeName(std::type_index type) const override;
    virtual bool DMAPI IsTypeRegistered(std::type_index type) const override;

private:
    DmReactiveFactoryImpl m_factory;
    std::atomic<bool> m_is_initialized{false};
    dmReactiveSchedulerPtr m_default_scheduler;
    dmReactiveSchedulerPtr m_io_scheduler;
    dmReactiveSchedulerPtr m_computation_scheduler;
};

#endif // __LIBDMREACTIVE_IMPL_H_INCLUDE__