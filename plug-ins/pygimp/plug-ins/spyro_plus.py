#!/usr/bin/env python2

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

from gimpshelf import shelf
from gimpenums import *
import gimp
import gimpplugin
import gimpui
import gobject
import gtk

from math import pi, sin, cos, atan, atan2, fmod, radians
import gettext
import fractions
import time


# i18n
t = gettext.translation("gimp20-python", gimp.locale_directory, fallback=True)
_ = t.ugettext

def N_(message):
    return message


pdb = gimp.pdb

two_pi, half_pi = 2 * pi, pi / 2
layer_name = _("Spyro Layer")

# "Enums"
GEAR_NOTATION, TOY_KIT_NOTATION = range(2)       # Pattern notations

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


### Shapes


class CanRotateShape:
    pass


class Shape:
    def configure(self, img, pp, cp, drawing_no):
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

    def configure(self, img, pp, cp, drawing_no):
        Shape.configure(self, img, pp, cp, drawing_no)
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

    def configure(self, img, pp, cp, drawing_no):
        Shape.configure(self, img, pp, cp, drawing_no)

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

    def configure(self, img, pp, cp, drawing_no):
        Shape.configure(self, img, pp, cp, drawing_no)

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


class SelectionToPath:
    """ Converts a selection to a path """

    def __init__(self, image):
        self.image = image

        # Compute hash of selection, so we can detect when it was modified.
        self.last_selection_hash = self.compute_selection_hash()

        self.convert_selection_to_path()

    def convert_selection_to_path(self):

        if pdb.gimp_selection_is_empty(self.image):
            selection_was_empty = True
            pdb.gimp_selection_all(self.image)
        else:
            selection_was_empty = False

        pdb.plug_in_sel2path(self.image, self.image.active_layer)

        self.path = self.image.vectors[0]

        self.num_strokes, self.stroke_ids = pdb.gimp_vectors_get_strokes(self.path)
        self.stroke_ids = list(self.stroke_ids)

        # A path may contain several strokes. If so lets throw away a stroke that
        # simply describes the borders of the image, if one exists.
        if self.num_strokes > 1:
            # Lets compute what a stroke of the image borders should look like.
            w, h = float(self.image.width), float(self.image.height)
            frame_strokes = [0.0] * 6 + [0.0, h] * 3 + [w, h] * 3 + [w, 0.0] * 3

            for stroke in range(self.num_strokes):
                strokes = self.path.strokes[stroke].points[0]
                if strokes == frame_strokes:
                    del self.stroke_ids[stroke]
                    self.num_strokes -= 1
                    break

        self.set_current_stroke(0)

        if selection_was_empty:
            # Restore empty selection if it was empty.
            pdb.gimp_selection_none(self.image)

    def compute_selection_hash(self):
        px = self.image.selection.get_pixel_rgn(0, 0, self.image.width, self.image.height)
        return px[0:self.image.width, 0:self.image.height].__hash__()

    def regenerate_path_if_selection_changed(self):
        current_selection_hash = self.compute_selection_hash()
        if self.last_selection_hash != current_selection_hash:
            self.last_selection_hash = current_selection_hash
            self.convert_selection_to_path()

    def get_num_strokes(self):
        return self.num_strokes

    def set_current_stroke(self, stroke_id=0):
        # Compute path length.
        self.path_length = pdb.gimp_vectors_stroke_get_length(self.path, self.stroke_ids[stroke_id], 1.0)
        self.current_stroke = stroke_id

    def point_at_angle(self, oangle):
        oangle_mod = fmod(oangle, two_pi)
        dist = self.path_length * oangle_mod / two_pi
        return pdb.gimp_vectors_stroke_get_point_at_dist(self.path, self.stroke_ids[self.current_stroke], dist, 1.0)


class SelectionShape(Shape):
    name = _("Selection")

    def __init__(self):
        self.path = None

    def process_selection(self, img):
        if self.path is None:
            self.path = SelectionToPath(img)
        else:
            self.path.regenerate_path_if_selection_changed()

    def configure(self, img, pp, cp, drawing_no):
        """ Set bounds of pattern """
        Shape.configure(self, img, pp, cp, drawing_no)
        self.drawing_no = drawing_no
        self.path.set_current_stroke(drawing_no)

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
        x, y, slope, valid = self.path.point_at_angle(oangle)
        slope_angle = atan(slope)
        # We want to find an angle perpendicular to the slope, but in which direction?
        # Lets try both sides and see which of them is inside the selection.
        perpendicular_p, perpendicular_m = slope_angle + half_pi, slope_angle - half_pi
        step_size = 2   # The distance we are going to go in the direction of each angle.
        xp, yp = x + step_size * cos(perpendicular_p), y + step_size * sin(perpendicular_p)
        value_plus = pdb.gimp_selection_value(self.image, xp, yp)
        xp, yp = x + step_size * cos(perpendicular_m), y + step_size * sin(perpendicular_m)
        value_minus = pdb.gimp_selection_value(self.image, xp, yp)

        perpendicular = perpendicular_p if value_plus > value_minus else perpendicular_m
        return x + dist * cos(perpendicular), y + dist * sin(perpendicular)


shapes = [
    CircleShape(), RackShape(), FrameShape(), SelectionShape(),
    PolygonShape(), SineShape(), BumpShape()
]


### Tools


