/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpsymmetry-mirror.c
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

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"

#include "core-types.h"

#include "gimp.h"
#include "gimp-cairo.h"
#include "gimpbrush.h"
#include "gimpguide.h"
#include "gimpimage.h"
#include "gimpimage-guides.h"
#include "gimpimage-symmetry.h"
#include "gimpitem.h"
#include "gimpsymmetry-mirror.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_HORIZONTAL_SYMMETRY,
  PROP_VERTICAL_SYMMETRY,
  PROP_POINT_SYMMETRY,
  PROP_DISABLE_TRANSFORMATION,
  PROP_MIRROR_POSITION_X,
  PROP_MIRROR_POSITION_Y
};


/* Local function prototypes */

static void       gimp_mirror_constructed             (GObject             *object);
static void       gimp_mirror_finalize                (GObject             *object);
static void       gimp_mirror_set_property            (GObject             *object,
                                                       guint                property_id,
                                                       const GValue        *value,
                                                       GParamSpec          *pspec);
static void       gimp_mirror_get_property            (GObject             *object,
                                                       guint                property_id,
                                                       GValue              *value,
                                                       GParamSpec          *pspec);

static void       gimp_mirror_update_strokes          (GimpSymmetry        *mirror,
                                                       GimpDrawable        *drawable,
                                                       GimpCoords          *origin);
static void       gimp_mirror_get_transform           (GimpSymmetry        *mirror,
                                                       gint                 stroke,
                                                       gdouble             *angle,
                                                       gboolean            *reflect);
static void       gimp_mirror_reset                   (GimpMirror          *mirror);
static void       gimp_mirror_add_guide               (GimpMirror          *mirror,
                                                       GimpOrientationType  orientation);
static void       gimp_mirror_remove_guide            (GimpMirror          *mirror,
                                                       GimpOrientationType  orientation);
static void       gimp_mirror_guide_removed_cb        (GObject             *object,
                                                       GimpMirror          *mirror);
static void       gimp_mirror_guide_position_cb       (GObject             *object,
                                                       GParamSpec          *pspec,
                                                       GimpMirror          *mirror);
static void       gimp_mirror_active_changed          (GimpSymmetry        *sym);
static void       gimp_mirror_set_horizontal_symmetry (GimpMirror          *mirror,
                                                       gboolean             active);
static void       gimp_mirror_set_vertical_symmetry   (GimpMirror          *mirror,
                                                       gboolean             active);
static void       gimp_mirror_set_point_symmetry      (GimpMirror          *mirror,
                                                       gboolean             active);

static void       gimp_mirror_image_size_changed_cb   (GimpImage           *image,
                                                       gint                 previous_origin_x,
                                                       gint                 previous_origin_y,
                                                       gint                 previous_width,
                                                       gint                 previous_height,
                                                       GimpSymmetry        *sym);

G_DEFINE_TYPE (GimpMirror, gimp_mirror, GIMP_TYPE_SYMMETRY)

#define parent_class gimp_mirror_parent_class


