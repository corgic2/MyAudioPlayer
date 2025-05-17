#include <QApplication>
#include <QDebug>
#include <QTextCodec>
#include "UIModule/AudioMainWidget.h"
#include "Utils/FFmpegUtils.h"


void InitCoreObject()
{
    FFmpegUtils::ResigsterDevice();
    // 设置全局编码为UTF-8（Qt 5及以下）
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
}
int main(int argc, char* argv[])
{
    InitCoreObject();
    QApplication a(argc, argv);
    AudioMainWidget widget;
    widget.show();
    return a.exec();
}