def get_gradient_samples(num_samples):
    gradient_name = pdb.gimp_context_get_gradient()
    reverse_mode = pdb.gimp_context_get_gradient_reverse()
    repeat_mode = pdb.gimp_context_get_gradient_repeat_mode()

    if repeat_mode == REPEAT_TRIANGULAR:
        # Get two uniform samples, which are reversed from each other, and connect them.

        samples = num_samples/2 + 1
        num, color_samples = pdb.gimp_gradient_get_uniform_samples(gradient_name,
             samples, reverse_mode)

        color_samples = list(color_samples)
        del color_samples[-4:]   # Delete last color because it will appear in the next sample

        # If num_samples is odd, lets get an extra sample this time.
        if num_samples % 2 == 1:
            samples += 1

        num, color_samples2 = pdb.gimp_gradient_get_uniform_samples(gradient_name,
             samples, 1 - reverse_mode)

        color_samples2 = list(color_samples2)
        del color_samples2[-4:]  # Delete last color because it will appear in the very first sample

        color_samples.extend(color_samples2)
        color_samples = tuple(color_samples)
    else:
        num, color_samples = pdb.gimp_gradient_get_uniform_samples(gradient_name, num_samples, reverse_mode)

    return color_samples


class PencilTool():
    name = _("Pencil")
    can_color = True

    def draw(self, layer, strokes, color=None):
        if color:
            pdb.gimp_context_push()
            pdb.gimp_context_set_dynamics('Dynamics Off')
            pdb.gimp_context_set_foreground(color)

        pdb.gimp_pencil(layer, len(strokes), strokes)

        if color:
            pdb.gimp_context_pop()


class AirBrushTool():
    name = _("AirBrush")
    can_color = True

    def draw(self, layer, strokes, color=None):
        if color:
            pdb.gimp_context_push()
            pdb.gimp_context_set_dynamics('Dynamics Off')
            pdb.gimp_context_set_foreground(color)

        pdb.gimp_airbrush_default(layer, len(strokes), strokes)

        if color:
            pdb.gimp_context_pop()


class AbstractStrokeTool():

    def draw(self, layer, strokes, color=None):
        # We need to mutiply every point by 3, because we are creating a path,
        #  where each point has two additional control points.
        control_points = []
        for i, k in zip(strokes[0::2], strokes[1::2]):
            control_points += [i, k] * 3

        # Create path
        path = pdb.gimp_vectors_new(layer.image, 'temp_path')
        pdb.gimp_image_add_vectors(layer.image, path, 0)
        sid = pdb.gimp_vectors_stroke_new_from_points(path, 0, len(control_points),
                                                      control_points, False)

        # Draw it.

        pdb.gimp_context_push()

        # Call template method to set the kind of stroke to draw.
        self.prepare_stroke_context(color)

        pdb.gimp_drawable_edit_stroke_item(layer, path)
        pdb.gimp_context_pop()

        # Get rid of the path.
        pdb.gimp_image_remove_vectors(layer.image, path)


# Drawing tool that should be quick, for purposes of previewing the pattern.
class PreviewTool:

    # Implementation using pencil.  (A previous implementation using stroke was slower, and thus removed).
    def draw(self, layer, strokes, color=None):
        foreground = pdb.gimp_context_get_foreground()
        pdb.gimp_context_push()
        pdb.gimp_context_set_defaults()
        pdb.gimp_context_set_foreground(foreground)
        pdb.gimp_context_set_dynamics('Dynamics Off')
        pdb.gimp_context_set_brush('1. Pixel')
        pdb.gimp_context_set_brush_size(1.0)
        pdb.gimp_context_set_brush_spacing(3.0)
        pdb.gimp_pencil(layer, len(strokes), strokes)
        pdb.gimp_context_pop()

    name = _("Preview")
    can_color = False


class StrokeTool(AbstractStrokeTool):
    name = _("Stroke")
    can_color = True

    def prepare_stroke_context(self, color):
        if color:
            pdb.gimp_context_set_dynamics('Dynamics Off')
            pdb.gimp_context_set_foreground(color)

        pdb.gimp_context_set_stroke_method(STROKE_LINE)


class StrokePaintTool(AbstractStrokeTool):
    def __init__(self, name, paint_method, can_color=True):
        self.name = name
        self.paint_method = paint_method
        self.can_color = can_color

    def prepare_stroke_context(self, color):
        if self.can_color and color is not None:
            pdb.gimp_context_set_dynamics('Dynamics Off')
            pdb.gimp_context_set_foreground(color)

        pdb.gimp_context_set_stroke_method(STROKE_PAINT_METHOD)
        pdb.gimp_context_set_paint_method(self.paint_method)


tools = [
    PreviewTool(),
    StrokePaintTool(_("PaintBrush"), "gimp-paintbrush"),
    PencilTool(), AirBrushTool(), StrokeTool(),
    StrokePaintTool(_("Ink"), 'gimp-ink'),
    StrokePaintTool(_("MyPaintBrush"), 'gimp-mybrush')
    # Clone does not work properly when an image is not set.  When that happens, drawing fails, and
    # I am unable to catch the error. This causes the plugin to crash, and subsequent problems with undo.
    # StrokePaintTool("Clone", 'gimp-clone', False)
]


