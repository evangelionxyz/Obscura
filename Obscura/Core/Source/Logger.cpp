#include "Obscura/Logger.hpp"
#include <memory>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <vector>

struct LoggerImpl
{
    std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> stdoutSink;
    std::shared_ptr<spdlog::logger> logger;
};

static LoggerImpl *impl = nullptr;

namespace Obscura
{
    void Logger::Init()
    {
        if (impl)
        {
            return;
        }

        impl = new LoggerImpl();

        impl->stdoutSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        impl->stdoutSink->set_pattern("%^[%T] [%l] %n: %v%$");

        std::vector<spdlog::sink_ptr> sinks { impl->stdoutSink };

        impl->logger = std::make_shared<spdlog::logger>(
            "Obscura", sinks.begin(), sinks.end()
        );

        impl->logger->set_level(spdlog::level::trace);
        spdlog::set_default_logger(impl->logger);
    }

    void Logger::Shutdown()
    {
        if (impl)
        {
            if (impl->stdoutSink)
            {
                impl->stdoutSink->flush();
            }
            if (impl->logger)
            {
                impl->logger->flush();
            }
            delete impl;
            impl = nullptr;
        }
    }

    spdlog::logger *Logger::GetLogger()
    {
        if (impl && impl->logger)
        {
            return impl->logger.get();
        }
        return spdlog::default_logger().get();
    }
}
