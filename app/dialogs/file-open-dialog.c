/* The GIMP -- an image manipulation program
 * Copyright (C) 1995, 1996, 1997 Spencer Kimball and Peter Mattis
 * Copyright (C) 1997 Josh MacDonald
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "apptypes.h"

#include "cursorutil.h"
#include "file-dialog-utils.h"
#include "file-open-dialog.h"
#include "dialog_handler.h"
#include "docindex.h"
#include "gdisplay.h"
#include "gimpui.h"
#include "gui/menus.h"

#include "gimpdrawable.h"
#include "gimage.h"
#include "gimpcontext.h"
#include "gimpdrawable.h"
#include "gimprc.h"
#include "file-open.h"
#include "file-utils.h"
#include "plug_in.h"
#include "temp_buf.h"
#include "undo.h"

#include "libgimp/gimpintl.h"


#define REVERT_DATA_KEY "revert_confirm_dialog"


static void     file_open_dialog_create      (void);

static void     file_revert_confirm_callback (GtkWidget     *widget,
					      gboolean       revert,
					      gpointer       data);

static void     file_open_genbutton_callback (GtkWidget     *widget,
					      gpointer       data);

static void     file_open_clistrow_callback  (GtkWidget     *widget,
					      gint           row);

static void     file_open_ok_callback        (GtkWidget     *widget,
					      gpointer       data);

static void     file_open_type_callback      (GtkWidget     *widget,
					      gpointer       data);

static GSList * clist_to_slist               (GtkCList      *file_list);



static GtkWidget  *fileload                    = NULL;
static GtkWidget  *open_options                = NULL;

static GtkPreview *open_options_preview        = NULL;
static GtkWidget  *open_options_fixed          = NULL;
static GtkWidget  *open_options_label          = NULL;
static GtkWidget  *open_options_frame          = NULL;
static GtkWidget  *open_options_genbuttonlabel = NULL;

/* Some state for the thumbnailer */
static gchar *preview_fullname = NULL;

static PlugInProcDef *load_file_proc = NULL;

extern GSList *display_list; /* from gdisplay.c */


/*  public functions  */

void
file_open_menu_init (void)
{
  GimpItemFactoryEntry  entry;
  PlugInProcDef        *file_proc;
  GSList               *list;

  load_procs = g_slist_reverse (load_procs);

  for (list = load_procs; list; list = g_slist_next (list))
    {
      gchar *help_page;

      file_proc = (PlugInProcDef *) list->data;

      help_page = g_strconcat ("filters/",
			       g_basename (file_proc->prog),
			       ".html",
			       NULL);
      g_strdown (help_page);

      entry.entry.path            = file_proc->menu_path;
      entry.entry.accelerator     = NULL;
      entry.entry.callback        = file_open_type_callback;
      entry.entry.callback_action = 0;
      entry.entry.item_type       = NULL;
      entry.help_page             = help_page;
      entry.description           = NULL;

      menus_create_item_from_full_path (&entry, NULL, file_proc);
    }
}

void
file_open_callback (GtkWidget *widget,
		    gpointer   data)
{
  if (!fileload)
    file_open_dialog_create ();

  gtk_widget_set_sensitive (GTK_WIDGET (fileload), TRUE);
  if (GTK_WIDGET_VISIBLE (fileload))
    return;

  gtk_file_selection_set_filename (GTK_FILE_SELECTION (fileload),
				   "." G_DIR_SEPARATOR_S);
  gtk_window_set_title (GTK_WINDOW (fileload), _("Load Image"));

  file_dialog_show (fileload);
}

