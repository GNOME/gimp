/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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
#ifndef __PLUG_IN_H__
#define __PLUG_IN_H__


#include <sys/types.h>
#include <gtk/gtk.h>

#include "procedural_db.h"
#include "gimpprogress.h"
#include "drawable.h"

#define WRITE_BUFFER_SIZE  512

#define RGB_IMAGE       0x01
#define GRAY_IMAGE      0x02
#define INDEXED_IMAGE   0x04
#define RGBA_IMAGE      0x08
#define GRAYA_IMAGE     0x10
#define INDEXEDA_IMAGE  0x20



typedef enum
{
  RUN_INTERACTIVE    = 0,
  RUN_NONINTERACTIVE = 1,
  RUN_WITH_LAST_VALS = 2
} RunModeType;


struct _PlugIn
{
  unsigned int open : 1;                 /* Is the plug-in open */
  unsigned int destroy : 1;              /* Should the plug-in by destroyed */
  unsigned int query : 1;                /* Are we querying the plug-in? */
  unsigned int synchronous : 1;          /* Is the plug-in running synchronously or not */
  unsigned int recurse : 1;              /* Have we called 'gtk_main' recursively? */
  unsigned int busy : 1;                 /* Is the plug-in busy with a temp proc? */
  pid_t pid;                             /* Plug-ins process id */
  char *args[7];                         /* Plug-ins command line arguments */

  GIOChannel *my_read, *my_write;        /* App's read and write channels */
  GIOChannel *his_read, *his_write;      /* Plug-in's read and write channels */
#ifdef NATIVE_WIN32
  guint his_thread_id;			 /* Plug-in's thread ID */
  int his_read_fd;			 /* Plug-in's read pipe fd */
#endif

  guint32 input_id;                      /* Id of input proc */

  char write_buffer[WRITE_BUFFER_SIZE];  /* Buffer for writing */
  int write_buffer_index;                /* Buffer index for writing */

  GSList *temp_proc_defs;                /* Temporary procedures  */

  gimp_progress *progress;               /* Progress dialog */

  gpointer user_data;                    /* Handle for hanging data onto */
};

struct _PlugInDef
{
  char *prog;
  GSList *proc_defs;
  time_t mtime;
  int query;
};

struct _PlugInProcDef
{
  char *prog;
  char *menu_path;
  char *accelerator;
  char *extensions;
  char *prefixes;
  char *magics;
  char *image_types;
  int   image_types_val;
  ProcRecord db_info;
  GSList *extensions_list;
  GSList *prefixes_list;
  GSList *magics_list;
  time_t mtime;
};


/* Initialize the plug-ins */
void plug_in_init (void);

/* Kill all running plug-ins */
void plug_in_kill (void);

/* Add a plug-in to the list of valid plug-ins
 *  and query the plug-in for information if
 *  necessary.
 */
void plug_in_add (char *name,
		  char *menu_path,
		  char *accelerator);

/* Get the "image_types" the plug-in works on.
 */
char* plug_in_image_types (char *name);

/* Add in the file load/save handler fields procedure.
 */
PlugInProcDef* plug_in_file_handler (char *name,
				     char *extensions,
				     char *prefixes,
				     char *magics);

/* Add a plug-in definition.
 */
void plug_in_def_add (PlugInDef *plug_in_def);

/* Retrieve a plug-ins menu path
 */
char* plug_in_menu_path (char *name);

/* Create a new plug-in structure */
PlugIn* plug_in_new (char *name);

/* Destroy a plug-in structure. This will close the plug-in
 *  first if necessary.
 */
void plug_in_destroy (PlugIn *plug_in);

/* Open a plug-in. This cause the plug-in to run.
 * If returns 1, you must destroy the plugin.  If returns 0 you
 * may not destroy the plugin.
 */
int plug_in_open (PlugIn *plug_in);

/* Close a plug-in. This kills the plug-in and releases
 *  its resources.
 */
void plug_in_close (PlugIn *plug_in,
		    int     kill_it);

/* Run a plug-in as if it were a procedure database procedure */
Argument* plug_in_run (ProcRecord *proc_rec,
		       Argument   *args,
		       int         argc,
		       int         synchronous,
		       int         destroy_values,
		       int         gdisp_ID);

/* Run the last plug-in again with the same arguments. Extensions
 *  are exempt from this "privelege".
 */
void plug_in_repeat (int with_interface);

/* Set the sensitivity for plug-in menu items based on the image
 * type.
 */
void plug_in_set_menu_sensitivity (GimpImageType type);

/* Register an internal plug-in.  This is for file load-save
 * handlers, which are organized around the plug-in data structure.
 * This could all be done a little better, but oh well.  -josh
 */
void plug_in_add_internal (PlugInProcDef* proc_def);
GSList* plug_in_extensions_parse  (char     *extensions);
int     plug_in_image_types_parse (char     *image_types);

void plug_in_progress_init   (PlugIn *plug_in, char *message, gint gdisp_ID);
void plug_in_progress_update (PlugIn *plug_in, double percentage);

extern PlugIn *current_plug_in;
extern GSList *proc_defs;

#endif /* __PLUG_IN_H__ */
