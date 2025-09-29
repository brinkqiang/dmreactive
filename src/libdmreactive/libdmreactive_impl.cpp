#include "libdmreactive_impl.h"
#include <iostream>
#include <stdexcept>
#include <chrono>

//=============================================================================
// ObserverList
//=============================================================================
void ObserverList::Add(IdmReactiveObserver* observer) {
    if (!observer) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    m_observers.push_back(observer);
}
void ObserverList::Remove(IdmReactiveObserver* observer) {
    if (!observer) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    m_observers.remove(observer);
}
void ObserverList::Clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_observers.clear();
}
std::vector<IdmReactiveObserver*> ObserverList::GetSnapshot() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return std::vector<IdmReactiveObserver*>(m_observers.begin(), m_observers.end());
}
size_t ObserverList::GetCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_observers.size();
}

//=============================================================================
// DmReactiveEventImpl
//=============================================================================
DmReactiveEventImpl::DmReactiveEventImpl(Type type, const std::any& value, std::type_index value_type, const std::string& error)
    : m_type(type), m_value(value), m_value_type(value_type), m_error(error) {
}
void DmReactiveEventImpl::Release() { delete this; }
IdmReactiveEvent::Type DmReactiveEventImpl::GetType() const { return m_type; }
std::any DmReactiveEventImpl::GetValue() const { return m_value; }
std::string DmReactiveEventImpl::GetError() const { return m_error; }
std::type_index DmReactiveEventImpl::GetValueType() const { return m_value_type; }

//=============================================================================
// DmReactiveObserverImpl
//=============================================================================
DmReactiveObserverImpl::DmReactiveObserverImpl(std::type_index expected_type, std::function<void(IdmReactiveEvent*)> on_event)
    : m_expected_type(expected_type), m_on_event(on_event), m_is_active(true) {
}
void DmReactiveObserverImpl::Release() { delete this; }
void DmReactiveObserverImpl::OnEvent(IdmReactiveEvent* event) {
    if (m_is_active && m_on_event) {
        m_on_event(event);
    }
}
std::type_index DmReactiveObserverImpl::GetExpectedType() const { return m_expected_type; }
bool DmReactiveObserverImpl::IsActive() const { return m_is_active; }
void DmReactiveObserverImpl::SetActive(bool active) { m_is_active = active; }

//=============================================================================
// DmReactiveStreamImpl
//=============================================================================
DmReactiveStreamImpl::DmReactiveStreamImpl(std::type_index value_type) : m_value_type(value_type) {}
DmReactiveStreamImpl::~DmReactiveStreamImpl() { UnsubscribeAll(); }
void DmReactiveStreamImpl::Release() { delete this; }
void DmReactiveStreamImpl::Subscribe(IdmReactiveObserver* observer) { m_observers.Add(observer); }
void DmReactiveStreamImpl::Unsubscribe(IdmReactiveObserver* observer) { m_observers.Remove(observer); }
void DmReactiveStreamImpl::UnsubscribeAll() { m_observers.Clear(); }
size_t DmReactiveStreamImpl::GetObserverCount() const { return m_observers.GetCount(); }
bool DmReactiveStreamImpl::IsActive() const { return m_is_active; }
bool DmReactiveStreamImpl::IsCompleted() const { return m_is_completed; }
bool DmReactiveStreamImpl::HasError() const { return m_has_error; }
std::string DmReactiveStreamImpl::GetLastError() const { std::lock_guard<std::mutex> lock(m_state_mutex); return m_last_error; }
std::type_index DmReactiveStreamImpl::GetValueType() const { return m_value_type; }
void DmReactiveStreamImpl::Start() { m_is_active = true; }
void DmReactiveStreamImpl::Stop() { m_is_active = false; }
void DmReactiveStreamImpl::Reset() {
    std::lock_guard<std::mutex> lock(m_state_mutex);
    m_is_active = true;
    m_is_completed = false;
    m_has_error = false;
    m_last_error.clear();
    m_total_events = 0;
    m_error_events = 0;
}
size_t DmReactiveStreamImpl::GetTotalEventCount() const { return m_total_events; }
size_t DmReactiveStreamImpl::GetErrorEventCount() const { return m_error_events; }

