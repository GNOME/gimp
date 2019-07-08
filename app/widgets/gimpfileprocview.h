/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpfileprocview.h
 * Copyright (C) 2004  Sven Neumann <sven@gimp.org>
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

#ifndef __GIMP_FILE_PROC_VIEW_H__
#define __GIMP_FILE_PROC_VIEW_H__


#define GIMP_TYPE_FILE_PROC_VIEW            (gimp_file_proc_view_get_type ())
#define GIMP_FILE_PROC_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_FILE_PROC_VIEW, GimpFileProcView))
#define GIMP_FILE_PROC_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_FILE_PROC_VIEW, GimpFileProcViewClass))
#define GIMP_IS_FILE_PROC_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_FILE_PROC_VIEW))
#define GIMP_IS_FILE_PROC_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_FILE_PROC_VIEW))
#define GIMP_FILE_PROC_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_FILE_PROC_VIEW, GimpFileProcViewClass))


typedef struct _GimpFileProcViewClass GimpFileProcViewClass;

struct _GimpFileProcView
{
  GtkTreeView        parent_instance;

  GList             *meta_extensions;
};

struct _GimpFileProcViewClass
{
  GtkTreeViewClass   parent_class;

  void (* changed) (GimpFileProcView *view);
};


GType                 gimp_file_proc_view_get_type    (void) G_GNUC_CONST;

GtkWidget           * gimp_file_proc_view_new         (Gimp                 *gimp,
                                                       GSList               *procedures,
                                                       const gchar          *automatic,
                                                       const gchar          *automatic_help_id);

GimpPlugInProcedure * gimp_file_proc_view_get_proc    (GimpFileProcView     *view,
                                                       gchar               **label,
                                                       GtkFileFilter       **filter);
gboolean              gimp_file_proc_view_set_proc    (GimpFileProcView     *view,
                                                       GimpPlugInProcedure  *proc);

gchar               * gimp_file_proc_view_get_help_id (GimpFileProcView     *view);


#endif  /*  __GIMP_FILE_PROC_VIEW_H__  */
