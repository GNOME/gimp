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

#include <glib.h>

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <errno.h>

#ifdef NATIVE_WIN32
#include <direct.h>		/* For _mkdir() */
#define mkdir(path,mode) _mkdir(path)
#endif

#include "appenv.h"
#include "actionarea.h"
#include "cursorutil.h"
#include "gdisplay.h"
#include "general.h"
#include "gimage.h"
#include "fileops.h"
#include "interface.h"
#include "menus.h"
#include "plug_in.h"
#include "procedural_db.h"
#include "gimprc.h"
#include "docindex.h"
#include "undo.h"

#include "libgimp/gimpintl.h"

typedef struct _OverwriteBox OverwriteBox;

struct _OverwriteBox
{
  GtkWidget * obox;
  char *      full_filename;
  char *      raw_filename;
};

static void file_overwrite              (char *filename,
					 char* raw_filename);
static void file_overwrite_yes_callback (GtkWidget *w,
					 gpointer   client_data);
static void file_overwrite_no_callback  (GtkWidget *w,
					 gpointer   client_data);

static gint file_overwrite_delete_callback  (GtkWidget *w,
					     GdkEvent  *e,
					     gpointer   client_data);

static GimpImage* file_open_image   (char *filename,
				     char *raw_filename,
				     RunModeType runmode);

static void genbutton_callback (GtkWidget *w,
				gpointer   client_data);

static void file_open_clistrow_callback (GtkWidget *w,
				         int        client_data);
static void file_open_ok_callback   (GtkWidget *w,
				     gpointer   client_data);
static void file_save_ok_callback   (GtkWidget *w,
				     gpointer   client_data);

static void file_dialog_show        (GtkWidget *filesel);
static int  file_dialog_hide        (GtkWidget *filesel);
static void file_update_name        (PlugInProcDef *proc,
				     GtkWidget     *filesel);
static void file_load_type_callback (GtkWidget *w,
				     gpointer   client_data);
static void file_save_type_callback (GtkWidget *w,
				     gpointer   client_data);

static void file_convert_string (char *instr,
                                 char *outmem,
                                 int maxmem,
                                 int *nmem);

static int  file_check_single_magic (char *offset,
                                     char *type,
                                     char *value,
                                     int headsize,
                                     unsigned char *file_head,
                                     FILE *ifp);

static int  file_check_magic_list (GSList *magics_list,
                                   int headsize,
                                   unsigned char *head,
                                   FILE *ifp);

static void      file_update_menus      (GSList *procs,
					 int     image_type);


GtkWidget  *fileload = NULL; /* Shared with dialog_handler.c */
static GtkWidget  *filesave = NULL;
static GtkWidget  *open_options = NULL;
static GtkWidget  *save_options = NULL;
/* widgets for the open_options menu */
static GtkPreview *open_options_preview = NULL;
static GtkWidget  *open_options_fixed = NULL;
static GtkWidget  *open_options_label = NULL;
static GtkWidget  *open_options_frame = NULL;
static GtkWidget  *open_options_genbuttonlabel = NULL;
/* Some state for the thumbnailer */
static gchar      *preview_fullname = NULL;

GSList *load_procs = NULL;
GSList *save_procs = NULL;

static PlugInProcDef *load_file_proc = NULL;
static PlugInProcDef *save_file_proc = NULL;

static GimpImage* the_gimage = NULL;

#define FILE_ERR_MESSAGE(str)				G_STMT_START{	\
  if (message_handler == MESSAGE_BOX)					\
    message_box ((str), file_message_box_close_callback, (void *) fs);	\
  else									\
    g_message (str);							\
    gtk_widget_set_sensitive (GTK_WIDGET (fs), TRUE);	}G_STMT_END

static void
file_message_box_close_callback (GtkWidget *w,
				 gpointer   client_data)
{
  GtkFileSelection *fs;

  fs = (GtkFileSelection *) client_data;

  gtk_widget_set_sensitive (GTK_WIDGET (fs), TRUE);
}

void
file_ops_pre_init (void)
{
}

void
file_ops_post_init (void)
{
  GtkMenuEntry entry;
  PlugInProcDef *file_proc;
  GSList *tmp;

  load_procs = g_slist_reverse (load_procs);
  save_procs = g_slist_reverse (save_procs);

  tmp = load_procs;
  while (tmp)
    {
      file_proc = tmp->data;
      tmp = tmp->next;

      entry.path = file_proc->menu_path;
      entry.accelerator = NULL;
      entry.callback = file_load_type_callback;
      entry.callback_data = file_proc;

      menus_create (&entry, 1);
    }

  tmp = save_procs;
  while (tmp)
    {
      file_proc = tmp->data;
      tmp = tmp->next;

      entry.path = file_proc->menu_path;
      entry.accelerator = NULL;
      entry.callback = file_save_type_callback;
      entry.callback_data = file_proc;

      menus_create (&entry, 1);
    }
}

