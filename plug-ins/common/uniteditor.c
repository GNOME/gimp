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

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

#include "pixmaps/new.xpm"
#include "pixmaps/duplicate.xpm"
#include "pixmaps/yes.xpm"
#include "pixmaps/no.xpm"


static void   query              (void);
static void   run                (gchar   *name,
				  gint     nparams,
				  GParam  *param,
				  gint    *nreturn_vals,
				  GParam **return_vals);

static void   unit_editor_dialog (void);

GPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

MAIN ()

static void
query (void)
{
  static GParamDef args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive" }
  };
  static gint nargs = sizeof (args) / sizeof (args[0]);

  gimp_install_procedure ("extension_gimp_unit_editor",
                          "The GIMP unit editor (runs in interactive mode only)",
                          "The GIMP unit editor (runs in interactive mode only)",
                          "Michael Natterer <mitch@gimp.org>",
                          "Michael Natterer <mitch@gimp.org>",
                          "2000",
			  N_("<Toolbox>/Xtns/Unit Editor..."),
			  "",
                          PROC_EXTENSION,
			  nargs, 0,
                          args, NULL);
}

static void
run (gchar   *name,
     gint     nparams,
     GParam  *param,
     gint    *nreturn_vals,
     GParam **return_vals)
{
  static GParam  values[2];
  GRunModeType   run_mode;

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals  = values;
  values[0].type          = PARAM_STATUS;
  values[0].data.d_status = STATUS_CALLING_ERROR;

  if (strcmp (name, "extension_gimp_unit_editor") == 0)
    {
      values[0].data.d_status = STATUS_SUCCESS;

      INIT_I18N_UI();

      unit_editor_dialog ();
    }
}

static GdkPixmap *yes_pixmap = NULL;
static GdkBitmap *yes_mask   = NULL;
static GdkPixmap *no_pixmap  = NULL;
static GdkBitmap *no_mask    = NULL;

static GtkWidget *delete_button    = NULL;
static GtkWidget *undelete_button  = NULL;

static GtkWidget *main_dialog = NULL;
static GtkWidget *clist       = NULL;
static GdkColor   color;

static GimpUnit   current_unit = GIMP_UNIT_INCH;
static gint       current_row  = 0;

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
  NUM_FIELDS
};

static gchar *help_strings[NUM_FIELDS];

static void
new_unit_ok_callback (GtkWidget *widget,
		      gpointer   data)
{
  *((gboolean *) data) = TRUE;

  gtk_main_quit ();
}

