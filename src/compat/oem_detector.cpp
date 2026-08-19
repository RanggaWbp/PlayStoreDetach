/**
 * ============================================================
 * OEM Detection System v2.1 — miui/hyperos GABUNGAN
 * ============================================================
 * Urutan deteksi Xiaomi:
 *   1. HyperOS props (ro.mi.os.*)
 *   2. MIUI props klasik (ro.miui.*)
 *   3. Fingerprint fisik (direktori khas Xiaomi)
 * ============================================================
 */
#include "../../include/common.h"

#include <sys/stat.h>
#include <cstring>
#include <cstdlib>

// ============================================================
// Static member definitions (wajib — dari deklarasi di common.h)
// ============================================================
OEMType OEMDetector::detected_oem = OEMType::UNKNOWN;
XiaomiSkin OEMDetector::xiaomi_skin = XiaomiSkin::NONE;
std::string OEMDetector::skin_version_str = "";
bool OEMDetector::detected = false;

// ============================================================
// Helpers
// ============================================================
static std::string getProp(const char *name) {
    char value[PROP_VALUE_MAX] = {0};
    __system_property_get(name, value);
    return std::string(value);
}

static bool pathExists(const char *path) {
    struct stat st;
    return stat(path, &st) == 0;
}

// ============================================================
// Deteksi Xiaomi miui/hyperos — SATU entitas gabungan
// ============================================================
static bool detectXiaomiSkin(XiaomiSkin &out_skin, std::string &out_version) {

    // ---- 1. HyperOS: properti ro.mi.os.* (eksklusif HyperOS) ----
    std::string hyperos_name = getProp("ro.mi.os.version.name");
    std::string hyperos_code = getProp("ro.mi.os.version.code");
    if (!hyperos_name.empty() || !hyperos_code.empty()) {
        out_skin = XiaomiSkin::HYPEROS;
        out_version = !hyperos_name.empty() ? hyperos_name : hyperos_code;
        return true;
    }

    // ---- 2. MIUI klasik: ro.miui.* ----
    std::string miui_name = getProp("ro.miui.ui.version.name");
    std::string miui_ver  = getProp("ro.miui.version");
    if (!miui_name.empty() || !miui_ver.empty()) {
        out_skin = XiaomiSkin::MIUI;
        out_version = !miui_name.empty() ? miui_name : miui_ver;

        // Edge case: MIUI V15 sebenarnya HyperOS
        if (miui_name.find("V15") != std::string::npos ||
            miui_name.find("OS")  != std::string::npos) {
            out_skin = XiaomiSkin::HYPEROS;
        }
        return true;
    }

    // ---- 3. Fingerprint fisik (build China kadang tanpa props) ----
    if (pathExists("/data/miui") || pathExists("/system/app/miui") ||
        pathExists("/system/miui") || pathExists("/system_ext/app/miui")) {
        out_skin = XiaomiSkin::HYPEROS;
        out_version = "unknown (deteksi fisik)";
        return true;
    }

    // ---- 4. Brand Xiaomi tapi tanpa skin → ROM custom, jangan klaim ----
    std::string brand = getProp("ro.product.brand");
    if (brand == "Xiaomi" || brand == "Redmi" || brand == "POCO") {
        return false;  // kemungkinan custom ROM (PixelOS, LineageOS, dll)
    }

    return false;
}

// ============================================================
// Deteksi OEM Utama
// ============================================================
OEMType OEMDetector::detect() {
    if (detected) return detected_oem;

    detected = true;
    detected_oem = OEMType::UNKNOWN;

    // ---- 1. miui/hyperos (gabungan — prioritas pertama) ----
    if (detectXiaomiSkin(xiaomi_skin, skin_version_str)) {
        detected_oem = OEMType::MIUI_HYPEROS;
        return detected_oem;
    }

    // ---- 2. Samsung OneUI ----
    std::string prop = getProp("ro.build.version.oneui");
    if (!prop.empty()) { detected_oem = OEMType::ONEUI; return detected_oem; }
    prop = getProp("ro.build.version.sep");
    if (!prop.empty() && prop.length() >= 6) {
        detected_oem = OEMType::ONEUI; return detected_oem;
    }

    // ---- 3. OPPO ColorOS (termasuk OnePlus/Realme baru, base OPLUS) ----
    prop = getProp("ro.build.version.opporom");
    if (!prop.empty()) { detected_oem = OEMType::COLOROS; return detected_oem; }
    prop = getProp("ro.oplus.version.release");
    if (!prop.empty()) { detected_oem = OEMType::COLOROS; return detected_oem; }

    // Realme UI (lama, sebelum merger OPLUS)
    prop = getProp("ro.build.version.realmeui");
    if (!prop.empty()) { detected_oem = OEMType::REALME_UI; return detected_oem; }

    // OnePlus OxygenOS (lama)
    prop = getProp("ro.build.version.ota");
    if (!prop.empty() && getProp("ro.product.brand") == "OnePlus" &&
        getProp("ro.oplus.version.release").empty()) {
        detected_oem = OEMType::OXYGENOS; return detected_oem;
    }

    // ---- 4. Vivo OriginOS / FunTouchOS ----
    prop = getProp("ro.vivo.os.version");
    if (!prop.empty()) {
        std::string display = getProp("ro.vivo.os.build.display.id");
        detected_oem = (display.find("Origin") != std::string::npos)
                       ? OEMType::ORIGINOS : OEMType::FUNTOUCHOS;
        return detected_oem;
    }

    // ---- 5. Huawei EMUI / Harmony ----
    prop = getProp("ro.build.version.emui");
    if (!prop.empty()) { detected_oem = OEMType::EMUI; return detected_oem; }
    prop = getProp("hw_sc.build.platform.version");
    if (!prop.empty()) { detected_oem = OEMType::EMUI; return detected_oem; }

    // ---- 6. Honor MagicOS ----
    prop = getProp("ro.build.version.magic");
    if (!prop.empty()) { detected_oem = OEMType::MAGICOS; return detected_oem; }

    // ---- 7. Meizu Flyme ----
    std::string display_id = getProp("ro.build.display.id");
    if (display_id.find("Flyme") != std::string::npos) {
        detected_oem = OEMType::FLYME; return detected_oem;
    }

    // ---- 8. Default: AOSP / Stock ----
    detected_oem = OEMType::STOCK_AOSP;
    return detected_oem;
}

OEMType OEMDetector::get() { return detect(); }

std::string OEMDetector::getOEMName() {
    switch (get()) {
        case OEMType::MIUI_HYPEROS: return "miui/hyperos";
        case OEMType::ONEUI:       return "Samsung OneUI";
        case OEMType::COLOROS:     return "OPPO/OnePlus ColorOS";
        case OEMType::ORIGINOS:    return "Vivo OriginOS";
        case OEMType::EMUI:        return "Huawei EMUI/HarmonyOS";
        case OEMType::FLYME:       return "Meizu Flyme";
        case OEMType::MAGICOS:     return "Honor MagicOS";
        case OEMType::REALME_UI:   return "Realme UI";
        case OEMType::OXYGENOS:    return "OnePlus OxygenOS";
        case OEMType::FUNTOUCHOS:  return "Vivo FunTouchOS";
        case OEMType::STOCK_AOSP:  return "Stock Android/AOSP";
        default: return "Unknown";
    }
}

XiaomiSkin OEMDetector::getXiaomiSkin() {
    detect();  // pastikan sudah dideteksi
    return xiaomi_skin;
}

std::string OEMDetector::getSkinVersion() {
    detect();
    return skin_version_str;
}
