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
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include "appenv.h"
#include "actionarea.h"
#include "gdisplay.h"
#include "general.h"
#include "gimage.h"
#include "fileops.h"
#include "interface.h"
#include "menus.h"
#include "plug_in.h"
#include "procedural_db.h"
#include "gimprc.h"

typedef struct _OverwriteBox OverwriteBox;

struct _OverwriteBox
{
  GtkWidget * obox;
  char *      full_filename;
  char *      raw_filename;
};

static Argument* register_load_handler_invoker (Argument *args);
static Argument* register_magic_load_handler_invoker (Argument *args);
static Argument* register_save_handler_invoker (Argument *args);
static Argument* file_load_invoker             (Argument *args);
static Argument* file_save_invoker             (Argument *args);
static Argument* file_temp_name_invoker        (Argument *args);

static void file_overwrite              (char *filename,
					 char* raw_filename);
static void file_overwrite_yes_callback (GtkWidget *w,
					 gpointer   client_data);
static void file_overwrite_no_callback  (GtkWidget *w,
					 gpointer   client_data);

static gint file_overwrite_delete_callback  (GtkWidget *w,
					     GdkEvent  *e,
					     gpointer   client_data);

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

static PlugInProcDef* file_proc_find         (GSList *procs,
					 char   *filename);
static void      file_update_menus      (GSList *procs,
					 int     image_type);


static GtkWidget *fileload = NULL;
static GtkWidget *filesave = NULL;
static GtkWidget *open_options = NULL;
static GtkWidget *save_options = NULL;

/* Load by extension.
 */
static ProcArg file_load_args[] =
{
  { PDB_INT32, "run_mode", "Interactive, non-interactive." },
  { PDB_STRING, "filename", "The name of the file to load." },
  { PDB_STRING, "raw_filename", "The name entered." },
};

static ProcArg file_load_return_vals[] =
{
  { PDB_IMAGE, "image", "Output image." },
};

static ProcRecord file_load_proc =
{
  "gimp_file_load",
  "Loads a file by extension",
  "This procedure invokes the correct file load handler according to the file's extension and/or prefix.  The name of the file to load is typically a full pathname, and the name entered is what the user actually typed before prepending a directory path.  The reason for this is that if the user types http://www.xcf/~gimp he wants to fetch a URL, and the full pathname will not look like a URL.",
  "Josh MacDonald",
  "Josh MacDonald",
  "1997",
  PDB_INTERNAL,
  3,
  file_load_args,
  1,
  file_load_return_vals,
  { { file_load_invoker } },
};

/* Save by extension.
 */
static ProcArg file_save_args[] =
{
  { PDB_INT32, "run_mode", "Interactive, non-interactive" },
  { PDB_IMAGE, "image", "Input image" },
  { PDB_DRAWABLE, "drawable", "Drawable to save" },
  { PDB_STRING, "filename", "The name of the file to save the image in" },
  { PDB_STRING, "raw_filename", "The name of the file to save the image in" }
};

static ProcRecord file_save_proc =
{
  "gimp_file_save",
  "Saves a file by extension",
  "This procedure invokes the correct file save handler according to the file's extension and/or prefix.  The name of the file to save is typically a full pathname, and the name entered is what the user actually typed before prepending a directory path.  The reason for this is that if the user types http://www.xcf/~gimp he wants to fetch a URL, and the full pathname will not look like a URL.",
  "Josh MacDonald",
  "Josh MacDonald",
  "1997",
  PDB_INTERNAL,
  5,
  file_save_args,
  0,
  NULL,
  { { file_save_invoker } },
};

/* Temp name.
 */

static ProcArg file_temp_name_args[] =
{
  { PDB_STRING, "extension", "The extension the file will have." }
};

static ProcArg file_temp_name_values[] =
{
  { PDB_STRING, "name", "The temp name." }
};

static ProcRecord file_temp_name_proc =
{
  "gimp_temp_name",
  "Generates a unique filename.",
  "Generates a unique filename using the temp path supplied in the user's gimprc.",
  "Josh MacDonald",
  "Josh MacDonald",
  "1997",
  PDB_INTERNAL,
  1,
  file_temp_name_args,
  1,
  file_temp_name_values,
  { { file_temp_name_invoker } },
};

