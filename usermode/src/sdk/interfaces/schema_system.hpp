#pragma once

namespace schema_detail
{
    inline bool is_probable_pointer(const uintptr_t pointer)
    {
        return pointer > 0x10000 && pointer < 0x00007FFFFFFF0000;
    }

    inline bool is_printable_ascii(const std::string& text)
    {
        if (text.empty())
            return false;

        return std::ranges::all_of(text, [](const unsigned char character)
        {
            return character >= 32 && character <= 126;
        });
    }

    inline bool is_valid_module_name(const std::string& module_name)
    {
        if (module_name.empty() || module_name.size() >= 96 || !is_printable_ascii(module_name))
            return false;

        if (module_name.find(".dll") != std::string::npos)
            return true;

        return module_name.find("client") != std::string::npos ||
            module_name.find("engine") != std::string::npos ||
            module_name.find("schema") != std::string::npos;
    }
}

class c_schema_class_field_data
{
public:
    SCHEMA_ADD_STRING_OFFSET(m_name, 0x00);
    SCHEMA_ADD_OFFSET(uint16_t, m_single_inheritance_offset, 0x10);
    uint8_t m_pad0[0x20];
};
static_assert(sizeof(c_schema_class_field_data) == 0x20, "wrong size on c_schema_class_field_data");

class c_schema_type_declared_class
{
public:
    SCHEMA_ADD_STRING_OFFSET(m_binary_name, 0x08);

    std::pair<uint16_t, c_schema_class_field_data*> get_fields() const
    {
        static uint32_t count_offset = 0x1C;
        static uint32_t fields_offset = 0x28;

        auto field_count = m_memory->read_t<uint16_t>(reinterpret_cast<uintptr_t>(this) + count_offset);
        auto field_data = m_memory->read_t<c_schema_class_field_data*>(reinterpret_cast<uintptr_t>(this) + fields_offset);
        if (field_count > 0 && field_count < 4096 && schema_detail::is_probable_pointer(reinterpret_cast<uintptr_t>(field_data)))
            return { field_count, field_data };

        static constexpr std::array<std::pair<uint32_t, uint32_t>, 5> candidate_offsets = {
            std::pair{0x1C, 0x28},
            std::pair{0x20, 0x30},
            std::pair{0x18, 0x28},
            std::pair{0x24, 0x30},
            std::pair{0x20, 0x28}
        };

        for (const auto& [candidate_count_offset, candidate_fields_offset] : candidate_offsets)
        {
            field_count = m_memory->read_t<uint16_t>(reinterpret_cast<uintptr_t>(this) + candidate_count_offset);
            field_data = m_memory->read_t<c_schema_class_field_data*>(reinterpret_cast<uintptr_t>(this) + candidate_fields_offset);

            if (field_count <= 0 || field_count >= 4096 || !schema_detail::is_probable_pointer(reinterpret_cast<uintptr_t>(field_data)))
                continue;

            const auto first_field_name = field_data->m_name();
            if (first_field_name.empty())
                continue;

            count_offset = candidate_count_offset;
            fields_offset = candidate_fields_offset;
            return { field_count, field_data };
        }

        return {};
    }
};

class c_schema_system_type_scope
{
public:
    std::string m_module_name() const
    {
        static uint32_t module_name_offset = 0x08;

        auto read_module_name = [this](const uint32_t offset) -> std::string
        {
            const auto name_ptr = m_memory->read_t<uintptr_t>(reinterpret_cast<uintptr_t>(this) + offset);
            if (schema_detail::is_probable_pointer(name_ptr))
            {
                auto name = m_memory->read_t<std::string>(name_ptr);
                if (schema_detail::is_valid_module_name(name))
                    return name;
            }

            auto name = m_memory->read_t<std::string>(reinterpret_cast<uintptr_t>(this) + offset);
            if (schema_detail::is_valid_module_name(name))
                return name;

            return {};
        };

        auto module_name = read_module_name(module_name_offset);
        if (!module_name.empty())
            return module_name;

        static constexpr std::array<uint32_t, 5> module_name_candidates = { 0x08, 0x10, 0x18, 0x20, 0x28 };
        for (const auto candidate_offset : module_name_candidates)
        {
            module_name = read_module_name(candidate_offset);
            if (module_name.empty())
                continue;

            module_name_offset = candidate_offset;
            return module_name;
        }

        return {};
    }

