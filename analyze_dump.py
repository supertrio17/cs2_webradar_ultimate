#!/usr/bin/env python3
import json

with open('/home/engine/project/dump/client_dll.json') as f:
    data = json.load(f)

classes = data.get('client.dll', {}).get('classes', {})
print(f"Total classes: {len(classes)}")

interesting = [
    'CEntityIdentity', 'CGameSceneNode', 'C_BaseEntity', 'C_BaseModelEntity',
    'CBasePlayerController', 'CCSPlayerController', 'CCSPlayerController_InGameMoneyServices',
    'CCSPlayer_ItemServices', 'C_CSPlayerPawn', 'C_CSPlayerPawnBase', 'CBasePlayerPawn',
    'CPlayer_WeaponServices', 'C_PlantedC4', 'C_SmokeGrenadeProjectile', 'C_Inferno',
    'C_BasePlayerWeapon', 'CCSWeaponBaseVData', 'CSkeletonInstance',
    'CBodyComponentSkeletonInstance', 'C_BombTarget', 'CBaseAnimGraph',
    'C_BaseCSGrenadeProjectile', 'C_CSWeaponBase',
    'CEntityInstance',
]

for name in interesting:
    if name in classes:
        c = classes[name]
        parent = c.get('parent')
        fields = c.get('fields', {})
        print(f"\n=== {name} ===")
        print(f"  Parent: {parent}")
        for fname, foff in sorted(fields.items(), key=lambda x: x[1]):
            print(f"    {fname}: {hex(foff)}")
    else:
        print(f"\n=== {name} === NOT FOUND")

# Inheritance chains
print("\n\n=== INHERITANCE CHAINS ===")
def get_parent_chain(name, classes, depth=0):
    if name not in classes or depth > 10:
        return [name]
    c = classes[name]
    p = c.get('parent')
    if p:
        return [name] + get_parent_chain(p, classes, depth+1)
    return [name]

for name in interesting:
    if name in classes:
        chain = get_parent_chain(name, classes)
        print(f"  {' -> '.join(chain)}")