void
file_open_callback (GtkWidget *w,
		    gpointer   client_data)
{
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *label;
  GtkWidget *option_menu;
  GtkWidget *load_menu;
  GtkWidget *open_options_genbutton;
  GDisplay *gdisplay;

  if (!fileload)
    {
      fileload = gtk_file_selection_new (_("Load Image"));
      gtk_window_set_position (GTK_WINDOW (fileload), GTK_WIN_POS_MOUSE);
      gtk_window_set_wmclass (GTK_WINDOW (fileload), "load_image", "Gimp");
      gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION (fileload)->cancel_button),
				 "clicked",
				 GTK_SIGNAL_FUNC (file_dialog_hide),
				 GTK_OBJECT (fileload));
      gtk_signal_connect (GTK_OBJECT (fileload),
			  "delete_event",
			  GTK_SIGNAL_FUNC (file_dialog_hide),
			  NULL);
      gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (fileload)->ok_button), "clicked", (GtkSignalFunc) file_open_ok_callback, fileload);
      gtk_quit_add_destroy (1, GTK_OBJECT (fileload));

      gtk_clist_set_selection_mode (GTK_CLIST
				    (GTK_FILE_SELECTION (fileload)->file_list),
				    GTK_SELECTION_EXTENDED);

      /* Catch file-clist clicks so we can update the preview thumbnail */
      gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (fileload)->file_list), "select_row", (GtkSignalFunc) file_open_clistrow_callback, fileload);
    }
  else
    {
      gtk_widget_set_sensitive (GTK_WIDGET (fileload), TRUE);
      if (GTK_WIDGET_VISIBLE (fileload))
	return;

      gtk_file_selection_set_filename (GTK_FILE_SELECTION(fileload),
				       "." G_DIR_SEPARATOR_S);
      gtk_window_set_title (GTK_WINDOW (fileload), _("Load Image"));
    }

  gdisplay = gdisplay_active ();

  if (!open_options)
    {
      GtkWidget* frame;

      open_options = gtk_hbox_new (TRUE, 1);

      /* format-chooser frame */
      frame = gtk_frame_new (_("Open Options"));
      {
	gtk_frame_set_shadow_type (GTK_FRAME (frame),
				   GTK_SHADOW_ETCHED_IN);
	
	vbox = gtk_vbox_new (FALSE, 0);
	{
	  gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
	  gtk_container_add (GTK_CONTAINER (frame), vbox);

	  hbox = gtk_hbox_new (FALSE, 1);
	  {
	    gtk_container_set_border_width (GTK_CONTAINER (hbox), 2);
	    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);
	    
	    label = gtk_label_new (_("Determine file type:"));
	    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 5);
	    gtk_widget_show (label);
	    
	    option_menu = gtk_option_menu_new ();
	    gtk_box_pack_start (GTK_BOX (hbox), option_menu, FALSE, TRUE, 0);
	    gtk_widget_show (option_menu);
	    
	    menus_get_load_menu (&load_menu, NULL);
	    gtk_option_menu_set_menu (GTK_OPTION_MENU (option_menu), load_menu);
	    gtk_box_pack_start (GTK_BOX (open_options), frame, TRUE, TRUE, 5);
	  }
	  gtk_widget_show (hbox);
	}
	gtk_widget_show (vbox);
      }
      gtk_widget_show (frame);

      /* Preview frame */
      frame = gtk_frame_new ("");
      open_options_frame = frame;
      {
	gtk_frame_set_shadow_type (GTK_FRAME (frame),
				   GTK_SHADOW_ETCHED_IN);
	gtk_box_pack_end (GTK_BOX (open_options), frame, FALSE, TRUE, 5);
	
	vbox = gtk_vbox_new (FALSE, 1);
	{
	  gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);
	  gtk_container_add (GTK_CONTAINER (frame), vbox);

	  hbox = gtk_hbox_new (TRUE, 1);
	  {
	    gtk_container_set_border_width (GTK_CONTAINER (hbox), 0);
	    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

	    open_options_genbutton = gtk_button_new();
	    {
	      gtk_signal_connect (GTK_OBJECT (open_options_genbutton),
				  "clicked",
				  (GtkSignalFunc) genbutton_callback,
				  open_options_genbutton);
	      gtk_box_pack_end (GTK_BOX (hbox), open_options_genbutton,
				TRUE, FALSE, 0);
	    }
	    gtk_widget_show (open_options_genbutton);	
	    
	    open_options_fixed = gtk_fixed_new ();
	    {
	      GtkWidget* abox;
	      GtkWidget* sbox;
	      
	      gtk_widget_set_usize (open_options_fixed, 80, 60);
	      gtk_container_add (GTK_CONTAINER (GTK_BIN (open_options_genbutton)), open_options_fixed);
	      
	      sbox = gtk_vbox_new(TRUE, 0);
	      {
		GtkWidget* align;

		gtk_container_add (GTK_CONTAINER (open_options_fixed),
				   GTK_WIDGET (sbox));

		align = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
		{
		  gtk_widget_set_usize (align, 80, 60);
		  
		  gtk_box_pack_start (GTK_BOX (sbox),
				      GTK_WIDGET (align),
				      FALSE, TRUE, 0);
		  
		  abox = gtk_hbox_new (FALSE, 0);
		  {
		    gtk_container_add (GTK_CONTAINER (align), abox);
		    
		    open_options_preview = GTK_PREVIEW (gtk_preview_new
							(GTK_PREVIEW_COLOR));
		    {
		      gtk_box_pack_start (GTK_BOX (abox),
					  GTK_WIDGET (open_options_preview),
					  FALSE, TRUE, 0);
		    }
		    gtk_widget_show(GTK_WIDGET (open_options_preview));
		    
		    open_options_genbuttonlabel = gtk_label_new("generate\npreview");
		    {
		      gtk_box_pack_start (GTK_BOX (abox),
					  GTK_WIDGET (open_options_genbuttonlabel),
					  FALSE, TRUE, 0);
		    }
		    gtk_widget_show(GTK_WIDGET (open_options_genbuttonlabel));
		  }
		  gtk_widget_show(abox);
		}
		gtk_widget_show(align);
	      }
	      gtk_widget_show(sbox);
	    }
	    gtk_widget_show (open_options_fixed);
	  }
	  gtk_widget_show (hbox);

	  open_options_label = gtk_label_new ("");
	  {
	    gtk_box_pack_end (GTK_BOX (vbox), open_options_label,
			      FALSE, TRUE, 0);
	  }
	  gtk_widget_show (open_options_label); 
	}
	gtk_widget_show (vbox);
      }
      gtk_widget_show (frame);

      /* pack the containing open_options hbox into the load-dialog */
      gtk_box_pack_end (GTK_BOX (GTK_FILE_SELECTION (fileload)->main_vbox),
			open_options, FALSE, FALSE, 1);
    }

  gtk_frame_set_label (GTK_FRAME(open_options_frame), _("Preview"));
  gtk_label_set_text (GTK_LABEL(open_options_label), "No selection.");

  gtk_widget_show (GTK_WIDGET(open_options_genbuttonlabel));
  gtk_widget_hide (GTK_WIDGET(open_options_preview));
  gtk_widget_set_sensitive (GTK_WIDGET(open_options_frame), FALSE);

  gtk_widget_show (open_options);

  file_dialog_show (fileload);
}

void
file_save_callback (GtkWidget *w,
		    gpointer   client_data)
{
  GDisplay *gdisplay;

  gdisplay = gdisplay_active ();

  /*  Only save if the gimage has been modified  */
  if (gdisplay->gimage->dirty != 0)
    {
      if (gdisplay->gimage->has_filename == FALSE)
	{
	  popup_shell = gdisplay->shell;
	  file_save_as_callback (w, client_data);
	}
      else
	file_save (gdisplay->gimage, gimage_filename (gdisplay->gimage),
		   g_basename (gimage_filename(gdisplay->gimage)), 2);
    }
}

