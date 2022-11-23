/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * LigmaGrid
 * Copyright (C) 2003  Henrik Brix Andersen <brix@ligma.org>
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

#include "libligmamath/ligmamath.h"
#include "libligmabase/ligmabase.h"
#include "libligmaconfig/ligmaconfig.h"

#include "libligmacolor/ligmacolor.h"

#include "core-types.h"

#include "ligmagrid.h"

#include "ligma-intl.h"


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


static void   ligma_grid_get_property (GObject      *object,
                                      guint         property_id,
                                      GValue       *value,
                                      GParamSpec   *pspec);
static void   ligma_grid_set_property (GObject      *object,
                                      guint         property_id,
                                      const GValue *value,
                                      GParamSpec   *pspec);


G_DEFINE_TYPE_WITH_CODE (LigmaGrid, ligma_grid, LIGMA_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_CONFIG, NULL))


static void
ligma_grid_class_init (LigmaGridClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  LigmaRGB       black;
  LigmaRGB       white;

  object_class->get_property = ligma_grid_get_property;
  object_class->set_property = ligma_grid_set_property;

  ligma_rgba_set (&black, 0.0, 0.0, 0.0, LIGMA_OPACITY_OPAQUE);
  ligma_rgba_set (&white, 1.0, 1.0, 1.0, LIGMA_OPACITY_OPAQUE);

  LIGMA_CONFIG_PROP_ENUM (object_class, PROP_STYLE,
                         "style",
                         _("Line style"),
                         _("Line style used for the grid."),
                         LIGMA_TYPE_GRID_STYLE,
                         LIGMA_GRID_SOLID,
                         LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_RGB (object_class, PROP_FGCOLOR,
                        "fgcolor",
                        _("Foreground color"),
                        _("The foreground color of the grid."),
                        TRUE, &black,
                        LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_RGB (object_class, PROP_BGCOLOR,
                        "bgcolor",
                        _("Background color"),
                        _("The background color of the grid; "
                          "only used in double dashed line style."),
                        TRUE, &white,
                        LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_DOUBLE (object_class, PROP_XSPACING,
                           "xspacing",
                           _("Spacing X"),
                           _("Horizontal spacing of grid lines."),
                           0.0, LIGMA_MAX_IMAGE_SIZE, 10.0,
                           LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_DOUBLE (object_class, PROP_YSPACING,
                           "yspacing",
                           _("Spacing Y"),
                           _("Vertical spacing of grid lines."),
                           0.0, LIGMA_MAX_IMAGE_SIZE, 10.0,
                           LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_UNIT (object_class, PROP_SPACING_UNIT,
                         "spacing-unit",
                         _("Spacing unit"),
                         NULL,
                         FALSE, FALSE, LIGMA_UNIT_INCH,
                         LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_DOUBLE (object_class, PROP_XOFFSET,
                           "xoffset",
                           _("Offset X"),
                           _("Horizontal offset of the first grid "
                             "line; this may be a negative number."),
                           - LIGMA_MAX_IMAGE_SIZE,
                           LIGMA_MAX_IMAGE_SIZE, 0.0,
                           LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_DOUBLE (object_class, PROP_YOFFSET,
                           "yoffset",
                           _("Offset Y"),
                           _("Vertical offset of the first grid "
                             "line; this may be a negative number."),
                           - LIGMA_MAX_IMAGE_SIZE,
                           LIGMA_MAX_IMAGE_SIZE, 0.0,
                           LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_UNIT (object_class, PROP_OFFSET_UNIT,
                         "offset-unit",
                         _("Offset unit"),
                         NULL,
                         FALSE, FALSE, LIGMA_UNIT_INCH,
                         LIGMA_PARAM_STATIC_STRINGS);
}

static void
ligma_grid_init (LigmaGrid *grid)
{
}

static void
ligma_grid_get_property (GObject      *object,
                        guint         property_id,
                        GValue       *value,
                        GParamSpec   *pspec)
{
  LigmaGrid *grid = LIGMA_GRID (object);

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
ligma_grid_set_property (GObject      *object,
                        guint         property_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
  LigmaGrid *grid = LIGMA_GRID (object);
  LigmaRGB  *color;

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

LigmaGridStyle
ligma_grid_get_style (LigmaGrid *grid)
{
  g_return_val_if_fail (LIGMA_IS_GRID (grid), LIGMA_GRID_SOLID);

  return grid->style;
}

void
ligma_grid_get_fgcolor (LigmaGrid *grid,
                       LigmaRGB  *fgcolor)
{
  g_return_if_fail (LIGMA_IS_GRID (grid));
  g_return_if_fail (fgcolor != NULL);

  *fgcolor = grid->fgcolor;
}

void
ligma_grid_get_bgcolor (LigmaGrid *grid,
                       LigmaRGB  *bgcolor)
{
  g_return_if_fail (LIGMA_IS_GRID (grid));
  g_return_if_fail (bgcolor != NULL);

  *bgcolor = grid->bgcolor;
}

void
ligma_grid_get_spacing (LigmaGrid *grid,
                       gdouble  *xspacing,
                       gdouble  *yspacing)
{
  g_return_if_fail (LIGMA_IS_GRID (grid));

  if (xspacing) *xspacing = grid->xspacing;
  if (yspacing) *yspacing = grid->yspacing;
}

void
ligma_grid_get_offset (LigmaGrid *grid,
                      gdouble  *xoffset,
                      gdouble  *yoffset)
{
  g_return_if_fail (LIGMA_IS_GRID (grid));

  if (xoffset) *xoffset = grid->xoffset;
  if (yoffset) *yoffset = grid->yoffset;
}

const gchar *
ligma_grid_parasite_name (void)
{
  return "ligma-image-grid";
}

LigmaParasite *
ligma_grid_to_parasite (LigmaGrid *grid)
{
  g_return_val_if_fail (LIGMA_IS_GRID (grid), NULL);

  return ligma_config_serialize_to_parasite (LIGMA_CONFIG (grid),
                                            ligma_grid_parasite_name (),
                                            LIGMA_PARASITE_PERSISTENT,
                                            NULL);
}

LigmaGrid *
ligma_grid_from_parasite (const LigmaParasite *parasite)
{
  LigmaGrid *grid;
  GError   *error = NULL;

  g_return_val_if_fail (parasite != NULL, NULL);
  g_return_val_if_fail (strcmp (ligma_parasite_get_name (parasite),
                                ligma_grid_parasite_name ()) == 0, NULL);

  if (! ligma_parasite_get_data (parasite, NULL))
    {
      g_warning ("Empty grid parasite");

      return NULL;
    }

  grid = g_object_new (LIGMA_TYPE_GRID, NULL);

  if (! ligma_config_deserialize_parasite (LIGMA_CONFIG (grid),
                                          parasite,
                                          NULL,
                                          &error))
    {
      g_warning ("Failed to deserialize grid parasite: %s", error->message);
      g_error_free (error);
    }

  return grid;
}
