
#include "dmreactive.h"
#include <gtest.h>
#include <vector>
#include <string>

// 为了测试，我们需要访问内部实现类来创建Observer
// 在实际项目中，工厂应该提供一个CreateObserver方法
#include "libdmreactive_impl.h" 

// 辅助类：一个简单的观察者，用于收集结果
class TestObserver {
public:
    std::vector<int> received_values;
    std::string error_message;
    bool is_completed = false;
    int next_count = 0;
    int error_count = 0;
    int complete_count = 0;

    dmReactiveObserverPtr Create(std::type_index type_idx = std::type_index(typeid(int))) {
        return dmReactiveObserverPtr(
            new DmReactiveObserverImpl(type_idx,
                [this](IdmReactiveEvent* event) {
                    switch (event->GetType()) {
                    case IdmReactiveEvent::DMNEXT: {
                        next_count++;
                        int val;
                        if (event->TryGetTypedValue(val)) {
                            received_values.push_back(val);
                        }
                        break;
                    }
                    case IdmReactiveEvent::DMERROR: {
                        error_count++;
                        error_message = event->GetError();
                        break;
                    }
                    case IdmReactiveEvent::DMCOMPLETE: {
                        complete_count++;
                        is_completed = true;
                        break;
                    }
                    }
                })
        );
    }
};

// 测试套件
class ReactiveTest : public ::testing::Test {
protected:
    dmReactiveCorePtr core_instance;
    IdmReactiveFactory* factory;

    void SetUp() override {
        // 注意：我们不使用智能指针管理单例
        core_instance = dmReactiveGetCore();
        factory = core_instance->GetFactory();
    }
};

// 测试基本的流创建和订阅
TEST_F(ReactiveTest, CreateFromRangeAndSubscribe) {
    TestObserver collector;
    dmReactiveObserverPtr observer = collector.Create();

    dmReactiveStreamPtr range_stream(factory->CreateFromRange(1, 5));
    ASSERT_TRUE(range_stream.get() != nullptr);

    range_stream->Subscribe(observer.get());

    ASSERT_EQ(collector.next_count, 5);
    ASSERT_EQ(collector.complete_count, 1);
    ASSERT_EQ(collector.error_count, 0);
    ASSERT_TRUE(collector.is_completed);

    std::vector<int> expected = {1, 2, 3, 4, 5};
    ASSERT_EQ(collector.received_values, expected);
}

// 测试 Filter 操作符
TEST_F(ReactiveTest, FilterOperator) {
    TestObserver collector;
    dmReactiveObserverPtr observer = collector.Create();

    dmReactiveStreamPtr source(factory->CreateFromRange(1, 10));

    auto predicate = [](const std::any& val) {
        return std::any_cast<int32_t>(val) % 2 == 0;
        };
    dmReactiveStreamPtr filtered_stream(source->CreateFilterStream(predicate));
    ASSERT_TRUE(filtered_stream.get() != nullptr);

    filtered_stream->Subscribe(observer.get());

    ASSERT_EQ(collector.next_count, 5);
    ASSERT_TRUE(collector.is_completed);

    std::vector<int> expected = {2, 4, 6, 8, 10};
    ASSERT_EQ(collector.received_values, expected);
}

// 测试 Map 操作符
TEST_F(ReactiveTest, MapOperator) {
    TestObserver collector;
    dmReactiveObserverPtr observer = collector.Create();

    dmReactiveStreamPtr source(factory->CreateFromRange(1, 5));

    auto mapper = [](const std::any& val) -> std::any {
        return std::any_cast<int32_t>(val) * std::any_cast<int32_t>(val);
        };
    dmReactiveStreamPtr mapped_stream(source->CreateMapStream(mapper, std::type_index(typeid(int32_t))));
    ASSERT_TRUE(mapped_stream.get() != nullptr);

    mapped_stream->Subscribe(observer.get());

    ASSERT_EQ(collector.next_count, 5);
    ASSERT_TRUE(collector.is_completed);

    std::vector<int> expected = {1, 4, 9, 16, 25};
    ASSERT_EQ(collector.received_values, expected);
}

// 测试链式调用 Filter 和 Map
TEST_F(ReactiveTest, ChainedOperators) {
    TestObserver collector;
    dmReactiveObserverPtr observer = collector.Create();

    dmReactiveStreamPtr source(factory->CreateFromRange(1, 10));

    auto predicate = [](const std::any& val) { return std::any_cast<int32_t>(val) % 2 == 0; };
    auto mapper = [](const std::any& val) -> std::any { return std::any_cast<int32_t>(val) * std::any_cast<int32_t>(val); };

    dmReactiveStreamPtr final_stream(
        source->CreateFilterStream(predicate)
        ->CreateMapStream(mapper, std::type_index(typeid(int32_t)))
    );
    ASSERT_TRUE(final_stream.get() != nullptr);

    final_stream->Subscribe(observer.get());

    ASSERT_EQ(collector.next_count, 5);
    ASSERT_TRUE(collector.is_completed);

    std::vector<int> expected = {4, 16, 36, 64, 100};
    ASSERT_EQ(collector.received_values, expected);
}

// 测试 Subject
TEST_F(ReactiveTest, SubjectEmitsValues) {
    TestObserver collector;
    dmReactiveObserverPtr observer = collector.Create(std::type_index(typeid(int32_t)));

    dmReactiveSubjectPtr subject(factory->CreateSubject(std::type_index(typeid(int32_t))));
    ASSERT_TRUE(subject.get() != nullptr);

    subject->Subscribe(observer.get());

    subject->EmitNext(std::any(10), std::type_index(typeid(int32_t)));
    subject->EmitNext(std::any(20), std::type_index(typeid(int32_t)));
    subject->EmitError("Test Error");
    subject->EmitNext(std::any(30), std::type_index(typeid(int32_t))); // should be ignored
    subject->EmitComplete(); // should be ignored

    ASSERT_EQ(collector.next_count, 2);
    ASSERT_EQ(collector.error_count, 1);
    ASSERT_EQ(collector.complete_count, 0);
    ASSERT_FALSE(collector.is_completed);

    ASSERT_EQ(collector.error_message, "Test Error");

    std::vector<int> expected = {10, 20};
    ASSERT_EQ(collector.received_values, expected);
}
