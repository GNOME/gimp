/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * module-browser.c
 * (C) 1999 Austin Donnelly <austin@gimp.org>
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

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "gui-types.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpmoduleinfo.h"
#include "core/gimpmodules.h"

#include "module-browser.h"

#include "app_procs.h"

#include "libgimp/gimpintl.h"


#define NUM_INFO_LINES 7

typedef struct
{
  GtkWidget         *table;
  GtkWidget         *label[NUM_INFO_LINES];
  GtkWidget         *button_label;
  GimpModuleInfoObj *last_update;
  GtkWidget         *button;
  GtkWidget         *list;
  GtkWidget         *load_inhibit_check;

  GQuark             modules_handler_id;
} BrowserState;


/*  local function prototypes  */

static void   browser_popdown_callback      (GtkWidget         *widget,
                                             gpointer           data);
static void   browser_destroy_callback      (GtkWidget         *widget,
                                             gpointer           data);
static void   browser_load_inhibit_callback (GtkWidget         *widget,
                                             gpointer           data);
static void   browser_select_callback       (GtkWidget         *widget,
                                             GtkWidget         *child);
static void   browser_load_unload_callback  (GtkWidget         *widget,
                                             gpointer           data);
static void   browser_refresh_callback      (GtkWidget         *widget,
                                             gpointer           data);
static void   make_list_item                (gpointer           data,
                                             gpointer           user_data);
static void   browser_info_add              (GimpContainer     *container,
                                             GimpModuleInfoObj *mod,
                                             BrowserState      *st);
static void   browser_info_remove           (GimpContainer     *container,
                                             GimpModuleInfoObj *mod,
                                             BrowserState      *st);
static void   browser_info_update           (GimpModuleInfoObj *mod,
                                             BrowserState      *st);
static void   browser_info_init             (BrowserState      *st,
                                             GtkWidget         *table);


/*  public functions  */

GtkWidget *
module_browser_new (void)
{
  GtkWidget    *shell;
  GtkWidget    *hbox;
  GtkWidget    *vbox;
  GtkWidget    *listbox;
  GtkWidget    *button;
  BrowserState *st;

  shell = gimp_dialog_new (_("Module DB"), "module_db_dialog",
			   gimp_standard_help_func,
			   "dialogs/module_browser.html",
			   GTK_WIN_POS_NONE,
			   FALSE, TRUE, FALSE,

			   GTK_STOCK_OK, browser_popdown_callback,
			   NULL, NULL, NULL, TRUE, TRUE,

			   NULL);

  vbox = gtk_vbox_new (FALSE, 5);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (shell)->vbox), vbox);
  gtk_widget_show (vbox);

  listbox = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (listbox),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (vbox), listbox, TRUE, TRUE, 0);
  gtk_widget_set_usize (listbox, 125, 100);
  gtk_widget_show (listbox);

  st = g_new0 (BrowserState, 1);

  st->list = gtk_list_new ();
  gtk_list_set_selection_mode (GTK_LIST (st->list), GTK_SELECTION_BROWSE);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (listbox),
					 st->list);

  gimp_container_foreach (the_gimp->modules, make_list_item, st);

  gtk_widget_show (st->list);

  st->table = gtk_table_new (5, NUM_INFO_LINES + 1, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (st->table), 4);  
  gtk_box_pack_start (GTK_BOX (vbox), st->table, FALSE, FALSE, 0);
  gtk_widget_show (st->table);

  hbox = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (GTK_BUTTON_BOX (hbox), GTK_BUTTONBOX_SPREAD);

  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, FALSE, 5);

  button = gtk_button_new_with_label (_("Refresh"));
  gtk_widget_show (button);
  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (browser_refresh_callback), st);
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);

  st->button = gtk_button_new_with_label ("");
  st->button_label = GTK_BIN (st->button)->child;
  gtk_box_pack_start (GTK_BOX (hbox), st->button, TRUE, TRUE, 0);
  gtk_widget_show (st->button);
  g_signal_connect (G_OBJECT (st->button), "clicked",
                    G_CALLBACK (browser_load_unload_callback), st);

  browser_info_init (st, st->table);
  browser_info_update (st->last_update, st);

  g_object_set_data (G_OBJECT (st->list), "state", st);

  g_signal_connect (G_OBJECT (st->list), "select_child",
                    G_CALLBACK (browser_select_callback), NULL);

  /* hook the GimpContainer signals so we can refresh the display
   * appropriately.
   */
  st->modules_handler_id =
    gimp_container_add_handler (the_gimp->modules, "modified", 
                                G_CALLBACK (browser_info_update), st);

  g_signal_connect (G_OBJECT (the_gimp->modules), "add", 
                    G_CALLBACK (browser_info_add), 
                    st);
  g_signal_connect (G_OBJECT (the_gimp->modules), "remove", 
                    G_CALLBACK (browser_info_remove), 
                    st);

  g_signal_connect (G_OBJECT (shell), "destroy",
                    G_CALLBACK (browser_destroy_callback), 
                    st);

  return shell;
}