void DmReactiveStreamImpl::NotifyObservers(IdmReactiveEvent* event) {
    if (!m_is_active) return;

    m_total_events++;
    if (event->GetType() == IdmReactiveEvent::DMERROR) {
        std::lock_guard<std::mutex> lock(m_state_mutex);
        m_error_events++;
        m_has_error = true;
        m_last_error = event->GetError();
        m_is_active = false;
    }
    else if (event->GetType() == IdmReactiveEvent::DMCOMPLETE) {
        std::lock_guard<std::mutex> lock(m_state_mutex);
        m_is_completed = true;
        m_is_active = false;
    }

    auto current_observers = m_observers.GetSnapshot();
    for (auto* obs : current_observers) {
        if (obs && obs->IsActive()) {
            obs->OnEvent(event);
        }
    }
}

//--- Operator Stream Base ---
class DmOperatorStreamImpl : public DmReactiveStreamImpl {
protected:
    IdmReactiveStream* m_source;
public:
    DmOperatorStreamImpl(IdmReactiveStream* source, std::type_index new_type)
        : DmReactiveStreamImpl(new_type), m_source(source) {
    }
};

//--- Map ---
class DmMapStreamImpl : public DmOperatorStreamImpl {
    std::function<std::any(const std::any&)> m_mapper;
public:
    DmMapStreamImpl(IdmReactiveStream* source, std::function<std::any(const std::any&)> mapper, std::type_index target_type)
        : DmOperatorStreamImpl(source, target_type), m_mapper(mapper) {
    }

    void DMAPI Subscribe(IdmReactiveObserver* downstream_observer) override {
        m_observers.Add(downstream_observer);
        auto op_observer = new DmReactiveObserverImpl(m_source->GetValueType(),
            [this](IdmReactiveEvent* event) {
                if (event->GetType() == IdmReactiveEvent::DMNEXT) {
                    try {
                        dmReactiveEventPtr next(new DmReactiveEventImpl(IdmReactiveEvent::DMNEXT, m_mapper(event->GetValue()), m_value_type, ""));
                        this->NotifyObservers(next.get());
                    }
                    catch (const std::exception& e) {
                        dmReactiveEventPtr error(new DmReactiveEventImpl(IdmReactiveEvent::DMERROR, {}, m_value_type, e.what()));
                        this->NotifyObservers(error.get());
                    }
                }
                else {
                    this->NotifyObservers(event);
                }
            });
        m_source->Subscribe(op_observer);
    }
};
IdmReactiveStream* DmReactiveStreamImpl::CreateMapStream(std::function<std::any(const std::any&)> m, std::type_index t) { return new DmMapStreamImpl(this, m, t); }

//--- Filter ---
class DmFilterStreamImpl : public DmOperatorStreamImpl {
    std::function<bool(const std::any&)> m_predicate;
public:
    DmFilterStreamImpl(IdmReactiveStream* source, std::function<bool(const std::any&)> predicate)
        : DmOperatorStreamImpl(source, source->GetValueType()), m_predicate(predicate) {
    }
    void DMAPI Subscribe(IdmReactiveObserver* downstream_observer) override {
        m_observers.Add(downstream_observer);
        auto op_observer = new DmReactiveObserverImpl(m_source->GetValueType(),
            [this](IdmReactiveEvent* event) {
                if (event->GetType() == IdmReactiveEvent::DMNEXT) {
                    if (m_predicate(event->GetValue())) {
                        this->NotifyObservers(event);
                    }
                }
                else {
                    this->NotifyObservers(event);
                }
            });
        m_source->Subscribe(op_observer);
    }
};
IdmReactiveStream* DmReactiveStreamImpl::CreateFilterStream(std::function<bool(const std::any&)> p) { return new DmFilterStreamImpl(this, p); }

