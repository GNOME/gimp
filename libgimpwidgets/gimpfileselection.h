/* LIBGIMP - The GIMP Library                                                   
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball                
 *
 * gimpfileselection.h
 * Copyright (C) 1999 Michael Natterer <mitschel@cs.tu-berlin.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.             
 *                                                                              
 * This library is distributed in the hope that it will be useful,              
 * but WITHOUT ANY WARRANTY; without even the implied warranty of               
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU            
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_FILE_SELECTION_H__
#define __GIMP_FILE_SELECTION_H__

#include <gtk/gtk.h>

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
  GtkVBox    hbox;

  GtkWidget *file_exists;
  GtkWidget *entry;
  GtkWidget *browse_button;

  GtkWidget *file_selection;

  GdkPixmap *yes_pixmap;
  GdkBitmap *yes_mask;
  GdkPixmap *no_pixmap;
  GdkBitmap *no_mask;

  gchar     *title;
  gboolean   dir_only;
  gboolean   check_valid;
};

struct _GimpFileSelectionClass
{
  GtkHBoxClass parent_class;

  void (* gimp_file_selection) (GimpFileSelection *gfs);
};

guint       gimp_file_selection_get_type    (void);

/*  creates a new GimpFileSelection widget
 *
 *  dir_only    == TRUE  will allow only directories
 *  check_valid == TRUE  will show a pixmap which indicates if
 *                       the filename is valid
 */
GtkWidget*  gimp_file_selection_new         (gchar             *title,
					     gchar             *filename,
					     gboolean           dir_only,
					     gboolean           check_valid);

/*  it's up to the caller to g_free() the returned string
 */
gchar*      gimp_file_selection_get_filename (GimpFileSelection *gfs);

void        gimp_file_selection_set_filename (GimpFileSelection *gfs,
					      gchar             *filename);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GIMP_FILE_SELECTION_H__ */
