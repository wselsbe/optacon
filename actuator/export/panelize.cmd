@echo off
kikit panelize ^
--layout "rows: 1; cols: 3;" ^
--source "auto; tolerance: 5mm" ^
--cuts vcuts ^
--framing "railstb; width: 5mm; space: 2.5mm; fillet: 1mm" ^
--fiducials "3fid; hoffset: 7.5mm; voffset: 2.5mm; coppersize: 2mm; opening: 1mm;" ^
--tooling "type:plugin;code:fixture_plate_plugin.py.FixturePlatePlugin" ^
--post "millradius: 1mm" ^
..\actuator.kicad_pcb actuator_panel.kicad_pcb