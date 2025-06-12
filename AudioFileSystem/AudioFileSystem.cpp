#include "AudioFileSystem.h"
#include <algorithm>
#include <QString>

namespace audio_player
{
    bool AudioFileSystem::IsAudioFile(const std::string& filePath)
    {
        std::string ext = my_sdk::FileSystem::GetExtension(filePath);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        const std::vector<std::string> audioExts = GetSupportedAudioExtensions();
        return std::find(audioExts.begin(), audioExts.end(), ext) != audioExts.end();
    }

    EM_AudioFileType AudioFileSystem::GetAudioFileType(const std::string& filePath)
    {
        std::string ext = my_sdk::FileSystem::GetExtension(filePath);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        if (ext == ".mp3")
        {
            return EM_AudioFileType::MP3;
        }
        if (ext == ".wav")
        {
            return EM_AudioFileType::WAV;
        }
        if (ext == ".flac")
        {
            return EM_AudioFileType::FLAC;
        }
        if (ext == ".m4a")
        {
            return EM_AudioFileType::M4A;
        }

        return EM_AudioFileType::Unknown;
    }

    ST_AudioFileInfo AudioFileSystem::GetAudioFileInfo(const std::string& filePath)
    {
        ST_AudioFileInfo info;
        my_sdk::ST_FileInfo baseInfo = my_sdk::FileSystem::GetFileInfo(filePath);

        // 复制基本信息
        info.m_name = baseInfo.m_name;
        info.m_path = baseInfo.m_path;
        info.m_size = baseInfo.m_size;
        info.m_createTime = baseInfo.m_createTime;
        info.m_modifyTime = baseInfo.m_modifyTime;
        info.m_accessTime = baseInfo.m_accessTime;
        info.m_isDirectory = baseInfo.m_isDirectory;
        info.m_isReadOnly = baseInfo.m_isReadOnly;

        // 设置音频特有信息
        info.m_fileType = GetAudioFileType(filePath);
        info.m_displayName = my_sdk::FileSystem::GetFileNameWithoutExtension(filePath);
        info.m_iconPath = ":/icons/audio.png"; // 默认图标

        return info;
    }

    std::vector<std::string> AudioFileSystem::GetAudioFiles(const std::string& dir, bool recursive)
    {
        std::vector<std::string> allFiles = my_sdk::FileSystem::GetFiles(dir, recursive);
        std::vector<std::string> audioFiles;

        for (const auto& file : allFiles)
        {
            if (IsAudioFile(file))
            {
                audioFiles.push_back(file);
            }
        }

        return audioFiles;
    }

    std::vector<std::string> AudioFileSystem::GetSupportedAudioExtensions()
    {
        return {"mp3", "wav", "flac", "m4a"};
    }

    std::string AudioFileSystem::GetAudioFileFilter()
    {
        return "音频文件 (*.mp3 *.wav *.flac *.m4a);;所有文件 (*.*)";
    }
} 