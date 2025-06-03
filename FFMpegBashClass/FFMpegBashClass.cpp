#include "FFMpegBashClass.h"
#include <QElapsedTimer>
#include <QProcess>
#include <QRegularExpression>

FFMpegBashClass::ST_CommandResult FFMpegBashClass::ExecuteCommand(const QString& command, int timeoutMs, bool realtimeOutput, std::function<void(const QString&, bool)> callback)
{
    QProcess process;
    ST_CommandResult result;
    QElapsedTimer timer;
    timer.start();

    // 设置工作目录为可执行文件所在目录
    process.setWorkingDirectory(QCoreApplication::applicationDirPath());

#if defined(Q_OS_WIN)
    process.setProgram("cmd.exe");
    process.setNativeArguments(QString("/C \"%1\"").arg(command));
#else
    process.setProgram("bash");
    process.setArguments({"-c", command});
#endif

    // 实时输出处理
    if (realtimeOutput && callback)
    {
        QObject::connect(&process, &QProcess::readyReadStandardOutput,
                         [&]()
                         {
                             QString output = QString::fromLocal8Bit(process.readAllStandardOutput());
                             callback(output, false);
                             result.output += output;
                         });

        QObject::connect(&process, &QProcess::readyReadStandardError,
                         [&]()
                         {
                             QString error = QString::fromLocal8Bit(process.readAllStandardError());
                             callback(error, true);
                             result.error += error;
                         });
    }

    process.start();
    bool started = process.waitForStarted(5000);

    if (!started)
    {
        result.success = false;
        result.exitCode = -1;
        result.error = "Failed to start process";
        return result;
    }

    // 等待执行完成
    if (timeoutMs >= 0)
    {
        if (!process.waitForFinished(timeoutMs))
        {
            process.kill();
            result.success = false;
            result.exitCode = -2;
            result.error = QString("Timeout after %1 ms").arg(timeoutMs);
            return result;
        }
    }
    else
    {
        process.waitForFinished(-1);
    }

    // 收集输出结果
    if (!realtimeOutput)
    {
        result.output = QString::fromLocal8Bit(process.readAllStandardOutput());
        result.error = QString::fromLocal8Bit(process.readAllStandardError());
    }

    result.exitCode = process.exitCode();
    result.success = (process.exitStatus() == QProcess::NormalExit) && (result.exitCode == 0);
    result.executionTime = timer.elapsed();

    return result;
}
