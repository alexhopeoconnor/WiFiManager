#!/usr/bin/env python3
"""One-shot migration: DEBUG_WM -> WiFiManager::log(...). Run from repo root."""

import re
import sys

SUB = "kWiFiMgrLogSubsystem"

REPLACEMENTS = [
    # Longest / most specific first (with optional whitespace)
    (r"_wm->\s*DEBUG_WM\s*\(\s*WM_DEBUG_ERROR\s*,", f"_wm->log(WiFiManagerLogLevel::Error, {SUB},"),
    (r"_wm->\s*DEBUG_WM\s*\(\s*WM_DEBUG_VERBOSE\s*,", f"_wm->log(WiFiManagerLogLevel::Debug, {SUB},"),
    (r"_wm->\s*DEBUG_WM\s*\(\s*WM_DEBUG_DEV\s*,", f"_wm->log(WiFiManagerLogLevel::Trace, {SUB},"),
    (r"_wm->\s*DEBUG_WM\s*\(\s*WM_DEBUG_NOTIFY\s*,", f"_wm->log(WiFiManagerLogLevel::Info, {SUB},"),
    (r"_wm->\s*DEBUG_WM\s*\(\s*WM_DEBUG_MAX\s*,", f"_wm->log(WiFiManagerLogLevel::Trace, {SUB},"),
    (r"DEBUG_WM\s*\(\s*WM_DEBUG_ERROR\s*,", f"log(WiFiManagerLogLevel::Error, {SUB},"),
    (r"DEBUG_WM\s*\(\s*WM_DEBUG_VERBOSE\s*,", f"log(WiFiManagerLogLevel::Debug, {SUB},"),
    (r"DEBUG_WM\s*\(\s*WM_DEBUG_DEV\s*,", f"log(WiFiManagerLogLevel::Trace, {SUB},"),
    (r"DEBUG_WM\s*\(\s*WM_DEBUG_NOTIFY\s*,", f"log(WiFiManagerLogLevel::Info, {SUB},"),
    (r"DEBUG_WM\s*\(\s*WM_DEBUG_MAX\s*,", f"log(WiFiManagerLogLevel::Trace, {SUB},"),
    # Default / notify-level (no explicit WM_DEBUG_* level)
    (r"_wm->\s*DEBUG_WM\s*\(\s*F\(", f"_wm->log(WiFiManagerLogLevel::Info, {SUB}, F("),
    (r"DEBUG_WM\s*\(\s*F\(", f"log(WiFiManagerLogLevel::Info, {SUB}, F("),
    (r"_wm->\s*DEBUG_WM\s*\(", f"_wm->log(WiFiManagerLogLevel::Info, {SUB}, "),
    (r"DEBUG_WM\s*\(", f"log(WiFiManagerLogLevel::Info, {SUB}, "),
]


def migrate(content: str) -> str:
    content = content.replace("#ifdef WM_DEBUG_LEVEL", "#ifndef WM_NO_LOG")
    content = content.replace("#ifdef  WM_DEBUG_LEVEL", "#ifndef WM_NO_LOG")
    for pat, repl in REPLACEMENTS:
        content = re.sub(pat, repl, content)
    return content


def main():
    for path in sys.argv[1:]:
        with open(path, "r", encoding="utf-8") as f:
            old = f.read()
        new = migrate(old)
        if new != old:
            with open(path, "w", encoding="utf-8") as f:
                f.write(new)
            print(f"updated: {path}")
        else:
            print(f"unchanged: {path}")


if __name__ == "__main__":
    main()
