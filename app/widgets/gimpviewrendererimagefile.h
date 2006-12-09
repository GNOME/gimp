/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpviewrendererimagefile.h
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_VIEW_RENDERER_IMAGEFILE_H__
#define __GIMP_VIEW_RENDERER_IMAGEFILE_H__


#include "gimpviewrenderer.h"

/* #define ENABLE_FILE_SYSTEM_ICONS 1 */


#define GIMP_TYPE_VIEW_RENDERER_IMAGEFILE            (gimp_view_renderer_imagefile_get_type ())
#define GIMP_VIEW_RENDERER_IMAGEFILE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_VIEW_RENDERER_IMAGEFILE, GimpViewRendererImagefile))
#define GIMP_VIEW_RENDERER_IMAGEFILE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_VIEW_RENDERER_IMAGEFILE, GimpViewRendererImagefileClass))
#define GIMP_IS_VIEW_RENDERER_IMAGEFILE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, GIMP_TYPE_VIEW_RENDERER_IMAGEFILE))
#define GIMP_IS_VIEW_RENDERER_IMAGEFILE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_VIEW_RENDERER_IMAGEFILE))
#define GIMP_VIEW_RENDERER_IMAGEFILE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_VIEW_RENDERER_IMAGEFILE, GimpViewRendererImagefileClass))


typedef struct _GimpViewRendererImagefileClass  GimpViewRendererImagefileClass;

struct _GimpViewRendererImagefile
{
  GimpViewRenderer parent_instance;

#ifdef ENABLE_FILE_SYSTEM_ICONS
  gpointer         file_system;
#endif
};

struct _GimpViewRendererImagefileClass
{
  GimpViewRendererClass parent_class;
};


GType   gimp_view_renderer_imagefile_get_type (void) G_GNUC_CONST;


#endif /* __GIMP_VIEW_RENDERER_IMAGEFILE_H__ */
