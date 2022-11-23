/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmasymmetry-mirror.c
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

#include "core-types.h"

#include "ligma.h"
#include "ligma-cairo.h"
#include "ligmabrush.h"
#include "ligmaguide.h"
#include "ligmaimage.h"
#include "ligmaimage-guides.h"
#include "ligmaimage-symmetry.h"
#include "ligmaitem.h"
#include "ligmasymmetry-mirror.h"

#include "ligma-intl.h"


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

static void       ligma_mirror_constructed             (GObject             *object);
static void       ligma_mirror_finalize                (GObject             *object);
static void       ligma_mirror_set_property            (GObject             *object,
                                                       guint                property_id,
                                                       const GValue        *value,
                                                       GParamSpec          *pspec);
static void       ligma_mirror_get_property            (GObject             *object,
                                                       guint                property_id,
                                                       GValue              *value,
                                                       GParamSpec          *pspec);

static void       ligma_mirror_update_strokes          (LigmaSymmetry        *mirror,
                                                       LigmaDrawable        *drawable,
                                                       LigmaCoords          *origin);
static void       ligma_mirror_get_transform           (LigmaSymmetry        *mirror,
                                                       gint                 stroke,
                                                       gdouble             *angle,
                                                       gboolean            *reflect);
static void       ligma_mirror_reset                   (LigmaMirror          *mirror);
static void       ligma_mirror_add_guide               (LigmaMirror          *mirror,
                                                       LigmaOrientationType  orientation);
static void       ligma_mirror_remove_guide            (LigmaMirror          *mirror,
                                                       LigmaOrientationType  orientation);
static void       ligma_mirror_guide_removed_cb        (GObject             *object,
                                                       LigmaMirror          *mirror);
static void       ligma_mirror_guide_position_cb       (GObject             *object,
                                                       GParamSpec          *pspec,
                                                       LigmaMirror          *mirror);
static void       ligma_mirror_active_changed          (LigmaSymmetry        *sym);
static void       ligma_mirror_set_horizontal_symmetry (LigmaMirror          *mirror,
                                                       gboolean             active);
static void       ligma_mirror_set_vertical_symmetry   (LigmaMirror          *mirror,
                                                       gboolean             active);
static void       ligma_mirror_set_point_symmetry      (LigmaMirror          *mirror,
                                                       gboolean             active);

static void       ligma_mirror_image_size_changed_cb   (LigmaImage           *image,
                                                       gint                 previous_origin_x,
                                                       gint                 previous_origin_y,
                                                       gint                 previous_width,
                                                       gint                 previous_height,
                                                       LigmaSymmetry        *sym);

G_DEFINE_TYPE (LigmaMirror, ligma_mirror, LIGMA_TYPE_SYMMETRY)

#define parent_class ligma_mirror_parent_class


