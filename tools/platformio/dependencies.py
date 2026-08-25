"""Expose WiFiManager's async dependencies to PlatformIO's nested library builds."""
from __future__ import print_function

import glob
import os

from SCons.Script import Import

Import("env")


def append_dependency_include(package_prefix):
    """Add the installed package's source directory when PlatformIO has it."""
    package_root = env.subst("$PROJECT_LIBDEPS_DIR")
    candidates = glob.glob(os.path.join(package_root, package_prefix + "*", "src"))
    if candidates:
        env.Append(CPPPATH=[candidates[0]])


append_dependency_include("ESPAsyncWebServer")
if env.subst("$PIOPLATFORM") == "espressif8266":
    append_dependency_include("ESPAsyncTCP")
elif env.subst("$PIOPLATFORM") == "espressif32":
    append_dependency_include("AsyncTCP")
