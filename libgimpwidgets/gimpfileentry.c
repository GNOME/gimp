/* LIBGIMP - The GIMP Library 
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball                
 *
 * gimpfileselection.c
 * Copyright (C) 1999 Michael Natterer <mitschel@cs.tu-berlin.de>
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

#include <glib.h>		/* Needed here by Win32 gcc compilation */

#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <string.h>

#include "gimpfileselection.h"

#include "libgimp/gimpintl.h"

#include "pixmaps/yes.xpm"
#include "pixmaps/no.xpm"

#ifdef G_OS_WIN32
# ifndef S_ISDIR
#  define S_ISDIR(m) ((m) & _S_IFDIR)
# endif
# ifndef S_ISREG
#  define S_ISREG(m) ((m) & _S_IFREG)
# endif
#endif

/*  callbacks  */
static void gimp_file_selection_realize         (GtkWidget *);
static void gimp_file_selection_entry_callback  (GtkWidget *, gpointer);
static int  gimp_file_selection_entry_focus_out_callback (GtkWidget *,
							  GdkEvent *, gpointer);
static void gimp_file_selection_browse_callback (GtkWidget *, gpointer);

/*  private functions  */
static void gimp_file_selection_check_filename  (GimpFileSelection *gfs);

enum
{
  FILENAME_CHANGED,
  LAST_SIGNAL
};

static guint gimp_file_selection_signals[LAST_SIGNAL] = { 0 };

static GtkHBoxClass *parent_class = NULL;

static void
gimp_file_selection_destroy (GtkObject *object)
{
  GimpFileSelection *gfs;

  g_return_if_fail (object != NULL);
  g_return_if_fail (GIMP_IS_FILE_SELECTION (object));

  gfs = GIMP_FILE_SELECTION (object);

  if (gfs->file_selection)
    gtk_widget_destroy (gfs->file_selection);

  if (gfs->title)
    g_free (gfs->title);

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
gimp_file_selection_class_init (GimpFileSelectionClass *class)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;

  object_class = (GtkObjectClass *) class;
  widget_class = (GtkWidgetClass *) class;

  parent_class = gtk_type_class (gtk_hbox_get_type ());

  gimp_file_selection_signals[FILENAME_CHANGED] = 
    gtk_signal_new ("filename_changed",
		    GTK_RUN_FIRST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (GimpFileSelectionClass,
				       filename_changed),
		    gtk_signal_default_marshaller, GTK_TYPE_NONE, 0);

  gtk_object_class_add_signals (object_class, gimp_file_selection_signals, 
				LAST_SIGNAL);

  class->filename_changed = NULL;

  object_class->destroy = gimp_file_selection_destroy;

  widget_class->realize = gimp_file_selection_realize;
}

static void
gimp_file_selection_init (GimpFileSelection *gfs)
{
  gfs->title = NULL;
  gfs->file_selection = NULL;
  gfs->file_exists = FALSE;
  gfs->check_valid = FALSE;

  gtk_box_set_spacing (GTK_BOX (gfs), 2);
  gtk_box_set_homogeneous (GTK_BOX (gfs), FALSE);

  gfs->browse_button = gtk_button_new_with_label (" ... ");
  gtk_box_pack_end (GTK_BOX (gfs), gfs->browse_button, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT(gfs->browse_button), "clicked",
		      GTK_SIGNAL_FUNC (gimp_file_selection_browse_callback),
		      gfs);
  gtk_widget_show (gfs->browse_button);

  gfs->entry = gtk_entry_new ();
  gtk_box_pack_end (GTK_BOX (gfs), gfs->entry, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (gfs->entry), "activate",
		      GTK_SIGNAL_FUNC (gimp_file_selection_entry_callback),
		      gfs);
  gtk_signal_connect (GTK_OBJECT (gfs->entry), "focus_out_event",
		      GTK_SIGNAL_FUNC (gimp_file_selection_entry_focus_out_callback),
		      gfs);
  gtk_widget_show (gfs->entry);
}

GtkType
gimp_file_selection_get_type (void)
{
  static GtkType gfs_type = 0;

  if (!gfs_type)
    {
      GtkTypeInfo gfs_info =
      {
	"GimpFileSelection",
	sizeof (GimpFileSelection),
	sizeof (GimpFileSelectionClass),
	(GtkClassInitFunc) gimp_file_selection_class_init,
	(GtkObjectInitFunc) gimp_file_selection_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL
      };

      gfs_type = gtk_type_unique (gtk_hbox_get_type (), &gfs_info);
    }
  
  return gfs_type;
}

GtkWidget *
gimp_file_selection_new (gchar    *title,
			 gchar    *filename,
			 gboolean  dir_only,
			 gboolean  check_valid)
{
  GimpFileSelection *gfs;

  gfs = gtk_type_new (gimp_file_selection_get_type ());

  gfs->title = g_strdup (title);
  gfs->dir_only = dir_only;
  gfs->check_valid = check_valid;
  gimp_file_selection_set_filename (gfs, filename);

  return GTK_WIDGET (gfs);
}

