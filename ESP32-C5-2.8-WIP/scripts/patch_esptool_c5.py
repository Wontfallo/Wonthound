from pathlib import Path

Import("env")


def _tool_esptool_dir():
    try:
        package_dir = env.PioPlatform().get_package_dir("tool-esptoolpy")
        if package_dir:
            return Path(package_dir)
    except Exception:
        pass
    return Path.home() / ".platformio" / "packages" / "tool-esptoolpy"


loader = _tool_esptool_dir() / "esptool" / "loader.py"
if not loader.exists():
    print(f"ESP32-C5 esptool patch skipped, loader.py not found: {loader}")
else:
    text = loader.read_text(encoding="utf-8")
    old = """        for b in data:
            state ^= b

        return state
"""
    new = """        for b in data:
            if not isinstance(b, int):
                b = int.from_bytes(b, "little")
            state ^= b

        return state
"""
    if new in text:
        pass
    elif old in text:
        loader.write_text(text.replace(old, new), encoding="utf-8")
        print(f"Patched ESP32-C5 esptool checksum compatibility: {loader}")
    else:
        print(f"ESP32-C5 esptool patch skipped, checksum block not recognized: {loader}")
