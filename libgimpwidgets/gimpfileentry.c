/* LIBGIMP - The GIMP Library 
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpfileselection.c
 * Copyright (C) 1999-2000 Michael Natterer <mitch@gimp.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <glib.h>         /* Needed here by Win32 gcc compilation */

#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <string.h>

#include <gtk/gtk.h>

#include "gimpwidgetstypes.h"

#include "gimpfileselection.h"

#include "libgimp/libgimp-intl.h"


#ifdef G_OS_WIN32
# ifndef S_ISDIR
#  define S_ISDIR(m) ((m) & _S_IFDIR)
# endif
# ifndef S_ISREG
#  define S_ISREG(m) ((m) & _S_IFREG)
# endif
#endif


static void   gimp_file_selection_class_init (GimpFileSelectionClass *klass);
static void   gimp_file_selection_init       (GimpFileSelection      *gfs);

static void   gimp_file_selection_destroy                  (GtkObject *object);

static void   gimp_file_selection_entry_callback           (GtkWidget *widget,
							    gpointer   data);
static gint   gimp_file_selection_entry_focus_out_callback (GtkWidget *widget,
							    GdkEvent  *event,
							    gpointer   data);
static void   gimp_file_selection_browse_callback          (GtkWidget *widget,
							    gpointer   data);

/*  private functions  */
static void gimp_file_selection_check_filename (GimpFileSelection *gfs);

enum
{
  FILENAME_CHANGED,
  LAST_SIGNAL
};

static guint gimp_file_selection_signals[LAST_SIGNAL] = { 0 };

static GtkHBoxClass *parent_class = NULL;


