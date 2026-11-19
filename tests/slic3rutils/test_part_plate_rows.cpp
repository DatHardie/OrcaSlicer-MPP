#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cstddef>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

#include <cereal/archives/binary.hpp>

#include "libslic3r/Config.hpp"
#include "libslic3r/Model.hpp"
#include "libslic3r/Point.hpp"
#include "libslic3r/PrintConfig.hpp"
#include "libslic3r/TriangleMesh.hpp"
#include "slic3r/GUI/PartPlate.hpp"

using namespace Slic3r;
using namespace Slic3r::GUI;
using Catch::Matchers::WithinAbs;

namespace {

constexpr int    project_size = 350;
constexpr double gap          = 0.2; // LOGICAL_PART_PLATE_GAP

Pointfs square(double size) { return {Vec2d(0., 0.), Vec2d(size, 0.), Vec2d(size, size), Vec2d(0., size)}; }

// Plate list without Plater, like the CLI.
struct PlateFixture
{
    Model         model;
    GUI::PartPlateList plates{project_size, project_size, 300., nullptr, &model, ptFFF};

    explicit PlateFixture(int count)
    {
        plates.set_shapes(square(project_size), {}, {}, {}, {}, {}, 0.f, 0.f);
        for (int i = 1; i < count; ++i)
            plates.create_plate();
        plates.find_or_add_printer_slot("Voron 0.2");
        plates.find_or_add_printer_slot("Trident");
    }

    Vec2d origin(int index) { return plates.get_plate(index)->get_origin().head<2>(); }
};

void check_size(const PartPlate* plate, double width, double depth)
{
    CHECK_THAT(plate->get_size().x(), WithinAbs(width, 1e-6));
    CHECK_THAT(plate->get_size().y(), WithinAbs(depth, 1e-6));
}

void check_points(const Pointfs& actual, const Pointfs& expected)
{
    REQUIRE(actual.size() == expected.size());
    for (size_t i = 0; i < actual.size(); ++i) {
        CHECK_THAT(actual[i].x(), WithinAbs(expected[i].x(), 1e-9));
        CHECK_THAT(actual[i].y(), WithinAbs(expected[i].y(), 1e-9));
    }
}

PlatePrinterBed small_bed(int slot)
{
    PlatePrinterBed bed;
    bed.slot   = slot;
    bed.shape  = square(120.);
    bed.height = 115.;
    return bed;
}

} // namespace

TEST_CASE("Without own beds plates keep the legacy grid", "[PartPlateRows]")
{
    PlateFixture f(3);
    const double stride = project_size * (1. + gap);
    // Three plates use two columns.
    CHECK_THAT(f.origin(1).x(), WithinAbs(stride, 1e-6));
    CHECK_THAT(f.origin(1).y(), WithinAbs(0., 1e-6));
    CHECK_THAT(f.origin(2).x(), WithinAbs(0., 1e-6));
    CHECK_THAT(f.origin(2).y(), WithinAbs(-stride, 1e-6));
    CHECK_FALSE(f.plates.has_printer_beds());
}

TEST_CASE("A plate with its own bed moves to its printer row", "[PartPlateRows]")
{
    PlateFixture f(3);
    REQUIRE(f.plates.set_plate_printer_bed(1, small_bed(0)));
    CHECK(f.plates.has_printer_beds());

    // Project printer row: plates 0 and 2.
    CHECK_THAT(f.origin(0).x(), WithinAbs(0., 1e-6));
    CHECK_THAT(f.origin(2).x(), WithinAbs(project_size * (1. + gap), 1e-6));
    CHECK_THAT(f.origin(2).y(), WithinAbs(0., 1e-6));
    // Small printer row below it.
    CHECK_THAT(f.origin(1).x(), WithinAbs(0., 1e-6));
    CHECK_THAT(f.origin(1).y(), WithinAbs(-(120. + gap * project_size), 1e-6));

    const PartPlate* own = f.plates.get_plate(1);
    check_size(own, 120., 120.);
    check_size(f.plates.get_plate(0), project_size, project_size);
}