void
file_revert_callback (GtkWidget *widget,
		      gpointer   data)
{
  GDisplay    *gdisplay;
  GimpImage   *gimage;
  GtkWidget   *query_box;
  const gchar *filename;

  gdisplay = gdisplay_active ();
  if (!gdisplay || !gdisplay->gimage)
    return;

  gimage = gdisplay->gimage;

  filename = gimp_object_get_name (GIMP_OBJECT (gimage));

  query_box = gtk_object_get_data (GTK_OBJECT (gimage), REVERT_DATA_KEY);

  if (! filename)
    {
      g_message (_("Revert failed.\n"
		   "No filename associated with this image."));
    }
  else if (query_box)
    {
      gdk_window_raise (query_box->window);
    }
  else
    {
      gchar *text;

      text = g_strdup_printf (_("Reverting %s to\n"
				"%s\n\n"
				"(You will lose all your changes\n"
				"including all undo information)"),
			      g_basename (filename),
			      filename);

      query_box = gimp_query_boolean_box (_("Revert Image?"),
					  gimp_standard_help_func,
					  "file/revert.html",
					  FALSE,
					  text,
					  _("Yes"), _("No"),
					  GTK_OBJECT (gimage), "destroy",
					  file_revert_confirm_callback,
					  gimage);

      g_free (text);

      gtk_object_set_data (GTK_OBJECT (gimage), REVERT_DATA_KEY, query_box);

      gtk_widget_show (query_box);
    }
}

void
file_open_by_extension_callback (GtkWidget *widget,
				 gpointer   data)
{
  load_file_proc = NULL;
}

gint
file_open_with_display (gchar *filename,
			gchar *raw_filename)
{
  GimpImage *gimage;
  GDisplay  *gdisplay;
  gchar     *absolute;
  gint       status;

  if ((gimage = file_open_image (filename,
				 raw_filename,
				 _("Open"),
				 NULL,
				 RUN_INTERACTIVE,
				 &status)) != NULL)
    {
      /* enable & clear all undo steps */
      gimp_image_undo_enable (gimage);

      /* set the image to clean  */
      gimp_image_clean_all (gimage);

      /* display the image */
      gdisplay = gdisplay_new (gimage, 0x0101);

      /* always activate the first display */
      if (g_slist_length (display_list) == 1)
	gimp_context_set_display (gimp_context_get_user (), gdisplay);

      absolute = file_open_absolute_filename (filename);

      document_index_add (absolute);
      menus_last_opened_add (absolute);

      g_free (absolute);
    }

  return status;
}


/*  private functions  */