GType
gimp_file_selection_get_type (void)
{
  static GType gfs_type = 0;

  if (!gfs_type)
    {
      static const GTypeInfo gfs_info =
      {
        sizeof (GimpFileSelectionClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_file_selection_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data     */
	sizeof (GimpFileSelection),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_file_selection_init,
      };

      gfs_type = g_type_register_static (GTK_TYPE_HBOX, "GimpFileSelection", 
                                         &gfs_info, 0);
    }
  
  return gfs_type;
}

static void
gimp_file_selection_class_init (GimpFileSelectionClass *klass)
{
  GtkObjectClass *object_class;

  object_class = (GtkObjectClass *) klass;

  parent_class = g_type_class_peek_parent (klass);

  gimp_file_selection_signals[FILENAME_CHANGED] = 
    g_signal_new ("filename_changed",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpFileSelectionClass,
				   filename_changed),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  object_class->destroy   = gimp_file_selection_destroy;

  klass->filename_changed = NULL;
}

static void
gimp_file_selection_init (GimpFileSelection *gfs)
{
  gfs->title          = NULL;
  gfs->file_selection = NULL;
  gfs->check_valid    = FALSE;

  gfs->file_exists    = NULL;

  gtk_box_set_spacing (GTK_BOX (gfs), 2);
  gtk_box_set_homogeneous (GTK_BOX (gfs), FALSE);

  gfs->browse_button = gtk_button_new_with_label (" ... ");
  gtk_box_pack_end (GTK_BOX (gfs), gfs->browse_button, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT(gfs->browse_button), "clicked",
                    G_CALLBACK (gimp_file_selection_browse_callback),
                    gfs);
  gtk_widget_show (gfs->browse_button);

  gfs->entry = gtk_entry_new ();
  gtk_box_pack_end (GTK_BOX (gfs), gfs->entry, TRUE, TRUE, 0);
  g_signal_connect (G_OBJECT (gfs->entry), "activate",
                    G_CALLBACK (gimp_file_selection_entry_callback),
                    gfs);
  g_signal_connect (G_OBJECT (gfs->entry), "focus_out_event",
                    G_CALLBACK (gimp_file_selection_entry_focus_out_callback),
                    gfs);
  gtk_widget_show (gfs->entry);
}

static void
gimp_file_selection_destroy (GtkObject *object)
{
  GimpFileSelection *gfs;

  g_return_if_fail (GIMP_IS_FILE_SELECTION (object));

  gfs = GIMP_FILE_SELECTION (object);

  if (gfs->file_selection)
    {
      gtk_widget_destroy (gfs->file_selection);
      gfs->file_selection = NULL;
    }

  if (gfs->title)
    {
      g_free (gfs->title);
      gfs->title = NULL;
    }

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

/**
 * gimp_file_selection_new:
 * @title: The title of the #GtkFileSelection dialog.
 * @filename: The initial filename.
 * @dir_only: #TRUE if the file selection should accept directories only.
 * @check_valid: #TRUE if the widget should check if the entered file
 *               really exists.
 *
 * Creates a new #GimpFileSelection widget.
 *
 * Returns: A pointer to the new #GimpFileSelection widget.
 **/
GtkWidget *
gimp_file_selection_new (const gchar *title,
			 const gchar *filename,
			 gboolean     dir_only,
			 gboolean     check_valid)
{
  GimpFileSelection *gfs;

  gfs = g_object_new (GIMP_TYPE_FILE_SELECTION, NULL);

  gfs->title       = g_strdup (title);
  gfs->dir_only    = dir_only;
  gfs->check_valid = check_valid;

  if (check_valid)
    {
      gfs->file_exists = gtk_image_new_from_stock (GTK_STOCK_NO,
						   GTK_ICON_SIZE_BUTTON);
      gtk_box_pack_start (GTK_BOX (gfs), gfs->file_exists, FALSE, FALSE, 0);
      gtk_widget_show (gfs->file_exists);
    }

  gimp_file_selection_set_filename (gfs, filename);

  return GTK_WIDGET (gfs);
}

/**
 * gimp_file_selection_get_filename:
 * @gfs: The file selection you want to know the filename from.
 *
 * Note that you have to g_free() the returned string.
 *
 * Returns: The file or directory the user has entered.
 **/
gchar *
gimp_file_selection_get_filename (GimpFileSelection *gfs)
{
  g_return_val_if_fail (GIMP_IS_FILE_SELECTION (gfs), NULL);

  return gtk_editable_get_chars (GTK_EDITABLE (gfs->entry), 0, -1);
}

/**
 * gimp_file_selection_set_filename:
 * @gfs: The file selection you want to set the filename for.
 * @filename: The new filename.
 *
 * If you specified @check_valid as #TRUE in gimp_file_selection_new()
 * the #GimpFileSelection will immediately check the validity of the file
 * name.
 *
 */
void
gimp_file_selection_set_filename (GimpFileSelection *gfs,
				  const gchar       *filename)
{
  g_return_if_fail (GIMP_IS_FILE_SELECTION (gfs));

  gtk_entry_set_text (GTK_ENTRY (gfs->entry), filename ? filename : "");

  /*  update everything
   */
  gimp_file_selection_entry_callback (gfs->entry, (gpointer) gfs);
}

static void
gimp_file_selection_entry_callback (GtkWidget *widget,
				    gpointer   data)
{
  GimpFileSelection *gfs;
  gchar             *filename;
  gint               len;

  gfs = GIMP_FILE_SELECTION (data);

  /*  filenames still need more sanity checking
   *  (erase double G_DIR_SEPARATORS, ...)
   */
  filename = gtk_editable_get_chars (GTK_EDITABLE (widget), 0, -1);
  filename = g_strstrip (filename);

  while (((len = strlen (filename)) > 1) &&
	 (filename[len - 1] == G_DIR_SEPARATOR))
    filename[len - 1] = '\0';

  g_signal_handlers_block_by_func (G_OBJECT (gfs->entry), 
                                   gimp_file_selection_entry_callback,
                                   gfs);
  gtk_entry_set_text (GTK_ENTRY (gfs->entry), filename);
  g_signal_handlers_unblock_by_func (G_OBJECT (gfs->entry), 
                                     gimp_file_selection_entry_callback,
                                     gfs);

  if (gfs->file_selection)
    gtk_file_selection_set_filename (GTK_FILE_SELECTION (gfs->file_selection),
				     filename);
  g_free (filename);

  gimp_file_selection_check_filename (gfs);

  gtk_entry_set_position (GTK_ENTRY (gfs->entry), -1);

  g_signal_emit (G_OBJECT (gfs),
                 gimp_file_selection_signals[FILENAME_CHANGED], 0);
}

static gboolean
gimp_file_selection_entry_focus_out_callback (GtkWidget *widget,
					      GdkEvent  *event,
					      gpointer   data)
{
  gimp_file_selection_entry_callback (widget, data);

  return FALSE;
}

/*  local callbacks of gimp_file_selection_browse_callback()  */
static void
gimp_file_selection_filesel_ok_callback (GtkWidget *widget,
					 gpointer   data)
{
  GimpFileSelection *gfs;
  const gchar       *filename;

  gfs = GIMP_FILE_SELECTION (data);
  filename =
    gtk_file_selection_get_filename (GTK_FILE_SELECTION (gfs->file_selection));

  gtk_entry_set_text (GTK_ENTRY (gfs->entry), filename);

  gtk_widget_hide (gfs->file_selection);

  /*  update everything  */
  gimp_file_selection_entry_callback (gfs->entry, data);
}

static void
gimp_file_selection_browse_callback (GtkWidget *widget,
				     gpointer   data)
{
  GimpFileSelection *gfs;
  gchar             *filename;

  gfs = GIMP_FILE_SELECTION (data);
  filename = gtk_editable_get_chars (GTK_EDITABLE (gfs->entry), 0, -1);
  
  if (gfs->file_selection == NULL)
    {
      if (gfs->dir_only)
	{
	  gfs->file_selection = gtk_file_selection_new (gfs->title);

	  /*  hiding these widgets uses internal gtk+ knowledge, but it's
	   *  easier than creating my own directory browser -- michael
	   */
	  gtk_widget_hide
	    (GTK_FILE_SELECTION (gfs->file_selection)->fileop_del_file);
	  gtk_widget_hide
	    (GTK_FILE_SELECTION (gfs->file_selection)->file_list->parent);
	}
      else
	{
	  gfs->file_selection = gtk_file_selection_new (_("Select File"));
	}

      gtk_window_set_position (GTK_WINDOW (gfs->file_selection),
			       GTK_WIN_POS_MOUSE);
      gtk_window_set_wmclass (GTK_WINDOW (gfs->file_selection),
			      "file_select", "Gimp");

      /* slightly compress the dialog */
      gtk_container_set_border_width (GTK_CONTAINER (gfs->file_selection), 2);
      gtk_container_set_border_width (GTK_CONTAINER (GTK_FILE_SELECTION (gfs->file_selection)->button_area), 2);

      g_signal_connect 
	(G_OBJECT (GTK_FILE_SELECTION (gfs->file_selection)->ok_button),
	 "clicked",
	 G_CALLBACK (gimp_file_selection_filesel_ok_callback),
	 gfs);
      g_signal_connect
	(G_OBJECT (GTK_FILE_SELECTION (gfs->file_selection)->selection_entry),
	 "activate",
	 G_CALLBACK (gimp_file_selection_filesel_ok_callback),
	 gfs);

      g_signal_connect_swapped (G_OBJECT (GTK_FILE_SELECTION (gfs->file_selection)->cancel_button),
                                "clicked",
                                G_CALLBACK (gtk_widget_hide),
                                gfs->file_selection);
      g_signal_connect_swapped (GTK_OBJECT (gfs), "unmap",
                                G_CALLBACK (gtk_widget_hide),
                                gfs->file_selection);
      g_signal_connect_swapped (GTK_OBJECT (gfs->file_selection),
                                "delete_event",
                                G_CALLBACK (gtk_widget_hide),
                                gfs->file_selection);
    }

  gtk_file_selection_set_filename (GTK_FILE_SELECTION (gfs->file_selection),
				   filename);

  if (! GTK_WIDGET_VISIBLE (gfs->file_selection))
    gtk_widget_show (gfs->file_selection);
  else
    gdk_window_raise (gfs->file_selection->window);
}

static void
gimp_file_selection_check_filename (GimpFileSelection *gfs)
{
  static struct stat  statbuf;
  gchar              *filename;

  if (! gfs->check_valid)
    return;

  if (gfs->file_exists == NULL)
    return;

  filename = gtk_editable_get_chars (GTK_EDITABLE (gfs->entry), 0, -1);

  if ((stat (filename, &statbuf) == 0) &&
      (gfs->dir_only ? S_ISDIR (statbuf.st_mode) : S_ISREG (statbuf.st_mode)))
    {
      gtk_image_set_from_stock (GTK_IMAGE (gfs->file_exists),
				GTK_STOCK_YES, GTK_ICON_SIZE_BUTTON);
    }
  else
    {
      gtk_image_set_from_stock (GTK_IMAGE (gfs->file_exists),
				GTK_STOCK_NO, GTK_ICON_SIZE_BUTTON);
    }

  g_free (filename);
}
