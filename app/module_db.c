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

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <time.h>

#include "appenv.h"
#include "module_db.h"
#include "gimpsignal.h"
#include "gimprc.h"
#include "datafiles.h"
#include "actionarea.h"
#include "gimpset.h"
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


/* one of these objects is kept per-module */
typedef struct {
  GtkObject      object;
  gchar          *fullpath;   /* path to the module */
  module_state    state;      /* what's happened to the module */
  gboolean        ondisk;     /* TRUE if file still exists */
  /* stuff from now on may be NULL depending on the state the module is in */
  GimpModuleInfo *info;       /* returned values from module_init */
  gint            refs;       /* how many time we're running in the module */
  GModule        *module;     /* handle on the module */
  gchar          *last_module_error;
  GimpModuleInitFunc   *init;
  GimpModuleUnloadFunc *unload;
} module_info;


static guint module_info_get_type (void);
#define MODULE_INFO_TYPE module_info_get_type()
#define MODULE_INFO(obj) GTK_CHECK_CAST (obj, MODULE_INFO_TYPE, module_info)
#define IS_MODULE_INFO(obj) GTK_CHECK_TYPE (obj, MODULE_INFO_TYPE)


#define NUM_INFO_LINES 7

typedef struct {
  GtkWidget *table;
  GtkWidget *label[NUM_INFO_LINES];
  GtkWidget *button_label;
  module_info *last_update;
  GtkWidget *button;
  GtkWidget *list;
} browser_st;

/* global set of module_info pointers */
static GimpSet *modules;


/*#define DUMP_DB*/


/* prototypes */
static void module_initialize (char *filename);
static void mod_load (module_info *mod, gboolean verbose);
static void mod_unload (module_info *mod, gboolean verbose);
static module_info *module_find_by_path (const char *fullpath);
#ifdef DUMP_DB
static void print_module_info (gpointer data, gpointer user_data);
#endif

static void browser_popdown_callback (GtkWidget *w, gpointer client_data);
static void browser_destroy_callback (GtkWidget *w, gpointer client_data);
static void browser_info_update (module_info *, browser_st *);
static void browser_info_add (GimpSet *, module_info *, browser_st *);
static void browser_info_remove (GimpSet *, module_info *, browser_st *);
static void browser_info_init (browser_st *st, GtkWidget *table);
static void browser_select_callback (GtkWidget *widget, GtkWidget *child);
static void browser_load_unload_callback (GtkWidget *widget, gpointer data);
static void browser_refresh_callback (GtkWidget *widget, gpointer data);
static void make_list_item (gpointer data, gpointer user_data);

static void gimp_module_ref (module_info *mod);
static void gimp_module_unref (module_info *mod);



/**************************************************************/
/* Exported functions */

void
module_db_init (void)
{
  /* Load and initialize gimp modules */

  modules = gimp_set_new (MODULE_INFO_TYPE, FALSE);

  if (g_module_supported ())
    datafiles_read_directories (module_path,
				module_initialize, 0 /* no flags */);
#ifdef DUMP_DB
  gimp_set_foreach (modules, print_module_info, NULL);
#endif
}



