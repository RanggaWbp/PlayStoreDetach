#!/system/bin/sh
MODDIR="${0%/*}"
DETACH_DIR="/data/adb/detach"
PLAY_STORE_PKG="com.android.vending"

mkdir -p "$DETACH_DIR"
[ ! -f "$DETACH_DIR/detach.list" ] && touch "$DETACH_DIR/detach.list"
[ ! -f "$DETACH_DIR/config.json" ] && \
    echo '{"debug": false, "force_detach": true, "hide_from_play": true}' > "$DETACH_DIR/config.json"

# Tunggu boot selesai
while [ "$(getprop sys.boot_completed)" != "1" ]; do
    sleep 1
done
sleep 5

# Restart Play Store agar hook di-inject ulang
am force-stop "$PLAY_STORE_PKG" 2>/dev/null

echo "$(date '+%Y-%m-%d %H:%M:%S') Detach service started" >> "$DETACH_DIR/detach.log"
