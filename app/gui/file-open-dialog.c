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

#include "core/core-types.h"

#include "base/temp-buf.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpcoreconfig.h"
#include "core/gimpdocuments.h"
#include "core/gimpimage.h"

#include "plug-in/plug-in.h"

#include "file/file-open.h"
#include "file/file-utils.h"

#include "widgets/gimpitemfactory.h"

#include "file-dialog-utils.h"
#include "file-open-dialog.h"

#include "undo.h"

#include "libgimp/gimpintl.h"


/*  local function prototypes  */

static void     file_open_dialog_create      (Gimp      *gimp);
static void     file_open_genbutton_callback (GtkWidget *widget,
                                              gpointer   data);
static void     file_open_clistrow_callback  (GtkWidget *widget,
                                              gint       row,
                                              gint       column,
                                              GdkEvent  *event,
                                              gpointer   data);
static void     file_open_ok_callback        (GtkWidget *widget,
                                              gpointer   data);
static void     file_open_type_callback      (GtkWidget *widget,
                                              gpointer   data);
static GSList * clist_to_slist               (GtkCList  *file_list);



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


/*  public functions  */

void
file_open_dialog_menu_init (Gimp *gimp)
{
  GimpItemFactoryEntry  entry;
  PlugInProcDef        *file_proc;
  GSList               *list;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  gimp->load_procs = g_slist_reverse (gimp->load_procs);

  for (list = gimp->load_procs; list; list = g_slist_next (list))
    {
      gchar *basename;
      gchar *lowercase_basename;
      gchar *help_page;

      file_proc = (PlugInProcDef *) list->data;

      basename = g_path_get_basename (file_proc->prog);

      lowercase_basename = g_ascii_strdown (basename, -1);

      g_free (basename);

      help_page = g_strconcat ("filters/",
			       lowercase_basename,
			       ".html",
			       NULL);

      g_free (lowercase_basename);

      entry.entry.path            = file_proc->menu_path;
      entry.entry.accelerator     = NULL;
      entry.entry.callback        = file_open_type_callback;
      entry.entry.callback_action = 0;
      entry.entry.item_type       = NULL;
      entry.quark_string          = NULL;
      entry.help_page             = help_page;
      entry.description           = NULL;

      gimp_menu_item_create (&entry, NULL, file_proc);
    }
}

void
file_open_dialog_menu_reset (void)
{
  load_file_proc = NULL;
}

void
file_open_dialog_show (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  if (! fileload)
    file_open_dialog_create (gimp);

  gtk_widget_set_sensitive (GTK_WIDGET (fileload), TRUE);
  if (GTK_WIDGET_VISIBLE (fileload))
    return;

  gtk_file_selection_set_filename (GTK_FILE_SELECTION (fileload),
				   "." G_DIR_SEPARATOR_S);
  gtk_window_set_title (GTK_WINDOW (fileload), _("Open Image"));

  file_dialog_show (fileload);
}



/*  private functions  */

static void
file_open_dialog_create (Gimp *gimp)
{
  GtkFileSelection *file_sel;

  fileload = gtk_file_selection_new (_("Open Image"));

  g_object_set_data (G_OBJECT (fileload), "gimp", gimp);

  gtk_window_set_position (GTK_WINDOW (fileload), GTK_WIN_POS_MOUSE);
  gtk_window_set_wmclass (GTK_WINDOW (fileload), "load_image", "Gimp");

  file_sel = GTK_FILE_SELECTION (fileload);

  gtk_container_set_border_width (GTK_CONTAINER (fileload), 2);
  gtk_container_set_border_width (GTK_CONTAINER (file_sel->button_area), 2);

  g_signal_connect_swapped (G_OBJECT (file_sel->cancel_button), "clicked",
			    G_CALLBACK (file_dialog_hide),
			    fileload);
  g_signal_connect (G_OBJECT (fileload), "delete_event",
		    G_CALLBACK (file_dialog_hide),
		    NULL);

  g_signal_connect (G_OBJECT (file_sel->ok_button), "clicked",
		    G_CALLBACK (file_open_ok_callback),
		    fileload);

  gtk_quit_add_destroy (1, GTK_OBJECT (fileload));

  gtk_clist_set_selection_mode
    (GTK_CLIST (GTK_FILE_SELECTION (fileload)->file_list),
     GTK_SELECTION_EXTENDED);

  /* Catch file-clist clicks so we can update the preview thumbnail */
  g_signal_connect (G_OBJECT (file_sel->file_list), "select_row",
		    G_CALLBACK (file_open_clistrow_callback),
		    fileload);

  /*  Connect the "F1" help key  */
  gimp_help_connect (fileload,
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

    load_menu = gtk_item_factory_from_path ("<Load>")->widget;
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
    gtk_box_pack_start (GTK_BOX (hbox), open_options_genbutton,
			TRUE, FALSE, 0);
    gtk_widget_show (open_options_genbutton);	

    g_signal_connect (G_OBJECT (open_options_genbutton), "clicked",
		      G_CALLBACK (file_open_genbutton_callback),
		      fileload);

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
  gtk_label_set_text (GTK_LABEL (open_options_label), _("No Selection"));

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

  file_dialog_update_name (proc, GTK_FILE_SELECTION (fileload));

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
      g_warning ("%s: Unknown TempBuf width.", G_STRLOC);
    }

  return (rtn);
}

