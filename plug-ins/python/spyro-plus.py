#!/usr/bin/env python3

# Draw Spyrographs, Epitrochoids, and Lissajous curves with interactive feedback.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

import gi
gi.require_version('Gimp', '3.0')
from gi.repository import Gimp
gi.require_version('GimpUi', '3.0')
from gi.repository import GimpUi
from gi.repository import GObject
from gi.repository import GLib
from gi.repository import Gio
gi.require_version('Gegl', '0.4')
from gi.repository import Gegl
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk
gi.require_version('Gdk', '3.0')
from gi.repository import Gdk
import time
import sys

def N_(message): return message
def _(message): return GLib.dgettext(None, message)


from math import pi, sin, cos, atan, atan2, fmod, radians, sqrt
import math
import time


def result_success():
    return procedure.new_return_values(Gimp.PDBStatusType.SUCCESS, GLib.Error())


PROC_NAME = "plug-in-spyrogimp"

two_pi, half_pi = 2 * pi, pi / 2
layer_name = _("Spyro Layer")
path_name = _("Spyro Path")

# "Enums"
GEAR_NOTATION, TOY_KIT_NOTATION, VISUAL_NOTATION = range(3)       # Pattern notations

RESPONSE_REDRAW, RESPONSE_RESET_PARAMS = range(2)   # Button responses in dialog.

# Save options of the dialog
SAVE_AS_NEW_LAYER, SAVE_BY_REDRAW, SAVE_AS_PATH = range(3)
save_options = [
    _("As New Layer"),
    _("Redraw on last active layer"),
    _("As Path")
]

# Mapping of pattern notation to the corresponding tab in the pattern notation notebook.
pattern_notation_page = {}

ring_teeth = [96, 144, 105, 150]

# Moving gear. Each gear is a pair of (#teeth, #holes)
# Hole #1 is closest to the edge of the wheel.
# The last hole is closest to the center.
wheel = [
    (24, 5), (30, 8), (32, 9), (36, 11), (40, 13), (42, 14), (45, 16),
    (48, 17), (50, 18), (52, 19), (56, 21), (60, 23), (63, 25), (64, 25),
    (72, 29), (75, 31), (80, 33), (84, 35)
]
wheel_teeth = [wh[0] for wh in wheel]


def lcm(a, b):
    """ Least common multiplier """
    return a * b // math.gcd(a, b)

### Shapes


class CanRotateShape:
    pass


class Shape:
    def configure(self, img, pp, cp):
        self.image, self.pp, self.cp = img, pp, cp

    def can_equal_w_h(self):
        return True

    def has_sides(self):
        return isinstance(self, SidedShape)

    def can_rotate(self):
        return isinstance(self, CanRotateShape)

    def can_morph(self):
        return self.has_sides()


class CircleShape(Shape):
    name = _("Circle")

    def get_center_of_moving_gear(self, oangle, dist=None):
        """
        :return: x,y - position where the center of the moving gear should be,
                     after going over oangle/two_pi of a full cycle over the outer gear.
        """
        cp = self.cp
        if dist is None:
            dist = cp.moving_gear_radius

        return (cp.x_center + (cp.x_half_size - dist) * cos(oangle),
                cp.y_center + (cp.y_half_size - dist) * sin(oangle))


class SidedShape(CanRotateShape, Shape):

    def configure(self, img, pp, cp):
        Shape.configure(self, img, pp, cp)
        self.angle_of_each_side = two_pi / pp.sides
        self.half_angle = self.angle_of_each_side / 2.0
        self.cos_half_angle = cos(self.half_angle)

    def get_center_of_moving_gear(self, oangle, dist=None):
        if dist is None:
            dist = self.cp.moving_gear_radius
        shape_factor = self.get_shape_factor(oangle)
        return (
            self.cp.x_center +
            (self.cp.x_half_size - dist) * shape_factor * cos(oangle),
            self.cp.y_center +
            (self.cp.y_half_size - dist) * shape_factor * sin(oangle)
        )


class PolygonShape(SidedShape):
    name = _("Polygon-Star")

    def get_shape_factor(self, oangle):
        oangle_mod = fmod(oangle + self.cp.shape_rotation_radians, self.angle_of_each_side)
        if oangle_mod > self.half_angle:
            oangle_mod = self.angle_of_each_side - oangle_mod

        # When oangle_mod = 0, the shape_factor will be cos(half_angle)) - which is the minimal shape_factor.
        # When oangle_mod is near the half_angle, the shape_factor will near 1.
        shape_factor = self.cos_half_angle / cos(oangle_mod)
        shape_factor -= self.pp.morph * (1 - shape_factor) * (1 + (self.pp.sides - 3) * 2)
        return shape_factor


class SineShape(SidedShape):
    # Sine wave on a circle ring.
    name = _("Sine")

    def get_shape_factor(self, oangle):
        oangle_mod = fmod(oangle + self.cp.shape_rotation_radians, self.angle_of_each_side)
        oangle_stretched = oangle_mod * self.pp.sides
        return 1 - self.pp.morph * (cos(oangle_stretched) + 1)


class BumpShape(SidedShape):
    # Semi-circles, based on a polygon
    name = _("Bumps")

    def get_shape_factor(self, oangle):
        oangle_mod = fmod(oangle + self.cp.shape_rotation_radians, self.angle_of_each_side)
        # Stretch back to angle between 0 and pi
        oangle_stretched = oangle_mod/2.0 * self.pp.sides

        # Compute factor for polygon.
        poly_angle = oangle_mod
        if poly_angle > self.half_angle:
            poly_angle = self.angle_of_each_side - poly_angle
        # When poly_oangle = 0, the shape_factor will be cos(half_angle)) - the minimal shape_factor.
        # When poly_angle is near the half_angle, the shape_factor will near 1.
        polygon_factor = self.cos_half_angle / cos(poly_angle)

        # Bump
        return polygon_factor - self.pp.morph * (1 - abs(cos(oangle_stretched)))


class ShapePart(object):
    def set_bounds(self, start, end):
        self.bound_start, self.bound_end = start, end
        self.bound_diff = self.bound_end - self.bound_start


class StraightPart(ShapePart):

    def __init__(self, teeth, perp_direction, x1, y1, x2, y2):
        self.teeth, self.perp_direction = max(teeth, 1),  perp_direction
        self.x1, self.y1, self.x2, self.y2 = x1, y1, x2, y2
        self.x_diff = self.x2 - self.x1
        self.y_diff = self.y2 - self.y1

        angle =  atan2(self.y_diff, self.x_diff) # - shape_rotation_radians
        perp_angle = angle + perp_direction * half_pi
        self.sin_angle = sin(perp_angle)
        self.cos_angle = cos(perp_angle)

    def perpendicular_at_oangle(self, oangle, perp_distance):
        factor = (oangle - self.bound_start) / self.bound_diff
        return (self.x1 + factor * self.x_diff + perp_distance * self.cos_angle,
                self.y1 + factor * self.y_diff + perp_distance * self.sin_angle)


class RoundPart(ShapePart):

    def __init__(self, teeth, x, y, start_angle, end_angle):
        self.teeth = max(teeth, 1)
        self.start_angle, self.end_angle = start_angle, end_angle
        self.x, self.y = x, y

        self.diff_angle = self.end_angle - self.start_angle

    def perpendicular_at_oangle(self, oangle, perp_distance):
        angle = (
            self.start_angle +
            self.diff_angle * (oangle - self.bound_start) / self.bound_diff
        )
        return (self.x + perp_distance * cos(angle),
                self.y + perp_distance * sin(angle))


class ShapeParts(list):
    """ A list of shape parts. """

    def __init__(self):
        list.__init__(self)
        self.total_teeth = 0

    def finish(self):
        for part in self:
            self.total_teeth += part.teeth
        teeth = 0
        bound_end = 0.0
        for part in self:
            bound_start = bound_end
            teeth += part.teeth
            bound_end = teeth/float(self.total_teeth) * two_pi
            part.set_bounds(bound_start, bound_end)

    def perpendicular_at_oangle(self, oangle, perp_distance):
        for part in self:
            if oangle <= part.bound_end:
                return part.perpendicular_at_oangle(oangle, perp_distance)

        # We shouldn't reach here
        return 0.0, 0.0


class AbstractShapeFromParts(Shape):
    def __init__(self):
        self.parts = None

    def get_center_of_moving_gear(self, oangle, dist=None):
        """
        :param oangle: an angle in radians, between 0 and 2*pi
        :return: x,y - position where the center of the moving gear should be,
                     after going over oangle/two_pi of a full cycle over the outer gear.
        """
        if dist is None:
            dist = self.cp.moving_gear_radius
        return self.parts.perpendicular_at_oangle(oangle, dist)


class RackShape(CanRotateShape, AbstractShapeFromParts):
    name = _("Rack")

    def configure(self, img, pp, cp):
        Shape.configure(self, img, pp, cp)

        round_teeth = 12
        side_teeth = (cp.fixed_gear_teeth - 2 * round_teeth) / 2

        # Determine start and end points of rack.

        cos_rot = cos(cp.shape_rotation_radians)
        sin_rot = sin(cp.shape_rotation_radians)

        x_size = cp.x2 - cp.x1 - cp.moving_gear_radius * 4
        y_size = cp.y2 - cp.y1 - cp.moving_gear_radius * 4

        size = ((x_size * cos_rot)**2 + (y_size * sin_rot)**2) ** 0.5

        x1 = cp.x_center - size/2.0 * cos_rot
        y1 = cp.y_center - size/2.0 * sin_rot
        x2 = cp.x_center + size/2.0 * cos_rot
        y2 = cp.y_center + size/2.0 * sin_rot

        # Build shape from shape parts.
        self.parts = ShapeParts()
        self.parts.append(StraightPart(side_teeth, -1, x2, y2, x1, y1))
        self.parts.append(
            RoundPart(
                round_teeth, x1, y1,
                half_pi + cp.shape_rotation_radians,
                3 * half_pi + cp.shape_rotation_radians
            )
        )
        self.parts.append(StraightPart(side_teeth, -1, x1, y1, x2, y2))
        self.parts.append(
            RoundPart(
                round_teeth, x2, y2,
                3 * half_pi + cp.shape_rotation_radians,
                5 * half_pi + cp.shape_rotation_radians)
        )
        self.parts.finish()


class FrameShape(AbstractShapeFromParts):
    name = _("Frame")

    def configure(self, img, pp, cp):
        Shape.configure(self, img, pp, cp)

        x1, x2 = cp.x1 + cp.moving_gear_radius, cp.x2 - cp.moving_gear_radius
        y1, y2 = cp.y1 + cp.moving_gear_radius, cp.y2 - cp.moving_gear_radius
        x_diff, y_diff = abs(x2 - x1), abs(y2 - y1)

        # Build shape from shape parts.
        self.parts = ShapeParts()
        self.parts.append(StraightPart(x_diff, 1, x2, cp.y2, x1, cp.y2))
        self.parts.append(StraightPart(y_diff, 1, cp.x1, y2, cp.x1, y1))
        self.parts.append(StraightPart(x_diff, 1, x1, cp.y1, x2, cp.y1))
        self.parts.append(StraightPart(y_diff, 1, cp.x2, y1, cp.x2, y2))
        self.parts.finish()


