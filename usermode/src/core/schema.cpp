#include "pch.hpp"

struct schema_data_t
{
    fnv1a_t m_hashed_field_name = 0;
    uint32_t m_offset = 0;
};

static std::unordered_map<fnv1a_t, uint32_t> m_schema_data = {};
static constexpr std::array<schema_data_t, 41> m_schema_fallback_data = {
    schema_data_t{ fnv1a::hash_const("CBasePlayerController->m_hPawn"), 0x6BC },
    schema_data_t{ fnv1a::hash_const("CBasePlayerController->m_steamID"), 0x780 },
    schema_data_t{ fnv1a::hash_const("CCSPlayerController->m_iCompTeammateColor"), 0x848 },
    schema_data_t{ fnv1a::hash_const("CCSPlayerController->m_pInGameMoneyServices"), 0x808 },
    schema_data_t{ fnv1a::hash_const("CCSPlayerController->m_sSanitizedPlayerName"), 0x860 },
    schema_data_t{ fnv1a::hash_const("CCSPlayerController_InGameMoneyServices->m_iAccount"), 0x40 },
    schema_data_t{ fnv1a::hash_const("CCSPlayer_ItemServices->m_bHasDefuser"), 0x48 },
    schema_data_t{ fnv1a::hash_const("CCSPlayer_ItemServices->m_bHasHelmet"), 0x49 },
    schema_data_t{ fnv1a::hash_const("CCSWeaponBaseVData->m_WeaponType"), 0x520 },
    schema_data_t{ fnv1a::hash_const("CCSWeaponBaseVData->m_szName"), 0x720 },
    schema_data_t{ fnv1a::hash_const("CEntityIdentity->m_designerName"), 0x20 },
    schema_data_t{ fnv1a::hash_const("CEntityIdentity->m_flags"), 0x30 },
    schema_data_t{ fnv1a::hash_const("CEntityInstance->m_pEntity"), 0x10 },
    schema_data_t{ fnv1a::hash_const("CGameSceneNode->m_vecAbsOrigin"), 0xC8 },
    schema_data_t{ fnv1a::hash_const("CGameSceneNode->m_vecOrigin"), 0x80 },
    schema_data_t{ fnv1a::hash_const("CModelState->m_ModelName"), 0xA8 },
    schema_data_t{ fnv1a::hash_const("CPlayer_WeaponServices->m_hActiveWeapon"), 0x60 },
    schema_data_t{ fnv1a::hash_const("CPlayer_WeaponServices->m_hMyWeapons"), 0x48 },
    schema_data_t{ fnv1a::hash_const("CSkeletonInstance->m_modelState"), 0x150 },
    schema_data_t{ fnv1a::hash_const("C_BaseEntity->m_hOwnerEntity"), 0x520 },
    schema_data_t{ fnv1a::hash_const("C_BaseEntity->m_iHealth"), 0x34C },
    schema_data_t{ fnv1a::hash_const("C_BaseEntity->m_iTeamNum"), 0x3EB },
    schema_data_t{ fnv1a::hash_const("C_BaseEntity->m_nSubclassID"), 0x380 },
    schema_data_t{ fnv1a::hash_const("C_BaseEntity->m_pGameSceneNode"), 0x330 },
    schema_data_t{ fnv1a::hash_const("C_BasePlayerPawn->m_pItemServices"), 0x11E8 },
    schema_data_t{ fnv1a::hash_const("C_BasePlayerPawn->m_pWeaponServices"), 0x11E0 },
    schema_data_t{ fnv1a::hash_const("C_CSPlayerPawn->m_ArmorValue"), 0x1C7C },
    schema_data_t{ fnv1a::hash_const("C_CSPlayerPawn->m_angEyeAngles"), 0x3320 },
    schema_data_t{ fnv1a::hash_const("C_CSPlayerPawn->m_bIsScoped"), 0x1C50 },
    schema_data_t{ fnv1a::hash_const("C_CSPlayerPawnBase->m_flFlashOverlayAlpha"), 0x13F4 },
    schema_data_t{ fnv1a::hash_const("C_Inferno->m_bFireIsBurning"), 0x1618 },
    schema_data_t{ fnv1a::hash_const("C_Inferno->m_fireCount"), 0x1958 },
    schema_data_t{ fnv1a::hash_const("C_Inferno->m_firePositions"), 0x1018 },
    schema_data_t{ fnv1a::hash_const("C_Inferno->m_nFireEffectTickBegin"), 0x196C },
    schema_data_t{ fnv1a::hash_const("C_PlantedC4->m_bBeingDefused"), 0x119C },
    schema_data_t{ fnv1a::hash_const("C_PlantedC4->m_bBombDefused"), 0x11B4 },
    schema_data_t{ fnv1a::hash_const("C_PlantedC4->m_bBombTicking"), 0x1160 },
    schema_data_t{ fnv1a::hash_const("C_PlantedC4->m_flC4Blow"), 0x1190 },
    schema_data_t{ fnv1a::hash_const("C_PlantedC4->m_flDefuseCountDown"), 0x11B0 },
    schema_data_t{ fnv1a::hash_const("C_SmokeGrenadeProjectile->m_nSmokeEffectTickBegin"), 0x1250 },
    schema_data_t{ fnv1a::hash_const("C_SmokeGrenadeProjectile->m_vSmokeDetonationPos"), 0x1268 },
};

