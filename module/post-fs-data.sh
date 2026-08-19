#!/system/bin/sh
MODDIR="${0%/*}"
DETACH_DIR="/data/adb/detach"

mkdir -p "$DETACH_DIR"

# SELinux context (bukan menonaktifkan SELinux)
if [ -f "/sys/fs/selinux/enforce" ]; then
    chcon u:object_r:magisk_file:s0 "$DETACH_DIR" 2>/dev/null
fi

# Optimisasi miui/hyperos dimatikan agar hook lebih reliable
MIUI_V=$(getprop ro.miui.ui.version.name)
HYPER_V=$(getprop ro.mi.os.version.name)
if [ "$MIUI_V" != "" ] || [ "$HYPER_V" != "" ]; then
    resetprop persist.sys.miui_optimization false 2>/dev/null
fi

exit 0
