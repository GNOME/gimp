/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This is a plug-in for the GIMP.
 *
 * Copyright (C) 2000 Michael Natterer <mitch@gimp.org>
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
#include <string.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

#include "pixmaps/new.xpm"
#include "pixmaps/duplicate.xpm"


enum
{
  SAVE,
  IDENTIFIER,
  FACTOR,
  DIGITS,
  SYMBOL,
  ABBREVIATION,
  SINGULAR,
  PLURAL,
  UNIT,
  USER_UNIT,
  BG_COLOR,
  NUM_COLUMNS
};


static void   query              (void);
static void   run                (gchar      *name,
				  gint        nparams,
				  GimpParam  *param,
				  gint       *nreturn_vals,
				  GimpParam **return_vals);

static void   unit_editor_dialog (void);


GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

MAIN ()


static const gchar *help_strings[] =
{
  N_("A unit definition will only be saved before "
     "GIMP exits if this column is checked."),
  N_("This string will be used to identify a "
     "unit in GIMP's configuration files."),
  N_("How many units make up an inch."),
  N_("This field is a hint for numerical input fields. "
     "It specifies how many decimal digits the input field "
     "should provide to get approximately the same accuracy as an "
     "\"inch\" input field with two decimal digits."),
  N_("The unit's symbol if it has one (e.g. \"'\" for inches). "
     "Use the unit's abbreviation if it doesn't have a symbol."),
  N_("The unit's abbreviation (e.g. \"cm\" for centimeters)."),
  N_("The unit's singular form."),
  N_("The unit's plural form.")
};


static void
query (void)
{
  static GimpParamDef args[] =
  {
    { GIMP_PDB_INT32, "run_mode", "Interactive" }
  };

  gimp_install_procedure ("extension_gimp_unit_editor",
                          "The GIMP unit editor (runs in interactive mode only)",
                          "The GIMP unit editor (runs in interactive mode only)",
                          "Michael Natterer <mitch@gimp.org>",
                          "Michael Natterer <mitch@gimp.org>",
                          "2000",
			  N_("<Toolbox>/Xtns/Unit Editor..."),
			  "",
                          GIMP_EXTENSION,
			  G_N_ELEMENTS (args), 0,
                          args, NULL);
}

static void
run (gchar      *name,
     gint        nparams,
     GimpParam  *param,
     gint       *nreturn_vals,
     GimpParam **return_vals)
{
  static GimpParam values[2];
  GimpRunMode      run_mode;

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_CALLING_ERROR;

  if (strcmp (name, "extension_gimp_unit_editor") == 0)
    {
      values[0].data.d_status = GIMP_PDB_SUCCESS;

      INIT_I18N_UI();

      unit_editor_dialog ();
    }
}

static void
new_unit_ok_callback (GtkWidget *widget,
		      gpointer   data)
{
  *((gboolean *) data) = TRUE;

  gtk_main_quit ();
}