static void
gimp_file_selection_realize (GtkWidget *widget)
{
  GimpFileSelection *gfs;
  GtkStyle          *style;

  gfs = GIMP_FILE_SELECTION (widget);
  if (! gfs->check_valid)
    return;

  if (GTK_WIDGET_CLASS (parent_class)->realize)
    (* GTK_WIDGET_CLASS (parent_class)->realize) (widget);

  style = gtk_widget_get_style (widget);

  gfs->yes_pixmap = gdk_pixmap_create_from_xpm_d (widget->window,
						  &gfs->yes_mask,
						  &style->bg[GTK_STATE_NORMAL],
						  yes_xpm);
  gfs->no_pixmap = gdk_pixmap_create_from_xpm_d (widget->window,
						 &gfs->no_mask,
						 &style->bg[GTK_STATE_NORMAL],
						 no_xpm);

  gfs->file_exists = gtk_pixmap_new (gfs->no_pixmap, gfs->no_mask);
  gtk_box_pack_start (GTK_BOX (gfs), gfs->file_exists, FALSE, FALSE, 0);

  gimp_file_selection_check_filename (gfs);
  gtk_widget_show (gfs->file_exists);
}

gchar *
gimp_file_selection_get_filename (GimpFileSelection *gfs)
{
  g_return_val_if_fail (gfs != NULL, g_strdup (""));
  g_return_val_if_fail (GIMP_IS_FILE_SELECTION (gfs), g_strdup (""));

  return gtk_editable_get_chars (GTK_EDITABLE (gfs->entry), 0, -1);
}

void
gimp_file_selection_set_filename (GimpFileSelection *gfs,
				  gchar             *filename)
{
  g_return_if_fail (gfs != NULL);
  g_return_if_fail (GIMP_IS_FILE_SELECTION (gfs));

  gtk_entry_set_text (GTK_ENTRY (gfs->entry), filename);

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

  gtk_signal_handler_block_by_data (GTK_OBJECT (gfs->entry), gfs);
  gtk_entry_set_text (GTK_ENTRY (gfs->entry), filename);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (gfs->entry), gfs);

  if (gfs->file_selection)
    gtk_file_selection_set_filename (GTK_FILE_SELECTION (gfs->file_selection),
				     filename);
  g_free (filename);

  gimp_file_selection_check_filename (gfs);

  gtk_entry_set_position (GTK_ENTRY (gfs->entry), -1);

  gtk_signal_emit (GTK_OBJECT (gfs),
		   gimp_file_selection_signals[FILENAME_CHANGED]);
}

static int
gimp_file_selection_entry_focus_out_callback (GtkWidget *widget,
					      GdkEvent  *event,
					      gpointer   data)
{
  gimp_file_selection_entry_callback (widget, data);

  return TRUE;
}

/*  local callbacks of gimp_file_selection_browse_callback()  */
static void
gimp_file_selection_filesel_ok_callback (GtkWidget *widget,
					 gpointer   data)
{
  GimpFileSelection *gfs;
  gchar             *filename;

  gfs = GIMP_FILE_SELECTION (data);
  filename =
    gtk_file_selection_get_filename (GTK_FILE_SELECTION (gfs->file_selection));

  gtk_entry_set_text (GTK_ENTRY (gfs->entry), filename);

  /*  update everything  */
  gimp_file_selection_entry_callback (gfs->entry, data);
}

static void
gimp_file_selection_filesel_cancel_callback (GtkWidget *widget,
					     gpointer   data)
{
  gtk_widget_hide (GIMP_FILE_SELECTION (data)->file_selection);
}

static int
gimp_file_selection_filesel_delete_callback (GtkWidget *widget,
					     GdkEvent  *event,
					     gpointer   data)
{
  gimp_file_selection_filesel_cancel_callback (NULL, data);

  return TRUE;
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

      gtk_label_set_text (GTK_LABEL (GTK_BIN (GTK_FILE_SELECTION (gfs->file_selection)->ok_button)->child), _("Select"));
      gtk_label_set_text (GTK_LABEL (GTK_BIN (GTK_FILE_SELECTION (gfs->file_selection)->cancel_button)->child), _("Close"));

      gtk_signal_connect
	(GTK_OBJECT (GTK_FILE_SELECTION (gfs->file_selection)->ok_button),
	 "clicked",
	 GTK_SIGNAL_FUNC (gimp_file_selection_filesel_ok_callback),
	 gfs);
      gtk_signal_connect
	(GTK_OBJECT (GTK_FILE_SELECTION (gfs->file_selection)->selection_entry),
	 "activate",
	 GTK_SIGNAL_FUNC (gimp_file_selection_filesel_ok_callback),
	 gfs);
      gtk_signal_connect
	(GTK_OBJECT (GTK_FILE_SELECTION (gfs->file_selection)->cancel_button),
	 "clicked",
	 GTK_SIGNAL_FUNC (gimp_file_selection_filesel_cancel_callback),
	 gfs);
      gtk_signal_connect
	(GTK_OBJECT (gfs), "unmap",
	 GTK_SIGNAL_FUNC (gimp_file_selection_filesel_cancel_callback),
	 gfs);
      gtk_signal_connect
	(GTK_OBJECT (gfs->file_selection), "delete_event",
	 GTK_SIGNAL_FUNC (gimp_file_selection_filesel_delete_callback),
	 gfs);
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
  static struct stat statbuf;
  gchar* filename;

  if (! gfs->check_valid)
    return;
  if (gfs->file_exists == NULL)
    return;

  filename = gtk_editable_get_chars (GTK_EDITABLE (gfs->entry), 0, -1);
  if ((stat (filename, &statbuf) == 0) &&
      (gfs->dir_only ? S_ISDIR(statbuf.st_mode) : S_ISREG(statbuf.st_mode)))
    {
      gtk_pixmap_set (GTK_PIXMAP (gfs->file_exists),
		      gfs->yes_pixmap, gfs->yes_mask);
    }
  else
    {
      gtk_pixmap_set (GTK_PIXMAP (gfs->file_exists),
		      gfs->no_pixmap, gfs->no_mask);
    }
  g_free (filename);
}
