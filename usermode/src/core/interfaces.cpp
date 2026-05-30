#include "pch.hpp"

bool i::refresh_global_vars()
{
	const auto [client_base, client_size] = m_memory->get_module_info(CLIENT_DLL);
	if (!client_base.has_value() || !client_size.has_value())
		return false;

	m_global_vars = m_memory->read_t<c_global_vars*>(client_base.value() + offsets::m_dw_global_vars);
	if (!m_global_vars)
	{
		const auto global_vars_pattern = m_memory->find_pattern(CLIENT_DLL, GET_GLOBAL_VARS);
		if (!global_vars_pattern.has_value())
			return false;

		m_global_vars = m_memory->read_t<c_global_vars*>(global_vars_pattern->rip().as<c_global_vars*>());
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

	m_schema_system = m_memory->read_t<c_schema_system*>(schema_system_base.value() + offsets::m_schema_system);
	if (!m_schema_system)
	{
		const auto schema_pattern = m_memory->find_pattern(SCHEMASYSTEM_DLL, GET_SCHEMA_SYSTEM);
		if (schema_pattern.has_value())
			m_schema_system = schema_pattern->rip().as<c_schema_system*>();
	}
	
	success &= (m_schema_system != nullptr);
	success &= refresh_global_vars();

	m_game_entity_system = m_memory->read_t<c_game_entity_system*>(client_base.value() + offsets::m_dw_entity_list);
	if (!m_game_entity_system)
	{
		const auto entity_list_pattern = m_memory->find_pattern(CLIENT_DLL, GET_ENTITY_LIST);
		if (entity_list_pattern.has_value())
			m_game_entity_system = m_memory->read_t<c_game_entity_system*>(entity_list_pattern->rip().as<c_game_entity_system*>());
	}

	success &= (m_game_entity_system != nullptr);

	return success;
}
