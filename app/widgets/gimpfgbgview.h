/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpfgbgview.h
 * Copyright (C) 2005  Sven Neumann <sven@gimp.org>
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

#ifndef __GIMP_FG_BG_VIEW_H__
#define __GIMP_FG_BG_VIEW_H__


#define GIMP_TYPE_FG_BG_VIEW            (gimp_fg_bg_view_get_type ())
#define GIMP_FG_BG_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_FG_BG_VIEW, GimpFgBgView))
#define GIMP_FG_BG_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_FG_BG_VIEW, GimpFgBgViewClass))
#define GIMP_IS_FG_BG_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_FG_BG_VIEW))
#define GIMP_IS_FG_BG_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_FG_BG_VIEW))
#define GIMP_FG_BG_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_FG_BG_VIEW, GimpFgBgViewClass))


typedef struct _GimpFgBgViewClass GimpFgBgViewClass;

struct _GimpFgBgView
{
  GtkWidget           parent_instance;

  GimpContext        *context;
  GimpColorConfig    *color_config;
  GimpColorTransform *transform;
};

struct _GimpFgBgViewClass
{
  GtkWidgetClass  parent_class;
};


GType       gimp_fg_bg_view_get_type    (void) G_GNUC_CONST;

GtkWidget * gimp_fg_bg_view_new         (GimpContext  *context);

void        gimp_fg_bg_view_set_context (GimpFgBgView *view,
                                         GimpContext  *context);


#endif  /*  __GIMP_FG_BG_VIEW_H__  */