class PatternParameters:
    """
    All the parameters that define a pattern live in objects of this class.
    If you serialize and saved this class, you should reproduce
    the pattern that the plugin would draw.
    """
    def __init__(self):
        if not hasattr(self, 'curve_type'):
            self.curve_type = 0

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

        # Shape
        if not hasattr(self, 'shape_index'):
            self.shape_index = 0  # Index in the shapes array
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
            self.tool_index = 0   # Index in the tools array.
        if not hasattr(self, 'long_gradient'):
            self.long_gradient = False

        if not hasattr(self, 'keep_separate_layer'):
            self.keep_separate_layer = True

    def kit_max_hole_number(self):
        return wheel[self.kit_moving_gear_index][1]


# Handle shelving of plugin parameters

def unshelf_parameters():
    if shelf.has_key("p"):
        parameters = shelf["p"]
        parameters.__init__()  # Fill in missing values with defaults.
        return parameters

    return PatternParameters()


def shelf_parameters(pp):
    shelf["p"] = pp


class ComputedParameters:
    """
    Stores computations performed on a PatternParameters object.
    The results of these computations are used to perform the drawing.
    Having all these computations in one place makes it convenient to pass
    around as a parameter.
    """
    def __init__(self, pp, x1, y1, x2, y2):

        def lcm(a, b):
            """ Least common multiplier """
            return a * b // fractions.gcd(a, b)

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
            self.outer_R = min(self.x_half_size, self.y_half_size)
            size_of_tooth_in_pixels = two_pi * self.outer_R / self.fixed_gear_teeth
            self.moving_gear_radius = size_of_tooth_in_pixels * self.moving_gear_teeth / two_pi
            self.hole_dist_from_center = self.hole_percent / 100.0 * self.moving_gear_radius

        self.pp = pp

        # Combine different ways to specify patterns, into a unified set of computed parameters.
        if self.pp.pattern_notation == GEAR_NOTATION:
            self.fixed_gear_teeth = int(round(pp.outer_teeth))
            self.moving_gear_teeth = int(round(pp.inner_teeth))
            self.hole_percent = pp.hole_percent
        elif self.pp.pattern_notation == TOY_KIT_NOTATION:
            self.fixed_gear_teeth = ring_teeth[pp.kit_fixed_gear_index]
            self.moving_gear_teeth = wheel[pp.kit_moving_gear_index][0]
            # We want to map hole #1 to 100% and hole of max_hole_number to 2.5%
            # We don't want 0% because that would be the exact center of the moving gear,
            # and that would create a boring pattern.
            max_hole_number = wheel[pp.kit_moving_gear_index][1]
            self.hole_percent = (max_hole_number - pp.hole_number) / float(max_hole_number - 1) * 97.5 + 2.5

        # Rotations
        self.shape_rotation_radians = self.radians_from_degrees(pp.shape_rotation)
        self.pattern_rotation_radians = self.radians_from_degrees(pp.pattern_rotation)

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

    def radians_from_degrees(self, degrees):
        positive_degrees = degrees if degrees >= 0 else degrees + 360
        return radians(positive_degrees)

    def get_color(self, n):
        return self.gradients[4*n:4*(n+1)]


### Curve types


class CurveType:

    def supports_shapes(self):
        return True

class RouletteCurveType(CurveType):

    def get_strokes(self, p, cp):
        strokes = []
        for curr_tooth in range(cp.num_points):
            iangle = curr_tooth * cp.iangle_factor
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
            oangle = fmod(curr_tooth * cp.oangle_factor + cp.pattern_rotation_radians, two_pi)

            strokes.append(cp.x_center + cp.x_half_size * cos(oangle))
            strokes.append(cp.y_center + cp.y_half_size * cos(iangle))

        return strokes

    def supports_shapes(self):
        return False


curve_types = [SpyroCurveType(), EpitrochoidCurvetype(), SineCurveType(), LissaCurveType()]

# Drawing engine. Also implements drawing incrementally.
# We don't draw the entire stroke, because it could take several seconds,
# Instead, we break it into chunks.  Incremental drawing is also used for drawing gradients.
class DrawingEngine:

    def __init__(self, img, p):
        self.img, self.p = img, p
        self.cp = None
        self.num_drawings = 0

        # For incremental drawing
        self.strokes = []
        self.start = 0
        self.current_drawing = 0
        self.chunk_size_lines = 600
        self.chunk_no = 0
        # We are aiming for the drawing time of a chunk to be no longer than max_time.
        self.max_time_sec = 0.1

        self.dynamic_chunk_size = True

    def pre_draw(self):
        """ Needs to be called before starting to draw a pattern. """

        self.current_drawing = 0

        if isinstance(shapes[self.p.shape_index], SelectionShape) and curve_types[self.p.curve_type].supports_shapes():
            shapes[self.p.shape_index].process_selection(self.img)
            pdb.gimp_displays_flush()
            self.num_drawings = shapes[self.p.shape_index].get_num_drawings()
        else:
            self.num_drawings = 1

        # Get bounds. We don't care weather a selection exists or not.
        exists, x1, y1, x2, y2 = pdb.gimp_selection_bounds(self.img)

        self.cp = ComputedParameters(self.p, x1, y1, x2, y2)

    def draw_full(self, layer):
        """ Non incremental drawing. """

        self.pre_draw()
        self.img.undo_group_start()

        for drawing_no in range(self.num_drawings):
            self.current_drawing = drawing_no
            self.set_strokes()

            if self.cp.use_gradient:
                while self.has_more_strokes():
                    self.draw_next_chunk(layer, fetch_next_drawing=False)
            else:
                tools[self.p.tool_index].draw(layer, self.strokes)

        self.img.undo_group_end()

        pdb.gimp_displays_flush()

    # Methods for incremental drawing.

    def draw_next_chunk(self, layer, fetch_next_drawing=True):
        stroke_chunk, color = self.next_chunk(fetch_next_drawing)
        tools[self.p.tool_index].draw(layer, stroke_chunk, color)
        return len(stroke_chunk)

    def set_strokes(self):
        """ Compute the strokes of the current pattern. The heart of the plugin. """

        shapes[self.p.shape_index].configure(self.img, self.p, self.cp, drawing_no=self.current_drawing)

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
            self.current_drawing += 1
            if self.current_drawing < self.num_drawings:
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


