#pragma once

#ifndef NDEBUG

    #define ENABLE_RUNTIME_DEBUG 1
    #define ENABLE_DEBUG_REPORT_LOGGING 1
    #define ENABLE_DEBUG_REPORT_INFORMATION_BIT (ENABLE_DEBUG_REPORT_LOGGING & 1)
    #define ENABLE_DEBUG_REPORT_DEBUG_BIT (ENABLE_DEBUG_REPORT_LOGGING & 1)
    #define ENABLE_FPS 1

#else

    #define ENABLE_RUNTIME_DEBUG 0
    #define ENABLE_DEBUG_REPORT_LOGGING 0
    #define ENABLE_DEBUG_REPORT_INFORMATION_BIT 0
    #define ENABLE_DEBUG_REPORT_DEBUG_BIT 0
    #define ENABLE_FPS 0

#endif
