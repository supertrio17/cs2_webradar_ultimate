import json, os

os.chdir('/home/engine/project/dump')
f = open('client_dll.json')
d = json.load(f)
c = d['client.dll']['classes']
print('Total classes:', len(c))

def get_chain(name, depth=0):
    if depth > 10:
        return [name]
    if name not in c:
        return [name + '?']
    p = c[name].get('parent')
    if p:
        return [name] + get_chain(p, depth+1)
    return [name]

targets = [
    'CEntityIdentity', 'CGameSceneNode', 'C_BaseEntity', 'C_BaseModelEntity',
    'CBasePlayerController', 'CCSPlayerController', 'CCSPlayerController_InGameMoneyServices',
    'CCSPlayer_ItemServices', 'C_CSPlayerPawn', 'C_CSPlayerPawnBase', 'CBasePlayerPawn',
    'CPlayer_WeaponServices', 'C_PlantedC4', 'C_SmokeGrenadeProjectile', 'C_Inferno',
    'C_BasePlayerWeapon', 'CCSWeaponBaseVData', 'CSkeletonInstance',
    'CBodyComponentSkeletonInstance', 'CBombTarget', 'CBaseAnimGraph',
    'C_BaseCSGrenadeProjectile', 'C_CSWeaponBase', 'CEntityInstance',
    'C_BaseCombatCharacter', 'C_EconEntity', 'C_BaseCSGrenade',
]

for name in targets:
    if name in c:
        chain = ' -> '.join(get_chain(name))
        fields = c[name].get('fields', {})
        print(f'\n=== {name} (chain: {chain}) ===')
        for fname, foff in sorted(fields.items(), key=lambda x: x[1]):
            print(f'  {fname}: {hex(foff)}')
    else:
        print(f'\n=== {name} === NOT FOUND')
