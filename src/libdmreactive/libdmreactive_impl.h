
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
#include <vector>
#include <functional>
#include <memory>
#include <atomic>

class DmobservableImpl : public Idmobservable {
private:
    std::vector<std::function<void(const void*)>> m_observers;
    std::atomic<bool> m_active{true};
    
public:
    virtual ~DmobservableImpl() {}
    virtual void DMAPI Release(void) override;
    virtual void DMAPI Subscribe(const std::function<void(const void*)>& observer) override;
    virtual void DMAPI UnsubscribeAll() override;
    
    void Notify(const void* data);
    bool IsActive() const { return m_active.load(); }
};

class DmsignalImpl : public Idmsignal {
private:
    std::vector<std::function<void()>> m_slots;
    std::atomic<bool> m_connected{true};
    
public:
    virtual ~DmsignalImpl() {}
    virtual void DMAPI Release(void) override;
    virtual void DMAPI Connect(const std::function<void()>& slot) override;
    virtual void DMAPI DisconnectAll() override;
    virtual void DMAPI Emit() override;
};

class DmpipelineImpl : public Idmpipeline {
private:
    std::function<void*(void*)> m_processor;
    std::atomic<bool> m_processing{false};
    
public:
    virtual ~DmpipelineImpl() {}
    virtual void DMAPI Release(void) override;
    virtual void DMAPI Process(const void* data) override;
    virtual void DMAPI SetProcessor(const std::function<void*(void*)>& processor) override;
};

class DmreactiveImpl : public Idmreactive {
public:
    virtual ~DmreactiveImpl() {}
    virtual void DMAPI Release(void) override;
    
    virtual dmobservablePtr DMAPI CreateObservable() override;
    virtual void DMAPI NotifyObservers(Idmobservable* observable, const void* data) override;
    
    virtual dmsignalPtr DMAPI CreateSignal() override;
    
    virtual dmpipelinePtr DMAPI CreatePipeline() override;
    
    virtual dmobservablePtr DMAPI FromFunction(const std::function<void*(void*)>& func) override;
    virtual dmobservablePtr DMAPI FromContainer(const std::vector<void*>& container) override;
};

#endif // __LIBDMREACTIVE_IMPL_H_INCLUDE__