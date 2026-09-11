#pragma once
#include <array>
namespace THPrac::TH06NC {
// Append NC-only IDs so existing TH06 selections and saved replays stay stable.
inline constexpr int FragileWing=71, CreepingBloodstain=72, DanmakuHeart=73;
inline constexpr std::array<int,7> PortionCounts={6,4,7,9,5,2,7};
inline constexpr bool IsAddedSpell(int section) {
    return section>=FragileWing && section<=DanmakuHeart;
}
}