//--- Take ---
class DmTakeStreamImpl : public DmOperatorStreamImpl {
    size_t m_count;
    std::atomic<size_t> m_taken{0};
public:
    DmTakeStreamImpl(IdmReactiveStream* source, size_t count) : DmOperatorStreamImpl(source, source->GetValueType()), m_count(count) {}
    void DMAPI Subscribe(IdmReactiveObserver* downstream_observer) override {
        m_observers.Add(downstream_observer);
        auto op_observer = new DmReactiveObserverImpl(m_source->GetValueType(),
            [this](IdmReactiveEvent* event) {
                if (event->GetType() == IdmReactiveEvent::DMNEXT && m_taken < m_count) {
                    m_taken++;
                    this->NotifyObservers(event);
                    if (m_taken >= m_count) {
                        dmReactiveEventPtr complete(new DmReactiveEventImpl(IdmReactiveEvent::DMCOMPLETE, {}, m_value_type, ""));
                        this->NotifyObservers(complete.get());
                    }
                }
                else if (event->GetType() != IdmReactiveEvent::DMNEXT) {
                    this->NotifyObservers(event);
                }
            });
        m_source->Subscribe(op_observer);
    }
};
IdmReactiveStream* DmReactiveStreamImpl::CreateTakeStream(size_t c) { return new DmTakeStreamImpl(this, c); }

//--- ToVector (Terminal) ---
std::shared_ptr<std::future<std::vector<std::any>>> DmReactiveStreamImpl::ToVector() {
    auto promise = std::make_shared<std::promise<std::vector<std::any>>>();
    auto future = std::make_shared<std::future<std::vector<std::any>>>(promise->get_future());
    auto collector = std::make_shared<std::vector<std::any>>();

    auto observer = new DmReactiveObserverImpl(m_value_type,
        [promise, collector, this](IdmReactiveEvent* event) {
            if (event->GetType() == IdmReactiveEvent::DMNEXT) {
                collector->push_back(event->GetValue());
            }
            else if (event->GetType() == IdmReactiveEvent::DMCOMPLETE) {
                promise->set_value(*collector);
            }
            else if (event->GetType() == IdmReactiveEvent::DMERROR) {
                promise->set_exception(std::make_exception_ptr(std::runtime_error(event->GetError())));
            }
        });

    this->Subscribe(observer);
    return future;
}

//--- Other placeholders returning a valid but inactive stream ---
IdmReactiveStream* CreateEmptyStream(std::type_index t) {
    auto stream = new DmReactiveStreamImpl(t);
    dmReactiveEventPtr complete(new DmReactiveEventImpl(IdmReactiveEvent::DMCOMPLETE, {}, t, ""));
    stream->NotifyObservers(complete.get());
    return stream;
}

IdmReactiveStream* DmReactiveStreamImpl::CreateSkipStream(size_t) { return CreateEmptyStream(m_value_type); }
IdmReactiveStream* DmReactiveStreamImpl::CreateDebounceStream(uint32_t) { return CreateEmptyStream(m_value_type); }
IdmReactiveStream* DmReactiveStreamImpl::CreateThrottleStream(uint32_t) { return CreateEmptyStream(m_value_type); }
IdmReactiveStream* DmReactiveStreamImpl::CreateDistinctStream() { return CreateEmptyStream(m_value_type); }
IdmReactiveStream* DmReactiveStreamImpl::CreateBufferStream(size_t) { return CreateEmptyStream(m_value_type); }
IdmReactiveStream* DmReactiveStreamImpl::CreateMergeStream(IdmReactiveStream*) { return CreateEmptyStream(m_value_type); }
IdmReactiveStream* DmReactiveStreamImpl::CreateConcatStream(IdmReactiveStream*) { return CreateEmptyStream(m_value_type); }
IdmReactiveStream* DmReactiveStreamImpl::CreateCombineLatestStream(IdmReactiveStream*) { return CreateEmptyStream(m_value_type); }
IdmReactiveStream* DmReactiveStreamImpl::CreateZipStream(IdmReactiveStream*) { return CreateEmptyStream(m_value_type); }
IdmReactiveStream* DmReactiveStreamImpl::CreateObserveOnStream(IdmReactiveScheduler*) { return CreateEmptyStream(m_value_type); }
IdmReactiveStream* DmReactiveStreamImpl::CreateSubscribeOnStream(IdmReactiveScheduler*) { return CreateEmptyStream(m_value_type); }
std::shared_ptr<std::future<std::any>> DmReactiveStreamImpl::GetFirst() { auto p = std::make_shared<std::promise<std::any>>(); p->set_exception(std::make_exception_ptr(std::runtime_error("Not Implemented"))); return std::make_shared<std::future<std::any>>(p->get_future()); }
std::shared_ptr<std::future<std::any>> DmReactiveStreamImpl::GetLast() { auto p = std::make_shared<std::promise<std::any>>(); p->set_exception(std::make_exception_ptr(std::runtime_error("Not Implemented"))); return std::make_shared<std::future<std::any>>(p->get_future()); }
std::shared_ptr<std::future<bool>> DmReactiveStreamImpl::Any(std::function<bool(const std::any&)>) { auto p = std::make_shared<std::promise<bool>>(); p->set_exception(std::make_exception_ptr(std::runtime_error("Not Implemented"))); return std::make_shared<std::future<bool>>(p->get_future()); }
std::shared_ptr<std::future<bool>> DmReactiveStreamImpl::All(std::function<bool(const std::any&)>) { auto p = std::make_shared<std::promise<bool>>(); p->set_exception(std::make_exception_ptr(std::runtime_error("Not Implemented"))); return std::make_shared<std::future<bool>>(p->get_future()); }
std::shared_ptr<std::future<size_t>> DmReactiveStreamImpl::Count() { auto p = std::make_shared<std::promise<size_t>>(); p->set_exception(std::make_exception_ptr(std::runtime_error("Not Implemented"))); return std::make_shared<std::future<size_t>>(p->get_future()); }