void
file_save_as_callback (GtkWidget *w,
		       gpointer   client_data)
{
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *option_menu;
  GtkWidget *save_menu;
  GDisplay *gdisplay;

  if (!filesave)
    {
      filesave = gtk_file_selection_new (_("Save Image"));
      gtk_window_set_wmclass (GTK_WINDOW (filesave), "save_image", "Gimp");
      gtk_window_set_position (GTK_WINDOW (filesave), GTK_WIN_POS_MOUSE);
      gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION (filesave)->cancel_button),
			  "clicked",
			  GTK_SIGNAL_FUNC (file_dialog_hide),
			  GTK_OBJECT (filesave));
      gtk_signal_connect (GTK_OBJECT (filesave),
			  "delete_event",
			  GTK_SIGNAL_FUNC (file_dialog_hide),
			  NULL);
      gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (filesave)->ok_button), "clicked", (GtkSignalFunc) file_save_ok_callback, filesave);
      gtk_quit_add_destroy (1, GTK_OBJECT (filesave));
    }
  else
    {
      gtk_widget_set_sensitive (GTK_WIDGET (filesave), TRUE);
      if (GTK_WIDGET_VISIBLE (filesave))
	return;

      gtk_file_selection_set_filename (GTK_FILE_SELECTION(filesave),
				       "." G_DIR_SEPARATOR_S);
      gtk_window_set_title (GTK_WINDOW (filesave), _("Save Image"));
    }

  gdisplay = gdisplay_active ();
  the_gimage = gdisplay->gimage;

  if (!save_options)
    {
      save_options = gtk_frame_new (_("Save Options"));
      gtk_frame_set_shadow_type (GTK_FRAME (save_options), GTK_SHADOW_ETCHED_IN);

      hbox = gtk_hbox_new (FALSE, 1);
      gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);
      gtk_container_add (GTK_CONTAINER (save_options), hbox);
      gtk_widget_show (hbox);

      label = gtk_label_new (_("Determine file type:"));
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
      gtk_widget_show (label);

      option_menu = gtk_option_menu_new ();
      gtk_box_pack_start (GTK_BOX (hbox), option_menu, TRUE, TRUE, 0);
      gtk_widget_show (option_menu);

      menus_get_save_menu (&save_menu, NULL);
      gtk_option_menu_set_menu (GTK_OPTION_MENU (option_menu), save_menu);
      gtk_box_pack_end (GTK_BOX (GTK_FILE_SELECTION (filesave)->main_vbox),
			save_options, FALSE, FALSE, 5);
    }

  switch (gdisplay->gimage->base_type)
    {
    case RGB:
      file_update_menus (save_procs, RGB_IMAGE);
      break;
    case GRAY:
      file_update_menus (save_procs, GRAY_IMAGE);
      break;
    case INDEXED:
      file_update_menus (save_procs, INDEXED_IMAGE);
      break;
    }

  gtk_widget_show (save_options);

  file_dialog_show (filesave);
}

void
file_revert_callback (GtkWidget *w,
		      gpointer   client_data)
{
  GDisplay *gdisplay;
  GimpImage *gimage;
  char *filename, *raw_filename;

  gdisplay = gdisplay_active ();

  if (gdisplay->gimage->has_filename == FALSE)
    g_message (_("Can't revert. No filename associated with this image"));
  else
    {
      filename = gimage_filename (gdisplay->gimage);
      raw_filename = g_basename (filename);

      gimage = file_open_image (filename, raw_filename, RUN_INTERACTIVE);

      if (gimage != NULL)
	{
	  undo_free (gimage);
	  gdisplay_reconnect (gdisplay, gimage);
	}
      else
        g_message (_("Revert failed."));
    }
}

void
file_load_by_extension_callback (GtkWidget *w,
				 gpointer   client_data)
{
  load_file_proc = NULL;
}

void
file_save_by_extension_callback (GtkWidget *w,
				 gpointer   client_data)
{
  save_file_proc = NULL;
}

static void
file_update_name (PlugInProcDef *proc, GtkWidget *filesel)
{
  if (proc->extensions_list)
    {
      char* text = gtk_entry_get_text (GTK_ENTRY(GTK_FILE_SELECTION(filesel)->selection_entry));
      char* last_dot = strrchr (text, '.');
      GString *s;

      if (last_dot == text || !text[0])
	return;

      s = g_string_new (text);

      if (last_dot)
	g_string_truncate (s, last_dot-text);

      g_string_append (s, ".");
      g_string_append (s, (char*) proc->extensions_list->data);

      gtk_entry_set_text (GTK_ENTRY(GTK_FILE_SELECTION(filesel)->selection_entry), s->str);

      g_string_free (s, TRUE);
    }
}

static void
file_load_type_callback (GtkWidget *w,
			 gpointer   client_data)
{
  PlugInProcDef* proc = (PlugInProcDef *) client_data;

  file_update_name (proc, fileload);

  load_file_proc = proc;
}

static void
file_save_type_callback (GtkWidget *w,
			 gpointer   client_data)
{
  PlugInProcDef* proc = (PlugInProcDef *) client_data;

  file_update_name (proc, filesave);

  save_file_proc = proc;
}

static GimpImage*
file_open_image (char *filename, char *raw_filename, RunModeType runmode)
{
  PlugInProcDef *file_proc;
  ProcRecord *proc;
  Argument *args;
  Argument *return_vals;
  int gimage_id;
  gboolean status;
  int i;

  file_proc = load_file_proc;
  if (!file_proc)
    file_proc = file_proc_find (load_procs, filename);

  if (!file_proc)
    {
      /* WARNING */
      return NULL;
    }

  proc = &file_proc->db_info;

  args = g_new (Argument, proc->num_args);
  memset (args, 0, (sizeof (Argument) * proc->num_args));

  for (i = 0; i < proc->num_args; i++)
    args[i].arg_type = proc->args[i].arg_type;

  args[0].value.pdb_int = runmode;
  args[1].value.pdb_pointer = filename;
  args[2].value.pdb_pointer = raw_filename;

  return_vals = procedural_db_execute (proc->name, args);
  status = (return_vals[0].value.pdb_int == PDB_SUCCESS);
  gimage_id = return_vals[1].value.pdb_int;

  procedural_db_destroy_args (return_vals, proc->num_values);
  g_free (args);

  if (status)
    return pdb_id_to_image (gimage_id);
  else
    return NULL;
}

int
file_open (char *filename, char *raw_filename)
{
  GimpImage *gimage;

  if ((gimage = file_open_image (filename, raw_filename, RUN_INTERACTIVE)) != NULL)
    {
      /* enable & clear all undo steps */
      gimage_enable_undo (gimage);

      /* set the image to clean  */
      gimage_clean_all (gimage);

      /* display the image */
      gdisplay_new (gimage, 0x0101);

      idea_add (filename);
      menus_last_opened_add (filename);

      return TRUE;
    }

  return FALSE;
}