class SpyroWindow(gtk.Window):

    # Define signal to catch escape key.
    __gsignals__ = dict(
        myescape=(gobject.SIGNAL_RUN_LAST | gobject.SIGNAL_ACTION,
                  None,  # return type
                  (str,))  # arguments
    )

    class MyScale():
        """ Combintation of scale and spin that control the same adjuster. """
        def __init__(self, scale, spin):
            self.scale, self.spin = scale, spin

        def set_sensitive(self, val):
            self.scale.set_sensitive(val)
            self.spin.set_sensitive(val)

    def __init__(self, img, layer):

        def add_horizontal_separator(vbox):
            hsep = gtk.HSeparator()
            vbox.add(hsep)
            hsep.show()

        def add_vertical_space(vbox, height):
            hbox = gtk.HBox()
            hbox.set_border_width(height/2)
            vbox.add(hbox)
            hbox.show()

        def add_to_box(box, w):
            box.add(w)
            w.show()

        def create_table(rows, columns, border_width):
            table = gtk.Table(rows=rows, columns=columns, homogeneous=False)
            table.set_border_width(border_width)
            table.set_col_spacings(10)
            table.set_row_spacings(10)
            return table

        def label_in_table(label_text, table, row, tooltip_text=None):
            """ Create a label and set it in first col of table. """
            label = gtk.Label(label_text)
            label.set_alignment(xalign=0.0, yalign=1.0)
            if tooltip_text:
                label.set_tooltip_text(tooltip_text)
            table.attach(label, 0, 1, row, row + 1, xoptions=gtk.FILL, yoptions=0)
            label.show()

        def hscale_in_table(adj, table, row, callback, digits=0):
            """ Create an hscale and a spinner using the same Adjustment, and set it in table. """
            scale = gtk.HScale(adj)
            scale.set_size_request(150, -1)
            scale.set_digits(digits)
            scale.set_update_policy(gtk.UPDATE_DISCONTINUOUS)
            table.attach(scale, 1, 2, row, row + 1, xoptions=gtk.EXPAND|gtk.FILL, yoptions=0)
            scale.show()

            spin = gtk.SpinButton(adj, climb_rate=0.5, digits=digits)
            spin.set_numeric(True)
            spin.set_snap_to_ticks(True)
            spin.set_max_length(5)
            spin.set_width_chars(5)
            table.attach(spin, 2, 3, row, row + 1, xoptions=0, yoptions=0)
            spin.show()

            adj.connect("value_changed", callback)

            return self.MyScale(scale, spin)

        def rotation_in_table(val, table, row, callback):
            adj = gtk.Adjustment(val, -180.0, 180.0, 1.0)
            myscale = hscale_in_table(adj, table, row, callback, digits=1)
            myscale.scale.add_mark(0.0, gtk.POS_BOTTOM, None)
            return adj, myscale

        def set_combo_in_table(txt_list, table, row, callback):
            combo = gtk.combo_box_new_text()
            for txt in txt_list:
                combo.append_text(txt)
            table.attach(combo, 1, 2, row, row + 1, xoptions=gtk.FILL, yoptions=0)
            combo.show()
            combo.connect("changed", callback)
            return combo

        # Return table which is at the top of the dialog, and has several major input widgets.
        def top_table():

            # Add table for displaying attributes, each having a label and an input widget.
            table = create_table(2, 3, 10)

            # Curve type
            row = 0
            label_in_table(_("Curve Type"), table, row,
                           _("An Epitrochoid pattern is when the moving gear is on the outside of the fixed gear."))
            self.curve_type_combo = set_combo_in_table([ct.name for ct in curve_types], table, row,
                                                       self.curve_type_changed)

            row += 1
            label_in_table(_("Tool"), table, row,
                           _("The tool with which to draw the pattern."
                             "The Preview tool just draws quickly."))
            self.tool_combo = set_combo_in_table([tool.name for tool in tools], table, row,
                                                 self.tool_combo_changed)

            self.long_gradient_checkbox = gtk.CheckButton(_("Long Gradient"))
            self.long_gradient_checkbox.set_tooltip_text(
                _("When unchecked, the current tool settings will be used. "
                  "When checked, will use a long gradient to match the length of the pattern, "
                  "based on current gradient and repeat mode from the gradient tool settings.")
            )
            self.long_gradient_checkbox.set_border_width(0)
            table.attach(self.long_gradient_checkbox, 2, 3, row, row + 1, xoptions=0, yoptions=0)
            self.long_gradient_checkbox.show()
            self.long_gradient_checkbox.connect("toggled", self.long_gradient_changed)

            return table

        def pattern_notation_frame():

            vbox = gtk.VBox(spacing=0, homogeneous=False)

            add_vertical_space(vbox, 14)

            hbox = gtk.HBox(spacing=5)
            hbox.set_border_width(5)

            label = gtk.Label(_("Specify pattern using one of the following tabs:"))
            label.set_tooltip_text(_(
                "The pattern is specified only by the active tab. Toy Kit is similar to Gears, "
                "but it uses gears and hole numbers which are found in toy kits. "
                "If you follow the instructions from the toy kit manuals, results should be similar."))
            hbox.pack_start(label)
            label.show()

            alignment = gtk.Alignment(xalign=0.0, yalign=0.0, xscale=0.0, yscale=0.0)
            alignment.add(hbox)
            hbox.show()
            vbox.add(alignment)
            alignment.show()

            self.pattern_notebook = gtk.Notebook()
            self.pattern_notebook.set_border_width(0)
            self.pattern_notebook.connect('switch-page', self.pattern_notation_tab_changed)

            # "Gear" pattern notation.

            # Add table for displaying attributes, each having a label and an input widget.
            gear_table = create_table(3, 3, 5)

            # Teeth
            row = 0
            fixed_gear_tooltip = _(
                "Number of teeth of fixed gear. The size of the fixed gear is "
                "proportional to the number of teeth."
            )
            label_in_table(_("Fixed Gear Teeth"), gear_table, row, fixed_gear_tooltip)
            self.outer_teeth_adj = gtk.Adjustment(self.p.outer_teeth, 10, 180, 1)
            hscale_in_table(self.outer_teeth_adj, gear_table, row, self.outer_teeth_changed)

            row += 1
            moving_gear_tooltip = _(
                "Number of teeth of moving gear. The size of the moving gear is "
                "proportional to the number of teeth."
            )
            label_in_table(_("Moving Gear Teeth"), gear_table, row, moving_gear_tooltip)
            self.inner_teeth_adj = gtk.Adjustment(self.p.inner_teeth, 2, 100, 1)
            hscale_in_table(self.inner_teeth_adj, gear_table, row, self.inner_teeth_changed)

            row += 1
            label_in_table(_("Hole percent"), gear_table, row,
                           _("How far is the hole from the center of the moving gear. "
                             "100% means that the hole is at the gear's edge."))
            self.hole_percent_adj = gtk.Adjustment(self.p.hole_percent, 2.5, 100.0, 0.5)
            self.hole_percent_myscale = hscale_in_table(self.hole_percent_adj, gear_table,
                                                        row, self.hole_percent_changed, digits=1)

            # "Kit" pattern notation.

            kit_table = create_table(3, 3, 5)

            row = 0
            label_in_table(_("Fixed Gear Teeth"), kit_table, row, fixed_gear_tooltip)
            self.kit_outer_teeth_combo = set_combo_in_table([str(t) for t in ring_teeth], kit_table, row,
                                                            self.kit_outer_teeth_combo_changed)

            row += 1
            label_in_table(_("Moving Gear Teeth"), kit_table, row, moving_gear_tooltip)
            self.kit_inner_teeth_combo = set_combo_in_table([str(t) for t in wheel_teeth], kit_table, row,
                                                            self.kit_inner_teeth_combo_changed)

            row += 1
            label_in_table(_("Hole Number"), kit_table, row,
                           _("Hole #1 is at the edge of the gear. "
                             "The maximum hole number is near the center. "
                             "The maximum hole number is different for each gear."))
            self.kit_hole_adj = gtk.Adjustment(self.p.hole_number, 1, self.p.kit_max_hole_number(), 1)
            self.kit_hole_myscale = hscale_in_table(self.kit_hole_adj, kit_table, row, self.kit_hole_changed)

            # Add tables as childs of the pattern notebook

            pattern_notation_page[TOY_KIT_NOTATION] = self.pattern_notebook.append_page(kit_table)
            self.pattern_notebook.set_tab_label_text(kit_table, _("Toy Kit"))
            self.pattern_notebook.set_tab_label_packing(kit_table, 0, 0, gtk.PACK_END)
            kit_table.show()

            pattern_notation_page[GEAR_NOTATION] = self.pattern_notebook.append_page(gear_table)
            self.pattern_notebook.set_tab_label_text(gear_table, _("Gears"))
            self.pattern_notebook.set_tab_label_packing(gear_table, 0, 0, gtk.PACK_END)
            gear_table.show()

            add_to_box(vbox, self.pattern_notebook)

            add_vertical_space(vbox, 14)

            hbox = gtk.HBox(spacing=5)
            pattern_table = create_table(1, 3, 5)

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

            vbox = gtk.VBox(spacing=0, homogeneous=False)

            add_vertical_space(vbox, 14)

            table = create_table(4, 2, 10)

            row = 0
            label_in_table(_("Shape"), table, row,
                           _("The shape of the fixed gear to be used inside current selection. "
                             "Rack is a long round-edged shape provided in the toy kits. "
                             "Frame hugs the boundaries of the rectangular selection, "
                             "use hole=100 in Gear notation to touch boundary. "
                             "Selection will hug boundaries of current selection - try something non-rectangular."))
            self.shape_combo = set_combo_in_table([shape.name for shape in shapes], table, row,
                                                  self.shape_combo_changed)

            row += 1
            label_in_table(_("Sides"), table, row, _("Number of sides of the shape."))
            self.sides_adj = gtk.Adjustment(self.p.sides, 3, 16, 1)
            self.sides_myscale = hscale_in_table(self.sides_adj, table, row, self.sides_changed)

            row += 1
            label_in_table(_("Morph"), table, row, _("Morph fixed gear shape. Only affects some of the shapes."))
            self.morph_adj = gtk.Adjustment(self.p.morph, 0.0, 1.0, 0.01)
            self.morph_myscale = hscale_in_table(self.morph_adj, table, row, self.morph_changed, digits=2)

            row += 1
            label_in_table(_("Rotation"), table, row, _("Rotation of the fixed gear, in degrees"))
            self.shape_rotation_adj, self.shape_rotation_myscale = rotation_in_table(
                self.p.shape_rotation, table, row, self.shape_rotation_changed
            )

            add_to_box(vbox, table)
            return vbox

        def size_page():

            vbox = gtk.VBox(spacing=0, homogeneous=False)
            add_vertical_space(vbox, 14)
            table = create_table(2, 2, 10)

            row = 0
            label_in_table(_("Margin (px)"), table, row, _("Margin from edge of selection."))
            self.margin_adj = gtk.Adjustment(self.p.margin_pixels, 0, max(img.height, img.width), 1)
            hscale_in_table(self.margin_adj, table, row, self.margin_changed)

            row += 1
            self.equal_w_h_checkbox = gtk.CheckButton(_("Make width and height equal"))
            self.equal_w_h_checkbox.set_tooltip_text(
                _("When unchecked, the pattern will fill the current image or selection. "
                  "When checked, the pattern will have same width and height, and will be centered.")
            )
            self.equal_w_h_checkbox.set_border_width(15)
            table.attach(self.equal_w_h_checkbox, 0, 2, row, row + 1)
            self.equal_w_h_checkbox.show()
            self.equal_w_h_checkbox.connect("toggled", self.equal_w_h_checkbox_changed)


            add_to_box(vbox, table)
            return vbox

        def add_button_to_box(box, text, callback, tooltip_text=None):
            btn = gtk.Button(text)
            if tooltip_text:
                btn.set_tooltip_text(tooltip_text)
            box.add(btn)
            btn.show()
            btn.connect("clicked", callback)
            return btn

        def dialog_button_box():
            hbox = gtk.HBox(homogeneous=True, spacing=20)

            add_button_to_box(hbox, _("Redraw"), self.redraw,
                              _("If you change the settings of a tool, change color, or change the selection, "
                                "press this to preview how the pattern looks."))
            add_button_to_box(hbox, _("Reset"), self.reset_params)
            add_button_to_box(hbox, _("Cancel"), self.cancel_window)
            self.ok_btn = add_button_to_box(hbox, _("OK"), self.ok_window)

            self.keep_separate_layer_checkbox = gtk.CheckButton(_("Keep\nLayer"))
            self.keep_separate_layer_checkbox.set_tooltip_text(
                _("If checked, then once OK is pressed, the spyro layer is kept, and the plugin exits quickly. "
                  "If unchecked, the spyro layer is deleted, and the pattern is redrawn on the layer that was "
                  "active when the plugin was launched.")
            )
            hbox.add(self.keep_separate_layer_checkbox)
            self.keep_separate_layer_checkbox.show()
            self.keep_separate_layer_checkbox.connect("toggled", self.keep_separate_layer_checkbox_changed)

            return hbox

        def create_ui():

            # Create the dialog
            gtk.Window.__init__(self)
            self.set_title(_("Spyrogimp"))
            self.set_default_size(350, -1)
            self.set_border_width(10)
            # self.set_keep_above(True) # keep the window on top

            # Vertical box in which we will add all the UI elements.
            vbox = gtk.VBox(spacing=10, homogeneous=False)
            self.add(vbox)

            box = gimpui.HintBox(_("Draw spyrographs using current tool settings and selection."))
            vbox.pack_start(box, expand=False)
            box.show()

            add_horizontal_separator(vbox)

            add_to_box(vbox, top_table())

            self.main_notebook = gtk.Notebook()
            self.main_notebook.set_show_tabs(True)
            self.main_notebook.set_border_width(5)

            pattern_frame = pattern_notation_frame()
            self.main_notebook.append_page(pattern_frame, gtk.Label(_("Curve Pattern")))
            pattern_frame.show()
            fixed_g_page = fixed_gear_page()
            self.main_notebook.append_page(fixed_g_page, gtk.Label(_("Fixed Gear")))
            fixed_g_page.show()
            size_p = size_page()
            self.main_notebook.append_page(size_p, gtk.Label(_("Size")))
            size_p.show()

            vbox.add(self.main_notebook)
            self.main_notebook.show()

            add_horizontal_separator(vbox)

            self.progress_bar = gtk.ProgressBar()   # gimpui.ProgressBar() - causes gimppdbprogress errror message.
            self.progress_bar.set_size_request(-1, 30)
            vbox.add(self.progress_bar)
            self.progress_bar.show()

            add_to_box(vbox, dialog_button_box())

            vbox.show()
            self.show()

        self.enable_incremental_drawing = False

        self.img = img
        # Remember active layer, so we can restore it when the plugin is done.
        self.active_layer = layer

        self.p = unshelf_parameters()  # Model

        self.engine = DrawingEngine(img, self.p)

        # Make a new GIMP layer to draw on
        self.spyro_layer = gimp.Layer(img, layer_name, img.width, img.height, RGBA_IMAGE, 100, NORMAL_MODE)
        img.add_layer(self.spyro_layer, 0)

        self.drawing_layer = self.spyro_layer

        gimpui.gimp_ui_init()
        create_ui()
        self.update_view()

        # Obey the window manager quit signal
        self.connect("destroy", self.cancel_window)
        # Connect Escape key to quit the window as well.
        self.connect('myescape', self.cancel_window)

        # Setup for Handling incremental/interactive drawing of pattern
        self.idle_task = None
        self.enable_incremental_drawing = True

        # Draw pattern of the current settings.
        self.start_new_incremental_drawing()

    # Callbacks for closing the plugin

    def ok_window(self, widget):
        """ Called when clicking on the 'close' button. """

        self.ok_btn.set_sensitive(False)

        shelf_parameters(self.p)

        if self.p.keep_separate_layer:
            if self.spyro_layer in self.img.layers:
                self.img.active_layer = self.spyro_layer

            # If we are in the middle of incremental draw, we want to complete it, and only then to exit.
            # However, in order to complete it, we need to create another idle task.
            if self.idle_task:
                def quit_dialog_on_completion():
                    while self.idle_task:
                        yield True

                    gtk.main_quit()  # This will quit the dialog.
                    yield False

                task = quit_dialog_on_completion()
                gobject.idle_add(task.next)
            else:
                gtk.main_quit()
        else:
            # If there is an incremental drawing taking place, lets stop it.
            if self.idle_task:
                gobject.source_remove(self.idle_task)

            if self.spyro_layer in self.img.layers:
                self.img.remove_layer(self.spyro_layer)
                self.img.active_layer = self.active_layer

            self.drawing_layer = self.active_layer

            def draw_full():
                self.progress_start()
                yield True

                self.engine.reset_incremental()

                self.img.undo_group_start()

                while self.engine.has_more_strokes():
                    yield True
                    self.draw_next_chunk(undo_group=False)

                self.img.undo_group_end()

                pdb.gimp_displays_flush()

                gtk.main_quit()
                yield False

            task = draw_full()
            gobject.idle_add(task.next)

    def cancel_window(self, widget, what=None):

        # Note that once we call gtk.main_quit, the idle task is stopped automatically.

        # We want to delete the temporary layer, but as a precaution, lets ask first,
        # maybe it was already deleted by the user.
        if self.spyro_layer in self.img.layers:
            self.img.remove_layer(self.spyro_layer)
            pdb.gimp_displays_flush()
        gtk.main_quit()

    def update_view(self):
        """ Update the UI to reflect the values in the Pattern Parameters. """
        self.curve_type_combo.set_active(self.p.curve_type)
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

        self.shape_combo.set_active(self.p.shape_index)
        self.shape_combo_side_effects()
        self.sides_adj.set_value(self.p.sides)
        self.morph_adj.set_value(self.p.morph)
        self.equal_w_h_checkbox.set_active(self.p.equal_w_h)
        self.shape_rotation_adj.set_value(self.p.shape_rotation)

        self.margin_adj.set_value(self.p.margin_pixels)
        self.tool_combo.set_active(self.p.tool_index)
        self.long_gradient_checkbox.set_active(self.p.long_gradient)
        self.keep_separate_layer_checkbox.set_active(self.p.keep_separate_layer)

    def reset_params(self, widget):
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
        else:
            # Lissajous curves do not have shapes, or holes for moving gear
            self.shape_combo.set_sensitive(False)

            self.sides_myscale.set_sensitive(False)
            self.morph_myscale.set_sensitive(False)
            self.shape_rotation_myscale.set_sensitive(False)

            self.hole_percent_myscale.set_sensitive(False)
            self.kit_hole_myscale.set_sensitive(False)

    def curve_type_changed(self, val):
        self.p.curve_type = val.get_active()
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
        self.p.hole_number = val.value
        self.redraw()

    # Callbacks: pattern changes using the Gears notation.

    def outer_teeth_changed(self, val):
        self.p.outer_teeth = val.value
        self.redraw()

    def inner_teeth_changed(self, val):
        self.p.inner_teeth = val.value
        self.redraw()

    def hole_percent_changed(self, val):
        self.p.hole_percent = val.value
        self.redraw()

    def pattern_rotation_changed(self, val):
        self.p.pattern_rotation = val.value
        self.redraw()

    # Callbacks: Fixed gear

    def shape_combo_side_effects(self):
        self.sides_myscale.set_sensitive(shapes[self.p.shape_index].has_sides())
        self.morph_myscale.set_sensitive(shapes[self.p.shape_index].can_morph())
        self.shape_rotation_myscale.set_sensitive(shapes[self.p.shape_index].can_rotate())
        self.equal_w_h_checkbox.set_sensitive(shapes[self.p.shape_index].can_equal_w_h())

    def shape_combo_changed(self, val):
        self.p.shape_index = val.get_active()
        self.shape_combo_side_effects()
        self.redraw()

    def sides_changed(self, val):
        self.p.sides = val.value
        self.redraw()

    def morph_changed(self, val):
        self.p.morph = val.value
        self.redraw()

    def equal_w_h_checkbox_changed(self, val):
        self.p.equal_w_h = val.get_active()
        self.redraw()

    def shape_rotation_changed(self, val):
        self.p.shape_rotation = val.value
        self.redraw()

    def margin_changed(self, val) :
        self.p.margin_pixels = val.value
        self.redraw()

    # Style callbacks

    def tool_changed_side_effects(self):
        self.long_gradient_checkbox.set_sensitive(tools[self.p.tool_index].can_color)

    def tool_combo_changed(self, val):
        self.p.tool_index = val.get_active()
        self.tool_changed_side_effects()
        self.redraw()

    def long_gradient_changed(self, val):
        self.p.long_gradient = val.get_active()
        self.redraw()

    def keep_separate_layer_checkbox_changed(self, val):
        self.p.keep_separate_layer = self.keep_separate_layer_checkbox.get_active()

    # Progress bar of plugin window.

    def progress_start(self):
        self.progress_bar.set_text(_("Rendering Pattern"))
        self.progress_bar.set_fraction(0.0)
        pdb.gimp_displays_flush()

    def progress_end(self):
        self.progress_bar.set_text("")
        self.progress_bar.set_fraction(0.0)

    def progress_update(self):
        self.progress_bar.set_fraction(self.engine.fraction_done())

    def progress_unknown(self):
        self.progress_bar.set_text(_("Please wait : Rendering Pattern"))
        self.progress_bar.pulse()
        pdb.gimp_displays_flush()

    # Incremental drawing.

    def draw_next_chunk(self, undo_group=True):
        """ Incremental drawing """

        t = time.time()

        if undo_group:
            self.img.undo_group_start()
        chunk_size = self.engine.draw_next_chunk(self.drawing_layer)
        if undo_group:
            self.img.undo_group_end()

        draw_time = time.time() - t
        self.engine.report_time(draw_time)
        print("Chunk size " + str(chunk_size) + " time " + str(draw_time))

        if self.engine.has_more_strokes():
            self.progress_update()
        else:
            self.progress_end()

        pdb.gimp_displays_flush()

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
            while self.engine.has_more_strokes():
                yield True
                self.draw_next_chunk()

            self.idle_task = None
            yield False

        # Remove old idle task if exists.
        if self.idle_task:
            gobject.source_remove(self.idle_task)

        # Start new idle task to perform incremental drawing in the background.
        task = incremental_drawing()
        self.idle_task = gobject.idle_add(task.next)

    def clear(self):
        """ Clear current drawing. """
        # pdb.gimp_edit_clear(self.spyro_layer)
        self.spyro_layer.fill(FILL_TRANSPARENT)

    def redraw(self, data=None):
        if self.enable_incremental_drawing:
            self.clear()
            self.start_new_incremental_drawing()