static GimpUnit
new_unit (GimpUnit template)
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

  gchar    *str;
  gboolean  ok   = FALSE;
  GimpUnit  unit = GIMP_UNIT_PIXEL;

  gtk_widget_set_sensitive (main_dialog, FALSE);

  dialog = gimp_dialog_new (_("New Unit"), "uniteditor",
			    gimp_plugin_help_func, "filters/uniteditor.html",
			    GTK_WIN_POS_MOUSE,
			    FALSE, TRUE, FALSE,

			    _("OK"), new_unit_ok_callback,
			    &ok, NULL, NULL, TRUE, FALSE,
			    _("Cancel"), gtk_main_quit,
			    NULL, 1, NULL, FALSE, TRUE,

			    NULL);

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
      str = gimp_unit_get_identifier (template);
      gtk_entry_set_text (GTK_ENTRY (entry), str);
      g_free (str);
    }
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("ID:"), 1.0, 0.5,
			     entry, 1, FALSE);

  gimp_help_set_help_data (entry, help_strings[IDENTIFIER], NULL);

  spinbutton = gimp_spin_button_new (&factor_adj,
				     (template != GIMP_UNIT_PIXEL) ?
				     gimp_unit_get_factor (template) : 1.0,
				     GIMP_MIN_RESOLUTION, GIMP_MAX_RESOLUTION,
				     0.01, 0.1, 0.0, 0.01, 5);
  gtk_widget_set_usize (spinbutton, 100, -1);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
			     _("Factor:"), 1.0, 0.5,
			     spinbutton, 1, TRUE);

  gimp_help_set_help_data (spinbutton, help_strings[FACTOR], NULL);

  spinbutton = gimp_spin_button_new (&digits_adj,
				     (template != GIMP_UNIT_PIXEL) ?
				     gimp_unit_get_digits (template) : 2.0,
				     0, 5, 1, 1, 0, 1, 0);
  gtk_widget_set_usize (spinbutton, 50, -1);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 2,
			     _("Digits:"), 1.0, 0.5,
			     spinbutton, 1, TRUE);

  gimp_help_set_help_data (spinbutton, help_strings[DIGITS], NULL);

  entry = symbol_entry = gtk_entry_new ();
  if (template != GIMP_UNIT_PIXEL)
    {
      str = gimp_unit_get_symbol (template);
      gtk_entry_set_text (GTK_ENTRY (entry), str);
      g_free (str);
    }
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 3,
			     _("Symbol:"), 1.0, 0.5,
			     entry, 1, FALSE);

  gimp_help_set_help_data (entry, help_strings[SYMBOL], NULL);

  entry = abbreviation_entry = gtk_entry_new ();
  if (template != GIMP_UNIT_PIXEL)
    {
      str = gimp_unit_get_abbreviation (template);
      gtk_entry_set_text (GTK_ENTRY (entry), str);
      g_free (str);
    }
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 4,
			     _("Abbreviation:"), 1.0, 0.5,
			     entry, 1, FALSE);

  gimp_help_set_help_data (entry, help_strings[ABBREVIATION], NULL);

  entry = singular_entry = gtk_entry_new ();
  if (template != GIMP_UNIT_PIXEL)
    {
      str = gimp_unit_get_singular (template);
      gtk_entry_set_text (GTK_ENTRY (entry), str);
      g_free (str);
    }
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 5,
			     _("Singular:"), 1.0, 0.5,
			     entry, 1, FALSE);

  gimp_help_set_help_data (entry, help_strings[SINGULAR], NULL);

  entry = plural_entry = gtk_entry_new ();
  if (template != GIMP_UNIT_PIXEL)
    {
      str = gimp_unit_get_plural (template);
      gtk_entry_set_text (GTK_ENTRY (entry), str);
      g_free (str);
    }
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 6,
			     _("Plural:"), 1.0, 0.5,
			     entry, 1, FALSE);

  gimp_help_set_help_data (entry, help_strings[PLURAL], NULL);

  gtk_widget_show (dialog);

  while (1)
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

	  identifier   = gtk_entry_get_text (GTK_ENTRY (identifier_entry));
	  factor       = GTK_ADJUSTMENT (factor_adj)->value;
	  digits       = ROUND (GTK_ADJUSTMENT (digits_adj)->value);
	  symbol       = gtk_entry_get_text (GTK_ENTRY (symbol_entry));
	  abbreviation = gtk_entry_get_text (GTK_ENTRY (abbreviation_entry));
	  singular     = gtk_entry_get_text (GTK_ENTRY (singular_entry));
	  plural       = gtk_entry_get_text (GTK_ENTRY (plural_entry));

	  identifier   = g_strstrip (g_strdup (identifier));
	  symbol       = g_strstrip (g_strdup (symbol));
	  abbreviation = g_strstrip (g_strdup (abbreviation));
	  singular     = g_strstrip (g_strdup (singular));
	  plural       = g_strstrip (g_strdup (plural));

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
  gdk_flush ();

  gtk_widget_set_sensitive (main_dialog, TRUE);

  return unit;
}

static void
clist_init (void)
{
  gchar    *entries[8];
  gint      num_units;
  GimpUnit  unit;
  gint      column_width;
  gint      row;
  gint      i;

  entries[0] = NULL;

  num_units = gimp_unit_get_number_of_units ();
  for (unit = GIMP_UNIT_INCH; unit < num_units; unit++)
    {
      row = unit - GIMP_UNIT_INCH;

      entries[1] = gimp_unit_get_identifier (unit);
      entries[2] = g_strdup_printf ("%.5f", gimp_unit_get_factor (unit));
      entries[3] = g_strdup_printf ("%d", gimp_unit_get_digits (unit));
      entries[4] = gimp_unit_get_symbol (unit);
      entries[5] = gimp_unit_get_abbreviation (unit);
      entries[6] = gimp_unit_get_singular (unit);
      entries[7] = gimp_unit_get_plural (unit);

      gtk_clist_append (GTK_CLIST (clist), entries);
      gtk_clist_set_row_data (GTK_CLIST (clist), row,
                              (gpointer) unit);

      if (unit < gimp_unit_get_number_of_built_in_units ())
	gtk_clist_set_background (GTK_CLIST (clist), row, &color);

      if (gimp_unit_get_deletion_flag (unit))
	{
	  gtk_clist_set_pixmap (GTK_CLIST (clist), row, 0,
				no_pixmap, no_mask);
	}
      else
	{
	  gtk_clist_set_pixmap (GTK_CLIST (clist), row, 0,
				yes_pixmap, yes_mask);
	}

      for (i = 1; i < 8; i++)
	g_free (entries[i]);
    }

  for (i = 0; i < 8; i++)
    {
      column_width = gtk_clist_optimal_column_width (GTK_CLIST (clist), i);

      gtk_clist_set_column_width (GTK_CLIST (clist), i, column_width);
    }
}

