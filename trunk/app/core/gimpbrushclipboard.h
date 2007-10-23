/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpbrushclipboard.h
 * Copyright (C) 2006 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_BRUSH_CLIPBOARD_H__
#define __GIMP_BRUSH_CLIPBOARD_H__


#include "gimpbrush.h"


#define GIMP_TYPE_BRUSH_CLIPBOARD            (gimp_brush_clipboard_get_type ())
#define GIMP_BRUSH_CLIPBOARD(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_BRUSH_CLIPBOARD, GimpBrushClipboard))
#define GIMP_BRUSH_CLIPBOARD_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_BRUSH_CLIPBOARD, GimpBrushClipboardClass))
#define GIMP_IS_BRUSH_CLIPBOARD(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_BRUSH_CLIPBOARD))
#define GIMP_IS_BRUSH_CLIPBOARD_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_BRUSH_CLIPBOARD))
#define GIMP_BRUSH_CLIPBOARD_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_BRUSH_CLIPBOARD, GimpBrushClipboardClass))


typedef struct _GimpBrushClipboardClass GimpBrushClipboardClass;

struct _GimpBrushClipboard
{
  GimpBrush  parent_instance;

  Gimp      *gimp;
};

struct _GimpBrushClipboardClass
{
  GimpBrushClass  parent_class;
};


GType      gimp_brush_clipboard_get_type (void) G_GNUC_CONST;

GimpData * gimp_brush_clipboard_new      (Gimp *gimp);


#endif  /*  __GIMP_BRUSH_CLIPBOARD_H__  */
