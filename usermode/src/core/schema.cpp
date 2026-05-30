#include "pch.hpp"

struct schema_data_t
{
	fnv1a_t m_hashed_field_name = 0;
	uint32_t m_offset = 0;
};

static std::vector<schema_data_t> m_schema_data = {};
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

bool schema::setup()
{
	m_schema_data.clear();

	if (!i::m_schema_system)
		return false;

	const auto type_scopes = i::m_schema_system->get_type_scopes();
	if (type_scopes.empty())
		return false;

	uint32_t found_classes = 0;
	for (const auto& type_scope : type_scopes)
	{
		if (!type_scope)
			continue;

		const auto module_name = type_scope->m_module_name();
		if (module_name.empty())
			continue;

		if (module_name.find(CLIENT_DLL) == std::string::npos && module_name.find(ENGINE2_DLL) == std::string::npos)
			continue;

		auto hash_classes = type_scope->m_hash_classes();
		const auto table_size = hash_classes.size();
		if (!table_size)
			continue;

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

			auto [schema_field_size, schema_field] = class_binding->get_fields();
			for (uint32_t f_idx = 0; f_idx < schema_field_size; f_idx++)
			{
				if (!schema_field)
					break;

				const auto field_name = schema_field->m_name();
				if (!field_name.empty())
				{
					auto buff = format("{}->{}", class_name, field_name);
					m_schema_data.emplace_back(fnv1a::hash(buff), schema_field->m_single_inheritance_offset());
				}

				schema_field = reinterpret_cast<c_schema_class_field_data*>(reinterpret_cast<uintptr_t>(schema_field) + sizeof(c_schema_class_field_data));
			}
		}
	}

	uint32_t fallback_added = 0;
	for (const auto& fallback_entry : m_schema_fallback_data)
	{
		const auto exists = std::ranges::any_of(m_schema_data, [fallback_entry](const schema_data_t& data)
		{
			return data.m_hashed_field_name == fallback_entry.m_hashed_field_name;
		});

		if (exists)
			continue;

		m_schema_data.emplace_back(fallback_entry);
		fallback_added++;
	}

	LOG_INFO("loaded '%d' schema classes and '%d' schema offsets ('%d' fallbacks)", found_classes, static_cast<int32_t>(m_schema_data.size()), fallback_added);
	return !m_schema_data.empty();
}

uint32_t schema::get_offset(const fnv1a_t hashed_field_name)
{
	if (const auto it = std::ranges::find_if(m_schema_data, [hashed_field_name](const schema_data_t& data)
	{
		return data.m_hashed_field_name == hashed_field_name;
	});

	it != m_schema_data.end())
		return it->m_offset;

	LOG_ERROR("failed to find an offset for the field with the hash value '%d'", hashed_field_name);
	return {};
}
