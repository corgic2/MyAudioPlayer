#include "CoreServerGlobal.h"

#include "SDKCommonDefine/SDKCommonDefine.h"

CoreServerGlobal::~CoreServerGlobal()
{
    SAFE_DELETE_POINTER_VALUE(m_threadPool);
}

CoreServerGlobal::CoreServerGlobal()
{
    Initialize();
}