# Naive hash that gets only a limited amount of points from the selection.
# We use this hash to detect whether the selection has changed or not.
def naive_hash(img):
    selection = img.get_selection()

    # Get bounds of selection.
    flag, non_empty, x1, y1, x2, y2 = selection.bounds(img)

    # We want to compute a hash of the selection, but getting all the points in the selection
    # will take too long. We will get at most 25 points in each axis, so at most 25**2 points.
    step_x = 1 if (x2 - x1) <= 25 else (x2 - x1) // 25 + 1
    step_y = 1 if (y2 - y1) <= 25 else (y2 - y1) // 25 + 1
    hash = x1 * y1 + x2 * y2

    for x in range(x1, x2, step_x):
        for y in range(y1, y2, step_y):
            color = selection.get_pixel(x, y)
            hash += color.get_rgba()[0] * x * y
    return hash


class SelectionToPath:
    """ Converts a selection to a path """

    def __init__(self, image):
        self.image = image

        # Compute hash of selection, so we can detect when it was modified.
        self.last_selection_hash = self.compute_selection_hash()

        self.convert_selection_to_path()

    def convert_selection_to_path(self):

        if Gimp.Selection.is_empty(self.image):
            selection_was_empty = True
            Gimp.Selection.all(self.image)
        else:
            selection_was_empty = False

        proc   = Gimp.get_pdb().lookup_procedure('plug-in-sel2path')
        config = proc.create_config()
        config.set_property('image', self.image)
        result = proc.run(config)

        self.path = self.image.get_vectors()[0]
        self.stroke_ids = self.path.get_strokes()

        # A path may contain several strokes. If so lets throw away a stroke that
        # simply describes the borders of the image, if one exists.
        if len(self.stroke_ids) > 1:
            # Lets compute what a stroke of the image borders should look like.
            w, h = float(self.image.get_width()), float(self.image.get_height())
            frame_strokes = [0.0] * 6 + [0.0, h] * 3 + [w, h] * 3 + [w, 0.0] * 3

            for stroke in range(len(self.stroke_ids)):
                stroke_id = self.stroke_ids[stroke]
                vectors_stroke_type, control_points, closed = self.path.stroke_get_points(stroke_id)
                if control_points == frame_strokes:
                    del self.stroke_ids[stroke]
                    break

        self.set_current_stroke(0)

        if selection_was_empty:
            # Restore empty selection if it was empty.
            Gimp.Selection.none(self.image)

    def compute_selection_hash(self):
        return naive_hash(self.image)
        # In gimp 2 we used this:
        #px = self.image.get_selection().  get_pixel_rgn(0, 0, self.image.get_width(), self.image.get_height())
        #return px[0:self.image.get_width(), 0:self.image.get_height()].__hash__()

    def regenerate_path_if_selection_changed(self):
        current_selection_hash = self.compute_selection_hash()
        if self.last_selection_hash != current_selection_hash:
            self.last_selection_hash = current_selection_hash
            self.convert_selection_to_path()

    def get_num_strokes(self):
        return len(self.stroke_ids)

    def set_current_stroke(self, stroke_id=0):
        # Compute path length.
        self.path_length = self.path.stroke_get_length(self.stroke_ids[stroke_id], 1.0)
        self.current_stroke = stroke_id

    def point_at_angle(self, oangle):
        oangle_mod = fmod(oangle, two_pi)
        dist = self.path_length * oangle_mod / two_pi
        return self.path.stroke_get_point_at_dist(self.stroke_ids[self.current_stroke], dist, 1.0)


class SelectionShape(Shape):
    name = _("Selection")

    def __init__(self):
        self.path = None

    def process_selection(self, img):
        if self.path is None:
            self.path = SelectionToPath(img)
        else:
            self.path.regenerate_path_if_selection_changed()

    def configure(self, img, pp, cp):
        """ Set bounds of pattern """
        Shape.configure(self, img, pp, cp)
        self.drawing_no = cp.current_drawing
        self.path.set_current_stroke(self.drawing_no)

    def get_num_drawings(self):
        return self.path.get_num_strokes()

    def can_equal_w_h(self):
        return False

    def get_center_of_moving_gear(self, oangle, dist=None):
        """
        :param oangle: an angle in radians, between 0 and 2*pi
        :return: x,y - position where the center of the moving gear should be,
                     after going over oangle/two_pi of a full cycle over the outer gear.
        """
        cp = self.cp
        if dist is None:
            dist = cp.moving_gear_radius
        another_bool, x, y, slope, valid = self.path.point_at_angle(oangle)
        slope_angle = atan(slope)
        # We want to find an angle perpendicular to the slope, but in which direction?
        # Lets try both sides and see which of them is inside the selection.
        perpendicular_p, perpendicular_m = slope_angle + half_pi, slope_angle - half_pi
        step_size = 2   # The distance we are going to go in the direction of each angle.
        xp, yp = x + step_size * cos(perpendicular_p), y + step_size * sin(perpendicular_p)
        value_plus = Gimp.Selection.value(self.image, xp, yp)
        xp, yp = x + step_size * cos(perpendicular_m), y + step_size * sin(perpendicular_m)
        value_minus = Gimp.Selection.value(self.image, xp, yp)

        perpendicular = perpendicular_p if value_plus > value_minus else perpendicular_m
        return x + dist * cos(perpendicular), y + dist * sin(perpendicular)


shapes = {
    "circle":        CircleShape(),
    "rack":          RackShape(),
    "frame":         FrameShape(),
    "selection":     SelectionShape(),
    "polygon-star":  PolygonShape(),
    "sine":          SineShape(),
    "bumps":         BumpShape()
}


### Tools


def get_gradient_samples(num_samples):
    gradient = Gimp.context_get_gradient()
    reverse_mode = Gimp.context_get_gradient_reverse()
    repeat_mode = Gimp.context_get_gradient_repeat_mode()

    if repeat_mode == Gimp.RepeatMode.TRIANGULAR:
        # Get two uniform samples, which are reversed from each other, and connect them.

        sample_count = num_samples/2 + 1
        success, color_samples  = gradient.get_uniform_samples(sample_count, reverse_mode)

        del color_samples[-4:]   # Delete last color because it will appear in the next sample

        # If num_samples is odd, lets get an extra sample this time.
        if num_samples % 2 == 1:
            sample_count += 1

        success, color_samples2 = gradient.get_uniform_samples(sample_count, 1 - reverse_mode)

        del color_samples2[-4:]  # Delete last color because it will appear in the very first sample

        color_samples = tuple(color_samples)
    else:
        success, color_samples = gradient.get_uniform_samples(num_samples, reverse_mode)

    return color_samples


class PencilTool():
    name = _("Pencil")
    can_color = True

    def draw(self, layer, strokes, color=None):
        if color:
            Gimp.context_push()
            Gimp.context_enable_dynamics(False)
            Gimp.context_set_foreground(color)

        Gimp.pencil(layer, strokes)

        if color:
            Gimp.context_pop()


class AirBrushTool():
    name = _("AirBrush")
    can_color = True

    def draw(self, layer, strokes, color=None):
        if color:
            Gimp.context_push()
            Gimp.context_enable_dynamics(False)
            Gimp.context_set_foreground(color)

        Gimp.airbrush_default(layer, strokes)

        if color:
            Gimp.context_pop()


class AbstractStrokeTool():

    def draw(self, layer, strokes, color=None):
        # We need to multiply every point by 3, because we are creating a path,
        #  where each point has two additional control points.
        control_points = []
        for i, k in zip(strokes[0::2], strokes[1::2]):
            control_points += [i, k] * 3

        # Create path
        path = Gimp.Vectors.new(layer.get_image(), 'temp_path')
        layer.get_image().insert_vectors(path, None, 0)
        sid = path.stroke_new_from_points(Gimp.VectorsStrokeType.BEZIER,
                                          control_points, False)

        # Draw it.

        Gimp.context_push()

        # Call template method to set the kind of stroke to draw.
        self.prepare_stroke_context(color)

        layer.edit_stroke_item(path)
        Gimp.context_pop()

        # Get rid of the path.
        layer.get_image().remove_vectors(path)


# Drawing tool that should be quick, for purposes of previewing the pattern.
class PreviewTool:

    # Implementation using pencil.  (A previous implementation using stroke was slower, and thus removed).
    def draw(self, layer, strokes, color=None):
        foreground = Gimp.context_get_foreground()
        Gimp.context_push()
        Gimp.context_set_defaults()
        Gimp.context_set_foreground(foreground)
        Gimp.context_enable_dynamics(False)

        # TODO: in the future, there could be several brushes with this name. We
        # should have methods to loop through all of them and to know whether
        # they are default/system brushes or not.
        brush = Gimp.Brush.get_by_name('1. Pixel')
        assert brush is not None

        Gimp.context_set_brush(brush)
        Gimp.context_set_brush_size(1.0)
        Gimp.context_set_brush_spacing(3.0)
        Gimp.pencil(layer, strokes)
        Gimp.context_pop()

    name = _("Preview")
    can_color = False


class StrokeTool(AbstractStrokeTool):
    name = _("Stroke")
    can_color = True

    def prepare_stroke_context(self, color):
        if color:
            Gimp.context_enable_dynamics(False)
            Gimp.context_set_foreground(color)

        Gimp.context_set_stroke_method(Gimp.StrokeMethod.LINE)


class StrokePaintTool(AbstractStrokeTool):
    def __init__(self, name, paint_method, can_color=True):
        self.name = name
        self.paint_method = paint_method
        self.can_color = can_color

    def prepare_stroke_context(self, color):
        if self.can_color and color is not None:
            Gimp.context_enable_dynamics(False)
            Gimp.context_set_foreground(color)

        Gimp.context_set_stroke_method(Gimp.StrokeMethod.PAINT_METHOD)
        Gimp.context_set_paint_method(self.paint_method)


class SaveToPathTool():
    """ This tool cannot be chosen by the user from the tools menu.
        We dont add this to the list of tools. """

    def __init__(self, img):
        self.path = Gimp.Vectors.new(img, path_name)
        img.insert_vectors(self.path, None, 0)

    def draw(self, layer, strokes, color=None):
        # We need to multiply every point by 3, because we are creating a path,
        #  where each point has two additional control points.
        control_points = []
        for i, k in zip(strokes[0::2], strokes[1::2]):
            control_points += [i, k] * 3

        self.path.stroke_new_from_points(Gimp.VectorsStrokeType.BEZIER,
                                         control_points, False)


tools = {
    "preview":       PreviewTool(),
    "paintbrush":    StrokePaintTool(_("PaintBrush"), "gimp-paintbrush"),
    "penciltool":    PencilTool(),
    "airbrush":      AirBrushTool(),
    "stroke":        StrokeTool(),
    "ink":           StrokePaintTool(_("Ink"), 'gimp-ink'),
    "mypaintbrush":  StrokePaintTool(_("MyPaintBrush"), 'gimp-mybrush')
    # Clone does not work properly when an image is not set.  When that happens, drawing fails, and
    # I am unable to catch the error. This causes the plugin to crash, and subsequent problems with undo.
    # StrokePaintTool("Clone", 'gimp-clone', False)
}