static void
select_row_callback (GtkWidget      *widget,
		     gint            row, 
		     gint            column, 
		     GdkEventButton *bevent,
		     gpointer        data)
{
  GimpUnit unit;
  gboolean delete_sensitive   = TRUE;
  gboolean undelete_sensitive = TRUE;

  current_row = row;

  unit = current_unit =
    (GimpUnit) gtk_clist_get_row_data (GTK_CLIST (widget), row);

  if (unit < gimp_unit_get_number_of_built_in_units ())
    {
      delete_sensitive   = FALSE;
      undelete_sensitive = FALSE;
    }
  else if (gimp_unit_get_deletion_flag (unit))
    {
      delete_sensitive = FALSE;
      gtk_clist_set_pixmap (GTK_CLIST (clist), current_row, 0,
			    no_pixmap, no_mask);
    }
  else
    {
      undelete_sensitive = FALSE;
      gtk_clist_set_pixmap (GTK_CLIST (clist), current_row, 0,
			    yes_pixmap, yes_mask);
    }

  gtk_widget_set_sensitive (delete_button,    delete_sensitive);
  gtk_widget_set_sensitive (undelete_button,  undelete_sensitive);
}

static void
refresh_callback (GtkWidget *widget,
		  gpointer   data)
{
  gtk_clist_clear (GTK_CLIST (clist));
  clist_init ();
  gtk_clist_select_row (GTK_CLIST (clist), 0, 0);
}

static void
new_callback (GtkWidget *widget,
	      gpointer   data)
{
  GimpUnit unit;

  unit = new_unit (GIMP_UNIT_PIXEL);

  if (unit != GIMP_UNIT_PIXEL)
    {
      refresh_callback (NULL, NULL);
      gtk_clist_select_row (GTK_CLIST (clist), unit - GIMP_UNIT_INCH, 0);
      gtk_clist_moveto (GTK_CLIST (clist), unit - GIMP_UNIT_INCH, -1, 0.5, 0.0);
    }
}

static void
duplicate_callback (GtkWidget *widget,
		    gpointer   data)
{
  GimpUnit unit;

  unit = new_unit (current_unit);

  if (unit != GIMP_UNIT_PIXEL)
    {
      refresh_callback (NULL, NULL);
      gtk_clist_select_row (GTK_CLIST (clist), unit - GIMP_UNIT_INCH, 0);
      gtk_clist_moveto (GTK_CLIST (clist), unit - GIMP_UNIT_INCH, -1, 0.5, 0.0);
    }
}

static void
delete_callback (GtkWidget *widget,
		 gpointer   data)
{
  gimp_unit_set_deletion_flag (current_unit, TRUE);
  gtk_clist_set_pixmap (GTK_CLIST (clist), current_row, 0,
			no_pixmap, no_mask);
  gtk_clist_select_row (GTK_CLIST (clist), current_row, 0);
}

static void
undelete_callback (GtkWidget *widget,
		   gpointer   data)
{
  gimp_unit_set_deletion_flag (current_unit, FALSE);
  gtk_clist_set_pixmap (GTK_CLIST (clist), current_row, 0,
			yes_pixmap, yes_mask);
  gtk_clist_select_row (GTK_CLIST (clist), current_row, 0);
}

