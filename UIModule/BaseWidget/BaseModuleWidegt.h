#pragma once

#include <QWidget>
#include "BasePlayer/BaseFFmpegPlayer.h"

/// <summary>
/// 基础模块控件类
/// </summary>
class BaseModuleWidget : public QWidget
{
    Q_OBJECT

public:
    /// <summary>
    /// 构造函数
    /// </summary>
    /// <param name="parent">父窗口指针</param>
    BaseModuleWidget(QWidget* parent);
    
    /// <summary>
    /// 析构函数
    /// </summary>
    ~BaseModuleWidget();
};

