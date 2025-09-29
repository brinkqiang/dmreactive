#include "gtest.h"
#include "dmreactive.h"
#include <string>
#include <vector>
#include <atomic>

class DmReactiveTest : public ::testing::Test {
protected:
    void SetUp() override {
        module.reset(dmreactiveGetModule());
    }

    void TearDown() override {
        module.reset();
    }

    dmreactivePtr module;
};

// 测试模块创建
TEST_F(DmReactiveTest, ModuleCreation) {
    ASSERT_TRUE(module.get() != nullptr) << "Module should be created successfully";
}

// 测试Observable创建
TEST_F(DmReactiveTest, ObservableCreation) {
    auto observable = module->CreateObservable();
    ASSERT_TRUE(observable.get() != nullptr) << "Observable should be created successfully";
}

// 测试Observable订阅和通知
TEST_F(DmReactiveTest, ObservableSubscription) {
    auto observable = module->CreateObservable();
    ASSERT_TRUE(observable.get() != nullptr);

    std::atomic<int> callbackCount{0};
    std::string receivedData;

    // 订阅回调函数
    auto callback = [&](const void* data) {
        if (data) {
            const std::string* str = static_cast<const std::string*>(data);
            receivedData = *str;
            callbackCount++;
        }
        };

    observable->Subscribe(callback);

    // 发送通知
    std::string testData = "Test Observable Data";
    module->NotifyObservers(observable.get(), &testData);

    EXPECT_EQ(callbackCount.load(), 1) << "Callback should be called once";
    EXPECT_EQ(receivedData, "Test Observable Data") << "Received data should match sent data";
}

// 测试Observable多订阅者
TEST_F(DmReactiveTest, ObservableMultipleSubscribers) {
    auto observable = module->CreateObservable();
    ASSERT_TRUE(observable.get() != nullptr);

    std::atomic<int> callback1Count{0};
    std::atomic<int> callback2Count{0};

    auto callback1 = [&](const void* data) { callback1Count++; };
    auto callback2 = [&](const void* data) { callback2Count++; };

    observable->Subscribe(callback1);
    observable->Subscribe(callback2);

    std::string testData = "Test Data";
    module->NotifyObservers(observable.get(), &testData);

    EXPECT_EQ(callback1Count.load(), 1) << "First callback should be called once";
    EXPECT_EQ(callback2Count.load(), 1) << "Second callback should be called once";
}

// 测试Observable取消订阅
TEST_F(DmReactiveTest, ObservableUnsubscribe) {
    auto observable = module->CreateObservable();
    ASSERT_TRUE(observable.get() != nullptr);

    std::atomic<int> callbackCount{0};
    auto callback = [&](const void* data) { callbackCount++; };

    observable->Subscribe(callback);

    // 第一次通知
    std::string testData1 = "Data 1";
    module->NotifyObservers(observable.get(), &testData1);
    EXPECT_EQ(callbackCount.load(), 1) << "Callback should be called after first notification";

    // 取消所有订阅
    observable->UnsubscribeAll();

    // 第二次通知，不应该触发回调
    std::string testData2 = "Data 2";
    module->NotifyObservers(observable.get(), &testData2);
    EXPECT_EQ(callbackCount.load(), 1) << "Callback count should remain unchanged after unsubscribe";
}

// 测试Signal创建
TEST_F(DmReactiveTest, SignalCreation) {
    auto signal = module->CreateSignal();
    ASSERT_TRUE(signal.get() != nullptr) << "Signal should be created successfully";
}

// 测试Signal连接和触发
TEST_F(DmReactiveTest, SignalConnectAndEmit) {
    auto signal = module->CreateSignal();
    ASSERT_TRUE(signal.get() != nullptr);

    std::atomic<int> slotCallCount{0};

    auto slot = [&]() { slotCallCount++; };
    signal->Connect(slot);

    // 触发信号
    signal->Emit();
    EXPECT_EQ(slotCallCount.load(), 1) << "Slot should be called once after emit";

    // 再次触发
    signal->Emit();
    EXPECT_EQ(slotCallCount.load(), 2) << "Slot should be called twice after second emit";
}

// 测试Signal多槽连接
TEST_F(DmReactiveTest, SignalMultipleSlots) {
    auto signal = module->CreateSignal();
    ASSERT_TRUE(signal.get() != nullptr);

    std::atomic<int> slot1Count{0};
    std::atomic<int> slot2Count{0};

    auto slot1 = [&]() { slot1Count++; };
    auto slot2 = [&]() { slot2Count++; };

    signal->Connect(slot1);
    signal->Connect(slot2);

    signal->Emit();

    EXPECT_EQ(slot1Count.load(), 1) << "First slot should be called once";
    EXPECT_EQ(slot2Count.load(), 1) << "Second slot should be called once";
}