class PatternParameters:
    """
    All the parameters that define a pattern live in objects of this class.
    If you serialize and saved this class, you should reproduce
    the pattern that the plugin would draw.
    """
    def __init__(self):
        if not hasattr(self, 'curve_type'):
            self.curve_type = "spyrograph"

        # Pattern
        if not hasattr(self, 'pattern_notation'):
            self.pattern_notation = 0
        if not hasattr(self, 'outer_teeth'):
            self.outer_teeth = 96
        if not hasattr(self, 'inner_teeth'):
            self.inner_teeth = 36
        if not hasattr(self, 'pattern_rotation'):
            self.pattern_rotation = 0
        # Location of hole as a percent of the radius of the inner gear - runs between 0 and 100.
        # A value of 0 means, the hole is at the center of the wheel, which would produce a boring circle.
        # A value of 100 means the edge of the wheel.
        if not hasattr(self, 'hole_percent'):
            self.hole_percent = 100.0

        # Toy Kit parameters
        # Hole number in Toy Kit notation. Hole #1 is at the edge of the wheel, and the last hole is
        # near the center of the wheel, but not exactly at the center.
        if not hasattr(self, 'hole_number'):
            self.hole_number = 1
        if not hasattr(self, 'kit_fixed_gear_index'):
            self.kit_fixed_gear_index = 1
        if not hasattr(self, 'kit_moving_gear_index'):
            self.kit_moving_gear_index = 1

        # Visual notation parameters
        if not hasattr(self, 'petals'):
            self.petals = 5
        if not hasattr(self, 'petal_skip'):
            self.petal_skip = 2
        if not hasattr(self, 'doughnut_hole'):
            self.doughnut_hole = 50.0
        if not hasattr(self, 'doughnut_width'):
            self.doughnut_width = 50.0

        # Shape
        if not hasattr(self, 'shape_index'):
            self.shape_index = "circle"  # Index in the shapes array
        if not hasattr(self, 'sides'):
            self.sides = 5
        if not hasattr(self, 'morph'):
            self.morph = 0.5
        if not hasattr(self, 'shape_rotation'):
            self.shape_rotation = 0

        if not hasattr(self, 'equal_w_h'):
            self.equal_w_h = False
        if not hasattr(self, 'margin_pixels'):
            self.margin_pixels = 0  # Distance between the drawn shape, and the selection borders.

        # Drawing style
        if not hasattr(self, 'tool_index'):
            self.tool_index = "preview"   # Index in the tools array.
        if not hasattr(self, 'long_gradient'):
            self.long_gradient = False

        if not hasattr(self, 'save_option'):
            self.save_option = SAVE_AS_NEW_LAYER

    def kit_max_hole_number(self):
        return wheel[self.kit_moving_gear_index][1]


# Handle shelving of plugin parameters

def unshelf_parameters():
    # TODO: we'd usually use Gimp.PDB.set_data() but this won't work on
    # introspection bindings. We will need to work on this.
    #if shelf.has_key("p"):
        #parameters = shelf["p"]
        #parameters.__init__()  # Fill in missing values with defaults.
        #return parameters
    return PatternParameters()


def shelf_parameters(pp):
    # TODO: see unshelf_parameters() which explains why we can't use
    # Gimp.PDB.get_data().
    pass
    #shelf["p"] = pp


class ComputedParameters:
    """
    Stores computations performed on a PatternParameters object.
    The results of these computations are used to perform the drawing.
    Having all these computations in one place makes it convenient to pass
    around as a parameter.

    If the pattern parameters should result in multiple patterns to be drawn, the
    compute parameters also stores which one is currently being drawn.
    """
    def __init__(self, pp, img):

        def compute_gradients():
            self.use_gradient = self.pp.long_gradient and tools[self.pp.tool_index].can_color

            # If gradient is used, determine how the lines are two be split to different colors.
            if self.use_gradient:
                # We want to use enough samples to be beautiful, but not too many, that would
                # force us to make many separate calls for drawing the pattern.
                if self.rotations > 30:
                    self.chunk_num = self.rotations
                    self.chunk_size_lines = self.fixed_gear_teeth
                else:
                    # Lets try to find a chunk size, such that it divides num_lines, and we get at least 30 chunks.
                    # In the worse case, we will just use "1"
                    for chunk_size in range(self.fixed_gear_teeth - 1, 0, -1):
                        if self.num_lines % chunk_size == 0:
                            if self.num_lines / chunk_size > 30:
                                break

                    self.chunk_num = self.num_lines / chunk_size
                    self.chunk_size_lines = chunk_size

                self.gradients = get_gradient_samples(self.chunk_num)
            else:
                self.chunk_num, self.chunk_size_lines = None, None

        def compute_sizes():
            # Get rid of the margins.
            self.x1 = x1 + pp.margin_pixels
            self.y1 = y1 + pp.margin_pixels
            self.x2 = x2 - pp.margin_pixels
            self.y2 = y2 - pp.margin_pixels

            # Compute size and position of the pattern
            self.x_half_size, self.y_half_size = (self.x2 - self.x1) / 2, (self.y2 - self.y1) / 2
            self.x_center, self.y_center = (self.x1 + self.x2) / 2.0, (self.y1 + self.y2) / 2.0

            if pp.equal_w_h:
                if self.x_half_size < self.y_half_size:
                    self.y_half_size = self.x_half_size
                    self.y1, self.y2 = self.y_center - self.y_half_size, self.y_center + self.y_half_size
                elif self.x_half_size > self.y_half_size:
                    self.x_half_size = self.y_half_size
                    self.x1, self.x2 = self.x_center - self.x_half_size, self.x_center + self.x_half_size

            # Find the distance between the hole and the center of the inner circle.
            # To do this, we compute the size of the gears, by the number of teeth.
            # The circumference of the outer ring is 2 * pi * outer_R = #fixed_gear_teeth * tooth size.
            outer_R = min(self.x_half_size, self.y_half_size)
            if self.pp.pattern_notation == VISUAL_NOTATION:
                doughnut_width = self.pp.doughnut_width
                if doughnut_width + self.pp.doughnut_hole > 100:
                    doughnut_width = 100.0 - self.pp.doughnut_hole

                # Let R, r be the radius of fixed and moving gear, and let hp be the hole percent.
                # Let dwp, dhp be the doughnut width and hole in percents of R.
                # The two sides of the following equation calculate how to reach the center of the moving
                # gear from the center of the fixed gear:
                # I)     R * (dhp/100 + dwp/100/2) = R - r
                # The following equation expresses which r and hp would generate a doughnut of width dw.
                # II)    R * dw/100 = 2 * r * hp/100
                # We solve the two above equations to calculate hp and r:
                self.hole_percent = doughnut_width / (2.0 * (1 - (self.pp.doughnut_hole + doughnut_width / 2.0) / 100.0))
                self.moving_gear_radius = outer_R * doughnut_width / (2 * self.hole_percent)
            else:
                size_of_tooth_in_pixels = two_pi * outer_R / self.fixed_gear_teeth
                self.moving_gear_radius = size_of_tooth_in_pixels * self.moving_gear_teeth / two_pi

            self.hole_dist_from_center = self.hole_percent / 100.0 * self.moving_gear_radius

        self.pp = pp

        # Check if the shape is made of multiple shapes, as in using Selection as fixed gear.
        if (
            isinstance(shapes[self.pp.shape_index], SelectionShape) and
            curve_types[self.pp.curve_type].supports_shapes()
        ):
            shapes[self.pp.shape_index].process_selection(img)
            Gimp.displays_flush()
            self.num_drawings = shapes[self.pp.shape_index].get_num_drawings()
        else:
            self.num_drawings = 1
        self.current_drawing = 0

        # Get bounds. We don't care weather a selection exists or not.
        success, exists, x1, y1, x2, y2 = Gimp.Selection.bounds(img)

        # Combine different ways to specify patterns, into a unified set of computed parameters.
        self.num_notation_drawings = 1
        self.current_notation_drawing = 0
        if self.pp.pattern_notation == GEAR_NOTATION:
            self.fixed_gear_teeth = int(round(pp.outer_teeth))
            self.moving_gear_teeth = int(round(pp.inner_teeth))
            self.petals = self.num_petals()
            self.hole_percent = pp.hole_percent
        elif self.pp.pattern_notation == TOY_KIT_NOTATION:
            self.fixed_gear_teeth = ring_teeth[pp.kit_fixed_gear_index]
            self.moving_gear_teeth = wheel[pp.kit_moving_gear_index][0]
            self.petals = self.num_petals()
            # We want to map hole #1 to 100% and hole of max_hole_number to 2.5%
            # We don't want 0% because that would be the exact center of the moving gear,
            # and that would create a boring pattern.
            max_hole_number = wheel[pp.kit_moving_gear_index][1]
            self.hole_percent = (max_hole_number - pp.hole_number) / float(max_hole_number - 1) * 97.5 + 2.5
        elif self.pp.pattern_notation == VISUAL_NOTATION:
            self.petals = pp.petals
            self.fixed_gear_teeth = pp.petals
            self.moving_gear_teeth = pp.petals - pp.petal_skip
            if self.moving_gear_teeth < 20:
                self.fixed_gear_teeth *= 10
                self.moving_gear_teeth *= 10
            self.hole_percent = 100.0
            self.num_notation_drawings = math.gcd(pp.petals, pp.petal_skip)
            self.notation_drawings_rotation = two_pi / pp.petals

        # Rotations
        self.shape_rotation_radians = self.radians_from_degrees(pp.shape_rotation)
        self.pattern_rotation_start_radians = self.radians_from_degrees(pp.pattern_rotation)
        self.pattern_rotation_radians = self.pattern_rotation_start_radians
        # Additional fixed pattern rotation for lissajous.
        self.lissajous_rotation = two_pi / self.petals / 4.0

        # Compute the total number of teeth we have to go over.
        # Another way to view it is the total of lines we are going to draw.
        # To find this we compute the Least Common Multiplier.
        self.num_lines = lcm(self.fixed_gear_teeth, self.moving_gear_teeth)
        # The number of points we are going to compute. This is the number of lines, plus 1, because to draw
        # a line we need two points.
        self.num_points = self.num_lines + 1

        # Compute gradients.

        # The number or rotations needed in order to complete the pattern.
        # Each rotation has cp.fixed_gear_teeth points + 1 points.
        self.rotations = self.num_lines / self.fixed_gear_teeth

        compute_gradients()

        # Computations needed for the actual drawing of the patterns - how much should we advance each angle
        # in each step of the computation.

        # How many radians is each tooth of outer gear.  This is also the amount that we
        # will step in the iterations that generate the points of the pattern.
        self.oangle_factor = two_pi / self.fixed_gear_teeth
        # How many radians should the moving gear be moved, for each tooth of the fixed gear
        angle_factor = curve_types[pp.curve_type].get_angle_factor(self)
        self.iangle_factor = self.oangle_factor * angle_factor

        compute_sizes()

    def num_petals(self):
        """ The number of 'petals' (or points) that will be produced by a spirograph drawing. """
        return lcm(self.fixed_gear_teeth, self.moving_gear_teeth) / self.moving_gear_teeth

    def radians_from_degrees(self, degrees):
        positive_degrees = degrees if degrees >= 0 else degrees + 360
        return radians(positive_degrees)

    def get_color(self, n):
        colors = self.gradients[4*n:4*(n+1)]
        color = Gegl.Color.new("black")
        color.set_rgba(colors[0], colors[1], colors[2], colors[3])
        return color

    def next_drawing(self):
        """ Multiple drawings can be drawn either when the selection is used as a fixed
            gear, and/or the visual tab is used, which causes multiple drawings
            to be drawn at different rotations. """
        if self.current_notation_drawing < self.num_notation_drawings - 1:
            self.current_notation_drawing += 1
            self.pattern_rotation_radians = self.pattern_rotation_start_radians + (
                    self.current_notation_drawing * self.notation_drawings_rotation)
        else:
            self.current_drawing += 1
            self.current_notation_drawing = 0
            self.pattern_rotation_radians = self.pattern_rotation_start_radians

    def has_more_drawings(self):
        return (self.current_notation_drawing < self.num_notation_drawings - 1 or
                self.current_drawing < self.num_drawings - 1)

