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


enum
{
  FILENAME_CHANGED,
  LAST_SIGNAL
};


static void   gimp_file_selection_class_init  (GimpFileSelectionClass *klass);
static void   gimp_file_selection_init        (GimpFileSelection      *selection);

static void   gimp_file_selection_destroy                  (GtkObject *object);

static void   gimp_file_selection_entry_callback           (GtkWidget *widget,
							    gpointer   data);
static gint   gimp_file_selection_entry_focus_out_callback (GtkWidget *widget,
							    GdkEvent  *event,
							    gpointer   data);
static void   gimp_file_selection_browse_callback          (GtkWidget *widget,
							    gpointer   data);
static void   gimp_file_selection_check_filename   (GimpFileSelection *selection);


static guint gimp_file_selection_signals[LAST_SIGNAL] = { 0 };

static GtkHBoxClass *parent_class = NULL;


GType
gimp_file_selection_get_type (void)
{
  static GType selection_type = 0;

  if (! selection_type)
    {
      static const GTypeInfo selection_info =
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

      selection_type = g_type_register_static (GTK_TYPE_HBOX,
					       "GimpFileSelection",
					       &selection_info, 0);
    }

  return selection_type;
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
		  G_STRUCT_OFFSET (GimpFileSelectionClass, filename_changed),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  object_class->destroy = gimp_file_selection_destroy;

  klass->filename_changed = NULL;
}

static void
gimp_file_selection_init (GimpFileSelection *selection)
{
  selection->title          = NULL;
  selection->file_selection = NULL;
  selection->check_valid    = FALSE;

  selection->file_exists    = NULL;

  gtk_box_set_spacing (GTK_BOX (selection), 2);
  gtk_box_set_homogeneous (GTK_BOX (selection), FALSE);

  selection->browse_button = gtk_button_new_with_label (" ... ");
  gtk_box_pack_end (GTK_BOX (selection),
		    selection->browse_button, FALSE, FALSE, 0);
  g_signal_connect (selection->browse_button, "clicked",
                    G_CALLBACK (gimp_file_selection_browse_callback),
                    selection);
  gtk_widget_show (selection->browse_button);

  selection->entry = gtk_entry_new ();
  gtk_box_pack_end (GTK_BOX (selection), selection->entry, TRUE, TRUE, 0);
  g_signal_connect (selection->entry, "activate",
                    G_CALLBACK (gimp_file_selection_entry_callback),
                    selection);
  g_signal_connect (selection->entry, "focus_out_event",
                    G_CALLBACK (gimp_file_selection_entry_focus_out_callback),
                    selection);
  gtk_widget_show (selection->entry);
}

