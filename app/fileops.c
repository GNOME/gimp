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

#include <glib.h>

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <errno.h>

#ifdef G_OS_WIN32
#include <direct.h>		/* For _mkdir() */
#define mkdir(path,mode) _mkdir(path)
#include <io.h>
#ifndef S_IWUSR
#define S_IWUSR _S_IWRITE
#endif
#ifndef S_IRUSR
#define S_IRUSR _S_IREAD
#endif
#ifndef S_IWGRP
#define S_IWGRP (_S_IWRITE>>3)
#define S_IWOTH (_S_IWRITE>>6)
#endif
#ifndef S_IRGRP
#define S_IRGRP (_S_IREAD>>3)
#define S_IROTH (_S_IREAD>>6)
#endif
#define uid_t gint
#define gid_t gint
#define geteuid() 0
#define getegid() 0
#endif

#include "appenv.h"
#include "cursorutil.h"
#include "dialog_handler.h"
#include "gdisplay.h"
#include "gimage.h"
#include "gimpcontext.h"
#include "gimpui.h"
#include "fileops.h"
#include "fileopsP.h"
#include "menus.h"
#include "layer.h"
#include "channel.h"
#include "plug_in.h"
#include "procedural_db.h"
#include "gimprc.h"
#include "docindex.h"
#include "undo.h"

#include "libgimp/gimpmath.h"

#include "libgimp/gimpintl.h"


typedef struct _OverwriteData OverwriteData;

struct _OverwriteData
{
  gchar *full_filename;
  gchar *raw_filename;
};

static void    file_overwrite               (gchar         *filename,
					     gchar         *raw_filename);
static void    file_overwrite_callback      (GtkWidget     *widget,
					     gboolean       overwrite,
					     gpointer       data);

static GimpImage * file_open_image          (gchar         *filename,
					     gchar         *raw_filename,
					     gchar         *open_mode,
					     RunModeType    run_mode,
					     gint          *status);

static gint        file_save                (GimpImage     *gimage,
					     gchar         *filename,
					     gchar         *raw_filename,
					     RunModeType    run_mode);

static void    file_open_genbutton_callback (GtkWidget     *widget,
					     gpointer       data);

static void    file_open_clistrow_callback  (GtkWidget     *widget,
					     gint           row);

static void    file_open_ok_callback        (GtkWidget     *widget,
					     gpointer       data);

static void    file_save_ok_callback        (GtkWidget     *widget,
					     gpointer       data);

static void    file_dialog_show             (GtkWidget     *filesel);
static gint    file_dialog_hide             (GtkWidget     *filesel);
static void    file_update_name             (PlugInProcDef *proc,
					     GtkWidget     *filesel);

static void    file_open_type_callback      (GtkWidget     *widget,
					     gpointer       data);
static void    file_save_type_callback      (GtkWidget     *widget,
					     gpointer       data);

static void    file_convert_string          (gchar         *instr,
					     gchar         *outmem,
					     gint           maxmem,
					     gint          *nmem);

static gchar * file_absolute_filename       (gchar         *name);

static gint    file_check_single_magic      (gchar         *offset,
					     gchar         *type,
					     gchar         *value,
					     gint           headsize,
					     guchar        *file_head,
					     FILE          *ifp);

static gint    file_check_magic_list        (GSList        *magics_list,
					     gint           headsize,
					     guchar        *head,
					     FILE          *ifp);

static void    file_update_menus            (GSList        *procs,
					     gint           image_type);

static GSList* clist_to_slist               (GtkCList *file_list);



static GtkWidget  *fileload = NULL;
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

static GimpImage *the_gimage = NULL;

extern GSList *display_list; /* from gdisplay.c */


void
file_ops_pre_init (void)
{
}

