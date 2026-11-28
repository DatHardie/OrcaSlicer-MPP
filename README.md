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
- The project file stores the printer of each plate and unsaved preset changes ([format](docs/HLSD/multi-printer-3mf.md)).
- With a print host such as Moonraker, **Print** sends the selected plate to its printer, and **Print all** sends each sliced plate to its own printer.
- **Project Printers** in the Prepare sidebar lists the printers with their plates. Adding a printer creates a plate for it, clicking a printer selects its plates, and removing a printer moves its plates to the project printer.

## Use

1. **Multi-printer projects (experimental)** in **Preferences** is on by default. Turned off (after a restart), this build works like OrcaSlicer.
2. Add a printer under **Project Printers**, or change the **Printer** of a plate in the plate settings. **Project printer** is the normal printer of the project.

The plates of each printer form one row. Without plates for other printers, OrcaSlicer works as before.

Selecting a plate of another printer, also through **Project Printers**, keeps the view. To zoom to its bed instead, turn on **Zoom to the bed when selecting a plate of another printer** in **Preferences**.

## Next to OrcaSlicer

This build can be installed and used next to OrcaSlicer:

- It keeps its settings in its own folder, `OrcaSlicer-MPP` instead of `OrcaSlicer`. The Windows zip keeps them in `data_dir` next to `orca-slicer.exe`. Presets are not shared; export them in OrcaSlicer and import them here.
- The Windows installer, the macOS app and the Flatpak install as **OrcaSlicer MPP**.
- `orcaslicer://` links open in OrcaSlicer when it is installed.

## Pitfalls

- Every printer keeps at least one plate. To drop a printer, remove it under **Project Printers**.
- Selecting a plate switches the presets. Unsaved changes stay with their printer and mark the project as modified.
- Removing a printer or rearranging plates discards the slice results of all plates.
- If undo or redo drops a printer with unsaved changes, it asks until **Save** or **Discard** is chosen.
- OrcaSlicer behaviour: saving a filament in the unsaved-changes dialog also sets filament 1 to it, and the print button returns to its default action after a printer change.

## Limitations

- Presets of other printers are stored by name. They must exist where the project is opened.
- Official OrcaSlicer, or this fork with the option off, opens such a project with all plates on one printer.
- **Print** was tested with one real printer (upload without starting the print), **Print all** only with simulated Moonraker hosts. Bambu Lab network printing is not adapted.
- Tested on Linux (x86_64) and Windows (x64, portable). The other downloads are not tested.

## Build

Like OrcaSlicer, see [How to build](https://www.orcaslicer.com/wiki/how_to_build).

## License

GNU Affero General Public License, version 3, like OrcaSlicer ([LICENSE.txt](LICENSE.txt)).

- OrcaSlicer includes a **pressure advance calibration pattern test** adapted from Andrew Ellis' generator, which is licensed under GNU General Public License, version 3. Ellis' generator is itself adapted from a generator developed by Sineos for Marlin, which is licensed under GNU General Public License, version 3.
- The **Bambu networking plugin** is based on non-free libraries from BambuLab. It is optional to the OrcaSlicer and provides extended functionalities for Bambulab printer users.
