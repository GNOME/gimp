/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * module_db.c (C) 1999 Austin Donnelly <austin@gimp.org>
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

#include <stdio.h>
#include <string.h>

#include "appenv.h"
#include "module_db.h"
#include "gimprc.h"
#include "datafiles.h"
#include "actionarea.h"
#include "libgimp/gimpintl.h"

#include "libgimp/gimpmodule.h"


typedef enum {
  ST_MODULE_ERROR,         /* missing module_load function or other error */
  ST_LOADED_OK,            /* happy and running (normal state of affairs) */
  ST_LOAD_FAILED,          /* module_load returned GIMP_MODULE_UNLOAD */
  ST_UNLOAD_REQUESTED,     /* sent unload request, waiting for callback */
  ST_UNLOADED_OK           /* callback arrived, module not in memory anymore */
} module_state;

static const char * const statename[] = {
  "ST_MODULE_ERROR",
  "ST_LOADED_OK",
  "ST_LOAD_FAILED",
  "ST_UNLOAD_REQUESTED",
  "ST_UNLOADED_OK"
};


/* one of these is kept per-module */
typedef struct {
  gchar          *fullpath;   /* path to the module */
  module_state    state;      /* what's happened to the module */
  /* stuff from now on may be NULL depending on the state the module is in */
  GimpModuleInfo *info;       /* returned values from module_init */
  GModule        *module;     /* handle on the module */
  gchar          *last_module_error;
  GimpModuleInitFunc   *init;
  GimpModuleUnloadFunc *unload;
} module_info;

#define NUM_INFO_LINES 6

typedef struct {
  GtkWidget *table;
  GtkWidget *label[NUM_INFO_LINES];
  GtkWidget *button_label;
  module_info *last_update;
  GtkWidget *button;
} browser_st;

/* global list of module_info */
static GSList *modules = NULL;


/*#define DUMP_DB*/


/* prototypes */
static void module_initialize (char *filename);
static void mod_load (module_info *mod, gboolean verbose);
static void mod_unload (module_info *mod, gboolean verbose);
#ifdef DUMP_DB
static void print_module_info (gpointer data, gpointer user_data);
#endif

static void browser_popdown_callback (GtkWidget *w, gpointer client_data);
static void browser_info_update (browser_st *st, module_info *mod);
static void browser_info_init (GtkWidget *table);
static void browser_select_callback (GtkWidget *widget, GtkWidget *child);
static void browser_load_unload_callback (GtkWidget *widget, gpointer data);


/**************************************************************/
/* Exported functions */

void
module_db_init (void)
{
  /* Load and initialize gimp modules */

  if (g_module_supported ())
    datafiles_read_directories (module_path,
				module_initialize, 0 /* no flags */);

#ifdef DUMP_DB
  g_slist_foreach (modules, print_module_info, NULL);
#endif
}


GtkWidget *
module_db_browser_new (void)
{
  GtkWidget *shell;
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *listbox;
  GtkWidget *list;
  GtkWidget *list_item;
  GSList *here;
  module_info *info;
  browser_st *st;
  ActionAreaItem action_items[] =
  {
    { N_("OK"), browser_popdown_callback, NULL, NULL }
  };


  shell = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (shell), "module_db_dialog", "Gimp");
  gtk_window_set_title (GTK_WINDOW (shell), _("Module DB"));

  hbox = gtk_hbox_new (FALSE, 5);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 1);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (shell)->vbox), hbox, TRUE, TRUE, 0);
  gtk_widget_show (hbox);

  listbox = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (listbox),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (hbox), listbox, TRUE, TRUE, 0);
  gtk_widget_set_usize (listbox, 125, 100);
  gtk_widget_show (listbox);

  list = gtk_list_new ();
  gtk_list_set_selection_mode (GTK_LIST (list), GTK_SELECTION_BROWSE);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (listbox), list);

  st = g_new0 (browser_st, 1);

  here = modules;
  while (here)
  {
    info = here->data;
    here = g_slist_next (here);

    if (!st->last_update)
      st->last_update = info;

    list_item = gtk_list_item_new_with_label (info->fullpath);
    gtk_container_add (GTK_CONTAINER (list), list_item);
    gtk_widget_show (list_item);
    gtk_object_set_user_data (GTK_OBJECT (list_item), info);
  }

  gtk_widget_show (list);

  vbox = gtk_vbox_new (FALSE, 5);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  st->table = gtk_table_new (2, NUM_INFO_LINES, FALSE);
  gtk_box_pack_start (GTK_BOX (vbox), st->table, TRUE, TRUE, 0);
  gtk_widget_show (st->table);

  st->button = gtk_button_new ();
  gtk_box_pack_start (GTK_BOX (vbox), st->button, TRUE, TRUE, 0);
  gtk_widget_show (st->button);
  gtk_signal_connect (GTK_OBJECT (st->button), "clicked",
		      browser_load_unload_callback, st);

  browser_info_init (st->table);
  browser_info_update (st, modules->data);

  gtk_object_set_user_data (GTK_OBJECT (list), st);

  gtk_signal_connect (GTK_OBJECT (list), "select_child",
		      browser_select_callback, NULL);

  action_items[0].user_data = shell;
  build_action_area (GTK_DIALOG (shell), 
		     action_items,  
		     sizeof( action_items)/sizeof( ActionAreaItem), 
		     0);

  return shell;
}


