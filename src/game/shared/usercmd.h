#pragma once

#include "mathlib/vector.h"

#include <cstddef>
#include <cstdint>

class CUserCmd
{
  public:
    int command_number;
    int tick_count;
    float command_time;
    QAngle viewangles;
    bool hasLocalViewAngles;
    std::byte pad_0019[3];
    QAngle localViewAngles;
    QAngle attackangles;
    float forwardmove;
    float sidemove;
    float upmove;
    std::uint32_t buttons;
    std::uint8_t impulse;
    std::byte pad_0045;
    std::int16_t weaponselect;
    std::int32_t meleetarget;
    std::uint8_t unknown_004C;
    std::byte pad_004D[3];
    std::uint32_t random_seed;
    bool respawnInputDebounce;
    bool queuePrimaryAttack;
    bool hasbeenpredicted;
    std::byte pad_0057;
    Vector3D playerEyePositionAfterLastPrediction;
    float playerFovAfterLastPrediction;
    QAngle headangles;
    Vector3D headoffset;
    Vector3D camerapos;
    QAngle cameraangles;
    bool trace_camera;
    std::byte pad_0099[3];
    int snapshot_start_tick;
    int snapshot_end_tick;
    std::uint32_t predicted_server_event_ack;
    std::uint32_t previous_predicted_server_event_ack;
    float frameTime;
    std::uint64_t creation_time_ms;
    Vector3D touchingSlipTriggersOrigin[6];
    Vector3D touchingSlipTriggersDirection[6];
};

static_assert(sizeof(CUserCmd) == 0x148);

class alignas(4) SV_CUserCmd
{
  public:
    std::uint32_t command_number;
    std::uint32_t tick_count;
    float command_time;
    Vector3D worldViewAngles;
    std::byte gap18[4];
    Vector3D localViewAngles;
    Vector3D attackangles;
    Vector3D move;
    std::uint32_t buttons;
    std::uint8_t impulse;
    std::int16_t weaponselect;
    std::uint32_t meleetarget;
    bool unk;
    bool unkownBool;
    bool unknownBool2;
    bool unknownBool3;
    std::uint32_t random_seed;
    bool unknownBool54;
    bool unknownBool55;
    bool unknownBool56;
    bool unknownBool57;
    Vector3D headAngles;
    Vector3D headOffset;
    Vector3D cameraPos;
    Vector3D cameraAngles;
    std::byte gap88[4];
    int tickSomething;
    std::uint32_t dword90;
    std::uint32_t predictedServerEventAck;
    std::uint32_t dword98;
    float frameTime;
    std::byte reservedA0[0x98];
};

static_assert(sizeof(SV_CUserCmd) == 0x138);
static_assert(alignof(SV_CUserCmd) == 4);
static_assert(offsetof(SV_CUserCmd, command_time) == 0x8);
static_assert(offsetof(SV_CUserCmd, attackangles) == 0x28);
static_assert(offsetof(SV_CUserCmd, buttons) == 0x40);
static_assert(offsetof(SV_CUserCmd, tickSomething) == 0x8C);
static_assert(offsetof(SV_CUserCmd, dword90) == 0x90);
static_assert(offsetof(SV_CUserCmd, frameTime) == 0x9C);