void
file_ops_post_init (void)
{
  GimpItemFactoryEntry entry;
  PlugInProcDef *file_proc;
  GSList *tmp;

  load_procs = g_slist_reverse (load_procs);
  save_procs = g_slist_reverse (save_procs);

  for (tmp = load_procs; tmp;  tmp = g_slist_next (tmp))
    {
      gchar *help_page;

      file_proc = tmp->data;

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

  for (tmp = save_procs; tmp; tmp = g_slist_next (tmp))
    {
      gchar *help_page;

      file_proc = tmp->data;

      help_page = g_strconcat ("filters/",
			       g_basename (file_proc->prog),
			       ".html",
			       NULL);
      g_strdown (help_page);

      entry.entry.path            = file_proc->menu_path;
      entry.entry.accelerator     = NULL;
      entry.entry.callback        = file_save_type_callback;
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
    {
      fileload = gtk_file_selection_new (_("Load Image"));
      gtk_window_set_position (GTK_WINDOW (fileload), GTK_WIN_POS_MOUSE);
      gtk_window_set_wmclass (GTK_WINDOW (fileload), "load_image", "Gimp");

      gtk_container_set_border_width (GTK_CONTAINER (fileload), 2);
      gtk_container_set_border_width 
	(GTK_CONTAINER (GTK_FILE_SELECTION (fileload)->button_area), 2);

      dialog_register_fileload (fileload);

      gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION (fileload)->cancel_button),
				 "clicked",
				 GTK_SIGNAL_FUNC (file_dialog_hide),
				 GTK_OBJECT (fileload));
      gtk_signal_connect (GTK_OBJECT (fileload), "delete_event",
			  GTK_SIGNAL_FUNC (file_dialog_hide),
			  NULL);
      gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (fileload)->ok_button),
			  "clicked",
			  GTK_SIGNAL_FUNC (file_open_ok_callback),
			  fileload);
      gtk_quit_add_destroy (1, GTK_OBJECT (fileload));

      gtk_clist_set_selection_mode (GTK_CLIST
				    (GTK_FILE_SELECTION (fileload)->file_list),
				    GTK_SELECTION_EXTENDED);

      /* Catch file-clist clicks so we can update the preview thumbnail */
      gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (fileload)->file_list),
			  "select_row",
			  GTK_SIGNAL_FUNC (file_open_clistrow_callback),
			  fileload);

      /*  Connect the "F1" help key  */
      gimp_help_connect_help_accel (fileload,
				    gimp_standard_help_func,
				    "open/dialogs/file_open.html");
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

  if (!open_options)
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

      menus_get_load_menu (&load_menu, NULL);
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

  file_dialog_show (fileload);
}

void
file_save_callback (GtkWidget *widget,
		    gpointer   data)
{
  GDisplay *gdisplay;

  gdisplay = gdisplay_active ();
  if (! gdisplay)
    return;

  if (! gimage_active_drawable (gdisplay->gimage))
    return;

  /*  Only save if the gimage has been modified  */
  if (!trust_dirty_flag || gdisplay->gimage->dirty != 0)
    {
      if (gdisplay->gimage->has_filename == FALSE)
	{
	  file_save_as_callback (widget, data);
	}
      else
	{
	  gchar *filename;
	  gchar *raw_filename;
	  gint   status;

	  filename     = g_strdup (gimage_filename (gdisplay->gimage));
	  raw_filename = g_basename (filename);
	  
	  status = file_save (gdisplay->gimage,
			      filename,
			      raw_filename,
			      RUN_WITH_LAST_VALS);

	  if (status != PDB_SUCCESS &&
	      status != PDB_CANCEL)
	    {
	      g_message (_("Save failed.\n%s"), filename);
	    }

	  g_free (filename);
	}
    }
}

void
file_save_as_callback (GtkWidget *widget,
		       gpointer   data)
{
  GDisplay *gdisplay;

  gdisplay = gdisplay_active ();
  if (! gdisplay)
    return;

  if (! gimage_active_drawable (gdisplay->gimage))
    return;

  the_gimage = gdisplay->gimage;

  if (!filesave)
    {
      filesave = gtk_file_selection_new (_("Save Image"));
      gtk_window_set_wmclass (GTK_WINDOW (filesave), "save_image", "Gimp");
      gtk_window_set_position (GTK_WINDOW (filesave), GTK_WIN_POS_MOUSE);

      gtk_container_set_border_width (GTK_CONTAINER (filesave), 2);
      gtk_container_set_border_width 
	(GTK_CONTAINER (GTK_FILE_SELECTION (filesave)->button_area), 2);

      gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION (filesave)->cancel_button),
				 "clicked",
				 GTK_SIGNAL_FUNC (file_dialog_hide),
				 GTK_OBJECT (filesave));
      gtk_signal_connect (GTK_OBJECT (filesave), "delete_event",
			  GTK_SIGNAL_FUNC (file_dialog_hide),
			  NULL);
      gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (filesave)->ok_button),
			  "clicked",
			  GTK_SIGNAL_FUNC (file_save_ok_callback),
			  filesave);
      gtk_quit_add_destroy (1, GTK_OBJECT (filesave));

      /*  Connect the "F1" help key  */
      gimp_help_connect_help_accel (filesave,
				    gimp_standard_help_func,
				    "save/dialogs/file_save.html");
    }
  else
    {
      gtk_widget_set_sensitive (GTK_WIDGET (filesave), TRUE);
      if (GTK_WIDGET_VISIBLE (filesave))
	return;

      gtk_window_set_title (GTK_WINDOW (filesave), _("Save Image"));
    }

  gtk_file_selection_set_filename (GTK_FILE_SELECTION(filesave),
                                   gdisplay->gimage->has_filename
                                   ? gimage_filename(gdisplay->gimage)
                                   : "." G_DIR_SEPARATOR_S);
  if (!save_options)
    {
      GtkWidget *frame;
      GtkWidget *hbox;
      GtkWidget *label;
      GtkWidget *option_menu;
      GtkWidget *save_menu;

      save_options = gtk_hbox_new (TRUE, 1);

      frame = gtk_frame_new (_("Save Options"));
      gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
      gtk_box_pack_start (GTK_BOX (save_options), frame, TRUE, TRUE, 4);

      hbox = gtk_hbox_new (FALSE, 4);
      gtk_container_set_border_width (GTK_CONTAINER (hbox), 4);
      gtk_container_add (GTK_CONTAINER (frame), hbox);
      gtk_widget_show (hbox);

      label = gtk_label_new (_("Determine File Type:"));
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
      gtk_widget_show (label);

      option_menu = gtk_option_menu_new ();
      gtk_box_pack_start (GTK_BOX (hbox), option_menu, TRUE, TRUE, 0);
      gtk_widget_show (option_menu);

      menus_get_save_menu (&save_menu, NULL);
      gtk_option_menu_set_menu (GTK_OPTION_MENU (option_menu), save_menu);

      gtk_widget_show (frame);

      /* pack the containing save_options hbox into the save-dialog */
      gtk_box_pack_end (GTK_BOX (GTK_FILE_SELECTION (filesave)->main_vbox),
			save_options, FALSE, FALSE, 0);
    }

  switch (drawable_type (gimage_active_drawable (gdisplay->gimage)))
    {
    case RGB_GIMAGE:
      file_update_menus (save_procs, PLUG_IN_RGB_IMAGE);
      break;
    case RGBA_GIMAGE:
      file_update_menus (save_procs, PLUG_IN_RGBA_IMAGE);
      break;
    case GRAY_GIMAGE:
      file_update_menus (save_procs, PLUG_IN_GRAY_IMAGE);
      break;
    case GRAYA_GIMAGE:
      file_update_menus (save_procs, PLUG_IN_GRAYA_IMAGE);
      break;
    case INDEXED_GIMAGE:
      file_update_menus (save_procs, PLUG_IN_INDEXED_IMAGE);
      break;
    case INDEXEDA_GIMAGE:
      file_update_menus (save_procs, PLUG_IN_INDEXEDA_IMAGE);
      break;
    }

  gtk_widget_show (save_options);

  file_dialog_show (filesave);
}

