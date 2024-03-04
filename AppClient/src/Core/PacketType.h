#pragma once
#include <stdint.h>

enum class PacketType : uint16_t
{
	None = 0,
	AddPlayer,
	RemovePlayer,
	MovePlayer,
	RequestPlayerInfo,
	ShootProjectile,
};