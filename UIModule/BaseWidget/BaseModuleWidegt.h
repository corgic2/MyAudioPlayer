#pragma once

#include <QWidget>
#include "BaseFFmpegUtils.h"

class BaseModuleWidegt  : public QWidget
{
    Q_OBJECT

public:
    BaseModuleWidegt(QWidget *parent);
    ~BaseModuleWidegt();

     virtual BaseFFmpegUtils *GetFFMpegUtils() = 0;
};

