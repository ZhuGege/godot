/**************************************************************************/
/*  global_input_monitor.h                                                */
/**************************************************************************/
/*  全局键盘 / 鼠标监听模块 — 公共接口                                        */
/*                                                                        */
/*  暴露给 GDScript 的单例：GlobalInputMonitor                              */
/*  信号：                                                                  */
/*    key_pressed(keycode: int)     —— 按键按下（虚拟键码 / CGKeyCode）       */
/*    key_released(keycode: int)    —— 按键释放                              */
/*    mouse_button_pressed(btn: int)  —— 鼠标键按下 (1=左 2=右 3=中)        */
/*    mouse_button_released(btn: int) —— 鼠标键释放                         */
/*    mouse_moved(x: int, y: int)   —— 鼠标移动（全局屏幕坐标）              */
/*    mouse_wheel_rolled(delta: int) —— 鼠标滚轮滚动（正=上 负=下）            */
/*                                                                        */
/*  属性：                                                                  */
/*    enabled : bool  —— 运行时开关，可随时打开 / 关闭监听                    */
/**************************************************************************/

#pragma once

#include "core/object/object.h"
#include "core/os/thread.h"
#include "core/os/mutex.h"
#include "core/templates/safe_refcount.h"

class GlobalInputMonitor : public Object {
    GDCLASS(GlobalInputMonitor, Object);

    static GlobalInputMonitor *singleton;

protected:
    static void _bind_methods();

public:
    /* ---- 单例访问 ---- */
    static GlobalInputMonitor *get_singleton();

    /* ---- GDScript 可用的 API ---- */
    void set_enabled(bool p_enabled);
    virtual bool is_enabled() const { return false; } /* 由平台子类覆盖 */

    /* ---- 平台子类调用：投递事件到主线程 ---- */
    void _emit_key_pressed(int p_keycode);
    void _emit_key_released(int p_keycode);
    void _emit_mouse_button_pressed(int p_button);
    void _emit_mouse_button_released(int p_button);
    void _emit_mouse_moved(int p_x, int p_y);
    void _emit_mouse_wheel_rolled(int p_delta);

    /* ---- 生命周期（由 register_types 调用） ---- */
    virtual void start() {}
    virtual void stop()  {}

    GlobalInputMonitor();
    virtual ~GlobalInputMonitor();
};
