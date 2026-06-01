#include "pch.hpp"

namespace
{
    bool is_probable_pointer(const uintptr_t pointer)
    {
        return pointer > 0x10000 && pointer < 0x00007FFFFFFF0000;
    }

    struct dynamic_offsets_t
    {
        uintptr_t m_entity_list = offsets::m_dw_entity_list;
        uintptr_t m_global_vars = offsets::m_dw_global_vars;
        uintptr_t m_local_player_controller = offsets::m_dw_local_player_controller;
        uintptr_t m_schema_system = offsets::m_schema_system;
        bool m_initialized = false;
    };

    dynamic_offsets_t& dynamic_offsets()
    {
        static dynamic_offsets_t offsets = {};
        return offsets;
    }

    std::optional<std::filesystem::path> get_executable_directory()
    {
        std::array<char, MAX_PATH> path_buffer = {};
        const auto path_size = GetModuleFileNameA(nullptr, path_buffer.data(), static_cast<DWORD>(path_buffer.size()));
        if (!path_size || path_size == path_buffer.size())
            return {};

        std::filesystem::path executable_path(path_buffer.data());
        return executable_path.parent_path();
    }

    std::optional<std::filesystem::path> find_offsets_dump_file()
    {
        std::vector<std::filesystem::path> search_roots = {};
        search_roots.emplace_back(std::filesystem::current_path());

        if (const auto executable_directory = get_executable_directory(); executable_directory.has_value())
            search_roots.emplace_back(executable_directory.value());

        for (const auto& root : search_roots)
        {
            auto current = root;
            for (uint32_t idx = 0; idx < 4; ++idx)
            {
                const auto offsets_path = current / "dump" / "offsets.json";
                if (std::filesystem::exists(offsets_path))
                    return offsets_path;

                if (!current.has_parent_path())
                    break;

                current = current.parent_path();
            }
        }

        return {};
    }

    void load_dynamic_offsets_from_dump()
    {
        auto& runtime_offsets = dynamic_offsets();
        if (runtime_offsets.m_initialized)
            return;

        runtime_offsets.m_initialized = true;
        LOG_DEBUG("attempting to load runtime offsets from offsets.json");

        const auto dump_file = find_offsets_dump_file();
        if (!dump_file.has_value())
        {
            LOG_DEBUG("offsets.json was not found, using built-in offsets");
            return;
        }

        std::ifstream file(dump_file.value());
        if (!file.is_open())
        {
            const auto dump_path = dump_file.value().string();
            LOG_WARNING("failed to open offsets dump file ('%s')", dump_path.c_str());
            return;
        }

        nlohmann::json json_offsets = {};
        try
        {
            json_offsets = nlohmann::json::parse(file);
        }
        catch (...)
        {
            const auto dump_path = dump_file.value().string();
            LOG_WARNING("failed to parse offsets dump file ('%s')", dump_path.c_str());
            return;
        }

        auto parse_offset = [](const nlohmann::json& module, const std::string_view& name, uintptr_t& output)
        {
            const std::string key(name);
            if (!module.contains(key))
                return;

            const auto& value = module[key];
            if (!value.is_number_integer() && !value.is_number_unsigned())
                return;

            output = static_cast<uintptr_t>(value.get<uint64_t>());
        };

        if (json_offsets.contains(CLIENT_DLL))
        {
            const auto& client = json_offsets[CLIENT_DLL];
            parse_offset(client, "dwEntityList", runtime_offsets.m_entity_list);
            parse_offset(client, "dwGameEntitySystem", runtime_offsets.m_entity_list);
            parse_offset(client, "dwGlobalVars", runtime_offsets.m_global_vars);
            parse_offset(client, "dwLocalPlayerController", runtime_offsets.m_local_player_controller);
        }

        if (json_offsets.contains(SCHEMASYSTEM_DLL))
        {
            const auto& schema_system = json_offsets[SCHEMASYSTEM_DLL];
            parse_offset(schema_system, "dwSchemaSystem", runtime_offsets.m_schema_system);
            parse_offset(schema_system, "m_schema_system", runtime_offsets.m_schema_system);
        }

        const auto dump_path_string = dump_file.value().string();
        LOG_INFO("loaded dynamic offsets from dump ('%s')", dump_path_string.c_str());
        LOG_DEBUG("runtime offsets entity='0x%llX' global='0x%llX' local_controller='0x%llX' schema='0x%llX'",
            static_cast<unsigned long long>(runtime_offsets.m_entity_list),
            static_cast<unsigned long long>(runtime_offsets.m_global_vars),
            static_cast<unsigned long long>(runtime_offsets.m_local_player_controller),
            static_cast<unsigned long long>(runtime_offsets.m_schema_system));
    }
}

uintptr_t i::get_entity_list_offset()
{
    load_dynamic_offsets_from_dump();
    return dynamic_offsets().m_entity_list;
}

uintptr_t i::get_global_vars_offset()
{
    load_dynamic_offsets_from_dump();
    return dynamic_offsets().m_global_vars;
}

uintptr_t i::get_local_player_controller_offset()
{
    load_dynamic_offsets_from_dump();
    return dynamic_offsets().m_local_player_controller;
}

uintptr_t i::get_schema_system_offset()
{
    load_dynamic_offsets_from_dump();
    return dynamic_offsets().m_schema_system;
}