static gboolean
file_save_thumbnail (GimpImage* gimage,
		     const char *full_source_filename)
{
  TempBuf* tempbuf;
  gint i,j;
  unsigned char* tbd;
  gchar* pathname;
  gchar* filename;
  gchar* xvpathname;
  gchar* thumbnailname;
  gint w,h;
  GimpImageBaseType basetype;
  FILE* fp;
  struct stat statbuf;

  if (stat(full_source_filename, &statbuf) != 0)
    {
      return FALSE;
    }

  if (gimp_image_preview_valid (gimage, GRAY_CHANNEL))
    {
      /* just for debugging */
      printf("(incidentally, gimage already has a valid preview - %dx%d)\n",
	     gimage->comp_preview->width,
	     gimage->comp_preview->height);
    }

  pathname = g_dirname(full_source_filename);
  filename = g_basename(full_source_filename); /* Don't free! */

  xvpathname = g_strconcat(pathname, G_DIR_SEPARATOR_S, ".xvpics",
			   NULL);

  thumbnailname = g_strconcat (xvpathname, G_DIR_SEPARATOR_S,
			       filename,
			       NULL);

  if (gimage->width<=80 && gimage->height<=60)
    {
      w = gimage->width;
      h = gimage->height;
    }
  else
    {
      /* Ratio molesting to fit within .xvpic thumbnail size limits */
      if (60*gimage->width < 80*gimage->height)
	{
	  h = 60;
	  w = (60*gimage->width)/gimage->height;
	  if (w==0)
	    w = 1;
	}
      else
	{
	  w = 80;
	  h = (80*gimage->height)/gimage->width;
	  if (h==0)
	    h = 1;
	}
    }

  /*printf("tn: %d x %d -> ", w, h);fflush(stdout);*/

  tempbuf = gimp_image_composite_preview (gimage, GRAY_CHANNEL, w, h);
  tbd = temp_buf_data(tempbuf);

  w = tempbuf->width;
  h = tempbuf->height;
  /*printf("tn: %d x %d\n", w, h);fflush(stdout);*/

  mkdir (xvpathname, 0755);

  fp = fopen(thumbnailname,"wb");
  g_free(pathname);
  g_free(xvpathname);
  g_free(thumbnailname);

  if (fp)
    {
      basetype = gimp_image_base_type(gimage);

      fprintf(fp,
	      "P7 332\n#IMGINFO:%dx%d %s (%d %s)\n"
	      "#END_OF_COMMENTS\n%d %d 255\n",
	      gimage->width, gimage->height,
	      (basetype == RGB) ? "RGB" :
	      (basetype == GRAY) ? "Greyscale" :
	      (basetype == INDEXED) ? "Indexed" :
	      "(UNKNOWN COLOUR TYPE)",
	      (int)statbuf.st_size,
	      (statbuf.st_size == 1) ? "byte" : "bytes",
	      w, h);

      switch (basetype)
	{
	case INDEXED:
	case RGB:
	  for (i=0; i<h; i++)
	    {
	      /* Do a cheap unidirectional error-spread to look better */
	      gint rerr=0, gerr=0, berr=0, a;

	      for (j=0; j<w; j++)
		{
		  gint32 r,g,b;

		  if (128 & *(tbd + 3))
		    {
		      r = *(tbd++) + rerr;
		      g = *(tbd++) + gerr;
		      b = *(tbd++) + berr;
		      tbd++;
		    }
		  else
		    {
		      a = (( (i^j) & 4 ) << 5) | 64; /* cute. */
		      r = a + rerr;
		      g = a + gerr;
		      b = a + berr;
		      tbd += 4;
		    }

		  r = CLAMP0255 (r);
		  g = CLAMP0255 (g);
		  b = CLAMP0255 (b);

		  fputc(((r>>5)<<5) | ((g>>5)<<2) | (b>>6), fp);

		  rerr = r - ( (r>>5) * 255 ) / 7;
		  gerr = g - ( (g>>5) * 255 ) / 7;
		  berr = b - ( (b>>6) * 255 ) / 3;
		}
	    }
	  break;
	case GRAY:
	  for (i=0; i<h; i++)
	    {
	      /* Do a cheap unidirectional error-spread to look better */
	      gint b3err=0, b2err=0, v, a;

	      for (j=0; j<w; j++)
		{
		  gint32 b3, b2;

		  v = *(tbd++);
		  a = *(tbd++);

		  if (!(128 & a))
		    v = (( (i^j) & 4 ) << 5) | 64;

		  b2 = v + b2err;
		  b3 = v + b3err;

		  b2 = CLAMP0255 (b2);
		  b3 = CLAMP0255 (b3);

		  fputc(((b3>>5)<<5) | ((b3>>5)<<2) | (b2>>6), fp);

		  b2err = b2 - ( (b2>>6) * 255 ) / 3;
		  b3err = b3 - ( (b3>>5) * 255 ) / 7;
		}
	    }
	  break;
	default:
	  g_warning("UNKNOWN GIMAGE TYPE IN THUMBNAIL SAVE");
	}
      fclose(fp);
    }
  else /* Error writing thumbnail */
    {
      return (FALSE);
    }

  return (TRUE);
}


int
file_save (GimpImage* gimage,
	   char *filename,
	   char *raw_filename,
           gint mode)
{
  PlugInProcDef *file_proc;
  ProcRecord *proc;
  Argument *args;
  Argument *return_vals;
  int return_val;
  int i;

  if (gimage_active_drawable (gimage) == NULL)
    return FALSE;

  file_proc = gimage_get_save_proc(gimage);

  if (!file_proc)
    file_proc = file_proc_find (save_procs, raw_filename);

  if (!file_proc)
    return FALSE;

  proc = &file_proc->db_info;

  args = g_new (Argument, proc->num_args);
  memset (args, 0, (sizeof (Argument) * proc->num_args));

  for (i = 0; i < proc->num_args; i++)
    args[i].arg_type = proc->args[i].arg_type;

  args[0].value.pdb_int = mode;
  args[1].value.pdb_int = pdb_image_to_id(gimage);
  args[2].value.pdb_int = drawable_ID (gimage_active_drawable (gimage));
  args[3].value.pdb_pointer = filename;
  args[4].value.pdb_pointer = raw_filename;

  return_vals = procedural_db_execute (proc->name, args);
  return_val = (return_vals[0].value.pdb_int == PDB_SUCCESS);

  if (return_val)
    {
      /*  set this image to clean  */
      gimage_clean_all (gimage);

      /* these calls must come before the call to gimage_set_filename */
      idea_add (filename);
      menus_last_opened_add (filename);

      /*  use the same plug-in for this image next time  */
      /* DISABLED - gets stuck on first saved format... needs
	 attention --Adam */
      /* gimage_set_save_proc(gimage, file_proc); */

      file_save_thumbnail (gimage, filename);
      
      /*  set the image title  */
      gimp_image_set_filename (gimage, filename);
      /* note: 'filename' may have been free'd by above call! */
    }

  g_free (return_vals);
  g_free (args);


  return return_val;
}


