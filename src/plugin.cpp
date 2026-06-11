#include "plugin.h"
#include "reaper_media_parser.h"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <iomanip>

EDIT_HANDLE* g_edit_handle = nullptr;
LOG_HANDLE* g_logger = nullptr;
HINSTANCE g_dll_hinst = nullptr;

std::wstring utf8_to_wide(const std::string& utf8) {
    if (utf8.empty()) return L"";
    int len = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    if (len <= 0) return L"";
    std::wstring result(len - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &result[0], len);
    return result;
}

std::string wide_to_utf8(const std::wstring& wide) {
    if (wide.empty()) return "";
    int len = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (len <= 0) return "";
    std::string result(len - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, &result[0], len, nullptr, nullptr);
    return result;
}

std::string cp932_to_utf8(const std::string& cp932) {
    if (cp932.empty()) return "";
    int wide_len = MultiByteToWideChar(932, 0, cp932.c_str(), -1, nullptr, 0);
    if (wide_len <= 0) return cp932;
    std::wstring wide(wide_len - 1, L'\0');
    MultiByteToWideChar(932, 0, cp932.c_str(), -1, &wide[0], wide_len);
    return wide_to_utf8(wide);
}

static bool is_valid_utf8(const std::string& s) {
    size_t i = 0;
    while (i < s.size()) {
        unsigned char c = s[i];
        if (c < 0x80) { i++; continue; }
        size_t n = 0;
        if ((c & 0xE0) == 0xC0) n = 2;
        else if ((c & 0xF0) == 0xE0) n = 3;
        else if ((c & 0xF8) == 0xF0) n = 4;
        else return false;
        if (i + n > s.size()) return false;
        for (size_t j = 1; j < n; j++) {
            if ((s[i + j] & 0xC0) != 0x80) return false;
        }
        i += n;
    }
    return true;
}

std::string maybe_cp932_to_utf8(const std::string& s) {
    if (s.empty()) return s;
    if (is_valid_utf8(s)) return s;
    return cp932_to_utf8(s);
}

COMMON_PLUGIN_TABLE common_plugin_table = {
    L"UltraPaste",
    L"UltraPaste - REAPER clipboard importer for AviUtl2",
};

EXTERN_C __declspec(dllexport) COMMON_PLUGIN_TABLE* GetCommonPluginTable(void) {
    return &common_plugin_table;
}

EXTERN_C __declspec(dllexport) DWORD RequiredVersion() {
    return 2003300;
}

EXTERN_C __declspec(dllexport) void InitializeLogger(LOG_HANDLE* handle) {
    g_logger = handle;
}

EXTERN_C __declspec(dllexport) bool InitializePlugin(DWORD version) {
    return true;
}

EXTERN_C __declspec(dllexport) void UninitializePlugin() {
}

static int assign_layer_impl(double obj_fp, double bf, std::vector<double>& layer_ends, int strategy) {
    if (strategy == 0) {
        for (size_t k = 0; k < layer_ends.size(); k++) {
            if (layer_ends[k] < obj_fp) {
                layer_ends[k] = bf;
                return (int)k;
            }
        }
        layer_ends.push_back(bf);
        return (int)layer_ends.size() - 1;
    } else {
        bool all_free = true;
        for (double b : layer_ends) {
            if (b >= obj_fp) { all_free = false; break; }
        }
        if (all_free && !layer_ends.empty()) {
            layer_ends[0] = bf;
            return 0;
        } else {
            int new_layer = (int)layer_ends.size();
            layer_ends.push_back(bf);
            return new_layer;
        }
    }
}

static LPCWSTR effect_name_for_source(const std::string& source_type) {
    if (source_type == "VIDEO") return L"動画ファイル";
    if (source_type == "AUDIO") return L"音声ファイル";
    if (source_type == "IMAGE") return L"画像ファイル";
    return L"動画ファイル";
}