### Curve types


class CurveType:

    def supports_shapes(self):
        return True


class RouletteCurveType(CurveType):

    def get_strokes(self, p, cp):
        strokes = []
        for curr_tooth in range(cp.num_points):
            iangle = fmod(curr_tooth * cp.iangle_factor + cp.pattern_rotation_radians, two_pi)
            oangle = fmod(curr_tooth * cp.oangle_factor + cp.pattern_rotation_radians, two_pi)

            x, y = shapes[p.shape_index].get_center_of_moving_gear(oangle)
            strokes.append(x + cp.hole_dist_from_center * cos(iangle))
            strokes.append(y + cp.hole_dist_from_center * sin(iangle))

        return strokes


class SpyroCurveType(RouletteCurveType):
    name = _("Spyrograph")

    def get_angle_factor(self, cp):
        return - (cp.fixed_gear_teeth - cp.moving_gear_teeth) / float(cp.moving_gear_teeth)


class EpitrochoidCurvetype(RouletteCurveType):
    name = _("Epitrochoid")

    def get_angle_factor(self, cp):
        return (cp.fixed_gear_teeth + cp.moving_gear_teeth) / float(cp.moving_gear_teeth)


class SineCurveType(CurveType):
    name = _("Sine")

    def get_angle_factor(self, cp):
        return cp.fixed_gear_teeth / float(cp.moving_gear_teeth)

    def get_strokes(self, p, cp):
        strokes = []
        for curr_tooth in range(cp.num_points):
            iangle = curr_tooth * cp.iangle_factor
            oangle = fmod(curr_tooth * cp.oangle_factor + cp.pattern_rotation_radians, two_pi)

            dist = cp.moving_gear_radius + sin(iangle) * cp.hole_dist_from_center
            x, y = shapes[p.shape_index].get_center_of_moving_gear(oangle, dist)
            strokes.append(x)
            strokes.append(y)

        return strokes


class LissaCurveType:
    name = _("Lissajous")

    def get_angle_factor(self, cp):
        return cp.fixed_gear_teeth / float(cp.moving_gear_teeth)

    def get_strokes(self, p, cp):
        strokes = []
        for curr_tooth in range(cp.num_points):
            iangle = curr_tooth * cp.iangle_factor
            # Adding the cp.lissajous_rotation rotation makes the pattern have the same number of curves
            # as the other curve types. Without it, many lissajous patterns would redraw the same lines twice,
            # and thus look less dense than the other curves.
            oangle = fmod(curr_tooth * cp.oangle_factor + cp.pattern_rotation_radians + cp.lissajous_rotation, two_pi)

            strokes.append(cp.x_center + cp.x_half_size * cos(oangle))
            strokes.append(cp.y_center + cp.y_half_size * cos(iangle))

        return strokes

    def supports_shapes(self):
        return False


curve_types = {
  "spyrograph":   SpyroCurveType(),
  "epitrochoid": EpitrochoidCurvetype(),
  "sine":         SineCurveType(),
  "lissajous":    LissaCurveType()
}

# Drawing engine. Also implements drawing incrementally.
# We don't draw the entire stroke, because it could take several seconds,
# Instead, we break it into chunks.  Incremental drawing is also used for drawing gradients.
class DrawingEngine:

    def __init__(self, img, p):
        self.img, self.p = img, p
        self.cp = None

        # For incremental drawing
        self.strokes = []
        self.start = 0
        self.chunk_size_lines = 600
        self.chunk_no = 0
        # We are aiming for the drawing time of a chunk to be no longer than max_time.
        self.max_time_sec = 0.1

        self.dynamic_chunk_size = True

    def pre_draw(self):
        """ Needs to be called before starting to draw a pattern. """
        self.cp = ComputedParameters(self.p, self.img)

    def draw_full(self, layer):
        """ Non incremental drawing. """

        self.pre_draw()
        self.img.undo_group_start()

        while True:
            self.set_strokes()

            if self.cp.use_gradient:
                while self.has_more_strokes():
                    self.draw_next_chunk(layer, fetch_next_drawing=False)
            else:
                tools[self.p.tool_index].draw(layer, self.strokes)

            if self.cp.has_more_drawings():
                self.cp.next_drawing()
            else:
                break

        self.img.undo_group_end()

        Gimp.displays_flush()

    # Methods for incremental drawing.

    def draw_next_chunk(self, layer, fetch_next_drawing=True, tool=None):
        stroke_chunk, color = self.next_chunk(fetch_next_drawing)
        if not tool:
            tool = tools[self.p.tool_index]
        tool.draw(layer, stroke_chunk, color)
        return len(stroke_chunk)

    def set_strokes(self):
        """ Compute the strokes of the current pattern. The heart of the plugin. """

        shapes[self.p.shape_index].configure(self.img, self.p, self.cp)

        self.strokes = curve_types[self.p.curve_type].get_strokes(self.p, self.cp)

        self.start = 0
        self.chunk_no = 0

        if self.cp.use_gradient:
            self.chunk_size_lines = self.cp.chunk_size_lines
            self.dynamic_chunk_size = False
        else:
            self.dynamic_chunk_size = True

    def reset_incremental(self):
        """ Setup incremental drawing to start drawing from scratch. """
        self.pre_draw()
        self.set_strokes()

    def next_chunk(self, fetch_next_drawing):

        # chunk_size_lines, is the number of lines we want to draw. We need 1 extra point to draw that.
        end = self.start + (self.chunk_size_lines + 1) * 2
        if end > len(self.strokes):
            end = len(self.strokes)
        result = self.strokes[self.start:end]
        # Promote the start to the last point. This is the start of the first line to draw next time.
        self.start = end - 2
        color = self.cp.get_color(self.chunk_no) if self.cp.use_gradient else None

        self.chunk_no += 1

        # If self.strokes has ended, lets fetch strokes for the next drawing.
        if fetch_next_drawing and not self.has_more_strokes():
            if self.cp.has_more_drawings():
                self.cp.next_drawing()
                self.set_strokes()

        return result, color

    def has_more_strokes(self):
        return self.start + 2 < len(self.strokes)

    # Used for displaying progress.
    def fraction_done(self):
        return (self.start + 2.0) / len(self.strokes)

    def report_time(self, time_sec):
        """
        Report the time it took, in seconds, to draw the last stroke chunk.
        This helps to determine the size of chunks to return in future calls of 'next_chunk',
        since we want the calls to be short, to not make the user interface feel stuck.
        """
        if time_sec != 0 and self.dynamic_chunk_size:
            self.chunk_size_lines = int(self.chunk_size_lines * self.max_time_sec / time_sec)
            # Don't let chunk size be too large or small.
            self.chunk_size_lines = max(10, self.chunk_size_lines)
            self.chunk_size_lines = min(1000, self.chunk_size_lines)


# Constants for DoughnutWidget

# Enum - When the mouse is pressed, which target value is being changed.
TARGET_NONE, TARGET_HOLE, TARGET_WIDTH = range(3)

CIRCLE_CENTER_X = 4
RIGHT_MARGIN = 2
TOTAL_MARGIN = CIRCLE_CENTER_X + RIGHT_MARGIN

