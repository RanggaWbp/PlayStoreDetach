#!/system/bin/sh
SKIPUNZIP=0

API=$(getprop ro.build.version.sdk)
if [ "$API" -lt 24 ]; then
    abort "! Minimal Android 7.0 (API 24)"
fi

ui_print "- Device    : $(getprop ro.product.model)"
ui_print "- Android   : $(getprop ro.build.version.release) (API $API)"
ui_print "- Arch      : $(getprop ro.product.cpu.abi)"

# Deteksi OEM — miui/hyperos GABUNGAN
OEM="AOSP"
if [ "$(getprop ro.mi.os.version.name)" != "" ] || \
   [ "$(getprop ro.mi.os.version.code)" != "" ] || \
   [ "$(getprop ro.miui.ui.version.name)" != "" ] || \
   [ -d "/data/miui" ] || [ -d "/system/miui" ]; then
    OEM="miui/hyperos"
elif [ "$(getprop ro.build.version.oneui)" != "" ]; then
    OEM="Samsung OneUI"
elif [ "$(getprop ro.build.version.opporom)" != "" ] || \
     [ "$(getprop ro.oplus.version.release)" != "" ]; then
    OEM="OPPO ColorOS"
elif [ "$(getprop ro.build.version.emui)" != "" ]; then
    OEM="Huawei EMUI"
fi
ui_print "- OS Skin   : $OEM"

if [ "$API" -gt 37 ]; then
    ui_print "- CATATAN: SDK $API > 37 — mode future-proof aktif"
fi

unzip -o "$ZIPFILE" -d "$MODPATH" >/dev/null 2>&1

set_perm_recursive "$MODPATH" 0 0 0755 0644
set_perm "$MODPATH/service.sh" 0 0 0755
set_perm "$MODPATH/post-fs-data.sh" 0 0 0755
set_perm "$MODPATH/action.sh" 0 0 0755
set_perm "$MODPATH/uninstall.sh" 0 0 0755
set_perm_recursive "$MODPATH/webroot" 0 0 0755 0644

mkdir -p "$MODPATH/config"
touch "$MODPATH/config/detach.list"

ui_print " "
ui_print "  ╔══════════════════════════════════════╗"
ui_print "  ║   PlayStoreDetach V1.0 — Terinstall! ║"
ui_print "  ╠══════════════════════════════════════╣"
ui_print "  ║  Kelola daftar via WebUI di          ║"
ui_print "  ║  KernelSU/APatch Manager, atau edit: ║"
ui_print "  ║  /data/adb/detach/detach.list        ║"
ui_print "  ╚══════════════════════════════════════╝"
