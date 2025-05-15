#include <QApplication>
// 为了使用qDebug函数
#include <QDebug>

// FFmpeg是C语言库
// 有了extern "C"，才能在C++中导入C语言函数
extern "C" {
#include "libavutil/avutil.h"
}

int main(int argc, char* argv[])
{
    // 打印版本信息
    qDebug() << av_version_info();

    QApplication a(argc, argv);
    return a.exec();
}
