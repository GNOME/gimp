/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpText
 * Copyright (C) 2002-2003  Sven Neumann <sven@gimp.org>
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

#ifndef __GIMP_TEXT_H__
#define __GIMP_TEXT_H__

#define GIMP_TYPE_TEXT            (gimp_text_get_type ())
#define GIMP_TEXT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_TEXT, GimpText))
#define GIMP_TEXT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_TEXT, GimpTextClass))
#define GIMP_IS_TEXT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_TEXT))
#define GIMP_IS_TEXT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_TEXT))
#define GIMP_TEXT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_TEXT, GimpTextClass))


typedef struct _GimpTextClass  GimpTextClass;

struct _GimpText
{
  GObject                parent_instance;

  gchar                 *text;
  gchar                 *font;
  gdouble                font_size;
  GimpUnit               font_size_unit;
  gboolean               hinting;
  gboolean               autohint;
  gboolean               antialias;
  gchar                 *language;
  gint                   base_dir;  /* actually a PangoDirection */
  GimpRGB                color;
  GimpTextJustification  justify;
  gdouble                indent;
  gdouble                line_spacing;
  gdouble                letter_spacing;
  gint                   box_width;

  /* for historical reasons, don't use */
  gint                   border;
};

struct _GimpTextClass
{
  GObjectClass           parent_class;
};


GType  gimp_text_get_type (void) G_GNUC_CONST;


#endif /* __GIMP_TEXT_H__ */
