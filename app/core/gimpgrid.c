/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpGrid
 * Copyright (C) 2003  Henrik Brix Andersen <brix@gimp.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <glib-object.h>

#include "libgimpbase/gimplimits.h"
#include "libgimpcolor/gimpcolor.h"

#include "core-types.h"

#include "config/gimpconfig.h"
#include "config/gimpconfig-params.h"

#include "gimpgrid.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_XSPACING,
  PROP_YSPACING,
  PROP_SPACING_UNIT,
  PROP_XOFFSET,
  PROP_YOFFSET,
  PROP_OFFSET_UNIT,
  PROP_FGCOLOR,
  PROP_BGCOLOR,
  PROP_TYPE
};

static void gimp_grid_class_init   (GimpGridClass *klass);
static void gimp_grid_finalize     (GObject       *object);
static void gimp_grid_get_property (GObject       *object,
                                    guint          property_id,
                                    GValue        *value,
                                    GParamSpec    *pspec);
static void gimp_grid_set_property (GObject       *object,
                                    guint          property_id,
                                    const GValue  *value,
                                    GParamSpec    *pspec);


static GimpObjectClass *parent_class = NULL;


GType
gimp_grid_get_type (void)
{
  static GType grid_type = 0;

  if (! grid_type)
    {
      static const GTypeInfo grid_info =
      {
        sizeof (GimpGridClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_grid_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpGrid),
	0,              /* n_preallocs    */
	NULL            /* instance_init  */
      };
      static const GInterfaceInfo grid_iface_info =
      {
        NULL,           /* iface_init     */
        NULL,           /* iface_finalize */
        NULL            /* iface_data     */
      };

      grid_type = g_type_register_static (GIMP_TYPE_OBJECT,
                                          "GimpGrid", &grid_info, 0);

      g_type_add_interface_static (grid_type,
                                   GIMP_TYPE_CONFIG_INTERFACE,
                                   &grid_iface_info);
    }

  return grid_type;
}

static void
gimp_grid_class_init (GimpGridClass *klass)
{
  GObjectClass *object_class;
  GimpRGB       black;
  GimpRGB       white;

  object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize     = gimp_grid_finalize;
  object_class->get_property = gimp_grid_get_property;
  object_class->set_property = gimp_grid_set_property;

  gimp_rgba_set (&black, 0.0, 0.0, 0.0, GIMP_OPACITY_OPAQUE);
  gimp_rgba_set (&white, 1.0, 1.0, 1.0, GIMP_OPACITY_OPAQUE);

  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_XSPACING,
				   "xspacing", NULL,
				   1.0, GIMP_MAX_IMAGE_SIZE, 10.0,
				   0);
  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_YSPACING,
				   "yspacing", NULL,
				   1.0, GIMP_MAX_IMAGE_SIZE, 10.0,
				   0);
  GIMP_CONFIG_INSTALL_PROP_UNIT (object_class, PROP_SPACING_UNIT,
				 "spacing-unit", NULL,
				 FALSE, FALSE, GIMP_UNIT_INCH,
				 0);
  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_XOFFSET,
				   "xoffset", NULL,
				   - GIMP_MAX_IMAGE_SIZE,
                                   GIMP_MAX_IMAGE_SIZE, 0.0,
				   0);
  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_YOFFSET,
				   "yoffset", NULL,
				   - GIMP_MAX_IMAGE_SIZE,
                                   GIMP_MAX_IMAGE_SIZE, 0.0,
				   0);
  GIMP_CONFIG_INSTALL_PROP_UNIT (object_class, PROP_OFFSET_UNIT,
				 "offset-unit", NULL,
				 FALSE, FALSE, GIMP_UNIT_INCH,
				 0);
  GIMP_CONFIG_INSTALL_PROP_COLOR (object_class, PROP_FGCOLOR,
				  "fgcolor", NULL,
				  &black,
				  0);
  GIMP_CONFIG_INSTALL_PROP_COLOR (object_class, PROP_BGCOLOR,
				  "bgcolor", NULL,
				  &white,
				  0);
  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_TYPE,
                                 "type", NULL,
                                 GIMP_TYPE_GRID_TYPE,
                                 GIMP_GRID_TYPE_INTERSECTION,
                                 0);
}

static void
gimp_grid_finalize (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->finalize (object);
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
    case PROP_FGCOLOR:
      g_value_set_boxed (value, &grid->fgcolor);
      break;
    case PROP_BGCOLOR:
      g_value_set_boxed (value, &grid->bgcolor);
      break;
    case PROP_TYPE:
      g_value_set_enum (value, grid->type);
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
    case PROP_FGCOLOR:
      color = g_value_get_boxed (value);
      grid->fgcolor = *color;
      break;
    case PROP_BGCOLOR:
      color = g_value_get_boxed (value);
      grid->bgcolor = *color;
      break;
    case PROP_TYPE:
      grid->type = g_value_get_enum (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}
