#!/usr/bin/env python3

import json
import os
from pathlib import Path

FIRMWARE_DIR = Path("web/firmware")
REQUIRED_FILES = ("bootloader.bin", "partitions.bin", "boot_app0.bin", "firmware.bin")

def fill_manifest(github_username, github_project, github_build_number):
    missing = [name for name in REQUIRED_FILES if not (FIRMWARE_DIR / name).is_file()]
    if missing:
        raise ValueError(f"Missing firmware parts in {FIRMWARE_DIR}: {', '.join(missing)}")

    return {
        "name": f"{github_username}/{github_project}",
        "version": f"0.0.{github_build_number}",
        "funding_url": f"https://github.com/{github_username}/{github_project}/contributors",
        "builds": [
            {
                "chipFamily": "ESP32",
                "parts": [
                    {"path": "firmware/bootloader.bin", "offset": 4096},
                    {"path": "firmware/partitions.bin", "offset": 32768},
                    {"path": "firmware/boot_app0.bin", "offset": 57344},
                    {"path": "firmware/firmware.bin", "offset": 65536}
                ]
            }
        ]
    }

if __name__ == "__main__":
    github_username, github_project = os.environ.get("GITHUB_REPOSITORY", "user/repo").split("/", 1)
    github_build_number = os.environ.get("GITHUB_RUN_NUMBER", "0")
    manifest_data = fill_manifest(github_username, github_project, github_build_number)
    output_file = Path("web/esp-webtools-manifest.json")
    output_file.parent.mkdir(parents=True, exist_ok=True)
    output_file.write_text(json.dumps(manifest_data, indent=2) + "\n", encoding="utf-8")
    print(f"Manifest generated: {output_file}")
