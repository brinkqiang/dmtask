
#include "dmtask.h"
#include "gtest.h"

class env_dmtask
{
public:
    void init(){}
    void uninit(){}
};

class frame_dmtask : public testing::Test
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
    env_dmtask env;
};

TEST_F(frame_dmtask, init)
{
    Idmtask* module = dmtaskGetModule();
    if (module)
    {
        module->Test();
        module->Release();
    }
}
