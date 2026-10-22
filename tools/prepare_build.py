"""Prepare the pinned ESP-IDF 5.4.2 build without storing a developer's paths.

Run from an activated ESP-IDF shell. No network, device access, or deletion.
Existing sdkconfig is retained so menuconfig changes are not overwritten.
"""
import json
import os
from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[1]


def main():
    value = os.environ.get("IDF_PATH")
    if not value:
        raise ValueError("Activate the ESP-IDF 5.4.2 environment first (IDF_PATH is missing)")
    idf = Path(value).resolve()
    header = (idf / "components/esp_common/include/esp_idf_version.h").read_text(encoding="utf-8")
    version = tuple(int(re.search(r"#define\s+ESP_IDF_VERSION_" + part + r"\s+(\d+)", header)[1])
                    for part in ("MAJOR", "MINOR", "PATCH"))
    if version != (5, 4, 2):
        raise ValueError("This release requires ESP-IDF 5.4.2")
    usb = idf / "components/usb"
    if not (usb / "CMakeLists.txt").is_file():
        raise ValueError("ESP-IDF USB component is missing")
    template = (ROOT / "dependencies.lock.template").read_text(encoding="utf-8")
    if template.count("@IDF_USB_PATH@") != 1:
        raise ValueError("Invalid dependency lock template")
    lock = template.replace("@IDF_USB_PATH@", json.dumps(usb.as_posix()))
    destination = ROOT / "dependencies.lock"
    if destination.exists() and destination.read_text(encoding="utf-8") != lock:
        raise ValueError("Existing dependencies.lock differs; preserve it elsewhere before preparing this release")
    if not destination.exists():
        destination.write_text(lock, encoding="utf-8", newline="\n")
    config = ROOT / "sdkconfig"
    if not config.exists():
        config.write_bytes((ROOT / "config/sdkconfig.release").read_bytes())
    print("Ready: pinned dependencies and local sdkconfig; run idf.py build")


if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError, TypeError) as error:
        raise SystemExit(f"STOP: {error}")