static void
ligma_mirror_class_init (LigmaMirrorClass *klass)
{
  GObjectClass      *object_class   = G_OBJECT_CLASS (klass);
  LigmaSymmetryClass *symmetry_class = LIGMA_SYMMETRY_CLASS (klass);
  GParamSpec        *pspec;

  object_class->constructed         = ligma_mirror_constructed;
  object_class->finalize            = ligma_mirror_finalize;
  object_class->set_property        = ligma_mirror_set_property;
  object_class->get_property        = ligma_mirror_get_property;

  symmetry_class->label             = _("Mirror");
  symmetry_class->update_strokes    = ligma_mirror_update_strokes;
  symmetry_class->get_transform     = ligma_mirror_get_transform;
  symmetry_class->active_changed    = ligma_mirror_active_changed;

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_HORIZONTAL_SYMMETRY,
                            "horizontal-symmetry",
                            _("Horizontal Symmetry"),
                            _("Reflect the initial stroke across a horizontal axis"),
                            FALSE,
                            LIGMA_PARAM_STATIC_STRINGS |
                            LIGMA_SYMMETRY_PARAM_GUI);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_VERTICAL_SYMMETRY,
                            "vertical-symmetry",
                            _("Vertical Symmetry"),
                            _("Reflect the initial stroke across a vertical axis"),
                            FALSE,
                            LIGMA_PARAM_STATIC_STRINGS |
                            LIGMA_SYMMETRY_PARAM_GUI);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_POINT_SYMMETRY,
                            "point-symmetry",
                            _("Central Symmetry"),
                            _("Invert the initial stroke through a point"),
                            FALSE,
                            LIGMA_PARAM_STATIC_STRINGS |
                            LIGMA_SYMMETRY_PARAM_GUI);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_DISABLE_TRANSFORMATION,
                            "disable-transformation",
                            _("Disable brush transform"),
                            _("Disable brush reflection"),
                            FALSE,
                            LIGMA_PARAM_STATIC_STRINGS |
                            LIGMA_SYMMETRY_PARAM_GUI);

  LIGMA_CONFIG_PROP_DOUBLE (object_class, PROP_MIRROR_POSITION_X,
                           "mirror-position-x",
                           _("Vertical axis position"),
                           NULL,
                           0.0, G_MAXDOUBLE, 0.0,
                           LIGMA_PARAM_STATIC_STRINGS |
                           LIGMA_SYMMETRY_PARAM_GUI);

  pspec = g_object_class_find_property (object_class, "mirror-position-x");
  gegl_param_spec_set_property_key (pspec, "unit", "pixel-coordinate");
  gegl_param_spec_set_property_key (pspec, "axis", "x");

  LIGMA_CONFIG_PROP_DOUBLE (object_class, PROP_MIRROR_POSITION_Y,
                           "mirror-position-y",
                           _("Horizontal axis position"),
                           NULL,
                           0.0, G_MAXDOUBLE, 0.0,
                           LIGMA_PARAM_STATIC_STRINGS |
                           LIGMA_SYMMETRY_PARAM_GUI);

  pspec = g_object_class_find_property (object_class, "mirror-position-y");
  gegl_param_spec_set_property_key (pspec, "unit", "pixel-coordinate");
  gegl_param_spec_set_property_key (pspec, "axis", "y");
}

static void
ligma_mirror_init (LigmaMirror *mirror)
{
}

static void
ligma_mirror_constructed (GObject *object)
{
  LigmaSymmetry *sym = LIGMA_SYMMETRY (object);

  g_signal_connect_object (sym->image, "size-changed-detailed",
                           G_CALLBACK (ligma_mirror_image_size_changed_cb),
                           sym, 0);
}