GtkWidget *
module_db_browser_new (void)
{
  GtkWidget *shell;
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *listbox;
  GtkWidget *button;
  browser_st *st;
  ActionAreaItem action_items[] =
  {
    { N_("OK"), browser_popdown_callback, NULL, NULL }
  };


  shell = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (shell), "module_db_dialog", "Gimp");
  gtk_window_set_title (GTK_WINDOW (shell), _("Module DB"));

  hbox = gtk_hbox_new (FALSE, 5);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (shell)->vbox), hbox, TRUE, TRUE, 0);
  gtk_widget_show (hbox);

  listbox = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (listbox),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (hbox), listbox, TRUE, TRUE, 0);
  gtk_widget_set_usize (listbox, 125, 100);
  gtk_widget_show (listbox);

  st = g_new0 (browser_st, 1);

  st->list = gtk_list_new ();
  gtk_list_set_selection_mode (GTK_LIST (st->list), GTK_SELECTION_BROWSE);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (listbox),
					 st->list);

  gimp_set_foreach (modules, make_list_item, st);

  gtk_widget_show (st->list);

  vbox = gtk_vbox_new (FALSE, 10);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  st->table = gtk_table_new (5, NUM_INFO_LINES, FALSE);
  gtk_box_pack_start (GTK_BOX (vbox), st->table, FALSE, FALSE, 0);
  gtk_widget_show (st->table);

  hbox = gtk_hbox_new (FALSE, 10);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 5);

  button = gtk_button_new_with_label (_("Refresh"));
  gtk_widget_show (button);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      browser_refresh_callback, st);
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);

  st->button = gtk_button_new_with_label ("");
  st->button_label = GTK_BIN (st->button)->child;
  gtk_box_pack_start (GTK_BOX (hbox), st->button, TRUE, TRUE, 0);
  gtk_widget_show (st->button);
  gtk_signal_connect (GTK_OBJECT (st->button), "clicked",
		      browser_load_unload_callback, st);

  browser_info_init (st, st->table);
  browser_info_update (st->last_update, st);

  gtk_object_set_user_data (GTK_OBJECT (st->list), st);

  gtk_signal_connect (GTK_OBJECT (st->list), "select_child",
		      browser_select_callback, NULL);

  /* hook the gimpset signals so we can refresh the display
   * appropriately. */
  gimp_set_add_handler (modules, "modified", browser_info_update, st);

  gtk_signal_connect (GTK_OBJECT (modules), "add", 
		      browser_info_add, st);
  gtk_signal_connect (GTK_OBJECT (modules), "remove", 
		      browser_info_remove, st);

  gtk_signal_connect (GTK_OBJECT (shell), "destroy",
		      browser_destroy_callback, st);

  action_items[0].user_data = shell;
  build_action_area (GTK_DIALOG (shell), 
		     action_items,  
		     sizeof( action_items)/sizeof( ActionAreaItem), 
		     0);

  return shell;
}


/**************************************************************/
/* module_info object glue */


typedef struct {
  GtkObjectClass parent_class;
} module_infoClass;

enum {
  MODIFIED,
  LAST_SIGNAL
};
static guint module_info_signals[LAST_SIGNAL];


static void
module_info_destroy (GtkObject *object)
{
  module_info *mod = MODULE_INFO (object);

  if (mod->last_module_error)
    g_free (mod->last_module_error);
  g_free (mod->fullpath);
}

static void
module_info_class_init (module_infoClass *klass)
{
  GtkObjectClass *object_class;
  GtkType type;

  object_class = GTK_OBJECT_CLASS(klass);

  type = object_class->type;

  object_class->destroy = module_info_destroy;

  module_info_signals[MODIFIED] =
      gimp_signal_new ("modified", 0, type, 0, gimp_sigtype_void);

  gtk_object_class_add_signals(object_class, module_info_signals, LAST_SIGNAL);
}

static void
module_info_init (module_info *mod)
{
  /* don't need to do anything */
}