/* Register magic load handler.
 */

static ProcArg register_magic_load_handler_args[] =
{
  { PDB_STRING,
    "procedure_name",
    "the name of the procedure to be used for loading" },
  { PDB_STRING,
    "extensions",
    "comma separated list of extensions this handler can load (ie. \"jpeg,jpg\")" },
  { PDB_STRING,
    "prefixes",
    "comma separated list of prefixes this handler can load (ie. \"http:,ftp:\")" },
  { PDB_STRING,
    "magics",
    "comma separated list of magic file information this handler can load (ie. \"0,string,GIF\")" },
};

static ProcRecord register_magic_load_handler_proc =
{
  "gimp_register_magic_load_handler",
  "Registers a file load handler procedure",
  "Registers a procedural database procedure to be called to load files of a \
particular file format using magic file information.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,
  3,
  register_magic_load_handler_args,
  0,
  NULL,
  { { register_magic_load_handler_invoker } },
};

/* Register load handler.
 */

static ProcArg register_load_handler_args[] =
{
  { PDB_STRING,
    "procedure_name",
    "the name of the procedure to be used for loading" },
  { PDB_STRING,
    "extensions",
    "comma separated list of extensions this handler can load (ie. \"jpeg,jpg\")" },
  { PDB_STRING,
    "prefixes",
    "comma separated list of prefixes this handler can load (ie. \"http:,ftp:\")" },
};

static ProcRecord register_load_handler_proc =
{
  "gimp_register_load_handler",
  "Registers a file load handler procedure",
  "Registers a procedural database procedure to be called to load files of a particular file format.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,
  3,
  register_load_handler_args,
  0,
  NULL,
  { { register_load_handler_invoker } },
};

/* Register save handler.
 */

static ProcArg register_save_handler_args[] =
{
  { PDB_STRING,
    "procedure_name",
    "the name of the procedure to be used for saving" },
  { PDB_STRING,
    "extensions",
    "comma separated list of extensions this handler can save (ie. \"jpeg,jpg\")" },
  { PDB_STRING,
    "prefixes",
    "comma separated list of prefixes this handler can save (ie. \"http:,ftp:\")" },
};

static ProcRecord register_save_handler_proc =
{
  "gimp_register_save_handler",
  "Registers a file save handler procedure",
  "Registers a procedural database procedure to be called to save files in a particular file format.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,
  3,
  register_save_handler_args,
  0,
  NULL,
  { { register_save_handler_invoker } },
};

static GSList *load_procs = NULL;
static GSList *save_procs = NULL;

static PlugInProcDef *load_file_proc = NULL;
static PlugInProcDef *save_file_proc = NULL;

static int image_ID = 0;

static void
file_message_box_close_callback (GtkWidget *w,
				 gpointer   client_data)
{
  GtkFileSelection *fs;

  fs = (GtkFileSelection *) client_data;

  gtk_widget_set_sensitive (GTK_WIDGET (fs), TRUE);
}

void
file_ops_pre_init ()
{
  procedural_db_register (&register_magic_load_handler_proc);
  procedural_db_register (&register_load_handler_proc);
  procedural_db_register (&register_save_handler_proc);
  procedural_db_register (&file_load_proc);
  procedural_db_register (&file_save_proc);
  procedural_db_register (&file_temp_name_proc);
}

void
file_ops_post_init ()
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

static Argument*
register_load_handler_invoker (Argument *args)
{
  Argument fargs[4];

  memcpy ((char *)fargs, (char *)args, 3*sizeof (args[0]));
  fargs[3].arg_type = PDB_STRING;
  fargs[3].value.pdb_pointer = NULL;

  return (register_magic_load_handler_invoker (fargs));
}

