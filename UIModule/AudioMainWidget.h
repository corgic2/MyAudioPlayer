#pragma once

#include <QMainWindow>
#include "ui_AudioMainWidget.h"

QT_BEGIN_NAMESPACE

namespace Ui
{
    class AudioMainWidgetClass;
};
QT_END_NAMESPACE

class AudioMainWidget : public QMainWindow
{
    Q_OBJECT

public:
    AudioMainWidget(QWidget* parent = nullptr);
    ~AudioMainWidget();

private:
    Ui::AudioMainWidgetClass* ui;
};