//=============================================================================
// DmReactiveSubjectImpl
//=============================================================================
DmReactiveSubjectImpl::DmReactiveSubjectImpl(std::type_index value_type, SubjectType subject_type)
    : m_stream_impl(value_type), m_subject_type(subject_type) {
}

// --- Corrected Section ---
void DmReactiveSubjectImpl::SetActive(bool active) {
    if (active) {
        m_stream_impl.Start();
    }
    else {
        m_stream_impl.Stop();
    }
}
// --- End Corrected Section ---

// Delegate Stream methods to implementation
void DmReactiveSubjectImpl::Release() { delete this; }
void DmReactiveSubjectImpl::Subscribe(IdmReactiveObserver* o) { m_stream_impl.Subscribe(o); }
void DmReactiveSubjectImpl::Unsubscribe(IdmReactiveObserver* o) { m_stream_impl.Unsubscribe(o); }
void DmReactiveSubjectImpl::UnsubscribeAll() { m_stream_impl.UnsubscribeAll(); }
size_t DmReactiveSubjectImpl::GetObserverCount() const { return m_stream_impl.GetObserverCount(); }
bool DmReactiveSubjectImpl::IsActive() const { return m_stream_impl.IsActive(); }
bool DmReactiveSubjectImpl::IsCompleted() const { return m_stream_impl.IsCompleted(); }
bool DmReactiveSubjectImpl::HasError() const { return m_stream_impl.HasError(); }
std::string DmReactiveSubjectImpl::GetLastError() const { return m_stream_impl.GetLastError(); }
std::type_index DmReactiveSubjectImpl::GetValueType() const { return m_stream_impl.GetValueType(); }
void DmReactiveSubjectImpl::Start() { m_stream_impl.Start(); }
void DmReactiveSubjectImpl::Stop() { m_stream_impl.Stop(); }
void DmReactiveSubjectImpl::Reset() { m_stream_impl.Reset(); }
size_t DmReactiveSubjectImpl::GetTotalEventCount() const { return m_stream_impl.GetTotalEventCount(); }
size_t DmReactiveSubjectImpl::GetErrorEventCount() const { return m_stream_impl.GetErrorEventCount(); }
IdmReactiveStream* DmReactiveSubjectImpl::CreateMapStream(std::function<std::any(const std::any&)> m, std::type_index t) { return m_stream_impl.CreateMapStream(m, t); }
IdmReactiveStream* DmReactiveSubjectImpl::CreateFilterStream(std::function<bool(const std::any&)> p) { return m_stream_impl.CreateFilterStream(p); }
IdmReactiveStream* DmReactiveSubjectImpl::CreateTakeStream(size_t c) { return m_stream_impl.CreateTakeStream(c); }
// Delegate other stream methods...
IdmReactiveStream* DmReactiveSubjectImpl::CreateSkipStream(size_t c) { return m_stream_impl.CreateSkipStream(c); }
IdmReactiveStream* DmReactiveSubjectImpl::CreateDebounceStream(uint32_t t) { return m_stream_impl.CreateDebounceStream(t); }
IdmReactiveStream* DmReactiveSubjectImpl::CreateThrottleStream(uint32_t t) { return m_stream_impl.CreateThrottleStream(t); }
IdmReactiveStream* DmReactiveSubjectImpl::CreateDistinctStream() { return m_stream_impl.CreateDistinctStream(); }
IdmReactiveStream* DmReactiveSubjectImpl::CreateBufferStream(size_t s) { return m_stream_impl.CreateBufferStream(s); }
IdmReactiveStream* DmReactiveSubjectImpl::CreateMergeStream(IdmReactiveStream* o) { return m_stream_impl.CreateMergeStream(o); }
IdmReactiveStream* DmReactiveSubjectImpl::CreateConcatStream(IdmReactiveStream* o) { return m_stream_impl.CreateConcatStream(o); }
IdmReactiveStream* DmReactiveSubjectImpl::CreateCombineLatestStream(IdmReactiveStream* o) { return m_stream_impl.CreateCombineLatestStream(o); }
IdmReactiveStream* DmReactiveSubjectImpl::CreateZipStream(IdmReactiveStream* o) { return m_stream_impl.CreateZipStream(o); }
IdmReactiveStream* DmReactiveSubjectImpl::CreateObserveOnStream(IdmReactiveScheduler* s) { return m_stream_impl.CreateObserveOnStream(s); }
IdmReactiveStream* DmReactiveSubjectImpl::CreateSubscribeOnStream(IdmReactiveScheduler* s) { return m_stream_impl.CreateSubscribeOnStream(s); }
std::shared_ptr<std::future<std::any>> DmReactiveSubjectImpl::GetFirst() { return m_stream_impl.GetFirst(); }
std::shared_ptr<std::future<std::any>> DmReactiveSubjectImpl::GetLast() { return m_stream_impl.GetLast(); }
std::shared_ptr<std::future<std::vector<std::any>>> DmReactiveSubjectImpl::ToVector() { return m_stream_impl.ToVector(); }
std::shared_ptr<std::future<bool>> DmReactiveSubjectImpl::Any(std::function<bool(const std::any&)> p) { return m_stream_impl.Any(p); }
std::shared_ptr<std::future<bool>> DmReactiveSubjectImpl::All(std::function<bool(const std::any&)> p) { return m_stream_impl.All(p); }
std::shared_ptr<std::future<size_t>> DmReactiveSubjectImpl::Count() { return m_stream_impl.Count(); }


