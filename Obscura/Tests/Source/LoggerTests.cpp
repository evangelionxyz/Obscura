#include <gtest/gtest.h>
#include <Obscura/Logger.hpp>

TEST(LoggerTests, CallbackAndSinkCapture)
{
    std::vector<std::pair<spdlog::level::level_enum, std::string>> captured;

    Obscura::Logger::SetCallback([&captured](spdlog::level::level_enum level, const std::string &message) {
        captured.emplace_back(level, message);
    });

    LOG_INFO("Test Info Message from Test Suite");
    LOG_WARN("Test Warning Message with param: {}", 42);
    LOG_ERROR("Test Error Message");

    ASSERT_GE(captured.size(), 3u);
    bool foundInfo = false;
    bool foundWarn = false;
    bool foundError = false;

    for (const auto &[lvl, msg] : captured)
    {
        if (msg.find("Test Info Message from Test Suite") != std::string::npos && lvl == spdlog::level::info)
        {
            foundInfo = true;
        }
        if (msg.find("Test Warning Message with param: 42") != std::string::npos && lvl == spdlog::level::warn)
        {
            foundWarn = true;
        }
        if (msg.find("Test Error Message") != std::string::npos && lvl == spdlog::level::err)
        {
            foundError = true;
        }
    }

    EXPECT_TRUE(foundInfo);
    EXPECT_TRUE(foundWarn);
    EXPECT_TRUE(foundError);

    Obscura::Logger::ClearCallback();
}
