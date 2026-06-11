#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <cstdint>

#include "plugin2.h"
#include "logger2.h"

struct ReaperItem {
    double position = 0.0;
    double length = 0.0;
    double playrate = 1.0;
    double soffs = 0.0;
    bool loop = false;
    std::string file_path;
    std::string source_type;
    bool selected = false;
};

struct ReaperTrack {
    std::wstring name;
    int index = 0;
    std::vector<ReaperItem> items;
};

struct ReaperProject {
    std::vector<ReaperTrack> tracks;
    double bpm = 120.0;
    int sample_rate = 44100;
};

extern EDIT_HANDLE* g_edit_handle;
extern LOG_HANDLE* g_logger;
extern HINSTANCE g_dll_hinst;

std::wstring utf8_to_wide(const std::string& utf8);
std::string wide_to_utf8(const std::wstring& wide);
std::string cp932_to_utf8(const std::string& cp932);
std::string maybe_cp932_to_utf8(const std::string& s);

bool parse_reaper_media(const std::vector<uint8_t>& data, ReaperProject& proj);
void generate_objects(EDIT_SECTION* edit, const ReaperProject& proj, LOG_HANDLE* logger);