// Subject as Observer
void DmReactiveSubjectImpl::OnEvent(IdmReactiveEvent* event) { m_stream_impl.NotifyObservers(event); }
std::type_index DmReactiveSubjectImpl::GetExpectedType() const { return GetValueType(); }

// Subject Emitters
void DmReactiveSubjectImpl::EmitNext(const std::any& value, std::type_index value_type) {
    if (value_type != GetValueType()) {
        EmitError("Mismatched type in EmitNext"); return;
    }
    dmReactiveEventPtr event(new DmReactiveEventImpl(IdmReactiveEvent::DMNEXT, value, value_type, ""));
    OnEvent(event.get());
}
void DmReactiveSubjectImpl::EmitError(const std::string& error) {
    dmReactiveEventPtr event(new DmReactiveEventImpl(IdmReactiveEvent::DMERROR, {}, GetValueType(), error));
    OnEvent(event.get());
}
void DmReactiveSubjectImpl::EmitComplete() {
    dmReactiveEventPtr event(new DmReactiveEventImpl(IdmReactiveEvent::DMCOMPLETE, {}, GetValueType(), ""));
    OnEvent(event.get());
}

// Other Subject methods
bool DmReactiveSubjectImpl::HasCurrentValue() const { return false; }
std::any DmReactiveSubjectImpl::GetCurrentValue() const { return {}; }
std::type_index DmReactiveSubjectImpl::GetCurrentValueType() const { return std::type_index(typeid(void)); }
void DmReactiveSubjectImpl::SetBufferSize(size_t) {}
size_t DmReactiveSubjectImpl::GetBufferSize() const { return 0; }
void DmReactiveSubjectImpl::ClearBuffer() {}
IdmReactiveSubject::SubjectType DmReactiveSubjectImpl::GetSubjectType() const { return m_subject_type; }

