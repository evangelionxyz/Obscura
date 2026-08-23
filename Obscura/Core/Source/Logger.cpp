#include "Obscura/Logger.hpp"
#include "Obscura/Types.hpp"

#include <spdlog/sinks/base_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <memory>
#include <mutex>

namespace Obscura
{
    class ConsoleSink : public spdlog::sinks::base_sink<std::mutex>
    {
    public:
        void sink_it_(const spdlog::details::log_msg& msg) override
        {
            spdlog::memory_buf_t formatted;
            spdlog::sinks::base_sink<std::mutex>::formatter_->format(msg, formatted);
            m_Messages.push_back({ msg.level, fmt::to_string(formatted) });
        }

        void flush_() override {}

        std::vector<Obscura::LogMessage> m_Messages;
    };

    struct LoggerImpl
    {
        Ref<spdlog::logger> logger;
        Ref<spdlog::sinks::stdout_color_sink_mt> stdoutSink;
        Ref<Obscura::ConsoleSink> consoleSink;
    };

    static LoggerImpl *impl = nullptr;

    void Logger::Init()
    {
        if (impl)
        {
            return;
        }

        impl = new LoggerImpl();

        impl->stdoutSink = CreateRef<spdlog::sinks::stdout_color_sink_mt>();
        impl->stdoutSink->set_pattern("%^[%T] [%l] %n: %v%$");

        impl->consoleSink = CreateRef<Obscura::ConsoleSink>();
        impl->consoleSink->set_pattern("[%T] [%l] %n: %v");

        std::vector<spdlog::sink_ptr> sinks = { impl->stdoutSink, impl->consoleSink };

        impl->logger = CreateRef<spdlog::logger>("Obscura", sinks.begin(), sinks.end());
        impl->logger->set_level(spdlog::level::trace);
        spdlog::set_default_logger(impl->logger);
    }

    void Logger::Shutdown()
    {
        if (impl)
        {
            impl->logger->flush();
            impl->stdoutSink->flush();
            impl->consoleSink->flush();

            delete impl;
            impl = nullptr;
        }
    }

    const std::vector<LogMessage> &Logger::GetLogs()
    {
        return impl->consoleSink->m_Messages;
    }

    void Logger::ClearLogs()
    {
        if (impl)
        {
            impl->consoleSink->m_Messages.clear();
        }
    }

    void Logger::PushLog(spdlog::level::level_enum level, const std::string &message)
    {
        if (impl)
        {
            impl->consoleSink->m_Messages.push_back({ level, message });
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
