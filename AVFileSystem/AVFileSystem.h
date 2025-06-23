#pragma once
#include <string>
#include <vector>
#include "FileSystem/FileSystem.h"

namespace AV_player
{
    /// <summary>
    /// 音视频文件类型枚举
    /// </summary>
    enum class EM_AVFileType
    {
        Unknown,
        /// 未知类型
        // 音频格式
        MP3,
        /// MP3格式
        WAV,
        /// WAV格式
        FLAC,
        /// FLAC格式
        M4A,
        /// M4A格式
        AAC,
        /// AAC格式
        OGG,
        /// OGG格式
        APE,
        /// APE格式
        WMA,
        /// WMA格式
        // 视频格式
        MP4,
        /// MP4格式
        AVI,
        /// AVI格式
        MKV,
        /// MKV格式
        MOV,
        /// MOV格式
        WMV,
        /// WMV格式
        FLV,
        /// FLV格式
        WEBM,
        /// WEBM格式
        M4V,
        /// M4V格式
        _3GP,
        /// 3GP格式
        RM,
        /// RM格式
        RMVB,
        /// RMVB格式
        ASF,
        /// ASF格式
        TS,
        /// TS格式
        MTS /// MTS格式
    };

    /// <summary>
    /// 音视频文件信息结构体
    /// </summary>
    struct ST_AVFileInfo : public my_sdk::ST_FileInfo
    {
        EM_AVFileType m_fileType;  /// 音视频文件类型
        std::string m_displayName; /// 显示名称
        std::string m_iconPath;    /// 图标路径
        bool m_isAudioFile;        /// 是否为音频文件
        bool m_isVideoFile;        /// 是否为视频文件

        /// <summary>
        /// 构造函数
        /// </summary>
        ST_AVFileInfo()
            : my_sdk::ST_FileInfo(), m_fileType(EM_AVFileType::Unknown), m_isAudioFile(false), m_isVideoFile(false)
        {
        }
    };

    /// <summary>
    /// 音视频文件信息结构体（兼容性别名）
    /// </summary>
    using ST_AudioFileInfo = ST_AVFileInfo;

    /// <summary>
    /// 音视频文件系统类
    /// </summary>
    class AVFileSystem
    {
    public:
        /// <summary>
        /// 判断文件是否为音视频文件
        /// </summary>
        /// <param name="filePath">文件路径</param>
        /// <returns>true表示是音视频文件，false表示不是</returns>
        static bool IsAVFile(const std::string& filePath);

        /// <summary>
        /// 判断文件是否为音频文件
        /// </summary>
        /// <param name="filePath">文件路径</param>
        /// <returns>true表示是音频文件，false表示不是</returns>
        static bool IsAudioFile(const std::string& filePath);

        /// <summary>
        /// 判断文件是否为视频文件
        /// </summary>
        /// <param name="filePath">文件路径</param>
        /// <returns>true表示是视频文件，false表示不是</returns>
        static bool IsVideoFile(const std::string& filePath);

        /// <summary>
        /// 获取音视频文件类型
        /// </summary>
        /// <param name="filePath">文件路径</param>
        /// <returns>音视频文件类型</returns>
        static EM_AVFileType GetAVFileType(const std::string& filePath);

        /// <summary>
        /// 获取音视频文件信息
        /// </summary>
        /// <param name="filePath">文件路径</param>
        /// <returns>音视频文件信息</returns>
        static ST_AVFileInfo GetAVFileInfo(const std::string& filePath);

        /// <summary>
        /// 获取音频文件信息（兼容性方法）
        /// </summary>
        /// <param name="filePath">文件路径</param>
        /// <returns>音频文件信息</returns>
        static ST_AudioFileInfo GetAudioFileInfo(const std::string& filePath);

        /// <summary>
        /// 获取目录下所有音视频文件
        /// </summary>
        /// <param name="dir">目录路径</param>
        /// <param name="recursive">是否递归搜索子目录</param>
        /// <returns>音视频文件路径列表</returns>
        static std::vector<std::string> GetAVFiles(const std::string& dir, bool recursive = false);

        /// <summary>
        /// 获取目录下所有音频文件
        /// </summary>
        /// <param name="dir">目录路径</param>
        /// <param name="recursive">是否递归搜索子目录</param>
        /// <returns>音频文件路径列表</returns>
        static std::vector<std::string> GetAudioFiles(const std::string& dir, bool recursive = false);

        /// <summary>
        /// 获取目录下所有视频文件
        /// </summary>
        /// <param name="dir">目录路径</param>
        /// <param name="recursive">是否递归搜索子目录</param>
        /// <returns>视频文件路径列表</returns>
        static std::vector<std::string> GetVideoFiles(const std::string& dir, bool recursive = false);

        /// <summary>
        /// 获取支持的音视频文件扩展名列表
        /// </summary>
        /// <returns>扩展名列表（不包含点号，如 mp3）</returns>
        static std::vector<std::string> GetSupportedAVExtensions();

        /// <summary>
        /// 获取支持的音频文件扩展名列表
        /// </summary>
        /// <returns>扩展名列表（不包含点号，如 mp3）</returns>
        static std::vector<std::string> GetSupportedAudioExtensions();

        /// <summary>
        /// 获取支持的视频文件扩展名列表
        /// </summary>
        /// <returns>扩展名列表（不包含点号，如 mp4）</returns>
        static std::vector<std::string> GetSupportedVideoExtensions();

        /// <summary>
        /// 获取音视频文件过滤器（用于文件对话框）
        /// </summary>
        /// <returns>过滤器字符串</returns>
        static std::string GetAVFileFilter();

        /// <summary>
        /// 获取音频文件过滤器（用于文件对话框）
        /// </summary>
        /// <returns>过滤器字符串</returns>
        static std::string GetAudioFileFilter();

        /// <summary>
        /// 获取视频文件过滤器（用于文件对话框）
        /// </summary>
        /// <returns>过滤器字符串</returns>
        static std::string GetVideoFileFilter();

    private:
        /// <summary>
        /// 判断文件类型是否为音频
        /// </summary>
        /// <param name="fileType">文件类型</param>
        /// <returns>true表示是音频文件类型，false表示不是</returns>
        static bool IsAudioType(EM_AVFileType fileType);

        /// <summary>
        /// 判断文件类型是否为视频
        /// </summary>
        /// <param name="fileType">文件类型</param>
        /// <returns>true表示是视频文件类型，false表示不是</returns>
        static bool IsVideoType(EM_AVFileType fileType);
    };
}
