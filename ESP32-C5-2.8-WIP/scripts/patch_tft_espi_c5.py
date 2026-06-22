from pathlib import Path

Import("env")


def patch_file(path, replacements):
    text = path.read_text(encoding="utf-8")
    patched = text
    for old, new in replacements:
        patched = patched.replace(old, new)
    if patched != text:
        path.write_text(patched, encoding="utf-8", newline="")
        print(f"Patched ESP32-C5 TFT_eSPI support: {path}")


pioenv = env.subst("$PIOENV")
project_dir = Path(env.subst("$PROJECT_DIR"))
tft_dir = project_dir / ".pio" / "libdeps" / pioenv / "TFT_eSPI"

if tft_dir.exists():
    patch_file(
        tft_dir / "TFT_eSPI.h",
        [
            (
                "#elif defined(CONFIG_IDF_TARGET_ESP32C3)\n  #include \"Processors/TFT_eSPI_ESP32_C3.h\"",
                "#elif defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32C5)\n  #include \"Processors/TFT_eSPI_ESP32_C3.h\"",
            ),
        ],
    )
    patch_file(
        tft_dir / "TFT_eSPI.cpp",
        [
            (
                "#elif defined(CONFIG_IDF_TARGET_ESP32C3)\n    #include \"Processors/TFT_eSPI_ESP32_C3.c\"",
                "#elif defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32C5)\n    #include \"Processors/TFT_eSPI_ESP32_C3.c\"",
            ),
        ],
    )
    patch_file(
        tft_dir / "Processors" / "TFT_eSPI_ESP32_C3.h",
        [
            (
                "#if !defined(CONFIG_IDF_TARGET_ESP32C3) && !defined(CONFIG_IDF_TARGET_ESP32S2) && !defined(CONFIG_IDF_TARGET_ESP32)",
                "#if !defined(CONFIG_IDF_TARGET_ESP32C3) && !defined(CONFIG_IDF_TARGET_ESP32C5) && !defined(CONFIG_IDF_TARGET_ESP32S2) && !defined(CONFIG_IDF_TARGET_ESP32)",
            ),
            (
                "#if CONFIG_IDF_TARGET_ESP32C3",
                "#if CONFIG_IDF_TARGET_ESP32C3 || CONFIG_IDF_TARGET_ESP32C5",
            ),
            (
                "defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32S2)",
                "defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32C5) || defined(CONFIG_IDF_TARGET_ESP32S2)",
            ),
            (
                "#if !defined(CONFIG_IDF_TARGET_ESP32C3)",
                "#if !defined(CONFIG_IDF_TARGET_ESP32C3) && !defined(CONFIG_IDF_TARGET_ESP32C5)",
            ),
        ],
    )
    patch_file(
        tft_dir / "Processors" / "TFT_eSPI_ESP32_C3.c",
        [
            (
                "#if CONFIG_IDF_TARGET_ESP32C3",
                "#if CONFIG_IDF_TARGET_ESP32C3 || CONFIG_IDF_TARGET_ESP32C5",
            ),
        ],
    )
else:
    print(f"TFT_eSPI not installed yet for {pioenv}; C5 patch will apply after libdeps are present.")
