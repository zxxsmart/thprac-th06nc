#pragma once
#include "sections.h"
#include "thprac_locale_def.h"
#include <algorithm>
#include <iterator>
#include <type_traits>

namespace THPrac::TH06NC {
// Runtime IDs 71-73 are already used in NC replays. The upstream name table
// uses those indices for stage-four shot variants, so UI labels need their own IDs.
inline constexpr int OriginalSectionNameCount = int(std::size(TH06::th_sections_str[0][0]));
inline constexpr int SectionNameCount = OriginalSectionNameCount + 3;

struct SectionMenu {
    std::array<int, std::size(TH06::th_sections_cba[0][0]) + 3> values{};
    std::array<int, std::size(TH06::th_sections_cba[0][0]) + 3> labels{};
    int count = 0;

    void Normalize(int& selection, const char* const* names) const {
        if (selection < 0 || selection >= count) selection = 0;
        for (int n = 0; n < count; ++n) {
            const auto* name = names[labels[selection]];
            if (name && *name) return;
            selection = (selection + 1) % count;
        }
    }
};

inline SectionMenu BuildSectionMenu(int stage, int warp, int shot, int fakeShot) {
    SectionMenu menu;
    if (stage < 0 || stage >= 7 || warp < 2 || warp > 5) return menu;
    const int labelStage = stage == 3 ? 7 + std::clamp(fakeShot ? fakeShot - 1 : shot, 0, 3) : stage;
    const auto append = [&](const auto& values, const auto& labels) {
        static_assert(std::extent_v<std::remove_reference_t<decltype(values)>> < std::tuple_size_v<decltype(menu.values)>);
        for (size_t i = 0; i < std::size(values) && values[i]; ++i) {
            menu.values[menu.count] = values[i];
            menu.labels[menu.count++] = labels[i];
        }
    };
    if (warp <= 3) append(TH06::th_sections_cba[stage][warp - 2], TH06::th_sections_cba[labelStage][warp - 2]);
    else append(TH06::th_sections_cbt[stage][warp - 4], TH06::th_sections_cbt[labelStage][warp - 4]);
    if (stage == 6 && (warp == 3 || warp == 5)) {
        for (int i = 0; i < 3; ++i) {
            menu.values[menu.count] = FragileWing + i;
            menu.labels[menu.count++] = OriginalSectionNameCount + i;
        }
    }
    return menu;
}
}