static void
file_open_dialog_create (void)
{
  fileload = gtk_file_selection_new (_("Load Image"));
  gtk_window_set_position (GTK_WINDOW (fileload), GTK_WIN_POS_MOUSE);
  gtk_window_set_wmclass (GTK_WINDOW (fileload), "load_image", "Gimp");

  gtk_container_set_border_width (GTK_CONTAINER (fileload), 2);
  gtk_container_set_border_width 
    (GTK_CONTAINER (GTK_FILE_SELECTION (fileload)->button_area), 2);

  dialog_register_fileload (fileload);

  gtk_signal_connect_object
    (GTK_OBJECT (GTK_FILE_SELECTION (fileload)->cancel_button), "clicked",
     GTK_SIGNAL_FUNC (file_dialog_hide),
     GTK_OBJECT (fileload));
  gtk_signal_connect (GTK_OBJECT (fileload), "delete_event",
		      GTK_SIGNAL_FUNC (file_dialog_hide),
		      NULL);
  gtk_signal_connect
    (GTK_OBJECT (GTK_FILE_SELECTION (fileload)->ok_button), "clicked",
     GTK_SIGNAL_FUNC (file_open_ok_callback),
     fileload);
  gtk_quit_add_destroy (1, GTK_OBJECT (fileload));

  gtk_clist_set_selection_mode
    (GTK_CLIST (GTK_FILE_SELECTION (fileload)->file_list),
     GTK_SELECTION_EXTENDED);

  /* Catch file-clist clicks so we can update the preview thumbnail */
  gtk_signal_connect
    (GTK_OBJECT (GTK_FILE_SELECTION (fileload)->file_list), "select_row",
     GTK_SIGNAL_FUNC (file_open_clistrow_callback),
     fileload);

  /*  Connect the "F1" help key  */
  gimp_help_connect_help_accel (fileload,
				gimp_standard_help_func,
				"open/dialogs/file_open.html");

  {
    GtkWidget *frame;
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *option_menu;
    GtkWidget *load_menu;
    GtkWidget *open_options_genbutton;

    open_options = gtk_hbox_new (TRUE, 1);

    /* format-chooser frame */
    frame = gtk_frame_new (_("Determine File Type"));
    gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
    gtk_box_pack_start (GTK_BOX (open_options), frame, TRUE, TRUE, 4);

    vbox = gtk_vbox_new (FALSE, 2);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
    gtk_container_add (GTK_CONTAINER (frame), vbox);

    hbox = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
    gtk_widget_show (hbox);

    option_menu = gtk_option_menu_new ();
    gtk_box_pack_start (GTK_BOX (hbox), option_menu, FALSE, FALSE, 0);
    gtk_widget_show (option_menu);

    load_menu = menus_get_load_factory ()->widget;
    gtk_option_menu_set_menu (GTK_OPTION_MENU (option_menu), load_menu);

    gtk_widget_show (vbox);
    gtk_widget_show (frame);

    /* Preview frame */
    open_options_frame = frame = gtk_frame_new ("");
    gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
    gtk_box_pack_end (GTK_BOX (open_options), frame, FALSE, TRUE, 4);

    vbox = gtk_vbox_new (FALSE, 2);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
    gtk_container_add (GTK_CONTAINER (frame), vbox);

    hbox = gtk_hbox_new (TRUE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
    gtk_widget_show (hbox);

    open_options_genbutton = gtk_button_new ();
    gtk_signal_connect (GTK_OBJECT (open_options_genbutton), "clicked",
			GTK_SIGNAL_FUNC (file_open_genbutton_callback),
			fileload);
    gtk_box_pack_start (GTK_BOX (hbox), open_options_genbutton,
			TRUE, FALSE, 0);
    gtk_widget_show (open_options_genbutton);	

    open_options_fixed = gtk_fixed_new ();
    gtk_widget_set_usize (open_options_fixed, 80, 60);
    gtk_container_add (GTK_CONTAINER (GTK_BIN (open_options_genbutton)),
		       open_options_fixed);
    gtk_widget_show (open_options_fixed);

    {
      GtkWidget* abox;
      GtkWidget* sbox;
      GtkWidget* align;

      sbox = gtk_vbox_new (TRUE, 0);
      gtk_container_add (GTK_CONTAINER (open_options_fixed), sbox);
      gtk_widget_show (sbox);

      align = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
      gtk_widget_set_usize (align, 80, 60);
      gtk_box_pack_start (GTK_BOX (sbox), align, FALSE, TRUE, 0);
      gtk_widget_show (align);

      abox = gtk_hbox_new (FALSE, 0);
      gtk_container_add (GTK_CONTAINER (align), abox);
      gtk_widget_show (abox);

      open_options_preview =
	GTK_PREVIEW (gtk_preview_new (GTK_PREVIEW_COLOR));
      gtk_box_pack_start (GTK_BOX (abox), GTK_WIDGET (open_options_preview),
			  FALSE, TRUE, 0);
      gtk_widget_show (GTK_WIDGET (open_options_preview));

      open_options_genbuttonlabel = gtk_label_new (_("Generate\nPreview"));
      gtk_box_pack_start (GTK_BOX (abox), open_options_genbuttonlabel,
			  FALSE, TRUE, 0);
      gtk_widget_show (open_options_genbuttonlabel);
    }

    open_options_label = gtk_label_new ("");
    gtk_box_pack_start (GTK_BOX (vbox), open_options_label, FALSE, FALSE, 0);
    gtk_widget_show (open_options_label); 

    gtk_widget_show (vbox);
    gtk_widget_show (frame);

    /* pack the containing open_options hbox into the open-dialog */
    gtk_box_pack_end (GTK_BOX (GTK_FILE_SELECTION (fileload)->main_vbox),
		      open_options, FALSE, FALSE, 0);
  }

  gtk_frame_set_label (GTK_FRAME (open_options_frame), _("Preview"));
  gtk_label_set_text (GTK_LABEL (open_options_label), _("No Selection."));

  gtk_widget_show (GTK_WIDGET (open_options_genbuttonlabel));
  gtk_widget_hide (GTK_WIDGET (open_options_preview));
  gtk_widget_set_sensitive (GTK_WIDGET (open_options_frame), FALSE);

  gtk_widget_show (open_options);
}

static void
file_open_type_callback (GtkWidget *widget,
			 gpointer   data)
{
  PlugInProcDef *proc = (PlugInProcDef *) data;

  file_dialog_update_name (proc, fileload);

  load_file_proc = proc;
}

static guchar *
make_RGBbuf_from_tempbuf (TempBuf *tempbuf,
			  gint    *width_rtn,
			  gint    *height_rtn)
{
  gint    i, j, w, h;
  guchar *tbd;
  guchar *ptr;
  guchar *rtn = NULL;
  guchar  alpha, r, g, b;

  w = (*width_rtn) = tempbuf->width;
  h = (*height_rtn) = tempbuf->height;
  tbd = temp_buf_data (tempbuf);

  switch (tempbuf->bytes)
    {
    case 4:
      rtn = ptr = g_malloc (3 * w * h);
      for (i=0; i<h; i++)
	{
	  for (j=0; j<w; j++)
	    {
	      r = *(tbd++);
	      g = *(tbd++);
	      b = *(tbd++);
	      alpha = *(tbd++);

	      if (alpha & 128)
		{
		  *(ptr++) = r;
		  *(ptr++) = g;
		  *(ptr++) = b;
		}
	      else
		{
		  r = (( (i^j) & 4 ) << 5) | 64;
		  *(ptr++) = r;
		  *(ptr++) = r;
		  *(ptr++) = r;
		}
	    }
	}
      break;

    case 2:
      rtn = ptr = g_malloc (3 * w * h);
      for (i=0; i<h; i++)
	{
	  for (j=0; j<w; j++)
	    {
	      r = *(tbd++);
	      alpha = *(tbd++);

	      if (!(alpha & 128))
		r = (( (i^j) & 4 ) << 5) | 64;

	      *(ptr++) = r;
	      *(ptr++) = r;
	      *(ptr++) = r;
	    }
	}
      break;

    default:
      g_warning("UNKNOWN TempBuf width in make_RGBbuf_from_tempbuf()");
    }

  return (rtn);
}

/* don't call with preview_fullname as parameter!  will be clobbered! */
static void
set_preview (const gchar *fullfname,
	     guchar      *RGB_source,
	     gint         RGB_w,
	     gint         RGB_h)
{
  guchar      *thumb_rgb;
  guchar      *raw_thumb;
  gint         tnw,tnh, i;
  gchar       *pname;
  gchar       *fname;
  gchar       *tname;
  gchar       *imginfo = NULL;
  struct stat  file_stat;
  struct stat  thumb_stat;
  gboolean     thumb_may_be_outdated = FALSE;
  gboolean     show_generate_label   = TRUE;

  pname = g_dirname (fullfname);
  fname = g_basename (fullfname); /* Don't free this! */
  tname = g_strconcat (pname, G_DIR_SEPARATOR_S,
		       ".xvpics", G_DIR_SEPARATOR_S,
		       fname, NULL);

  g_free (pname);

  /*  If the file is newer than its thumbnail, the thumbnail may
   *  be out of date.
   */
  if ((stat (tname,     &thumb_stat) == 0) &&
      (stat (fullfname, &file_stat ) == 0))
    {
      if ((thumb_stat.st_mtime) < (file_stat.st_mtime))
	{
	  thumb_may_be_outdated = TRUE;
	}
    }

  raw_thumb = readXVThumb (tname, &tnw, &tnh, &imginfo);

  g_free (tname);

  gtk_frame_set_label (GTK_FRAME (open_options_frame), fname);

  g_free (preview_fullname);
  preview_fullname = g_strdup (fullfname);

  if (RGB_source)
    {
      gtk_preview_size (open_options_preview, RGB_w, RGB_h);
      
      for (i = 0; i < RGB_h; i++)
	{
	  gtk_preview_draw_row (open_options_preview, &RGB_source[3*i*RGB_w],
				0, i,
				RGB_w);
	}
    }
  else
    {
      if (raw_thumb)
	{
	  thumb_rgb = g_malloc (3 * tnw * tnh);

	  for (i = 0; i < tnw * tnh; i++)
	    {
	      thumb_rgb[i*3  ] = ((raw_thumb[i]>>5)*255)/7;
	      thumb_rgb[i*3+1] = (((raw_thumb[i]>>2)&7)*255)/7;
	      thumb_rgb[i*3+2] = (((raw_thumb[i])&3)*255)/3;
	    }

	  gtk_preview_size (open_options_preview, tnw, tnh);

	  for (i = 0; i < tnh; i++)
	    {
	      gtk_preview_draw_row (open_options_preview, &thumb_rgb[3*i*tnw],
				    0, i,
				    tnw);
	    }

	  g_free (thumb_rgb);
	}
    }

  if (raw_thumb || RGB_source)  /* We can show *some* kind of preview. */
    {
      if (raw_thumb) /* Managed to commit thumbnail file to disk */
	{
	  gtk_label_set_text (GTK_LABEL (open_options_label),
			      thumb_may_be_outdated ?
			      _("(This thumbnail may be out of date)") :
			      (imginfo ? imginfo : _("(No Information)")));
	  if (imginfo)
	    g_free (imginfo);
	}
      else
	{
	  switch (thumbnail_mode)
	    {
	    case 0:
	      gtk_label_set_text (GTK_LABEL(open_options_label),
				  _("(Thumbnail saving is disabled)"));
	      break;
	    case 1:
	      gtk_label_set_text (GTK_LABEL(open_options_label),
				  _("(Could not write thumbnail file)"));
	      break;
	    default:
	      gtk_label_set_text (GTK_LABEL(open_options_label),
				  _("(Thumbnail file not written)"));
	    }
	}

      gtk_widget_show (GTK_WIDGET (open_options_preview));
      gtk_widget_queue_draw (GTK_WIDGET(open_options_preview));

      show_generate_label = FALSE;
      
      g_free (raw_thumb);
    }
  else
    {
      if (imginfo)
	g_free (imginfo);

      gtk_widget_hide (GTK_WIDGET (open_options_preview));
      gtk_label_set_text (GTK_LABEL (open_options_label),
			  _("No preview available"));
    }

  if (show_generate_label)
    {
      gtk_widget_hide (GTK_WIDGET (open_options_preview));
      gtk_widget_show (GTK_WIDGET (open_options_genbuttonlabel));
    }
  else
    {
      gtk_widget_hide (GTK_WIDGET (open_options_genbuttonlabel));
      gtk_widget_show (GTK_WIDGET (open_options_preview));
    }
}

static void
file_open_clistrow_callback (GtkWidget *widget,
			     gint       row)
{
  gchar *fullfname = NULL;

  fullfname = gtk_file_selection_get_filename (GTK_FILE_SELECTION (fileload));

  gtk_widget_set_sensitive (GTK_WIDGET (open_options_frame), TRUE);
  set_preview (fullfname, NULL, 0, 0);
}

static void
file_open_genbutton_callback (GtkWidget *widget,
			      gpointer   data)
{
  GimpImage *gimage_to_be_thumbed;
  guchar    *RGBbuf;
  TempBuf   *tempbuf;
  gint       RGBbuf_w;
  gint       RGBbuf_h;

  /* added for multi-file preview generation... */
  GtkFileSelection *fs;
  gchar            *full_filename = NULL;
  gchar            *filedirname;
  struct stat       buf;
  gint              err;

  fs = GTK_FILE_SELECTION (data);

  if (! preview_fullname)
    {
      g_warning ("Tried to generate thumbnail for NULL filename.");
      return;
    }

  gimp_add_busy_cursors ();
  gtk_widget_set_sensitive (GTK_WIDGET (fileload), FALSE);

  /* new mult-file preview make: */  
  {
    GSList *list, *toplist;

    /* Have to read the clist before touching anything else */

    list= clist_to_slist(GTK_CLIST(fs->file_list));
    toplist = list;
    
    /* Find a real base directory for the multiple selection */

    gtk_file_selection_set_filename (fs, "");
    filedirname= gtk_file_selection_get_filename (fs);
    if (filedirname[strlen (filedirname) - 1] == G_DIR_SEPARATOR)
      filedirname[strlen (filedirname) - 1] = '\0';

    while(list)
      {
        full_filename = g_strconcat (filedirname, G_DIR_SEPARATOR_S,
				     (gchar *) list->data, NULL);

	err = stat (full_filename, &buf);

	if (! (err == 0 && (buf.st_mode & S_IFDIR)))
	  { /* Is not directory. */
	    gint dummy;

	    gimage_to_be_thumbed = file_open_image (full_filename,
						    list->data,
						    NULL,
						    load_file_proc,
						    RUN_NONINTERACTIVE,
						    &dummy);

	    if (gimage_to_be_thumbed)
	      {			
		tempbuf = make_thumb_tempbuf (gimage_to_be_thumbed);
		RGBbuf  = make_RGBbuf_from_tempbuf (tempbuf,
						    &RGBbuf_w,
						    &RGBbuf_h);
		if (thumbnail_mode)
		  {
		    file_save_thumbnail (gimage_to_be_thumbed,
					 full_filename, tempbuf);
		  }
		set_preview (full_filename, RGBbuf, RGBbuf_w, RGBbuf_h);

		gtk_object_unref (GTK_OBJECT (gimage_to_be_thumbed));

		if (RGBbuf)
		  g_free (RGBbuf);
	      }
	    else
	      {
		gtk_label_set_text (GTK_LABEL (open_options_label),
				    _("(could not make preview)"));
	      }
	  }

        g_free(full_filename);
        list= g_slist_next(list);
      }
    
    for (list = toplist; list; list = g_slist_next (list))
      {
	if (!(g_slist_next (list)))
	  {
	    full_filename = g_strconcat (filedirname, G_DIR_SEPARATOR_S,
					 (gchar *) list->data, NULL);
            gtk_file_selection_set_filename (fs, full_filename);
	    g_free (full_filename);
	  }

        g_free (list->data);
      }

    g_slist_free (toplist);
    toplist = NULL;
  }

  gtk_widget_set_sensitive (GTK_WIDGET (fileload), TRUE);
  gimp_remove_busy_cursors (NULL);
}

static void
file_open_ok_callback (GtkWidget *widget,
		       gpointer   data)
{
  GtkFileSelection *fs;
  gchar            *full_filename;
  gchar            *raw_filename;
  gchar            *filedirname;
  struct stat       buf;
  gint              err;
  gint              status;

  fs = GTK_FILE_SELECTION (data);
  full_filename = gtk_file_selection_get_filename (fs);
  raw_filename = gtk_entry_get_text (GTK_ENTRY(fs->selection_entry));

  g_assert (full_filename && raw_filename);

  if (strlen (raw_filename) == 0)
    return;

  err = stat (full_filename, &buf);

  if (err == 0 && (buf.st_mode & S_IFDIR))
    {
      if (full_filename[strlen (full_filename) - 1] != G_DIR_SEPARATOR)
	{
	  gchar *s = g_strconcat (full_filename, G_DIR_SEPARATOR_S, NULL);
	  gtk_file_selection_set_filename (fs, s);
	  g_free (s);
	}
      else
	gtk_file_selection_set_filename (fs, full_filename);

      return;
    }

  gtk_widget_set_sensitive (GTK_WIDGET (fs), FALSE);

  if (err) /* e.g. http://server/filename.jpg */
    full_filename = raw_filename;

  status = file_open_with_display (full_filename, raw_filename);

  if (status == PDB_SUCCESS)
    {
      file_dialog_hide (data);
    }
  else if (status != PDB_CANCEL)
    {
      g_message (_("Open failed.\n%s"), full_filename);
    }


  /*
   * Now deal with multiple selections from the filesel clist
   */

  {
    GSList *list;

    /* Have to read the clist before touching anything else */

    list = clist_to_slist (GTK_CLIST (fs->file_list));

    /* Find a real base directory for the multiple selection */

    raw_filename = g_strdup(raw_filename);
    gtk_file_selection_set_filename (fs, "");
    filedirname = gtk_file_selection_get_filename (fs);

    while (list)
      {
        full_filename = g_strconcat (filedirname, G_DIR_SEPARATOR_S,
				     (gchar *) list->data, NULL);

        if (strcmp (list->data, raw_filename))
          { /* don't load current selection twice */

            err = stat (full_filename, &buf);

            if (! (err == 0 && (buf.st_mode & S_IFDIR)))
              { /* Is not directory. */

                status = file_open_with_display (full_filename,
						 (gchar *) list->data);

                if (status == PDB_SUCCESS)
                  {
                    file_dialog_hide (data);
                  }
                else if (status != PDB_CANCEL)
                  {
                    g_message (_("Open failed.\n%s"), full_filename);
                  }
              }
          }
     
        g_free (full_filename);
        g_free (list->data);
        list = g_slist_next (list);
      }

    g_slist_free (list);
    list = NULL;
  }
    
  gtk_file_selection_set_filename (fs, raw_filename);
  g_free (raw_filename);
  gtk_widget_set_sensitive (GTK_WIDGET (fs), TRUE);

}

static GSList *
clist_to_slist (GtkCList *file_list)
{
  GSList *list = NULL;
  GList  *row;
  gint    rownum;
  gchar  *temp;
  
  for (row = file_list->row_list, rownum = 0;
       row; 
       row = g_list_next (row), rownum++)
    {
      if (GTK_CLIST_ROW (row)->state == GTK_STATE_SELECTED)
        {
          if (gtk_clist_get_cell_type (file_list, rownum, 0) == GTK_CELL_TEXT)
            {
              gtk_clist_get_text (file_list, rownum, 0, &temp);
              list = g_slist_prepend (list, g_strdup (temp));
            }
        }
    }

  return list;
}

static void
file_revert_confirm_callback (GtkWidget *widget,
			      gboolean   revert,
			      gpointer   data)
{
  GimpImage *old_gimage;

  old_gimage = (GimpImage *) data;

  gtk_object_set_data (GTK_OBJECT (old_gimage), REVERT_DATA_KEY, NULL);

  if (revert)
    {
      GimpImage   *new_gimage;
      const gchar *filename;
      gint         status;

      filename = gimp_object_get_name (GIMP_OBJECT (old_gimage));

      new_gimage = file_open_image (filename, filename,
				    _("Revert"),
				    load_file_proc,
				    RUN_INTERACTIVE,
				    &status);

      if (new_gimage != NULL)
	{
	  undo_free (new_gimage);
	  gdisplays_reconnect (old_gimage, new_gimage);
	  gdisplays_resize_cursor_label (new_gimage);
	  gdisplays_update_full (new_gimage);
	  gdisplays_shrink_wrap (new_gimage);
	  gimp_image_clean_all (new_gimage);
	}
      else if (status != PDB_CANCEL)
	{
	  g_message (_("Revert failed.\n%s"), filename);
	}
    }
}
