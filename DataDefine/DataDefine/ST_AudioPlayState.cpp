#include "ST_AudioPlayState.h"

void ST_AudioPlayState::Reset()
{
    m_isPlaying = false;
    m_isPaused = false;
    m_isRecording = false;
    m_currentPosition = 0.0;
    m_duration = 0.0;
    m_startTime = 0;
}