static void
gimp_file_selection_destroy (GtkObject *object)
{
  GimpFileSelection *selection;

  g_return_if_fail (GIMP_IS_FILE_SELECTION (object));

  selection = GIMP_FILE_SELECTION (object);

  if (selection->file_selection)
    {
      gtk_widget_destroy (selection->file_selection);
      selection->file_selection = NULL;
    }

  if (selection->title)
    {
      g_free (selection->title);
      selection->title = NULL;
    }

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

/**
 * gimp_file_selection_new:
 * @title: The title of the #GtkFileSelection dialog.
 * @filename: The initial filename.
 * @dir_only: %TRUE if the file selection should accept directories only.
 * @check_valid: %TRUE if the widget should check if the entered file
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
  GimpFileSelection *selection;

  selection = g_object_new (GIMP_TYPE_FILE_SELECTION, NULL);

  selection->title       = g_strdup (title);
  selection->dir_only    = dir_only;
  selection->check_valid = check_valid;

  if (check_valid)
    {
      selection->file_exists = gtk_image_new_from_stock (GTK_STOCK_NO,
							 GTK_ICON_SIZE_BUTTON);
      gtk_box_pack_start (GTK_BOX (selection),
			  selection->file_exists, FALSE, FALSE, 0);
      gtk_widget_show (selection->file_exists);
    }

  gimp_file_selection_set_filename (selection, filename);

  return GTK_WIDGET (selection);
}

/**
 * gimp_file_selection_get_filename:
 * @selection: The file selection you want to know the filename from.
 *
 * Note that you have to g_free() the returned string.
 *
 * Returns: The file or directory the user has entered.
 **/
gchar *
gimp_file_selection_get_filename (GimpFileSelection *selection)
{
  g_return_val_if_fail (GIMP_IS_FILE_SELECTION (selection), NULL);

  return gtk_editable_get_chars (GTK_EDITABLE (selection->entry), 0, -1);
}

/**
 * gimp_file_selection_set_filename:
 * @selection: The file selection you want to set the filename for.
 * @filename: The new filename.
 *
 * If you specified @check_valid as %TRUE in gimp_file_selection_new()
 * the #GimpFileSelection will immediately check the validity of the file
 * name.
 *
 */
void
gimp_file_selection_set_filename (GimpFileSelection *selection,
				  const gchar       *filename)
{
  g_return_if_fail (GIMP_IS_FILE_SELECTION (selection));

  gtk_entry_set_text (GTK_ENTRY (selection->entry), filename ? filename : "");

  /*  update everything
   */
  gimp_file_selection_entry_callback (selection->entry, (gpointer) selection);
}

static void
gimp_file_selection_entry_callback (GtkWidget *widget,
				    gpointer   data)
{
  GimpFileSelection *selection;
  gchar             *filename;
  gint               len;

  selection = GIMP_FILE_SELECTION (data);

  /*  filenames still need more sanity checking
   *  (erase double G_DIR_SEPARATORS, ...)
   */
  filename = gtk_editable_get_chars (GTK_EDITABLE (widget), 0, -1);
  filename = g_strstrip (filename);

  while (((len = strlen (filename)) > 1) &&
	 (filename[len - 1] == G_DIR_SEPARATOR))
    filename[len - 1] = '\0';

  g_signal_handlers_block_by_func (selection->entry,
                                   gimp_file_selection_entry_callback,
                                   selection);
  gtk_entry_set_text (GTK_ENTRY (selection->entry), filename);
  g_signal_handlers_unblock_by_func (selection->entry,
                                     gimp_file_selection_entry_callback,
                                     selection);

  if (selection->file_selection)
    gtk_file_selection_set_filename (GTK_FILE_SELECTION (selection->file_selection),
				     filename);
  g_free (filename);

  gimp_file_selection_check_filename (selection);

  gtk_editable_set_position (GTK_EDITABLE (selection->entry), -1);

  g_signal_emit (selection, gimp_file_selection_signals[FILENAME_CHANGED], 0);
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
  GimpFileSelection *selection;
  const gchar       *filename;

  selection = GIMP_FILE_SELECTION (data);
  filename =
    gtk_file_selection_get_filename (GTK_FILE_SELECTION (selection->file_selection));

  gtk_entry_set_text (GTK_ENTRY (selection->entry), filename);

  gtk_widget_hide (selection->file_selection);

  /*  update everything  */
  gimp_file_selection_entry_callback (selection->entry, data);
}

static void
gimp_file_selection_browse_callback (GtkWidget *widget,
				     gpointer   data)
{
  GimpFileSelection *selection;
  gchar             *filename;

  selection = GIMP_FILE_SELECTION (data);
  filename = gtk_editable_get_chars (GTK_EDITABLE (selection->entry), 0, -1);

  if (selection->file_selection == NULL)
    {
      if (selection->dir_only)
	{
          selection->file_selection =
            gtk_file_selection_new (selection->title ?
                                    selection->title : _("Select Folder"));

	  /*  hiding these widgets uses internal gtk+ knowledge, but it's
	   *  easier than creating my own directory browser -- michael
	   */
	  gtk_widget_hide
	    (GTK_FILE_SELECTION (selection->file_selection)->fileop_del_file);
	  gtk_widget_hide
	    (GTK_FILE_SELECTION (selection->file_selection)->file_list->parent);
	}
      else
        {
          selection->file_selection =
            gtk_file_selection_new (selection->title ?
                                    selection->title : _("Select File"));
        }

      gtk_window_set_position (GTK_WINDOW (selection->file_selection),
			       GTK_WIN_POS_MOUSE);
      gtk_window_set_role (GTK_WINDOW (selection->file_selection),
                           "gimp-file-entry-file-selection");

      /* slightly compress the dialog */
      gtk_container_set_border_width (GTK_CONTAINER (selection->file_selection),
				      2);
      gtk_container_set_border_width (GTK_CONTAINER (GTK_FILE_SELECTION (selection->file_selection)->button_area),
				      2);

      g_signal_connect
	(GTK_FILE_SELECTION (selection->file_selection)->ok_button,
	 "clicked",
	 G_CALLBACK (gimp_file_selection_filesel_ok_callback),
	 selection);
      g_signal_connect
	(GTK_FILE_SELECTION (selection->file_selection)->selection_entry,
	 "activate",
	 G_CALLBACK (gimp_file_selection_filesel_ok_callback),
	 selection);

      g_signal_connect_swapped (GTK_FILE_SELECTION (selection->file_selection)->cancel_button,
                                "clicked",
                                G_CALLBACK (gtk_widget_hide),
                                selection->file_selection);
      g_signal_connect_swapped (selection, "unmap",
                                G_CALLBACK (gtk_widget_hide),
                                selection->file_selection);
      g_signal_connect_swapped (selection->file_selection,
                                "delete_event",
                                G_CALLBACK (gtk_widget_hide),
                                selection->file_selection);
    }

  gtk_file_selection_set_filename (GTK_FILE_SELECTION (selection->file_selection),
				   filename);

  gtk_window_set_screen (GTK_WINDOW (selection->file_selection),
                         gtk_widget_get_screen (widget));

  gtk_window_present (GTK_WINDOW (selection->file_selection));
}

static void
gimp_file_selection_check_filename (GimpFileSelection *selection)
{
  gchar    *filename;
  gboolean  exists;

  if (! selection->check_valid)
    return;

  if (selection->file_exists == NULL)
    return;

  filename = gtk_editable_get_chars (GTK_EDITABLE (selection->entry), 0, -1);

  if (selection->dir_only)
    exists = g_file_test (filename, G_FILE_TEST_IS_DIR);
  else
    exists = g_file_test (filename, G_FILE_TEST_IS_REGULAR);

  g_free (filename);

  gtk_image_set_from_stock (GTK_IMAGE (selection->file_exists),
                            exists ? GTK_STOCK_YES : GTK_STOCK_NO,
                            GTK_ICON_SIZE_BUTTON);
}
