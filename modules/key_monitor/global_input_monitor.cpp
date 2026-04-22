/**************************************************************************/
/*  global_input_monitor.cpp                                              */
/**************************************************************************/
/*  平台无关的公共实现：单例管理、信号绑定、事件投递                           */
/**************************************************************************/

#include "global_input_monitor.h"

#include "core/config/project_settings.h"
#include "core/object/class_db.h"

GlobalInputMonitor *GlobalInputMonitor::singleton = nullptr;

GlobalInputMonitor *GlobalInputMonitor::get_singleton() {
    return singleton;
}

void GlobalInputMonitor::_bind_methods() {
    /* 属性 */
    ClassDB::bind_method(D_METHOD("set_enabled", "enabled"), &GlobalInputMonitor::set_enabled);
    ClassDB::bind_method(D_METHOD("is_enabled"), &GlobalInputMonitor::is_enabled);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "enabled"), "set_enabled", "is_enabled");

    /* 内部投递方法（供 call_deferred 从工作线程回调到主线程） */
    ClassDB::bind_method(D_METHOD("_emit_key_pressed",           "keycode"), &GlobalInputMonitor::_emit_key_pressed);
    ClassDB::bind_method(D_METHOD("_emit_key_released",          "keycode"), &GlobalInputMonitor::_emit_key_released);
    ClassDB::bind_method(D_METHOD("_emit_mouse_button_pressed",  "button"),  &GlobalInputMonitor::_emit_mouse_button_pressed);
    ClassDB::bind_method(D_METHOD("_emit_mouse_button_released", "button"),  &GlobalInputMonitor::_emit_mouse_button_released);
    ClassDB::bind_method(D_METHOD("_emit_mouse_moved",           "x", "y"), &GlobalInputMonitor::_emit_mouse_moved);
    ClassDB::bind_method(D_METHOD("_emit_mouse_wheel_rolled",     "delta"), &GlobalInputMonitor::_emit_mouse_wheel_rolled);

    /* 信号 */
    ADD_SIGNAL(MethodInfo("key_pressed",
            PropertyInfo(Variant::INT, "keycode")));
    ADD_SIGNAL(MethodInfo("key_released",
            PropertyInfo(Variant::INT, "keycode")));
    ADD_SIGNAL(MethodInfo("mouse_button_pressed",
            PropertyInfo(Variant::INT, "button")));
    ADD_SIGNAL(MethodInfo("mouse_button_released",
            PropertyInfo(Variant::INT, "button")));
    ADD_SIGNAL(MethodInfo("mouse_moved",
            PropertyInfo(Variant::INT, "x"),
            PropertyInfo(Variant::INT, "y")));
    ADD_SIGNAL(MethodInfo("mouse_wheel_rolled",
            PropertyInfo(Variant::INT, "delta")));
}

void GlobalInputMonitor::set_enabled(bool p_enabled) {
    if (p_enabled == is_enabled()) {
        return;
    }
    if (p_enabled) {
        start();
    } else {
        stop();
    }
}

/* ---------- 事件投递（平台实现在派生类线程中调用） ---------- */

void GlobalInputMonitor::_emit_key_pressed(int p_keycode) {
    emit_signal("key_pressed", p_keycode);
}

void GlobalInputMonitor::_emit_key_released(int p_keycode) {
    emit_signal("key_released", p_keycode);
}

void GlobalInputMonitor::_emit_mouse_button_pressed(int p_button) {
    emit_signal("mouse_button_pressed", p_button);
}

void GlobalInputMonitor::_emit_mouse_button_released(int p_button) {
    emit_signal("mouse_button_released", p_button);
}

void GlobalInputMonitor::_emit_mouse_moved(int p_x, int p_y) {
    emit_signal("mouse_moved", p_x, p_y);
}

void GlobalInputMonitor::_emit_mouse_wheel_rolled(int p_delta) {
    emit_signal("mouse_wheel_rolled", p_delta);
}

GlobalInputMonitor::GlobalInputMonitor() {
    singleton = this;
}

GlobalInputMonitor::~GlobalInputMonitor() {
    if (singleton == this) {
        singleton = nullptr;
    }
}
