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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_SUB_PROGRESS_H__
#define __GIMP_SUB_PROGRESS_H__


#define GIMP_TYPE_SUB_PROGRESS            (gimp_sub_progress_get_type ())
#define GIMP_SUB_PROGRESS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_SUB_PROGRESS, GimpSubProgress))
#define GIMP_SUB_PROGRESS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_SUB_PROGRESS, GimpSubProgressClass))
#define GIMP_IS_SUB_PROGRESS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_SUB_PROGRESS))
#define GIMP_IS_SUB_PROGRESS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_SUB_PROGRESS))
#define GIMP_SUB_PROGRESS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_SUB_PROGRESS, GimpSubProgressClass))


typedef struct _GimpSubProgressClass GimpSubProgressClass;

struct _GimpSubProgress
{
  GObject       parent_instance;

  GimpProgress *progress;
  gdouble       start;
  gdouble       end;
};

struct _GimpSubProgressClass
{
  GObjectClass  parent_class;
};


GType          gimp_sub_progress_get_type (void) G_GNUC_CONST;

GimpProgress * gimp_sub_progress_new       (GimpProgress    *progress);
void           gimp_sub_progress_set_range (GimpSubProgress *progress,
                                            gdouble          start,
                                            gdouble          end);
void           gimp_sub_progress_set_step  (GimpSubProgress *progress,
                                            gint             index,
                                            gint             num_steps);



#endif /* __GIMP_SUB_PROGRESS_H__ */