void generate_objects(EDIT_SECTION* edit, const ReaperProject& proj, LOG_HANDLE* logger) {
    if (!edit) return;
    if (proj.tracks.empty()) {
        if (logger) logger->log(logger, L"UltraPaste: No tracks to import");
        return;
    }

    int rate = 60;
    int scale = 1;
    int cursor_layer = 0;
    if (edit->info) {
        rate = edit->info->rate;
        scale = edit->info->scale;
        cursor_layer = edit->info->layer;
    }

    double fps = (scale > 0) ? (double)rate / (double)scale : 60.0;

    int base_layer = cursor_layer;
    int total_items = 0;
    int final_layer = base_layer;
    int final_frame = 0;

    for (size_t ti = 0; ti < proj.tracks.size(); ti++) {
        const auto& track = proj.tracks[ti];
        if (track.items.empty()) continue;

        std::vector<double> layer_ends;

        for (const auto& item : track.items) {
            if (item.file_path.empty()) continue;

            int sf = (int)std::round(item.position * fps) + 1;
            if (sf < 1) sf = 1;

            int frames = (int)std::round(item.length * fps);
            if (frames < 1) frames = 1;

            int ef = sf + frames;
            int length = ef - sf;

            int layer_offset = assign_layer_impl((double)sf, (double)ef, layer_ends, 0);
            int use_layer = base_layer + layer_offset;

            std::wstring wpath = utf8_to_wide(item.file_path);
            OBJECT_HANDLE obj = edit->create_object_from_media_file(wpath.c_str(), use_layer, sf, length);

            if (obj) {
                total_items++;
                if (use_layer > final_layer) final_layer = use_layer;
                if (ef > final_frame) final_frame = ef;

                LPCWSTR effect = effect_name_for_source(item.source_type);
                double pr = item.playrate;
                if (std::abs(pr) < 0.0001) pr = 1.0;

                double source_duration = item.length / std::abs(pr);
                double pos_start = item.soffs;
                double pos_end = item.soffs + source_duration;

                std::ostringstream pos_oss;
                pos_oss << std::fixed << std::setprecision(3)
                        << pos_start << "," << pos_end << u8",再生範囲,0";
                edit->set_object_item_value(obj, effect, L"再生位置", pos_oss.str().c_str());

                std::ostringstream speed_oss;
                speed_oss << std::fixed << std::setprecision(2) << (pr * 100.0);
                edit->set_object_item_value(obj, effect, L"再生速度", speed_oss.str().c_str());

                edit->set_object_item_value(obj, effect, L"ループ再生", item.loop ? "1" : "0");

                if (logger && logger->verbose) {
                    std::wostringstream voss;
                    voss << L"UltraPaste: L" << use_layer << L" F" << sf
                         << L" pos=" << pos_start << L"-" << pos_end
                         << L" speed=" << (pr * 100.0) << L"% loop=" << item.loop;
                    logger->verbose(logger, voss.str().c_str());
                }
            } else {
                if (logger) {
                    std::wstring msg = L"UltraPaste: Failed to create object at layer " +
                        std::to_wstring(use_layer) + L" frame " + std::to_wstring(sf) + L": " + wpath;
                    logger->log(logger, msg.c_str());
                }
            }
        }

        base_layer += (int)layer_ends.size();
    }

    if (total_items > 0 && final_frame > 0) {
        edit->set_cursor_layer_frame(final_layer, final_frame);
    }

    if (logger) {
        std::wostringstream oss;
        oss << L"UltraPaste: Imported " << total_items << L" items from "
            << proj.tracks.size() << L" tracks";
        logger->log(logger, oss.str().c_str());
    }
}

static ReaperProject g_parsed_project;

static void on_import_clipboard(EDIT_SECTION* edit) {
    UINT cf = RegisterClipboardFormatW(L"REAPERMedia");
    if (cf == 0) {
        if (g_logger) g_logger->log(g_logger, L"UltraPaste: REAPERMedia format not registered");
        return;
    }

    if (!OpenClipboard(nullptr)) {
        if (g_logger) g_logger->log(g_logger, L"UltraPaste: Failed to open clipboard");
        return;
    }

    HGLOBAL hMem = GetClipboardData(cf);
    if (!hMem) {
        CloseClipboard();
        if (g_logger) g_logger->log(g_logger, L"UltraPaste: No REAPERMedia data in clipboard");
        return;
    }

    SIZE_T size = GlobalSize(hMem);
    void* ptr = GlobalLock(hMem);
    if (!ptr || size == 0) {
        GlobalUnlock(hMem);
        CloseClipboard();
        if (g_logger) g_logger->log(g_logger, L"UltraPaste: Empty clipboard data");
        return;
    }

    std::vector<uint8_t> data((uint8_t*)ptr, (uint8_t*)ptr + size);
    GlobalUnlock(hMem);
    CloseClipboard();

    ReaperProject proj;
    if (parse_reaper_media(data, proj)) {
        g_parsed_project = proj;
        generate_objects(edit, proj, g_logger);
    } else {
        if (g_logger) {
            g_logger->log(g_logger, L"UltraPaste: Failed to parse clipboard data");
        }
    }
}

EXTERN_C __declspec(dllexport) void RegisterPlugin(HOST_APP_TABLE* host) {
    g_dll_hinst = GetModuleHandle(nullptr);

    host->register_layer_menu(L"[UltraPaste] 导入 REAPER 剪贴板", on_import_clipboard);

    g_edit_handle = host->create_edit_handle();

    if (g_logger) g_logger->log(g_logger, L"UltraPaste plugin registered");
}
