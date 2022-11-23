/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmafileprocview.h
 * Copyright (C) 2004  Sven Neumann <sven@ligma.org>
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

#ifndef __LIGMA_FILE_PROC_VIEW_H__
#define __LIGMA_FILE_PROC_VIEW_H__


#define LIGMA_TYPE_FILE_PROC_VIEW            (ligma_file_proc_view_get_type ())
#define LIGMA_FILE_PROC_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_FILE_PROC_VIEW, LigmaFileProcView))
#define LIGMA_FILE_PROC_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_FILE_PROC_VIEW, LigmaFileProcViewClass))
#define LIGMA_IS_FILE_PROC_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_FILE_PROC_VIEW))
#define LIGMA_IS_FILE_PROC_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_FILE_PROC_VIEW))
#define LIGMA_FILE_PROC_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_FILE_PROC_VIEW, LigmaFileProcViewClass))


typedef struct _LigmaFileProcViewClass LigmaFileProcViewClass;

struct _LigmaFileProcView
{
  GtkTreeView        parent_instance;

  GList             *meta_extensions;
};

struct _LigmaFileProcViewClass
{
  GtkTreeViewClass   parent_class;

  void (* changed) (LigmaFileProcView *view);
};


GType                 ligma_file_proc_view_get_type    (void) G_GNUC_CONST;

GtkWidget           * ligma_file_proc_view_new         (Ligma                 *ligma,
                                                       GSList               *procedures,
                                                       const gchar          *automatic,
                                                       const gchar          *automatic_help_id);

LigmaPlugInProcedure * ligma_file_proc_view_get_proc    (LigmaFileProcView     *view,
                                                       gchar               **label,
                                                       GtkFileFilter       **filter);
gboolean              ligma_file_proc_view_set_proc    (LigmaFileProcView     *view,
                                                       LigmaPlugInProcedure  *proc);

gchar               * ligma_file_proc_view_get_help_id (LigmaFileProcView     *view);


#endif  /*  __LIGMA_FILE_PROC_VIEW_H__  */