TEST_CASE("Objects move with their plate into the row layout", "[PartPlateRows]")
{
    PlateFixture f(3);
    ModelObject* object = f.model.add_object();
    object->add_volume(make_cube(20., 20., 20.));
    ModelInstance* instance = object->add_instance();
    const Vec2d    before   = f.origin(2) + Vec2d(100., 100.);
    instance->set_offset(Vec3d(before.x(), before.y(), 10.));
    f.plates.reload_all_objects();
    REQUIRE(f.plates.get_plate(2)->contain_instance(0, 0));

    REQUIRE(f.plates.set_plate_printer_bed(1, small_bed(0)));
    const Vec2d after = instance->get_offset().head<2>();
    CHECK_THAT(after.x(), WithinAbs(f.origin(2).x() + 100., 1e-6));
    CHECK_THAT(after.y(), WithinAbs(f.origin(2).y() + 100., 1e-6));
    CHECK(f.plates.get_plate(2)->contain_instance(0, 0));
}

TEST_CASE("Clearing the last own bed restores the legacy grid", "[PartPlateRows]")
{
    PlateFixture f(3);
    const Vec2d legacy = f.origin(2);
    REQUIRE(f.plates.set_plate_printer_bed(1, small_bed(0)));
    REQUIRE(f.plates.set_plate_printer_bed(1, PlatePrinterBed{}));
    CHECK_FALSE(f.plates.has_printer_beds());
    CHECK_THAT(f.origin(2).x(), WithinAbs(legacy.x(), 1e-6));
    CHECK_THAT(f.origin(2).y(), WithinAbs(legacy.y(), 1e-6));
    check_size(f.plates.get_plate(1), project_size, project_size);
}

TEST_CASE("A new plate joins the project printer row", "[PartPlateRows]")
{
    PlateFixture f(2);
    REQUIRE(f.plates.set_plate_printer_bed(1, small_bed(0)));
    const int index = f.plates.create_plate();
    REQUIRE(index == 2);
    CHECK_THAT(f.origin(2).x(), WithinAbs(project_size * (1. + gap), 1e-6));
    CHECK_THAT(f.origin(2).y(), WithinAbs(0., 1e-6));
}

TEST_CASE("Deleting a printer row's only plate closes the row", "[PartPlateRows]")
{
    PlateFixture f(3);
    REQUIRE(f.plates.set_plate_printer_bed(1, small_bed(0)));
    REQUIRE(f.plates.set_plate_printer_bed(2, small_bed(1)));
    REQUIRE(f.plates.delete_plate(1) == 0);
    REQUIRE(f.plates.get_plate_count() == 2);
    // Former plate 2 now starts the second row.
    CHECK_THAT(f.origin(1).x(), WithinAbs(0., 1e-6));
    CHECK_THAT(f.origin(1).y(), WithinAbs(-(120. + gap * project_size), 1e-6));
}

TEST_CASE("Invalid own beds are rejected and change nothing", "[PartPlateRows]")
{
    PlateFixture f(2);
    const double nan = std::numeric_limits<double>::quiet_NaN();

    CHECK_FALSE(f.plates.set_plate_printer_bed(-1, small_bed(0)));
    CHECK_FALSE(f.plates.set_plate_printer_bed(2, small_bed(0)));

    PlatePrinterBed bed = small_bed(0);
    const int       variant = GENERATE(0, 1, 2, 3, 4);
    switch (variant) {
    case 0: bed.shape.resize(2); break;
    case 1: bed.shape[2] = Vec2d(nan, 10.); break;
    case 2: bed.height = 0.; break;
    case 3: bed.height = nan; break;
    case 4: bed.shape = {Vec2d(0., 0.), Vec2d(0.5, 0.), Vec2d(0.5, 100.)}; break;
    }
    CHECK_FALSE(f.plates.set_plate_printer_bed(1, bed));
    CHECK_FALSE(f.plates.has_printer_beds());
    CHECK_THAT(f.origin(1).x(), WithinAbs(project_size * (1. + gap), 1e-6));
}

