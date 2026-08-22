#pragma once

#include <fstream>
#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#if defined(_DEBUG)
    #ifdef _WIN32
        #define DEBUGBREAK() __debugbreak()
    #elif __linux__
        #define DEBUGBREAK() __builtin_trap()
    #endif
#else
    #define DEBUGBREAK()
#endif

namespace Obscura
{
    struct LogMessage
    {
        spdlog::level::level_enum level;
        std::string message;
    };

    class Logger
    {
    public:
        static void Init();
        static void Shutdown();
        static spdlog::logger* GetLogger();
    };
}

namespace fmt
{
    template<>
    struct formatter<std::streamsize>
    {
        template<typename ParseContext>
        constexpr auto parse(ParseContext &ctx)
        {
            return ctx.begin();
        }

        template<typename FormatContext>
        auto format(const std::streamsize streamSize, FormatContext &ctx) const
        {
            return fmt::format_to(ctx.out(), "{}", static_cast<size_t>(streamSize));
        }
    };

    template<>
    struct formatter<std::streampos>
    {
        template<typename ParseContext>
        constexpr auto parse(ParseContext &ctx)
        {
            return ctx.begin();
        }

        template<typename FormatContext>
        auto format(const std::streampos streamPos, FormatContext &ctx) const
        {
            return fmt::format_to(ctx.out(), "{}", static_cast<size_t>(streamPos));
        }
    };
}

#define LOG_ERROR(...) ::Obscura::Logger::GetLogger()->error(__VA_ARGS__)
#define LOG_INFO(...)  ::Obscura::Logger::GetLogger()->info(__VA_ARGS__)
#define LOG_WARN(...)  ::Obscura::Logger::GetLogger()->warn(__VA_ARGS__)
#define LOG_DEBUG(...) ::Obscura::Logger::GetLogger()->debug(__VA_ARGS__)
#define LOG_TRACE(...) ::Obscura::Logger::GetLogger()->trace(__VA_ARGS__)
#define LOG_ASSERT(check, ...) do { if (!(check)) { LOG_ERROR(__VA_ARGS__); DEBUGBREAK(); } } while(false)

#define LOG_NOT_IMPLEMENTED LOG_ERROR("Not implemented yet!")