/* The readXVThumb function source may be re-used under
   the XFree86-style license. <adam@gimp.org> */
static guchar*
readXVThumb(const gchar *fnam,
	    gint* w, gint* h,
	    gchar** imginfo /* caller frees if != NULL */)
{
  FILE *fp;
  const gchar *P7_332 = "P7 332";
  gchar P7_buf[7];
  gchar linebuf[200];
  guchar *buf;
  gint twofivefive;
  void *ptr;

  *w = *h = 0;
  *imginfo = NULL;

  fp = fopen (fnam, "rb");
  if (!fp)
    return (NULL);

  fread (P7_buf, 6, 1, fp);

  if (strncmp(P7_buf, P7_332, 6)!=0)
    {
      g_warning("Thumbnail doesn't have the 'P7 332' header.");
      fclose(fp);
      return(NULL);
    }

  /*newline*/
  fread (P7_buf, 1, 1, fp);

  do
    {
      ptr = fgets(linebuf, 199, fp);
      if ((strncmp(linebuf, "#IMGINFO:", 9) == 0) &&
	  (linebuf[9] != '\0') &&
	  (linebuf[9] != '\n'))
	{
	  if (linebuf[strlen(linebuf)-1] == '\n')
	    linebuf[strlen(linebuf)-1] = '\0';

	  if (linebuf[9] != '\0')
	    {
	      if (*imginfo)
		g_free(*imginfo);
	      *imginfo = g_strdup (&linebuf[9]);
	    }
	}
    }
  while (ptr && linebuf[0]=='#'); /* keep throwing away comment lines */

  if (!ptr)
    {
      /* g_warning("Thumbnail ended - not an image?"); */
      fclose(fp);
      return(NULL);
    }

  sscanf(linebuf, "%d %d %d\n", w, h, &twofivefive);

  if (twofivefive!=255)
    {
      g_warning("Thumbnail is of funky depth.");
      fclose(fp);
      return(NULL);
    }

  if ((*w)<1 || (*h)<1 || (*w)>80 || (*h)>60)
    {
      g_warning ("Thumbnail size bad.  Corrupted?");
      fclose(fp);
      return (NULL);
    }

  buf = g_malloc((*w)*(*h));

  fread(buf, (*w)*(*h), 1, fp);
  
  fclose(fp);
  
  return(buf);
}


/* don't call with preview_fullname as parameter!  will be clobbered! */
static void
set_preview (const gchar* fullfname)
{
  guchar *thumb_rgb;
  guchar *raw_thumb;
  gint    tnw,tnh;
  gchar  *pname;
  gchar  *fname;
  gchar  *tname;
  gchar  *imginfo;
  struct stat file_stat;
  struct stat thumb_stat;
  gboolean thumb_may_be_outdated = FALSE;
  gboolean show_generate_label = TRUE;


  pname = g_dirname (fullfname);
  fname = g_basename (fullfname); /* Don't free this! */
  tname = g_strconcat (pname, G_DIR_SEPARATOR_S, ".xvpics", G_DIR_SEPARATOR_S,
		       fname,
		       NULL);

  g_free (pname);

  /* If the file is newer than its thumbnail, the thumbnail may
     be out of date. */
  if ((stat(tname,     &thumb_stat)==0) &&
      (stat(fullfname, &file_stat )==0))
    {
      if ((thumb_stat.st_mtime) < (file_stat.st_mtime))
	{
	  thumb_may_be_outdated = TRUE;
	}
    }

  raw_thumb = readXVThumb(tname, &tnw, &tnh, &imginfo);

  g_free (tname);

  gtk_frame_set_label (GTK_FRAME(open_options_frame),
		       fname);

  if (preview_fullname)
    {
      g_free (preview_fullname);
    }
  preview_fullname = g_strdup (fullfname);

  if (raw_thumb)
    {
      int i;
      thumb_rgb = g_malloc (3 * tnw * tnh);

      for (i=0; i<tnw*tnh; i++)
	{
	  thumb_rgb[i*3  ] = ((raw_thumb[i]>>5)*255)/7;
	  thumb_rgb[i*3+1] = (((raw_thumb[i]>>2)&7)*255)/7;
	  thumb_rgb[i*3+2] = (((raw_thumb[i])&3)*255)/3;
	}
      
      gtk_preview_size (open_options_preview, tnw, tnh);

      for (i=0; i<tnh; i++)
	{
	  gtk_preview_draw_row(open_options_preview, &thumb_rgb[3*i*tnw],
			       0, i,
			       tnw);
	}

      gtk_label_set_text (GTK_LABEL(open_options_label),
			  thumb_may_be_outdated ?
			  "(this thumbnail may be out of date)" :
			  (imginfo ? imginfo : "(no information)"));
      gtk_widget_show (GTK_WIDGET(open_options_preview));
      gtk_widget_queue_draw (GTK_WIDGET(open_options_preview));

      show_generate_label = FALSE;
      
      g_free (thumb_rgb);
      g_free (raw_thumb);
    }
  else
    {
      if (imginfo)
	g_free(imginfo);

      gtk_widget_hide (GTK_WIDGET(open_options_preview));
      gtk_label_set_text (GTK_LABEL(open_options_label),
			  "no preview available");
    }

  if (show_generate_label)
    {
      gtk_widget_hide (GTK_WIDGET(open_options_preview));
      gtk_widget_show (GTK_WIDGET(open_options_genbuttonlabel));
    }
  else
    {
      gtk_widget_hide (GTK_WIDGET(open_options_genbuttonlabel));
      gtk_widget_show (GTK_WIDGET(open_options_preview));
    }
}


static void
file_open_clistrow_callback (GtkWidget *w,
			     int client_data)
{
  gchar  *fullfname = NULL;

  fullfname = gtk_file_selection_get_filename(GTK_FILE_SELECTION(fileload));

  gtk_widget_set_sensitive (GTK_WIDGET(open_options_frame), TRUE);
  set_preview (fullfname);
}


static void
genbutton_callback (GtkWidget *w,
		    gpointer   client_data)
{
  GimpImage* gimage_to_be_thumbed;
  gchar* filename;

  if (!preview_fullname)
    {
      g_warning ("Tried to generate thumbnail for NULL filename.");
      return;
    }
  
  filename = g_strdup(preview_fullname);

  gimp_add_busy_cursors();
  gtk_widget_set_sensitive (GTK_WIDGET (fileload), FALSE);
  if ((gimage_to_be_thumbed = file_open_image (filename,
 					       g_basename(filename),
					       RUN_NONINTERACTIVE)))
    {
      file_save_thumbnail (gimage_to_be_thumbed, filename);
      set_preview(filename);
      gimage_delete (gimage_to_be_thumbed);
    }
  else
    {
      gtk_label_set_text (GTK_LABEL(open_options_label),
			  "(could not make preview)");
    }

  gtk_widget_set_sensitive (GTK_WIDGET (fileload), TRUE);
  gimp_remove_busy_cursors(NULL);

  g_free (filename);
}


