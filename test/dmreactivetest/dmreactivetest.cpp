
#include "dmreactive.h"
#include "gtest.h"

class env_dmreactive
{
public:
    void init(){}
    void uninit(){}
};

class frame_dmreactive : public testing::Test
{
public:
    virtual void SetUp()
    {
        env.init();
    }
    virtual void TearDown()
    {
        env.uninit();
    }
protected:
    env_dmreactive env;
};

TEST_F(frame_dmreactive, init)
{
    Idmreactive* module = dmreactiveGetModule();
    if (module)
    {
        module->Test();
        module->Release();
    }
}
