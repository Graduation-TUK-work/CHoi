#pragma once

#pragma pack(push, 1)

// ÇÃ·§Æûº° Å¸ÀÔ Ã³¸®
#ifdef _WIN32
#include <cstdint>
typedef uint8_t uint8;
typedef int32_t int32;
#else
#include "CoreMinimal.h"
#endif



enum EPacketType : uint8 {
    PKT_JOIN = 1,
    PKT_MOVE = 2,
    PKT_ACTION = 3,
    PKT_ROLE_SELECT = 4,
};

enum ECharacterType : uint8 {
    CHARACTER_SURVIVOR = 1,
    CHARACTER_KILLER = 2,
};

enum EPlayerRole : uint8 {
    ROLE_NONE = 0,
    ROLE_KILLER = 1,
    ROLE_SURVIVOR = 2
};

enum EActionType : uint8 {
    ACTION_KILLER_ATTACK = 1,
    ACTION_SURVIVOR_HIT = 2,
    ACTION_SURVIVOR_PICKUP = 3,
    ACTION_GENERATOR_START = 4,
    ACTION_GENERATOR_CANCEL = 5,
    ACTION_GENERATOR_COMPLETE = 6,
};

struct FPacketAction {
    uint8 Type;
    uint8 ActionType;
    int32 InstigatorId;
    int32 TargetId;
    float X, Y, Z;
    float RotationYaw;
};


struct FPacketRoleSelect {
    uint8 Type;
    int32 PlayerId;
    uint8 Role;
};
struct FPlayerData {
    int32 PlayerId;
    uint8 CharacterType;
    float X, Y, Z;
    float RotationYaw;
    float ForwardValue;
    float RightValue;
    bool bIsSprinting;
};

struct FPacketJoin {
    uint8 Type;
    int32 MyId;
};

struct FPacketMove {
    uint8 Type;
    FPlayerData Data;
};

#pragma pack(pop)
