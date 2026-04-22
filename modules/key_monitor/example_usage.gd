## GlobalInputMonitor 使用示例
## 将本脚本挂载到任意节点即可

extends Node

func _ready() -> void:
    # 方式一：通过 ProjectSettings 配置（推荐）
    # 在 项目设置 → input/global_monitor/enabled 中勾选即可自动启动
    # 下面演示运行时动态控制：

    var monitor := GlobalInputMonitor  # 引擎单例，直接使用类名访问

    # 连接信号
    monitor.key_pressed.connect(_on_key_pressed)
    monitor.key_released.connect(_on_key_released)
    monitor.mouse_button_pressed.connect(_on_mouse_button_pressed)
    monitor.mouse_button_released.connect(_on_mouse_button_released)
    monitor.mouse_moved.connect(_on_mouse_moved)

    # 运行时启用（若 ProjectSettings 已设 true 则不需要这行）
    monitor.enabled = true


func _on_key_pressed(keycode: int) -> void:
    # Windows: keycode 是 Virtual Key Code (VK_*)
    # macOS  : keycode 是 CGKeyCode
    print("按键按下: ", keycode)


func _on_key_released(keycode: int) -> void:
    print("按键释放: ", keycode)


func _on_mouse_button_pressed(button: int) -> void:
    # 1=左键 2=右键 3=中键 4=X1 5=X2
    print("鼠标键按下: ", button)


func _on_mouse_button_released(button: int) -> void:
    print("鼠标键释放: ", button)


func _on_mouse_moved(x: int, y: int) -> void:
    # 全局屏幕坐标
    print("鼠标移动: (%d, %d)" % [x, y])


func _exit_tree() -> void:
    # 离开时关闭监听（可选）
    GlobalInputMonitor.enabled = false
