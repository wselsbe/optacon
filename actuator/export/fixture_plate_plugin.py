import math
from pcbnewTransition import pcbnew

from kikit.common import *
from kikit.panelize import Panel
from kikit.plugin import ToolingPlugin


class FixturePlatePlugin(ToolingPlugin):


    def buildTooling(self, panel: Panel) -> None:
        
        rail_thickness = fromMm(5)
        hole_diameter = fromMm(2.51)
        hole_spacing = fromMm(10)
        offset = rail_thickness / 2

        minx, miny, maxx, maxy = panel.boardSubstrate.bounds()

        minx = minx + offset
        miny = miny + offset
        maxx = maxx - offset
        maxy = maxy - offset

        x_hole_count = math.floor((maxx - minx) / hole_spacing)
        y_hole_count = math.floor((maxy - miny) / hole_spacing)
        x_offset_aligned = (x_hole_count * hole_spacing)
        y_offset_aligned = (y_hole_count * hole_spacing)

        #panel.addNPTHole(toKiCADPoint((minx, miny)), hole_diameter, paste=True, adhesive=True)
        panel.addNPTHole(toKiCADPoint((minx, miny + y_offset_aligned)), hole_diameter, paste=True, adhesive=True)
        #panel.addNPTHole(toKiCADPoint((minx + x_offset_aligned, miny)), hole_diameter, paste=True, adhesive=True)
        panel.addNPTHole(toKiCADPoint((minx + x_offset_aligned, miny + y_offset_aligned)), hole_diameter, paste=True, adhesive=True)

