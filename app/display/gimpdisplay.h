/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#ifndef __GIMP_DISPLAY_H__
#define __GIMP_DISPLAY_H__


#include "core/gimpobject.h"


#define GIMP_TYPE_DISPLAY            (gimp_display_get_type ())
#define GIMP_DISPLAY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_DISPLAY, GimpDisplay))
#define GIMP_DISPLAY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_DISPLAY, GimpDisplayClass))
#define GIMP_IS_DISPLAY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_DISPLAY))
#define GIMP_IS_DISPLAY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_DISPLAY))
#define GIMP_DISPLAY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_DISPLAY, GimpDisplayClass))


typedef struct _GimpDisplayClass GimpDisplayClass;

struct _GimpDisplay
{
  GimpObject  parent_instance;

  gint        ID;               /*  unique identifier for this gdisplay     */

  GimpImage  *gimage;	        /*  pointer to the associated gimage        */
  gint        instance;         /*  the instance # of this gdisplay as      */
                                /*  taken from the gimage at creation       */

  GtkWidget  *shell;            /*  shell widget for this gdisplay          */

  GSList     *update_areas;     /*  Update areas list                       */
};

struct _GimpDisplayClass
{
  GimpObjectClass  parent_class;
};


GType         gimp_display_get_type    (void) G_GNUC_CONST;

GimpDisplay * gimp_display_new         (GimpImage       *gimage,
                                        GimpUnit         unit,
                                        gdouble          scale,
                                        GimpMenuFactory *menu_factory,
                                        GimpUIManager   *popup_manager);
void          gimp_display_delete      (GimpDisplay     *gdisp);

gint          gimp_display_get_ID      (GimpDisplay     *gdisp);
GimpDisplay * gimp_display_get_by_ID   (Gimp            *gimp,
                                        gint             ID);

void          gimp_display_reconnect   (GimpDisplay     *gdisp,
                                        GimpImage       *gimage);

void          gimp_display_update_area (GimpDisplay     *gdisp,
                                        gboolean         now,
                                        gint             x,
                                        gint             y,
                                        gint             w,
                                        gint             h);

void          gimp_display_flush       (GimpDisplay     *gdisp);
void          gimp_display_flush_now   (GimpDisplay     *gdisp);


#endif /*  __GIMP_DISPLAY_H__  */
