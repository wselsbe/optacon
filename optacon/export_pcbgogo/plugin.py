import math

from pcbnew import wxPoint

from kikit.common import fromMm
from kikit.panelize import Panel
from kikit.plugin import ToolingPlugin


class FixturePlatePlugin(ToolingPlugin):

    def buildTooling(self, panel: Panel) -> None:
        rail_thickness = fromMm(5)
        hole_diameter = fromMm(2.5)
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

        panel.addNPTHole(wxPoint(minx, miny), hole_diameter, paste=True, adhesive=True)
        panel.addNPTHole(wxPoint(minx, miny + y_offset_aligned), hole_diameter, paste=True, adhesive=True)
        panel.addNPTHole(wxPoint(minx + x_offset_aligned, miny), hole_diameter, paste=True, adhesive=True)
        panel.addNPTHole(wxPoint(minx + x_offset_aligned, miny + y_offset_aligned), hole_diameter, paste=True, adhesive=True)

        copper_diameter = fromMm(2)
        opening_diameter = fromMm(1)
        #panel.addFiducial(wxPoint(minx + hole_spacing, miny), copper_diameter, opening_diameter)
