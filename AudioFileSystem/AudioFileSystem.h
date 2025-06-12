#pragma once
#include <string>
#include <vector>
#include "FileSystem/FileSystem.h"

namespace audio_player
{
    /// <summary>
    /// 音频文件类型枚举
    /// </summary>
    enum class EM_AudioFileType
    {
        Unknown,    /// 未知类型
        MP3,        /// MP3格式
        WAV,        /// WAV格式
        FLAC,       /// FLAC格式
        M4A         /// M4A格式
    };

    /// <summary>
    /// 音频文件信息结构体
    /// </summary>
    struct ST_AudioFileInfo : public my_sdk::ST_FileInfo
    {
        EM_AudioFileType m_fileType;     /// 音频文件类型
        std::string m_displayName;        /// 显示名称
        std::string m_iconPath;          /// 图标路径

        /// <summary>
        /// 构造函数
        /// </summary>
        ST_AudioFileInfo() : my_sdk::ST_FileInfo(), m_fileType(EM_AudioFileType::Unknown)
        {
        }
    };

    /// <summary>
    /// 音频文件系统类
    /// </summary>
    class AudioFileSystem
    {
    public:
        /// <summary>
        /// 判断文件是否为音频文件
        /// </summary>
        /// <param name="filePath">文件路径</param>
        /// <returns>true表示是音频文件，false表示不是</returns>
        static bool IsAudioFile(const std::string& filePath);

        /// <summary>
        /// 获取音频文件类型
        /// </summary>
        /// <param name="filePath">文件路径</param>
        /// <returns>音频文件类型</returns>
        static EM_AudioFileType GetAudioFileType(const std::string& filePath);

        /// <summary>
        /// 获取音频文件信息
        /// </summary>
        /// <param name="filePath">文件路径</param>
        /// <returns>音频文件信息</returns>
        static ST_AudioFileInfo GetAudioFileInfo(const std::string& filePath);

        /// <summary>
        /// 获取目录下所有音频文件
        /// </summary>
        /// <param name="dir">目录路径</param>
        /// <param name="recursive">是否递归搜索子目录</param>
        /// <returns>音频文件路径列表</returns>
        static std::vector<std::string> GetAudioFiles(const std::string& dir, bool recursive = false);

        /// <summary>
        /// 获取支持的音频文件扩展名列表
        /// </summary>
        /// <returns>扩展名列表（包含点号，如 .mp3）</returns>
        static std::vector<std::string> GetSupportedAudioExtensions();

        /// <summary>
        /// 获取音频文件过滤器（用于文件对话框）
        /// </summary>
        /// <returns>过滤器字符串</returns>
        static std::string GetAudioFileFilter();
    };
} 