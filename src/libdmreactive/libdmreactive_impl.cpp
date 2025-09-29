#include "libdmreactive_impl.h"
#include <iostream>
#include <thread>

// DmobservableImpl 实现
void DmobservableImpl::Release(void) {
    delete this;
}

void DmobservableImpl::Subscribe(const std::function<void(const void*)>& observer) {
    if (m_active.load()) {
        m_observers.push_back(observer);
    }
}

void DmobservableImpl::UnsubscribeAll() {
    m_observers.clear();
}

void DmobservableImpl::Notify(const void* data) {
    if (!m_active.load()) return;
    
    for (const auto& observer : m_observers) {
        if (observer) {
            observer(data);
        }
    }
}

// DmsignalImpl 实现
void DmsignalImpl::Release(void) {
    delete this;
}

void DmsignalImpl::Connect(const std::function<void()>& slot) {
    if (m_connected.load()) {
        m_slots.push_back(slot);
    }
}

void DmsignalImpl::DisconnectAll() {
    m_slots.clear();
}

void DmsignalImpl::Emit() {
    if (!m_connected.load()) return;
    
    for (const auto& slot : m_slots) {
        if (slot) {
            slot();
        }
    }
}

// DmpipelineImpl 实现
void DmpipelineImpl::Release(void) {
    delete this;
}

void DmpipelineImpl::Process(const void* data) {
    if (m_processing.load() || !m_processor) return;
    
    m_processing.store(true);
    // 在实际实现中，这里应该使用线程池或异步任务
    m_processor(const_cast<void*>(data));
    m_processing.store(false);
}

void DmpipelineImpl::SetProcessor(const std::function<void*(void*)>& processor) {
    m_processor = processor;
}

// DmreactiveImpl 实现
void DmreactiveImpl::Release(void) {
    delete this;
}

dmobservablePtr DmreactiveImpl::CreateObservable() {
    return dmobservablePtr(new DmobservableImpl());
}

void DmreactiveImpl::NotifyObservers(Idmobservable* observable, const void* data) {
    if (auto impl = dynamic_cast<DmobservableImpl*>(observable)) {
        impl->Notify(data);
    }
}

dmsignalPtr DmreactiveImpl::CreateSignal() {
    return dmsignalPtr(new DmsignalImpl());
}

dmpipelinePtr DmreactiveImpl::CreatePipeline() {
    return dmpipelinePtr(new DmpipelineImpl());
}

dmobservablePtr DmreactiveImpl::FromFunction(const std::function<void*(void*)>& func) {
    auto observable = new DmobservableImpl();
    // 这里可以添加函数到观察者的转换逻辑
    return dmobservablePtr(observable);
}

dmobservablePtr DmreactiveImpl::FromContainer(const std::vector<void*>& container) {
    auto observable = new DmobservableImpl();
    // 这里可以添加容器到观察者的转换逻辑
    return dmobservablePtr(observable);
}

extern "C" DMEXPORT_DLL Idmreactive* DMAPI dmreactiveGetModule() {
    return new DmreactiveImpl();
}