/* don't call with preview_fullname as parameter!  will be clobbered! */
static void
set_preview (Gimp        *gimp,
             const gchar *fullfname,
	     guchar      *RGB_source,
	     gint         RGB_w,
	     gint         RGB_h)
{
  guchar      *thumb_rgb;
  guchar      *raw_thumb;
  gint         tnw,tnh, i;
  gchar       *dirname;
  gchar       *basename;
  gchar       *tname;
  gchar       *imginfo = NULL;
  struct stat  file_stat;
  struct stat  thumb_stat;
  gboolean     thumb_may_be_outdated = FALSE;
  gboolean     show_generate_label   = TRUE;

  dirname  = g_path_get_dirname (fullfname);
  basename = g_path_get_basename (fullfname);

  tname = g_build_filename (dirname, ".xvpics", basename, NULL);

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

  gtk_frame_set_label (GTK_FRAME (open_options_frame), basename);

  g_free (dirname);
  g_free (basename);

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
			      _("This thumbnail may be out of date") :
			      (imginfo ? imginfo : _("No Information")));
	  if (imginfo)
	    g_free (imginfo);
	}
      else
	{
          if (gimp->config->write_thumbnails)
            {
	      gtk_label_set_text (GTK_LABEL(open_options_label),
				  _("Could not write thumbnail file."));
            }
          else
            {
	      gtk_label_set_text (GTK_LABEL(open_options_label),
				  _("Thumbnail saving is disabled."));
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
			     gint       row,
                             gint       column,
                             GdkEvent  *event,
                             gpointer   data)
{
  GtkFileSelection *fileload;
  Gimp             *gimp;
  const gchar      *fullfname;

  fileload = GTK_FILE_SELECTION (data);

  gimp = GIMP (g_object_get_data (G_OBJECT (fileload), "gimp"));

  fullfname = gtk_file_selection_get_filename (fileload);

  gtk_widget_set_sensitive (GTK_WIDGET (open_options_frame), TRUE);
  set_preview (gimp, fullfname, NULL, 0, 0);
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
  Gimp      *gimp;

  /* added for multi-file preview generation... */
  GtkFileSelection *fs;
  gchar            *full_filename = NULL;
  gchar            *dirname       = NULL;
  struct stat       buf;
  gint              err;
  GSList           *list, *toplist;

  fs = GTK_FILE_SELECTION (data);

  gimp = GIMP (g_object_get_data (G_OBJECT (fs), "gimp"));

  if (! preview_fullname)
    {
      g_warning ("%s: Tried to generate thumbnail for NULL file name.", G_STRLOC);
      return;
    }

  gimp_set_busy (gimp);
  gtk_widget_set_sensitive (GTK_WIDGET (fileload), FALSE);

  /* new mult-file preview make: */  

  /* Have to read the clist before touching anything else */
  toplist = clist_to_slist (GTK_CLIST (fs->file_list));

  dirname = g_path_get_dirname (gtk_file_selection_get_filename (fs));
  
  for (list= toplist; list; list = g_slist_next (list))
    {
      full_filename = g_build_filename (dirname,
                                        (gchar *) list->data,
                                        NULL);
      
      err = stat (full_filename, &buf);
      
      if (! (err == 0 && (buf.st_mode & S_IFDIR)))
        {
          /* Is not directory. */
          GimpPDBStatusType dummy;
          
          gimage_to_be_thumbed = file_open_image (gimp,
                                                  full_filename,
                                                  list->data,
                                                  NULL,
                                                  NULL,
                                                  RUN_NONINTERACTIVE,
                                                  &dummy);
          
          if (gimage_to_be_thumbed)
            {			
              tempbuf = make_thumb_tempbuf (gimage_to_be_thumbed);
              RGBbuf  = make_RGBbuf_from_tempbuf (tempbuf,
                                                  &RGBbuf_w,
                                                  &RGBbuf_h);
              if (gimp->config->write_thumbnails)
                {
                  file_save_thumbnail (gimage_to_be_thumbed,
                                       full_filename, tempbuf);
                }
              
              set_preview (gimp, full_filename,
                           RGBbuf, RGBbuf_w, RGBbuf_h);
              
              g_object_unref (G_OBJECT (gimage_to_be_thumbed));
              
              if (RGBbuf)
                g_free (RGBbuf);
            }
          else
            {
              gtk_label_set_text (GTK_LABEL (open_options_label),
                                  _("Failed to generate preview."));
            }
        }      
     }

  g_free (full_filename);

  for (list = toplist; list; list = g_slist_next (list))
    g_free (list->data);

  g_slist_free (toplist);
  g_free (dirname);

  gtk_widget_set_sensitive (GTK_WIDGET (fileload), TRUE);
  gimp_unset_busy (gimp);
}

static void
file_open_ok_callback (GtkWidget *widget,
		       gpointer   data)
{
  GtkFileSelection  *fs;
  Gimp              *gimp;
  gchar             *full_filename;
  gchar             *raw_filename;
  gchar             *dirname;
  struct stat        buf;
  gint               err;
  GimpPDBStatusType  status;
  GSList            *list;

  fs = GTK_FILE_SELECTION (data);

  gimp = GIMP (g_object_get_data (G_OBJECT (fs), "gimp"));

  full_filename = g_strdup (gtk_file_selection_get_filename (fs));
  raw_filename  = g_strdup (gtk_entry_get_text (GTK_ENTRY(fs->selection_entry)));

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
        {
          gtk_file_selection_set_filename (fs, full_filename);
        }

      return;
    }

  gtk_widget_set_sensitive (GTK_WIDGET (fs), FALSE);

  if (err) /* e.g. http://server/filename.jpg */
    full_filename = raw_filename;

  status = file_open_with_proc_and_display (gimp,
                                            full_filename, 
                                            raw_filename, 
                                            load_file_proc);

  if (status == GIMP_PDB_SUCCESS)
    {
      file_dialog_hide (data);
    }
  else if (status != GIMP_PDB_CANCEL)
    {
      /* Hackery required. Please add error message. --bex */
      g_message (_("Opening '%s' failed."), full_filename);
    }


  /*
   * Now deal with multiple selections from the filesel clist
   */

  /* Have to read the clist before touching anything else */

  list = clist_to_slist (GTK_CLIST (fs->file_list));
  
  raw_filename = g_strdup (raw_filename);
  dirname = g_path_get_dirname (full_filename);
  
  while (list)
    {
      g_free (full_filename);
      
      full_filename = g_build_filename (dirname,
                                        (gchar *) list->data, NULL);
      
      if (strcmp (list->data, raw_filename))
        { /* don't load current selection twice */
          
          err = stat (full_filename, &buf);
          
          if (! (err == 0 && (buf.st_mode & S_IFDIR)))
            { /* Is not directory. */
              
              status = file_open_with_proc_and_display (gimp,
                                                        full_filename,
                                                        (const gchar *) list->data,
                                                        load_file_proc);
              
              if (status == GIMP_PDB_SUCCESS)
                {
                  file_dialog_hide (data);
                }
              else if (status != GIMP_PDB_CANCEL)
                {
		  /* same as previous. --bex */
                  g_message (_("Opening '%s' failed."), full_filename);
                }
            }
        }
      
      g_free (list->data);
      list = g_slist_next (list);
    }
  
  g_slist_free (list);

  g_free (dirname);
  g_free (full_filename);
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
