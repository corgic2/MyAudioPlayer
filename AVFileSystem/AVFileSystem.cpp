#include "AVFileSystem.h"
#include <algorithm>

namespace av_fileSystem
{
    bool AVFileSystem::IsAVFile(const std::string& filePath)
    {
        std::string ext = my_sdk::FileSystem::GetExtension(filePath);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        const std::vector<std::string> avExts = GetSupportedAVExtensions();
        return std::find(avExts.begin(), avExts.end(), ext) != avExts.end();
    }

    bool AVFileSystem::IsAudioFile(const std::string& filePath)
    {
        EM_AVFileType fileType = GetAVFileType(filePath);
        return IsAudioType(fileType);
    }

    bool AVFileSystem::IsVideoFile(const std::string& filePath)
    {
        EM_AVFileType fileType = GetAVFileType(filePath);
        return IsVideoType(fileType);
    }

    EM_AVFileType AVFileSystem::GetAVFileType(const std::string& filePath)
    {
        std::string ext = my_sdk::FileSystem::GetExtension(filePath);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        ext.erase(ext.begin());
        // 音频格式
        if (ext == "mp3")
        {
            return EM_AVFileType::MP3;
        }
        if (ext == "wav")
        {
            return EM_AVFileType::WAV;
        }
        if (ext == "flac")
        {
            return EM_AVFileType::FLAC;
        }
        if (ext == "m4a")
        {
            return EM_AVFileType::M4A;
        }
        if (ext == "aac")
        {
            return EM_AVFileType::AAC;
        }
        if (ext == "ogg")
        {
            return EM_AVFileType::OGG;
        }
        if (ext == "ape")
        {
            return EM_AVFileType::APE;
        }
        if (ext == "wma")
        {
            return EM_AVFileType::WMA;
        }

        // 视频格式
        if (ext == "mp4")
        {
            return EM_AVFileType::MP4;
        }
        if (ext == "avi")
        {
            return EM_AVFileType::AVI;
        }
        if (ext == "mkv")
        {
            return EM_AVFileType::MKV;
        }
        if (ext == "mov")
        {
            return EM_AVFileType::MOV;
        }
        if (ext == "wmv")
        {
            return EM_AVFileType::WMV;
        }
        if (ext == "flv")
        {
            return EM_AVFileType::FLV;
        }
        if (ext == "webm")
        {
            return EM_AVFileType::WEBM;
        }
        if (ext == "m4v")
        {
            return EM_AVFileType::M4V;
        }
        if (ext == "3gp")
        {
            return EM_AVFileType::_3GP;
        }
        if (ext == "rm")
        {
            return EM_AVFileType::RM;
        }
        if (ext == "rmvb")
        {
            return EM_AVFileType::RMVB;
        }
        if (ext == "asf")
        {
            return EM_AVFileType::ASF;
        }
        if (ext == "ts")
        {
            return EM_AVFileType::TS;
        }
        if (ext == "mts")
        {
            return EM_AVFileType::MTS;
        }

        return EM_AVFileType::Unknown;
    }

    ST_AVFileInfo AVFileSystem::GetAVFileInfo(const std::string& filePath)
    {
        ST_AVFileInfo info;
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

        // 设置音视频特有信息
        info.m_fileType = GetAVFileType(filePath);
        info.m_displayName = my_sdk::FileSystem::GetFileNameWithoutExtension(filePath);
        info.m_isAudioFile = IsAudioType(info.m_fileType);
        info.m_isVideoFile = IsVideoType(info.m_fileType);

        // 根据文件类型设置图标
        if (info.m_isAudioFile)
        {
            info.m_iconPath = ":/icons/audio.png";
        }
        else if (info.m_isVideoFile)
        {
            info.m_iconPath = ":/icons/video.png";
        }
        else
        {
            info.m_iconPath = ":/icons/unknown.png";
        }

        return info;
    }

    ST_AudioFileInfo AVFileSystem::GetAudioFileInfo(const std::string& filePath)
    {
        return GetAVFileInfo(filePath);
    }