// 测试Signal断开连接
TEST_F(DmReactiveTest, SignalDisconnect) {
    auto signal = module->CreateSignal();
    ASSERT_TRUE(signal.get() != nullptr);

    std::atomic<int> slotCallCount{0};
    auto slot = [&]() { slotCallCount++; };

    signal->Connect(slot);

    // 第一次触发
    signal->Emit();
    EXPECT_EQ(slotCallCount.load(), 1) << "Slot should be called after first emit";

    // 断开所有连接
    signal->DisconnectAll();

    // 第二次触发，不应该调用槽函数
    signal->Emit();
    EXPECT_EQ(slotCallCount.load(), 1) << "Slot call count should remain unchanged after disconnect";
}

// 测试Pipeline创建
TEST_F(DmReactiveTest, PipelineCreation) {
    auto pipeline = module->CreatePipeline();
    ASSERT_TRUE(pipeline.get() != nullptr) << "Pipeline should be created successfully";
}

// 测试Pipeline处理
TEST_F(DmReactiveTest, PipelineProcessing) {
    auto pipeline = module->CreatePipeline();
    ASSERT_TRUE(pipeline.get() != nullptr);

    std::atomic<int> processCount{0};
    std::string processedData;

    auto processor = [&](void* data) -> void* {
        if (data) {
            std::string* str = static_cast<std::string*>(data);
            processedData = *str + " [Processed]";
            processCount++;
        }
        return data;
        };

    pipeline->SetProcessor(processor);

    std::string testData = "Pipeline Test";
    pipeline->Process(&testData);

    // 注意：由于是异步处理，这里可能需要等待处理完成
    // 在实际实现中应该使用更完善的同步机制
    EXPECT_EQ(processCount.load(), 1) << "Processor should be called once";
    EXPECT_EQ(processedData, "Pipeline Test [Processed]") << "Processed data should match expected format";
}

// 测试Pipeline处理器设置
TEST_F(DmReactiveTest, PipelineProcessorSetting) {
    auto pipeline = module->CreatePipeline();
    ASSERT_TRUE(pipeline.get() != nullptr);

    std::atomic<int> processCount{0};

    // 第一次设置处理器
    auto processor1 = [&](void* data) -> void* {
        processCount++;
        return data;
        };

    pipeline->SetProcessor(processor1);
    pipeline->Process(nullptr);
    EXPECT_EQ(processCount.load(), 1) << "First processor should be called";

    // 第二次设置处理器
    processCount = 0;
    auto processor2 = [&](void* data) -> void* {
        processCount += 2; // 不同的行为
        return data;
        };

    pipeline->SetProcessor(processor2);
    pipeline->Process(nullptr);
    EXPECT_EQ(processCount.load(), 2) << "Second processor should be called with different behavior";
}

// 测试FromFunction功能
TEST_F(DmReactiveTest, FromFunction) {
    auto observable = module->FromFunction([](void* data) -> void* {
        if (data) {
            std::string* str = static_cast<std::string*>(data);
            *str = "Processed: " + *str;
        }
        return data;
        });

    ASSERT_TRUE(observable.get() != nullptr) << "Observable from function should be created";

    // 测试Observable基本功能仍然可用
    std::atomic<int> callbackCount{0};
    observable->Subscribe([&](const void* data) { callbackCount++; });

    // 注意：FromFunction的具体实现可能需要更复杂的测试
    // 这里主要测试接口可用性
}

// 测试FromContainer功能
TEST_F(DmReactiveTest, FromContainer) {
    std::vector<void*> testContainer;
    std::string item1 = "item1";
    std::string item2 = "item2";
    testContainer.push_back(&item1);
    testContainer.push_back(&item2);

    auto observable = module->FromContainer(testContainer);
    ASSERT_TRUE(observable.get() != nullptr) << "Observable from container should be created";

    // 测试Observable基本功能仍然可用
    std::atomic<int> callbackCount{0};
    observable->Subscribe([&](const void* data) { callbackCount++; });

    // 注意：FromContainer的具体实现可能需要更复杂的测试
    // 这里主要测试接口可用性
}

// 测试模块完整性
TEST_F(DmReactiveTest, ModuleCompleteness) {
    // 测试所有主要组件都可以创建并协同工作
    auto observable = module->CreateObservable();
    auto signal = module->CreateSignal();
    auto pipeline = module->CreatePipeline();

    ASSERT_TRUE(observable.get() != nullptr) << "Observable should be created";
    ASSERT_TRUE(signal.get() != nullptr) << "Signal should be created";
    ASSERT_TRUE(pipeline.get() != nullptr) << "Pipeline should be created";

    // 测试基本功能
    std::atomic<int> observableCount{0};
    std::atomic<int> signalCount{0};
    std::atomic<int> pipelineCount{0};

    observable->Subscribe([&](const void* data) { observableCount++; });
    signal->Connect([&]() { signalCount++; });
    pipeline->SetProcessor([&](void* data) -> void* { pipelineCount++; return data; });

    std::string testData = "Test";
    module->NotifyObservers(observable.get(), &testData);
    signal->Emit();
    pipeline->Process(&testData);

    EXPECT_EQ(observableCount.load(), 1) << "Observable should work";
    EXPECT_EQ(signalCount.load(), 1) << "Signal should work";
    EXPECT_EQ(pipelineCount.load(), 1) << "Pipeline should work";
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}