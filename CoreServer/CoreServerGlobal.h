#pragma once
#include <filesystem>
#include <memory>
#include <QApplication>
#include "FileSystem/FileSystem.h"
#include "LogSystem/LogSystem.h"
#include "ThreadPool/ThreadPool.h"
// 根据构建类型设置默认日志级别
#ifdef NDEBUG
#define DEFAULT_LOG_LEVEL EM_LogLevel::Error
#else
#define DEFAULT_LOG_LEVEL EM_LogLevel::Info
#endif

/// <summary>
/// 核心服务全局类
/// </summary>
class CoreServerGlobal
{
public:
    /// <summary>
    /// 获取单例
    /// </summary>
    /// <returns>单例引用</returns>
    static CoreServerGlobal& Instance()
    {
        static CoreServerGlobal instance;
        return instance;
    }

    /// <summary>
    /// 获取线程池
    /// </summary>
    /// <returns>线程池引用</returns>
    ThreadPool& GetThreadPool()
    {
        return *m_threadPool;
    }

    /// <summary>
    /// 获取日志系统
    /// </summary>
    /// <returns>日志系统引用</returns>
    LogSystem& GetLogSystem()
    {
        return LogSystem::Instance();
    }
    /// <summary>
    /// 初始化服务
    /// </summary>
    void Initialize();


    /// <summary>
    /// 关闭服务
    /// </summary>
    void Shutdown()
    {
        // 记录关闭日志
        LOG_INFO("Application shutting down");

        // 关闭日志系统
        GetLogSystem().Shutdown();

        // 关闭线程池
        m_threadPool->Shutdown();
    }

private:
    /// <summary>
    /// 构造函数
    /// </summary>
    CoreServerGlobal();
    CoreServerGlobal(const CoreServerGlobal&) = delete;
    CoreServerGlobal& operator=(const CoreServerGlobal&) = delete;
    ~CoreServerGlobal();

private:
    ThreadPool* m_threadPool = nullptr; /// 线程池
};
