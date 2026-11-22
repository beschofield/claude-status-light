"""Turn the builds into files for the browser flasher (see firmware/README.md).

Usage, from firmware/ after `pio run`:
    python3 tools/make_image.py

For each ring type (rgb, rgbw) it writes to images/:
    status-light-<ring>-full.bin    flash at 0x0: first time, or to recover a board.
                                    Wipes the saved Wi-Fi and settings.
    status-light-<ring>-update.bin  flash at 0xe000: every later update. Keeps Wi-Fi and settings.

Flash layout (min_spiffs.csv): bootloader 0x0, partition table 0x8000, saved Wi-Fi and
settings (nvs) 0x9000, boot selector (otadata) 0xe000, app 0x10000. The update image
starts at the boot selector so a board that ever ran the second app slot boots the new app.
"""

import subprocess
from pathlib import Path

PIO = Path.home() / ".platformio"
ESPTOOL = PIO / "packages/tool-esptool/esptool"
BOOT_APP0 = PIO / "packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin"
OUT = Path("images")


def merge(out: Path, parts: list[tuple[str, Path]], target_offset: str) -> None:
    args = [str(ESPTOOL), "--chip", "esp32c3", "merge-bin", "-o", str(out),
            "--flash-mode", "keep", "--flash-freq", "keep", "--flash-size", "4MB",
            "--target-offset", target_offset]
    for offset, path in parts:
        args += [offset, str(path)]
    subprocess.run(args, check=True, stdout=subprocess.DEVNULL)


def main() -> None:
    OUT.mkdir(exist_ok=True)
    for ring in ("rgb", "rgbw"):
        build = Path(f".pio/build/xiao_{ring}")
        if not (build / "firmware.bin").exists():
            print(f"skip {ring}: run `pio run -e xiao_{ring}` first")
            continue
        app = build / "firmware.bin"
        full = OUT / f"status-light-{ring}-full.bin"
        update = OUT / f"status-light-{ring}-update.bin"
        merge(full, [("0x0", build / "bootloader.bin"), ("0x8000", build / "partitions.bin"),
                     ("0xe000", BOOT_APP0), ("0x10000", app)], "0x0")
        merge(update, [("0xe000", BOOT_APP0), ("0x10000", app)], "0xe000")
        print(f"{ring}: {full} ({full.stat().st_size // 1024} KB), {update}")


if __name__ == "__main__":
    main()
