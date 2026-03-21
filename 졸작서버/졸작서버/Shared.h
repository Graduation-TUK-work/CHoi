#pragma once

#pragma pack(push, 1)

// 패킷 타입 (언리얼 uint8과 호환)
enum EPacketType : unsigned char {
    PKT_JOIN = 1,
    PKT_MOVE = 2,
};

// 언리얼 FVector를 담을 구조체
struct FPlayerData {
    int PlayerId;
    float X, Y, Z;
};

struct FPacketJoin {
    unsigned char Type;
    int MyId;
};

struct FPacketMove {
    unsigned char Type;
    FPlayerData Data;
};

#pragma pack(pop)
