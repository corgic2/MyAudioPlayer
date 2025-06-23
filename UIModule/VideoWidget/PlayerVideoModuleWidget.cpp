#include "PlayerVideoModuleWidget.h"

#include "SDKCommonDefine/SDKCommonDefine.h"


PlayerVideoModuleWidget::PlayerVideoModuleWidget(QWidget *parent) : BaseModuleWidegt(parent), ui(new Ui::PlayerVideoModuleWidgetClass())
{
    ui->setupUi(this);
}


PlayerVideoModuleWidget::~PlayerVideoModuleWidget()
{
    SAFE_DELETE_POINTER_VALUE(ui)
}

BaseFFmpegUtils* PlayerVideoModuleWidget::GetFFMpegUtils()
{
    return &m_audioFFmpeg;
}