//=============================================================================
// DmThreadPoolSchedulerImpl
//=============================================================================
DmThreadPoolSchedulerImpl::DmThreadPoolSchedulerImpl(size_t thread_count) {
    SetThreadCount(thread_count);
    Start();
}
DmThreadPoolSchedulerImpl::~DmThreadPoolSchedulerImpl() {
    Shutdown();
}
void DmThreadPoolSchedulerImpl::Release() { delete this; }
void DmThreadPoolSchedulerImpl::Start() { m_running = true; }
void DmThreadPoolSchedulerImpl::Stop() {
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_running = false;
    }
    m_condition.notify_all();
}
void DmThreadPoolSchedulerImpl::Shutdown() {
    Stop();
    for (std::thread& worker : m_threads) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}
void DmThreadPoolSchedulerImpl::Schedule(std::function<void()> task) {
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_tasks.push(task);
    }
    m_condition.notify_one();
}
void DmThreadPoolSchedulerImpl::SetThreadCount(size_t count) {
    if (IsRunning() || !m_threads.empty()) return; // Can only set before start
    for (size_t i = 0; i < count; ++i) {
        m_threads.emplace_back(&DmThreadPoolSchedulerImpl::WorkerThread, this);
    }
}
void DmThreadPoolSchedulerImpl::WorkerThread() {
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_condition.wait(lock, [this] { return !m_running || !m_tasks.empty(); });
            if (!m_running && m_tasks.empty()) return;
            task = std::move(m_tasks.front());
            m_tasks.pop();
        }
        m_active_threads++;
        task();
        m_active_threads--;
    }
}
bool DmThreadPoolSchedulerImpl::IsRunning() const { return m_running; }
size_t DmThreadPoolSchedulerImpl::GetThreadCount() const { return m_threads.size(); }
size_t DmThreadPoolSchedulerImpl::GetActiveThreadCount() const { return m_active_threads; }
size_t DmThreadPoolSchedulerImpl::GetQueuedTaskCount() const
{ 
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_tasks.size();
}

// Other scheduler methods
void DmThreadPoolSchedulerImpl::ScheduleDelayed(std::function<void()> task, uint32_t delay_ms) {
    std::thread([task, delay_ms]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
        task();
        }).detach();
}
void DmThreadPoolSchedulerImpl::SchedulePeriodic(std::function<void()>, uint32_t) {}
void DmThreadPoolSchedulerImpl::ScheduleAt(std::function<void()>, uint64_t) {}

//=============================================================================
// DmReactiveFactoryImpl
//=============================================================================
void DmReactiveFactoryImpl::Release() { delete this; }
IdmReactiveEvent* DmReactiveFactoryImpl::CreateNextEvent(const std::any& v, std::type_index t) { return new DmReactiveEventImpl(IdmReactiveEvent::DMNEXT, v, t, ""); }
IdmReactiveEvent* DmReactiveFactoryImpl::CreateErrorEvent(const std::string& e) { return new DmReactiveEventImpl(IdmReactiveEvent::DMERROR, {}, std::type_index(typeid(void)), e); }
IdmReactiveEvent* DmReactiveFactoryImpl::CreateCompleteEvent() { return new DmReactiveEventImpl(IdmReactiveEvent::DMCOMPLETE, {}, std::type_index(typeid(void)), ""); }
IdmReactiveStream* DmReactiveFactoryImpl::CreateStream(std::type_index t) { return new DmReactiveStreamImpl(t); }
IdmReactiveSubject* DmReactiveFactoryImpl::CreateSubject(std::type_index t) { return new DmReactiveSubjectImpl(t, IdmReactiveSubject::SIMPLE); }
IdmReactiveScheduler* DmReactiveFactoryImpl::CreateThreadPoolScheduler(size_t c) { return new DmThreadPoolSchedulerImpl(c); }