    c_utl_ts_hash<c_schema_type_declared_class*, 256, uint32_t> m_hash_classes() const
    {
        static uint32_t hash_classes_offset = 0x540;

        auto read_hash_classes = [this](const uint32_t offset) -> std::optional<c_utl_ts_hash<c_schema_type_declared_class*, 256, uint32_t>>
        {
            const auto hash_classes = m_memory->read_t<c_utl_ts_hash<c_schema_type_declared_class*, 256, uint32_t>>(reinterpret_cast<uintptr_t>(this) + offset);
            const auto size = hash_classes.size();
            if (size > 0 && size < 65536)
                return hash_classes;

            return std::nullopt;
        };

        if (const auto hash_classes = read_hash_classes(hash_classes_offset); hash_classes.has_value())
            return hash_classes.value();

        static constexpr std::array<uint32_t, 8> hash_classes_candidates = { 0x540, 0x558, 0x570, 0x588, 0x5A0, 0x5B8, 0x5D0, 0x5E8 };
        for (const auto candidate_offset : hash_classes_candidates)
        {
            if (candidate_offset == hash_classes_offset)
                continue;

            const auto hash_classes = read_hash_classes(candidate_offset);
            if (!hash_classes.has_value())
                continue;

            hash_classes_offset = candidate_offset;
            return hash_classes.value();
        }

        return {};
    }
};

class c_schema_system
{
public:
    std::vector<c_schema_system_type_scope*> get_type_scopes() const
    {
        static uint32_t type_scope_size_offset = 0x190;
        static uint32_t type_scope_data_offset = 0x198;

        auto read_type_scopes = [this](const uint32_t size_offset, const uint32_t data_offset, const bool require_named_scope) -> std::vector<c_schema_system_type_scope*>
        {
            const auto size = m_memory->read_t<uint32_t>(reinterpret_cast<uintptr_t>(this) + size_offset);
            if (!size || size > 1024)
                return {};

            const auto data = m_memory->read_t<uintptr_t>(reinterpret_cast<uintptr_t>(this) + data_offset);
            if (!schema_detail::is_probable_pointer(data))
                return {};

            std::vector<c_schema_system_type_scope*> type_scopes(size);
            m_memory->read_t(data, type_scopes.data(), size * sizeof(uintptr_t));

            std::vector<c_schema_system_type_scope*> valid_type_scopes;
            valid_type_scopes.reserve(type_scopes.size());

            for (const auto& type_scope : type_scopes)
            {
                if (schema_detail::is_probable_pointer(reinterpret_cast<uintptr_t>(type_scope)))
                    valid_type_scopes.push_back(type_scope);
            }

            if (valid_type_scopes.empty())
                return {};

            if (!require_named_scope)
                return valid_type_scopes;

            const auto max_to_check = std::min<size_t>(valid_type_scopes.size(), 12);
            for (size_t idx = 0; idx < max_to_check; ++idx)
            {
                if (!valid_type_scopes[idx]->m_module_name().empty())
                    return valid_type_scopes;
            }

            return {};
        };

        if (const auto scopes = read_type_scopes(type_scope_size_offset, type_scope_data_offset, true); !scopes.empty())
            return scopes;

        static constexpr std::array<std::pair<uint32_t, uint32_t>, 5> known_layouts = {
            std::pair{0x190, 0x198},
            std::pair{0x188, 0x190},
            std::pair{0x1A0, 0x1A8},
            std::pair{0x180, 0x188},
            std::pair{0x198, 0x1A0}
        };

        for (const auto& [candidate_size_offset, candidate_data_offset] : known_layouts)
        {
            if (candidate_size_offset == type_scope_size_offset && candidate_data_offset == type_scope_data_offset)
                continue;

            const auto scopes = read_type_scopes(candidate_size_offset, candidate_data_offset, true);
            if (scopes.empty())
                continue;

            type_scope_size_offset = candidate_size_offset;
            type_scope_data_offset = candidate_data_offset;
            LOG_INFO("resolved dynamic type scope layout offsets ('0x%X', '0x%X')", type_scope_size_offset, type_scope_data_offset);
            return scopes;
        }

        for (uint32_t offset = 0x150; offset <= 0x260; offset += 0x8)
        {
            const auto scopes = read_type_scopes(offset, offset + 0x8, true);
            if (scopes.empty())
                continue;

            type_scope_size_offset = offset;
            type_scope_data_offset = offset + 0x8;
            LOG_INFO("resolved scanned type scope layout offsets ('0x%X', '0x%X')", type_scope_size_offset, type_scope_data_offset);
            return scopes;
        }

        LOG_WARNING("type scope size is either empty or not good");
        return {};
    }

    class c_schema_system_type_scope* find_type_scope_for_module(const std::string_view& name) const
    {
        const auto type_scopes = get_type_scopes();
        if (type_scopes.empty())
            return {};

        for (const auto& type_scope : type_scopes)
        {
            const auto module_name = type_scope->m_module_name();
            if (module_name.empty() || module_name.find(name) == std::string::npos)
                continue;

            return type_scope;
        }

        return {};
    }
};
