#ifndef _UIAFWD_HPP
#define _UIAFWD_HPP

#include <windows.h>

#include "disable_compiler_warning.hpp"

#if defined(_MSC_VER) && _MSC_VER >= 1500
#include <uiautomationclient.h>
#else
#include <um/uiautomationclient.h>
#endif

#include "enable_compiler_warning.hpp"

#include "smartcom.hpp"


namespace vind
{
    namespace util
    {
        using SmartElement      = SmartCom<IUIAutomationElement> ;
        using SmartElementArray = SmartCom<IUIAutomationElementArray> ;
        using SmartCacheReq     = SmartCom<IUIAutomationCacheRequest> ;
    }
}

#endif
