#pragma once
#include <QCoreApplication>
#include <QString>
/// <summary>
/// 封装FFmpeg命令
/// </summary>
class FFMpegBashClass
{
public:
    struct ST_CommandResult
    {
        bool success; // 是否成功执行
        int exitCode; // 退出代码
        QString output; // 标准输出内容
        QString error; // 标准错误内容
        qint64 executionTime; // 执行耗时(ms)
    };

private:
    /// <summary>
    /// 执行命令行命令（同步/异步可选）
    /// </summary>
    /// <param name="command">要执行的完整命令（如："ffmpeg -i input.mp4 output.avi"）</param>
    /// <param name="timeoutMs">超时时间（毫秒），-1表示无限等待</param>
    /// <param name="realtimeOutput">是否实时捕获输出（异步模式有效）</param>
    /// <param name="callback">回调函数，用于处理实时输出</param>
    /// <returns>包含执行结果的CommandResult结构体</returns>
    static ST_CommandResult ExecuteCommand(const QString& command, int timeoutMs = -1, bool realtimeOutput = false, std::function<void(const QString&, bool)> callback = nullptr);
};
