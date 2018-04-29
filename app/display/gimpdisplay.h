/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
  GimpObject         parent_instance;

  Gimp              *gimp;
  GimpDisplayConfig *config;

};

struct _GimpDisplayClass
{
  GimpObjectClass  parent_class;
};


GType              gimp_display_get_type        (void) G_GNUC_CONST;

GimpDisplay      * gimp_display_new             (Gimp              *gimp,
                                                 GimpImage         *image,
                                                 GimpUnit           unit,
                                                 gdouble            scale,
                                                 GimpUIManager     *popup_manager,
                                                 GimpDialogFactory *dialog_factory,
                                                 GdkMonitor        *monitor);
void               gimp_display_delete          (GimpDisplay       *display);
void               gimp_display_close           (GimpDisplay       *display);

gint               gimp_display_get_ID          (GimpDisplay       *display);
GimpDisplay      * gimp_display_get_by_ID       (Gimp              *gimp,
                                                 gint               ID);

gchar            * gimp_display_get_action_name (GimpDisplay       *display);

Gimp             * gimp_display_get_gimp        (GimpDisplay       *display);

GimpImage        * gimp_display_get_image       (GimpDisplay       *display);
void               gimp_display_set_image       (GimpDisplay       *display,
                                                 GimpImage         *image);

gint               gimp_display_get_instance    (GimpDisplay       *display);

GimpDisplayShell * gimp_display_get_shell       (GimpDisplay       *display);

void               gimp_display_empty           (GimpDisplay       *display);
void               gimp_display_fill            (GimpDisplay       *display,
                                                 GimpImage         *image,
                                                 GimpUnit           unit,
                                                 gdouble            scale);

void               gimp_display_update_area     (GimpDisplay       *display,
                                                 gboolean           now,
                                                 gint               x,
                                                 gint               y,
                                                 gint               w,
                                                 gint               h);

void               gimp_display_flush           (GimpDisplay       *display);
void               gimp_display_flush_now       (GimpDisplay       *display);


#endif /*  __GIMP_DISPLAY_H__  */
