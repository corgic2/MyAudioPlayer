#pragma once
#include <string>
#include <functional>

/// <summary>
/// 崩溃处理器类
/// </summary>
class CrashHandler
{
public:
    /// <summary>
    /// 获取单例
    /// </summary>
    /// <returns>单例引用</returns>
    static CrashHandler& Instance()
    {
        static CrashHandler instance;
        return instance;
    }

    /// <summary>
    /// 初始化崩溃处理器
    /// </summary>
    /// <param name="crashLogPath">崩溃日志路径</param>
    void Initialize(const std::string& crashLogPath);

    /// <summary>
    /// 设置崩溃回调函数
    /// </summary>
    /// <param name="callback">回调函数</param>
    void SetCrashCallback(std::function<void()> callback) { m_crashCallback = callback; }

private:
    /// <summary>
    /// 崩溃信号处理函数
    /// </summary>
    /// <param name="signal">信号类型</param>
    static void SignalHandler(int signal);

    /// <summary>
    /// 写入崩溃日志
    /// </summary>
    /// <param name="signal">信号类型</param>
    void WriteCrashLog(int signal);

private:
    CrashHandler() = default;
    CrashHandler(const CrashHandler&) = delete;
    CrashHandler& operator=(const CrashHandler&) = delete;

private:
    std::string m_crashLogPath;                  /// 崩溃日志路径
    std::function<void()> m_crashCallback;       /// 崩溃回调函数
}; 