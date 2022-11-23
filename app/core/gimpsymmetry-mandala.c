/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmasymmetry-mandala.c
 * Copyright (C) 2015 Jehan <jehan@ligma.org>
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

#include "libligmabase/ligmabase.h"
#include "libligmaconfig/ligmaconfig.h"
#include "libligmamath/ligmamath.h"

#include "core-types.h"

#include "ligma.h"
#include "ligma-cairo.h"
#include "ligmadrawable.h"
#include "ligmaguide.h"
#include "ligmaimage.h"
#include "ligmaimage-guides.h"
#include "ligmaimage-symmetry.h"
#include "ligmaitem.h"
#include "ligmasymmetry-mandala.h"

#include "ligma-intl.h"


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

static void       ligma_mandala_constructed        (GObject      *object);
static void       ligma_mandala_finalize           (GObject      *object);
static void       ligma_mandala_set_property       (GObject      *object,
                                                   guint         property_id,
                                                   const GValue *value,
                                                   GParamSpec   *pspec);
static void       ligma_mandala_get_property       (GObject      *object,
                                                   guint         property_id,
                                                   GValue       *value,
                                                   GParamSpec   *pspec);
static void       ligma_mandala_active_changed     (LigmaSymmetry *sym);

static void       ligma_mandala_add_guide          (LigmaMandala         *mandala,
                                                   LigmaOrientationType  orientation);
static void       ligma_mandala_remove_guide       (LigmaMandala         *mandala,
                                                   LigmaOrientationType  orientation);
static void       ligma_mandala_guide_removed_cb   (GObject      *object,
                                                   LigmaMandala  *mandala);
static void       ligma_mandala_guide_position_cb  (GObject      *object,
                                                   GParamSpec   *pspec,
                                                   LigmaMandala  *mandala);

static void       ligma_mandala_update_strokes     (LigmaSymmetry *mandala,
                                                   LigmaDrawable *drawable,
                                                   LigmaCoords   *origin);
static void       ligma_mandala_get_transform      (LigmaSymmetry *mandala,
                                                   gint          stroke,
                                                   gdouble      *angle,
                                                   gboolean     *reflect);
static void    ligma_mandala_image_size_changed_cb (LigmaImage    *image,
                                                   gint          previous_origin_x,
                                                   gint          previous_origin_y,
                                                   gint          previous_width,
                                                   gint          previous_height,
                                                   LigmaSymmetry *sym);


G_DEFINE_TYPE (LigmaMandala, ligma_mandala, LIGMA_TYPE_SYMMETRY)

#define parent_class ligma_mandala_parent_class


static void
ligma_mandala_class_init (LigmaMandalaClass *klass)
{
  GObjectClass      *object_class   = G_OBJECT_CLASS (klass);
  LigmaSymmetryClass *symmetry_class = LIGMA_SYMMETRY_CLASS (klass);
  GParamSpec        *pspec;

  object_class->constructed         = ligma_mandala_constructed;
  object_class->finalize            = ligma_mandala_finalize;
  object_class->set_property        = ligma_mandala_set_property;
  object_class->get_property        = ligma_mandala_get_property;

  symmetry_class->label             = _("Mandala");
  symmetry_class->update_strokes    = ligma_mandala_update_strokes;
  symmetry_class->get_transform     = ligma_mandala_get_transform;
  symmetry_class->active_changed    = ligma_mandala_active_changed;

  LIGMA_CONFIG_PROP_DOUBLE (object_class, PROP_CENTER_X,
                           "center-x",
                           _("Center abscissa"),
                           NULL,
                           0.0, G_MAXDOUBLE, 0.0,
                           LIGMA_PARAM_STATIC_STRINGS |
                           LIGMA_SYMMETRY_PARAM_GUI);

  pspec = g_object_class_find_property (object_class, "center-x");
  gegl_param_spec_set_property_key (pspec, "unit", "pixel-coordinate");
  gegl_param_spec_set_property_key (pspec, "axis", "x");

  LIGMA_CONFIG_PROP_DOUBLE (object_class, PROP_CENTER_Y,
                           "center-y",
                           _("Center ordinate"),
                           NULL,
                           0.0, G_MAXDOUBLE, 0.0,
                           LIGMA_PARAM_STATIC_STRINGS |
                           LIGMA_SYMMETRY_PARAM_GUI);

  pspec = g_object_class_find_property (object_class, "center-y");
  gegl_param_spec_set_property_key (pspec, "unit", "pixel-coordinate");
  gegl_param_spec_set_property_key (pspec, "axis", "y");

  LIGMA_CONFIG_PROP_INT (object_class, PROP_SIZE,
                        "size",
                        _("Number of points"),
                        NULL,
                        1, 100, 6.0,
                        LIGMA_PARAM_STATIC_STRINGS |
                        LIGMA_SYMMETRY_PARAM_GUI);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_DISABLE_TRANSFORMATION,
                            "disable-transformation",
                            _("Disable brush transform"),
                            _("Disable brush rotation"),
                            FALSE,
                            LIGMA_PARAM_STATIC_STRINGS |
                            LIGMA_SYMMETRY_PARAM_GUI);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_ENABLE_REFLECTION,
                            "enable-reflection",
                            _("Kaleidoscope"),
                            _("Reflect consecutive strokes"),
                            FALSE,
                            LIGMA_PARAM_STATIC_STRINGS |
                            LIGMA_SYMMETRY_PARAM_GUI);
}