static void
file_open_ok_callback (GtkWidget *w,
		       gpointer   client_data)
{
  GtkFileSelection *fs;
  char* filename, *raw_filename, *mfilename;
  gchar* filedirname;
  struct stat buf;
  int err;
  GString *s;

  fs = GTK_FILE_SELECTION (client_data);
  filename = gtk_file_selection_get_filename (fs);
  raw_filename = gtk_entry_get_text (GTK_ENTRY(fs->selection_entry));

  g_assert (filename && raw_filename);

  if (strlen (raw_filename) == 0)
    return;

  err = stat (filename, &buf);

  if (err == 0 && (buf.st_mode & S_IFDIR))
    {
      GString *s = g_string_new (filename);
      if (s->str[s->len - 1] != G_DIR_SEPARATOR)
        {
          g_string_append_c (s, G_DIR_SEPARATOR);
        }
      gtk_file_selection_set_filename (fs, s->str);
      g_string_free (s, TRUE);
      return;
    }

  gtk_widget_set_sensitive (GTK_WIDGET (fs), FALSE);

  if (err)
    filename = raw_filename;

  if (file_open (filename, raw_filename))
    {
      file_dialog_hide (client_data);
      gtk_widget_set_sensitive (GTK_WIDGET (fs), TRUE);
    }
  else
    {
      s = g_string_new (_("Open failed: "));
      g_string_append (s, raw_filename);
      FILE_ERR_MESSAGE (s->str);
      g_string_free (s, TRUE);
    }


  /*
   * Now deal with multiple selections from the filesel clist
   */

  /*
   * FIXME: cope with cwd != fileselcwd != dirname(filename)
   */

  /*
  {
    HistoryCallbackArg *callback_arg;
    GList *row = fs->history_list;

    while (row)
      {
	callback_arg = row->data;

	printf ("(%s)\n", callback_arg->directory );

	row = g_list_next (row);
      }
      }*/

  /*  filedirname = fs->history_list->data;*/
  
  filedirname = g_dirname (filename);

  {
    GList *row = GTK_CLIST(fs->file_list)->row_list;
    gint rownum = 0;
    gchar* temp;

    /*printf("%d\n", GTK_CLIST(fs->file_list)->rows);*/

    while (row)
      {
	if (GTK_CLIST_ROW(row)->state == GTK_STATE_SELECTED)
	  {
	    if (gtk_clist_get_cell_type(GTK_CLIST(fs->file_list),
					rownum, 0) == GTK_CELL_TEXT)
	      {
		gtk_clist_get_text (GTK_CLIST(fs->file_list),
				    rownum, 0, &temp);
		
		/*printf("%s\n", temp);*/

		/* When doing multiple selections, the name
		 * of the first item touched with the cursor will
		 * become the text-field default - and we don't
		 * want to load that twice.
		 */
		/*printf("[%s]{%s}\n", temp, raw_filename);*/
		/*		fflush(stdout);*/
		if (strcmp(temp, raw_filename)==0)
		  {
		    goto next_iter;
		  }

		mfilename = g_strdup (temp);
		
		err = stat (mfilename, &buf);
		
		if (! (err == 0 && (buf.st_mode & S_IFDIR)))
		  { /* Is not directory. */
		    
		    if (err)
		      {
			g_free (mfilename);
			mfilename = g_strconcat (filedirname,
						 G_DIR_SEPARATOR_S,
						 temp, NULL);

			/*printf(">>> %s\n", mfilename);*/
			/*fflush (stdout);*/
		      }
		    
		    if (file_open (mfilename, temp))
		      {
			file_dialog_hide (client_data);
			gtk_widget_set_sensitive (GTK_WIDGET (fs), TRUE);
		      }
		    else
		      {
			s = g_string_new (_("Open failed: "));
			g_string_append (s, temp);
			FILE_ERR_MESSAGE (s->str);
			g_string_free (s, TRUE);
		      }
		  }

		if (mfilename)
		  {
		    g_free (mfilename);
		    mfilename = NULL;
		  }
	      }
	  }

      next_iter:
	rownum++;
	row = g_list_next (row);
      }

    /*fflush(stdout);*/
  }

  if (filedirname)
    {
      g_free (filedirname);
      filedirname = NULL;
    }
}

static void
file_save_ok_callback (GtkWidget *w,
		       gpointer   client_data)
{
  GtkFileSelection *fs;
  char* filename, *raw_filename;
  GString* s;
  struct stat buf;
  int err;

  fs = GTK_FILE_SELECTION (client_data);
  filename = gtk_file_selection_get_filename (fs);
  raw_filename = gtk_entry_get_text (GTK_ENTRY(fs->selection_entry));
  err = stat (filename, &buf);

  g_assert (filename && raw_filename);

  if (err == 0)
    {
      if (buf.st_mode & S_IFDIR)
	{
	  GString *s = g_string_new (filename);
	  g_string_append_c (s, G_DIR_SEPARATOR);
	  gtk_file_selection_set_filename (fs, s->str);
	  g_string_free (s, TRUE);
	  return;
	}
      else if (buf.st_mode & S_IFREG)
	{
	  gtk_widget_set_sensitive (GTK_WIDGET (fs), FALSE);
	  file_overwrite (g_strdup (filename), g_strdup (raw_filename));
	  return;
	}
      else
	{
	  s = g_string_new (NULL);
	  g_string_sprintf (s, _("%s is an irregular file (%s)"), raw_filename, g_strerror(errno));
	}
    } else {
      gtk_widget_set_sensitive (GTK_WIDGET (fs), FALSE);

      gimage_set_save_proc(the_gimage, save_file_proc);
      if (file_save (the_gimage, filename, raw_filename, 0))
	{
	  file_dialog_hide (client_data);
	  gtk_widget_set_sensitive (GTK_WIDGET (fs), TRUE);
	  return;
	}
      else
	{
	  s = g_string_new (_("Save failed: "));
	  g_string_append (s, raw_filename);
	}
    }
  FILE_ERR_MESSAGE (s->str);

  g_string_free (s, TRUE);
}

static void
file_dialog_show (GtkWidget *filesel)
{
  menus_set_sensitive (_("<Toolbox>/File/Open"), FALSE);
  menus_set_sensitive (_("<Image>/File/Open"), FALSE);
  menus_set_sensitive (_("<Image>/File/Save"), FALSE);
  menus_set_sensitive (_("<Image>/File/Save as"), FALSE);

  gtk_widget_show (filesel);
}

