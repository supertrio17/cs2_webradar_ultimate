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

		const auto dump_file = find_offsets_dump_file();
		if (!dump_file.has_value())
			return;

		std::ifstream file(dump_file.value());
		if (!file.is_open())
			return;

		nlohmann::json json_offsets = {};
		try
		{
			json_offsets = nlohmann::json::parse(file);
		}
		catch (...)
		{
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
	const auto [client_base, client_size] = m_memory->get_module_info(CLIENT_DLL);
	if (!client_base.has_value() || !client_size.has_value())
		return false;

	m_global_vars = nullptr;

	const auto global_vars_pattern = m_memory->find_pattern(CLIENT_DLL, GET_GLOBAL_VARS);
	if (global_vars_pattern.has_value())
	{
		const auto global_vars_ptr = m_memory->read_t<uintptr_t>(global_vars_pattern->rip().as<uintptr_t>());
		if (is_probable_pointer(global_vars_ptr))
			m_global_vars = reinterpret_cast<c_global_vars*>(global_vars_ptr);
	}

	if (!m_global_vars)
	{
		const auto global_vars_ptr = m_memory->read_t<uintptr_t>(client_base.value() + get_global_vars_offset());
		if (is_probable_pointer(global_vars_ptr))
			m_global_vars = reinterpret_cast<c_global_vars*>(global_vars_ptr);
	}

	return m_global_vars != nullptr;
}

bool i::setup()
{
	bool success = true;

	const auto [client_base, client_size] = m_memory->get_module_info(CLIENT_DLL);
	if (!client_base.has_value() || !client_size.has_value())
		return false;

	const auto [schema_system_base, schema_system_size] = m_memory->get_module_info(SCHEMASYSTEM_DLL);
	if (!schema_system_base.has_value() || !schema_system_size.has_value())
		return false;

	m_schema_system = nullptr;

	const auto schema_pattern = m_memory->find_pattern(SCHEMASYSTEM_DLL, GET_SCHEMA_SYSTEM);
	if (schema_pattern.has_value())
	{
		const auto schema_ptr = m_memory->read_t<uintptr_t>(schema_pattern->rip().as<uintptr_t>());
		if (is_probable_pointer(schema_ptr))
			m_schema_system = reinterpret_cast<c_schema_system*>(schema_ptr);
	}

	if (!m_schema_system)
	{
		const auto schema_ptr = m_memory->read_t<uintptr_t>(schema_system_base.value() + get_schema_system_offset());
		if (is_probable_pointer(schema_ptr))
			m_schema_system = reinterpret_cast<c_schema_system*>(schema_ptr);
	}

	if (m_schema_system)
	{
		if (m_schema_system->get_type_scopes().empty())
			m_schema_system = nullptr;
	}

	success &= (m_schema_system != nullptr);
	success &= refresh_global_vars();

	m_game_entity_system = nullptr;

	const auto entity_list_pattern = m_memory->find_pattern(CLIENT_DLL, GET_ENTITY_LIST);
	if (entity_list_pattern.has_value())
	{
		const auto entity_system_ptr = m_memory->read_t<uintptr_t>(entity_list_pattern->rip().as<uintptr_t>());
		if (is_probable_pointer(entity_system_ptr))
			m_game_entity_system = reinterpret_cast<c_game_entity_system*>(entity_system_ptr);
	}

	if (!m_game_entity_system)
	{
		const auto entity_system_ptr = m_memory->read_t<uintptr_t>(client_base.value() + get_entity_list_offset());
		if (is_probable_pointer(entity_system_ptr))
			m_game_entity_system = reinterpret_cast<c_game_entity_system*>(entity_system_ptr);
	}

	success &= (m_game_entity_system != nullptr);

	return success;
}