# A widget for displaying and setting the pattern of a spirograph, using a "doughnut" as
# a visual metaphor.  This widget replaces two scale widgets.
class DoughnutWidget(Gtk.DrawingArea):
    __gtype_name__ = 'DoughnutWidget'

    def __init__(self, *args, **kwds):
        super().__init__(*args, **kwds)
        self.set_size_request(80, 40)
        self.set_margin_start(2)
        self.set_margin_end(2)
        self.set_margin_top(2)
        self.set_margin_bottom(2)

        self.add_events(
            Gdk.EventMask.BUTTON1_MOTION_MASK |
            Gdk.EventMask.BUTTON_PRESS_MASK |
            Gdk.EventMask.BUTTON_RELEASE_MASK |
            Gdk.EventMask.POINTER_MOTION_MASK
        )

        self.resize_cursor = Gdk.Cursor.new_for_display(self.get_display(),
                                 Gdk.CursorType.SB_H_DOUBLE_ARROW)

        self.button_pressed = False
        self.target = TARGET_NONE

        self.hole_radius = 30
        self.doughnut_width = 30

    def set_hole_radius(self, hole_radius):
        self.hole_radius = hole_radius
        self.queue_draw()

    def get_hole_radius(self):
        return self.hole_radius

    def set_width(self, width):
        self.doughnut_width = width
        self.queue_draw()

    def get_width(self):
        return self.doughnut_width

    def compute_doughnut(self):
        """ Compute the location of the doughnut circles.
            Returns (circle center x, circle center y, radius of inner circle, radius of outer circle) """
        allocation = self.get_allocation()
        alloc_width = allocation.width - TOTAL_MARGIN
        return (
            CIRCLE_CENTER_X, allocation.height/2,
            alloc_width * self.hole_radius/ 100.0,
            alloc_width * min(self.hole_radius + self.doughnut_width, 100.0)/ 100.0
        )

    def set_cursor_h_resize(self, flag):
        """ Set the mouse to be a double arrow, if flag is true.
            Otherwise, use the cursor of the parent window. """
        gdk_window = self.get_window()
        gdk_window.set_cursor(self.resize_cursor if flag else None)

    def get_target(self, x, y):
        # Find out if x, y is over one of the circle edges.

        center_x, center_y, hole_radius, outer_radius = self.compute_doughnut()

        # Compute distance from circle center to point
        dist = math.sqrt((center_x - x) ** 2 + (center_y - y) ** 2)

        if abs(dist - hole_radius) <= 3:
            return TARGET_HOLE
        if abs(dist - outer_radius) <= 3:
            return TARGET_WIDTH

        return TARGET_NONE

    def do_draw(self, cr):

        allocation = self.get_allocation()
        center_x, center_y, hole_radius, outer_radius = self.compute_doughnut()

        # Paint background
        Gtk.render_background(self.get_style_context(), cr,
                0, 0, allocation.width, allocation.height)

        fg_color = self.get_style_context().get_color(Gtk.StateFlags.NORMAL)

        # Draw doughnut interior
        arc = math.pi*3/2.0
        cr.set_source_rgba(fg_color.red, fg_color.green, fg_color.blue, fg_color.alpha/2);
        cr.arc(center_x, center_y, hole_radius, -arc, arc)
        cr.arc_negative(center_x, center_y, outer_radius, arc, -arc)
        cr.close_path()
        cr.fill()

        # Draw doughnut border.
        cr.set_source_rgba(*list(fg_color));
        cr.set_line_width(3)
        cr.arc_negative(center_x, center_y, outer_radius, arc, -arc)
        cr.stroke()
        if hole_radius < 1.0:
            # If the radius is too small, nothing will be drawn.
            # So draw a small cross marker instead.
            cr.set_line_width(2)
            cr.move_to(center_x-4, center_y)
            cr.line_to(center_x+4, center_y)
            cr.move_to(center_x, center_y-4)
            cr.line_to(center_x, center_y+4)
        else:
            cr.arc(center_x, center_y, hole_radius, -arc, arc)
        cr.stroke()

    def compute_new_radius(self, x):
        """ This method is called during mouse dragging of the widget.
            Compute the new radius based on
            the current x location of the mouse pointer. """
        allocation = self.get_allocation()

        # How much does a single pixel difference in x, change the radius?
        # Note that: allocation.width - TOTAL_MARGIN = 100 radius units,
        radius_per_pixel = 100.0 / (allocation.width - TOTAL_MARGIN)
        new_radius = self.start_radius + (x - self.start_x) * radius_per_pixel

        if self.target == TARGET_HOLE:
            self.hole_radius = max(min(new_radius, 99.0), 0.0)
        else:
            self.doughnut_width = max(min(new_radius, 100.0), 1.0)

        self.queue_draw()

    def do_button_press_event(self, event):
        self.button_pressed = True

        # If we clicked on one of the doughnut borders, remember which
        # border we clicked on, and setup variable to start dragging it.
        target = self.get_target(event.x, event.y)
        if target == TARGET_HOLE or target == TARGET_WIDTH:
            self.target = target
            self.start_x = event.x
            self.start_radius = (
                self.hole_radius if target == TARGET_HOLE else
                self.doughnut_width
            )

    def do_button_release_event(self, event):
        # If one the doughnut borders was being dragged, recompute doughnut size.
        if self.target != TARGET_NONE:
            self.compute_new_radius(event.x)
            # Clip the width, if it is too large to fit.
            if self.hole_radius + self.doughnut_width > 100:
                self.doughnut_width = 100 - self.hole_radius
            self.emit("values_changed", self.hole_radius, self.doughnut_width)

        self.button_pressed = False
        self.target = TARGET_NONE

    def do_motion_notify_event(self, event):
        if self.button_pressed:
            # We are dragging one of the doughnut borders; recompute its size.
            if self.target != TARGET_NONE:
                self.compute_new_radius(event.x)
        else:
            # Set cursor according to whether we are over one of the doughnut borders.
            target = self.get_target(event.x, event.y)
            self.set_cursor_h_resize(target != TARGET_NONE)

# Create signal that returns change parameters.
GObject.type_register(DoughnutWidget)
GObject.signal_new("values_changed", DoughnutWidget, GObject.SignalFlags.RUN_LAST,
                   GObject.TYPE_NONE, (GObject.TYPE_INT, GObject.TYPE_INT))