void
file_revert_callback (GtkWidget *widget,
		      gpointer   data)
{
  GDisplay  *gdisplay;
  GimpImage *gimage;
  gchar     *filename = NULL;
  gint       status;

  gdisplay = gdisplay_active ();
  if (!gdisplay || !gdisplay->gimage)
    return;

  if (gdisplay->gimage->has_filename == FALSE)
    {
      g_message (_("Revert failed.\n"
		   "No filename associated with this image."));
    }
  else
    {
      filename = gimage_filename (gdisplay->gimage);

      gimage = file_open_image (filename, filename, _("Revert"),
				RUN_INTERACTIVE, &status);

      if (gimage != NULL)
	{
	  undo_free (gimage);
	  gdisplays_reconnect (gdisplay->gimage, gimage);
	  gimp_image_clean_all (gimage);
	}
      else if (status != PDB_CANCEL)
	{
	  g_message (_("Revert failed.\n%s"), filename);
	}
    }
}

void
file_open_by_extension_callback (GtkWidget *widget,
				 gpointer   data)
{
  load_file_proc = NULL;
}

void
file_save_by_extension_callback (GtkWidget *widget,
				 gpointer   data)
{
  save_file_proc = NULL;
}

static void
file_update_name (PlugInProcDef *proc,
		  GtkWidget     *filesel)
{
  if (proc->extensions_list)
    {
      gchar *text;
      gchar *last_dot;
      GString *s;

      text = gtk_entry_get_text (GTK_ENTRY (GTK_FILE_SELECTION (filesel)->selection_entry));
      last_dot = strrchr (text, '.');

      if (last_dot == text || !text[0])
	return;

      s = g_string_new (text);

      if (last_dot)
	g_string_truncate (s, last_dot-text);

      g_string_append (s, ".");
      g_string_append (s, (gchar *) proc->extensions_list->data);

      gtk_entry_set_text (GTK_ENTRY (GTK_FILE_SELECTION (filesel)->selection_entry), s->str);

      g_string_free (s, TRUE);
    }
}

static void
file_open_type_callback (GtkWidget *widget,
			 gpointer   data)
{
  PlugInProcDef *proc = (PlugInProcDef *) data;

  file_update_name (proc, fileload);

  load_file_proc = proc;
}

static void
file_save_type_callback (GtkWidget *widget,
			 gpointer   data)
{
  PlugInProcDef *proc = (PlugInProcDef *) data;

  file_update_name (proc, filesave);

  save_file_proc = proc;
}

