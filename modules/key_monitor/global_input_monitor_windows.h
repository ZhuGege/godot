/**************************************************************************/
/*  global_input_monitor_windows.h                                        */
/**************************************************************************/
#pragma once

#ifdef WINDOWS_ENABLED

#include "global_input_monitor.h"
#include "core/os/thread.h"
#include "core/templates/safe_refcount.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

class GlobalInputMonitorWindows : public GlobalInputMonitor {
    GDCLASS(GlobalInputMonitorWindows, GlobalInputMonitor);

    Thread  poll_thread;
    SafeFlag running;

    /* 键盘轮询状态缓存 */
    bool prev_vk[256];

    /* 鼠标按键状态缓存 (1=左 2=右 3=中 4=X1 5=X2) */
    bool prev_mb[6];

    /* 低级鼠标钩子句柄 — 用于捕获滚轮事件 */
    HHOOK mouse_hook;

    static void _thread_func(void *p_userdata);
    void _poll_loop();

    /* 鼠标钩子回调（必须静态，WH_MOUSE_LL 要求） */
    static LRESULT CALLBACK _mouse_hook_proc(int nCode, WPARAM wParam, LPARAM lParam);

public:
    bool is_enabled() const { return running.is_set(); }

    virtual void start() override;
    virtual void stop()  override;

    GlobalInputMonitorWindows();
    ~GlobalInputMonitorWindows() { stop(); }
};

#endif // WINDOWS_ENABLED