/*  private functions  */

static void
browser_popdown_callback (GtkWidget *widget,
			  gpointer   data)
{
  gtk_widget_destroy (GTK_WIDGET (data));
}

static void
browser_destroy_callback (GtkWidget *widget,
			  gpointer   data)
{
  BrowserState *st = data;

  g_signal_handlers_disconnect_by_func (G_OBJECT (the_gimp->modules),
                                        browser_info_add,
                                        data);
  g_signal_handlers_disconnect_by_func (G_OBJECT (the_gimp->modules), 
                                        browser_info_remove,
                                        data);
  gimp_container_remove_handler (the_gimp->modules, st->modules_handler_id);
  g_free (data);
}

static void
browser_load_inhibit_callback (GtkWidget *widget,
			       gpointer   data)
{
  BrowserState *st = data;
  gboolean      new_value;

  g_return_if_fail (st->last_update != NULL);

  new_value = ! GTK_TOGGLE_BUTTON (widget)->active;

  if (new_value == st->last_update->load_inhibit)
    return;

  st->last_update->load_inhibit = new_value;
  gimp_module_info_modified (st->last_update);

  the_gimp->write_modulerc = TRUE;
}

static void
browser_select_callback (GtkWidget *widget, 
			 GtkWidget *child)
{
  GimpModuleInfoObj *info;
  BrowserState *st;

  info = g_object_get_data (G_OBJECT (child), "module_info");
  st = g_object_get_data (G_OBJECT (widget), "state");

  if (st->last_update == info)
    return;

  st->last_update = info;

  browser_info_update (st->last_update, st);
}

static void
browser_load_unload_callback (GtkWidget *widget, 
			      gpointer   data)
{
  BrowserState *st = data;

  if (st->last_update->state == GIMP_MODULE_STATE_LOADED_OK)
    gimp_module_info_module_unload (st->last_update, FALSE);
  else
    gimp_module_info_module_load (st->last_update, FALSE);

  gimp_module_info_modified (st->last_update);
}

static void
browser_refresh_callback (GtkWidget *widget, 
			  gpointer   data)
{
  gimp_modules_refresh (the_gimp);
}

static void
make_list_item (gpointer data, 
		gpointer user_data)
{
  GimpModuleInfoObj   *info = data;
  BrowserState *st = user_data;
  GtkWidget    *list_item;

  if (!st->last_update)
    st->last_update = info;

  list_item = gtk_list_item_new_with_label (info->fullpath);

  gtk_widget_show (list_item);
  g_object_set_data (G_OBJECT (list_item), "module_info", info);

  gtk_container_add (GTK_CONTAINER (st->list), list_item);
}

static void
browser_info_add (GimpContainer     *container,
		  GimpModuleInfoObj *mod, 
		  BrowserState      *st)
{
  make_list_item (mod, st);
}

static void
browser_info_remove (GimpContainer     *container,
		     GimpModuleInfoObj *mod, 
		     BrowserState      *st)
{
  GList             *dlist;
  GList             *free_list;
  GtkWidget         *list_item;
  GimpModuleInfoObj *info;

  dlist = gtk_container_get_children (GTK_CONTAINER (st->list));
  free_list = dlist;

  while (dlist)
    {
      list_item = dlist->data;

      info = g_object_get_data (G_OBJECT (list_item), "module_info");
      g_return_if_fail (info != NULL);

      if (info == mod)
	{
	  gtk_container_remove (GTK_CONTAINER (st->list), list_item);
	  g_list_free (free_list);
	  return;
	}

      dlist = dlist->next;
    }

  g_warning ("tried to remove module that wasn't in brower's list");
  g_list_free (free_list);
}

