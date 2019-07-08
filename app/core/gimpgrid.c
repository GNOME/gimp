/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpGrid
 * Copyright (C) 2003  Henrik Brix Andersen <brix@gimp.org>
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

#include "libgimpmath/gimpmath.h"
#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"

#include "libgimpcolor/gimpcolor.h"

#include "core-types.h"

#include "gimpgrid.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_STYLE,
  PROP_FGCOLOR,
  PROP_BGCOLOR,
  PROP_XSPACING,
  PROP_YSPACING,
  PROP_SPACING_UNIT,
  PROP_XOFFSET,
  PROP_YOFFSET,
  PROP_OFFSET_UNIT
};


static void   gimp_grid_get_property (GObject      *object,
                                      guint         property_id,
                                      GValue       *value,
                                      GParamSpec   *pspec);
static void   gimp_grid_set_property (GObject      *object,
                                      guint         property_id,
                                      const GValue *value,
                                      GParamSpec   *pspec);


G_DEFINE_TYPE_WITH_CODE (GimpGrid, gimp_grid, GIMP_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_CONFIG, NULL))


static void
gimp_grid_class_init (GimpGridClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GimpRGB       black;
  GimpRGB       white;

  object_class->get_property = gimp_grid_get_property;
  object_class->set_property = gimp_grid_set_property;

  gimp_rgba_set (&black, 0.0, 0.0, 0.0, GIMP_OPACITY_OPAQUE);
  gimp_rgba_set (&white, 1.0, 1.0, 1.0, GIMP_OPACITY_OPAQUE);

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_STYLE,
                         "style",
                         _("Line style"),
                         _("Line style used for the grid."),
                         GIMP_TYPE_GRID_STYLE,
                         GIMP_GRID_SOLID,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_RGB (object_class, PROP_FGCOLOR,
                        "fgcolor",
                        _("Foreground color"),
                        _("The foreground color of the grid."),
                        TRUE, &black,
                        GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_RGB (object_class, PROP_BGCOLOR,
                        "bgcolor",
                        _("Background color"),
                        _("The background color of the grid; "
                          "only used in double dashed line style."),
                        TRUE, &white,
                        GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_DOUBLE (object_class, PROP_XSPACING,
                           "xspacing",
                           _("Spacing X"),
                           _("Horizontal spacing of grid lines."),
                           0.0, GIMP_MAX_IMAGE_SIZE, 10.0,
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_DOUBLE (object_class, PROP_YSPACING,
                           "yspacing",
                           _("Spacing Y"),
                           _("Vertical spacing of grid lines."),
                           0.0, GIMP_MAX_IMAGE_SIZE, 10.0,
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_UNIT (object_class, PROP_SPACING_UNIT,
                         "spacing-unit",
                         _("Spacing unit"),
                         NULL,
                         FALSE, FALSE, GIMP_UNIT_INCH,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_DOUBLE (object_class, PROP_XOFFSET,
                           "xoffset",
                           _("Offset X"),
                           _("Horizontal offset of the first grid "
                             "line; this may be a negative number."),
                           - GIMP_MAX_IMAGE_SIZE,
                           GIMP_MAX_IMAGE_SIZE, 0.0,
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_DOUBLE (object_class, PROP_YOFFSET,
                           "yoffset",
                           _("Offset Y"),
                           _("Vertical offset of the first grid "
                             "line; this may be a negative number."),
                           - GIMP_MAX_IMAGE_SIZE,
                           GIMP_MAX_IMAGE_SIZE, 0.0,
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_UNIT (object_class, PROP_OFFSET_UNIT,
                         "offset-unit",
                         _("Offset unit"),
                         NULL,
                         FALSE, FALSE, GIMP_UNIT_INCH,
                         GIMP_PARAM_STATIC_STRINGS);
}

static void
gimp_grid_init (GimpGrid *grid)
{
}

static void
gimp_grid_get_property (GObject      *object,
                        guint         property_id,
                        GValue       *value,
                        GParamSpec   *pspec)
{
  GimpGrid *grid = GIMP_GRID (object);

  switch (property_id)
    {
    case PROP_STYLE:
      g_value_set_enum (value, grid->style);
      break;
    case PROP_FGCOLOR:
      g_value_set_boxed (value, &grid->fgcolor);
      break;
    case PROP_BGCOLOR:
      g_value_set_boxed (value, &grid->bgcolor);
      break;
    case PROP_XSPACING:
      g_value_set_double (value, grid->xspacing);
      break;
    case PROP_YSPACING:
      g_value_set_double (value, grid->yspacing);
      break;
    case PROP_SPACING_UNIT:
      g_value_set_int (value, grid->spacing_unit);
      break;
    case PROP_XOFFSET:
      g_value_set_double (value, grid->xoffset);
      break;
    case PROP_YOFFSET:
      g_value_set_double (value, grid->yoffset);
      break;
    case PROP_OFFSET_UNIT:
      g_value_set_int (value, grid->offset_unit);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_grid_set_property (GObject      *object,
                        guint         property_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
  GimpGrid *grid = GIMP_GRID (object);
  GimpRGB  *color;

  switch (property_id)
    {
    case PROP_STYLE:
      grid->style = g_value_get_enum (value);
      break;
    case PROP_FGCOLOR:
      color = g_value_get_boxed (value);
      grid->fgcolor = *color;
      break;
    case PROP_BGCOLOR:
      color = g_value_get_boxed (value);
      grid->bgcolor = *color;
      break;
    case PROP_XSPACING:
      grid->xspacing = g_value_get_double (value);
      break;
    case PROP_YSPACING:
      grid->yspacing = g_value_get_double (value);
      break;
    case PROP_SPACING_UNIT:
      grid->spacing_unit = g_value_get_int (value);
      break;
    case PROP_XOFFSET:
      grid->xoffset = g_value_get_double (value);
      break;
    case PROP_YOFFSET:
      grid->yoffset = g_value_get_double (value);
      break;
    case PROP_OFFSET_UNIT:
      grid->offset_unit = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


/*  public functions  */

GimpGridStyle
gimp_grid_get_style (GimpGrid *grid)
{
  g_return_val_if_fail (GIMP_IS_GRID (grid), GIMP_GRID_SOLID);

  return grid->style;
}

void
gimp_grid_get_fgcolor (GimpGrid *grid,
                       GimpRGB  *fgcolor)
{
  g_return_if_fail (GIMP_IS_GRID (grid));
  g_return_if_fail (fgcolor != NULL);

  *fgcolor = grid->fgcolor;
}

void
gimp_grid_get_bgcolor (GimpGrid *grid,
                       GimpRGB  *bgcolor)
{
  g_return_if_fail (GIMP_IS_GRID (grid));
  g_return_if_fail (bgcolor != NULL);

  *bgcolor = grid->bgcolor;
}

void
gimp_grid_get_spacing (GimpGrid *grid,
                       gdouble  *xspacing,
                       gdouble  *yspacing)
{
  g_return_if_fail (GIMP_IS_GRID (grid));

  if (xspacing) *xspacing = grid->xspacing;
  if (yspacing) *yspacing = grid->yspacing;
}

void
gimp_grid_get_offset (GimpGrid *grid,
                      gdouble  *xoffset,
                      gdouble  *yoffset)
{
  g_return_if_fail (GIMP_IS_GRID (grid));

  if (xoffset) *xoffset = grid->xoffset;
  if (yoffset) *yoffset = grid->yoffset;
}

const gchar *
gimp_grid_parasite_name (void)
{
  return "gimp-image-grid";
}

GimpParasite *
gimp_grid_to_parasite (GimpGrid *grid)
{
  GimpParasite *parasite;
  gchar        *str;

  g_return_val_if_fail (GIMP_IS_GRID (grid), NULL);

  str = gimp_config_serialize_to_string (GIMP_CONFIG (grid), NULL);
  g_return_val_if_fail (str != NULL, NULL);

  parasite = gimp_parasite_new (gimp_grid_parasite_name (),
                                GIMP_PARASITE_PERSISTENT,
                                strlen (str) + 1, str);
  g_free (str);

  return parasite;
}

GimpGrid *
gimp_grid_from_parasite (const GimpParasite *parasite)
{
  GimpGrid    *grid;
  const gchar *str;
  GError      *error = NULL;

  g_return_val_if_fail (parasite != NULL, NULL);
  g_return_val_if_fail (strcmp (gimp_parasite_name (parasite),
                                gimp_grid_parasite_name ()) == 0, NULL);

  str = gimp_parasite_data (parasite);

  if (! str)
    {
      g_warning ("Empty grid parasite");

      return NULL;
    }

  grid = g_object_new (GIMP_TYPE_GRID, NULL);

  if (! gimp_config_deserialize_string (GIMP_CONFIG (grid),
                                        str,
                                        gimp_parasite_data_size (parasite),
                                        NULL,
                                        &error))
    {
      g_warning ("Failed to deserialize grid parasite: %s", error->message);
      g_error_free (error);
    }

  return grid;
}