static GimpUnit
new_unit (GtkWidget *main_dialog,
          GimpUnit   template)
{
  GtkWidget *dialog;
  GtkWidget *table;
  GtkWidget *entry;
  GtkWidget *spinbutton;

  GtkWidget *identifier_entry;
  GtkObject *factor_adj;
  GtkObject *digits_adj;
  GtkWidget *symbol_entry;
  GtkWidget *abbreviation_entry;
  GtkWidget *singular_entry;
  GtkWidget *plural_entry;

  gboolean  ok   = FALSE;
  GimpUnit  unit = GIMP_UNIT_PIXEL;

  dialog = gimp_dialog_new (_("New Unit"), "uniteditor",
			    gimp_standard_help_func, "filters/uniteditor.html",
			    GTK_WIN_POS_MOUSE,
			    FALSE, TRUE, FALSE,

			    GTK_STOCK_CANCEL, gtk_main_quit,
			    NULL, 1, NULL, FALSE, TRUE,

			    GTK_STOCK_OK, new_unit_ok_callback,
			    &ok, NULL, NULL, TRUE, FALSE,

			    NULL);

  gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (main_dialog));
  gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);

  table = gtk_table_new (7, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_container_set_border_width (GTK_CONTAINER (table), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), table,
		      FALSE, FALSE, 0);
  gtk_widget_show (table);

  entry = identifier_entry = gtk_entry_new ();
  if (template != GIMP_UNIT_PIXEL)
    {
      gtk_entry_set_text (GTK_ENTRY (entry), 
                          gimp_unit_get_identifier (template));
    }
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("_ID:"), 1.0, 0.5,
			     entry, 1, FALSE);

  gimp_help_set_help_data (entry, gettext (help_strings[IDENTIFIER]), NULL);

  spinbutton = gimp_spin_button_new (&factor_adj,
				     (template != GIMP_UNIT_PIXEL) ?
				     gimp_unit_get_factor (template) : 1.0,
				     GIMP_MIN_RESOLUTION, GIMP_MAX_RESOLUTION,
				     0.01, 0.1, 0.0, 0.01, 5);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
			     _("_Factor:"), 1.0, 0.5,
			     spinbutton, 1, TRUE);

  gimp_help_set_help_data (spinbutton, gettext (help_strings[FACTOR]), NULL);

  spinbutton = gimp_spin_button_new (&digits_adj,
				     (template != GIMP_UNIT_PIXEL) ?
				     gimp_unit_get_digits (template) : 2.0,
				     0, 5, 1, 1, 0, 1, 0);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 2,
			     _("_Digits:"), 1.0, 0.5,
			     spinbutton, 1, TRUE);

  gimp_help_set_help_data (spinbutton, gettext (help_strings[DIGITS]), NULL);

  entry = symbol_entry = gtk_entry_new ();
  if (template != GIMP_UNIT_PIXEL)
    {
      gtk_entry_set_text (GTK_ENTRY (entry), 
                          gimp_unit_get_symbol (template));
    }
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 3,
			     _("_Symbol:"), 1.0, 0.5,
			     entry, 1, FALSE);

  gimp_help_set_help_data (entry, gettext (help_strings[SYMBOL]), NULL);

  entry = abbreviation_entry = gtk_entry_new ();
  if (template != GIMP_UNIT_PIXEL)
    {
      gtk_entry_set_text (GTK_ENTRY (entry), 
                          gimp_unit_get_abbreviation (template));
    }
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 4,
			     _("_Abbreviation:"), 1.0, 0.5,
			     entry, 1, FALSE);

  gimp_help_set_help_data (entry, gettext (help_strings[ABBREVIATION]), NULL);

  entry = singular_entry = gtk_entry_new ();
  if (template != GIMP_UNIT_PIXEL)
    {
      gtk_entry_set_text (GTK_ENTRY (entry),
                          gimp_unit_get_singular (template));
    }
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 5,
			     _("Si_ngular:"), 1.0, 0.5,
			     entry, 1, FALSE);

  gimp_help_set_help_data (entry, gettext (help_strings[SINGULAR]), NULL);

  entry = plural_entry = gtk_entry_new ();
  if (template != GIMP_UNIT_PIXEL)
    {
      gtk_entry_set_text (GTK_ENTRY (entry),
                          gimp_unit_get_plural (template));
    }
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 6,
			     _("_Plural:"), 1.0, 0.5,
			     entry, 1, FALSE);

  gimp_help_set_help_data (entry, gettext (help_strings[PLURAL]), NULL);

  gtk_widget_show (dialog);

  while (TRUE)
    {
      gtk_main ();

      if (ok)
	{
	  gchar   *identifier;
	  gdouble  factor;
	  gint     digits;
	  gchar   *symbol;
	  gchar   *abbreviation;
	  gchar   *singular;
	  gchar   *plural;

	  identifier   = g_strdup (gtk_entry_get_text (GTK_ENTRY (identifier_entry)));
	  factor       = GTK_ADJUSTMENT (factor_adj)->value;
	  digits       = ROUND (GTK_ADJUSTMENT (digits_adj)->value);
	  symbol       = g_strdup (gtk_entry_get_text (GTK_ENTRY (symbol_entry)));
	  abbreviation = g_strdup (gtk_entry_get_text (GTK_ENTRY (abbreviation_entry)));
	  singular     = g_strdup (gtk_entry_get_text (GTK_ENTRY (singular_entry)));
	  plural       = g_strdup (gtk_entry_get_text (GTK_ENTRY (plural_entry)));

	  identifier   = g_strstrip (identifier);
	  symbol       = g_strstrip (symbol);
	  abbreviation = g_strstrip (abbreviation);
	  singular     = g_strstrip (singular);
	  plural       = g_strstrip (plural);

	  if (factor < GIMP_MIN_RESOLUTION)
	    {
	      g_message (_("Unit factor must not be 0."));
	      ok = FALSE;
	    }

	  if (!strlen (identifier) |
	      !strlen (symbol) |
	      !strlen (abbreviation) |
	      !strlen (singular) |
	      !strlen (plural))
	    {
	      g_message (_("All text fields must contain a value."));
	      ok = FALSE;
	    }

	  if (ok)
	    unit = gimp_unit_new (identifier, factor, digits,
				  symbol, abbreviation,
				  singular, plural);

	  g_free (identifier);
	  g_free (symbol);
	  g_free (abbreviation);
	  g_free (singular);
	  g_free (plural);

	  if (ok)
	    break;
	}
      else
	{
	  break;
	}
    }

  gtk_widget_destroy (dialog);

  return unit;
}