static GimpImage *
file_open_image (gchar       *filename,
		 gchar       *raw_filename,
		 gchar       *open_mode,
		 RunModeType  run_mode,
		 gint        *status)
{
  PlugInProcDef *file_proc;
  ProcRecord *proc;
  Argument   *args;
  Argument   *return_vals;
  gint gimage_id;
  gint i;
  struct stat statbuf;

  *status = PDB_CANCEL;  /* inhibits error messages by caller */

  file_proc = load_file_proc;

  if (!file_proc)
    file_proc = file_proc_find (load_procs, filename);

  if (!file_proc)
    {
      /*  no errors when making thumbnails  */
      if (run_mode == RUN_INTERACTIVE)
	g_message (_("%s failed.\n"
		     "%s: Unknown file type."),
		   open_mode, filename);

      return NULL;
    }

  /* check if we are opening a file */
  if (stat (filename, &statbuf) == 0)
    {
      uid_t euid;
      gid_t egid;

      if (! (statbuf.st_mode & S_IFREG))
	{
	  /*  no errors when making thumbnails  */
	  if (run_mode == RUN_INTERACTIVE)
	    g_message (_("%s failed.\n"
			 "%s is not a regular file."),
		       open_mode, filename);

	  return NULL;
	}

      euid = geteuid ();
      egid = getegid ();

      if (! ((statbuf.st_mode & S_IRUSR) ||

	     ((statbuf.st_mode & S_IRGRP) &&
	      (statbuf.st_uid != euid)) ||

	     ((statbuf.st_mode & S_IROTH) &&
	      (statbuf.st_uid != euid) &&
	      (statbuf.st_gid != egid))))
	{
	  /*  no errors when making thumbnails  */
	  if (run_mode == RUN_INTERACTIVE)
	    g_message (_("%s failed.\n"
			 "%s: Permission denied."),
		       open_mode, filename);

	  return NULL;
	}
    }

  proc = &file_proc->db_info;

  args = g_new0 (Argument, proc->num_args);

  for (i = 0; i < proc->num_args; i++)
    args[i].arg_type = proc->args[i].arg_type;

  args[0].value.pdb_int     = run_mode;
  args[1].value.pdb_pointer = filename;
  args[2].value.pdb_pointer = raw_filename;

  return_vals = procedural_db_execute (proc->name, args);

  *status   = return_vals[0].value.pdb_int;
  gimage_id = return_vals[1].value.pdb_int;

  procedural_db_destroy_args (return_vals, proc->num_values);
  g_free (args);

  if (*status == PDB_SUCCESS && gimage_id != -1)
    {
      layer_invalidate_previews (gimage_get_ID (gimage_id));
      channel_invalidate_previews (gimage_get_ID (gimage_id));

      return pdb_id_to_image (gimage_id);
    }

  return NULL;
}

gint
file_open (gchar *filename,
	   gchar *raw_filename)
{
  GimpImage *gimage;
  GDisplay  *gdisplay;
  gchar     *absolute;
  gint       status;

  if ((gimage = file_open_image (filename,
				 raw_filename,
				 _("Open"),
				 RUN_INTERACTIVE,
				 &status)) != NULL)
    {
      /* enable & clear all undo steps */
      gimage_enable_undo (gimage);

      /* set the image to clean  */
      gimage_clean_all (gimage);

      /* display the image */
      gdisplay = gdisplay_new (gimage, 0x0101);

      /* always activate the first display */
      if (g_slist_length (display_list) == 1)
	gimp_context_set_display (gimp_context_get_user (), gdisplay);

      absolute = file_absolute_filename (filename);
      document_index_add (absolute);
      menus_last_opened_add (absolute);
      g_free (absolute);
    }

  return status;
}

