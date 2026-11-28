#pragma once

#include "libslic3r/Point.hpp"

#include <vector>

namespace Slic3r {

// A plate's bed size and printer row.
struct RowLayoutPlate
{
    int   row_key{-1};
    Vec2d size{Vec2d::Zero()};
};

// Offsets of plates, one row per printer key.
// Throws std::invalid_argument on invalid sizes or gap.
std::vector<Vec2d> printer_row_offsets(const std::vector<RowLayoutPlate> &plates, double gap_ratio);

} // namespace Slic3r
