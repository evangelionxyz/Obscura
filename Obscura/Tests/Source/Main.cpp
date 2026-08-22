#include <gtest/gtest.h>
#include <Obscura/Logger.hpp>

int main(int argc, char** argv)
{
    Obscura::Logger::Init();
    ::testing::InitGoogleTest(&argc, argv);
    int result = RUN_ALL_TESTS();
    Obscura::Logger::Shutdown();
    return result;
}