static void
list_init (GtkTreeView *tv)
{
  GtkListStore     *list_store;
  GtkTreeSelection *sel;
  GtkTreeIter       iter;
  gint              num_units;
  GimpUnit          unit;
  GdkColor          color;

  list_store = GTK_LIST_STORE (gtk_tree_view_get_model (tv));
  sel        = gtk_tree_view_get_selection (tv);

  gtk_list_store_clear (list_store);

  num_units = gimp_unit_get_number_of_units ();

  color.red   = 65535;
  color.green = 50000;
  color.blue  = 50000;

  for (unit = GIMP_UNIT_INCH; unit < num_units; unit++)
    {
      gboolean user_unit;

      user_unit = (unit >= gimp_unit_get_number_of_built_in_units ());

      gtk_list_store_append (list_store, &iter);
      gtk_list_store_set (list_store, &iter,
                          SAVE,         ! gimp_unit_get_deletion_flag (unit),
                          IDENTIFIER,   gimp_unit_get_identifier (unit),
                          FACTOR,       gimp_unit_get_factor (unit),
                          DIGITS,       gimp_unit_get_digits (unit),
                          SYMBOL,       gimp_unit_get_symbol (unit),
                          ABBREVIATION, gimp_unit_get_abbreviation (unit),
                          SINGULAR,     gimp_unit_get_singular (unit),
                          PLURAL,       gimp_unit_get_plural (unit),
                          UNIT,         unit,
                          USER_UNIT,    user_unit,

                          user_unit ? -1 : BG_COLOR, &color,

                          -1);
    }

  if (gtk_tree_model_get_iter_root (GTK_TREE_MODEL (list_store), &iter))
    gtk_tree_selection_select_iter (sel, &iter);
}

static void
new_callback (GtkWidget   *widget,
	      GtkTreeView *tv)
{
  GtkListStore     *list_store;
  GtkTreeSelection *sel;
  GimpUnit          unit;

  list_store = GTK_LIST_STORE (gtk_tree_view_get_model (tv));
  sel        = gtk_tree_view_get_selection (tv);

  unit = new_unit (gtk_widget_get_toplevel (widget), GIMP_UNIT_PIXEL);

  if (unit != GIMP_UNIT_PIXEL)
    {
      GtkTreeIter iter;

      list_init (tv);

      if (gtk_tree_model_get_iter_root (GTK_TREE_MODEL (list_store), &iter) &&
          gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (list_store), &iter,
                                         NULL, unit - GIMP_UNIT_INCH))
        {
          GtkAdjustment *adj;

          gtk_tree_selection_select_iter (sel, &iter);

          adj = gtk_tree_view_get_vadjustment (tv);
          gtk_adjustment_set_value (adj, adj->upper);
        }
    }
}

