#include "CrashHandler.h"
#include <csignal>
#include <ctime>
#include <iomanip>
#include "FileSystem/FileSystem.h"
#include "LogSystem/LogSystem.h"

#ifdef _WIN32
#include <windows.h>
#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")
#else
#include <execinfo.h>
#endif

void CrashHandler::Initialize(const std::string& crashLogPath)
{
    m_crashLogPath = crashLogPath;

    // 创建日志目录
    std::string logDir = my_sdk::FileSystem::GetFileNameWithoutExtension(crashLogPath);
    if (!my_sdk::FileSystem::Exists(logDir))
    {
        my_sdk::FileSystem::CreateWindowsDirectory(logDir);
    }

    // 注册信号处理器
    signal(SIGSEGV, SignalHandler); // 段错误
    signal(SIGABRT, SignalHandler); // 异常终止
    signal(SIGFPE, SignalHandler);  // 浮点异常
    signal(SIGILL, SignalHandler);  // 非法指令

#ifdef _WIN32
    SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)SignalHandler);
#endif

    LOG_INFO("Crash handler initialized, crash logs will be saved to: %s", crashLogPath.c_str());
}

void CrashHandler::SignalHandler(int signal)
{
    Instance().WriteCrashLog(signal);

    // 调用崩溃回调
    if (Instance().m_crashCallback)
    {
        Instance().m_crashCallback();
    }

    // 终止程序
    std::abort();
}

void CrashHandler::WriteCrashLog(int signal)
{
    try
    {
        // 获取当前时间
        auto now = std::time(nullptr);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&now), "%Y-%m-%d %H:%M:%S");

        // 准备崩溃日志内容
        std::stringstream crashContent;
        crashContent << "\n=== Crash Report ===\n";
        crashContent << "Time: " << ss.str() << "\n";
        crashContent << "Signal: " << signal << "\n";

        // 获取调用栈
#ifdef _WIN32
        HANDLE process = GetCurrentProcess();
        HANDLE thread = GetCurrentThread();

        SymInitialize(process, NULL, TRUE);

        void* stack[100];
        unsigned short frames = CaptureStackBackTrace(0, 100, stack, NULL);
        auto symbol = (SYMBOL_INFO*)calloc(sizeof(SYMBOL_INFO) + 256 * sizeof(char), 1);
        symbol->MaxNameLen = 255;
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

        crashContent << "\nCall Stack:\n";
        for (unsigned int i = 0; i < frames; i++)
        {
            SymFromAddr(process, (DWORD64)(stack[i]), 0, symbol);
            crashContent << "Frame " << frames - i - 1 << ": " << symbol->Name << "\n";
        }

        free(symbol);
#else
        void* array[100];
        int size = backtrace(array, 100);
        char** strings = backtrace_symbols(array, size);

        crashContent << "\nCall Stack:\n";
        for (int i = 0; i < size; i++)
        {
            crashContent << strings[i] << "\n";
        }

        free(strings);
#endif

        crashContent << "\n=== End of Crash Report ===\n\n";

        // 使用FileSystem API写入崩溃日志，确保UTF-8编码
        std::string existingContent;
        if (my_sdk::FileSystem::Exists(m_crashLogPath))
        {
            existingContent = my_sdk::FileSystem::ReadStringFromFile(m_crashLogPath, true); // 移除BOM
        }

        std::string finalContent = existingContent + crashContent.str();
        my_sdk::FileSystem::WriteStringToFile(m_crashLogPath, finalContent, true); // 写入BOM

        // 记录到日志系统
        LOG_ERROR("Application crashed with signal %d", signal);
    } catch (...)
    {
        // 防止在崩溃处理时发生异常
    }
}
