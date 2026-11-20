# Multi-printer projects in 3MF

A multi-printer project is a normal OrcaSlicer project 3MF with two additions: the key
`printer_slot` on plates with their own printer, and the file `Metadata/printer_slots.json`.
The GUI writes them only with the option **Multi-printer projects (experimental)** on.
`printer_slots.json` is written when a plate has its own printer, a printer other than the project
printer is active, or an inactive printer keeps unsaved changes. Otherwise neither addition is
written.

Code: `PartPlate.cpp` (`select_saved_printer_slots`, `printer_slots_to_json`,
`printer_slots_from_json`), `Plater.cpp` (`store_printer_slots`, `restore_printer_slots`),
`bbs_3mf.cpp` (`PRINTER_SLOT_ATTR`, `PRINTER_SLOTS_FILE`).

## Unchanged parts

- `Metadata/project_settings.config` holds the full configuration that was active when saving
  (printer, process and filaments with unsaved changes), as in OrcaSlicer. If `active_slot` is not
  `-1`, this is a plate printer, not the project printer.
- Other printers are stored by preset name plus their unsaved changes; the fork embeds no presets
  for them (project-embedded presets are embedded as in OrcaSlicer).
- Object positions are world coordinates. If a plate has its own printer, plates are laid out in
  one row per printer, in the order of each printer's first plate (`PrinterRowLayout.cpp`);
  otherwise OrcaSlicer's grid is used. The layout itself is not stored.

## `printer_slot` in `Metadata/model_settings.config`

```xml
<plate>
  <metadata key="plater_id" value="2"/>
  <metadata key="plater_name" value=""/>
  <metadata key="locked" value="false"/>
  <metadata key="printer_slot" value="0"/>
  ...
```

0-based index into `slots` of `printer_slots.json`. Plates without the key, or with a value
outside `slots`, use the project printer. The value is read with `atoi`, so a non-numeric value
counts as `0`. Indices are renumbered on every save.

## `Metadata/printer_slots.json`

Example (`active_slot` `-1`: the project printer was active):

```json
{
 "active_slot": -1,
 "project_printer": {
  "changed_filament": "",
  "filament_changes": {},
  "filaments": [
   "Generic PLA @Creality"
  ],
  "print": "0.20mm Standard @Creality Ender3V2",
  "print_changes": {},
  "printer": "Testdrucker Ender 3 V2",
  "printer_changes": {}
 },
 "slots": [
  {
   "changed_filament": "Generic PLA @System",
   "filament_changes": {},
   "filaments": [
    "Generic PLA @System"
   ],
   "print": "0.20mm Standard @Voron",
   "print_changes": {},
   "printer": "Testdrucker Voron 0.1",
   "printer_changes": {}
  }
 ],
 "version": 1
}
```

| Key | Content |
|---|---|
| `version` | `1`. |
| `active_slot` | Index in `slots` of the printer active when saving. `-1` for the project printer, and for an active printer without a plate (it is then not in `slots`). |
| `project_printer` | Printer entry of the project printer. |
| `slots` | Printers of plates in the order of their first plate, then printers without a plate that keep unsaved changes. Other printers without a plate are dropped. |

A printer entry:

| Key | Content |
|---|---|
| `printer`, `print` | Preset names. |
| `filaments` | Filament preset names, one per filament as of when this printer was last active. |
| `printer_changes`, `print_changes`, `filament_changes` | Unsaved changes to these presets: config key to a string from `ConfigOption::serialize()` (a vector is one string, e.g. `"0.4,0.4"` for floats, `;`-separated for strings, unlike the arrays in `project_settings.config`). `{}` for the active printer. Optional on load. |
| `changed_filament` | Filament preset selected when this printer was last left; `filament_changes` belong to it. Empty for the active printer. |

`printer_changes` never contains `print_host`, `print_host_webui`, `printhost_apikey`,
`printhost_cafile`, `printhost_user`, `printhost_password` or `printhost_port`.

## Saving

- Every 3MF export through `Plater::export_3mf` writes the additions, except exports with
  `SaveStrategy::SkipModel` (sliced files and files sent to a printer), which contain neither.
- Minimal published 3MF contain the `printer_slot` keys but not `printer_slots.json`; the keys are
  then ignored.
- In sliced files each `plate_N.gcode` is sliced with its plate's printer. In
  `Metadata/slice_info.config` only `printer_model_id` comes from each plate's printer (from the
  active printer if that preset is missing). Other values read from the configuration, such as
  `nozzle_diameters`, `extruder_type`, `nozzle_volume_type`, `timelapse_type` and filament type,
  colour and ID, come from the printer active when exporting. Filament usage and estimates come
  from each plate's slicing result.

## Loading

- The additions are ignored, and all plates use the printer of `project_settings.config`, with
  the option off, for geometry-only imports, without the file, with an empty file or one over
  16 MiB, and with a file that fails any check: not JSON, another `version`, a missing or wrongly
  typed key (missing change objects count as `{}`), an empty `printer` in `slots`, `active_slot`
  out of range, or a change value that is not a string or that its option cannot read. No case
  shows a message; a file that fails a check is logged as a warning.
- Unknown JSON keys and unknown config keys are dropped. An enum or bool value that cannot be read
  is replaced (enum by the option's built-in default, bool by `false` unless `enabled` or `on`).
- The active printer's bed comes from `project_settings.config`. Any other printer's bed comes
  from the local printer preset of exactly that name plus its `printer_changes`. Without that
  preset, or without a valid bed from it, the plates stay on the project printer and lose
  `printer_slot` on the next save. If the project printer's preset is missing while another
  printer is active, its bed is not restored.
- Objects are not moved.

Readers without these additions, such as OrcaSlicer itself or this fork with the option off,
ignore the key and do not use the file (OrcaSlicer only copies it into its temporary project
folder like other `Metadata/*.json`). All plates get the printer of `project_settings.config`,
and objects can lie outside their plates or on other plates. Saving there drops both additions.