static void
duplicate_callback (GtkWidget   *widget,
		    GtkTreeView *tv)
{
  GtkListStore     *list_store;
  GtkTreeSelection *sel;
  GtkTreeIter       iter;

  list_store = GTK_LIST_STORE (gtk_tree_view_get_model (tv));
  sel        = gtk_tree_view_get_selection (tv);

  if (gtk_tree_selection_get_selected (sel, NULL, &iter))
    {
      GimpUnit unit;

      gtk_tree_model_get (GTK_TREE_MODEL (list_store), &iter,
                          UNIT, &unit,
                          -1);

      unit = new_unit (gtk_widget_get_toplevel (widget), unit);

      if (unit != GIMP_UNIT_PIXEL)
        {
          GtkTreeIter iter;

          list_init (tv);

          if (gtk_tree_model_get_iter_root (GTK_TREE_MODEL (list_store),
                                            &iter) &&
              gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (list_store), &iter,
                                             NULL, unit - GIMP_UNIT_INCH))
            {
              GtkAdjustment *adj;

              gtk_tree_selection_select_iter (sel, &iter);

              adj = gtk_tree_view_get_vadjustment (tv);
              gtk_adjustment_set_value (adj, adj->upper);
            }
        }
    }
}

static void
saved_toggled_callback (GtkCellRendererToggle *celltoggle,
                        gchar                 *path_string,
                        GtkListStore          *list_store)
{
  GtkTreePath *path;
  GtkTreeIter  iter;
  gboolean     saved;
  GimpUnit     unit;

  path = gtk_tree_path_new_from_string (path_string);
  if (! gtk_tree_model_get_iter (GTK_TREE_MODEL (list_store), &iter, path))
    {
      g_warning ("%s: bad tree path?", G_STRLOC);
      return;
    }
  gtk_tree_path_free (path);

  gtk_tree_model_get (GTK_TREE_MODEL (list_store), &iter,
                      SAVE, &saved,
                      UNIT, &unit,
                      -1);

  if (unit >= gimp_unit_get_number_of_built_in_units ())
    {
      gimp_unit_set_deletion_flag (unit, saved);
      gtk_list_store_set (GTK_LIST_STORE (list_store), &iter,
                          SAVE, ! saved,
                          -1);
    }
}

