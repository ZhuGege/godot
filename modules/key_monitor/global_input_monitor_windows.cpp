/**************************************************************************/
/*  global_input_monitor_windows.cpp                                      */
/**************************************************************************/
/*  Windows 实现：                                                         */
/*    - 键盘：后台线程每 10 ms 轮询 GetAsyncKeyState                      */
/*    - 鼠标按键：同一线程 GetAsyncKeyState 轮询                           */
/*    - 鼠标滚轮：WH_MOUSE_LL 低级钩子捕获 WM_MOUSEWHEEL                   */
/**************************************************************************/

#ifdef WINDOWS_ENABLED

#include "global_input_monitor_windows.h"

#include <windows.h>

/* ------------------------------------------------------------------ */
/*  鼠标虚拟键码 → 按钮编号映射                                          */
/* ------------------------------------------------------------------ */
struct MouseVKMap {
    int vk;
    int btn; /* 1=左 2=右 3=中 4=X1 5=X2 */
};

static const MouseVKMap MOUSE_MAP[] = {
    { VK_LBUTTON,  1 },
    { VK_RBUTTON,  2 },
    { VK_MBUTTON,  3 },
    { VK_XBUTTON1, 4 },
    { VK_XBUTTON2, 5 },
};
static const int MOUSE_MAP_SIZE = sizeof(MOUSE_MAP) / sizeof(MOUSE_MAP[0]);

static bool _is_mouse_vk(int vk) {
    for (int i = 0; i < MOUSE_MAP_SIZE; i++) {
        if (MOUSE_MAP[i].vk == vk) return true;
    }
    return false;
}

/* ------------------------------------------------------------------ */
/*  鼠标钩子 — 用于全局鼠标滚轮检测                                       */
/* ------------------------------------------------------------------ */

/* 静态指针，钩子回调需要访问实例来投递事件 */
static GlobalInputMonitorWindows *hook_instance = nullptr;

LRESULT CALLBACK GlobalInputMonitorWindows::_mouse_hook_proc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0 && hook_instance && wParam == WM_MOUSEWHEEL) {
        MSLLHOOKSTRUCT *mb = (MSLLHOOKSTRUCT *)lParam;
        /* GET_WHEEL_DELTA_WPARAM → 正值向上，负值向下，单位 WHEEL_DELTA(120) */
        short delta = (short)(HIWORD(mb->mouseData));
        hook_instance->call_deferred("_emit_mouse_wheel_rolled", (int)delta);
    }
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

/* ------------------------------------------------------------------ */
/*  线程入口                                                             */
/* ------------------------------------------------------------------ */
void GlobalInputMonitorWindows::_thread_func(void *p_userdata) {
    static_cast<GlobalInputMonitorWindows *>(p_userdata)->_poll_loop();
}

void GlobalInputMonitorWindows::_poll_loop() {
    /* 初始化状态缓存，避免启动时产生误触发 */
    for (int vk = 1; vk <= 0xFE; vk++) {
        prev_vk[vk] = (GetAsyncKeyState(vk) & 0x8000) != 0;
    }
    for (int i = 0; i < MOUSE_MAP_SIZE; i++) {
        prev_mb[i] = (GetAsyncKeyState(MOUSE_MAP[i].vk) & 0x8000) != 0;
    }

    /* 安装低级鼠标钩子（用于捕获滚轮） */
    hook_instance = this;
    mouse_hook = SetWindowsHookEx(WH_MOUSE_LL, _mouse_hook_proc, nullptr, 0);

    POINT last_pt = {};
    GetCursorPos(&last_pt);

    /* 钩子需要消息循环才能工作 */
    while (running.is_set()) {
        /* -- 键盘 -- */
        for (int vk = 1; vk <= 0xFE; vk++) {
            if (_is_mouse_vk(vk)) continue;

            bool cur = (GetAsyncKeyState(vk) & 0x8000) != 0;
            if (cur && !prev_vk[vk]) {
                call_deferred("_emit_key_pressed",  vk);
            } else if (!cur && prev_vk[vk]) {
                call_deferred("_emit_key_released", vk);
            }
            prev_vk[vk] = cur;
        }

        /* -- 鼠标按键 -- */
        for (int i = 0; i < MOUSE_MAP_SIZE; i++) {
            bool cur = (GetAsyncKeyState(MOUSE_MAP[i].vk) & 0x8000) != 0;
            if (cur && !prev_mb[i]) {
                call_deferred("_emit_mouse_button_pressed",  MOUSE_MAP[i].btn);
            } else if (!cur && prev_mb[i]) {
                call_deferred("_emit_mouse_button_released", MOUSE_MAP[i].btn);
            }
            prev_mb[i] = cur;
        }

        /* -- 鼠标移动 -- */
        POINT pt;
        if (GetCursorPos(&pt)) {
            if (pt.x != last_pt.x || pt.y != last_pt.y) {
                call_deferred("_emit_mouse_moved", (int)pt.x, (int)pt.y);
                last_pt = pt;
            }
        }

        /* -- 泵送消息（让 WH_MOUSE_LL 钩子处理滚轮） -- */
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        Sleep(10);
    }

    /* 卸载钩子 */
    if (mouse_hook) {
        UnhookWindowsHookEx(mouse_hook);
        mouse_hook = nullptr;
    }
    hook_instance = nullptr;
}

/* ------------------------------------------------------------------ */
/*  构造 / 析构                                                         */
/* ------------------------------------------------------------------ */
GlobalInputMonitorWindows::GlobalInputMonitorWindows() {
    memset(prev_vk, 0, sizeof(prev_vk));
    memset(prev_mb, 0, sizeof(prev_mb));
    mouse_hook = nullptr;
}

/* ------------------------------------------------------------------ */
/*  start / stop                                                        */
/* ------------------------------------------------------------------ */
void GlobalInputMonitorWindows::start() {
    if (running.is_set()) return;
    running.set();
    poll_thread.start(_thread_func, this);
}

void GlobalInputMonitorWindows::stop() {
    if (!running.is_set()) return;
    running.clear();
    poll_thread.wait_to_finish();

    /* 重置缓存 */
    memset(prev_vk, 0, sizeof(prev_vk));
    memset(prev_mb, 0, sizeof(prev_mb));
}

#endif // WINDOWS_ENABLED
