/* LIGMA - The GNU Image Manipulation Program
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

#ifndef __LIGMA_SUB_PROGRESS_H__
#define __LIGMA_SUB_PROGRESS_H__


#define LIGMA_TYPE_SUB_PROGRESS            (ligma_sub_progress_get_type ())
#define LIGMA_SUB_PROGRESS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_SUB_PROGRESS, LigmaSubProgress))
#define LIGMA_SUB_PROGRESS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_SUB_PROGRESS, LigmaSubProgressClass))
#define LIGMA_IS_SUB_PROGRESS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_SUB_PROGRESS))
#define LIGMA_IS_SUB_PROGRESS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_SUB_PROGRESS))
#define LIGMA_SUB_PROGRESS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_SUB_PROGRESS, LigmaSubProgressClass))


typedef struct _LigmaSubProgressClass LigmaSubProgressClass;

struct _LigmaSubProgress
{
  GObject       parent_instance;

  LigmaProgress *progress;
  gdouble       start;
  gdouble       end;
};

struct _LigmaSubProgressClass
{
  GObjectClass  parent_class;
};


GType          ligma_sub_progress_get_type (void) G_GNUC_CONST;

LigmaProgress * ligma_sub_progress_new       (LigmaProgress    *progress);
void           ligma_sub_progress_set_range (LigmaSubProgress *progress,
                                            gdouble          start,
                                            gdouble          end);
void           ligma_sub_progress_set_step  (LigmaSubProgress *progress,
                                            gint             index,
                                            gint             num_steps);



#endif /* __LIGMA_SUB_PROGRESS_H__ */
