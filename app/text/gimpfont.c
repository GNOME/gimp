/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpfont.c
 * Copyright (C) 2003 Michael Natterer <mitch@gimp.org>
 *                    Sven Neumann <sven@gimp.org>
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

#include "text-types.h"

#include "gimpfont.h"


static void    gimp_font_class_init   (GimpFontClass *klass);
static void    gimp_font_init         (GimpFont      *font);

static void    gimp_font_finalize     (GObject       *object);

static gsize   gimp_font_get_memsize  (GimpObject    *object);


static GimpViewableClass *parent_class = NULL;


GType
gimp_font_get_type (void)
{
  static GType font_type = 0;

  if (! font_type)
    {
      static const GTypeInfo font_info =
      {
        sizeof (GimpFontClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_font_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_font     */
	sizeof (GimpFont),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_font_init,
      };

      font_type = g_type_register_static (GIMP_TYPE_VIEWABLE,
					  "GimpFont",
					  &font_info, 0);
  }

  return font_type;
}

static void
gimp_font_class_init (GimpFontClass *klass)
{
  GObjectClass    *object_class;
  GimpObjectClass *gimp_object_class;

  object_class      = G_OBJECT_CLASS (klass);
  gimp_object_class = GIMP_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize          = gimp_font_finalize;

  gimp_object_class->get_memsize  = gimp_font_get_memsize;
}

static void
gimp_font_init (GimpFont *font)
{
}

static void
gimp_font_finalize (GObject *object)
{
  GimpFont *font;

  font = GIMP_FONT (object);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gsize
gimp_font_get_memsize (GimpObject *object)
{
  GimpFont *font;
  gsize     memsize = 0;

  font = GIMP_FONT (object);

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object);
}

GimpFont *
gimp_font_get_standard (void)
{
  static GimpFont *standard_font = NULL;

  if (! standard_font)
    standard_font = g_object_new (GIMP_TYPE_FONT,
                                  "name", "Sans",
                                  NULL);

  return standard_font;
}
