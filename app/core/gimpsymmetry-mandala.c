/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpsymmetry-mandala.c
 * Copyright (C) 2015 Jehan <jehan@gimp.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpconfig/gimpconfig.h"
#include "libgimpmath/gimpmath.h"

#include "core-types.h"

#include "gimp.h"
#include "gimp-cairo.h"
#include "gimpdrawable.h"
#include "gimpguide.h"
#include "gimpimage.h"
#include "gimpimage-guides.h"
#include "gimpimage-symmetry.h"
#include "gimpitem.h"
#include "gimpsymmetry-mandala.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_CENTER_X,
  PROP_CENTER_Y,
  PROP_SIZE,
  PROP_DISABLE_TRANSFORMATION,
  PROP_ENABLE_REFLECTION,
};


/* Local function prototypes */

static void       gimp_mandala_constructed        (GObject      *object);
static void       gimp_mandala_finalize           (GObject      *object);
static void       gimp_mandala_set_property       (GObject      *object,
                                                   guint         property_id,
                                                   const GValue *value,
                                                   GParamSpec   *pspec);
static void       gimp_mandala_get_property       (GObject      *object,
                                                   guint         property_id,
                                                   GValue       *value,
                                                   GParamSpec   *pspec);
static void       gimp_mandala_active_changed     (GimpSymmetry *sym);

static void       gimp_mandala_add_guide          (GimpMandala         *mandala,
                                                   GimpOrientationType  orientation);
static void       gimp_mandala_remove_guide       (GimpMandala         *mandala,
                                                   GimpOrientationType  orientation);
static void       gimp_mandala_guide_removed_cb   (GObject      *object,
                                                   GimpMandala  *mandala);
static void       gimp_mandala_guide_position_cb  (GObject      *object,
                                                   GParamSpec   *pspec,
                                                   GimpMandala  *mandala);

static void       gimp_mandala_update_strokes     (GimpSymmetry *mandala,
                                                   GimpDrawable *drawable,
                                                   GimpCoords   *origin);
static void       gimp_mandala_get_transform      (GimpSymmetry *mandala,
                                                   gint          stroke,
                                                   gdouble      *angle,
                                                   gboolean     *reflect);
static void    gimp_mandala_image_size_changed_cb (GimpImage    *image,
                                                   gint          previous_origin_x,
                                                   gint          previous_origin_y,
                                                   gint          previous_width,
                                                   gint          previous_height,
                                                   GimpSymmetry *sym);


G_DEFINE_TYPE (GimpMandala, gimp_mandala, GIMP_TYPE_SYMMETRY)

#define parent_class gimp_mandala_parent_class


static void
gimp_mandala_class_init (GimpMandalaClass *klass)
{
  GObjectClass      *object_class   = G_OBJECT_CLASS (klass);
  GimpSymmetryClass *symmetry_class = GIMP_SYMMETRY_CLASS (klass);
  GParamSpec        *pspec;

  object_class->constructed         = gimp_mandala_constructed;
  object_class->finalize            = gimp_mandala_finalize;
  object_class->set_property        = gimp_mandala_set_property;
  object_class->get_property        = gimp_mandala_get_property;

  symmetry_class->label             = _("Mandala");
  symmetry_class->update_strokes    = gimp_mandala_update_strokes;
  symmetry_class->get_transform     = gimp_mandala_get_transform;
  symmetry_class->active_changed    = gimp_mandala_active_changed;

  GIMP_CONFIG_PROP_DOUBLE (object_class, PROP_CENTER_X,
                           "center-x",
                           _("Center abscissa"),
                           NULL,
                           0.0, G_MAXDOUBLE, 0.0,
                           GIMP_PARAM_STATIC_STRINGS |
                           GIMP_SYMMETRY_PARAM_GUI);

  pspec = g_object_class_find_property (object_class, "center-x");
  gegl_param_spec_set_property_key (pspec, "unit", "pixel-coordinate");
  gegl_param_spec_set_property_key (pspec, "axis", "x");

  GIMP_CONFIG_PROP_DOUBLE (object_class, PROP_CENTER_Y,
                           "center-y",
                           _("Center ordinate"),
                           NULL,
                           0.0, G_MAXDOUBLE, 0.0,
                           GIMP_PARAM_STATIC_STRINGS |
                           GIMP_SYMMETRY_PARAM_GUI);

  pspec = g_object_class_find_property (object_class, "center-y");
  gegl_param_spec_set_property_key (pspec, "unit", "pixel-coordinate");
  gegl_param_spec_set_property_key (pspec, "axis", "y");

  GIMP_CONFIG_PROP_INT (object_class, PROP_SIZE,
                        "size",
                        _("Number of points"),
                        NULL,
                        1, 100, 6.0,
                        GIMP_PARAM_STATIC_STRINGS |
                        GIMP_SYMMETRY_PARAM_GUI);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_DISABLE_TRANSFORMATION,
                            "disable-transformation",
                            _("Disable brush transform"),
                            _("Disable brush rotation"),
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS |
                            GIMP_SYMMETRY_PARAM_GUI);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_ENABLE_REFLECTION,
                            "enable-reflection",
                            _("Kaleidoscope"),
                            _("Reflect consecutive strokes"),
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS |
                            GIMP_SYMMETRY_PARAM_GUI);
}