static void
unit_editor_dialog (void)
{
  GtkWidget *main_vbox;
  GtkWidget *scrolled_win;
  GtkWidget *hbox;
  GtkWidget *button;
  GtkStyle  *style;

  gchar  *titles[NUM_FIELDS];
  gint    i;

  gimp_ui_init ("uniteditor", FALSE);

  main_dialog =
    gimp_dialog_new (_("Unit Editor"), "uniteditor",
		     gimp_plugin_help_func, "filters/uniteditor.html",
		     GTK_WIN_POS_MOUSE,
		     FALSE, TRUE, TRUE,

		     _("Refresh"), refresh_callback,
		     NULL, NULL, NULL, FALSE, FALSE,
		     _("Close"), gtk_widget_destroy,
		     NULL, 1, NULL, TRUE, TRUE,

		     NULL);

  gtk_signal_connect (GTK_OBJECT (main_dialog), "destroy",
                      GTK_SIGNAL_FUNC (gtk_main_quit),
		      NULL);

  gimp_help_init ();

  help_strings[SAVE]         = _("A unit definition will only be saved before "
				 "GIMP exits if this column is checked.");
  help_strings[IDENTIFIER]   = _("This string will be used to identify a "
				 "unit in GIMP's configuration files.");
  help_strings[FACTOR]       = _("How many units make up an inch.");
  help_strings[DIGITS]       = _("This field is a hint for numerical input "
				 "fields. It specifies how many decimal digits "
				 "the input field should provide to get "
				 "approximately the same accuracy as an "
				 "\"inch\" input field with two decimal "
				 "digits.");
  help_strings[SYMBOL]       = _("The unit's symbol if it has one (e.g. \"'\" "
				 "for inches). Use the unit's abbreviation if "
				 "it doesn't have a symbol.");
  help_strings[ABBREVIATION] = _("The unit's abbreviation (e.g. \"cm\" for "
				 "centimeters).");
  help_strings[SINGULAR]     = _("The unit's singular form.");
  help_strings[PLURAL]       = _("The unit's plural form.");

  /*  the main vbox  */
  main_vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 6);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (main_dialog)->vbox), main_vbox);
  gtk_widget_show (main_vbox);

  /*  the selection list  */
  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
                                  GTK_POLICY_NEVER,
                                  GTK_POLICY_ALWAYS);
  gtk_container_add (GTK_CONTAINER (main_vbox), scrolled_win);
  gtk_widget_show (scrolled_win);

  titles[SAVE]         = _("Saved");
  titles[IDENTIFIER]   = _("ID");
  titles[FACTOR]       = _("Factor");
  titles[DIGITS]       = _("Digits");
  titles[SYMBOL]       = _("Symbol");
  titles[ABBREVIATION] = _("Abbr.");
  titles[SINGULAR]     = _("Singular");
  titles[PLURAL]       = _("Plural");

  clist = gtk_clist_new_with_titles (NUM_FIELDS, titles);
  gtk_clist_set_shadow_type (GTK_CLIST (clist), GTK_SHADOW_IN);
  gtk_clist_set_selection_mode (GTK_CLIST (clist), GTK_SELECTION_BROWSE);
  gtk_clist_column_titles_passive (GTK_CLIST (clist));

  /*  eek, passive column titles don't show a tooltip, so the next 3 lines
   *  are useless...
   */
  for (i = 0; i < NUM_FIELDS; i++)
    gimp_help_set_help_data (gtk_clist_get_column_widget (GTK_CLIST (clist), i),
			     help_strings[i], NULL);

  gtk_widget_realize (main_dialog);
  style = gtk_widget_get_style (main_dialog);

  yes_pixmap =  gdk_pixmap_create_from_xpm_d (main_dialog->window,
					      &yes_mask,
					      &style->bg[GTK_STATE_NORMAL],
					      yes_xpm);

  no_pixmap =  gdk_pixmap_create_from_xpm_d (main_dialog->window,
					     &no_mask,
					     &style->bg[GTK_STATE_NORMAL],
					     no_xpm);

  color.red   = 65535;
  color.green = 50000;
  color.blue  = 50000;

  gdk_color_alloc (gtk_widget_get_colormap (main_dialog), &color);

  clist_init ();

  gtk_widget_set_usize (clist, -1, 200);

  gtk_container_add (GTK_CONTAINER (scrolled_win), clist);
  gtk_signal_connect (GTK_OBJECT (clist), "select_row",
                      GTK_SIGNAL_FUNC (select_row_callback),
                      NULL);
  gtk_widget_show (clist);

  hbox = gtk_hbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  button = gimp_pixmap_button_new (new_xpm, _("New Unit"));
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (new_callback),
		      NULL);
  gtk_widget_show (button);

  gimp_help_set_help_data (button, _("Create a new unit from scratch."), NULL);

  button = gimp_pixmap_button_new (duplicate_xpm, _("Duplicate Unit"));
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (duplicate_callback),
		      NULL);
  gtk_widget_show (button);

  gimp_help_set_help_data (button, _("Create a new unit with the currently "
				     "seleted unit as template."), NULL);

  button = gimp_pixmap_button_new (no_xpm, _("Don't Save Unit"));
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (delete_callback),
		      NULL);
  gtk_widget_show (button);

  gimp_help_set_help_data (button, _("Don't save the currently selected unit "
				     "before GIMP exits."), NULL);

  delete_button = button;

  button = gimp_pixmap_button_new (yes_xpm, _("Save Unit"));
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (undelete_callback),
		      NULL);
  gtk_widget_show (button);

  gimp_help_set_help_data (button, _("Save the currently selected unit "
				     "before GIMP exits."), NULL);

  undelete_button = button;

  gtk_clist_select_row (GTK_CLIST (clist), 0, 0);

  gtk_widget_show (main_dialog);

  gtk_main ();
  gimp_help_free ();
  gdk_flush ();

  gdk_pixmap_unref (yes_pixmap);
  gdk_bitmap_unref (yes_mask);
  gdk_pixmap_unref (no_pixmap);
  gdk_bitmap_unref (no_mask);
}