static void
gimp_mirror_class_init (GimpMirrorClass *klass)
{
  GObjectClass      *object_class   = G_OBJECT_CLASS (klass);
  GimpSymmetryClass *symmetry_class = GIMP_SYMMETRY_CLASS (klass);
  GParamSpec        *pspec;

  object_class->constructed         = gimp_mirror_constructed;
  object_class->finalize            = gimp_mirror_finalize;
  object_class->set_property        = gimp_mirror_set_property;
  object_class->get_property        = gimp_mirror_get_property;

  symmetry_class->label             = _("Mirror");
  symmetry_class->update_strokes    = gimp_mirror_update_strokes;
  symmetry_class->get_transform     = gimp_mirror_get_transform;
  symmetry_class->active_changed    = gimp_mirror_active_changed;

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_HORIZONTAL_SYMMETRY,
                            "horizontal-symmetry",
                            _("Horizontal symmetry"),
                            _("Reflect the initial stroke across a horizontal axis"),
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS |
                            GIMP_SYMMETRY_PARAM_GUI);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_VERTICAL_SYMMETRY,
                            "vertical-symmetry",
                            _("Vertical symmetry"),
                            _("Reflect the initial stroke across a vertical axis"),
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS |
                            GIMP_SYMMETRY_PARAM_GUI);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_POINT_SYMMETRY,
                            "point-symmetry",
                            _("Central symmetry"),
                            _("Invert the initial stroke through a point"),
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS |
                            GIMP_SYMMETRY_PARAM_GUI);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_DISABLE_TRANSFORMATION,
                            "disable-transformation",
                            _("Disable brush transform"),
                            _("Disable brush reflection"),
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS |
                            GIMP_SYMMETRY_PARAM_GUI);

  GIMP_CONFIG_PROP_DOUBLE (object_class, PROP_MIRROR_POSITION_X,
                           "mirror-position-x",
                           _("Vertical axis position"),
                           NULL,
                           0.0, G_MAXDOUBLE, 0.0,
                           GIMP_PARAM_STATIC_STRINGS |
                           GIMP_SYMMETRY_PARAM_GUI);

  pspec = g_object_class_find_property (object_class, "mirror-position-x");
  gegl_param_spec_set_property_key (pspec, "unit", "pixel-coordinate");
  gegl_param_spec_set_property_key (pspec, "axis", "x");

  GIMP_CONFIG_PROP_DOUBLE (object_class, PROP_MIRROR_POSITION_Y,
                           "mirror-position-y",
                           _("Horizontal axis position"),
                           NULL,
                           0.0, G_MAXDOUBLE, 0.0,
                           GIMP_PARAM_STATIC_STRINGS |
                           GIMP_SYMMETRY_PARAM_GUI);

  pspec = g_object_class_find_property (object_class, "mirror-position-y");
  gegl_param_spec_set_property_key (pspec, "unit", "pixel-coordinate");
  gegl_param_spec_set_property_key (pspec, "axis", "y");
}

static void
gimp_mirror_init (GimpMirror *mirror)
{
}

static void
gimp_mirror_constructed (GObject *object)
{
  GimpSymmetry *sym = GIMP_SYMMETRY (object);

  g_signal_connect_object (sym->image, "size-changed-detailed",
                           G_CALLBACK (gimp_mirror_image_size_changed_cb),
                           sym, 0);
}