bool i::refresh_global_vars()
{
    LOG_DEBUG("refresh_global_vars started");

    const auto [client_base, client_size] = m_memory->get_module_info(CLIENT_DLL);
    if (!client_base.has_value() || !client_size.has_value())
    {
        LOG_WARNING("refresh_global_vars failed: client.dll module not found");
        return false;
    }

    m_global_vars = nullptr;

    const auto global_vars_pattern = m_memory->find_pattern(CLIENT_DLL, GET_GLOBAL_VARS);
    if (global_vars_pattern.has_value())
    {
        const auto global_vars_ptr = m_memory->read_t<uintptr_t>(global_vars_pattern->rip().as<uintptr_t>());
        if (is_probable_pointer(global_vars_ptr))
        {
            m_global_vars = reinterpret_cast<c_global_vars*>(global_vars_ptr);
            LOG_DEBUG("refresh_global_vars resolved via signature at '0x%llX'", static_cast<unsigned long long>(global_vars_ptr));
        }
    }

    if (!m_global_vars)
    {
        const auto fallback_offset = get_global_vars_offset();
        const auto global_vars_ptr = m_memory->read_t<uintptr_t>(client_base.value() + fallback_offset);
        if (is_probable_pointer(global_vars_ptr))
        {
            m_global_vars = reinterpret_cast<c_global_vars*>(global_vars_ptr);
            LOG_DEBUG("refresh_global_vars resolved via fallback offset '0x%llX' to ptr '0x%llX'", static_cast<unsigned long long>(fallback_offset), static_cast<unsigned long long>(global_vars_ptr));
        }
    }

    if (!m_global_vars)
        LOG_WARNING("refresh_global_vars failed: unable to resolve global vars pointer");

    return m_global_vars != nullptr;
}

bool i::setup()
{
    LOG_DEBUG("interfaces::setup started");

    bool success = true;

    const auto [client_base, client_size] = m_memory->get_module_info(CLIENT_DLL);
    if (!client_base.has_value() || !client_size.has_value())
    {
        LOG_DEBUG("interfaces::setup waiting for client.dll module");
        return false;
    }

    const auto [schema_system_base, schema_system_size] = m_memory->get_module_info(SCHEMASYSTEM_DLL);
    if (!schema_system_base.has_value() || !schema_system_size.has_value())
    {
        LOG_DEBUG("interfaces::setup waiting for schemasystem.dll module");
        return false;
    }

    LOG_DEBUG("interfaces::setup module bases client='0x%llX' schema='0x%llX'", static_cast<unsigned long long>(client_base.value()), static_cast<unsigned long long>(schema_system_base.value()));

    m_schema_system = nullptr;

    const auto schema_pattern = m_memory->find_pattern(SCHEMASYSTEM_DLL, GET_SCHEMA_SYSTEM);
    if (schema_pattern.has_value())
    {
        const auto schema_ptr = m_memory->read_t<uintptr_t>(schema_pattern->rip().as<uintptr_t>());
        if (is_probable_pointer(schema_ptr))
        {
            m_schema_system = reinterpret_cast<c_schema_system*>(schema_ptr);
            LOG_DEBUG("interfaces::setup resolved schema system via signature at '0x%llX'", static_cast<unsigned long long>(schema_ptr));
        }
    }

    if (!m_schema_system)
    {
        const auto schema_offset = get_schema_system_offset();
        const auto schema_ptr = m_memory->read_t<uintptr_t>(schema_system_base.value() + schema_offset);
        if (is_probable_pointer(schema_ptr))
        {
            m_schema_system = reinterpret_cast<c_schema_system*>(schema_ptr);
            LOG_DEBUG("interfaces::setup resolved schema system via fallback offset '0x%llX' -> '0x%llX'", static_cast<unsigned long long>(schema_offset), static_cast<unsigned long long>(schema_ptr));
        }
    }

    if (m_schema_system)
    {
        const auto type_scopes = m_schema_system->get_type_scopes(true, false);
        if (type_scopes.empty())
        {
            LOG_DEBUG("interfaces::setup schema system pointer was valid but type scopes are not ready yet");
            m_schema_system = nullptr;
        }
        else
        {
            LOG_DEBUG("interfaces::setup schema system type scopes resolved ('%u' scopes)", static_cast<uint32_t>(type_scopes.size()));
        }
    }
    else
    {
        LOG_DEBUG("interfaces::setup failed to resolve schema system pointer");
    }

    success &= (m_schema_system != nullptr);
    success &= refresh_global_vars();

    m_game_entity_system = nullptr;

    const auto entity_list_pattern = m_memory->find_pattern(CLIENT_DLL, GET_ENTITY_LIST);
    if (entity_list_pattern.has_value())
    {
        const auto entity_system_ptr = m_memory->read_t<uintptr_t>(entity_list_pattern->rip().as<uintptr_t>());
        if (is_probable_pointer(entity_system_ptr))
        {
            m_game_entity_system = reinterpret_cast<c_game_entity_system*>(entity_system_ptr);
            LOG_DEBUG("interfaces::setup resolved game entity system via signature at '0x%llX'", static_cast<unsigned long long>(entity_system_ptr));
        }
    }

    if (!m_game_entity_system)
    {
        const auto entity_list_offset = get_entity_list_offset();
        const auto entity_system_ptr = m_memory->read_t<uintptr_t>(client_base.value() + entity_list_offset);
        if (is_probable_pointer(entity_system_ptr))
        {
            m_game_entity_system = reinterpret_cast<c_game_entity_system*>(entity_system_ptr);
            LOG_DEBUG("interfaces::setup resolved game entity system via fallback offset '0x%llX' -> '0x%llX'", static_cast<unsigned long long>(entity_list_offset), static_cast<unsigned long long>(entity_system_ptr));
        }
    }

    success &= (m_game_entity_system != nullptr);
    LOG_DEBUG("interfaces::setup completed (success='%u', schema='%u', global='%u', entity='%u')",
        success ? 1 : 0,
        m_schema_system ? 1 : 0,
        m_global_vars ? 1 : 0,
        m_game_entity_system ? 1 : 0);

    return success;
}
