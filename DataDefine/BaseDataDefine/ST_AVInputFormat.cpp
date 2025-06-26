#pragma once
#include "ST_AVInputFormat.h"

#include "LogSystem/LogSystem.h"

// ST_AVInputFormat implementation
ST_AVInputFormat::ST_AVInputFormat(ST_AVInputFormat &&other) noexcept : m_pInputFormatCtx(other.m_pInputFormatCtx)
{
    other.m_pInputFormatCtx = nullptr;
}

ST_AVInputFormat &ST_AVInputFormat::operator=(ST_AVInputFormat &&other) noexcept
{
    if (this != &other)
    {
        m_pInputFormatCtx = other.m_pInputFormatCtx;
        other.m_pInputFormatCtx = nullptr;
    }
    return *this;
}

void ST_AVInputFormat::FindInputFormat(const std::string &deviceFormat)
{
    m_pInputFormatCtx = av_find_input_format(deviceFormat.c_str());
}

QStringList ST_AVInputFormat::GetDeviceLists(const char* device_name, AVDictionary* device_options,bool bAudio)
{
    QStringList devices;
    int ret = avdevice_list_input_sources(m_pInputFormatCtx, device_name, device_options, &deviceList);
    if (ret < 0)
    {
        char errbuf[1024];
        av_strerror(ret, errbuf, sizeof(errbuf));
        LOG_WARN("Failed to get input devices:" + std::string(errbuf));
        return devices;
    }

    for (int i = 0; i < deviceList->nb_devices; i++)
    {
        if (bAudio)
        {
            if (*deviceList->devices[i]->media_types == AVMEDIA_TYPE_AUDIO)
            {
                devices.append(QString::fromUtf8(deviceList->devices[i]->device_description));
            }
        }
        else
        {
            if (*deviceList->devices[i]->media_types == AVMEDIA_TYPE_VIDEO)
            {
                devices.append(QString::fromUtf8(deviceList->devices[i]->device_description));
            }
        }
    }

    avdevice_free_list_devices(&deviceList);
    return devices;
}