static void
unit_editor_dialog (void)
{
  GtkWidget         *main_dialog;
  GtkWidget         *main_vbox;
  GtkWidget         *scrolled_win;
  GtkWidget         *hbox;
  GtkWidget         *button;
  GtkListStore      *list_store;
  GtkWidget         *tv;
  GtkTreeViewColumn *col;
  GtkCellRenderer   *rend;

  gimp_ui_init ("uniteditor", FALSE);

  list_store = gtk_list_store_new (NUM_COLUMNS,
                                   G_TYPE_BOOLEAN,
                                   G_TYPE_STRING,
                                   G_TYPE_DOUBLE,
                                   G_TYPE_INT,
                                   G_TYPE_STRING,
                                   G_TYPE_STRING,
                                   G_TYPE_STRING,
                                   G_TYPE_STRING,
                                   G_TYPE_INT,
                                   G_TYPE_BOOLEAN,
                                   GDK_TYPE_COLOR);
  tv = gtk_tree_view_new_with_model (GTK_TREE_MODEL (list_store));
  g_object_unref (list_store);

  main_dialog =
    gimp_dialog_new (_("Unit Editor"), "uniteditor",
                     gimp_standard_help_func, "filters/uniteditor.html",
                     GTK_WIN_POS_MOUSE,
                     FALSE, TRUE, TRUE,

                     GTK_STOCK_REFRESH, list_init,
                     NULL, tv, NULL, FALSE, FALSE,

                     GTK_STOCK_CLOSE, gtk_widget_destroy,
                     NULL, 1, NULL, TRUE, TRUE,

                     NULL);

  g_signal_connect (G_OBJECT (main_dialog), "destroy",
                    G_CALLBACK (gtk_main_quit),
                    NULL);

  gimp_help_init ();

  /*  the main vbox  */
  main_vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 6);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (main_dialog)->vbox), main_vbox);
  gtk_widget_show (main_vbox);

  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_win),
				       GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
                                  GTK_POLICY_NEVER,
                                  GTK_POLICY_ALWAYS);
  gtk_container_add (GTK_CONTAINER (main_vbox), scrolled_win);
  gtk_widget_show (scrolled_win);

  gtk_widget_set_size_request (tv, -1, 220);
  gtk_container_add (GTK_CONTAINER (scrolled_win), tv);
  gtk_widget_show (tv);


  rend = gtk_cell_renderer_toggle_new ();
  col = gtk_tree_view_column_new ();
  gtk_tree_view_column_set_title (col, _("Saved"));
  gtk_tree_view_column_pack_start (col, rend, FALSE);
  gtk_tree_view_column_set_attributes (col, rend,
                                       "active",      SAVE,
                                       "activatable", USER_UNIT,
                                       NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tv), col);

  gimp_help_set_help_data (col->button, gettext (help_strings[SAVE]), NULL);

  g_signal_connect (G_OBJECT (rend), "toggled",
                    G_CALLBACK (saved_toggled_callback),
                    list_store);

  rend = gtk_cell_renderer_text_new ();
  col  = gtk_tree_view_column_new_with_attributes (_("ID"), rend,
                                                   "text",           IDENTIFIER,
                                                   "background-gdk", BG_COLOR,
                                                   NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tv), col);

  gimp_help_set_help_data (col->button, gettext (help_strings[IDENTIFIER]), NULL);

  rend = gtk_cell_renderer_text_new ();
  col  = gtk_tree_view_column_new_with_attributes (_("Factor"), rend,
                                                   "text", FACTOR,
                                                   NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tv), col);

  gimp_help_set_help_data (col->button, gettext (help_strings[FACTOR]), NULL);

  rend = gtk_cell_renderer_text_new ();
  col  = gtk_tree_view_column_new_with_attributes (_("Digits"), rend,
                                                   "text", DIGITS,
                                                   NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tv), col);

  gimp_help_set_help_data (col->button, gettext (help_strings[DIGITS]), NULL);

  rend = gtk_cell_renderer_text_new ();
  col  = gtk_tree_view_column_new_with_attributes (_("Symbol"), rend,
                                                   "text", SYMBOL,
                                                   NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tv), col);

  gimp_help_set_help_data (col->button, gettext (help_strings[SYMBOL]), NULL);

  rend = gtk_cell_renderer_text_new ();
  col  = gtk_tree_view_column_new_with_attributes (_("Abbreviation"), rend,
                                                   "text", ABBREVIATION,
                                                   NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tv), col);

  gimp_help_set_help_data (col->button, gettext (help_strings[ABBREVIATION]), NULL);

  rend = gtk_cell_renderer_text_new ();
  col  = gtk_tree_view_column_new_with_attributes (_("Singular"), rend,
                                                   "text", SINGULAR,
                                                   NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tv), col);

  gimp_help_set_help_data (col->button, gettext (help_strings[SINGULAR]), NULL);

  rend = gtk_cell_renderer_text_new ();
  col  = gtk_tree_view_column_new_with_attributes (_("Plural"), rend,
                                                   "text", PLURAL,
                                                   NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tv), col);

  gimp_help_set_help_data (col->button, gettext (help_strings[PLURAL]), NULL);

  list_init (GTK_TREE_VIEW (tv));

  hbox = gtk_hbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  button = gimp_pixmap_button_new (new_xpm, _("_New Unit"));
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (new_callback),
                    tv);

  gimp_help_set_help_data (button, _("Create a new unit from scratch."), NULL);

  button = gimp_pixmap_button_new (duplicate_xpm, _("_Duplicate Unit"));
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (duplicate_callback),
                    tv);

  gimp_help_set_help_data (button, _("Create a new unit with the currently "
				     "seleted unit as template."), NULL);

  gtk_widget_show (main_dialog);

  gtk_main ();
  gimp_help_free ();
}
