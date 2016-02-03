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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

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
  PROP_HORIZONTAL_POSITION,
  PROP_VERTICAL_POSITION
};


/* Local function prototypes */

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
static void       gimp_mirror_prepare_operations      (GimpMirror          *mirror,
                                                       gint                 paint_width,
                                                       gint                 paint_height);
static GeglNode * gimp_mirror_get_operation           (GimpSymmetry        *mirror,
                                                       gint                 stroke,
                                                       gint                 paint_width,
                                                       gint                 paint_height);
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
static GParamSpec ** gimp_mirror_get_settings         (GimpSymmetry        *sym,
                                                       gint                *n_settings);
static void       gimp_mirror_active_changed          (GimpSymmetry        *sym);
static void       gimp_mirror_set_horizontal_symmetry (GimpMirror          *mirror,
                                                       gboolean             active);
static void       gimp_mirror_set_vertical_symmetry   (GimpMirror          *mirror,
                                                       gboolean             active);
static void       gimp_mirror_set_point_symmetry      (GimpMirror          *mirror,
                                                       gboolean             active);


G_DEFINE_TYPE (GimpMirror, gimp_mirror, GIMP_TYPE_SYMMETRY)

#define parent_class gimp_mirror_parent_class


static void
gimp_mirror_class_init (GimpMirrorClass *klass)
{
  GObjectClass      *object_class   = G_OBJECT_CLASS (klass);
  GimpSymmetryClass *symmetry_class = GIMP_SYMMETRY_CLASS (klass);

  object_class->finalize            = gimp_mirror_finalize;
  object_class->set_property        = gimp_mirror_set_property;
  object_class->get_property        = gimp_mirror_get_property;

  symmetry_class->label             = _("Mirror");
  symmetry_class->update_strokes    = gimp_mirror_update_strokes;
  symmetry_class->get_operation     = gimp_mirror_get_operation;
  symmetry_class->get_settings      = gimp_mirror_get_settings;
  symmetry_class->active_changed    = gimp_mirror_active_changed;

  /* Properties for user settings */
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_HORIZONTAL_SYMMETRY,
                                    "horizontal-symmetry",
                                    _("Horizontal Mirror"),
                                    FALSE,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_VERTICAL_SYMMETRY,
                                    "vertical-symmetry",
                                    _("Vertical Mirror"),
                                    FALSE,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_POINT_SYMMETRY,
                                    "point-symmetry",
                                    _("Central Symmetry"),
                                    FALSE,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_DISABLE_TRANSFORMATION,
                                    "disable-transformation",
                                    _("Disable Brush Transformation (faster)"),
                                    FALSE,
                                    GIMP_PARAM_STATIC_STRINGS);

  /* Properties for XCF serialization only */
  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_HORIZONTAL_POSITION,
                                   "horizontal-position",
                                   _("Horizontal guide position"),
                                   0.0, G_MAXDOUBLE, 0.0,
                                   GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_VERTICAL_POSITION,
                                   "vertical-position",
                                   _("Vertical guide position"),
                                   0.0, G_MAXDOUBLE, 0.0,
                                   GIMP_PARAM_STATIC_STRINGS);
}

static void
gimp_mirror_init (GimpMirror *mirror)
{
}

