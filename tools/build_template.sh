#!/bin/sh
# build_template.sh -- NEW FILE (2026-10-12), Stellar Frontier Godot port.
# Builds a slim Godot 4.7.2 Windows export template with only what the game
# uses (2D, GL Compatibility, GDScript, GDExtension, the fallback text
# server).  About 30 MB instead of the official 109 MB.
#   tools/build_template.sh GODOT_SOURCE_DIR [lto=full|none]
set -e
cd "$1"
scons platform=windows arch=x86_64 target=template_release use_mingw=yes production=yes \
  ${2:-lto=none} optimize=size debug_symbols=no \
  modules_enabled_by_default=no module_gdscript_enabled=yes module_freetype_enabled=yes \
  module_text_server_fb_enabled=yes module_webp_enabled=yes \
  disable_3d=yes disable_advanced_gui=yes disable_physics_2d=yes disable_physics_3d=yes \
  disable_navigation_2d=yes disable_navigation_3d=yes disable_xr=yes \
  vulkan=no d3d12=no angle=no accesskit=no sdl=no minizip=no scu_build=yes
