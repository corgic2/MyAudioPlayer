#pragma once

#include <QObject>

class BaseFFmpegUtils : public QObject
{
    Q_OBJECT public:
    BaseFFmpegUtils(QObject* parent);
    ~BaseFFmpegUtils() override;

    virtual void StartRecording(const QString& saveFilePath) = 0;
    virtual void StopRecording() = 0;

    virtual void StartPlay(const QString& filePath, double startPosition = 0.0, const QStringList& args = QStringList()) = 0;

    virtual void PausePlay() = 0;
    virtual void ResumePlay() = 0;

    virtual void StopPlay() = 0;

    virtual void SeekPlay(double position) = 0;

    virtual bool IsPlaying() = 0;
    virtual bool IsPaused() = 0;
    virtual bool IsRecording() = 0;
};