static void
gimp_mirror_finalize (GObject *object)
{
  GimpMirror *mirror = GIMP_MIRROR (object);

  if (mirror->horizontal_guide)
    g_object_unref (mirror->horizontal_guide);
  mirror->horizontal_guide = NULL;

  if (mirror->vertical_guide)
    g_object_unref (mirror->vertical_guide);
  mirror->vertical_guide = NULL;

  if (mirror->horizontal_op)
    g_object_unref (mirror->horizontal_op);
  mirror->horizontal_op = NULL;

  if (mirror->vertical_op)
    g_object_unref (mirror->vertical_op);
  mirror->vertical_op = NULL;

  if (mirror->central_op)
    g_object_unref (mirror->central_op);
  mirror->central_op = NULL;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_mirror_set_property (GObject      *object,
                          guint         property_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  GimpMirror *mirror = GIMP_MIRROR (object);

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
    case PROP_HORIZONTAL_POSITION:
      mirror->horizontal_position = g_value_get_double (value);
      if (mirror->horizontal_guide)
        gimp_guide_set_position (mirror->horizontal_guide,
                                 mirror->horizontal_position);
      break;
    case PROP_VERTICAL_POSITION:
      mirror->vertical_position = g_value_get_double (value);
      if (mirror->vertical_guide)
        gimp_guide_set_position (mirror->vertical_guide,
                                 mirror->vertical_position);
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
    case PROP_HORIZONTAL_POSITION:
      g_value_set_double (value, mirror->horizontal_position);
      break;
    case PROP_VERTICAL_POSITION:
      g_value_set_double (value, mirror->vertical_position);
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

  g_list_free_full (sym->strokes, g_free);
  strokes = g_list_prepend (strokes,
                            g_memdup (origin, sizeof (GimpCoords)));

  if (mirror->horizontal_mirror)
    {
      coords = g_memdup (origin, sizeof (GimpCoords));
      coords->y = 2.0 * mirror->horizontal_position - origin->y;
      strokes = g_list_prepend (strokes, coords);
    }

  if (mirror->vertical_mirror)
    {
      coords = g_memdup (origin, sizeof (GimpCoords));
      coords->x = 2.0 * mirror->vertical_position - origin->x;
      strokes = g_list_prepend (strokes, coords);
    }

  if (mirror->point_symmetry)
    {
      coords = g_memdup (origin, sizeof (GimpCoords));
      coords->x = 2.0 * mirror->vertical_position - origin->x;
      coords->y = 2.0 * mirror->horizontal_position - origin->y;
      strokes = g_list_prepend (strokes, coords);
    }

  sym->strokes = g_list_reverse (strokes);

  g_signal_emit_by_name (sym, "strokes-updated", sym->image);
}

static void gimp_mirror_prepare_operations (GimpMirror *mirror,
                                            gint        paint_width,
                                            gint        paint_height)
{
  if (paint_width == mirror->last_paint_width &&
      paint_height == mirror->last_paint_height)
    return;

  mirror->last_paint_width  = paint_width;
  mirror->last_paint_height = paint_height;

  if (mirror->horizontal_op)
    g_object_unref (mirror->horizontal_op);

  mirror->horizontal_op = gegl_node_new_child (NULL,
                                               "operation", "gegl:reflect",
                                               "origin-x",  0.0,
                                               "origin-y",  paint_height / 2.0,
                                               "x",         1.0,
                                               "y",         0.0,
                                               NULL);

  if (mirror->vertical_op)
    g_object_unref (mirror->vertical_op);

  mirror->vertical_op = gegl_node_new_child (NULL,
                                             "operation", "gegl:reflect",
                                             "origin-x",  paint_width / 2.0,
                                             "origin-y",  0.0,
                                             "x",         0.0,
                                             "y",         1.0,
                                             NULL);

  if (mirror->central_op)
    g_object_unref (mirror->central_op);

  mirror->central_op = gegl_node_new_child (NULL,
                                            "operation", "gegl:rotate",
                                            "origin-x",  paint_width / 2.0,
                                            "origin-y",  paint_height / 2.0,
                                            "degrees",   180.0,
                                            NULL);
}

static GeglNode *
gimp_mirror_get_operation (GimpSymmetry *sym,
                           gint          stroke,
                           gint          paint_width,
                           gint          paint_height)
{
  GimpMirror *mirror = GIMP_MIRROR (sym);
  GeglNode   *op;

  g_return_val_if_fail (stroke >= 0 &&
                        stroke < g_list_length (sym->strokes), NULL);

  gimp_mirror_prepare_operations (mirror, paint_width, paint_height);

  if (mirror->disable_transformation || stroke == 0 ||
      paint_width == 0 || paint_height == 0)
    op = NULL;
  else if (stroke == 1 && mirror->horizontal_mirror)
    op = g_object_ref (mirror->horizontal_op);
  else if ((stroke == 2 && mirror->horizontal_mirror &&
            mirror->vertical_mirror) ||
           (stroke == 1 && mirror->vertical_mirror &&
            !  mirror->horizontal_mirror))
    op = g_object_ref (mirror->vertical_op);
  else
    op = g_object_ref (mirror->central_op);

  return op;
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
  gint          position;

  image = sym->image;
  gimp  = image->gimp;

  guide = gimp_guide_custom_new (orientation,
                                 gimp->next_guide_ID++,
                                 GIMP_GUIDE_STYLE_MIRROR);

  if (orientation == GIMP_ORIENTATION_HORIZONTAL)
    {
      mirror->horizontal_guide = guide;

      /* Mirror guide position at first activation is at canvas middle. */
      if (mirror->horizontal_position < 1.0)
        mirror->horizontal_position = gimp_image_get_height (image) / 2.0;
      position = mirror->horizontal_position;
    }
  else
    {
      mirror->vertical_guide = guide;

      /* Mirror guide position at first activation is at canvas middle. */
      if (mirror->vertical_position < 1.0)
        mirror->vertical_position = gimp_image_get_width (image) / 2.0;
      position = mirror->vertical_position;
    }

  g_signal_connect (guide, "removed",
                    G_CALLBACK (gimp_mirror_guide_removed_cb),
                    mirror);

  gimp_image_add_guide (image, guide, position);

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

      mirror->horizontal_mirror   = FALSE;
      mirror->point_symmetry      = FALSE;
      mirror->horizontal_position = 0.0;

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

      mirror->vertical_mirror   = FALSE;
      mirror->point_symmetry    = FALSE;
      mirror->vertical_position = 0.0;

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
      g_signal_emit_by_name (mirror, "update-ui",
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
      mirror->horizontal_position = (gdouble) gimp_guide_get_position (guide);
    }
  else if (guide == mirror->vertical_guide)
    {
      mirror->vertical_position = (gdouble) gimp_guide_get_position (guide);
    }
}

static GParamSpec **
gimp_mirror_get_settings (GimpSymmetry *sym,
                          gint         *n_settings)
{
  GParamSpec **pspecs;

  *n_settings = 5;
  pspecs = g_new (GParamSpec*, 5);

  pspecs[0] = g_object_class_find_property (G_OBJECT_GET_CLASS (sym),
                                            "horizontal-symmetry");
  pspecs[1] = g_object_class_find_property (G_OBJECT_GET_CLASS (sym),
                                            "vertical-symmetry");
  pspecs[2] = g_object_class_find_property (G_OBJECT_GET_CLASS (sym),
                                            "point-symmetry");
  pspecs[3] = NULL;
  pspecs[4] = g_object_class_find_property (G_OBJECT_GET_CLASS (sym),
                                            "disable-transformation");

  return pspecs;
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
    gimp_mirror_remove_guide (mirror, GIMP_ORIENTATION_HORIZONTAL);

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
    gimp_mirror_remove_guide (mirror, GIMP_ORIENTATION_VERTICAL);

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
