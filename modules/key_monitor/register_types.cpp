/**************************************************************************/
/*  register_types.cpp  —  key_monitor module                            */
/**************************************************************************/

#include "register_types.h"

#include "core/config/project_settings.h"
#include "core/object/class_db.h"
#include "core/config/engine.h"   /* Engine 单例在此 */
#include "core/os/memory.h"
#include "global_input_monitor.h"

#if defined(WINDOWS_ENABLED)
#include "global_input_monitor_windows.h"
#endif
#if defined(MACOS_ENABLED)
#include "global_input_monitor_macos.h"
#endif

static GlobalInputMonitor *monitor_instance = nullptr;

void initialize_key_monitor_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }

    /* 注册 ProjectSettings 配置项：
       input/global_monitor/enabled  (默认 false)                        */
    GLOBAL_DEF_BASIC("input/global_monitor/enabled", false);

    /* 创建平台对应的实现 */
#if defined(WINDOWS_ENABLED)
    monitor_instance = memnew(GlobalInputMonitorWindows);
    ClassDB::register_class<GlobalInputMonitorWindows>();
#elif defined(MACOS_ENABLED)
    monitor_instance = memnew(GlobalInputMonitorMacOS);
    ClassDB::register_class<GlobalInputMonitorMacOS>();
#else
    /* 其他平台：创建空基类（仅提供接口，不启动监听） */
    monitor_instance = memnew(GlobalInputMonitor);
#endif

    ClassDB::register_class<GlobalInputMonitor>();

    /* 注册为引擎单例，GDScript 可通过 GlobalInputMonitor.xxx 访问 */
    Engine::get_singleton()->add_singleton(
            Engine::Singleton("GlobalInputMonitor", monitor_instance));

    /* 根据 ProjectSettings 决定是否自动启动 */
    bool auto_enable = GLOBAL_GET("input/global_monitor/enabled");
    if (auto_enable) {
        monitor_instance->start();
    }
}

void uninitialize_key_monitor_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }

    if (monitor_instance) {
        monitor_instance->stop();
        memdelete(monitor_instance);
        monitor_instance = nullptr;
    }
}
