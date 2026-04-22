/**************************************************************************/
/*  global_input_monitor_macos.mm                                         */
/**************************************************************************/
/*  macOS 实现：CGEventTap（全局事件监听）                                   */
/*                                                                        */
/*  注意：应用需在 Info.plist 声明                                           */
/*    NSAppleEventsUsageDescription  (或)                                  */
/*  并在系统偏好 → 安全与隐私 → 辅助功能 中授权，否则 tap 创建失败。          */
/**************************************************************************/

#ifdef MACOS_ENABLED

#include "global_input_monitor_macos.h"

#import <ApplicationServices/ApplicationServices.h>
#import <Foundation/Foundation.h>

/* ------------------------------------------------------------------ */
/*  CGEventTap 回调                                                     */
/* ------------------------------------------------------------------ */
CGEventRef GlobalInputMonitorMacOS::_event_callback(
        CGEventTapProxy /*proxy*/,
        CGEventType     type,
        CGEventRef      event,
        void           *refcon)
{
    GlobalInputMonitorMacOS *self =
            static_cast<GlobalInputMonitorMacOS *>(refcon);

    switch (type) {
        /* 键盘 */
        case kCGEventKeyDown: {
            CGKeyCode kc = (CGKeyCode)CGEventGetIntegerValueField(
                    event, kCGKeyboardEventKeycode);
            self->call_deferred("_emit_key_pressed", (int)kc);
        } break;
        case kCGEventKeyUp: {
            CGKeyCode kc = (CGKeyCode)CGEventGetIntegerValueField(
                    event, kCGKeyboardEventKeycode);
            self->call_deferred("_emit_key_released", (int)kc);
        } break;

        /* 鼠标按键：1=左 2=右 3=其他（通过 MouseButton 字段区分） */
        case kCGEventLeftMouseDown:
            self->call_deferred("_emit_mouse_button_pressed", 1);
            break;
        case kCGEventLeftMouseUp:
            self->call_deferred("_emit_mouse_button_released", 1);
            break;
        case kCGEventRightMouseDown:
            self->call_deferred("_emit_mouse_button_pressed", 2);
            break;
        case kCGEventRightMouseUp:
            self->call_deferred("_emit_mouse_button_released", 2);
            break;
        case kCGEventOtherMouseDown: {
            int btn = (int)CGEventGetIntegerValueField(
                    event, kCGMouseEventButtonNumber);
            /* btn==2 → 中键(3)，btn>=3 → X 键(4/5) */
            self->call_deferred("_emit_mouse_button_pressed", btn + 1);
        } break;
        case kCGEventOtherMouseUp: {
            int btn = (int)CGEventGetIntegerValueField(
                    event, kCGMouseEventButtonNumber);
            self->call_deferred("_emit_mouse_button_released", btn + 1);
        } break;

        /* 鼠标移动 */
        case kCGEventMouseMoved:
        case kCGEventLeftMouseDragged:
        case kCGEventRightMouseDragged:
        case kCGEventOtherMouseDragged: {
            CGPoint pt = CGEventGetLocation(event);
            self->call_deferred("_emit_mouse_moved", (int)pt.x, (int)pt.y);
        } break;

        /* tap 被系统禁用时（如锁屏），重新启用 */
        case kCGEventTapDisabledByTimeout:
        case kCGEventTapDisabledByUserInput:
            if (self->event_tap) {
                CGEventTapEnable((CFMachPortRef)self->event_tap, true);
            }
            break;

        default:
            break;
    }

    /* 返回原事件，不吞掉 */
    return event;
}

/* ------------------------------------------------------------------ */
/*  start                                                               */
/* ------------------------------------------------------------------ */
void GlobalInputMonitorMacOS::start() {
    if (_enabled) return;

    /* 监听的事件掩码 */
    CGEventMask mask =
            CGEventMaskBit(kCGEventKeyDown)         |
            CGEventMaskBit(kCGEventKeyUp)           |
            CGEventMaskBit(kCGEventLeftMouseDown)   |
            CGEventMaskBit(kCGEventLeftMouseUp)     |
            CGEventMaskBit(kCGEventRightMouseDown)  |
            CGEventMaskBit(kCGEventRightMouseUp)    |
            CGEventMaskBit(kCGEventOtherMouseDown)  |
            CGEventMaskBit(kCGEventOtherMouseUp)    |
            CGEventMaskBit(kCGEventMouseMoved)      |
            CGEventMaskBit(kCGEventLeftMouseDragged)|
            CGEventMaskBit(kCGEventRightMouseDragged)|
            CGEventMaskBit(kCGEventOtherMouseDragged);

    CFMachPortRef tap = CGEventTapCreate(
            kCGSessionEventTap,          /* 监听整个用户会话 */
            kCGHeadInsertEventTap,       /* 插入队列头部 */
            kCGEventTapOptionListenOnly, /* 只监听，不修改 */
            mask,
            _event_callback,
            this);

    if (!tap) {
        /* 没有辅助功能授权，静默失败并打印提示 */
        print_line("GlobalInputMonitor (macOS): CGEventTap creation failed. "
                   "Please grant Accessibility permission in System Preferences.");
        return;
    }

    CFRunLoopSourceRef src =
            CFMachPortCreateRunLoopSource(kCFAllocatorDefault, tap, 0);
    CFRunLoopAddSource(CFRunLoopGetMain(), src, kCFRunLoopCommonModes);
    CGEventTapEnable(tap, true);

    event_tap    = (void *)tap;
    run_loop_src = (void *)src;
    _enabled     = true;
}

/* ------------------------------------------------------------------ */
/*  stop                                                                */
/* ------------------------------------------------------------------ */
void GlobalInputMonitorMacOS::stop() {
    if (!_enabled) return;

    if (event_tap) {
        CGEventTapEnable((CFMachPortRef)event_tap, false);
        CFRunLoopRemoveSource(
                CFRunLoopGetMain(),
                (CFRunLoopSourceRef)run_loop_src,
                kCFRunLoopCommonModes);
        CFRelease((CFMachPortRef)event_tap);
        CFRelease((CFRunLoopSourceRef)run_loop_src);
        event_tap    = nullptr;
        run_loop_src = nullptr;
    }

    _enabled = false;
}

#endif // MACOS_ENABLED