    std::vector<std::string> AVFileSystem::GetAVFiles(const std::string& dir, bool recursive)
    {
        std::vector<std::string> allFiles = my_sdk::FileSystem::GetFiles(dir, recursive);
        std::vector<std::string> avFiles;

        for (const auto& file : allFiles)
        {
            if (IsAVFile(file))
            {
                avFiles.push_back(file);
            }
        }

        return avFiles;
    }

    std::vector<std::string> AVFileSystem::GetAudioFiles(const std::string& dir, bool recursive)
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

    std::vector<std::string> AVFileSystem::GetVideoFiles(const std::string& dir, bool recursive)
    {
        std::vector<std::string> allFiles = my_sdk::FileSystem::GetFiles(dir, recursive);
        std::vector<std::string> videoFiles;

        for (const auto& file : allFiles)
        {
            if (IsVideoFile(file))
            {
                videoFiles.push_back(file);
            }
        }

        return videoFiles;
    }

    std::vector<std::string> AVFileSystem::GetSupportedAVExtensions()
    {
        std::vector<std::string> audioExts = GetSupportedAudioExtensions();
        std::vector<std::string> videoExts = GetSupportedVideoExtensions();
        
        std::vector<std::string> allExts;
        allExts.insert(allExts.end(), audioExts.begin(), audioExts.end());
        allExts.insert(allExts.end(), videoExts.begin(), videoExts.end());
        
        return allExts;
    }

    std::vector<std::string> AVFileSystem::GetSupportedAudioExtensions()
    {
        return {
            "mp3", "wav", "flac", "m4a", "aac", "ogg", "ape", "wma"
        };
    }

    std::vector<std::string> AVFileSystem::GetSupportedVideoExtensions()
    {
        return {
            "mp4", "avi", "mkv", "mov", "wmv", "flv", "webm", "m4v", 
            "3gp", "rm", "rmvb", "asf", "ts", "mts"
        };
    }

    std::string AVFileSystem::GetAVFileFilter()
    {
        return "音视频文件 (*.mp3 *.wav *.flac *.m4a *.aac *.ogg *.ape *.wma *.mp4 *.avi *.mkv *.mov *.wmv *.flv *.webm *.m4v *.3gp *.rm *.rmvb *.asf *.ts *.mts);;所有文件 (*.*)";
    }

    std::string AVFileSystem::GetAudioFileFilter()
    {
        return "音频文件 (*.mp3 *.wav *.flac *.m4a *.aac *.ogg *.ape *.wma);;所有文件 (*.*)";
    }

    std::string AVFileSystem::GetVideoFileFilter()
    {
        return "视频文件 (*.mp4 *.avi *.mkv *.mov *.wmv *.flv *.webm *.m4v *.3gp *.rm *.rmvb *.asf *.ts *.mts);;所有文件 (*.*)";
    }

    bool AVFileSystem::IsAudioType(EM_AVFileType fileType)
    {
        switch (fileType)
        {
        case EM_AVFileType::MP3:
        case EM_AVFileType::WAV:
        case EM_AVFileType::FLAC:
        case EM_AVFileType::M4A:
        case EM_AVFileType::AAC:
        case EM_AVFileType::OGG:
        case EM_AVFileType::APE:
        case EM_AVFileType::WMA:
            return true;
        default:
            return false;
        }
    }

    bool AVFileSystem::IsVideoType(EM_AVFileType fileType)
    {
        switch (fileType)
        {
        case EM_AVFileType::MP4:
        case EM_AVFileType::AVI:
        case EM_AVFileType::MKV:
        case EM_AVFileType::MOV:
        case EM_AVFileType::WMV:
        case EM_AVFileType::FLV:
        case EM_AVFileType::WEBM:
        case EM_AVFileType::M4V:
        case EM_AVFileType::_3GP:
        case EM_AVFileType::RM:
        case EM_AVFileType::RMVB:
        case EM_AVFileType::ASF:
        case EM_AVFileType::TS:
        case EM_AVFileType::MTS:
            return true;
        default:
            return false;
        }
    }
} 