static Argument*
register_magic_load_handler_invoker (Argument *args)
{
  Argument *return_args;
  ProcRecord *proc;
  PlugInProcDef* file_proc;
  int success;

  success = FALSE;

  proc = procedural_db_lookup ((char*) args[0].value.pdb_pointer);

  if (proc && ((proc->num_args < 3) ||
	       (proc->num_values < 1) ||
	       (proc->args[0].arg_type != PDB_INT32) ||
	       (proc->args[1].arg_type != PDB_STRING) ||
	       (proc->args[2].arg_type != PDB_STRING) ||
	       (proc->values[0].arg_type != PDB_IMAGE)))
    {
      g_warning ("load handler \"%s\" does not take the standard load handler args",
		 (char*) args[0].value.pdb_pointer);
      goto done;
    }

  file_proc = plug_in_file_handler ((char*) args[0].value.pdb_pointer,
				    (char*) args[1].value.pdb_pointer,
				    (char*) args[2].value.pdb_pointer,
				    (char*) args[3].value.pdb_pointer);

  if (!file_proc)
    {
      g_warning ("attempt to register non-existant load handler \"%s\"",
		 (char*) args[0].value.pdb_pointer);
      goto done;
    }

  load_procs = g_slist_prepend (load_procs, file_proc);

  success = TRUE;

done:

  return_args = procedural_db_return_args (&register_load_handler_proc, success);

  return return_args;
}

static Argument*
register_save_handler_invoker (Argument *args)
{
  Argument *return_args;
  ProcRecord *proc;
  PlugInProcDef* file_proc;
  int success;

  success = FALSE;

  proc = procedural_db_lookup ((char*) args[0].value.pdb_pointer);
  if (proc && ((proc->num_args < 5) ||
	       (proc->args[0].arg_type != PDB_INT32) ||
	       (proc->args[1].arg_type != PDB_IMAGE) ||
	       (proc->args[2].arg_type != PDB_DRAWABLE) ||
	       (proc->args[3].arg_type != PDB_STRING) ||
	       (proc->args[4].arg_type != PDB_STRING)))
    {
      g_warning ("save handler \"%s\" does not take the standard save handler args",
		 (char*) args[0].value.pdb_pointer);
      goto done;
    }

  file_proc = plug_in_file_handler ((char*) args[0].value.pdb_pointer,
				    (char*) args[1].value.pdb_pointer,
				    (char*) args[2].value.pdb_pointer,
				    NULL);

  if (!file_proc)
    {
      g_warning ("attempt to register non-existant save handler \"%s\"",
		 (char*) args[0].value.pdb_pointer);
      goto done;
    }

  save_procs = g_slist_prepend (save_procs, file_proc);

  success = TRUE;

done:
  return_args = procedural_db_return_args (&register_save_handler_proc, success);

  return return_args;
}

