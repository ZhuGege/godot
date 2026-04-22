/**************************************************************************/
/*  global_input_monitor_macos.h                                          */
/**************************************************************************/
#pragma once

#ifdef MACOS_ENABLED

#include "global_input_monitor.h"

#ifdef __OBJC__
#import <ApplicationServices/ApplicationServices.h>
#import <Foundation/Foundation.h>
#endif

class GlobalInputMonitorMacOS : public GlobalInputMonitor {
    GDCLASS(GlobalInputMonitorMacOS, GlobalInputMonitor);

    bool _enabled = false;

    /* CGEventTap 句柄（用 void* 屏蔽 ObjC 类型，cpp 头不依赖 CF） */
    void *event_tap     = nullptr; /* CFMachPortRef */
    void *run_loop_src  = nullptr; /* CFRunLoopSourceRef */

    static CGEventRef _event_callback(
            CGEventTapProxy proxy,
            CGEventType     type,
            CGEventRef      event,
            void           *refcon);

public:
    bool is_enabled() const { return _enabled; }

    virtual void start() override;
    virtual void stop()  override;

    GlobalInputMonitorMacOS()  = default;
    ~GlobalInputMonitorMacOS() { stop(); }
};

#endif // MACOS_ENABLED
