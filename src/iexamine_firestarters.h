#pragma once
#ifndef CATA_SRC_IEXAMINE_FIRESTARTERS_H
#define CATA_SRC_IEXAMINE_FIRESTARTERS_H

#include <map>

#include "coords_fwd.h"
#include "item_location.h"

class Character;

namespace iexamine
{
/**
 * The firestarters @p you can light the fire at @p examp with, by how fast they light it: what
 * @p you carry, and with @p nearby also what lies (or is in vehicle cargo) within PICKUP_RANGE,
 * but not at @p examp itself.
 */
std::multimap<int, item_location> find_firestarters( Character &you, const tripoint_bub_ms &examp,
        bool nearby );
} // namespace iexamine

#endif // CATA_SRC_IEXAMINE_FIRESTARTERS_H