void
file_open_callback (GtkWidget *w,
		    gpointer   client_data)
{
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *option_menu;
  GtkWidget *load_menu;
  GDisplay *gdisplay;

  if (!fileload)
    {
      fileload = gtk_file_selection_new ("Load Image");
      gtk_window_position (GTK_WINDOW (fileload), GTK_WIN_POS_MOUSE);
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
    }
  else
    {
      gtk_widget_set_sensitive (GTK_WIDGET (fileload), TRUE);
      if (GTK_WIDGET_VISIBLE (fileload))
	return;

      gtk_file_selection_set_filename (GTK_FILE_SELECTION(fileload), "./");
      gtk_window_set_title (GTK_WINDOW (fileload), "Load Image");
    }

  gdisplay = gdisplay_active ();

  if (!open_options)
    {
      open_options = gtk_frame_new ("Open Options");
      gtk_frame_set_shadow_type (GTK_FRAME (open_options), GTK_SHADOW_ETCHED_IN);

      hbox = gtk_hbox_new (FALSE, 1);
      gtk_container_border_width (GTK_CONTAINER (hbox), 5);
      gtk_container_add (GTK_CONTAINER (open_options), hbox);
      gtk_widget_show (hbox);

      label = gtk_label_new ("Determine file type:");
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
      gtk_widget_show (label);

      option_menu = gtk_option_menu_new ();
      gtk_box_pack_start (GTK_BOX (hbox), option_menu, TRUE, TRUE, 0);
      gtk_widget_show (option_menu);

      menus_get_load_menu (&load_menu, NULL);
      gtk_option_menu_set_menu (GTK_OPTION_MENU (option_menu), load_menu);
      gtk_box_pack_end (GTK_BOX (GTK_FILE_SELECTION (fileload)->main_vbox),
			open_options, FALSE, FALSE, 5);
    }

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
	file_save (gdisplay->gimage->ID, gimage_filename (gdisplay->gimage),
		   prune_filename (gimage_filename(gdisplay->gimage)));
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
      filesave = gtk_file_selection_new ("Save Image");
      gtk_window_set_wmclass (GTK_WINDOW (filesave), "save_image", "Gimp");
      gtk_window_position (GTK_WINDOW (filesave), GTK_WIN_POS_MOUSE);
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

      gtk_file_selection_set_filename (GTK_FILE_SELECTION(filesave), "./");
      gtk_window_set_title (GTK_WINDOW (filesave), "Save Image");
    }

  gdisplay = gdisplay_active ();
  image_ID = gdisplay->gimage->ID;

  if (!save_options)
    {
      save_options = gtk_frame_new ("Save Options");
      gtk_frame_set_shadow_type (GTK_FRAME (save_options), GTK_SHADOW_ETCHED_IN);

      hbox = gtk_hbox_new (FALSE, 1);
      gtk_container_border_width (GTK_CONTAINER (hbox), 5);
      gtk_container_add (GTK_CONTAINER (save_options), hbox);
      gtk_widget_show (hbox);

      label = gtk_label_new ("Determine file type:");
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

int
file_open (char *filename, char* raw_filename)
{
  PlugInProcDef *file_proc;
  ProcRecord *proc;
  Argument *args;
  Argument *return_vals;
  GImage *gimage;
  int gimage_ID;
  int return_val;
  int i;

  file_proc = load_file_proc;
  if (!file_proc)
    file_proc = file_proc_find (load_procs, filename);

  if (!file_proc)
    {
      /* WARNING */
      return FALSE;
    }

  proc = &file_proc->db_info;

  args = g_new (Argument, proc->num_args);
  memset (args, 0, (sizeof (Argument) * proc->num_args));

  for (i = 0; i < proc->num_args; i++)
    args[i].arg_type = proc->args[i].arg_type;

  args[0].value.pdb_int = 0;
  args[1].value.pdb_pointer = filename;
  args[2].value.pdb_pointer = raw_filename;

  return_vals = procedural_db_execute (proc->name, args);
  return_val = (return_vals[0].value.pdb_int == PDB_SUCCESS);
  gimage_ID = return_vals[1].value.pdb_int;

  procedural_db_destroy_args (return_vals, proc->num_values);
  g_free (args);

  if ((gimage = gimage_get_ID (gimage_ID)) != NULL)
    {
      /*  enable & clear all undo steps  */
      gimage_enable_undo (gimage);

      /*  set the image to clean  */
      gimage_clean_all (gimage);

      /*  display the image */
      gdisplay_new (gimage, 0x0101);
    }

  return return_val;
}

int
file_save (int   image_ID,
	   char *filename,
	   char *raw_filename)
{
  PlugInProcDef *file_proc;
  ProcRecord *proc;
  Argument *args;
  Argument *return_vals;
  int return_val;
  GImage *gimage;
  int i;

  if ((gimage = gimage_get_ID (image_ID)) == NULL)
    return FALSE;
  if (gimage_active_drawable (gimage) == NULL)
    return FALSE;

  file_proc = save_file_proc;
  if (!file_proc)
    file_proc = file_proc_find (save_procs, raw_filename);

  if (!file_proc)
    return FALSE;

  proc = &file_proc->db_info;

  args = g_new (Argument, proc->num_args);
  memset (args, 0, (sizeof (Argument) * proc->num_args));

  for (i = 0; i < proc->num_args; i++)
    args[i].arg_type = proc->args[i].arg_type;

  args[0].value.pdb_int = 0;
  args[1].value.pdb_int = image_ID;
  args[2].value.pdb_int = drawable_ID (gimage_active_drawable (gimage));
  args[3].value.pdb_pointer = filename;
  args[4].value.pdb_pointer = raw_filename;

  return_vals = procedural_db_execute (proc->name, args);
  return_val = (return_vals[0].value.pdb_int == PDB_SUCCESS);

  if (return_val)
    {
      /*  set this image to clean  */
      gimage_clean_all (gimage);

      /*  set the image title  */
      gimage_set_filename (gimage, filename);
    }

  g_free (return_vals);
  g_free (args);

  return return_val;
}


static void
file_open_ok_callback (GtkWidget *w,
		       gpointer   client_data)
{
  GtkFileSelection *fs;
  char* filename, *raw_filename;
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
      if (s->str[s->len - 1] != '/')
        {
          g_string_append_c (s, '/');
        }
      gtk_file_selection_set_filename (fs, s->str);
      g_string_free (s, TRUE);
      return;
    }

  gtk_widget_set_sensitive (GTK_WIDGET (fs), FALSE);

  if (file_open (filename, raw_filename))
    {
      file_dialog_hide (client_data);
      gtk_widget_set_sensitive (GTK_WIDGET (fs), TRUE);
      return;
    }

  s = g_string_new ("Open failed: ");

  g_string_append (s, raw_filename);

  message_box (s->str, file_message_box_close_callback, (void *) fs);

  g_string_free (s, TRUE);
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
	  g_string_append_c (s, '/');
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
	  g_string_sprintf (s, "%s is an irregular file (%s)", raw_filename, g_strerror(errno));
	}
    } else {
      gtk_widget_set_sensitive (GTK_WIDGET (fs), FALSE);
      if (file_save (image_ID, filename, raw_filename))
	{
	  file_dialog_hide (client_data);
	  gtk_widget_set_sensitive (GTK_WIDGET (fs), TRUE);
	  return;
	}
      else
	{
	  s = g_string_new ("Save failed: ");
	  g_string_append (s, raw_filename);
	  g_string_append (s, "\nYou might have tried to save an RGB image with");
	  g_string_append (s, "\na plug-in that only supports Indexed or Gray images");
	}
    }
  message_box (s->str, file_message_box_close_callback, (void *) fs);


  g_string_free (s, TRUE);
}