/**************************************************************/
/* helper functions */


/* name must be of the form lib*.so */
/* TODO: need support for WIN32-style dll names.  Maybe this function
 * should live in libgmodule? */
static gboolean
valid_module_name (const char *filename)
{
  const char *basename;
  int len;

  basename = strrchr (filename, '/');
  if (basename)
    basename++;
  else
    basename = filename;

  len = strlen (basename);

  if (len < 3 + 1 + 3)
    return FALSE;

  if (strncmp (basename, "lib", 3))
    return FALSE;

  if (strcmp (basename + len - 3, ".so"))
    return FALSE;

  return TRUE;
}



static void
module_initialize (char *filename)
{
  module_info *mod;

  if (!valid_module_name (filename))
    return;

  mod = g_new0 (module_info, 1);
  modules = g_slist_append (modules, mod);

  mod->fullpath = g_strdup (filename);

  if ((be_verbose == TRUE) || (no_splash == TRUE))
    g_print (_("load module: \"%s\"\n"), filename);

  mod_load (mod, TRUE);
}

static void
mod_load (module_info *mod, gboolean verbose)
{
  gpointer symbol;

  g_return_if_fail (mod->module == NULL);

  mod->module = g_module_open (mod->fullpath, G_MODULE_BIND_LAZY);
  if (!mod->module)
    {
      mod->state = ST_MODULE_ERROR;

      if (mod->last_module_error)
	g_free (mod->last_module_error);
      mod->last_module_error = g_strdup (g_module_error ());

      if (verbose)
	g_warning (_("module load error: %s: %s"),
		   mod->fullpath, mod->last_module_error);
      return;
    }

  /* find the module_init symbol */
  if (!g_module_symbol (mod->module, "module_init", &symbol))
    {
      mod->state = ST_MODULE_ERROR;

      if (mod->last_module_error)
	g_free (mod->last_module_error);
      mod->last_module_error = g_strdup (_("missing module_init() symbol"));

      if (verbose)
	g_warning (_("%s: module_init() symbol not found"), mod->fullpath);

      g_module_close (mod->module);
      mod->module = NULL;
      mod->info = NULL;
      return;
    }

  /* run module's initialisation */
  mod->init = symbol;
  mod->info = NULL;
  if (mod->init (&mod->info) == GIMP_MODULE_UNLOAD)
  {
    mod->state = ST_LOAD_FAILED;
    g_module_close (mod->module);
    mod->module = NULL;
    mod->info = NULL;
    return;
  }

  /* module is now happy */
  mod->state = ST_LOADED_OK;

  /* do we have an unload function? */
  if (g_module_symbol (mod->module, "module_unload", &symbol))
    mod->unload = symbol;
  else
    mod->unload = NULL;
}


static void
mod_unload_completed_callback (void *data)
{
  module_info *mod = data;

  g_return_if_fail (mod->state == ST_UNLOAD_REQUESTED);

  g_module_close (mod->module);
  mod->module = NULL;
  mod->info = NULL;

  mod->state = ST_UNLOADED_OK;
}

static void
mod_unload (module_info *mod, gboolean verbose)
{
  g_return_if_fail (mod->module != NULL);
  g_return_if_fail (mod->unload != NULL);

  if (mod->state == ST_UNLOAD_REQUESTED)
    return;

  mod->state = ST_UNLOAD_REQUESTED;

  /* send the unload request */
  mod->unload (mod->info->shutdown_data, mod_unload_completed_callback, mod);
}