static int
file_dialog_hide (GtkWidget *filesel)
{
  gtk_widget_hide (filesel);

  menus_set_sensitive (_("<Toolbox>/File/Open"), TRUE);
  menus_set_sensitive (_("<Image>/File/Open"), TRUE);
  menus_set_sensitive (_("<Image>/File/Save"), TRUE);
  menus_set_sensitive (_("<Image>/File/Save as"), TRUE);

  return TRUE;
}

static void
file_overwrite (char *filename, char* raw_filename)
{
  static ActionAreaItem obox_action_items[2] =
  {
    { N_("Yes"), file_overwrite_yes_callback, NULL, NULL },
    { N_("No"), file_overwrite_no_callback, NULL, NULL }
  };

  OverwriteBox *overwrite_box;
  GtkWidget *vbox;
  GtkWidget *label;
  char *overwrite_text;

  overwrite_box = (OverwriteBox *) g_malloc (sizeof (OverwriteBox));
  overwrite_text = g_strdup_printf (_("%s exists, overwrite?"), filename);

  overwrite_box->full_filename = filename;
  overwrite_box->raw_filename = raw_filename;
  overwrite_box->obox = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (overwrite_box->obox), "file_exists", "Gimp");
  gtk_window_set_title (GTK_WINDOW (overwrite_box->obox), _("File Exists!"));
  gtk_window_set_position (GTK_WINDOW (overwrite_box->obox), GTK_WIN_POS_MOUSE);

  gtk_signal_connect (GTK_OBJECT (overwrite_box->obox),
		      "delete_event",
		      (GtkSignalFunc) file_overwrite_delete_callback,
		      overwrite_box);

  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 1);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (overwrite_box->obox)->vbox), vbox, TRUE, TRUE, 0);

  label = gtk_label_new (overwrite_text);
  gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, FALSE, 0);

  obox_action_items[0].user_data = overwrite_box;
  obox_action_items[1].user_data = overwrite_box;
  build_action_area (GTK_DIALOG (overwrite_box->obox), obox_action_items, 2, 0);

  gtk_widget_show (label);
  gtk_widget_show (vbox);
  gtk_widget_show (overwrite_box->obox);

  g_free (overwrite_text);
}

static void
file_overwrite_yes_callback (GtkWidget *w,
			     gpointer   client_data)
{
  OverwriteBox *overwrite_box;
  GImage *gimage;

  overwrite_box = (OverwriteBox *) client_data;

  gtk_widget_destroy (overwrite_box->obox);

  if ((gimage = the_gimage) != NULL &&
      file_save (the_gimage, overwrite_box->full_filename, overwrite_box->raw_filename, 0))
    {
      the_gimage = NULL;
      file_dialog_hide (filesave);
    }
  else
    {
      GString* s;
      GtkWidget *fs;

      fs = filesave;
      s = g_string_new (_("Save failed: "));
      g_string_append (s, overwrite_box->raw_filename);
      FILE_ERR_MESSAGE (s->str);
      g_string_free (s, TRUE);
    }

  g_free (overwrite_box->full_filename);
  g_free (overwrite_box->raw_filename);
  g_free (overwrite_box);
}

static gint
file_overwrite_delete_callback (GtkWidget *w,
				GdkEvent  *e,
				gpointer   client_data)
{
  file_overwrite_no_callback (w, client_data);

  return TRUE;
}

static void
file_overwrite_no_callback (GtkWidget *w,
			    gpointer   client_data)
{
  OverwriteBox *overwrite_box;

  overwrite_box = (OverwriteBox *) client_data;

  gtk_widget_destroy (overwrite_box->obox);
  g_free (overwrite_box->full_filename);
  g_free (overwrite_box->raw_filename);
  g_free (overwrite_box);

  gtk_widget_set_sensitive (GTK_WIDGET(filesave), TRUE);
}

PlugInProcDef*
file_proc_find (GSList *procs,
		char   *filename)
{
  PlugInProcDef *file_proc, *size_matched_proc;
  GSList *all_procs = procs;
  GSList *extensions;
  GSList *prefixes;
  char *extension;
  char *p1, *p2;
  FILE *ifp = NULL;
  int head_size = -2, size_match_count = 0;
  int match_val;
  unsigned char head[256];

  size_matched_proc = NULL;

  extension = strrchr (filename, '.');
  if (extension)
    extension += 1;

  /* At first look for magics */
  while (procs)
    {
      file_proc = procs->data;
      procs = procs->next;

      if (file_proc->magics_list)
        {
          if (head_size == -2)
            {
              head_size = 0;
              if ((ifp = fopen (filename, "rb")) != NULL)
                head_size = fread ((char *)head, 1, sizeof (head), ifp);
            }
          if (head_size >= 4)
            {
              match_val = file_check_magic_list (file_proc->magics_list,
                                                 head_size, head, ifp);
              if (match_val == 2)  /* size match ? */
                { /* Use it only if no other magic matches */
                  size_match_count++;
                  size_matched_proc = file_proc;
                }
              else if (match_val)
                {
                  fclose (ifp);
                  return (file_proc);
                }
            }
        }
    }
  if (ifp) fclose (ifp);
  if (size_match_count == 1) return (size_matched_proc);

  procs = all_procs;
  while (procs)
    {
      file_proc = procs->data;
      procs = procs->next;

      for (prefixes = file_proc->prefixes_list; prefixes; prefixes = prefixes->next)
	{
	  p1 = filename;
	  p2 = (char*) prefixes->data;

	  if (strncmp (filename, prefixes->data, strlen (prefixes->data)) == 0)
	    return file_proc;
	}
     }

  procs = all_procs;
  while (procs)
    {
      file_proc = procs->data;
      procs = procs->next;

      for (extensions = file_proc->extensions_list; extension && extensions; extensions = extensions->next)
	{
	  p1 = extension;
	  p2 = (char*) extensions->data;

	  while (*p1 && *p2)
	    {
	      if (tolower (*p1) != tolower (*p2))
		break;
	      p1 += 1;
	      p2 += 1;
	    }
	  if (!(*p1) && !(*p2))
	    return file_proc;
	}
    }

  return NULL;
}

