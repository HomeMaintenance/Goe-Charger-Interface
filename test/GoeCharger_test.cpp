#include <gtest/gtest.h>
#include <GoeCharger.h>

using namespace goe;

class GoeChargerTest: public ::testing::Test{
public:
    GoeChargerTest(){}
    virtual void SetUp() override {
        charger = std::make_unique<Charger>("goeCharger", "192.168.178.106");
        chargerState.alw = charger->get_alw().value;
        chargerState.amp = charger->get_amp().value;
    }

    virtual void TearDown() override {
        charger->set_alw(chargerState.alw);
        charger->set_amp(chargerState.amp);
    }

    std::unique_ptr<Charger> charger;

private:
    struct ChargerState{
        bool alw;
        int amp;
    } chargerState;

};

TEST_F(GoeChargerTest, Constructor){}

TEST_F(GoeChargerTest, TurnOn){
    std::vector<Charger::Response> responses;
    responses.push_back(charger->set_alw(true));
    Charger::GetResponse<bool> alw = charger->get_alw();
    EXPECT_EQ(alw.response, Charger::Response::FromDevice);
    EXPECT_EQ(alw.value, true);
}

TEST_F(GoeChargerTest, TurnOff){
    std::vector<Charger::Response> responses;
    responses.push_back(charger->set_alw(true));
    responses.push_back(charger->set_alw(false));
    Charger::GetResponse<bool> alw = charger->get_alw();
    EXPECT_EQ(alw.response, Charger::Response::FromDevice);
    EXPECT_EQ(alw.value, false);
}

int main(int argc, char* argv[]){
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
