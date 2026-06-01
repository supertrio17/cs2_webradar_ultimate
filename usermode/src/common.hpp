#pragma once

/* current build of cs2_webradar */
#define CS2_WEBRADAR_VERSION "v1.3.1"

/* game modules */
#define CLIENT_DLL "client.dll"
#define ENGINE2_DLL "engine2.dll"
#define SCHEMASYSTEM_DLL "schemasystem.dll"

/* game signatures */
#define GET_SCHEMA_SYSTEM "48 89 05 ? ? ? ? 4C 8D 0D ? ? ? ? 33 C0 48 C7 05 ? ? ? ? ? ? ? ? 89 05"
#define GET_ENTITY_LIST "48 8B 1D ? ? ? ? 48 89 1D ? ? ? ? 4C 63 B3"
#define GET_GLOBAL_VARS "48 89 15 ? ? ? ? 48 89 42"
#define GET_LOCAL_PLAYER_CONTROLLER "48 8B 05 ? ? ? ? 41 89 BE"

/* cs2-dumper offsets from dump folder (2026-05-29) */
namespace offsets
{
    inline constexpr uintptr_t m_dw_entity_list = 0x24E5590;
    inline constexpr uintptr_t m_dw_global_vars = 0x205F6D0;
    inline constexpr uintptr_t m_dw_local_player_controller = 0x231E700;
    inline constexpr uintptr_t m_schema_system = 0x76800;
}

/* custom defines */
#define LOG_INFO(str, ...) \
    { \
        FILE* log = fopen("WR_Log.txt", "a+"); \
        if (log) { \
            fprintf(log, "[INFO] " str "\n" __VA_OPT__(,) __VA_ARGS__); \
            fclose(log); \
        } \
    }

#define LOG_DEBUG(str, ...) \
    { \
        FILE* log = fopen("WR_Log.txt", "a+"); \
        if (log) { \
            fprintf(log, "[DEBUG] " str "\n" __VA_OPT__(,) __VA_ARGS__); \
            fclose(log); \
        } \
    }

#define LOG_WARNING(str, ...) \
    { \
        FILE* log = fopen("WR_Log.txt", "a+"); \
        if (log) { \
            fprintf(log, "[WARNING] " str "\n" __VA_OPT__(,) __VA_ARGS__); \
            fclose(log); \
        } \
    }

#define LOG_ERROR(str, ...) \
    { \
        const auto filename = std::filesystem::path(__FILE__).filename().string(); \
        FILE* log = fopen("WR_Log.txt", "a+"); \
        if (log) { \
            fprintf(log, "[ERROR] [%s:%d] " str "\n", filename.c_str(), __LINE__ __VA_OPT__(,) __VA_ARGS__); \
            fclose(log); \
        } \
    }

#define LOG_CLEAR() \
    { \
        FILE* log = fopen("WR_Log.txt", "w+"); \
        if (log) { \
            fclose(log); \
        } \
    }
    