TempBuf *
make_thumb_tempbuf (GimpImage *gimage)
{
  gint w, h;

  if (gimage->width<=80 && gimage->height<=60)
    {
      w = gimage->width;
      h = gimage->height;
    }
  else
    {
      /* Ratio molesting to fit within .xvpic thumbnail size limits */
      if (60 * gimage->width < 80 * gimage->height)
	{
	  h = 60;
	  w = (60 * gimage->width) / gimage->height;
	  if (w == 0)
	    w = 1;
	}
      else
	{
	  w = 80;
	  h = (80 * gimage->height) / gimage->width;
	  if (h == 0)
	    h = 1;
	}
    }

  /*printf("tn: %d x %d -> ", w, h);fflush(stdout);*/

  return (gimp_image_composite_preview (gimage, GRAY_CHANNEL, w, h));
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

gboolean
file_save_thumbnail (GimpImage  *gimage,
		     const char *full_source_filename,
		     TempBuf    *tempbuf)
{
  gint i,j;
  gint w,h;
  guchar *tbd;
  gchar* pathname;
  gchar* filename;
  gchar* xvpathname;
  gchar* thumbnailname;
  GimpImageBaseType basetype;
  FILE *fp;
  struct stat statbuf;

  if (stat (full_source_filename, &statbuf) != 0)
    {
      return FALSE;
    }

  /* just for debugging 
   *  if (gimp_image_preview_valid (gimage, GRAY_CHANNEL))
   *   {
   *     g_print ("(incidentally, gimage already has a valid preview - %dx%d)\n",
   *	         gimage->comp_preview->width,
   *	         gimage->comp_preview->height);
   *   }
   */

  pathname = g_dirname (full_source_filename);
  filename = g_basename (full_source_filename); /* Don't free! */

  xvpathname = g_strconcat (pathname, G_DIR_SEPARATOR_S, ".xvpics",
			    NULL);

  thumbnailname = g_strconcat (xvpathname, G_DIR_SEPARATOR_S,
			       filename,
			       NULL);

  tbd = temp_buf_data (tempbuf);

  w = tempbuf->width;
  h = tempbuf->height;
  /*printf("tn: %d x %d\n", w, h);fflush(stdout);*/

  mkdir (xvpathname, 0755);

  fp = fopen (thumbnailname, "wb");
  g_free (pathname);
  g_free (xvpathname);
  g_free (thumbnailname);

  if (fp)
    {
      basetype = gimp_image_base_type(gimage);

      fprintf (fp,
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
      return FALSE;
    }

  return TRUE;
}

static gint
file_save (GimpImage   *gimage,
	   gchar       *filename,
	   gchar       *raw_filename,
           RunModeType  run_mode)
{
  PlugInProcDef *file_proc;
  ProcRecord *proc;
  Argument   *args;
  Argument   *return_vals;
  gint status;
  gint i;
  struct stat statbuf;

  if (gimage_active_drawable (gimage) == NULL)
    return PDB_EXECUTION_ERROR;

  file_proc = gimage_get_save_proc (gimage);

  if (!file_proc)
    file_proc = file_proc_find (save_procs, raw_filename);

  if (!file_proc)
    {
      g_message (_("Save failed.\n"
		   "%s: Unknown file type."),
		 filename);

      return PDB_CANCEL;  /* inhibits error messages by caller */
    }

  /* check if we are saving to a file */
  if (stat (filename, &statbuf) == 0)
    {
      uid_t euid;
      gid_t egid;

      if (! (statbuf.st_mode & S_IFREG))
        {
	  g_message (_("Save failed.\n"
		       "%s is not a regular file."),
		     filename);

          return PDB_CANCEL;  /* inhibits error messages by caller */
        }

      euid = geteuid ();
      egid = getegid ();

      if (! ((statbuf.st_mode & S_IWUSR) ||

             ((statbuf.st_mode & S_IWGRP) &&
              (statbuf.st_uid != euid)) ||

             ((statbuf.st_mode & S_IWOTH) &&
              (statbuf.st_uid != euid) &&
              (statbuf.st_gid != egid))))
        {
	  g_message (_("Save failed.\n"
		       "%s: Permission denied."),
		     filename);

          return PDB_CANCEL;  /* inhibits error messages by caller */
        }
    }

  /* ref the image, so it can't get deleted during save */
  gtk_object_ref (GTK_OBJECT (gimage));

  proc = &file_proc->db_info;

  args = g_new0 (Argument, proc->num_args);

  for (i = 0; i < proc->num_args; i++)
    args[i].arg_type = proc->args[i].arg_type;

  args[0].value.pdb_int     = run_mode;
  args[1].value.pdb_int     = pdb_image_to_id (gimage);
  args[2].value.pdb_int     = drawable_ID (gimage_active_drawable (gimage));
  args[3].value.pdb_pointer = filename;
  args[4].value.pdb_pointer = raw_filename;

  return_vals = procedural_db_execute (proc->name, args);

  status = return_vals[0].value.pdb_int;

  if (status == PDB_SUCCESS)
    {
      /*  set this image to clean  */
      gimage_clean_all (gimage);

      /* these calls must come before the call to gimage_set_filename */
      document_index_add (filename);
      menus_last_opened_add (filename);

      /*  use the same plug-in for this image next time  */
      /* DISABLED - gets stuck on first saved format... needs
	 attention --Adam */
      /* gimage_set_save_proc(gimage, file_proc); */

      /* Write a thumbnail for the saved image, where appropriate */
      switch (thumbnail_mode)
	{
	case 0:
	  break;
	default:
	  {
	    TempBuf *tempbuf;

	    tempbuf = make_thumb_tempbuf (gimage);
	    file_save_thumbnail (gimage, filename, tempbuf);
	  }
	}

      /*  set the image title  */
      gimp_image_set_filename (gimage, filename);
      /* note: 'filename' may have been free'd by above call! */
    }

  g_free (return_vals);
  g_free (args);

  gtk_object_unref (GTK_OBJECT (gimage));

  return status;
}

/* The readXVThumb function source may be re-used under
   the XFree86-style license. <adam@gimp.org> */
guchar *
readXVThumb (const gchar  *fnam,
	     gint         *w,
	     gint         *h,
	     gchar       **imginfo /* caller frees if != NULL */)
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
    return NULL;

  fread (P7_buf, 6, 1, fp);

  if (strncmp(P7_buf, P7_332, 6)!=0)
    {
      g_warning ("Thumbnail doesn't have the 'P7 332' header.");
      fclose (fp);
      return NULL;
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
      fclose (fp);
      return NULL;
    }

  sscanf(linebuf, "%d %d %d\n", w, h, &twofivefive);

  if (twofivefive!=255)
    {
      g_warning ("Thumbnail is of funky depth.");
      fclose (fp);
      return NULL;
    }

  if ((*w)<1 || (*h)<1 || (*w)>80 || (*h)>60)
    {
      g_warning ("Thumbnail size bad.  Corrupted?");
      fclose (fp);
      return NULL;
    }

  buf = g_malloc ((*w) * (*h));

  fread (buf, (*w) * (*h), 1, fp);

  fclose (fp);
  
  return buf;
}

/* don't call with preview_fullname as parameter!  will be clobbered! */
static void
set_preview (const gchar *fullfname,
	     guchar      *RGB_source,
	     gint         RGB_w,
	     gint         RGB_h)
{
  guchar *thumb_rgb;
  guchar *raw_thumb;
  gint    tnw,tnh, i;
  gchar  *pname;
  gchar  *fname;
  gchar  *tname;
  gchar  *imginfo = NULL;
  struct stat file_stat;
  struct stat thumb_stat;
  gboolean thumb_may_be_outdated = FALSE;
  gboolean show_generate_label = TRUE;

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

  if (preview_fullname)
    {
      g_free (preview_fullname);
    }
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
  gchar *full_filename = NULL;
  gchar *filedirname;
  struct stat buf;
  gint err;

  fs = GTK_FILE_SELECTION (data);

  if (!preview_fullname)
    {
      g_warning ("Tried to generate thumbnail for NULL filename.");
      return;
    }

  gimp_add_busy_cursors ();
  gtk_widget_set_sensitive (GTK_WIDGET (fileload), FALSE);

  /* new mult-file preview make: */  
  {
    GSList *list;

    /* Have to read the clist before touching anything else */

    list= clist_to_slist(GTK_CLIST(fs->file_list));

    /* Find a real base directory for the multiple selection */

    gtk_file_selection_set_filename (fs, "");
    filedirname= gtk_file_selection_get_filename (fs);

    while(list)
      {
        full_filename = g_strconcat (filedirname, G_DIR_SEPARATOR_S,
                                 (char *) list->data, NULL);

	err = stat (full_filename, &buf);

	if (! (err == 0 && (buf.st_mode & S_IFDIR)))
	  { /* Is not directory. */
	    gint dummy;

	    gimage_to_be_thumbed = file_open_image (full_filename,
						    list->data,
						    NULL,
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

		gimage_delete (gimage_to_be_thumbed);

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
        g_free(list->data);
        list= g_slist_next(list);
      }
    g_slist_free(list);
    list= NULL;
  }

  gtk_widget_set_sensitive (GTK_WIDGET (fileload), TRUE);
  gimp_remove_busy_cursors (NULL);
}

static void
file_open_ok_callback (GtkWidget *widget,
		       gpointer   data)
{
  GtkFileSelection *fs;
  gchar *full_filename, *raw_filename;
  gchar *filedirname;
  struct stat buf;
  gint err;
  gint status;

  fs = GTK_FILE_SELECTION (data);
  full_filename = gtk_file_selection_get_filename (fs);
  raw_filename = gtk_entry_get_text (GTK_ENTRY(fs->selection_entry));

  g_assert (full_filename && raw_filename);

  if (strlen (raw_filename) == 0)
    return;

  err = stat (full_filename, &buf);

  if (err == 0 && (buf.st_mode & S_IFDIR))
    {
      GString *s = g_string_new (full_filename);
      if (s->str[s->len - 1] != G_DIR_SEPARATOR)
	{
	  g_string_append_c (s, G_DIR_SEPARATOR);
	}
      gtk_file_selection_set_filename (fs, s->str);
      g_string_free (s, TRUE);
      return;
    }

  gtk_widget_set_sensitive (GTK_WIDGET (fs), FALSE);

  if (err) /* e.g. http://server/filename.jpg */
    full_filename = raw_filename;

  status = file_open (full_filename, raw_filename);

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

    list= clist_to_slist(GTK_CLIST(fs->file_list));

    /* Find a real base directory for the multiple selection */

    raw_filename= g_strdup(raw_filename);
    gtk_file_selection_set_filename (fs, "");
    filedirname= gtk_file_selection_get_filename (fs);

    while(list)
      {
        full_filename = g_strconcat (filedirname, G_DIR_SEPARATOR_S,
                                 (char *) list->data, NULL);

        if (strcmp (list->data, raw_filename))
          { /* don't load current selection twice */

            err = stat (full_filename, &buf);

            if (! (err == 0 && (buf.st_mode & S_IFDIR)))
              { /* Is not directory. */

                status = file_open (full_filename, (char *) list->data);

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
     
        g_free(full_filename);
        g_free(list->data);
        list= g_slist_next(list);
      }
    g_slist_free(list);
    list= NULL;
  }
    
  gtk_file_selection_set_filename (fs, raw_filename);
  g_free(raw_filename);
  gtk_widget_set_sensitive (GTK_WIDGET (fs), TRUE);

}

static GSList *
clist_to_slist (GtkCList *file_list)
{
  GSList *list = NULL;
  GList *row = file_list->row_list;
  gint   rownum = 0;
  gchar *temp;
  
  while (row)
    {
      if (GTK_CLIST_ROW (row)->state == GTK_STATE_SELECTED)
        {
          if (gtk_clist_get_cell_type (file_list, rownum, 0) == GTK_CELL_TEXT)
            {
              gtk_clist_get_text (file_list, rownum, 0, &temp);
              list = g_slist_prepend(list, strdup(temp));
            }
        }
      rownum++;
      row = g_list_next (row);
    }
  return list;
}


/* Set "gimage"s save handler to "save_proc", then save the image.  
 * Hide the dialog if all went well, otherwise make the user knows an
 * error happened and leave the dialog up.  Make sure it's sensitive.
 */
static void
file_save_with_proc (GImage *gimage,
		     gchar *full_filename,
		     gchar *raw_filename,
		     PlugInProcDef *save_proc)
{
    gint status = PDB_EXECUTION_ERROR;

    if (gimage != NULL)
    {
	gimage_set_save_proc (gimage, save_proc);
	status = file_save (gimage,
			    full_filename,
			    raw_filename,
			    RUN_INTERACTIVE);

	/* hide the file save dialog on success */
	if (status == PDB_SUCCESS)
	    file_dialog_hide (filesave);
    }

    /* If there was an error but file_save() didn't print an error
     * message, then we'd better. */
    if (status != PDB_SUCCESS &&
	status != PDB_CANCEL)
    {
	g_message (_("Save failed.\n%s"), full_filename);
    }

    /* always make file save dialog sensitive */
    gtk_widget_set_sensitive (GTK_WIDGET (filesave), TRUE);
}


static void
file_save_ok_callback (GtkWidget *widget,
		       gpointer   data)
{
  GtkFileSelection *fs;
  gchar       *filename;
  gchar       *raw_filename;
  struct stat  buf;
  gint         err;

  fs = GTK_FILE_SELECTION (data);
  filename = gtk_file_selection_get_filename (fs);
  raw_filename = gtk_entry_get_text (GTK_ENTRY(fs->selection_entry));

  g_assert (filename && raw_filename);

  err = stat (filename, &buf);

  if (err == 0)
    {
      if (buf.st_mode & S_IFDIR)
	{
	  GString *s = g_string_new (filename);
	  g_string_append_c (s, G_DIR_SEPARATOR);
	  gtk_file_selection_set_filename (fs, s->str);
	  g_string_free (s, TRUE);
	}
      else
	{
	  gtk_widget_set_sensitive (GTK_WIDGET (fs), FALSE);
	  file_overwrite (g_strdup (filename), g_strdup (raw_filename));
	}
    }
  else
    {
      gtk_widget_set_sensitive (GTK_WIDGET (fs), FALSE);

      file_save_with_proc (the_gimage, filename, raw_filename, save_file_proc);
    }
}

static void
file_dialog_show (GtkWidget *filesel)
{
  menus_set_sensitive ("<Toolbox>/File/Open...", FALSE);
  menus_set_sensitive ("<Image>/File/Open...", FALSE);
  menus_set_sensitive ("<Image>/File/Save", FALSE);
  menus_set_sensitive ("<Image>/File/Save As...", FALSE);

  gtk_widget_grab_focus (GTK_FILE_SELECTION (filesel)->selection_entry);
  gtk_widget_show (filesel);
}

static int
file_dialog_hide (GtkWidget *filesel)
{
  gimp_dialog_hide (filesel);
  
  menus_set_sensitive ("<Toolbox>/File/Open...", TRUE);
  menus_set_sensitive ("<Image>/File/Open...", TRUE);

  if (gdisplay_active ())
    {
      menus_set_sensitive ("<Image>/File/Save", TRUE);
      menus_set_sensitive ("<Image>/File/Save As...", TRUE);
    }

  return TRUE;
}

static void
file_overwrite (gchar *filename,
		gchar *raw_filename)
{
  OverwriteData *overwrite_data;
  GtkWidget *query_box;
  gchar     *overwrite_text;

  overwrite_data = g_new (OverwriteData, 1);
  overwrite_data->full_filename = filename;
  overwrite_data->raw_filename  = raw_filename;

  overwrite_text = g_strdup_printf (_("%s exists, overwrite?"), filename);

  query_box = gimp_query_boolean_box (_("File Exists!"),
				      gimp_standard_help_func,
				      "save/file_exists.html",
				      FALSE,
				      overwrite_text,
				      _("Yes"), _("No"),
				      NULL, NULL,
				      file_overwrite_callback,
				      overwrite_data);

  g_free (overwrite_text);

  gtk_widget_show (query_box);
}

static void
file_overwrite_callback (GtkWidget *widget,
			 gboolean   overwrite,
			 gpointer   data)
{
  OverwriteData *overwrite_data;

  overwrite_data = (OverwriteData *) data;

  if (overwrite)
  {
      file_save_with_proc (the_gimage,
			   overwrite_data->full_filename,
			   overwrite_data->raw_filename,
			   save_file_proc);
  }

  g_free (overwrite_data->full_filename);
  g_free (overwrite_data->raw_filename);
  g_free (overwrite_data);
}

static PlugInProcDef *
file_proc_find_by_name (GSList   *procs,
		        gchar    *filename,
		        gboolean  skip_magic)
{
  GSList *p;
  gchar  *ext = strrchr (filename, '.');

  if (ext)
    ext++;

  for (p = procs; p; p = g_slist_next (p))
    {
      PlugInProcDef *proc = p->data;
      GSList        *prefixes;

      if (skip_magic && proc->magics_list)
	continue;

      for (prefixes = proc->prefixes_list;
	   prefixes;
	   prefixes = g_slist_next (prefixes))
	{
	  if (strncmp (filename, prefixes->data, strlen (prefixes->data)) == 0)
	    return proc;
	}
     }

  for (p = procs; p; p = g_slist_next (p))
    {
      PlugInProcDef *proc = p->data;
      GSList        *extensions;

      for (extensions = proc->extensions_list;
	   ext && extensions;
	   extensions = g_slist_next (extensions))
	{
	  gchar *p1 = ext;
	  gchar *p2 = (gchar *) extensions->data;

          if (skip_magic && proc->magics_list)
	    continue;

	  while (*p1 && *p2)
	    {
	      if (tolower (*p1) != tolower (*p2))
		break;

	      p1++;
	      p2++;
	    }

	  if (!(*p1) && !(*p2))
	    return proc;
	}
    }

  return NULL;
}

PlugInProcDef *
file_proc_find (GSList *procs,
		gchar  *filename)
{
  PlugInProcDef *file_proc;
  PlugInProcDef *size_matched_proc;
  GSList *all_procs = procs;
  FILE   *ifp = NULL;
  gint    head_size        = -2;
  gint    size_match_count = 0;
  gint    match_val;
  guchar  head[256];

  size_matched_proc = NULL;

  /* First, check magicless prefixes/suffixes */
  if ( (file_proc = file_proc_find_by_name (all_procs, filename, TRUE)) != NULL)
    return file_proc;

  /* Then look for magics */
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
                head_size = fread ((gchar *) head, 1, sizeof (head), ifp);
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
  if (size_match_count == 1)
    return (size_matched_proc);

  /* As a last ditch, try matching by name */
  return file_proc_find_by_name (all_procs, filename, FALSE);
}

static void
file_convert_string (gchar *instr,
		     gchar *outmem,
		     gint   maxmem,
		     gint  *nmem)
{
  /* Convert a string in C-notation to array of char */
  guchar *uin = (guchar *) instr;
  guchar *uout = (guchar *) outmem;
  guchar  tmp[5], *tmpptr;
  gint    k;

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
  *nmem = ((gchar *) uout) - outmem;
}

static gchar *
file_absolute_filename (gchar *name)
{
  PlugInProcDef *proc;
  GSList *procs;
  GSList *prefixes;
  gchar  *absolute;
  gchar  *current;

  g_return_val_if_fail (name != NULL, NULL);

  /*  check for prefixes like http or ftp  */
  for (procs = load_procs; procs; procs = g_slist_next (procs))
    {
      proc = (PlugInProcDef *)procs->data;

      for (prefixes = proc->prefixes_list;
	   prefixes;
	   prefixes = g_slist_next (prefixes))
	{
	  if (strncmp (name, prefixes->data, strlen (prefixes->data)) == 0)
	    return g_strdup (name);
	}
     }
 
  if (g_path_is_absolute (name))
    return g_strdup (name);
  
  current = g_get_current_dir ();
  absolute = g_strconcat (current, G_DIR_SEPARATOR_S, name, NULL);
  g_free (current);

  return absolute;
}

static gint
file_check_single_magic (gchar  *offset,
                         gchar  *type,
                         gchar  *value,
                         gint    headsize,
                         guchar *file_head,
                         FILE   *ifp)

{
  /* Return values are 0: no match, 1: magic match, 2: size match */
  glong   offs;
  gulong  num_testval, num_operatorval;
  gulong  fileval;
  gint    numbytes, k, c = 0, found = 0;
  gchar  *num_operator_ptr, num_operator, num_test;
  guchar  mem_testval[256];

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

      /* 
       * to anybody reading this: is strtol's parsing behaviour (e.g. "0x" prefix)
       * broken on some systems or why do we do the base detection ourselves?
       * */
      if (value[0] != '0')      /* decimal */
        num_testval = strtol(value, NULL, 10);
      else if (value[1] == 'x') /* hexadecimal */
        num_testval = (unsigned long)strtoul(value+2, NULL, 16);
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
		       gint    headsize,
		       guchar *head,
		       FILE   *ifp)

{
  /* Return values are 0: no match, 1: magic match, 2: size match */
  gchar *offset, *type, *value;
  gint and = 0;
  gint found = 0, match_val;

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
		   gint    image_type)
{
  PlugInProcDef *file_proc;

  while (procs)
    {
      file_proc = procs->data;
      procs = procs->next;

      if (file_proc->db_info.proc_type != PDB_EXTENSION)
	menus_set_sensitive (file_proc->menu_path,
			     (file_proc->image_types_val & image_type));
    }
}
