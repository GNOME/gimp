/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpfont.h
 * Copyright (C) 2003 Michael Natterer <mitch@gimp.org>
 *                    Sven Neumann <sven@gimp.org>
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

#pragma once

#include "core/gimpdata.h"


#define GIMP_TYPE_FONT            (gimp_font_get_type ())
#define GIMP_FONT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_FONT, GimpFont))
#define GIMP_FONT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_FONT, GimpFontClass))
#define GIMP_IS_FONT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_FONT))
#define GIMP_IS_FONT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_FONT))
#define GIMP_FONT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_FONT, GimpFontClass))


typedef struct _GimpFontClass GimpFontClass;

GType         gimp_font_get_type               (void) G_GNUC_CONST;

GimpData    * gimp_font_get_standard           (void);
const gchar * gimp_font_get_lookup_name        (GimpFont        *font);
void          gimp_font_set_lookup_name        (GimpFont        *font,
                                                gchar           *name);
gboolean      gimp_font_match_by_lookup_name   (GimpFont        *font,
                                                const gchar     *name);
gboolean      gimp_font_match_by_description   (GimpFont        *font,
                                                const gchar     *desc);
void          gimp_font_set_font_info          (GimpFont        *font,
                                                gpointer         font_info[]);
void          gimp_font_class_set_font_factory (GimpFontFactory *factory);

enum
{
  /* properties for serialization*/
  PROP_HASH,
  PROP_FULLNAME,
  PROP_FAMILY,
  PROP_STYLE,
  PROP_PSNAME,
  PROP_WEIGHT,
  PROP_WIDTH,
  PROP_INDEX,
  PROP_SLANT,
  PROP_FONTVERSION,

  /*for backward compatibility*/
  PROP_DESC,
  PROP_FILE,

  PROPERTIES_COUNT
};