# Bind escape to the new signal we created, named "myescape".
gobject.type_register(SpyroWindow)
gtk.binding_entry_add_signal(SpyroWindow, gtk.keysyms.Escape, 0, 'myescape', str, 'escape')


class SpyrogimpPlusPlugin(gimpplugin.plugin):

    # Implementation of plugin.
    def plug_in_spyrogimp(self, run_mode, image, layer,
                          curve_type=0, shape=0, sides=3, morph=0.0,
                          fixed_teeth=96, moving_teeth=36, hole_percent=100.0,
                          margin=0, equal_w_h=0,
                          pattern_rotation=0.0, shape_rotation=0.0,
                          tool=1, long_gradient=False):
        if run_mode == RUN_NONINTERACTIVE:
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
            engine.draw_full(layer)

        elif run_mode == RUN_INTERACTIVE:
            window = SpyroWindow(image, layer)
            gtk.main()

        elif run_mode == RUN_WITH_LAST_VALS:
            pp = unshelf_parameters()
            engine = DrawingEngine(image, pp)
            engine.draw_full(layer)

    def query(self):
        plugin_name = "plug_in_spyrogimp"
        label = N_("Spyrogimp...")
        menu = "<Image>/Filters/Render/"

        params = [
            # (type, name, description
            (PDB_INT32, "run-mode", "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }"),
            (PDB_IMAGE, "image", "Input image"),
            (PDB_DRAWABLE, "drawable", "Input drawable"),
            (PDB_INT32, "curve_type",
             "The curve type { Spyrograph (0), Epitrochoid (1), Sine (2), Lissajous(3) }"),
            (PDB_INT32, "shape", "Shape of fixed gear"),
            (PDB_INT32, "sides", "Number of sides of fixed gear (3 or greater). Only used by some shapes."),
            (PDB_FLOAT, "morph", "Morph shape of fixed gear, between 0 and 1. Only used by some shapes."),
            (PDB_INT32, "fixed_teeth", "Number of teeth for fixed gear"),
            (PDB_INT32, "moving_teeth", "Number of teeth for moving gear"),
            (PDB_FLOAT, "hole_percent", "Location of hole in moving gear in percent, where 100 means that "
             "the hole is at the edge of the gear, and 0 means the hole is at the center"),
            (PDB_INT32, "margin", "Margin from selection, in pixels"),
            (PDB_INT32, "equal_w_h", "Make height and width equal (TRUE or FALSE)"),
            (PDB_FLOAT, "pattern_rotation", "Pattern rotation, in degrees"),
            (PDB_FLOAT, "shape_rotation", "Shape rotation of fixed gear, in degrees"),
            (PDB_INT32, "tool", "Tool to use for drawing the pattern."),
            (PDB_INT32, "long_gradient",
             "Whether to apply a long gradient to match the length of the pattern (TRUE or FALSE). "
             "Only applicable to some of the tools.")
        ]

        gimp.domain_register("gimp20-python", gimp.locale_directory)

        gimp.install_procedure(
            plugin_name,
            N_("Draw spyrographs using current tool settings and selection."),
            "Uses current tool settings to draw Spyrograph patterns. "
            "The size and location of the pattern is based on the current selection.",
            "Elad Shahar",
            "Elad Shahar",
            "2018",
            label,
            "*",
            PLUGIN,
            params,
            []
        )

        gimp.menu_register(plugin_name, menu)


if __name__ == '__main__':
    SpyrogimpPlusPlugin().start()