static void
gimp_mirror_finalize (GObject *object)
{
  GimpMirror *mirror = GIMP_MIRROR (object);

  g_clear_object (&mirror->horizontal_guide);
  g_clear_object (&mirror->vertical_guide);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_mirror_set_property (GObject      *object,
                          guint         property_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  GimpMirror *mirror = GIMP_MIRROR (object);
  GimpImage  *image  = GIMP_SYMMETRY (mirror)->image;

  switch (property_id)
    {
    case PROP_HORIZONTAL_SYMMETRY:
      gimp_mirror_set_horizontal_symmetry (mirror,
                                           g_value_get_boolean (value));
      break;

    case PROP_VERTICAL_SYMMETRY:
      gimp_mirror_set_vertical_symmetry (mirror,
                                         g_value_get_boolean (value));
      break;

    case PROP_POINT_SYMMETRY:
      gimp_mirror_set_point_symmetry (mirror,
                                      g_value_get_boolean (value));
      break;

    case PROP_DISABLE_TRANSFORMATION:
      mirror->disable_transformation = g_value_get_boolean (value);
      break;

    case PROP_MIRROR_POSITION_X:
      if (g_value_get_double (value) >= 0.0 &&
          g_value_get_double (value) < (gdouble) gimp_image_get_width (image))
        {
          mirror->mirror_position_x = g_value_get_double (value);

          if (mirror->vertical_guide)
            {
              g_signal_handlers_block_by_func (mirror->vertical_guide,
                                               gimp_mirror_guide_position_cb,
                                               mirror);
              gimp_image_move_guide (image, mirror->vertical_guide,
                                     mirror->mirror_position_x,
                                     FALSE);
              g_signal_handlers_unblock_by_func (mirror->vertical_guide,
                                                 gimp_mirror_guide_position_cb,
                                                 mirror);
            }
        }
      break;

    case PROP_MIRROR_POSITION_Y:
      if (g_value_get_double (value) >= 0.0 &&
          g_value_get_double (value) < (gdouble) gimp_image_get_height (image))
        {
          mirror->mirror_position_y = g_value_get_double (value);

          if (mirror->horizontal_guide)
            {
              g_signal_handlers_block_by_func (mirror->horizontal_guide,
                                               gimp_mirror_guide_position_cb,
                                               mirror);
              gimp_image_move_guide (image, mirror->horizontal_guide,
                                     mirror->mirror_position_y,
                                     FALSE);
              g_signal_handlers_unblock_by_func (mirror->horizontal_guide,
                                                 gimp_mirror_guide_position_cb,
                                                 mirror);
            }
        }
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_mirror_get_property (GObject    *object,
                          guint       property_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  GimpMirror *mirror = GIMP_MIRROR (object);

  switch (property_id)
    {
    case PROP_HORIZONTAL_SYMMETRY:
      g_value_set_boolean (value, mirror->horizontal_mirror);
      break;
    case PROP_VERTICAL_SYMMETRY:
      g_value_set_boolean (value, mirror->vertical_mirror);
      break;
    case PROP_POINT_SYMMETRY:
      g_value_set_boolean (value, mirror->point_symmetry);
      break;
    case PROP_DISABLE_TRANSFORMATION:
      g_value_set_boolean (value, mirror->disable_transformation);
      break;
    case PROP_MIRROR_POSITION_X:
      g_value_set_double (value, mirror->mirror_position_x);
      break;
    case PROP_MIRROR_POSITION_Y:
      g_value_set_double (value, mirror->mirror_position_y);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_mirror_update_strokes (GimpSymmetry *sym,
                            GimpDrawable *drawable,
                            GimpCoords   *origin)
{
  GimpMirror *mirror  = GIMP_MIRROR (sym);
  GList      *strokes = NULL;
  GimpCoords *coords;
  gdouble     mirror_position_x, mirror_position_y;
  gint        offset_x,          offset_y;

  gimp_item_get_offset (GIMP_ITEM (drawable), &offset_x, &offset_y);

  mirror_position_x = mirror->mirror_position_x - offset_x;
  mirror_position_y = mirror->mirror_position_y - offset_y;

  g_list_free_full (sym->strokes, g_free);
  strokes = g_list_prepend (strokes,
                            g_memdup2 (origin, sizeof (GimpCoords)));

  if (mirror->horizontal_mirror)
    {
      coords = g_memdup2 (origin, sizeof (GimpCoords));
      coords->y = 2.0 * mirror_position_y - origin->y;
      strokes = g_list_prepend (strokes, coords);
    }

  if (mirror->vertical_mirror)
    {
      coords = g_memdup2 (origin, sizeof (GimpCoords));
      coords->x = 2.0 * mirror_position_x - origin->x;
      strokes = g_list_prepend (strokes, coords);
    }

  if (mirror->point_symmetry)
    {
      coords = g_memdup2 (origin, sizeof (GimpCoords));
      coords->x = 2.0 * mirror_position_x - origin->x;
      coords->y = 2.0 * mirror_position_y - origin->y;
      strokes = g_list_prepend (strokes, coords);
    }

  sym->strokes = g_list_reverse (strokes);

  g_signal_emit_by_name (sym, "strokes-updated", sym->image);
}

static void
gimp_mirror_get_transform (GimpSymmetry *sym,
                           gint          stroke,
                           gdouble      *angle,
                           gboolean     *reflect)
{
  GimpMirror *mirror = GIMP_MIRROR (sym);

  if (mirror->disable_transformation)
    return;

  if (! mirror->horizontal_mirror && stroke >= 1)
    stroke++;

  if (! mirror->vertical_mirror && stroke >= 2)
    stroke++;

  switch (stroke)
    {
    /* original */
    case 0:
      break;

    /* horizontal */
    case 1:
      *angle   = 180.0;
      *reflect = TRUE;
      break;

    /* vertical */
    case 2:
      *reflect = TRUE;
      break;

    /* central */
    case 3:
      *angle   = 180.0;
      break;

    default:
      g_return_if_reached ();
    }
}

static void
gimp_mirror_reset (GimpMirror *mirror)
{
  GimpSymmetry *sym = GIMP_SYMMETRY (mirror);

  if (sym->origin)
    {
      gimp_symmetry_set_origin (sym, sym->drawable,
                                sym->origin);
    }
}

static void
gimp_mirror_add_guide (GimpMirror          *mirror,
                       GimpOrientationType  orientation)
{
  GimpSymmetry *sym = GIMP_SYMMETRY (mirror);
  GimpImage    *image;
  Gimp         *gimp;
  GimpGuide    *guide;
  gdouble       position;

  image = sym->image;
  gimp  = image->gimp;

  guide = gimp_guide_custom_new (orientation,
                                 gimp->next_guide_id++,
                                 GIMP_GUIDE_STYLE_MIRROR);

  if (orientation == GIMP_ORIENTATION_HORIZONTAL)
    {
      /* Mirror guide position at first activation is at canvas middle. */
      if (mirror->mirror_position_y < 1.0)
        position = gimp_image_get_height (image) / 2.0;
      else
        position = mirror->mirror_position_y;

      g_object_set (mirror,
                    "mirror-position-y", position,
                    NULL);

      mirror->horizontal_guide = guide;
    }
  else
    {
      /* Mirror guide position at first activation is at canvas middle. */
      if (mirror->mirror_position_x < 1.0)
        position = gimp_image_get_width (image) / 2.0;
      else
        position = mirror->mirror_position_x;

      g_object_set (mirror,
                    "mirror-position-x", position,
                    NULL);

      mirror->vertical_guide = guide;
    }

  g_signal_connect (guide, "removed",
                    G_CALLBACK (gimp_mirror_guide_removed_cb),
                    mirror);

  gimp_image_add_guide (image, guide, (gint) position);

  g_signal_connect (guide, "notify::position",
                    G_CALLBACK (gimp_mirror_guide_position_cb),
                    mirror);
}

static void
gimp_mirror_remove_guide (GimpMirror          *mirror,
                          GimpOrientationType  orientation)
{
  GimpSymmetry *sym = GIMP_SYMMETRY (mirror);
  GimpImage    *image;
  GimpGuide    *guide;

  image = sym->image;
  guide = (orientation == GIMP_ORIENTATION_HORIZONTAL) ?
    mirror->horizontal_guide : mirror->vertical_guide;

  /* The guide may have already been removed, for instance from GUI. */
  if (guide)
    {
      g_signal_handlers_disconnect_by_func (G_OBJECT (guide),
                                            gimp_mirror_guide_removed_cb,
                                            mirror);
      g_signal_handlers_disconnect_by_func (G_OBJECT (guide),
                                            gimp_mirror_guide_position_cb,
                                            mirror);

      gimp_image_remove_guide (image, guide, FALSE);
      g_object_unref (guide);

      if (orientation == GIMP_ORIENTATION_HORIZONTAL)
        mirror->horizontal_guide = NULL;
      else
        mirror->vertical_guide = NULL;
    }
}

static void
gimp_mirror_guide_removed_cb (GObject    *object,
                              GimpMirror *mirror)
{
  GimpSymmetry *symmetry = GIMP_SYMMETRY (mirror);

  g_signal_handlers_disconnect_by_func (object,
                                        gimp_mirror_guide_removed_cb,
                                        mirror);
  g_signal_handlers_disconnect_by_func (object,
                                        gimp_mirror_guide_position_cb,
                                        mirror);

  if (GIMP_GUIDE (object) == mirror->horizontal_guide)
    {
      g_object_unref (mirror->horizontal_guide);
      mirror->horizontal_guide    = NULL;

      g_object_set (mirror,
                    "horizontal-symmetry", FALSE,
                    NULL);
      g_object_set (mirror,
                    "point-symmetry", FALSE,
                    NULL);
      g_object_set (mirror,
                    "mirror-position-y", 0.0,
                    NULL);

      if (mirror->vertical_guide &&
          ! mirror->vertical_mirror)
        {
          g_signal_handlers_disconnect_by_func (G_OBJECT (mirror->vertical_guide),
                                                gimp_mirror_guide_removed_cb,
                                                mirror);
          g_signal_handlers_disconnect_by_func (G_OBJECT (mirror->vertical_guide),
                                                gimp_mirror_guide_position_cb,
                                                mirror);

          gimp_image_remove_guide (symmetry->image,
                                   mirror->vertical_guide,
                                   FALSE);
          g_clear_object (&mirror->vertical_guide);
        }
    }
  else if (GIMP_GUIDE (object) == mirror->vertical_guide)
    {
      g_object_unref (mirror->vertical_guide);
      mirror->vertical_guide    = NULL;

      g_object_set (mirror,
                    "vertical-symmetry", FALSE,
                    NULL);
      g_object_set (mirror,
                    "point-symmetry", FALSE,
                    NULL);
      g_object_set (mirror,
                    "mirror-position-x", 0.0,
                    NULL);

      if (mirror->horizontal_guide &&
          ! mirror->horizontal_mirror)
        {
          g_signal_handlers_disconnect_by_func (G_OBJECT (mirror->horizontal_guide),
                                                gimp_mirror_guide_removed_cb,
                                                mirror);
          g_signal_handlers_disconnect_by_func (G_OBJECT (mirror->horizontal_guide),
                                                gimp_mirror_guide_position_cb,
                                                mirror);

          gimp_image_remove_guide (symmetry->image,
                                   mirror->horizontal_guide,
                                   FALSE);
          g_clear_object (&mirror->horizontal_guide);
        }
    }

  if (mirror->horizontal_guide == NULL &&
      mirror->vertical_guide   == NULL)
    {
      gimp_image_symmetry_remove (symmetry->image,
                                  GIMP_SYMMETRY (mirror));
    }
  else
    {
      gimp_mirror_reset (mirror);
      g_signal_emit_by_name (mirror, "gui-param-changed",
                             GIMP_SYMMETRY (mirror)->image);
    }
}

static void
gimp_mirror_guide_position_cb (GObject    *object,
                               GParamSpec *pspec,
                               GimpMirror *mirror)
{
  GimpGuide *guide = GIMP_GUIDE (object);

  if (guide == mirror->horizontal_guide)
    {
      g_object_set (mirror,
                    "mirror-position-y", (gdouble) gimp_guide_get_position (guide),
                    NULL);
    }
  else if (guide == mirror->vertical_guide)
    {
      g_object_set (mirror,
                    "mirror-position-x", (gdouble) gimp_guide_get_position (guide),
                    NULL);
    }
}

static void
gimp_mirror_active_changed (GimpSymmetry *sym)
{
  GimpMirror *mirror = GIMP_MIRROR (sym);

  if (sym->active)
    {
      if ((mirror->horizontal_mirror || mirror->point_symmetry) &&
          ! mirror->horizontal_guide)
        gimp_mirror_add_guide (mirror, GIMP_ORIENTATION_HORIZONTAL);

      if ((mirror->vertical_mirror || mirror->point_symmetry) &&
          ! mirror->vertical_guide)
        gimp_mirror_add_guide (mirror, GIMP_ORIENTATION_VERTICAL);
    }
  else
    {
      if (mirror->horizontal_guide)
        gimp_mirror_remove_guide (mirror, GIMP_ORIENTATION_HORIZONTAL);

      if (mirror->vertical_guide)
        gimp_mirror_remove_guide (mirror, GIMP_ORIENTATION_VERTICAL);
    }
}

static void
gimp_mirror_set_horizontal_symmetry (GimpMirror *mirror,
                                     gboolean    active)
{
  if (active == mirror->horizontal_mirror)
    return;

  mirror->horizontal_mirror = active;

  if (active)
    {
      if (! mirror->horizontal_guide)
        gimp_mirror_add_guide (mirror, GIMP_ORIENTATION_HORIZONTAL);
    }
  else if (! mirror->point_symmetry)
    {
      gimp_mirror_remove_guide (mirror, GIMP_ORIENTATION_HORIZONTAL);
    }

  gimp_mirror_reset (mirror);
}

static void
gimp_mirror_set_vertical_symmetry (GimpMirror *mirror,
                                   gboolean    active)
{
  if (active == mirror->vertical_mirror)
    return;

  mirror->vertical_mirror = active;

  if (active)
    {
      if (! mirror->vertical_guide)
        gimp_mirror_add_guide (mirror, GIMP_ORIENTATION_VERTICAL);
    }
  else if (! mirror->point_symmetry)
    {
      gimp_mirror_remove_guide (mirror, GIMP_ORIENTATION_VERTICAL);
    }

  gimp_mirror_reset (mirror);
}

static void
gimp_mirror_set_point_symmetry (GimpMirror *mirror,
                                gboolean    active)
{
  if (active == mirror->point_symmetry)
    return;

  mirror->point_symmetry = active;

  if (active)
    {
      /* Show the horizontal guide unless already shown */
      if (! mirror->horizontal_guide)
        gimp_mirror_add_guide (mirror, GIMP_ORIENTATION_HORIZONTAL);

      /* Show the vertical guide unless already shown */
      if (! mirror->vertical_guide)
        gimp_mirror_add_guide (mirror, GIMP_ORIENTATION_VERTICAL);
    }
  else
    {
      /* Remove the horizontal guide unless needed by horizontal mirror */
      if (! mirror->horizontal_mirror)
        gimp_mirror_remove_guide (mirror, GIMP_ORIENTATION_HORIZONTAL);

      /* Remove the vertical guide unless needed by vertical mirror */
      if (! mirror->vertical_mirror)
        gimp_mirror_remove_guide (mirror, GIMP_ORIENTATION_VERTICAL);
    }

  gimp_mirror_reset (mirror);
}

static void
gimp_mirror_image_size_changed_cb (GimpImage    *image,
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