class SpyroWindow():

    class MyScale():
        """ Combintation of scale and spin that control the same adjuster. """
        def __init__(self, scale, spin):
            self.scale, self.spin = scale, spin

        def set_sensitive(self, val):
            self.scale.set_sensitive(val)
            self.spin.set_sensitive(val)

    def on_adj_changed(self, widget):
        self.adj_changed = True

    def on_adj_release(self, widget, event, callback):
        # Force update to accommodate manually typing numbers into the entry.
        if isinstance(widget, Gtk.SpinButton):
            widget.update()
        if self.adj_changed:
            callback(widget)
        self.adj_changed = False

    def __init__(self, img, layer):

        def add_horizontal_separator(vbox):
            hsep = Gtk.HSeparator()
            vbox.add(hsep)
            hsep.show()

        def add_vertical_space(vbox, height):
            hbox = Gtk.HBox()
            hbox.set_border_width(height/2)
            vbox.add(hbox)
            hbox.show()

        def add_to_box(box, w):
            box.add(w)
            w.show()

        def create_table(border_width):
            table = Gtk.Grid()
            table.set_column_homogeneous(False)
            table.set_border_width(border_width)
            table.set_column_spacing(10)
            table.set_row_spacing(10)
            return table

        def label_in_table(label_text, table, row, tooltip_text=None, col=0):
            """ Create a label and set it in first col of table. """
            label = Gtk.Label(label=label_text)
            label.set_xalign(0.0)
            label.set_yalign(0.5)
            if tooltip_text:
                label.set_tooltip_text(tooltip_text)
            table.attach(label, col, row, 1, 1)
            label.show()

        def spin_in_table(adj, table, row, callback, digits=0, col=0):
            spin = Gtk.SpinButton.new(adj, climb_rate=0.5, digits=digits)
            spin.set_numeric(True)
            spin.set_snap_to_ticks(True)
            spin.set_max_length(5)
            spin.set_width_chars(5)
            table.attach(spin, col, row, 1, 1)
            spin.show()
            adj.connect("value_changed", self.on_adj_changed)
            spin.connect("button_release_event", self.on_adj_release, callback)
            spin.connect("activate", self.on_adj_release, None, callback)
            spin.connect("focus_out_event", self.on_adj_release, callback)
            return spin

        def hscale_in_table(adj, table, row, callback, digits=0, col=1, cols=1):
            """ Create an hscale and a spinner using the same Adjustment, and set it in table. """
            scale = Gtk.Scale.new(Gtk.Orientation.HORIZONTAL, adj)
            scale.set_size_request(150, -1)
            scale.set_digits(digits)
            scale.set_hexpand(True)
            scale.set_halign(Gtk.Align.FILL)
            table.attach(scale, col, row, cols, 1)
            scale.show()

            spin = Gtk.SpinButton.new(adj, climb_rate=0.5, digits=digits)
            spin.set_numeric(True)
            spin.set_snap_to_ticks(True)
            spin.set_max_length(5)
            spin.set_width_chars(5)
            table.attach(spin, col + cols, row, 1, 1)
            spin.show()

            adj.connect("value_changed", self.on_adj_changed)
            scale.connect("button_release_event", self.on_adj_release, callback)
            spin.connect("button_release_event", self.on_adj_release, callback)
            spin.connect("activate", self.on_adj_release, None, callback)
            spin.connect("focus_out_event", self.on_adj_release, callback)

            return self.MyScale(scale, spin)

        def rotation_in_table(val, table, row, callback):
            adj = Gtk.Adjustment.new(val, -180.0, 180.0, 1.0, 10.0, 0.0)
            myscale = hscale_in_table(adj, table, row, callback, digits=1)
            myscale.scale.add_mark(0.0, Gtk.PositionType.BOTTOM, None)
            myscale.spin.set_max_length(6)
            myscale.spin.set_width_chars(6)
            return adj, myscale

        def set_combo_in_table(txt_list, is_dictionary, table, row, callback):
            combo = Gtk.ComboBoxText.new()
            if (is_dictionary == False):
                for txt in txt_list:
                    combo.append_text(_(txt))
            else:
                for key in txt_list:
                    combo.append (key, _(txt_list[key].name))

            combo.set_halign(Gtk.Align.FILL)
            table.attach(combo, 1, row, 1, 1)
            combo.show()
            combo.connect("changed", callback)
            return combo

        # Return table which is at the top of the dialog, and has several major input widgets.
        def top_table():

            # Add table for displaying attributes, each having a label and an input widget.
            table = create_table(10)

            # Curve type
            row = 0
            label_in_table(_("Curve Type"), table, row,
                           _("An Epitrochoid pattern is when the moving gear is on the outside of the fixed gear."))
            self.curve_type_combo = set_combo_in_table(curve_types, True, table, row,
                                                       self.curve_type_changed)

            row += 1
            label_in_table(_("Tool"), table, row,
                           _("The tool with which to draw the pattern. "
                             "The Preview tool just draws quickly."))
            self.tool_combo = set_combo_in_table(tools, True, table, row,
                                                 self.tool_combo_changed)

            self.long_gradient_checkbox = Gtk.CheckButton(label=_("Long Gradient"))
            self.long_gradient_checkbox.set_tooltip_text(
                _("When unchecked, the current tool settings will be used. "
                  "When checked, will use a long gradient to match the length of the pattern, "
                  "based on current gradient and repeat mode from the gradient tool settings.")
            )
            self.long_gradient_checkbox.set_border_width(0)
            table.attach(self.long_gradient_checkbox, 2, row, 1, 1)
            self.long_gradient_checkbox.show()
            self.long_gradient_checkbox.connect("toggled", self.long_gradient_changed)

            return table

        def pattern_notation_frame():

            vbox = Gtk.VBox(spacing=0, homogeneous=False)

            add_vertical_space(vbox, 14)

            hbox = Gtk.HBox(spacing=5)
            hbox.set_border_width(5)

            label = Gtk.Label(label=_("Specify pattern using one of the following tabs:"))
            label.set_tooltip_text(_(
                "The pattern is specified only by the active tab. Toy Kit is similar to Gears, "
                "but it uses gears and hole numbers which are found in toy kits. "
                "If you follow the instructions from the toy kit manuals, results should be similar."))
            hbox.pack_start(label, False, False, 0)
            label.show()

            alignment = Gtk.Alignment(xalign=0.0, yalign=0.0, xscale=0.0, yscale=0.0)
            alignment.add(hbox)
            hbox.show()
            vbox.add(alignment)
            alignment.show()

            self.pattern_notebook = Gtk.Notebook()
            self.pattern_notebook.set_border_width(10)
            self.pattern_notebook.connect('switch-page', self.pattern_notation_tab_changed)

            # "Gear" pattern notation.

            # Add table for displaying attributes, each having a label and an input widget.
            gear_table = create_table(5)

            # Teeth
            row = 0
            fixed_gear_tooltip = _(
                "Number of teeth of fixed gear. The size of the fixed gear is "
                "proportional to the number of teeth."
            )
            label_in_table(_("Fixed Gear Teeth"), gear_table, row, fixed_gear_tooltip)
            self.outer_teeth_adj = Gtk.Adjustment(value=self.p.outer_teeth,
                                                  lower=10, upper=180,
                                                  step_increment=1)
            hscale_in_table(self.outer_teeth_adj, gear_table, row, self.outer_teeth_changed)

            row += 1
            moving_gear_tooltip = _(
                "Number of teeth of moving gear. The size of the moving gear is "
                "proportional to the number of teeth."
            )
            label_in_table(_("Moving Gear Teeth"), gear_table, row, moving_gear_tooltip)
            self.inner_teeth_adj = Gtk.Adjustment.new(self.p.inner_teeth, 2, 100, 1, 10, 0)
            hscale_in_table(self.inner_teeth_adj, gear_table, row, self.inner_teeth_changed)

            row += 1
            label_in_table(_("Hole percent"), gear_table, row,
                           _("How far is the hole from the center of the moving gear. "
                             "100% means that the hole is at the gear's edge."))
            self.hole_percent_adj = Gtk.Adjustment.new(self.p.hole_percent, 2.5, 100.0, 0.5, 10, 0)
            self.hole_percent_myscale = hscale_in_table(self.hole_percent_adj, gear_table,
                                                        row, self.hole_percent_changed, digits=1)

            # "Kit" pattern notation.

            kit_table = create_table(5)

            row = 0
            label_in_table(_("Fixed Gear Teeth"), kit_table, row, fixed_gear_tooltip)
            self.kit_outer_teeth_combo = set_combo_in_table([str(t) for t in ring_teeth], False, kit_table, row,
                                                            self.kit_outer_teeth_combo_changed)

            row += 1
            label_in_table(_("Moving Gear Teeth"), kit_table, row, moving_gear_tooltip)
            self.kit_inner_teeth_combo = set_combo_in_table([str(t) for t in wheel_teeth], False, kit_table, row,
                                                            self.kit_inner_teeth_combo_changed)

            row += 1
            label_in_table(_("Hole Number"), kit_table, row,
                           _("Hole #1 is at the edge of the gear. "
                             "The maximum hole number is near the center. "
                             "The maximum hole number is different for each gear."))
            self.kit_hole_adj = Gtk.Adjustment.new(self.p.hole_number, 1, self.p.kit_max_hole_number(), 1, 10, 0)
            self.kit_hole_myscale = hscale_in_table(self.kit_hole_adj, kit_table, row, self.kit_hole_changed)

            # "Visual" pattern notation.

            visual_table = create_table(5)

            row = 0
            label_in_table(_("Flower Petals"), visual_table, row,
                           _("The number of petals in the pattern."))
            self.petals_adj = Gtk.Adjustment.new(self.p.petals, 2, 100, 1, 5, 0)
            hscale_in_table(self.petals_adj, visual_table, row, self.petals_changed, cols=3)

            row += 1
            label_in_table(_("Petal Skip"), visual_table, row,
                           _( "The number of petals to advance for drawing the next petal."))
            self.petal_skip_adj = Gtk.Adjustment.new(self.p.petal_skip, 1, 50, 1, 5, 0)
            hscale_in_table(self.petal_skip_adj, visual_table, row, self.petal_skip_changed, cols=3)

            row += 1
            label_in_table(_("Hole Radius(%)"), visual_table, row,
                            _("The radius of the hole in the center of the pattern "
                              "where nothing will be drawn. Given as a percentage of the "
                              "size of the pattern. A value of 0 will produce no hole. "
                              "A Value of 99 will produce a thin line on the edge."))
            self.doughnut_hole_adj = Gtk.Adjustment.new(self.p.doughnut_hole, 0.0, 99.0, 0.1, 5.0, 0.0)
            self.doughnut_hole_myscale = spin_in_table(self.doughnut_hole_adj, visual_table,
                                                       row, self.doughnut_hole_changed, 1, 1)

            self.doughnut = DoughnutWidget()
            frame = Gtk.Frame()
            frame.add(self.doughnut)
            visual_table.attach(frame, 2, row, 1, 1)
            self.doughnut.set_hexpand(True)
            self.doughnut.set_halign(Gtk.Align.FILL)
            frame.set_hexpand(True)
            frame.set_halign(Gtk.Align.FILL)

            self.doughnut.connect('values_changed', self.doughnut_changed)
            frame.show()
            self.doughnut.show()

            label_in_table(_("Width(%)"), visual_table, row,
                           _("The width of the pattern as a percentage of the "
                             "size of the pattern. A Value of 1 will just draw a thin pattern. "
                              "A Value of 100 will fill the entire fixed gear."),
                            3)
            self.doughnut_width_adj = Gtk.Adjustment.new(self.p.doughnut_width, 1.0, 100.0, 0.1, 5.0, 0.0)
            self.doughnut_width_myscale = spin_in_table(self.doughnut_width_adj, visual_table,
                                                        row, self.doughnut_width_changed, 1, 4)

            # Add tables to the pattern notebook

            pattern_notation_page[VISUAL_NOTATION] = self.pattern_notebook.append_page(visual_table)
            self.pattern_notebook.set_tab_label_text(visual_table, _("Visual"))
            self.pattern_notebook.child_set_property(visual_table, 'tab-expand', False)
            self.pattern_notebook.child_set_property(visual_table, 'tab-fill', False)
            visual_table.show()

            pattern_notation_page[TOY_KIT_NOTATION] = self.pattern_notebook.append_page(kit_table)
            self.pattern_notebook.set_tab_label_text(kit_table, _("Toy Kit"))
            self.pattern_notebook.child_set_property(kit_table, 'tab-expand', False)
            self.pattern_notebook.child_set_property(kit_table, 'tab-fill', False)
            kit_table.show()

            pattern_notation_page[GEAR_NOTATION] = self.pattern_notebook.append_page(gear_table)
            self.pattern_notebook.set_tab_label_text(gear_table, _("Gears"))
            self.pattern_notebook.child_set_property(gear_table, 'tab-expand', False)
            self.pattern_notebook.child_set_property(gear_table, 'tab-fill', False)
            gear_table.show()

            add_to_box(vbox, self.pattern_notebook)

            add_vertical_space(vbox, 14)

            hbox = Gtk.HBox(spacing=5)
            pattern_table = create_table(5)

            row = 0
            label_in_table(_("Rotation"), pattern_table, row,
                           _("Rotation of the pattern, in degrees. "
                             "The starting position of the moving gear in the fixed gear."))
            self.pattern_rotation_adj, myscale = rotation_in_table(
                self.p.pattern_rotation, pattern_table, row, self.pattern_rotation_changed
            )

            hbox.pack_end(pattern_table, expand=True, fill=True, padding=0)
            pattern_table.show()

            vbox.add(hbox)
            hbox.show()

            return vbox

        def fixed_gear_page():

            vbox = Gtk.VBox(spacing=0, homogeneous=False)

            add_vertical_space(vbox, 14)

            table = create_table(10)

            row = 0
            label_in_table(_("Shape"), table, row,
                           _("The shape of the fixed gear to be used inside current selection. "
                             "Rack is a long round-edged shape provided in the toy kits. "
                             "Frame hugs the boundaries of the rectangular selection, "
                             "use hole=100 in Gear notation to touch boundary. "
                             "Selection will hug boundaries of current selection - try something non-rectangular."))
            self.shape_combo = set_combo_in_table(shapes, True, table, row,
                                                  self.shape_combo_changed)

            row += 1
            label_in_table(_("Sides"), table, row, _("Number of sides of the shape."))
            self.sides_adj = Gtk.Adjustment.new(self.p.sides, 3, 16, 1, 2, 2)
            self.sides_myscale = hscale_in_table(self.sides_adj, table, row, self.sides_changed)

            row += 1
            label_in_table(_("Morph"), table, row, _("Morph fixed gear shape. Only affects some of the shapes."))
            self.morph_adj = Gtk.Adjustment.new(self.p.morph, 0.0, 1.0, 0.01, 0.1, 0.1)
            self.morph_myscale = hscale_in_table(self.morph_adj, table, row, self.morph_changed, digits=2)

            row += 1
            label_in_table(_("Rotation"), table, row, _("Rotation of the fixed gear, in degrees"))
            self.shape_rotation_adj, self.shape_rotation_myscale = rotation_in_table(
                self.p.shape_rotation, table, row, self.shape_rotation_changed
            )

            add_to_box(vbox, table)
            return vbox

        def size_page():

            vbox = Gtk.VBox(spacing=0, homogeneous=False)
            add_vertical_space(vbox, 14)
            table = create_table(10)

            row = 0
            label_in_table(_("Margin (px)"), table, row, _("Margin from edge of selection."))
            self.margin_adj = Gtk.Adjustment.new(self.p.margin_pixels, 0, max(img.get_height(), img.get_width()), 1, 10, 10)
            hscale_in_table(self.margin_adj, table, row, self.margin_changed)

            row += 1
            self.equal_w_h_checkbox = Gtk.CheckButton(label=_("Make width and height equal"))
            self.equal_w_h_checkbox.set_tooltip_text(
                _("When unchecked, the pattern will fill the current image or selection. "
                  "When checked, the pattern will have same width and height, and will be centered.")
            )
            self.equal_w_h_checkbox.set_border_width(30)
            table.attach(self.equal_w_h_checkbox, 0, row, 3, 1)
            self.equal_w_h_checkbox.show()
            self.equal_w_h_checkbox.connect("toggled", self.equal_w_h_checkbox_changed)

            add_to_box(vbox, table)
            return vbox

        def dialog_button_box():

            self.dialog.add_button(_("_Cancel"), Gtk.ResponseType.CANCEL)
            self.ok_btn = self.dialog.add_button(_("_OK"), Gtk.ResponseType.OK)
            btn = self.dialog.add_button(_("Re_draw"), RESPONSE_REDRAW)
            btn.set_tooltip_text(
                _("If you change the settings of a tool, change color, or change the selection, "
                "press this to preview how the pattern looks.")
            )
            self.dialog.add_button(_("_Reset"), RESPONSE_RESET_PARAMS)

            hbox = Gtk.HBox(homogeneous=True, spacing=20)
            hbox.set_border_width(10)

            table = create_table(5)

            row = 0
            label_in_table(_("Save"), table, row,
                           _("Choose whether to save as new layer, redraw on last active layer, or save to path"))
            self.save_option_combo = set_combo_in_table(save_options, False, table, row,
                                                        self.save_option_changed)
            self.save_option_combo.show()

            hbox.add(table)
            table.show()

            return hbox

        def create_ui():

            use_header_bar = Gtk.Settings.get_default().get_property("gtk-dialogs-use-header")
            self.dialog = GimpUi.Dialog(use_header_bar=use_header_bar,
                                      title=_("Spyrogimp"))
            #self.set_default_size(350, -1)
            #self.set_border_width(10)

            vbox = Gtk.Box(orientation=Gtk.Orientation.VERTICAL,
                           homogeneous=False, spacing=10)
            self.dialog.get_content_area().add(vbox)
            vbox.show()

            box = GimpUi.HintBox.new(_("Draw spyrographs using current tool settings and selection."))
            vbox.pack_start(box, False, False, 0)
            box.show()

            add_horizontal_separator(vbox)

            add_to_box(vbox, top_table())

            self.main_notebook = Gtk.Notebook()
            self.main_notebook.set_show_tabs(True)
            self.main_notebook.set_border_width(5)

            pattern_frame = pattern_notation_frame()
            self.main_notebook.append_page(pattern_frame, Gtk.Label.new(_("Curve Pattern")))
            pattern_frame.show()
            fixed_g_page = fixed_gear_page()
            self.main_notebook.append_page(fixed_g_page, Gtk.Label.new(_("Fixed Gear")))
            fixed_g_page.show()
            size_p = size_page()
            self.main_notebook.append_page(size_p, Gtk.Label.new(_("Size")))
            size_p.show()

            vbox.add(self.main_notebook)
            self.main_notebook.show()

            # add_horizontal_separator(vbox)

            add_to_box(vbox, dialog_button_box())

            self.progress_bar = Gtk.ProgressBar()   # gimpui.ProgressBar() - causes gimppdbprogress error message.
            self.progress_bar.set_show_text(True)
            vbox.add(self.progress_bar)
            self.progress_bar.show()

            self.dialog.show()

        self.enable_incremental_drawing = False

        self.img = img
        # Remember active layer, so we can restore it when the plugin is done.
        self.active_layer = layer

        self.p = unshelf_parameters()  # Model

        self.engine = DrawingEngine(img, self.p)

        # Make a new GIMP layer to draw on
        self.spyro_layer = Gimp.Layer.new(img, layer_name, img.get_width(), img.get_height(),
                                          layer.type_with_alpha(), 100, Gimp.LayerMode.NORMAL)
        img.insert_layer(self.spyro_layer, None, 0)
        self.drawing_layer = self.spyro_layer

        # Create the UI.
        GimpUi.init(sys.argv[0])
        create_ui()
        self.update_view()   # Update UI to reflect the parameter values.

        # Map button responses to callback this method
        self.dialog.connect('response', self.handle_response)

        # Setup for Handling incremental/interactive drawing of pattern
        self.idle_task = None
        self.enable_incremental_drawing = True

        self.adj_changed = False

        # Draw pattern of the current settings.
        self.start_new_incremental_drawing()

    def handle_response(self, dialog, response_id):
        if response_id in [Gtk.ResponseType.CANCEL, Gtk.ResponseType.CLOSE, Gtk.ResponseType.DELETE_EVENT]:
            self.cancel_window(self.dialog)
        elif response_id == Gtk.ResponseType.OK:
            self.ok_window(self.dialog)
        elif response_id == RESPONSE_REDRAW:
            self.redraw()
        elif response_id == RESPONSE_RESET_PARAMS:
            self.reset_params()
        else:
            print("Unhandled response: " + str(response_id))
        #GTK_RESPONSE_APPLY
        #GTK_RESPONSE_HELP

    def clear_idle_task(self):
        if self.idle_task:
            GLib.source_remove(self.idle_task)
            # Close the undo group in the likely case the idle task left it open.
            self.img.undo_group_end()
            self.idle_task = None

    # Callbacks for closing the plugin

    def ok_window(self, widget):
        """ Called when clicking on the 'close' button. """

        self.ok_btn.set_sensitive(False)

        shelf_parameters(self.p)

        if self.p.save_option == SAVE_AS_NEW_LAYER:
            if self.spyro_layer in self.img.get_layers():
                self.img.active_layer = self.spyro_layer

            # If we are in the middle of incremental draw, we want to complete it, and only then to exit.
            # However, in order to complete it, we need to create another idle task.
            if self.idle_task:
                def quit_dialog_on_completion():
                    while self.idle_task:
                        yield True

                    Gtk.main_quit()  # This will quit the dialog.
                    yield False

                task = quit_dialog_on_completion()
                GLib.idle_add(task.__next__)
            else:
                Gtk.main_quit()
        else:
            # If there is an incremental drawing taking place, lets stop it.
            self.clear_idle_task()

            if self.spyro_layer in self.img.get_layers():
                self.img.remove_layer(self.spyro_layer)
                self.img.active_layer = self.active_layer

            self.drawing_layer = self.active_layer

            def draw_full(tool):
                self.progress_start()
                yield True

                self.engine.reset_incremental()

                self.img.undo_group_start()

                while self.engine.has_more_strokes():
                    yield True
                    self.draw_next_chunk(tool=tool)

                self.img.undo_group_end()

                Gimp.displays_flush()

                Gtk.main_quit()
                yield False

            tool = SaveToPathTool(self.img) if self.p.save_option == SAVE_AS_PATH else None
            task = draw_full(tool)
            GLib.idle_add(task.__next__)

    def cancel_window(self, widget, what=None):
        self.clear_idle_task()

        # We want to delete the temporary layer, but as a precaution, lets ask first,
        # maybe it was already deleted by the user.
        if self.spyro_layer in self.img.get_layers():
            self.img.remove_layer(self.spyro_layer)
            Gimp.displays_flush()

        Gtk.main_quit()

    def update_view(self):
        """ Update the UI to reflect the values in the Pattern Parameters. """
        self.curve_type_combo.set_active_id(self.p.curve_type)
        self.curve_type_side_effects()

        self.pattern_notebook.set_current_page(pattern_notation_page[self.p.pattern_notation])

        self.outer_teeth_adj.set_value(self.p.outer_teeth)
        self.inner_teeth_adj.set_value(self.p.inner_teeth)
        self.hole_percent_adj.set_value(self.p.hole_percent)
        self.pattern_rotation_adj.set_value(self.p.pattern_rotation)

        self.kit_outer_teeth_combo.set_active(self.p.kit_fixed_gear_index)
        self.kit_inner_teeth_combo.set_active(self.p.kit_moving_gear_index)
        self.kit_hole_adj.set_value(self.p.hole_number)
        self.kit_inner_teeth_combo_side_effects()

        self.petals_adj.set_value(self.p.petals)
        self.petal_skip_adj.set_value(self.p.petal_skip)
        self.doughnut_hole_adj.set_value(self.p.doughnut_hole)
        self.doughnut.set_hole_radius(self.p.doughnut_hole)
        self.doughnut_width_adj.set_value(self.p.doughnut_width)
        self.doughnut.set_width(self.p.doughnut_width)
        self.petals_changed_side_effects()

        self.shape_combo.set_active_id(self.p.shape_index)
        self.shape_combo_side_effects()
        self.sides_adj.set_value(self.p.sides)
        self.morph_adj.set_value(self.p.morph)
        self.equal_w_h_checkbox.set_active(self.p.equal_w_h)
        self.shape_rotation_adj.set_value(self.p.shape_rotation)

        self.margin_adj.set_value(self.p.margin_pixels)
        self.tool_combo.set_active_id(self.p.tool_index)
        self.long_gradient_checkbox.set_active(self.p.long_gradient)
        self.save_option_combo.set_active(self.p.save_option)

    def reset_params(self, widget=None):
        self.engine.p = self.p = PatternParameters()
        self.update_view()

    # Callbacks to handle changes in dialog parameters.

    def curve_type_side_effects(self):
        if curve_types[self.p.curve_type].supports_shapes():
            self.shape_combo.set_sensitive(True)

            self.sides_myscale.set_sensitive(shapes[self.p.shape_index].has_sides())
            self.morph_myscale.set_sensitive(shapes[self.p.shape_index].can_morph())
            self.shape_rotation_myscale.set_sensitive(shapes[self.p.shape_index].can_rotate())

            self.hole_percent_myscale.set_sensitive(True)
            self.kit_hole_myscale.set_sensitive(True)

            self.doughnut_hole_myscale.set_sensitive(True)
            self.doughnut_width_myscale.set_sensitive(True)
        else:
            # Lissajous curves do not have shapes, or holes for moving gear
            self.shape_combo.set_sensitive(False)

            self.sides_myscale.set_sensitive(False)
            self.morph_myscale.set_sensitive(False)
            self.shape_rotation_myscale.set_sensitive(False)

            self.hole_percent_myscale.set_sensitive(False)
            self.kit_hole_myscale.set_sensitive(False)

            self.doughnut_hole_myscale.set_sensitive(False)
            self.doughnut_width_myscale.set_sensitive(False)

    def curve_type_changed(self, val):
        self.p.curve_type = val.get_active_id()
        self.curve_type_side_effects()
        self.redraw()

    def pattern_notation_tab_changed(self, notebook, page, page_num, user_param1=None):
        if self.enable_incremental_drawing:
            for notation in pattern_notation_page:
                if pattern_notation_page[notation] == page_num:
                    self.p.pattern_notation = notation

            self.redraw()

    # Callbacks: pattern changes using the Toy Kit notation.

    def kit_outer_teeth_combo_changed(self, val):
        self.p.kit_fixed_gear_index = val.get_active()
        self.redraw()

    def kit_inner_teeth_combo_side_effects(self):
        # Change the max hole number according to the newly activated wheel.
        # We might also need to update the hole value, if it is larger than the new max.
        max_hole_number = self.p.kit_max_hole_number()
        if self.p.hole_number > max_hole_number:
            self.p.hole_number = max_hole_number
            self.kit_hole_adj.set_value(max_hole_number)
        self.kit_hole_adj.set_upper(max_hole_number)

    def kit_inner_teeth_combo_changed(self, val):
        self.p.kit_moving_gear_index = val.get_active()
        self.kit_inner_teeth_combo_side_effects()
        self.redraw()

    def kit_hole_changed(self, val):
        self.p.hole_number = val.get_value()
        self.redraw()

    # Callbacks: pattern changes using the Gears notation.

    def outer_teeth_changed(self, val):
        self.p.outer_teeth = val.get_value()
        self.redraw()

    def inner_teeth_changed(self, val):
        self.p.inner_teeth = val.get_value()
        self.redraw()

    def hole_percent_changed(self, val):
        self.p.hole_percent = val.get_value()
        self.redraw()

    def pattern_rotation_changed(self, val):
        self.p.pattern_rotation = val.get_value()
        self.redraw()

    # Callbacks: pattern changes using the Visual notation.

    def petals_changed_side_effects(self):
        max_petal_skip = int(self.p.petals / 2)
        if self.p.petal_skip > max_petal_skip:
            self.p.petal_skip = max_petal_skip
            self.petal_skip_adj.set_value(max_petal_skip)
        self.petal_skip_adj.set_upper(max_petal_skip)

    def petals_changed(self, val):
        self.p.petals = int(val.get_value())
        self.petals_changed_side_effects()
        self.redraw()

    def petal_skip_changed(self, val):
        self.p.petal_skip = int(val.get_value())
        self.redraw()

    def doughnut_hole_changed(self, val):
        self.p.doughnut_hole = val.get_value()

        self.doughnut.set_hole_radius(val.get_value())
        self.redraw()

    def doughnut_width_changed(self, val):
        self.p.doughnut_width = val.get_value()
        self.doughnut.set_width(val.get_value())
        self.redraw()

    def doughnut_changed(self, widget, hole, width):
        self.doughnut_hole_adj.set_value(hole)
        self.p.doughnut_hole = hole
        self.doughnut_width_adj.set_value(width)
        self.p.doughnut_width = width
        self.redraw()

    # Callbacks: Fixed gear

    def shape_combo_side_effects(self):
        self.sides_myscale.set_sensitive(shapes[self.p.shape_index].has_sides())
        self.morph_myscale.set_sensitive(shapes[self.p.shape_index].can_morph())
        self.shape_rotation_myscale.set_sensitive(shapes[self.p.shape_index].can_rotate())
        self.equal_w_h_checkbox.set_sensitive(shapes[self.p.shape_index].can_equal_w_h())

    def shape_combo_changed(self, val):
        self.p.shape_index = val.get_active_id()
        self.shape_combo_side_effects()
        self.redraw()

    def sides_changed(self, val):
        self.p.sides = val.get_value()
        self.redraw()

    def morph_changed(self, val):
        self.p.morph = val.get_value()
        self.redraw()

    def equal_w_h_checkbox_changed(self, val):
        self.p.equal_w_h = val.get_active()
        self.redraw()

    def shape_rotation_changed(self, val):
        self.p.shape_rotation = val.get_value()
        self.redraw()

    def margin_changed(self, val) :
        self.p.margin_pixels = val.get_value()
        self.redraw()

    # Style callbacks

    def tool_changed_side_effects(self):
        self.long_gradient_checkbox.set_sensitive(tools[self.p.tool_index].can_color)

    def tool_combo_changed(self, val):
        self.p.tool_index = val.get_active_id()
        self.tool_changed_side_effects()
        self.redraw()

    def long_gradient_changed(self, val):
        self.p.long_gradient = val.get_active()
        self.redraw()

    def save_option_changed(self, val):
        self.p.save_option = self.save_option_combo.get_active()

    # Progress bar of plugin window.

    def progress_start(self):
        self.progress_bar.set_text(_("Rendering Pattern"))
        self.progress_bar.set_fraction(0.0)
        Gimp.displays_flush()

    def progress_end(self):
        self.progress_bar.set_text("")
        self.progress_bar.set_fraction(0.0)

    def progress_update(self):
        self.progress_bar.set_fraction(self.engine.fraction_done())

    def progress_unknown(self):
        self.progress_bar.set_text(_("Please wait : Rendering Pattern"))
        self.progress_bar.pulse()
        Gimp.displays_flush()

    # Incremental drawing.

    def draw_next_chunk(self, tool=None):
        """ Incremental drawing """

        t = time.time()

        chunk_size = self.engine.draw_next_chunk(self.drawing_layer, tool=tool)

        draw_time = time.time() - t
        self.engine.report_time(draw_time)
        print("Chunk size " + str(chunk_size) + " time " + str(draw_time))

        if self.engine.has_more_strokes():
            self.progress_update()
        else:
            self.progress_end()

        Gimp.displays_flush()

    def start_new_incremental_drawing(self):
        """
        Compute strokes for the current pattern, and store then in the IncrementalDraw object,
        so they can be drawn in pieces without blocking the user.
        Finally, draw the first chunk of strokes.
        """

        def incremental_drawing():
            self.progress_start()
            yield True
            self.engine.reset_incremental()

            self.img.undo_group_start()
            while self.engine.has_more_strokes():
                yield True
                self.draw_next_chunk()
            self.img.undo_group_end()

            self.idle_task = None
            yield False

        # Start new idle task to perform incremental drawing in the background.
        self.clear_idle_task()
        task = incremental_drawing()
        self.idle_task = GLib.idle_add(task.__next__)

    def clear(self):
        """ Clear current drawing. """
        # pdb.gimp_edit_clear(self.spyro_layer)
        self.spyro_layer.fill(Gimp.FillType.TRANSPARENT)

    def redraw(self, data=None):
        if self.enable_incremental_drawing:
            self.clear()
            self.start_new_incremental_drawing()


