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
};

struct FPlayerData {
    int32 PlayerId;
    float X, Y, Z;
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