#ifdef DUMP_DB
static void
print_module_info (gpointer data, gpointer user_data)
{
  module_info *i = data;

  printf ("\n%s: %s\n",
	  i->fullpath, statename[i->state]);
  printf ("  module:%p  lasterr:%s  init:%p  unload:%p\n",
	  i->module, i->last_module_error? i->last_module_error : "NONE",
	  i->init, i->unload);
  if (i->info)
  {
    printf ("  shutdown_data: %p\n"
	    "  purpose:   %s\n"
	    "  author:    %s\n"
	    "  version:   %s\n"
	    "  copyright: %s\n"
	    "  date:      %s\n",
	    i->info->shutdown_data,
	    i->info->purpose, i->info->author, i->info->version,
	    i->info->copyright, i->info->date);
  }
}
#endif



/**************************************************************/
/* UI functions */

static void
browser_popdown_callback (GtkWidget *w, gpointer client_data)
{
  gtk_widget_destroy (GTK_WIDGET (client_data));
}


static void
browser_info_update (browser_st *st, module_info *mod)
{
  int i;
  const char *text[NUM_INFO_LINES];

  if (mod->info)
  {
    text[0] = mod->info->purpose;
    text[1] = mod->info->author;
    text[2] = mod->info->version;
    text[3] = mod->info->copyright;
    text[4] = mod->info->date;
  }
  else
  {
    text[0] = "--";
    text[1] = "--";
    text[2] = "--";
    text[3] = "--";
    text[4] = "--";
  }

  if (mod->state == ST_MODULE_ERROR && mod->last_module_error)
  {
    text[5] = g_malloc (strlen (statename[mod->state]) + 2 +
			strlen (mod->last_module_error) + 2);
    sprintf(text[5], "%s (%s)", statename[mod->state], mod->last_module_error);
  }
  else
  {
    text[5] = g_strdup (statename[mod->state]);
  }

  for (i=0; i < NUM_INFO_LINES; i++)
  {
    if (st->label[i])
      gtk_container_remove (GTK_CONTAINER (st->table), st->label[i]);
    st->label[i] = gtk_label_new (text[i]);
    gtk_misc_set_alignment (GTK_MISC (st->label[i]), 0.0, 0.5);
    gtk_table_attach (GTK_TABLE (st->table), st->label[i], 1, 2, i, i+1,
		      GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 2);
    gtk_widget_show (st->label[i]);
  }

  g_free (text[5]);

  /* work out what the button should do (if anything) */
  switch (mod->state) {
  case ST_MODULE_ERROR:
  case ST_LOAD_FAILED:
  case ST_UNLOADED_OK:
    if (st->button_label)
      gtk_container_remove (GTK_CONTAINER (st->button), st->button_label);
    st->button_label = gtk_label_new (_("Load"));
    gtk_widget_show (st->button_label);
    gtk_container_add (GTK_CONTAINER (st->button), st->button_label);
    gtk_widget_set_sensitive (GTK_WIDGET (st->button), TRUE);
    break;

  case ST_UNLOAD_REQUESTED:
    gtk_widget_set_sensitive (GTK_WIDGET (st->button), FALSE);
    break;

  case ST_LOADED_OK:
    if (st->button_label)
      gtk_container_remove (GTK_CONTAINER (st->button), st->button_label);
    st->button_label = gtk_label_new (_("Unload"));
    gtk_widget_show (st->button_label);
    gtk_container_add (GTK_CONTAINER (st->button), st->button_label);
    gtk_widget_set_sensitive (GTK_WIDGET (st->button),
			      mod->unload? TRUE : FALSE);
    break;    
  }
}

static void
browser_info_init (GtkWidget *table)
{
  GtkWidget *label;
  int i;
  char *text[] = {
    N_("Purpose: "),
    N_("Author: "),
    N_("Verson: "),
    N_("Copyright: "),
    N_("Date: "),
    N_("State: ")
  };

  for (i=0; i < sizeof(text) / sizeof(char *); i++)
  {
    label = gtk_label_new (gettext (text[i]));
    gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), label, 0, 1, i, i+1,
		      GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 2);
    gtk_widget_show (label);
  }
}

static void
browser_select_callback (GtkWidget *widget, GtkWidget *child)
{
  module_info *i;
  browser_st *st;

  i = gtk_object_get_user_data (GTK_OBJECT (child));
  st = gtk_object_get_user_data (GTK_OBJECT (widget));

  if (st->last_update == i)
    return;

  st->last_update = i;

  browser_info_update (st, i);
}


static void
browser_load_unload_callback (GtkWidget *widget, gpointer data)
{
  browser_st *st = data;

  if (st->last_update->state == ST_LOADED_OK)
    mod_unload (st->last_update, FALSE);
  else
    mod_load (st->last_update, FALSE);

  browser_info_update (st, st->last_update);
}
