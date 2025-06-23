#include "PlayerAudioModuleWidget.h"


PlayerAudioModuleWidget::PlayerAudioModuleWidget(QWidget* parent)
    : QWidget(parent), ui(new Ui::PlayerAudioModuleWidgetClass())
{
    ui->setupUi(this);
}


PlayerAudioModuleWidget::~PlayerAudioModuleWidget()
{
    SAFE_DELETE_POINTER_VALUE(ui)
}
