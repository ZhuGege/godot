/**************************************************************************/
/*  global_input_monitor_windows.h                                        */
/**************************************************************************/
#pragma once

#ifdef WINDOWS_ENABLED

#include "global_input_monitor.h"
#include "core/os/thread.h"
#include "core/templates/safe_refcount.h"

class GlobalInputMonitorWindows : public GlobalInputMonitor {
    GDCLASS(GlobalInputMonitorWindows, GlobalInputMonitor);

    Thread  poll_thread;
    SafeFlag running;

    /* 上一帧按键状态缓存 */
    bool prev_vk[256]  = {};
    bool prev_mb[6]    = {};  /* 1=左 2=右 3=中 4=X1 5=X2 */

    static void _thread_func(void *p_userdata);
    void _poll_loop();

public:
    bool is_enabled() const { return running.is_set(); }

    virtual void start() override;
    virtual void stop()  override;

    GlobalInputMonitorWindows()  = default;
    ~GlobalInputMonitorWindows() { stop(); }
};

#endif // WINDOWS_ENABLED
