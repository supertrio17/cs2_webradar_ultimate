import ctypes

def fnv1a_64(string):
    basis = 0xcbf29ce484222325
    prime = 0x100000001b3
    hash_val = basis
    for char in string:
        hash_val ^= ord(char)
        hash_val = (hash_val * prime) & 0xFFFFFFFFFFFFFFFF
    return hash_val

def to_int32(val):
    return ctypes.c_int32(val & 0xFFFFFFFF).value

test_strings = [
    "CEntityIdentity->m_designerName",
    "CEntityIdentity->m_flags",
    "CEntityInstance->m_pEntity",
    "CGameSceneNode->m_vecAbsOrigin",
    "CGameSceneNode->m_vecOrigin",
    "C_BaseEntity->m_pGameSceneNode",
    "C_BaseEntity->m_iHealth",
    "C_BaseEntity->m_iTeamNum",
    "C_BaseEntity->m_hOwnerEntity",
    "CPlayer_WeaponServices->m_hActiveWeapon",
    "CPlayer_WeaponServices->m_hMyWeapons",
    "CCSPlayer_ItemServices->m_bHasDefuser",
    "CCSPlayer_ItemServices->m_bHasHelmet",
    "C_BasePlayerPawn->m_pWeaponServices",
    "C_BasePlayerPawn->m_pItemServices",
    "C_CSPlayerPawn->m_ArmorValue",
    "C_CSPlayerPawn->m_angEyeAngles",
    "C_CSPlayerPawnBase->m_flFlashOverlayAlpha",
    "C_CSPlayerPawn->m_bIsScoped",
    "CBasePlayerController->m_hPawn",
    "CBasePlayerController->m_steamID",
    "CCSPlayerController_InGameMoneyServices->m_iAccount",
    "CCSPlayerController->m_pInGameMoneyServices",
    "CCSPlayerController->m_iCompTeammateColor",
    "CCSPlayerController->m_sSanitizedPlayerName",
    "C_PlantedC4->m_bBombTicking",
    "C_PlantedC4->m_flC4Blow",
    "C_PlantedC4->m_bBombDefused",
    "C_PlantedC4->m_bBeingDefused",
    "C_PlantedC4->m_flDefuseCountDown",
    "CCSWeaponBaseVData->m_WeaponType",
    "CCSWeaponBaseVData->m_szName",
    "C_BaseEntity->m_nSubclassID",
    "C_SmokeGrenadeProjectile->m_nSmokeEffectTickBegin",
    "C_SmokeGrenadeProjectile->m_vSmokeDetonationPos",
    "C_Inferno->m_bFireIsBurning",
    "C_Inferno->m_firePositions",
    "C_Inferno->m_fireCount",
    "C_Inferno->m_nFireEffectTickBegin",
]

target_hashes = [-329896605, 269165624, 375277544]

for s in test_strings:
    h = fnv1a_64(s)
    h32 = to_int32(h)
    if h32 in target_hashes:
        print(f"MATCH: {s} -> {h32}")
    else:
        # print(f"DEBUG: {s} -> {h32}")
        pass
