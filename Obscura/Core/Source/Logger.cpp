#include "Obscura/Logger.hpp"
#include "Obscura/Types.hpp"

#include <spdlog/sinks/base_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <memory>
#include <mutex>

namespace Obscura
{
    static LogCallback s_LogCallback = nullptr;
    static std::mutex  s_CallbackMutex;

    class ConsoleSink : public spdlog::sinks::base_sink<std::mutex>
    {
    public:
        void sink_it_(const spdlog::details::log_msg& msg) override
        {
            spdlog::memory_buf_t formatted;
            spdlog::sinks::base_sink<std::mutex>::formatter_->format(msg, formatted);
            std::string formattedStr = fmt::to_string(formatted);
            while (!formattedStr.empty() && (formattedStr.back() == '\n' || formattedStr.back() == '\r'))
            {
                formattedStr.pop_back();
            }

            m_Messages.push_back({ msg.level, formattedStr });

            LogCallback cb;
            {
                std::lock_guard<std::mutex> lock(s_CallbackMutex);
                cb = s_LogCallback;
            }
            if (cb)
            {
                cb(msg.level, formattedStr);
            }
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
        static const std::vector<LogMessage> s_EmptyLogs;
        if (impl && impl->consoleSink)
        {
            return impl->consoleSink->m_Messages;
        }
        return s_EmptyLogs;
    }

    void Logger::ClearLogs()
    {
        if (impl && impl->consoleSink)
        {
            impl->consoleSink->m_Messages.clear();
        }
    }

    void Logger::PushLog(spdlog::level::level_enum level, const std::string &message)
    {
        if (!impl)
        {
            Init();
        }
        if (impl && impl->consoleSink)
        {
            impl->consoleSink->m_Messages.push_back({ level, message });
            LogCallback cb;
            {
                std::lock_guard<std::mutex> lock(s_CallbackMutex);
                cb = s_LogCallback;
            }
            if (cb)
            {
                cb(level, message);
            }
        }
    }

    void Logger::SetCallback(LogCallback callback)
    {
        std::lock_guard<std::mutex> lock(s_CallbackMutex);
        s_LogCallback = std::move(callback);
    }

    void Logger::ClearCallback()
    {
        std::lock_guard<std::mutex> lock(s_CallbackMutex);
        s_LogCallback = nullptr;
    }

    spdlog::logger *Logger::GetLogger()
    {
        if (!impl)
        {
            Init();
        }
        if (impl && impl->logger)
        {
            return impl->logger.get();
        }
        return spdlog::default_logger().get();
    }
}