static guint
module_info_get_type (void)
{
  static guint module_info_type = 0;

  if (!module_info_type)
    {
      static const GtkTypeInfo module_info_info =
      {
	"module_info",
	sizeof (module_info),
	sizeof (module_infoClass),
	(GtkClassInitFunc) module_info_class_init,
	(GtkObjectInitFunc) module_info_init,
	/* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL,
      };

      module_info_type = gtk_type_unique (gtk_object_get_type(),
					  &module_info_info);
    }

  return module_info_type;
}

/* exported API: */

static void
module_info_modified (module_info *mod)
{
  gtk_signal_emit (GTK_OBJECT (mod), module_info_signals[MODIFIED]);
}

static module_info *
module_info_new (void)
{
  return MODULE_INFO (gtk_type_new (module_info_get_type ()));
}

static void
module_info_free (module_info *mod)
{
  gtk_object_unref (GTK_OBJECT (mod));
}


/**************************************************************/
/* helper functions */


/* name must be of the form lib*.so (Unix) or *.dll (Win32) */
static gboolean
valid_module_name (const char *filename)
{
  const char *basename;
  int len;

  basename = strrchr (filename, G_DIR_SEPARATOR);
  if (basename)
    basename++;
  else
    basename = filename;

  len = strlen (basename);

#ifndef WIN32
  if (len < 3 + 1 + 3)
    return FALSE;

  if (strncmp (basename, "lib", 3))
    return FALSE;

  if (strcmp (basename + len - 3, ".so"))
    return FALSE;
#else
  if (g_strcasecmp (basename + len - 4, ".dll"))
    return FALSE;
#endif

  return TRUE;
}



static void
module_initialize (char *filename)
{
  module_info *mod;

  if (!valid_module_name (filename))
    return;

  /* don't load if we already know about it */
  if (module_find_by_path (filename))
    return;

  mod = module_info_new ();

  mod->fullpath = g_strdup (filename);
  mod->ondisk = TRUE;

  if ((be_verbose == TRUE) || (no_splash == TRUE))
    g_print (_("load module: \"%s\"\n"), filename);

  mod_load (mod, TRUE);

  gimp_set_add (modules, mod);
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

  if (mod->refs == 0)
  {
      g_module_close (mod->module);
      mod->module = NULL;
  }
  mod->info = NULL;

  mod->state = ST_UNLOADED_OK;

  module_info_modified (mod);
}

static void
mod_unload (module_info *mod, gboolean verbose)
{
  g_return_if_fail (mod->module != NULL);
  g_return_if_fail (mod->unload != NULL);

  if (mod->state == ST_UNLOAD_REQUESTED)
    return;

  mod->state = ST_UNLOAD_REQUESTED;

  /* send the unload request.  Need to ref the module so we don't
   * accidentally unload it while this call is in progress (eg if the
   * callback is called before the unload function returns). */
  gimp_module_ref (mod);
  mod->unload (mod->info->shutdown_data, mod_unload_completed_callback, mod);
  gimp_module_unref (mod);
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
browser_destroy_callback (GtkWidget *w, gpointer client_data)
{
  gtk_signal_disconnect_by_data (GTK_OBJECT (modules), client_data);
  g_free (client_data);
}


static void
browser_info_update (module_info *mod, browser_st *st)
{
  int i;
  const char *text[NUM_INFO_LINES - 1];
  char *status;

  /* only update the info if we're actually showing it */
  if (mod != st->last_update)
    return;

  if (!mod)
  {
    for (i=0; i < NUM_INFO_LINES; i++)
      gtk_label_set_text (GTK_LABEL (st->label[i]), "");
    gtk_label_set_text (GTK_LABEL(st->button_label), _("<No modules>"));
    gtk_widget_set_sensitive (GTK_WIDGET (st->button), FALSE);
    return;
  }

  if (mod->info)
  {
    text[0] = mod->info->purpose;
    text[1] = mod->info->author;
    text[2] = mod->info->version;
    text[3] = mod->info->copyright;
    text[4] = mod->info->date;
    text[5] = mod->ondisk? _("on disk") : _("only in memory");
  }
  else
  {
    text[0] = "--";
    text[1] = "--";
    text[2] = "--";
    text[3] = "--";
    text[4] = "--";
    text[5] = mod->ondisk? _("on disk") : _("nowhere (click 'refresh')");
  }


  if (mod->state == ST_MODULE_ERROR && mod->last_module_error)
  {
    gulong len = strlen (statename[mod->state]) + 2 +
		 strlen (mod->last_module_error) + 2;

    status = g_malloc (len);
    g_snprintf (status, len,
		"%s (%s)", statename[mod->state], mod->last_module_error);
  }
  else
  {
    status = g_strdup (statename[mod->state]);
  }

  for (i=0; i < NUM_INFO_LINES - 1; i++)
  {
    gtk_label_set_text (GTK_LABEL (st->label[i]), text[i]);
  }

  gtk_label_set_text (GTK_LABEL (st->label[NUM_INFO_LINES-1]), status);

  g_free (status);

  /* work out what the button should do (if anything) */
  switch (mod->state) {
  case ST_MODULE_ERROR:
  case ST_LOAD_FAILED:
  case ST_UNLOADED_OK:
    gtk_label_set_text (GTK_LABEL(st->button_label), _("Load"));
    gtk_widget_set_sensitive (GTK_WIDGET (st->button), mod->ondisk);
    break;

  case ST_UNLOAD_REQUESTED:
    gtk_widget_set_sensitive (GTK_WIDGET (st->button), FALSE);
    break;

  case ST_LOADED_OK:
    gtk_label_set_text (GTK_LABEL(st->button_label), _("Unload"));
    gtk_widget_set_sensitive (GTK_WIDGET (st->button),
			      mod->unload? TRUE : FALSE);
    break;    
  }
}

static void
browser_info_init (browser_st *st, GtkWidget *table)
{
  GtkWidget *label;
  int i;
  char *text[] = {
    N_("Purpose: "),
    N_("Author: "),
    N_("Version: "),
    N_("Copyright: "),
    N_("Date: "),
    N_("Location: "),
    N_("State: ")
  };

  for (i=0; i < sizeof(text) / sizeof(char *); i++)
  {
    label = gtk_label_new (gettext (text[i]));
    gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), label, 0, 1, i, i+1,
		      GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 2);
    gtk_widget_show (label);

    st->label[i] = gtk_label_new ("");
    gtk_misc_set_alignment (GTK_MISC (st->label[i]), 0.0, 0.5);
    gtk_table_attach (GTK_TABLE (st->table), st->label[i], 1, 2, i, i+1,
		      GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 2);
    gtk_widget_show (st->label[i]);
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

  browser_info_update (st->last_update, st);
}


static void
browser_load_unload_callback (GtkWidget *widget, gpointer data)
{
  browser_st *st = data;

  if (st->last_update->state == ST_LOADED_OK)
    mod_unload (st->last_update, FALSE);
  else
    mod_load (st->last_update, FALSE);

  module_info_modified (st->last_update);
}


static void
make_list_item (gpointer data, gpointer user_data)
{
  module_info *info = data;
  browser_st *st = user_data;
  GtkWidget *list_item;

  if (!st->last_update)
    st->last_update = info;

  list_item = gtk_list_item_new_with_label (info->fullpath);

  gtk_widget_show (list_item);
  gtk_object_set_user_data (GTK_OBJECT (list_item), info);

  gtk_container_add (GTK_CONTAINER (st->list), list_item);
}


static void
browser_info_add (GimpSet *set, module_info *mod, browser_st *st)
{
  make_list_item (mod, st);
}


static void
browser_info_remove (GimpSet *set, module_info *mod, browser_st *st)
{
  GList *dlist, *free_list;
  GtkWidget *list_item;
  module_info *i;

  dlist = gtk_container_children (GTK_CONTAINER (st->list));
  free_list = dlist;

  while (dlist)
  {
    list_item = dlist->data;

    i = gtk_object_get_user_data (GTK_OBJECT (list_item));
    g_return_if_fail (i != NULL);

    if (i == mod)
    {
      gtk_container_remove (GTK_CONTAINER (st->list), list_item);
      g_list_free(free_list);
      return;
    }

    dlist = dlist->next;
  }

  g_warning ("tried to remove module that wasn't in brower's list");
  g_list_free(free_list);
}



static void
module_db_module_ondisk (gpointer data, gpointer user_data)
{
  module_info *mod = data;
  struct stat statbuf;
  int ret;
  int old_ondisk = mod->ondisk;
  GSList **kill_list = user_data;

  ret = stat (mod->fullpath, &statbuf);
  if (ret != 0)
    mod->ondisk = FALSE;
  else
    mod->ondisk = TRUE;

  /* if it's not on the disk, and it isn't in memory, mark it to be
   * removed later. */
  if (!mod->ondisk && !mod->module)
  {
    *kill_list = g_slist_append (*kill_list, mod);
    mod = NULL;
  }

  if (mod && mod->ondisk != old_ondisk)
    module_info_modified (mod);
}


static void
module_db_module_remove (gpointer data, gpointer user_data)
{
  module_info *mod = data;

  gimp_set_remove (modules, mod);

  module_info_free (mod);
}



typedef struct {
  const char *search_key;
  module_info *found;
} find_by_path_closure;

static void
module_db_path_cmp (gpointer data, gpointer user_data)
{
  module_info *mod = data;
  find_by_path_closure *cl = user_data;

  if (!strcmp (mod->fullpath, cl->search_key))
    cl->found = mod;
}

static module_info *
module_find_by_path (const char *fullpath)
{
  find_by_path_closure cl;

  cl.found = NULL;
  cl.search_key = fullpath;

  gimp_set_foreach (modules, module_db_path_cmp, &cl);

  return cl.found;
}



static void
browser_refresh_callback (GtkWidget *widget, gpointer data)
{
  GSList *kill_list = NULL;

  /* remove modules we don't have on disk anymore */
  gimp_set_foreach (modules, module_db_module_ondisk, &kill_list);
  g_slist_foreach (kill_list, module_db_module_remove, NULL);
  g_slist_free (kill_list);
  kill_list = NULL;

  /* walk filesystem and add new things we find */
  datafiles_read_directories (module_path,
			      module_initialize, 0 /* no flags */);
}


static void
gimp_module_ref (module_info *mod)
{
  g_return_if_fail (mod->refs >= 0);
  g_return_if_fail (mod->module != NULL);
  mod->refs++;
}

static void
gimp_module_unref (module_info *mod)
{
  g_return_if_fail (mod->refs > 0);
  g_return_if_fail (mod->module != NULL);

  mod->refs--;

  if (mod->refs == 0)
  {
    g_module_close (mod->module);
    mod->module = NULL;
  }
}

/* End of module_db.c */