class DmRangeStreamImpl : public DmReactiveStreamImpl {
    int32_t m_start, m_end, m_step;
public:
    DmRangeStreamImpl(int32_t s, int32_t e, int32_t p) : DmReactiveStreamImpl(std::type_index(typeid(int32_t))), m_start(s), m_end(e), m_step(p) {}
    void DMAPI Subscribe(IdmReactiveObserver* observer) override {
        m_observers.Add(observer);
        for (int32_t i = m_start; (m_step > 0 ? i <= m_end : i >= m_end); i += m_step) {
            dmReactiveEventPtr next(new DmReactiveEventImpl(IdmReactiveEvent::DMNEXT, std::any(i), m_value_type, ""));
            NotifyObservers(next.get());
        }
        dmReactiveEventPtr complete(new DmReactiveEventImpl(IdmReactiveEvent::DMCOMPLETE, {}, m_value_type, ""));
        NotifyObservers(complete.get());
    }
};
IdmReactiveStream* DmReactiveFactoryImpl::CreateFromRange(int32_t s, int32_t e, int32_t p) { return new DmRangeStreamImpl(s, e, p); }
// Other factory methods
IdmReactiveStream* DmReactiveFactoryImpl::CreateFromVector(const std::vector<std::any>&, std::type_index t) { return CreateEmptyStream(t); }
IdmReactiveStream* DmReactiveFactoryImpl::CreateInterval(uint32_t, std::type_index t) { return CreateEmptyStream(t); }
IdmReactiveStream* DmReactiveFactoryImpl::CreateTimer(uint32_t, const std::any&, std::type_index t) { return CreateEmptyStream(t); }
IdmReactiveStream* DmReactiveFactoryImpl::CreateEmpty(std::type_index t) { return CreateEmptyStream(t); }
IdmReactiveStream* DmReactiveFactoryImpl::CreateNever(std::type_index t) { return new DmReactiveStreamImpl(t); }
IdmReactiveStream* DmReactiveFactoryImpl::CreateError(const std::string&, std::type_index t) { return CreateEmptyStream(t); }
IdmReactiveStream* DmReactiveFactoryImpl::CreateFromCallback(std::function<void(std::function<void(const std::any&, std::type_index)>)>, std::type_index t) { return CreateEmptyStream(t); }
IdmReactiveSubject* DmReactiveFactoryImpl::CreateBehaviorSubject(const std::any&, std::type_index t) { return new DmReactiveSubjectImpl(t, IdmReactiveSubject::BEHAVIOR); }
IdmReactiveSubject* DmReactiveFactoryImpl::CreateReplaySubject(size_t, std::type_index t) { return new DmReactiveSubjectImpl(t, IdmReactiveSubject::REPLAY); }
IdmReactiveSubject* DmReactiveFactoryImpl::CreateAsyncSubject(std::type_index t) { return new DmReactiveSubjectImpl(t, IdmReactiveSubject::ASYNC); }
IdmReactiveScheduler* DmReactiveFactoryImpl::CreateScheduler(IdmReactiveScheduler::Type) { return nullptr; }
IdmReactiveOperator* DmReactiveFactoryImpl::CreateMapOperator(std::function<std::any(const std::any&)>, std::type_index) { return nullptr; }
IdmReactiveOperator* DmReactiveFactoryImpl::CreateFilterOperator(std::function<bool(const std::any&)>) { return nullptr; }
IdmReactiveOperator* DmReactiveFactoryImpl::CreateTakeOperator(size_t) { return nullptr; }
IdmReactiveOperator* DmReactiveFactoryImpl::CreateSkipOperator(size_t) { return nullptr; }
IdmReactiveOperator* DmReactiveFactoryImpl::CreateDebounceOperator(uint32_t) { return nullptr; }
IdmReactiveOperator* DmReactiveFactoryImpl::CreateThrottleOperator(uint32_t) { return nullptr; }
IdmReactiveOperator* DmReactiveFactoryImpl::CreateDistinctOperator() { return nullptr; }
IdmReactiveOperator* DmReactiveFactoryImpl::CreateBufferOperator(size_t) { return nullptr; }
IdmReactiveStream* DmReactiveFactoryImpl::MergeStreams(const std::vector<IdmReactiveStream*>& s) { return CreateEmptyStream(s.empty() ? std::type_index(typeid(void)) : s[0]->GetValueType()); }
IdmReactiveStream* DmReactiveFactoryImpl::ConcatStreams(const std::vector<IdmReactiveStream*>& s) { return CreateEmptyStream(s.empty() ? std::type_index(typeid(void)) : s[0]->GetValueType()); }
IdmReactiveStream* DmReactiveFactoryImpl::ZipStreams(const std::vector<IdmReactiveStream*>& s) { return CreateEmptyStream(s.empty() ? std::type_index(typeid(void)) : s[0]->GetValueType()); }
IdmReactiveStream* DmReactiveFactoryImpl::RaceStreams(const std::vector<IdmReactiveStream*>& s) { return CreateEmptyStream(s.empty() ? std::type_index(typeid(void)) : s[0]->GetValueType()); }
IdmReactiveStream* DmReactiveFactoryImpl::CombineLatestStreams(const std::vector<IdmReactiveStream*>& s) { return CreateEmptyStream(s.empty() ? std::type_index(typeid(void)) : s[0]->GetValueType()); }

