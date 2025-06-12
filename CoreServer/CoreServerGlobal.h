#pragma once
#include <filesystem>
#include <memory>
#include <QApplication>
#include "CrashHandler.h"
#include "FileSystem/FileSystem.h"
#include "LogSystem/LogSystem.h"
#include "ThreadPool/ThreadPool.h"

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
    ThreadPool& GetThreadPool() { return *m_threadPool; }

    /// <summary>
    /// 获取日志系统
    /// </summary>
    /// <returns>日志系统引用</returns>
    LogSystem& GetLogSystem() { return LogSystem::Instance(); }

    /// <summary>
    /// 获取崩溃处理器
    /// </summary>
    /// <returns>崩溃处理器引用</returns>
    CrashHandler& GetCrashHandler() { return CrashHandler::Instance(); }

    /// <summary>
    /// 初始化服务
    /// </summary>
    void Initialize()
    {
        // 配置线程池
        ST_ThreadPoolConfig config;
        config.m_minThreads = 2;
        config.m_maxThreads = 4;
        config.m_maxQueueSize = 1000;
        config.m_keepAliveTime = 60000; // 1分钟
        m_threadPool = new ThreadPool(config);
        // 创建日志目录
        QString logDir = QApplication::applicationDirPath() + QString("logs");
        if (!my_sdk::FileSystem::Exists(logDir.toStdString()))
        {
            my_sdk::FileSystem::CreateWindowsDirectory(logDir.toStdString());
        }

        // 配置日志系统
        ST_LogConfig logConfig;
        logConfig.m_logFilePath = logDir.toStdString() + "/app.log";
        logConfig.m_logLevel = EM_LogLevel::Debug;
        logConfig.m_maxFileSize = 10 * 1024 * 1024; // 10MB
        logConfig.m_maxQueueSize = 10000;
        logConfig.m_asyncEnabled = true;
        logConfig.m_compressLevel = EM_CompressLevel::Fast;

        GetLogSystem().Initialize(logConfig);

        // 初始化崩溃处理器
        GetCrashHandler().Initialize(logDir.toStdString() + "/crash.log");

        // 设置崩溃回调
        GetCrashHandler().SetCrashCallback([this]()
        {
            // 在崩溃时尝试保存一些重要的状态
            GetLogSystem().Flush(); // 确保所有日志都被写入
            m_threadPool->Shutdown(); // 安全关闭线程池
        });

        // 记录启动日志
        LOG_INFO("Application started");
    }

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
