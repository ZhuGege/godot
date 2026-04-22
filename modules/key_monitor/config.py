def can_build(env, platform):
    # 仅在 Windows 和 macOS 上编译（有实现）
    # 其他平台也可编译，只是监听功能为空实现
    return platform in ("windows", "macos", "linuxbsd")


def configure(env):
    pass
