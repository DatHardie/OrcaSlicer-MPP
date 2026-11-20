# OrcaSlicer Multi-Printer Projects

Unofficial fork of [OrcaSlicer](https://github.com/OrcaSlicer/OrcaSlicer). It is not made or supported by the OrcaSlicer team. The work is on the branch [`multi-printer`](../../tree/multi-printer).

## Goal

Split a large project across several printers in one OrcaSlicer project, instead of creating a separate project for each printer as is usual today.

Works now:

- Each plate can use its own printer.
- Each plate is shown at the bed size of its printer.
- Selecting a plate switches to its printer, process and filament presets.
- Each plate is sliced with its own printer.
- A new plate takes the printer of the selected plate.
- The project file stores the printer of each plate and unsaved preset changes.
- With a print host such as Moonraker, **Print** sends the selected plate to its printer, and **Print all** sends each sliced plate to its own printer.

Planned:

- A project view lists the printers of the project, and printers can be added there.

## Use

1. Turn on **Preferences → Behaviour → Multi-printer projects (experimental)** and restart OrcaSlicer. The option is off by default.
2. Open the plate settings and choose a **Printer**. **Project printer** is the normal printer of the project.

The plates of each printer form one row. Without plates for other printers, OrcaSlicer works as before.

## Limitations

- Presets of other printers are stored by name. They must exist where the project is opened.
- Official OrcaSlicer, or this fork with the option off, opens such a project with all plates on one printer.
- Printing was tested with simulated Moonraker hosts, not with real printers. Bambu Lab network printing is not adapted.
- Tested on Linux only.

## Build

Like OrcaSlicer, see [How to build](https://www.orcaslicer.com/wiki/how_to_build).

## License

GNU Affero General Public License, version 3, like OrcaSlicer ([LICENSE.txt](LICENSE.txt)).
