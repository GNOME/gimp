/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcolorhistory.h
 * Copyright (C) 2015 Jehan <jehan@girinstud.io>
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

#ifndef __GIMP_COLOR_HISTORY_H__
#define __GIMP_COLOR_HISTORY_H__


#define GIMP_TYPE_COLOR_HISTORY            (gimp_color_history_get_type ())
#define GIMP_COLOR_HISTORY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_COLOR_HISTORY, GimpColorHistory))
#define GIMP_COLOR_HISTORY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_COLOR_HISTORY, GimpColorHistoryClass))
#define GIMP_IS_COLOR_HISTORY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_COLOR_HISTORY))
#define GIMP_IS_COLOR_HISTORY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_COLOR_HISTORY))
#define GIMP_COLOR_HISTORY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_COLOR_HISTORY, GimpColorHistoryClass))


typedef struct _GimpColorHistoryClass GimpColorHistoryClass;

struct _GimpColorHistory
{
  GtkGrid       parent_instance;

  GimpContext  *context;

  GtkWidget   **color_areas;
  gint          history_size;
};

struct _GimpColorHistoryClass
{
  GtkGridClass  parent_class;

  /*  signals  */
  void   (* color_selected) (GimpColorHistory *history,
                             const GimpRGB    *rgb);
};


GType       gimp_color_history_get_type (void) G_GNUC_CONST;

GtkWidget * gimp_color_history_new      (GimpContext     *context,
                                         gint             history_size);

#endif /* __GIMP_COLOR_HISTORY_H__ */