static void
file_dialog_show (GtkWidget *filesel)
{
  menus_set_sensitive ("<Toolbox>/File/Open", FALSE);
  menus_set_sensitive ("<Image>/File/Open", FALSE);
  menus_set_sensitive ("<Image>/File/Save", FALSE);
  menus_set_sensitive ("<Image>/File/Save as", FALSE);

  gtk_widget_show (filesel);
}

static int
file_dialog_hide (GtkWidget *filesel)
{
  gtk_widget_hide (filesel);

  menus_set_sensitive ("<Toolbox>/File/Open", TRUE);
  menus_set_sensitive ("<Image>/File/Open", TRUE);
  menus_set_sensitive ("<Image>/File/Save", TRUE);
  menus_set_sensitive ("<Image>/File/Save as", TRUE);

  return TRUE;
}

static void
file_overwrite (char *filename, char* raw_filename)
{
  static ActionAreaItem obox_action_items[2] =
  {
    { "Yes", file_overwrite_yes_callback, NULL, NULL },
    { "No", file_overwrite_no_callback, NULL, NULL }
  };

  OverwriteBox *overwrite_box;
  GtkWidget *vbox;
  GtkWidget *label;
  char *overwrite_text;

  overwrite_box = (OverwriteBox *) g_malloc (sizeof (OverwriteBox));
  overwrite_text = (char *) g_malloc (strlen (" exists, overwrite?") + strlen (filename) + 1);
  sprintf (overwrite_text, "%s exists, overwrite?", filename);

  overwrite_box->full_filename = filename;
  overwrite_box->raw_filename = raw_filename;
  overwrite_box->obox = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (overwrite_box->obox), "file_exists", "Gimp");
  gtk_window_set_title (GTK_WINDOW (overwrite_box->obox), "File Exists!");
  gtk_window_position (GTK_WINDOW (overwrite_box->obox), GTK_WIN_POS_MOUSE);

  gtk_signal_connect (GTK_OBJECT (overwrite_box->obox),
		      "delete_event",
		      (GtkSignalFunc) file_overwrite_delete_callback,
		      overwrite_box);

  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_border_width (GTK_CONTAINER (vbox), 1);
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

  if (((gimage = gimage_get_ID (image_ID)) != NULL) &&
      file_save (image_ID, overwrite_box->full_filename, overwrite_box->raw_filename))
    {
      image_ID = 0;
      file_dialog_hide (filesave);
    }
  else
    {
      GString* s;

      s = g_string_new ("Save failed: ");

      g_string_append (s, overwrite_box->raw_filename);
      g_string_append (s, "\nYou might have tried to save an RGB image with");
      g_string_append (s, "\na plug-in that only supports Indexed or Gray images");

      message_box (s->str, file_message_box_close_callback, (void *) filesave);
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

static PlugInProcDef*
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

static void file_convert_string (char *instr,
                                 char *outmem,
                                 int maxmem,
                                 int *nmem)
{               /* Convert a string in C-notation to array of char */
  unsigned char *uin = (unsigned char *)instr;
  unsigned char *uout = (unsigned char *)outmem;
  unsigned char tmp[5], *tmpptr;
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
file_check_single_magic (char *offset,
                         char *type,
                         char *value,
                         int headsize,
                         unsigned char *file_head,
                         FILE *ifp)

{ /* Return values are 0: no match, 1: magic match, 2: size match */
  long offs;
  unsigned long num_testval, num_operatorval;
  unsigned long fileval;
  int numbytes, k, c = 0, found = 0;
  char *num_operator_ptr, num_operator, num_test;
  unsigned char mem_testval[256];

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
        {struct stat buf;

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

static int file_check_magic_list (GSList *magics_list,
                                  int headsize,
                                  unsigned char *head,
                                  FILE *ifp)

{ /* Return values are 0: no match, 1: magic match, 2: size match */
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
	menus_set_sensitive (file_proc->menu_path, (file_proc->image_types_val & image_type));
    }
}

static Argument*
file_load_invoker (Argument *args)
{
  PlugInProcDef *file_proc;
  ProcRecord *proc;

  file_proc = file_proc_find (load_procs, args[2].value.pdb_pointer);
  if (!file_proc)
    return procedural_db_return_args (&file_load_proc, FALSE);

  proc = &file_proc->db_info;

  return procedural_db_execute (proc->name, args);
}

static Argument*
file_save_invoker (Argument *args)
{
  Argument *new_args;
  Argument *return_vals;
  PlugInProcDef *file_proc;
  ProcRecord *proc;
  int i;

  file_proc = file_proc_find (save_procs, args[4].value.pdb_pointer);
  if (!file_proc)
    return procedural_db_return_args (&file_save_proc, FALSE);

  proc = &file_proc->db_info;

  new_args = g_new (Argument, proc->num_args);
  memset (new_args, 0, (sizeof (Argument) * proc->num_args));

  for (i = 0; i < proc->num_args; i++)
    new_args[i].arg_type = proc->args[i].arg_type;

  memcpy(new_args, args, (sizeof (Argument) * 5));

  return_vals = procedural_db_execute (proc->name, new_args);
  g_free (new_args);

  return return_vals;
}

static Argument*
file_temp_name_invoker (Argument *args)
{
  static gint id = 0;
  static gint pid;
  Argument *return_args;

  GString *s = g_string_new (NULL);

  if (id == 0)
    pid = getpid();

  g_string_sprintf (s, "%s/gimp_temp.%d%d.%s", temp_path, pid, id++, (char*)args[0].value.pdb_pointer);

  return_args = procedural_db_return_args (&file_temp_name_proc, TRUE);

  return_args[1].value.pdb_pointer = s->str;

  g_string_free (s, FALSE);

  return return_args;
}
