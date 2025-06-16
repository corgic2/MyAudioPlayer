#pragma once
#include "ST_ResampleSimpleData.h"

/// <summary>
/// 重采样参数封装类
/// </summary>
class ST_ResampleParams
{
  public:
    ST_ResampleParams() = default;

    ST_ResampleSimpleData &GetInput()
    {
        return input;
    }

    ST_ResampleSimpleData &GetOutput()
    {
        return output;
    }
    void SetInput(const ST_ResampleSimpleData &in)
    {
        input = in;
    }
    void SetOutput(const ST_ResampleSimpleData &out)
    {
        output = out;
    }

  private:
    ST_ResampleSimpleData input;  /// 输入参数
    ST_ResampleSimpleData output; /// 输出参数
};