//=============================================================================
// DmReactiveCoreImpl
//=============================================================================
DmReactiveCoreImpl::DmReactiveCoreImpl() { Initialize(); }
DmReactiveCoreImpl::~DmReactiveCoreImpl() { Shutdown(); }
void DmReactiveCoreImpl::Release() { delete this; }
IdmReactiveFactory* DmReactiveCoreImpl::GetFactory() { return &m_factory; }
void DmReactiveCoreImpl::Initialize() {
    if (!m_is_initialized) {
        m_default_scheduler.reset(m_factory.CreateThreadPoolScheduler(1));
        m_io_scheduler.reset(m_factory.CreateThreadPoolScheduler(std::thread::hardware_concurrency()));
        m_computation_scheduler.reset(m_factory.CreateThreadPoolScheduler(std::thread::hardware_concurrency()));
        m_is_initialized = true;
    }
}
void DmReactiveCoreImpl::Shutdown() { m_is_initialized = false; }
bool DmReactiveCoreImpl::IsInitialized() const { return m_is_initialized; }
void DmReactiveCoreImpl::SetDefaultScheduler(IdmReactiveScheduler* s) { m_default_scheduler.reset(s); }
IdmReactiveScheduler* DmReactiveCoreImpl::GetDefaultScheduler() { return m_default_scheduler.get(); }
IdmReactiveScheduler* DmReactiveCoreImpl::GetIOScheduler() { return m_io_scheduler.get(); }
IdmReactiveScheduler* DmReactiveCoreImpl::GetComputationScheduler() { return m_computation_scheduler.get(); }
IdmReactiveScheduler* DmReactiveCoreImpl::GetMainThreadScheduler() { return nullptr; }
// Other Core methods
void DmReactiveCoreImpl::SetGlobalErrorHandler(std::function<void(const std::string&)>) {}
void DmReactiveCoreImpl::SetLogLevel(int) {}
void DmReactiveCoreImpl::SetMaxBufferSize(size_t) {}
void DmReactiveCoreImpl::SetDefaultDebounceTime(uint32_t) {}
void DmReactiveCoreImpl::SetDefaultThrottleTime(uint32_t) {}
size_t DmReactiveCoreImpl::GetActiveStreamCount() const { return 0; }
size_t DmReactiveCoreImpl::GetActiveObserverCount() const { return 0; }
size_t DmReactiveCoreImpl::GetActiveSubjectCount() const { return 0; }
size_t DmReactiveCoreImpl::GetTotalEventsProcessed() const { return 0; }
size_t DmReactiveCoreImpl::GetTotalErrorsOccurred() const { return 0; }
double DmReactiveCoreImpl::GetAverageEventProcessingTime() const { return 0; }
size_t DmReactiveCoreImpl::GetPeakMemoryUsage() const { return 0; }
size_t DmReactiveCoreImpl::GetCurrentMemoryUsage() const { return 0; }
void DmReactiveCoreImpl::GarbageCollect() {}
void DmReactiveCoreImpl::EnableDebugMode(bool) {}
bool DmReactiveCoreImpl::IsDebugModeEnabled() const { return false; }
void DmReactiveCoreImpl::DumpStreamGraph(const std::string&) {}
void DmReactiveCoreImpl::DumpStatistics(const std::string&) {}
std::string DmReactiveCoreImpl::GetDebugInfo() const { return "Debug info not implemented."; }
void DmReactiveCoreImpl::RegisterType(std::type_index, const std::string&) {}
std::string DmReactiveCoreImpl::GetTypeName(std::type_index) const { return "Unknown"; }
bool DmReactiveCoreImpl::IsTypeRegistered(std::type_index) const { return false; }
//=============================================================================
// 全局工厂函数
//=============================================================================
DMEXPORT_DLL dmReactiveCorePtr DMAPI dmReactiveGetCore() {
    return dmReactiveCorePtr(new DmReactiveCoreImpl());
}