class SpyrogimpPlusPlugin(Gimp.PlugIn):
    ## GimpPlugIn virtual methods ##
    def do_set_i18n(self, procname):
        return True, 'gimp30-python', None

    def do_query_procedures(self):
        return [PROC_NAME]

    def do_create_procedure(self, name):
        if name == PROC_NAME:
            procedure = Gimp.ImageProcedure.new(self, name,
                                                Gimp.PDBProcType.PLUGIN,
                                                self.plug_in_spyrogimp, None)
            procedure.set_image_types("*")
            procedure.set_sensitivity_mask (Gimp.ProcedureSensitivityMask.DRAWABLE)
            procedure.set_documentation (_("Draw spyrographs using current tool settings and selection."),
                                         _("Uses current tool settings to draw Spyrograph patterns. "
                                         "The size and location of the pattern is based on the current selection."),
                                         name)
            procedure.set_menu_label(_("Spyrogimp..."))
            procedure.set_attribution("Elad Shahar",
                                      "Elad Shahar",
                                      "2018")
            procedure.add_menu_path ("<Image>/Filters/Render/")

            curve_choice = Gimp.Choice.new()
            curve_choice.add("spyrograph", 0, _("Spyrograph"), "")
            curve_choice.add("epitrochoid", 1, _("Epitrochoid"), "")
            curve_choice.add("sine", 2, _("Sine"), "")
            curve_choice.add("lissajous", 3, _("Lissajous"), "")
            procedure.add_choice_argument ("curve-type", _("Curve Type"), _("Curve Type"),
                                           curve_choice, "spyrograph", GObject.ParamFlags.READWRITE)
            shape_choice = Gimp.Choice.new()
            shape_choice.add("circle", 0, _("Circle"), "")
            shape_choice.add("rack", 1, _("rack"), "")
            shape_choice.add("frame", 2, _("frame"), "")
            shape_choice.add("selection", 3, _("Selection"), "")
            shape_choice.add("polygon-star", 4, _("Polygon-Star"), "")
            shape_choice.add("sine", 5, _("Sine"), "")
            shape_choice.add("bumps", 6, _("Bumps"), "")
            procedure.add_choice_argument ("shape", _("Shape"), _("Shape"),
                                           shape_choice, "circle", GObject.ParamFlags.READWRITE)
            procedure.add_int_argument ("sides", _("Si_des"),
                                        _("Number of sides of fixed gear (3 or greater). Only used by some shapes."),
                                        3, GLib.MAXINT, 3, GObject.ParamFlags.READWRITE)
            procedure.add_double_argument ("morph", _("_Morph"),
                                           _("Morph shape of fixed gear, between 0 and 1. Only used by some shapes."),
                                           0.0, 1.0, 0.0, GObject.ParamFlags.READWRITE)
            procedure.add_int_argument ("fixed-teeth", _("Fi_xed Gear Teeth"),
                                        _("Number of teeth for fixed gear."),
                                        0, GLib.MAXINT, 96, GObject.ParamFlags.READWRITE)
            procedure.add_int_argument ("moving-teeth", _("Mo_ving Gear Teeth"),
                                        _("Number of teeth for fixed gear."),
                                        0, GLib.MAXINT, 36, GObject.ParamFlags.READWRITE)
            procedure.add_double_argument ("hole_percent", _("_Hole Radius (%)"),
                                           _("Location of hole in moving gear in percent, where 100 means that "
                                             "the hole is at the edge of the gear, and 0 means the hole is at the center"),
                                           0.0, 100.0, 100.0, GObject.ParamFlags.READWRITE)
            procedure.add_int_argument ("margin", _("Margin (_px)"),
                                        _("Margin from selection, in pixels"),
                                        0, GLib.MAXINT, 0, GObject.ParamFlags.READWRITE)
            procedure.add_boolean_argument ("equal-w-h", _("Make width and height equal"),
                                            _("Make width and height equal"),
                                            False, GObject.ParamFlags.READWRITE)
            procedure.add_double_argument ("pattern-rotation", _("_Rotation"),
                                           _("Pattern rotation, in degrees"),
                                           -360.0, 360.0, 0.0, GObject.ParamFlags.READWRITE)
            procedure.add_double_argument ("shape-rotation", _("_Rotation"),
                                           _("Shape rotation of fixed gear, in degrees"),
                                           -360.0, 360.0, 0.0, GObject.ParamFlags.READWRITE)
            tool_choice = Gimp.Choice.new()
            tool_choice.add("preview", 0, _("Preview"), "")
            tool_choice.add("paintbrush", 1, _("PaintBrush"), "")
            tool_choice.add("pencil", 2, _("Pencil"), "")
            tool_choice.add("airbrush", 3, _("AirBrush"), "")
            tool_choice.add("stroke", 4, _("Stroke"), "")
            tool_choice.add("ink", 5, _("Ink"), "")
            tool_choice.add("mypaintbrush", 6, _("MyPaintBrush"), "")
            #TODO: Add Clone option once it's fixed
            procedure.add_choice_argument ("tool", _("Tool"), _("Tool"),
                                           tool_choice, "preview", GObject.ParamFlags.READWRITE)
            procedure.add_boolean_argument ("long-gradient",
                                            _("Long _Gradient"),
                                            _("Whether to apply a long gradient to match the length of the pattern. "
                                              "Only applicable to some of the tools."),
                                            False, GObject.ParamFlags.READWRITE)

        return procedure

    # Implementation of plugin.
    def plug_in_spyrogimp(self, procedure, run_mode, image, n_layers, layers, config, data):
        curve_type=config.get_property('curve-type')
        shape=config.get_property('shape')
        sides=config.get_property('sides')
        morph=config.get_property('morph')
        fixed_teeth=config.get_property('fixed-teeth')
        moving_teeth=config.get_property('moving-teeth')
        hole_percent=config.get_property('hole-percent')
        margin=config.get_property('margin')
        equal_w_h=config.get_property('equal-w-h')
        pattern_rotation=config.get_property('pattern-rotation')
        shape_rotation=config.get_property('shape-rotation')
        tool=config.get_property('tool')
        long_gradient=config.get_property('long-gradient')

        if run_mode == Gimp.RunMode.NONINTERACTIVE:
            pp = PatternParameters()
            pp.curve_type = curve_type
            pp.shape_index = shape
            pp.sides = sides
            pp.morph = morph
            pp.outer_teeth = fixed_teeth
            pp.inner_teeth = moving_teeth
            pp.hole_percent = hole_percent
            pp.margin_pixels = margin
            pp.equal_w_h = equal_w_h
            pp.pattern_rotation = pattern_rotation
            pp.shape_rotation = shape_rotation
            pp.tool_index = tool
            pp.long_gradient = long_gradient

            engine = DrawingEngine(image, pp)
            engine.draw_full(layers[0])

        elif run_mode == Gimp.RunMode.INTERACTIVE:
            Gegl.init (None)

            window = SpyroWindow(image, layers[0])
            Gtk.main()

        elif run_mode == Gimp.RunMode.WITH_LAST_VALS:
            pp = unshelf_parameters()
            engine = DrawingEngine(image, pp)
            engine.draw_full(layers[0])

        return procedure.new_return_values(Gimp.PDBStatusType.SUCCESS, GLib.Error())


Gimp.main(SpyrogimpPlusPlugin.__gtype__, sys.argv)