TEST_CASE("Printer slots are found by printer name", "[PartPlateRows]")
{
    PlateFixture f(1);
    CHECK(f.plates.find_or_add_printer_slot("Voron 0.2") == 0);
    CHECK(f.plates.find_or_add_printer_slot("Trident") == 1);
    CHECK(f.plates.find_or_add_printer_slot("Voron 2.4") == 2);
    CHECK(f.plates.printer_slots().size() == 3);
    CHECK(f.plates.find_or_add_printer_slot("") == -1);
    CHECK(f.plates.printer_slots().size() == 3);
}

TEST_CASE("Slot -1 is the project printer", "[PartPlateRows]")
{
    PlateFixture f(1);
    REQUIRE(f.plates.printer_slot(-1) != nullptr);
    f.plates.printer_slot(-1)->printer = "Voron 2.4";
    CHECK(f.plates.printer_slot(-1)->printer == "Voron 2.4");
    CHECK(f.plates.printer_slot(0)->printer == "Voron 0.2");
    CHECK(f.plates.printer_slot(2) == nullptr);
    CHECK(f.plates.printer_slot(-2) == nullptr);
}

TEST_CASE("A plate cannot use a slot that does not exist", "[PartPlateRows]")
{
    PlateFixture f(2);
    CHECK_FALSE(f.plates.set_plate_printer_bed(1, small_bed(2)));
    CHECK_FALSE(f.plates.has_printer_beds());
}

TEST_CASE("A slot's new bed reaches all its plates", "[PartPlateRows]")
{
    PlateFixture f(3);
    REQUIRE(f.plates.set_plate_printer_bed(1, small_bed(0)));
    REQUIRE(f.plates.set_plate_printer_bed(2, small_bed(0)));

    PlatePrinterBed bigger = small_bed(5); // slot is taken from the call
    bigger.shape           = square(180.);
    bigger.height          = 170.;
    REQUIRE(f.plates.set_slot_printer_bed(0, bigger));

    for (int i : {1, 2}) {
        check_size(f.plates.get_plate(i), 180., 180.);
        CHECK(f.plates.get_plate(i)->printer_bed().slot == 0);
        CHECK_THAT(f.plates.get_plate(i)->printer_bed().height, WithinAbs(170., 1e-9));
    }
    check_size(f.plates.get_plate(0), project_size, project_size);
    CHECK_THAT(f.origin(2).x(), WithinAbs(180. * (1. + gap), 1e-6));
}

TEST_CASE("Invalid slot beds are rejected", "[PartPlateRows]")
{
    PlateFixture f(2);
    REQUIRE(f.plates.set_plate_printer_bed(1, small_bed(0)));
    PlatePrinterBed bad = small_bed(0);
    bad.height          = -5.;
    CHECK_FALSE(f.plates.set_slot_printer_bed(0, bad));
    CHECK_FALSE(f.plates.set_slot_printer_bed(-1, small_bed(0)));
    CHECK_FALSE(f.plates.set_slot_printer_bed(7, small_bed(0)));
    check_size(f.plates.get_plate(1), 120., 120.);
}

TEST_CASE("Resetting the plates keeps printer slots for undo", "[PartPlateRows]")
{
    PlateFixture f(2);
    REQUIRE(f.plates.set_plate_printer_bed(1, small_bed(0)));
    f.plates.printer_slot(-1)->printer = "Voron 2.4";
    f.plates.reset(true);
    CHECK_FALSE(f.plates.has_printer_beds());
    CHECK(f.plates.printer_slots().size() == 2);
    CHECK(f.plates.printer_slot(-1)->printer == "Voron 2.4");
}

TEST_CASE("Clearing printer slots forgets them", "[PartPlateRows]")
{
    PlateFixture f(2);
    f.plates.printer_slot(-1)->printer = "Voron 2.4";
    f.plates.clear_printer_slots();
    CHECK(f.plates.printer_slots().empty());
    CHECK(f.plates.printer_slot(-1)->printer.empty());
    CHECK(f.plates.printer_slot(0) == nullptr);
}

