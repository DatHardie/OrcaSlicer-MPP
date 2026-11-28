#include "libslic3r/PrinterRowLayout.hpp"
#include "libslic3r/Point.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <map>
#include <stdexcept>
#include <vector>

namespace Slic3r {

std::vector<Vec2d> printer_row_offsets(const std::vector<RowLayoutPlate> &plates, double gap_ratio)
{
    if (!std::isfinite(gap_ratio) || gap_ratio < 0.)
        throw std::invalid_argument("printer_row_offsets: invalid gap ratio");

    std::map<int, size_t>            row_of_key;
    std::vector<std::vector<size_t>> rows;
    std::vector<double>              row_depth;
    for (size_t i = 0; i < plates.size(); ++i) {
        const Vec2d &size = plates[i].size;
        if (!size.allFinite() || size.x() <= 0. || size.y() <= 0.)
            throw std::invalid_argument("printer_row_offsets: invalid plate size");
        const auto [it, inserted] = row_of_key.try_emplace(plates[i].row_key, rows.size());
        if (inserted) {
            rows.emplace_back();
            row_depth.push_back(0.);
        }
        rows[it->second].push_back(i);
        row_depth[it->second] = std::max(row_depth[it->second], size.y());
    }

    std::vector<Vec2d> offsets(plates.size(), Vec2d::Zero());
    double             y = 0.;
    for (size_t r = 0; r < rows.size(); ++r) {
        if (r > 0)
            y -= row_depth[r] + gap_ratio * std::max(row_depth[r - 1], row_depth[r]);
        double x = 0.;
        for (size_t i : rows[r]) {
            offsets[i] = Vec2d(x, y);
            x += plates[i].size.x() * (1. + gap_ratio);
        }
    }
    return offsets;
}

} // namespace Slic3r
