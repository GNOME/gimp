
/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */


#include "config.h"
#include "gimp.h"


/*
 * Subclasses of GimpResource.
 *
 * id_is_valid and other methods are in pdb/groups/<foo>.pd
 */


/* Each subclass:
 * Is Final.
 * Inherits GimpResource.
 *
 * See pdb/groups/<subclass>.pdb for:
 *    - annotations for a subclass,
 *    - more methods of a subclass,
 *
 * In bound languages, notation is Gimp.Brush.get_id()
 * C code should use the superclass method, for example:
 *    gimp_resource_get_id (GIMP_RESOURCE (brush));
 *
 * For some methods, we need the same function name on the libgimp side and app/core side.
 * Until gimp/core has a GimpResource class and GimpBrush derives from it,
 * we must define the subclass specific gimp_brush_get_id method on the libgimp side.
 *
 * Some methods are in libgimp but not in the PDB e.g. is_valid
 *
 * Methods defined here may call class methods in the PDB.
 * E.G. is_valid(self) calls class method id_is_valid(char *id)
 */

 /* get_id is defined only for the superclass GimpResource.
  * C code using libgimp can use:  gimp_resource_get_id (GIMP_RESOURCE (brush))
  * Note the macro to cast to the superclass to avoid compiler warnings.
  * Bound languages can use for example: brush.get_id()
  */


/* Brush */

G_DEFINE_TYPE (GimpBrush, gimp_brush, GIMP_TYPE_RESOURCE);

static void gimp_brush_class_init (GimpBrushClass *klass) {}
static void gimp_brush_init       (GimpBrush       *self) {}


/**
 * gimp_brush_is_valid:
 * @self: The brush to check.
 *
 * Whether the brush has an ID that refers to installed data.
 * The data might have been uninstalled after this object was created
 * from saved settings.
 *
 * Returns: TRUE if the brush's ID refers to existing data.
 *
 * Since: 3.0
 */
gboolean
gimp_brush_is_valid (GimpBrush *self)
{
  gchar *id;

  /* Call superclass on cast self. */
  id = gimp_resource_get_id (GIMP_RESOURCE (self));
  /* Call class method in the PDB, which crosses the wire to core. */
  return gimp_brush_id_is_valid (id);
}


/* Font */

G_DEFINE_TYPE (GimpFont, gimp_font, GIMP_TYPE_RESOURCE);

static void gimp_font_class_init (GimpFontClass *klass) {}
static void gimp_font_init       (GimpFont       *self) {}


/**
 * gimp_font_is_valid:
 * @self: The font to check.
 *
 * Whether the font has an ID that refers to installed data.
 * The data might have been uninstalled after this object was created
 * from saved settings.
 *
 * Returns: TRUE if the font's ID refers to existing data.
 *
 * Since: 3.0
 */
gboolean
gimp_font_is_valid (GimpFont *self)
{
  gchar *id;

  /* Call superclass on cast self. */
  id = gimp_resource_get_id (GIMP_RESOURCE (self));
  /* Call class method in the PDB, which crosses the wire to core. */
  return gimp_font_id_is_valid (id);
}


/* Gradient */

G_DEFINE_TYPE (GimpGradient, gimp_gradient, GIMP_TYPE_RESOURCE);

static void gimp_gradient_class_init (GimpGradientClass *klass) {}
static void gimp_gradient_init       (GimpGradient       *self) {}


/**
 * gimp_gradient_is_valid:
 * @self: The gradient to check.
 *
 * Whether the gradient has an ID that refers to installed data.
 * The data might have been uninstalled after this object was created
 * from saved settings.
 *
 * Returns: TRUE if the gradient's ID refers to existing data.
 *
 * Since: 3.0
 */
gboolean
gimp_gradient_is_valid (GimpGradient *self)
{
  gchar *id;

  /* Call superclass on cast self. */
  id = gimp_resource_get_id (GIMP_RESOURCE (self));
  /* Call class method in the PDB, which crosses the wire to core. */
  return gimp_gradient_id_is_valid (id);
}


/* Palette */

G_DEFINE_TYPE (GimpPalette, gimp_palette, GIMP_TYPE_RESOURCE);

static void gimp_palette_class_init (GimpPaletteClass *klass) {}
static void gimp_palette_init       (GimpPalette       *self) {}


/**
 * gimp_palette_is_valid:
 * @self: The palette to check.
 *
 * Whether the palette has an ID that refers to installed data.
 * The data might have been uninstalled after this object was created
 * from saved settings.
 *
 * Returns: TRUE if the palette's ID refers to existing data.
 *
 * Since: 3.0
 */
gboolean
gimp_palette_is_valid (GimpPalette *self)
{
  gchar *id;

  /* Call superclass on cast self. */
  id = gimp_resource_get_id (GIMP_RESOURCE (self));
  /* Call class method in the PDB, which crosses the wire to core. */
  return gimp_palette_id_is_valid (id);
}


/* Pattern */

G_DEFINE_TYPE (GimpPattern, gimp_pattern, GIMP_TYPE_RESOURCE);

static void gimp_pattern_class_init (GimpPatternClass *klass) {}
static void gimp_pattern_init       (GimpPattern       *self) {}


/**
 * gimp_pattern_is_valid:
 * @self: The pattern to check.
 *
 * Whether the pattern has an ID that refers to installed data.
 * The data might have been uninstalled after this object was created
 * from saved settings.
 *
 * Returns: TRUE if the pattern's ID refers to existing data.
 *
 * Since: 3.0
 */
gboolean
gimp_pattern_is_valid (GimpPattern *self)
{
  gchar *id;

  /* Call superclass on cast self. */
  id = gimp_resource_get_id (GIMP_RESOURCE (self));
  /* Call class method in the PDB, which crosses the wire to core. */
  return gimp_pattern_id_is_valid (id);
}