TEST_CASE("A plate's own bed survives an undo snapshot round trip", "[PartPlateRows]")
{
    PlatePrinterBed bed = small_bed(1);
    bed.exclude_areas   = square(10.);
    bed.height_to_lid   = 20.f;
    std::stringstream data;
    {
        cereal::BinaryOutputArchive out(data);
        out(bed);
    }
    PlatePrinterBed loaded;
    {
        cereal::BinaryInputArchive in(data);
        in(loaded);
    }
    CHECK(loaded.slot == 1);
    check_points(loaded.shape, bed.shape);
    check_points(loaded.exclude_areas, bed.exclude_areas);
    CHECK_THAT(loaded.height, WithinAbs(115., 1e-9));
    CHECK_THAT(loaded.height_to_lid, WithinAbs(20., 1e-6));
}

TEST_CASE("Reapplying a slot's unchanged bed keeps slice results", "[PartPlateRows]")
{
    PlateFixture f(3);
    REQUIRE(f.plates.set_plate_printer_bed(1, small_bed(0)));
    for (int i = 0; i < 3; ++i)
        f.plates.get_plate(i)->update_slice_result_valid_state(true);

    REQUIRE(f.plates.set_slot_printer_bed(0, small_bed(0)));
    for (int i = 0; i < 3; ++i)
        CHECK(f.plates.get_plate(i)->is_slice_result_valid());

    PlatePrinterBed clearance = small_bed(0);
    clearance.height_to_lid   = 30.f;
    clearance.exclude_areas   = square(5.);
    REQUIRE(f.plates.set_slot_printer_bed(0, clearance));
    CHECK(f.plates.get_plate(1)->is_slice_result_valid());
    CHECK_THAT(f.plates.get_plate(1)->printer_bed().height_to_lid, WithinAbs(30., 1e-6));

    PlatePrinterBed taller = small_bed(0);
    taller.height          = 200.;
    REQUIRE(f.plates.set_slot_printer_bed(0, taller));
    CHECK_FALSE(f.plates.get_plate(1)->is_slice_result_valid());
}

TEST_CASE("Plates created with a bed template join that printer's row", "[PartPlateRows]")
{
    PlateFixture f(2);
    REQUIRE(f.plates.set_plate_printer_bed(1, small_bed(0)));
    f.plates.set_new_plate_bed(small_bed(0));
    const int index = f.plates.create_plate(false);
    REQUIRE(index == 2);
    CHECK(f.plates.get_plate(2)->printer_bed().slot == 0);
    check_size(f.plates.get_plate(2), 120., 120.);
    CHECK_THAT(f.origin(2).x(), WithinAbs(120. * (1. + gap), 1e-6));
    CHECK_THAT(f.origin(2).y(), WithinAbs(f.origin(1).y(), 1e-6));

    // Without a template new plates use the project printer.
    f.plates.set_new_plate_bed(PlatePrinterBed{});
    REQUIRE(f.plates.create_plate(false) == 3);
    CHECK_FALSE(f.plates.get_plate(3)->printer_bed().assigned());
}

TEST_CASE("A slot reports stored preset changes of any kind", "[PartPlateRows]")
{
    PrinterSlot slot{"Trident", "0.20mm Standard", {"Generic PLA"}};
    // A filament name alone is no change.
    slot.changed_filament = "Generic PLA";
    CHECK_FALSE(slot.has_changes());

    const int kind = GENERATE(0, 1, 2);
    DynamicPrintConfig& changes = kind == 0 ? slot.printer_changes : kind == 1 ? slot.print_changes : slot.filament_changes;
    changes.set_key_value("layer_height", new ConfigOptionFloat(0.3));
    CHECK(slot.has_changes());
    changes.clear();
    CHECK_FALSE(slot.has_changes());
}