static void
browser_info_update (GimpModuleInfoObj *mod, 
		     BrowserState      *st)
{
  gint         i;
  const gchar *text[NUM_INFO_LINES - 1];
  gchar       *status;

  static const gchar * const statename[] =
  {
    N_("Module error"),
    N_("Loaded OK"),
    N_("Load failed"),
    N_("Unload requested"),
    N_("Unloaded OK")
  };

  /* only update the info if we're actually showing it */
  if (mod != st->last_update)
    return;

  if (! mod)
    {
      for (i = 0; i < NUM_INFO_LINES; i++)
	gtk_label_set_text (GTK_LABEL (st->label[i]), "");
      gtk_label_set_text (GTK_LABEL(st->button_label), _("<No modules>"));
      gtk_widget_set_sensitive (GTK_WIDGET (st->button), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET (st->load_inhibit_check), FALSE);
      return;
    }

  if (mod->info)
    {
      text[0] = mod->info->purpose;
      text[1] = mod->info->author;
      text[2] = mod->info->version;
      text[3] = mod->info->copyright;
      text[4] = mod->info->date;
      text[5] = mod->on_disk? _("on disk") : _("only in memory");
    }
  else
    {
      text[0] = "--";
      text[1] = "--";
      text[2] = "--";
      text[3] = "--";
      text[4] = "--";
      text[5] = mod->on_disk? _("on disk") : _("nowhere (click 'refresh')");
    }

  if (mod->state == GIMP_MODULE_STATE_ERROR && mod->last_module_error)
    {
      status = g_strdup_printf ("%s (%s)", gettext (statename[mod->state]),
                                mod->last_module_error);
    }
  else
    {
      status = g_strdup (gettext (statename[mod->state]));
    }

  for (i=0; i < NUM_INFO_LINES - 1; i++)
    {
      gtk_label_set_text (GTK_LABEL (st->label[i]), gettext (text[i]));
    }

  gtk_label_set_text (GTK_LABEL (st->label[NUM_INFO_LINES-1]), status);

  g_free (status);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (st->load_inhibit_check),
				!mod->load_inhibit);
  gtk_widget_set_sensitive (GTK_WIDGET (st->load_inhibit_check), TRUE);

  /* work out what the button should do (if anything) */
  switch (mod->state)
    {
    case GIMP_MODULE_STATE_ERROR:
    case GIMP_MODULE_STATE_LOAD_FAILED:
    case GIMP_MODULE_STATE_UNLOADED_OK:
      gtk_label_set_text (GTK_LABEL(st->button_label), _("Load"));
      gtk_widget_set_sensitive (GTK_WIDGET (st->button), mod->on_disk);
      break;

    case GIMP_MODULE_STATE_UNLOAD_REQUESTED:
      gtk_label_set_text (GTK_LABEL(st->button_label), _("Unload"));
      gtk_widget_set_sensitive (GTK_WIDGET (st->button), FALSE);
      break;

    case GIMP_MODULE_STATE_LOADED_OK:
      gtk_label_set_text (GTK_LABEL(st->button_label), _("Unload"));
      gtk_widget_set_sensitive (GTK_WIDGET (st->button),
				mod->unload? TRUE : FALSE);
      break;    
    }
}

static void
browser_info_init (BrowserState *st, 
		   GtkWidget    *table)
{
  GtkWidget *label;
  gint i;

  gchar *text[] =
  {
    N_("Purpose:"),
    N_("Author:"),
    N_("Version:"),
    N_("Copyright:"),
    N_("Date:"),
    N_("Location:"),
    N_("State:")
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

  st->load_inhibit_check =
    gtk_check_button_new_with_label (_("Autoload during startup"));
  gtk_widget_show (st->load_inhibit_check);
  gtk_table_attach (GTK_TABLE (table), st->load_inhibit_check,
		    0, 2, i, i+1,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 2);
  g_signal_connect (G_OBJECT (st->load_inhibit_check), "toggled",
                    G_CALLBACK (browser_load_inhibit_callback), st);
}