namespace
{
    bool add_schema_entry(const fnv1a_t hash, const uint32_t offset, const bool overwrite = false)
    {
        if (!offset)
            return false;

        const auto it = m_schema_data.find(hash);
        if (it != m_schema_data.end())
        {
            if (overwrite)
                it->second = offset;

            return false;
        }

        m_schema_data.emplace(hash, offset);
        return true;
    }

    bool add_schema_entry(const schema_data_t& data, const bool overwrite = false)
    {
        return add_schema_entry(data.m_hashed_field_name, data.m_offset, overwrite);
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

    std::vector<std::filesystem::path> get_possible_dump_directories()
    {
        std::vector<std::filesystem::path> dump_directories = {};
        std::set<std::filesystem::path> unique_dump_directories = {};

        auto add_dump_dir = [&](const std::filesystem::path& base_path)
        {
            if (base_path.empty())
                return;

            for (uint32_t idx = 0; idx < 4; ++idx)
            {
                auto current = base_path;
                for (uint32_t up = 0; up < idx; ++up)
                    current = current.parent_path();

                if (current.empty())
                    continue;

                const auto dump_path = current / "dump";
                if (!unique_dump_directories.emplace(dump_path).second)
                    continue;

                dump_directories.emplace_back(dump_path);
            }
        };

        add_dump_dir(std::filesystem::current_path());

        if (const auto executable_directory = get_executable_directory(); executable_directory.has_value())
            add_dump_dir(executable_directory.value());

        return dump_directories;
    }

    std::optional<std::filesystem::path> find_dump_file(const std::string_view& file_name)
    {
        const auto dump_directories = get_possible_dump_directories();
        for (const auto& dump_directory : dump_directories)
        {
            const auto file_path = dump_directory / file_name;
            if (std::filesystem::exists(file_path))
                return file_path;
        }

        return {};
    }

    std::filesystem::path get_runtime_cache_path()
    {
        if (const auto client_dump = find_dump_file("client_dll.json"); client_dump.has_value())
            return client_dump.value().parent_path() / "runtime_schema_cache.json";

        return std::filesystem::current_path() / "runtime_schema_cache.json";
    }

    bool read_json_file(const std::filesystem::path& path, nlohmann::json& output)
    {
        if (!std::filesystem::exists(path))
            return false;

        std::ifstream file(path);
        if (!file.is_open())
            return false;

        try
        {
            output = nlohmann::json::parse(file);
        }
        catch (...)
        {
            return false;
        }

        return true;
    }

    uint32_t load_offsets_from_dump_file(const std::filesystem::path& file_path, const std::string_view& module_name)
    {
        nlohmann::json json_dump = {};
        if (!read_json_file(file_path, json_dump))
            return 0;

        const std::string module_key(module_name);
        if (!json_dump.contains(module_key))
            return 0;

        const auto& module = json_dump[module_key];
        if (!module.contains("classes") || !module["classes"].is_object())
            return 0;

        uint32_t loaded = 0;

        for (const auto& [class_name, class_data] : module["classes"].items())
        {
            if (!class_data.contains("fields") || !class_data["fields"].is_object())
                continue;

            for (const auto& [field_name, offset_data] : class_data["fields"].items())
            {
                if (!offset_data.is_number_integer() && !offset_data.is_number_unsigned())
                    continue;

                const auto field_offset = static_cast<uint32_t>(offset_data.get<uint64_t>());
                if (!field_offset)
                    continue;

                const auto field_path = std::format("{}->{}", class_name, field_name);
                if (add_schema_entry(fnv1a::hash(field_path), field_offset))
                    loaded++;
            }
        }

        return loaded;
    }

    uint32_t load_offsets_from_runtime_cache()
    {
        const auto runtime_cache_path = get_runtime_cache_path();

        nlohmann::json json_cache = {};
        if (!read_json_file(runtime_cache_path, json_cache))
            return 0;

        if (!json_cache.contains("entries") || !json_cache["entries"].is_array())
            return 0;

        uint32_t loaded = 0;

        for (const auto& entry : json_cache["entries"])
        {
            if (!entry.contains("hash") || !entry.contains("offset"))
                continue;

            if ((!entry["hash"].is_number_integer() && !entry["hash"].is_number_unsigned()) ||
                (!entry["offset"].is_number_integer() && !entry["offset"].is_number_unsigned()))
                continue;

            const auto hash = static_cast<fnv1a_t>(entry["hash"].get<uint64_t>());
            const auto offset = static_cast<uint32_t>(entry["offset"].get<uint64_t>());
            if (add_schema_entry(hash, offset))
                loaded++;
        }

        return loaded;
    }

    void save_runtime_cache(const std::vector<schema_data_t>& entries)
    {
        if (entries.empty())
            return;

        const auto runtime_cache_path = get_runtime_cache_path();
        const auto cache_directory = runtime_cache_path.parent_path();
        if (!cache_directory.empty())
            std::filesystem::create_directories(cache_directory);

        nlohmann::json json_cache = {};
        json_cache["entry_count"] = entries.size();
        json_cache["entries"] = nlohmann::json::array();

        for (const auto& entry : entries)
        {
            nlohmann::json json_entry = {};
            json_entry["hash"] = entry.m_hashed_field_name;
            json_entry["offset"] = entry.m_offset;
            json_cache["entries"].push_back(std::move(json_entry));
        }

        std::ofstream cache_file(runtime_cache_path, std::ios::trunc);
        if (!cache_file.is_open())
            return;

        cache_file << json_cache.dump(2);
        cache_file.close();

        const auto cache_path_string = runtime_cache_path.string();
        LOG_INFO("saved '%d' runtime schema cache entries ('%s')", static_cast<int32_t>(entries.size()), cache_path_string.c_str());
    }

    uint32_t load_offsets_from_dump_files()
    {
        uint32_t loaded = 0;

        if (const auto client_dump = find_dump_file("client_dll.json"); client_dump.has_value())
            loaded += load_offsets_from_dump_file(client_dump.value(), CLIENT_DLL);

        if (const auto engine_dump = find_dump_file("engine2_dll.json"); engine_dump.has_value())
            loaded += load_offsets_from_dump_file(engine_dump.value(), ENGINE2_DLL);

        return loaded;
    }
}

bool schema::setup()
{
    LOG_DEBUG("schema::setup started");
    m_schema_data.clear();

    std::vector<schema_data_t> runtime_entries = {};
    uint32_t found_classes = 0;
    uint32_t processed_scopes = 0;
    uint32_t skipped_null_scopes = 0;
    uint32_t skipped_unnamed_scopes = 0;
    uint32_t skipped_non_target_scopes = 0;
    uint32_t skipped_empty_hash_tables = 0;

    if (i::m_schema_system)
    {
        const auto type_scopes = i::m_schema_system->get_type_scopes(true, true);
        LOG_DEBUG("schema::setup discovered '%u' schema type scopes", static_cast<uint32_t>(type_scopes.size()));

        for (const auto& type_scope : type_scopes)
        {
            if (!type_scope)
            {
                skipped_null_scopes++;
                continue;
            }

            const auto module_name = type_scope->m_module_name();
            if (module_name.empty())
            {
                skipped_unnamed_scopes++;
                continue;
            }

            if (module_name.find(CLIENT_DLL) == std::string::npos && module_name.find(ENGINE2_DLL) == std::string::npos)
            {
                skipped_non_target_scopes++;
                continue;
            }

            processed_scopes++;

            auto hash_classes = type_scope->m_hash_classes();
            const auto table_size = hash_classes.size();
            if (!table_size)
            {
                skipped_empty_hash_tables++;
                LOG_DEBUG("schema::setup skipped scope '%s' because class hash table is empty", module_name.c_str());
                continue;
            }

            LOG_DEBUG("schema::setup reading scope '%s' with hash table size '%u'", module_name.c_str(), table_size);

            uint32_t scope_classes = 0;
            uint32_t scope_added_fields = 0;

            std::unique_ptr<uintptr_t[]> elements = std::make_unique_for_overwrite<uintptr_t[]>(table_size);
            const auto elements_size = hash_classes.get_elements(0, table_size, elements.get());
            for (uint32_t idx = 0; idx < elements_size; idx++)
            {
                const auto element = elements[idx];
                if (!element)
                    continue;

                const auto class_binding = hash_classes[element];
                if (!class_binding)
                    continue;

                const auto class_name = class_binding->m_binary_name();
                if (class_name.empty())
                    continue;

                found_classes++;
                scope_classes++;

                auto [schema_field_size, schema_field] = class_binding->get_fields();
                for (uint32_t field_idx = 0; field_idx < schema_field_size; field_idx++)
                {
                    if (!schema_field)
                        break;

                    const auto field_name = schema_field->m_name();
                    const auto field_offset = schema_field->m_single_inheritance_offset();

                    if (!field_name.empty() && field_offset)
                    {
                        const auto field_path = std::format("{}->{}", class_name, field_name);
                        const schema_data_t runtime_data = { fnv1a::hash(field_path), field_offset };

                        if (add_schema_entry(runtime_data, true))
                        {
                            runtime_entries.emplace_back(runtime_data);
                            scope_added_fields++;
                        }
                    }

                    schema_field = reinterpret_cast<c_schema_class_field_data*>(reinterpret_cast<uintptr_t>(schema_field) + sizeof(c_schema_class_field_data));
                }
            }

            LOG_DEBUG("schema::setup completed scope '%s' (classes='%u', added_fields='%u')", module_name.c_str(), scope_classes, scope_added_fields);
        }
    }
    else
    {
        LOG_WARNING("schema::setup cannot parse runtime schema because schema system interface is null");
    }

    LOG_DEBUG("schema::setup runtime scope summary processed='%u' null='%u' unnamed='%u' non_target='%u' empty_hash='%u'",
        processed_scopes,
        skipped_null_scopes,
        skipped_unnamed_scopes,
        skipped_non_target_scopes,
        skipped_empty_hash_tables);

    if (!runtime_entries.empty())
        save_runtime_cache(runtime_entries);

    const auto cached_offsets_loaded = load_offsets_from_runtime_cache();
    const auto dump_offsets_loaded = load_offsets_from_dump_files();

    if (cached_offsets_loaded)
        LOG_INFO("loaded '%d' schema offsets from runtime cache", cached_offsets_loaded);
    else
        LOG_DEBUG("schema::setup runtime cache did not provide offsets");

    if (dump_offsets_loaded)
        LOG_INFO("loaded '%d' schema offsets from dump files", dump_offsets_loaded);
    else
        LOG_DEBUG("schema::setup dump files did not provide additional offsets");

    uint32_t fallback_added = 0;
    for (const auto& fallback_entry : m_schema_fallback_data)
    {
        if (add_schema_entry(fallback_entry))
            fallback_added++;
    }

    if (!found_classes)
        LOG_WARNING("failed to load runtime schema classes, using cached and fallback offsets");

    LOG_INFO("loaded '%d' schema classes and '%d' schema offsets ('%d' runtime, '%d' cached, '%d' dumped, '%d' fallbacks)",
        found_classes,
        static_cast<int32_t>(m_schema_data.size()),
        static_cast<int32_t>(runtime_entries.size()),
        cached_offsets_loaded,
        dump_offsets_loaded,
        fallback_added);

    return !m_schema_data.empty();
}

uint32_t schema::get_offset(const fnv1a_t hashed_field_name)
{
    const auto it = m_schema_data.find(hashed_field_name);
    if (it != m_schema_data.end())
        return it->second;

    LOG_ERROR("failed to find an offset for the field with the hash value '%llu'", static_cast<unsigned long long>(hashed_field_name));
    return {};
}
