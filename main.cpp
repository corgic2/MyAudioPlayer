#include <QApplication>
#include <QDebug>
#include <QTextCodec>
#include "CoreServerGlobal.h"
#include "SDKCommonDefine/SDKCommonDefine.h"
#include "UIModule/MainWidget.h"
#include "AudioPlayer//AudioFFmpegPlayer.h"
#include "AudioPlayer/AudioPlayerUtils.h"
#include "BasePlayer//FFmpegPublicUtils.h"
void custom_log(void* ptr, int level, const char* fmt, va_list vl)
{
    if (level <= av_log_get_level())
    {
        vfprintf(stderr, fmt, vl); // 输出到控制台
        // 或写入文件
    }
}

void InitCoreObject()
{
    FFmpegPublicUtils::ResigsterDevice();
    av_log_set_level(AV_LOG_DEBUG);
    // 在初始化时设置回调
    av_log_set_callback(custom_log);
    // 设置全局编码为UTF-8（Qt 5及以下）
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
}
int main(int argc, char* argv[])
{
    InitCoreObject();
    QApplication a(argc, argv);
    MainWidget widget;
    widget.show();
    return a.exec();
}