static void
ligma_mirror_finalize (GObject *object)
{
  LigmaMirror *mirror = LIGMA_MIRROR (object);

  g_clear_object (&mirror->horizontal_guide);
  g_clear_object (&mirror->vertical_guide);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_mirror_set_property (GObject      *object,
                          guint         property_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  LigmaMirror *mirror = LIGMA_MIRROR (object);
  LigmaImage  *image  = LIGMA_SYMMETRY (mirror)->image;

  switch (property_id)
    {
    case PROP_HORIZONTAL_SYMMETRY:
      ligma_mirror_set_horizontal_symmetry (mirror,
                                           g_value_get_boolean (value));
      break;

    case PROP_VERTICAL_SYMMETRY:
      ligma_mirror_set_vertical_symmetry (mirror,
                                         g_value_get_boolean (value));
      break;

    case PROP_POINT_SYMMETRY:
      ligma_mirror_set_point_symmetry (mirror,
                                      g_value_get_boolean (value));
      break;

    case PROP_DISABLE_TRANSFORMATION:
      mirror->disable_transformation = g_value_get_boolean (value);
      break;

    case PROP_MIRROR_POSITION_X:
      if (g_value_get_double (value) >= 0.0 &&
          g_value_get_double (value) < (gdouble) ligma_image_get_width (image))
        {
          mirror->mirror_position_x = g_value_get_double (value);

          if (mirror->vertical_guide)
            {
              g_signal_handlers_block_by_func (mirror->vertical_guide,
                                               ligma_mirror_guide_position_cb,
                                               mirror);
              ligma_image_move_guide (image, mirror->vertical_guide,
                                     mirror->mirror_position_x,
                                     FALSE);
              g_signal_handlers_unblock_by_func (mirror->vertical_guide,
                                                 ligma_mirror_guide_position_cb,
                                                 mirror);
            }
        }
      break;

    case PROP_MIRROR_POSITION_Y:
      if (g_value_get_double (value) >= 0.0 &&
          g_value_get_double (value) < (gdouble) ligma_image_get_height (image))
        {
          mirror->mirror_position_y = g_value_get_double (value);

          if (mirror->horizontal_guide)
            {
              g_signal_handlers_block_by_func (mirror->horizontal_guide,
                                               ligma_mirror_guide_position_cb,
                                               mirror);
              ligma_image_move_guide (image, mirror->horizontal_guide,
                                     mirror->mirror_position_y,
                                     FALSE);
              g_signal_handlers_unblock_by_func (mirror->horizontal_guide,
                                                 ligma_mirror_guide_position_cb,
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
ligma_mirror_get_property (GObject    *object,
                          guint       property_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  LigmaMirror *mirror = LIGMA_MIRROR (object);

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
ligma_mirror_update_strokes (LigmaSymmetry *sym,
                            LigmaDrawable *drawable,
                            LigmaCoords   *origin)
{
  LigmaMirror *mirror  = LIGMA_MIRROR (sym);
  GList      *strokes = NULL;
  LigmaCoords *coords;
  gdouble     mirror_position_x, mirror_position_y;
  gint        offset_x,          offset_y;

  ligma_item_get_offset (LIGMA_ITEM (drawable), &offset_x, &offset_y);

  mirror_position_x = mirror->mirror_position_x - offset_x;
  mirror_position_y = mirror->mirror_position_y - offset_y;

  g_list_free_full (sym->strokes, g_free);
  strokes = g_list_prepend (strokes,
                            g_memdup2 (origin, sizeof (LigmaCoords)));

  if (mirror->horizontal_mirror)
    {
      coords = g_memdup2 (origin, sizeof (LigmaCoords));
      coords->y = 2.0 * mirror_position_y - origin->y;
      strokes = g_list_prepend (strokes, coords);
    }

  if (mirror->vertical_mirror)
    {
      coords = g_memdup2 (origin, sizeof (LigmaCoords));
      coords->x = 2.0 * mirror_position_x - origin->x;
      strokes = g_list_prepend (strokes, coords);
    }

  if (mirror->point_symmetry)
    {
      coords = g_memdup2 (origin, sizeof (LigmaCoords));
      coords->x = 2.0 * mirror_position_x - origin->x;
      coords->y = 2.0 * mirror_position_y - origin->y;
      strokes = g_list_prepend (strokes, coords);
    }

  sym->strokes = g_list_reverse (strokes);

  g_signal_emit_by_name (sym, "strokes-updated", sym->image);
}

static void
ligma_mirror_get_transform (LigmaSymmetry *sym,
                           gint          stroke,
                           gdouble      *angle,
                           gboolean     *reflect)
{
  LigmaMirror *mirror = LIGMA_MIRROR (sym);

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
ligma_mirror_reset (LigmaMirror *mirror)
{
  LigmaSymmetry *sym = LIGMA_SYMMETRY (mirror);

  if (sym->origin)
    {
      ligma_symmetry_set_origin (sym, sym->drawable,
                                sym->origin);
    }
}

static void
ligma_mirror_add_guide (LigmaMirror          *mirror,
                       LigmaOrientationType  orientation)
{
  LigmaSymmetry *sym = LIGMA_SYMMETRY (mirror);
  LigmaImage    *image;
  Ligma         *ligma;
  LigmaGuide    *guide;
  gdouble       position;

  image = sym->image;
  ligma  = image->ligma;

  guide = ligma_guide_custom_new (orientation,
                                 ligma->next_guide_id++,
                                 LIGMA_GUIDE_STYLE_MIRROR);

  if (orientation == LIGMA_ORIENTATION_HORIZONTAL)
    {
      /* Mirror guide position at first activation is at canvas middle. */
      if (mirror->mirror_position_y < 1.0)
        position = ligma_image_get_height (image) / 2.0;
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
        position = ligma_image_get_width (image) / 2.0;
      else
        position = mirror->mirror_position_x;

      g_object_set (mirror,
                    "mirror-position-x", position,
                    NULL);

      mirror->vertical_guide = guide;
    }

  g_signal_connect (guide, "removed",
                    G_CALLBACK (ligma_mirror_guide_removed_cb),
                    mirror);

  ligma_image_add_guide (image, guide, (gint) position);

  g_signal_connect (guide, "notify::position",
                    G_CALLBACK (ligma_mirror_guide_position_cb),
                    mirror);
}

static void
ligma_mirror_remove_guide (LigmaMirror          *mirror,
                          LigmaOrientationType  orientation)
{
  LigmaSymmetry *sym = LIGMA_SYMMETRY (mirror);
  LigmaImage    *image;
  LigmaGuide    *guide;

  image = sym->image;
  guide = (orientation == LIGMA_ORIENTATION_HORIZONTAL) ?
    mirror->horizontal_guide : mirror->vertical_guide;

  /* The guide may have already been removed, for instance from GUI. */
  if (guide)
    {
      g_signal_handlers_disconnect_by_func (G_OBJECT (guide),
                                            ligma_mirror_guide_removed_cb,
                                            mirror);
      g_signal_handlers_disconnect_by_func (G_OBJECT (guide),
                                            ligma_mirror_guide_position_cb,
                                            mirror);

      ligma_image_remove_guide (image, guide, FALSE);
      g_object_unref (guide);

      if (orientation == LIGMA_ORIENTATION_HORIZONTAL)
        mirror->horizontal_guide = NULL;
      else
        mirror->vertical_guide = NULL;
    }
}

static void
ligma_mirror_guide_removed_cb (GObject    *object,
                              LigmaMirror *mirror)
{
  LigmaSymmetry *symmetry = LIGMA_SYMMETRY (mirror);

  g_signal_handlers_disconnect_by_func (object,
                                        ligma_mirror_guide_removed_cb,
                                        mirror);
  g_signal_handlers_disconnect_by_func (object,
                                        ligma_mirror_guide_position_cb,
                                        mirror);

  if (LIGMA_GUIDE (object) == mirror->horizontal_guide)
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
                                                ligma_mirror_guide_removed_cb,
                                                mirror);
          g_signal_handlers_disconnect_by_func (G_OBJECT (mirror->vertical_guide),
                                                ligma_mirror_guide_position_cb,
                                                mirror);

          ligma_image_remove_guide (symmetry->image,
                                   mirror->vertical_guide,
                                   FALSE);
          g_clear_object (&mirror->vertical_guide);
        }
    }
  else if (LIGMA_GUIDE (object) == mirror->vertical_guide)
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
                                                ligma_mirror_guide_removed_cb,
                                                mirror);
          g_signal_handlers_disconnect_by_func (G_OBJECT (mirror->horizontal_guide),
                                                ligma_mirror_guide_position_cb,
                                                mirror);

          ligma_image_remove_guide (symmetry->image,
                                   mirror->horizontal_guide,
                                   FALSE);
          g_clear_object (&mirror->horizontal_guide);
        }
    }

  if (mirror->horizontal_guide == NULL &&
      mirror->vertical_guide   == NULL)
    {
      ligma_image_symmetry_remove (symmetry->image,
                                  LIGMA_SYMMETRY (mirror));
    }
  else
    {
      ligma_mirror_reset (mirror);
      g_signal_emit_by_name (mirror, "gui-param-changed",
                             LIGMA_SYMMETRY (mirror)->image);
    }
}

static void
ligma_mirror_guide_position_cb (GObject    *object,
                               GParamSpec *pspec,
                               LigmaMirror *mirror)
{
  LigmaGuide *guide = LIGMA_GUIDE (object);

  if (guide == mirror->horizontal_guide)
    {
      g_object_set (mirror,
                    "mirror-position-y", (gdouble) ligma_guide_get_position (guide),
                    NULL);
    }
  else if (guide == mirror->vertical_guide)
    {
      g_object_set (mirror,
                    "mirror-position-x", (gdouble) ligma_guide_get_position (guide),
                    NULL);
    }
}

static void
ligma_mirror_active_changed (LigmaSymmetry *sym)
{
  LigmaMirror *mirror = LIGMA_MIRROR (sym);

  if (sym->active)
    {
      if ((mirror->horizontal_mirror || mirror->point_symmetry) &&
          ! mirror->horizontal_guide)
        ligma_mirror_add_guide (mirror, LIGMA_ORIENTATION_HORIZONTAL);

      if ((mirror->vertical_mirror || mirror->point_symmetry) &&
          ! mirror->vertical_guide)
        ligma_mirror_add_guide (mirror, LIGMA_ORIENTATION_VERTICAL);
    }
  else
    {
      if (mirror->horizontal_guide)
        ligma_mirror_remove_guide (mirror, LIGMA_ORIENTATION_HORIZONTAL);

      if (mirror->vertical_guide)
        ligma_mirror_remove_guide (mirror, LIGMA_ORIENTATION_VERTICAL);
    }
}

static void
ligma_mirror_set_horizontal_symmetry (LigmaMirror *mirror,
                                     gboolean    active)
{
  if (active == mirror->horizontal_mirror)
    return;

  mirror->horizontal_mirror = active;

  if (active)
    {
      if (! mirror->horizontal_guide)
        ligma_mirror_add_guide (mirror, LIGMA_ORIENTATION_HORIZONTAL);
    }
  else if (! mirror->point_symmetry)
    {
      ligma_mirror_remove_guide (mirror, LIGMA_ORIENTATION_HORIZONTAL);
    }

  ligma_mirror_reset (mirror);
}

static void
ligma_mirror_set_vertical_symmetry (LigmaMirror *mirror,
                                   gboolean    active)
{
  if (active == mirror->vertical_mirror)
    return;

  mirror->vertical_mirror = active;

  if (active)
    {
      if (! mirror->vertical_guide)
        ligma_mirror_add_guide (mirror, LIGMA_ORIENTATION_VERTICAL);
    }
  else if (! mirror->point_symmetry)
    {
      ligma_mirror_remove_guide (mirror, LIGMA_ORIENTATION_VERTICAL);
    }

  ligma_mirror_reset (mirror);
}

static void
ligma_mirror_set_point_symmetry (LigmaMirror *mirror,
                                gboolean    active)
{
  if (active == mirror->point_symmetry)
    return;

  mirror->point_symmetry = active;

  if (active)
    {
      /* Show the horizontal guide unless already shown */
      if (! mirror->horizontal_guide)
        ligma_mirror_add_guide (mirror, LIGMA_ORIENTATION_HORIZONTAL);

      /* Show the vertical guide unless already shown */
      if (! mirror->vertical_guide)
        ligma_mirror_add_guide (mirror, LIGMA_ORIENTATION_VERTICAL);
    }
  else
    {
      /* Remove the horizontal guide unless needed by horizontal mirror */
      if (! mirror->horizontal_mirror)
        ligma_mirror_remove_guide (mirror, LIGMA_ORIENTATION_HORIZONTAL);

      /* Remove the vertical guide unless needed by vertical mirror */
      if (! mirror->vertical_mirror)
        ligma_mirror_remove_guide (mirror, LIGMA_ORIENTATION_VERTICAL);
    }

  ligma_mirror_reset (mirror);
}

static void
ligma_mirror_image_size_changed_cb (LigmaImage    *image,
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
