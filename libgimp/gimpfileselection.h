/* LIBGIMP - The GIMP Library                                                   
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball                
 *
 * gimpfileselection.h
 * Copyright (C) 1999 Michael Natterer <mitch@gimp.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.             
 *                                                                              
 * This library is distributed in the hope that it will be useful,              
 * but WITHOUT ANY WARRANTY; without even the implied warranty of               
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU            
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_FILE_SELECTION_H__
#define __GIMP_FILE_SELECTION_H__

#include <gtk/gtk.h>

#include "gimppixmap.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define GIMP_TYPE_FILE_SELECTION            (gimp_file_selection_get_type ())
#define GIMP_FILE_SELECTION(obj)            (GTK_CHECK_CAST ((obj), GIMP_TYPE_FILE_SELECTION, GimpFileSelection))
#define GIMP_FILE_SELECTION_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GIMP_TYPE_FILE_SELECTION, GimpFileSelectionClass))
#define GIMP_IS_FILE_SELECTION(obj)         (GTK_CHECK_TYPE (obj, GIMP_TYPE_FILE_SELECTION))
#define GIMP_IS_FILE_SELECTION_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_FILE_SELECTION))

typedef struct _GimpFileSelection       GimpFileSelection;
typedef struct _GimpFileSelectionClass  GimpFileSelectionClass;

struct _GimpFileSelection
{
  GtkHBox     hbox;

  GtkWidget  *yes_pixmap;
  GtkWidget  *no_pixmap;
  GtkWidget  *entry;
  GtkWidget  *browse_button;

  GtkWidget  *file_selection;

  gchar      *title;
  gboolean    dir_only;
  gboolean    check_valid;
};

struct _GimpFileSelectionClass
{
  GtkHBoxClass parent_class;

  void (* filename_changed) (GimpFileSelection *gfs);
};

/* For information look into the C source or the html documentation */

GtkType     gimp_file_selection_get_type    (void);

GtkWidget*  gimp_file_selection_new         (gchar              *title,
					     gchar              *filename,
					     gboolean            dir_only,
					     gboolean            check_valid);

gchar*      gimp_file_selection_get_filename (GimpFileSelection *gfs);

void        gimp_file_selection_set_filename (GimpFileSelection *gfs,
					      gchar             *filename);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GIMP_FILE_SELECTION_H__ */
