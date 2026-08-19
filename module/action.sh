#!/system/bin/sh
DETACH_DIR="/data/adb/detach"

echo "=== PlayStoreDetach — Action ==="
echo ""
am force-stop com.android.vending
echo "[✓] Play Store di-restart"

COUNT=$(grep -cv '^\s*$' "$DETACH_DIR/detach.list" 2>/dev/null || echo 0)
echo "[i] Package terputus: $COUNT"
exit 0
