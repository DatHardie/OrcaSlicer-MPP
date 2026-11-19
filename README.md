# OrcaSlicer Multi-Printer Projects

Unofficial fork of [OrcaSlicer](https://github.com/OrcaSlicer/OrcaSlicer). It is not made or supported by the OrcaSlicer team. The work is on the branch [`multi-printer`](../../tree/multi-printer).

## Goal

One project for several printers with different bed sizes:

- Each plate can use its own printer.
- Each plate is shown at the bed size of its printer.
- Selecting a plate switches to its printer, process and filament presets.
- Each plate is sliced with its own printer.
- The project file stores the printer of each plate.
- Adding a plate asks for its printer.
- Printing sends each plate to its printer.

## Use

1. Turn on **Preferences → Behaviour → Multi-printer projects (experimental)** and restart OrcaSlicer. The option is off by default.
2. Open the plate settings and choose a **Printer**. **Project printer** is the normal printer of the project.

The plates of each printer form one row. Without plates for other printers, OrcaSlicer works as before.

## Limitations

- Project files do not store the printers of the plates yet. A saved project opens with all plates on one printer.
- Adding a plate does not ask for a printer yet. The new plate takes the printer of the selected plate.
- Printing and sending are not adapted to plates of different printers yet and are untested.
- Tested on Linux only.

## Build

Like OrcaSlicer, see [How to build](https://www.orcaslicer.com/wiki/how_to_build).

## License

GNU Affero General Public License, version 3, like OrcaSlicer ([LICENSE.txt](LICENSE.txt)).