static void
gimp_mandala_init (GimpMandala *mandala)
{
}

static void
gimp_mandala_constructed (GObject *object)
{
  GimpSymmetry *sym = GIMP_SYMMETRY (object);

  g_signal_connect_object (sym->image, "size-changed-detailed",
                           G_CALLBACK (gimp_mandala_image_size_changed_cb),
                           sym, 0);
}

static void
gimp_mandala_finalize (GObject *object)
{
  GimpMandala  *mandala = GIMP_MANDALA (object);

  g_clear_object (&mandala->horizontal_guide);
  g_clear_object (&mandala->vertical_guide);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_mandala_set_property (GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  GimpMandala *mandala = GIMP_MANDALA (object);
  GimpImage   *image   = GIMP_SYMMETRY (mandala)->image;

  switch (property_id)
    {
    case PROP_CENTER_X:
      if (g_value_get_double (value) > 0.0 &&
          g_value_get_double (value) < (gdouble) gimp_image_get_width (image))
        {
          mandala->center_x = g_value_get_double (value);

          if (mandala->vertical_guide)
            {
              g_signal_handlers_block_by_func (mandala->vertical_guide,
                                               gimp_mandala_guide_position_cb,
                                               mandala);
              gimp_image_move_guide (image, mandala->vertical_guide,
                                     mandala->center_x,
                                     FALSE);
              g_signal_handlers_unblock_by_func (mandala->vertical_guide,
                                                 gimp_mandala_guide_position_cb,
                                                 mandala);
            }
        }
      break;

    case PROP_CENTER_Y:
      if (g_value_get_double (value) > 0.0 &&
          g_value_get_double (value) < (gdouble) gimp_image_get_height (image))
        {
          mandala->center_y = g_value_get_double (value);

          if (mandala->horizontal_guide)
            {
              g_signal_handlers_block_by_func (mandala->horizontal_guide,
                                               gimp_mandala_guide_position_cb,
                                               mandala);
              gimp_image_move_guide (image, mandala->horizontal_guide,
                                     mandala->center_y,
                                     FALSE);
              g_signal_handlers_unblock_by_func (mandala->horizontal_guide,
                                                 gimp_mandala_guide_position_cb,
                                                 mandala);
            }
        }
      break;

    case PROP_SIZE:
      mandala->size = g_value_get_int (value);
      break;

    case PROP_DISABLE_TRANSFORMATION:
      mandala->disable_transformation = g_value_get_boolean (value);
      break;

    case PROP_ENABLE_REFLECTION:
      mandala->enable_reflection = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_mandala_get_property (GObject    *object,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  GimpMandala *mandala = GIMP_MANDALA (object);

  switch (property_id)
    {
    case PROP_CENTER_X:
      g_value_set_double (value, mandala->center_x);
      break;
    case PROP_CENTER_Y:
      g_value_set_double (value, mandala->center_y);
      break;
    case PROP_SIZE:
      g_value_set_int (value, mandala->size);
      break;
    case PROP_DISABLE_TRANSFORMATION:
      g_value_set_boolean (value, mandala->disable_transformation);
      break;
    case PROP_ENABLE_REFLECTION:
      g_value_set_boolean (value, mandala->enable_reflection);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_mandala_active_changed (GimpSymmetry *sym)
{
  GimpMandala *mandala = GIMP_MANDALA (sym);

  if (sym->active)
    {
      if (! mandala->horizontal_guide)
        gimp_mandala_add_guide (mandala, GIMP_ORIENTATION_HORIZONTAL);

      if (! mandala->vertical_guide)
        gimp_mandala_add_guide (mandala, GIMP_ORIENTATION_VERTICAL);
    }
  else
    {
      if (mandala->horizontal_guide)
        gimp_mandala_remove_guide (mandala, GIMP_ORIENTATION_HORIZONTAL);

      if (mandala->vertical_guide)
        gimp_mandala_remove_guide (mandala, GIMP_ORIENTATION_VERTICAL);
    }
}

static void
gimp_mandala_add_guide (GimpMandala         *mandala,
                        GimpOrientationType  orientation)
{
  GimpSymmetry *sym = GIMP_SYMMETRY (mandala);
  GimpImage    *image;
  Gimp         *gimp;
  GimpGuide    *guide;
  gint          position;

  image = sym->image;
  gimp  = image->gimp;

  guide = gimp_guide_custom_new (orientation,
                                 gimp->next_guide_ID++,
                                 GIMP_GUIDE_STYLE_MANDALA);

  if (orientation == GIMP_ORIENTATION_HORIZONTAL)
    {
      mandala->horizontal_guide = guide;

      /* Mandala guide position at first activation is at canvas middle. */
      if (mandala->center_y < 1.0)
        mandala->center_y = (gdouble) gimp_image_get_height (image) / 2.0;

      position = (gint) mandala->center_y;
    }
  else
    {
      mandala->vertical_guide = guide;

      /* Mandala guide position at first activation is at canvas middle. */
      if (mandala->center_x < 1.0)
        mandala->center_x = (gdouble) gimp_image_get_width (image) / 2.0;

      position = (gint) mandala->center_x;
    }

  g_signal_connect (guide, "removed",
                    G_CALLBACK (gimp_mandala_guide_removed_cb),
                    mandala);

  gimp_image_add_guide (image, guide, position);

  g_signal_connect (guide, "notify::position",
                    G_CALLBACK (gimp_mandala_guide_position_cb),
                    mandala);
}

static void
gimp_mandala_remove_guide (GimpMandala         *mandala,
                           GimpOrientationType  orientation)
{
  GimpSymmetry *sym = GIMP_SYMMETRY (mandala);
  GimpImage    *image;
  GimpGuide    *guide;

  image = sym->image;
  guide = (orientation == GIMP_ORIENTATION_HORIZONTAL) ?
    mandala->horizontal_guide : mandala->vertical_guide;

  g_signal_handlers_disconnect_by_func (guide,
                                        gimp_mandala_guide_removed_cb,
                                        mandala);
  g_signal_handlers_disconnect_by_func (guide,
                                        gimp_mandala_guide_position_cb,
                                        mandala);

  gimp_image_remove_guide (image, guide, FALSE);
  g_object_unref (guide);

  if (orientation == GIMP_ORIENTATION_HORIZONTAL)
    mandala->horizontal_guide = NULL;
  else
    mandala->vertical_guide = NULL;
}

static void
gimp_mandala_guide_removed_cb (GObject     *object,
                               GimpMandala *mandala)
{
  GimpSymmetry *sym = GIMP_SYMMETRY (mandala);

  g_signal_handlers_disconnect_by_func (object,
                                        gimp_mandala_guide_removed_cb,
                                        mandala);
  g_signal_handlers_disconnect_by_func (object,
                                        gimp_mandala_guide_position_cb,
                                        mandala);

  if (GIMP_GUIDE (object) == mandala->horizontal_guide)
    {
      g_object_unref (mandala->horizontal_guide);

      mandala->horizontal_guide = NULL;
      mandala->center_y         = 0.0;

      gimp_mandala_remove_guide (mandala, GIMP_ORIENTATION_VERTICAL);
    }
  else if (GIMP_GUIDE (object) == mandala->vertical_guide)
    {
      g_object_unref (mandala->vertical_guide);
      mandala->vertical_guide = NULL;
      mandala->center_x       = 0.0;

      gimp_mandala_remove_guide (mandala, GIMP_ORIENTATION_HORIZONTAL);
    }

  gimp_image_symmetry_remove (sym->image, sym);
}

static void
gimp_mandala_guide_position_cb (GObject     *object,
                                GParamSpec  *pspec,
                                GimpMandala *mandala)
{
  GimpGuide *guide = GIMP_GUIDE (object);

  if (guide == mandala->horizontal_guide)
    {
      g_object_set (G_OBJECT (mandala),
                    "center-y", (gdouble) gimp_guide_get_position (guide),
                    NULL);
    }
  else if (guide == mandala->vertical_guide)
    {
      g_object_set (G_OBJECT (mandala),
                    "center-x", (gdouble) gimp_guide_get_position (guide),
                    NULL);
    }
}

static void
gimp_mandala_update_strokes (GimpSymmetry *sym,
                             GimpDrawable *drawable,
                             GimpCoords   *origin)
{
  GimpMandala *mandala = GIMP_MANDALA (sym);
  GimpCoords  *coords;
  GimpMatrix3  matrix;
  gint         i;
  gdouble slice_angle;
  gdouble mid_slice_angle = 0.0;

  g_list_free_full (sym->strokes, g_free);
  sym->strokes = NULL;

  coords = g_memdup (sym->origin, sizeof (GimpCoords));
  sym->strokes = g_list_prepend (sym->strokes, coords);

  /* The angle of each slice, in radians. */
  slice_angle = 2.0 * G_PI / mandala->size;

  if (mandala->enable_reflection)
    {
      /* Find out in which slice the user is currently drawing. */
      gdouble angle = atan2 (sym->origin->y - mandala->center_y,
                             sym->origin->x - mandala->center_x);
      gint slice_no = (int) floor(angle/slice_angle);

      /* Angle where the middle of that slice is. */
      mid_slice_angle = slice_no * slice_angle + slice_angle / 2.0;
    }

  for (i = 1; i < mandala->size; i++)
    {
      gdouble new_x, new_y;

      coords = g_memdup (sym->origin, sizeof (GimpCoords));
      gimp_matrix3_identity (&matrix);
      gimp_matrix3_translate (&matrix,
                              - mandala->center_x,
                              - mandala->center_y);
      if (mandala->enable_reflection && i % 2 == 1)
        {
          /* Reflecting over the mid_slice_angle axis, reflects slice without changing position. */
          gimp_matrix3_rotate(&matrix, -mid_slice_angle);
          gimp_matrix3_scale (&matrix, 1, -1);
          gimp_matrix3_rotate(&matrix, mid_slice_angle - i * slice_angle);
        }
      else
        {
          gimp_matrix3_rotate (&matrix, - i * slice_angle);
        }
      gimp_matrix3_translate (&matrix,
                              mandala->center_x,
                              mandala->center_y);
      gimp_matrix3_transform_point (&matrix,
                                    coords->x,
                                    coords->y,
                                    &new_x,
                                    &new_y);
      coords->x = new_x;
      coords->y = new_y;
      sym->strokes = g_list_prepend (sym->strokes, coords);
    }

  sym->strokes = g_list_reverse (sym->strokes);

  g_signal_emit_by_name (sym, "strokes-updated", sym->image);
}

static void
gimp_mandala_get_transform (GimpSymmetry *sym,
                            gint          stroke,
                            gdouble      *angle,
                            gboolean     *reflect)
{
  GimpMandala *mandala = GIMP_MANDALA (sym);
  gdouble     slice_angle;

  if (mandala->disable_transformation)
    return;

  slice_angle = 360.0 / mandala->size;

  if (mandala->enable_reflection && stroke % 2 == 1)
    {
      /* Find out in which slice the user is currently drawing. */
      gdouble origin_angle = gimp_rad_to_deg (atan2 (sym->origin->y - mandala->center_y,
                                                     sym->origin->x - mandala->center_x));
      gint slice_no = (int) floor(origin_angle/slice_angle);

      /* Angle where the middle of that slice is. */
      gdouble mid_slice_angle = slice_no * slice_angle + slice_angle / 2.0;

      *angle = 180.0 - (-2 * mid_slice_angle + stroke * slice_angle) ;
      *reflect = TRUE;
    }
  else
    {
      *angle = stroke * slice_angle;
    }
}

static void
gimp_mandala_image_size_changed_cb (GimpImage    *image,
                                    gint          previous_origin_x,
                                    gint          previous_origin_y,
                                    gint          previous_width,
                                    gint          previous_height,
                                    GimpSymmetry *sym)
{
  if (previous_width != gimp_image_get_width (image) ||
      previous_height != gimp_image_get_height (image))
    {
      g_signal_emit_by_name (sym, "gui-param-changed", sym->image);
    }
}
