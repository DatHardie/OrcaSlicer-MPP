#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cstddef>
#include <limits>
#include <stdexcept>
#include <vector>

#include "libslic3r/Point.hpp"
#include "libslic3r/PrinterRowLayout.hpp"

using namespace Slic3r;
using Catch::Matchers::WithinAbs;

namespace {
constexpr double gap = 0.2;
}

TEST_CASE("Plates of one printer form a single row", "[PrinterRowLayout]")
{
    const std::vector<RowLayoutPlate> plates(3, RowLayoutPlate{-1, Vec2d(350., 350.)});
    const auto offsets = printer_row_offsets(plates, gap);
    REQUIRE(offsets.size() == 3);
    for (size_t i = 0; i < offsets.size(); ++i) {
        CHECK_THAT(offsets[i].x(), WithinAbs(double(i) * 350. * (1. + gap), 1e-9));
        CHECK_THAT(offsets[i].y(), WithinAbs(0., 1e-9));
    }
}

TEST_CASE("Each printer gets its own row in first-use order", "[PrinterRowLayout]")
{
    // Voron 2.4, Voron 0.2, Voron 2.4, Trident
    const std::vector<RowLayoutPlate> plates{
        {0, Vec2d(350., 350.)}, {1, Vec2d(120., 120.)}, {0, Vec2d(350., 350.)}, {2, Vec2d(300., 300.)}};
    const auto offsets = printer_row_offsets(plates, gap);
    REQUIRE(offsets.size() == 4);

    CHECK_THAT(offsets[0].x(), WithinAbs(0., 1e-9));
    CHECK_THAT(offsets[2].x(), WithinAbs(350. * (1. + gap), 1e-9));
    CHECK_THAT(offsets[2].y(), WithinAbs(offsets[0].y(), 1e-9));

    const double row1_y = -(120. + gap * 350.);
    CHECK_THAT(offsets[1].x(), WithinAbs(0., 1e-9));
    CHECK_THAT(offsets[1].y(), WithinAbs(row1_y, 1e-9));

    const double row2_y = row1_y - (300. + gap * 300.);
    CHECK_THAT(offsets[3].x(), WithinAbs(0., 1e-9));
    CHECK_THAT(offsets[3].y(), WithinAbs(row2_y, 1e-9));
}

TEST_CASE("Rows of different depth never overlap", "[PrinterRowLayout]")
{
    const double small = GENERATE(50., 120., 350., 600.);
    const double large = GENERATE(50., 120., 350., 600.);
    const std::vector<RowLayoutPlate> plates{{0, Vec2d(200., small)}, {1, Vec2d(200., large)}};
    const auto offsets = printer_row_offsets(plates, gap);
    REQUIRE(offsets.size() == 2);
    // Lower row's top stays below upper row.
    CHECK(offsets[1].y() + large < offsets[0].y());
}

TEST_CASE("An empty plate list has no offsets", "[PrinterRowLayout]")
{
    CHECK(printer_row_offsets({}, gap).empty());
}

TEST_CASE("Invalid plate sizes and gaps are rejected", "[PrinterRowLayout]")
{
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    const Vec2d  bad = GENERATE_COPY(Vec2d(0., 100.), Vec2d(100., 0.), Vec2d(-1., 100.), Vec2d(nan, 100.), Vec2d(100., inf));
    CHECK_THROWS_AS(printer_row_offsets({{0, Vec2d(100., 100.)}, {1, bad}}, gap), std::invalid_argument);

    const double bad_gap = GENERATE_COPY(-0.1, nan, inf);
    CHECK_THROWS_AS(printer_row_offsets({{0, Vec2d(100., 100.)}}, bad_gap), std::invalid_argument);
    CHECK_NOTHROW(printer_row_offsets({{0, Vec2d(100., 100.)}}, 0.));
}