static void
file_convert_string (char *instr,
		     char *outmem,
		     int   maxmem,
		     int  *nmem)
{
  /* Convert a string in C-notation to array of char */
  guchar *uin = (guchar *) instr;
  guchar *uout = (guchar *) outmem;
  guchar tmp[5], *tmpptr;
  int k;

  while ((*uin != '\0') && ((((char *)uout) - outmem) < maxmem))
    {
      if (*uin != '\\')   /* Not an escaped character ? */
        {
          *(uout++) = *(uin++);
          continue;
        }
      if (*(++uin) == '\0')
        {
          *(uout++) = '\\';
          break;
        }
      switch (*uin)
        {
          case '0':  case '1':  case '2':  case '3': /* octal */
            for (tmpptr = tmp; (tmpptr-tmp) <= 3;)
              {
                *(tmpptr++) = *(uin++);
                if (   (*uin == '\0') || (!isdigit (*uin))
                    || (*uin == '8') || (*uin == '9'))
                  break;
              }
            *tmpptr = '\0';
            sscanf ((char *)tmp, "%o", &k);
            *(uout++) = k;
            break;

          case 'a': *(uout++) = '\a'; uin++; break;
          case 'b': *(uout++) = '\b'; uin++; break;
          case 't': *(uout++) = '\t'; uin++; break;
          case 'n': *(uout++) = '\n'; uin++; break;
          case 'v': *(uout++) = '\v'; uin++; break;
          case 'f': *(uout++) = '\f'; uin++; break;
          case 'r': *(uout++) = '\r'; uin++; break;

          default : *(uout++) = *(uin++); break;
        }
    }
  *nmem = ((char *)uout) - outmem;
}

static int
file_check_single_magic (char   *offset,
                         char   *type,
                         char   *value,
                         int     headsize,
                         guchar *file_head,
                         FILE   *ifp)

{ /* Return values are 0: no match, 1: magic match, 2: size match */
  long offs;
  gulong num_testval, num_operatorval;
  gulong fileval;
  int numbytes, k, c = 0, found = 0;
  char *num_operator_ptr, num_operator, num_test;
  guchar mem_testval[256];

  /* Check offset */
  if (sscanf (offset, "%ld", &offs) != 1) return (0);
  if (offs < 0) return (0);

  /* Check type of test */
  num_operator_ptr = NULL;
  num_operator = '\0';
  num_test = '=';
  if (strncmp (type, "byte", 4) == 0)
    {
      numbytes = 1;
      num_operator_ptr = type+4;
    }
  else if (strncmp (type, "short", 5) == 0)
    {
      numbytes = 2;
      num_operator_ptr = type+5;
    }
  else if (strncmp (type, "long", 4) == 0)
    {
      numbytes = 4;
      num_operator_ptr = type+4;
    }
  else if (strncmp (type, "size", 4) == 0)
    {
      numbytes = 5;
    }
  else if (strcmp (type, "string") == 0)
    {
      numbytes = 0;
    }
  else return (0);

  /* Check numerical operator value if present */
  if (num_operator_ptr && (*num_operator_ptr == '&'))
    {
      if (isdigit (num_operator_ptr[1]))
        {
          if (num_operator_ptr[1] != '0')      /* decimal */
            sscanf (num_operator_ptr+1, "%ld", &num_operatorval);
          else if (num_operator_ptr[2] == 'x') /* hexadecimal */
            sscanf (num_operator_ptr+3, "%lx", &num_operatorval);
          else                                 /* octal */
            sscanf (num_operator_ptr+2, "%lo", &num_operatorval);
          num_operator = *num_operator_ptr;
        }
    }

  if (numbytes > 0)   /* Numerical test ? */
    {
      /* Check test value */
      if ((value[0] == '=') || (value[0] == '>') || (value[0] == '<'))
      {
        num_test = value[0];
        value++;
      }
      if (!isdigit (value[0])) return (0);

      if (value[0] != '0')      /* decimal */
        num_testval = strtol(value, NULL, 10);
      else if (value[1] == 'x') /* hexadecimal */
        num_testval = strtol(value+2, NULL, 16);
      else                      /* octal */
        num_testval = strtol(value+1, NULL, 8);

      fileval = 0;
      if (numbytes == 5)    /* Check for file size ? */
        {
	  struct stat buf;
	  
          if (fstat (fileno (ifp), &buf) < 0) return (0);
          fileval = buf.st_size;
        }
      else if (offs + numbytes <= headsize)  /* We have it in memory ? */
        {
          for (k = 0; k < numbytes; k++)
          fileval = (fileval << 8) | (long)file_head[offs+k];
        }
      else   /* Read it from file */
        {
          if (fseek (ifp, offs, SEEK_SET) < 0) return (0);
          for (k = 0; k < numbytes; k++)
            fileval = (fileval << 8) | (c = getc (ifp));
          if (c == EOF) return (0);
        }
      if (num_operator == '&')
        fileval &= num_operatorval;

      if (num_test == '<')
        found = (fileval < num_testval);
      else if (num_test == '>')
        found = (fileval > num_testval);
      else
        found = (fileval == num_testval);

      if (found && (numbytes == 5)) found = 2;
    }
  else if (numbytes == 0) /* String test */
    {
      file_convert_string ((char *)value, (char *)mem_testval,
                           sizeof (mem_testval), &numbytes);
      if (numbytes <= 0) return (0);

      if (offs + numbytes <= headsize)  /* We have it in memory ? */
        {
          found = (memcmp (mem_testval, file_head+offs, numbytes) == 0);
        }
      else   /* Read it from file */
        {
          if (fseek (ifp, offs, SEEK_SET) < 0) return (0);
          found = 1;
          for (k = 0; found && (k < numbytes); k++)
            {
              c = getc (ifp);
              found = (c != EOF) && (c == (int)mem_testval[k]);
            }
        }
    }

  return (found);
}

static int
file_check_magic_list (GSList *magics_list,
		       int     headsize,
		       guchar *head,
		       FILE   *ifp)

{
  /* Return values are 0: no match, 1: magic match, 2: size match */
  char *offset, *type, *value;
  int and = 0;
  int found = 0, match_val;

  while (magics_list)
    {
      if ((offset = (char *)magics_list->data) == NULL) break;
      if ((magics_list = magics_list->next) == NULL) break;
      if ((type = (char *)magics_list->data) == NULL) break;
      if ((magics_list = magics_list->next) == NULL) break;
      if ((value = (char *)magics_list->data) == NULL) break;
      magics_list = magics_list->next;

      match_val = file_check_single_magic (offset, type, value,
                                           headsize, head, ifp);
      if (and)
	found = found && match_val;
      else
	found = match_val;

      and = (strchr (offset, '&') != NULL);
      if ((!and) && found) return (match_val);
    }
  return (0);
}

static void
file_update_menus (GSList *procs,
		   int     image_type)
{
  PlugInProcDef *file_proc;

  while (procs)
    {
      file_proc = procs->data;
      procs = procs->next;

      if (file_proc->db_info.proc_type != PDB_EXTENSION)
	menus_set_sensitive (gettext(file_proc->menu_path), (file_proc->image_types_val & image_type));
    }
}
