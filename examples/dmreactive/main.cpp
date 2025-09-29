#include "dmreactive.h"
#include <iostream>
#include <string>

// 测试观察者
void TestObserver(const void* data) {
    if (data) {
        const std::string* str = static_cast<const std::string*>(data);
        std::cout << "Observer received: " << *str << std::endl;
    }
}

// 测试信号槽
void TestSlot() {
    std::cout << "Signal slot triggered!" << std::endl;
}

int main(int argc, char* argv[]) {
    dmreactivePtr module(dmreactiveGetModule());

    if (module) {
        // 测试响应式数据流
        auto observable = module->CreateObservable();
        if (observable) {
            observable->Subscribe(TestObserver);

            std::string testData = "Hello Reactive Programming!";
            module->NotifyObservers(observable.get(), &testData);
        }

        // 测试信号槽机制
        auto signal = module->CreateSignal();
        if (signal) {
            signal->Connect(TestSlot);
            signal->Emit();
        }

        // 测试异步管道
        auto pipeline = module->CreatePipeline();
        if (pipeline) {
            pipeline->SetProcessor([](void* data) -> void* {
                if (data) {
                    std::string* str = static_cast<std::string*>(data);
                    *str += " [Processed]";
                    std::cout << "Pipeline processed: " << *str << std::endl;
                }
                return data;
                });

            std::string pipeData = "Pipeline data";
            pipeline->Process(&pipeData);
        }
    }

    return 0;
}