static void
ligma_mandala_init (LigmaMandala *mandala)
{
}

static void
ligma_mandala_constructed (GObject *object)
{
  LigmaSymmetry *sym = LIGMA_SYMMETRY (object);

  g_signal_connect_object (sym->image, "size-changed-detailed",
                           G_CALLBACK (ligma_mandala_image_size_changed_cb),
                           sym, 0);
}

static void
ligma_mandala_finalize (GObject *object)
{
  LigmaMandala  *mandala = LIGMA_MANDALA (object);

  g_clear_object (&mandala->horizontal_guide);
  g_clear_object (&mandala->vertical_guide);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_mandala_set_property (GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  LigmaMandala *mandala = LIGMA_MANDALA (object);
  LigmaImage   *image   = LIGMA_SYMMETRY (mandala)->image;

  switch (property_id)
    {
    case PROP_CENTER_X:
      if (g_value_get_double (value) > 0.0 &&
          g_value_get_double (value) < (gdouble) ligma_image_get_width (image))
        {
          mandala->center_x = g_value_get_double (value);

          if (mandala->vertical_guide)
            {
              g_signal_handlers_block_by_func (mandala->vertical_guide,
                                               ligma_mandala_guide_position_cb,
                                               mandala);
              ligma_image_move_guide (image, mandala->vertical_guide,
                                     mandala->center_x,
                                     FALSE);
              g_signal_handlers_unblock_by_func (mandala->vertical_guide,
                                                 ligma_mandala_guide_position_cb,
                                                 mandala);
            }
        }
      break;

    case PROP_CENTER_Y:
      if (g_value_get_double (value) > 0.0 &&
          g_value_get_double (value) < (gdouble) ligma_image_get_height (image))
        {
          mandala->center_y = g_value_get_double (value);

          if (mandala->horizontal_guide)
            {
              g_signal_handlers_block_by_func (mandala->horizontal_guide,
                                               ligma_mandala_guide_position_cb,
                                               mandala);
              ligma_image_move_guide (image, mandala->horizontal_guide,
                                     mandala->center_y,
                                     FALSE);
              g_signal_handlers_unblock_by_func (mandala->horizontal_guide,
                                                 ligma_mandala_guide_position_cb,
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
ligma_mandala_get_property (GObject    *object,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  LigmaMandala *mandala = LIGMA_MANDALA (object);

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
ligma_mandala_active_changed (LigmaSymmetry *sym)
{
  LigmaMandala *mandala = LIGMA_MANDALA (sym);

  if (sym->active)
    {
      if (! mandala->horizontal_guide)
        ligma_mandala_add_guide (mandala, LIGMA_ORIENTATION_HORIZONTAL);

      if (! mandala->vertical_guide)
        ligma_mandala_add_guide (mandala, LIGMA_ORIENTATION_VERTICAL);
    }
  else
    {
      if (mandala->horizontal_guide)
        ligma_mandala_remove_guide (mandala, LIGMA_ORIENTATION_HORIZONTAL);

      if (mandala->vertical_guide)
        ligma_mandala_remove_guide (mandala, LIGMA_ORIENTATION_VERTICAL);
    }
}

static void
ligma_mandala_add_guide (LigmaMandala         *mandala,
                        LigmaOrientationType  orientation)
{
  LigmaSymmetry *sym = LIGMA_SYMMETRY (mandala);
  LigmaImage    *image;
  Ligma         *ligma;
  LigmaGuide    *guide;
  gint          position;

  image = sym->image;
  ligma  = image->ligma;

  guide = ligma_guide_custom_new (orientation,
                                 ligma->next_guide_id++,
                                 LIGMA_GUIDE_STYLE_MANDALA);

  if (orientation == LIGMA_ORIENTATION_HORIZONTAL)
    {
      mandala->horizontal_guide = guide;

      /* Mandala guide position at first activation is at canvas middle. */
      if (mandala->center_y < 1.0)
        mandala->center_y = (gdouble) ligma_image_get_height (image) / 2.0;

      position = (gint) mandala->center_y;
    }
  else
    {
      mandala->vertical_guide = guide;

      /* Mandala guide position at first activation is at canvas middle. */
      if (mandala->center_x < 1.0)
        mandala->center_x = (gdouble) ligma_image_get_width (image) / 2.0;

      position = (gint) mandala->center_x;
    }

  g_signal_connect (guide, "removed",
                    G_CALLBACK (ligma_mandala_guide_removed_cb),
                    mandala);

  ligma_image_add_guide (image, guide, position);

  g_signal_connect (guide, "notify::position",
                    G_CALLBACK (ligma_mandala_guide_position_cb),
                    mandala);
}

static void
ligma_mandala_remove_guide (LigmaMandala         *mandala,
                           LigmaOrientationType  orientation)
{
  LigmaSymmetry *sym = LIGMA_SYMMETRY (mandala);
  LigmaImage    *image;
  LigmaGuide    *guide;

  image = sym->image;
  guide = (orientation == LIGMA_ORIENTATION_HORIZONTAL) ?
    mandala->horizontal_guide : mandala->vertical_guide;

  g_signal_handlers_disconnect_by_func (guide,
                                        ligma_mandala_guide_removed_cb,
                                        mandala);
  g_signal_handlers_disconnect_by_func (guide,
                                        ligma_mandala_guide_position_cb,
                                        mandala);

  ligma_image_remove_guide (image, guide, FALSE);
  g_object_unref (guide);

  if (orientation == LIGMA_ORIENTATION_HORIZONTAL)
    mandala->horizontal_guide = NULL;
  else
    mandala->vertical_guide = NULL;
}

static void
ligma_mandala_guide_removed_cb (GObject     *object,
                               LigmaMandala *mandala)
{
  LigmaSymmetry *sym = LIGMA_SYMMETRY (mandala);

  g_signal_handlers_disconnect_by_func (object,
                                        ligma_mandala_guide_removed_cb,
                                        mandala);
  g_signal_handlers_disconnect_by_func (object,
                                        ligma_mandala_guide_position_cb,
                                        mandala);

  if (LIGMA_GUIDE (object) == mandala->horizontal_guide)
    {
      g_object_unref (mandala->horizontal_guide);

      mandala->horizontal_guide = NULL;
      mandala->center_y         = 0.0;

      ligma_mandala_remove_guide (mandala, LIGMA_ORIENTATION_VERTICAL);
    }
  else if (LIGMA_GUIDE (object) == mandala->vertical_guide)
    {
      g_object_unref (mandala->vertical_guide);
      mandala->vertical_guide = NULL;
      mandala->center_x       = 0.0;

      ligma_mandala_remove_guide (mandala, LIGMA_ORIENTATION_HORIZONTAL);
    }

  ligma_image_symmetry_remove (sym->image, sym);
}

static void
ligma_mandala_guide_position_cb (GObject     *object,
                                GParamSpec  *pspec,
                                LigmaMandala *mandala)
{
  LigmaGuide *guide = LIGMA_GUIDE (object);

  if (guide == mandala->horizontal_guide)
    {
      g_object_set (G_OBJECT (mandala),
                    "center-y", (gdouble) ligma_guide_get_position (guide),
                    NULL);
    }
  else if (guide == mandala->vertical_guide)
    {
      g_object_set (G_OBJECT (mandala),
                    "center-x", (gdouble) ligma_guide_get_position (guide),
                    NULL);
    }
}

static void
ligma_mandala_update_strokes (LigmaSymmetry *sym,
                             LigmaDrawable *drawable,
                             LigmaCoords   *origin)
{
  LigmaMandala *mandala = LIGMA_MANDALA (sym);
  LigmaCoords  *coords;
  LigmaMatrix3  matrix;
  gdouble      slice_angle;
  gdouble      mid_slice_angle = 0.0;
  gdouble      center_x, center_y;
  gint         offset_x, offset_y;
  gint         i;

  ligma_item_get_offset (LIGMA_ITEM (drawable), &offset_x, &offset_y);

  center_x = mandala->center_x - offset_x;
  center_y = mandala->center_y - offset_y;

  g_list_free_full (sym->strokes, g_free);
  sym->strokes = NULL;

  coords = g_memdup2 (sym->origin, sizeof (LigmaCoords));
  sym->strokes = g_list_prepend (sym->strokes, coords);

  /* The angle of each slice, in radians. */
  slice_angle = 2.0 * G_PI / mandala->size;

  if (mandala->enable_reflection)
    {
      /* Find out in which slice the user is currently drawing. */
      gdouble angle = atan2 (sym->origin->y - center_y,
                             sym->origin->x - center_x);
      gint slice_no = (int) floor(angle/slice_angle);

      /* Angle where the middle of that slice is. */
      mid_slice_angle = slice_no * slice_angle + slice_angle / 2.0;
    }

  for (i = 1; i < mandala->size; i++)
    {
      gdouble new_x, new_y;

      coords = g_memdup2 (sym->origin, sizeof (LigmaCoords));
      ligma_matrix3_identity (&matrix);
      ligma_matrix3_translate (&matrix,
                              -center_x,
                              -center_y);
      if (mandala->enable_reflection && i % 2 == 1)
        {
          /* Reflecting over the mid_slice_angle axis, reflects slice without changing position. */
          ligma_matrix3_rotate(&matrix, -mid_slice_angle);
          ligma_matrix3_scale (&matrix, 1, -1);
          ligma_matrix3_rotate(&matrix, mid_slice_angle - i * slice_angle);
        }
      else
        {
          ligma_matrix3_rotate (&matrix, - i * slice_angle);
        }
      ligma_matrix3_translate (&matrix,
                              +center_x,
                              +center_y);
      ligma_matrix3_transform_point (&matrix,
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
ligma_mandala_get_transform (LigmaSymmetry *sym,
                            gint          stroke,
                            gdouble      *angle,
                            gboolean     *reflect)
{
  LigmaMandala *mandala = LIGMA_MANDALA (sym);
  gdouble     slice_angle;

  if (mandala->disable_transformation)
    return;

  slice_angle = 360.0 / mandala->size;

  if (mandala->enable_reflection && stroke % 2 == 1)
    {
      /* Find out in which slice the user is currently drawing. */
      gdouble origin_angle = ligma_rad_to_deg (atan2 (sym->origin->y - mandala->center_y,
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
ligma_mandala_image_size_changed_cb (LigmaImage    *image,
                                    gint          previous_origin_x,
                                    gint          previous_origin_y,
                                    gint          previous_width,
                                    gint          previous_height,
                                    LigmaSymmetry *sym)
{
  if (previous_width != ligma_image_get_width (image) ||
      previous_height != ligma_image_get_height (image))
    {
      g_signal_emit_by_name (sym, "gui-param-changed", sym->image);
    }
}
