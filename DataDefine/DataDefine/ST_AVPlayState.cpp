#include "ST_AVPlayState.h"


void ST_AVPlayState::Reset()
{
    m_isPlaying = false;
    m_isPaused = false;
    m_isRecording = false;
    m_currentPosition = 0.0;
    m_duration = 0.0;
    m_startTime = 0;
}