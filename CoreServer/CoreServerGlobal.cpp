#include "CoreServerGlobal.h"

#include "LogSystem/LogCompressor.h"
#include "SDKCommonDefine/SDKCommonDefine.h"

CoreServerGlobal::~CoreServerGlobal()
{
    SAFE_DELETE_POINTER_VALUE(m_threadPool);
}

void CoreServerGlobal::Initialize()
{
    // 配置线程池
    ST_ThreadPoolConfig config;
    config.m_minThreads = 4;
    config.m_maxThreads = 6;
    config.m_maxQueueSize = 100;
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
    logConfig.m_logFilePath = logDir + "/app.log";
    logConfig.m_logLevel = EM_LogLevel::Debug;
    logConfig.m_maxFileSize = 10 * 1024 * 1024; // 10MB
    logConfig.m_maxQueueSize = 10000;
    logConfig.m_asyncEnabled = true;


    GetLogSystem().Initialize(logConfig);

    // 记录启动日志
    LOG_INFO("Application started");
}

CoreServerGlobal::CoreServerGlobal()
{
    Initialize();
}
