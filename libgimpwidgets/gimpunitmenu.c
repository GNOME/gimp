/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1999 Peter Mattis and Spencer Kimball
 *
 * gimpunitmenu.c
 * Copyright (C) 1999 Michael Natterer <mitch@gimp.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"

#include "gimpwidgetstypes.h"

#include "gimpdialog.h"
#include "gimphelpui.h"
#include "gimpunitmenu.h"

#include "libgimp/libgimp-intl.h"


enum
{
  UNIT_CHANGED,
  LAST_SIGNAL
};

enum
{
  UNIT_COLUMN,
  FACTOR_COLUMN,
  DATA_COLUMN,
  NUM_COLUMNS
};


static void          gimp_unit_menu_class_init   (GimpUnitMenuClass *klass);
static void          gimp_unit_menu_init         (GimpUnitMenu      *gum);

static void          gimp_unit_menu_finalize     (GObject     *object);

static const gchar * gimp_unit_menu_build_string (const gchar *format,
						  GimpUnit     unit);
static void          gimp_unit_menu_callback     (GtkWidget   *widget,
						  gpointer     data);


static guint gimp_unit_menu_signals[LAST_SIGNAL] = { 0 };

static GtkOptionMenuClass *parent_class = NULL;


GType
gimp_unit_menu_get_type (void)
{
  static GType gum_type = 0;

  if (! gum_type)
    {
      static const GTypeInfo gum_info =
      {
        sizeof (GimpUnitMenuClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_unit_menu_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data     */
	sizeof (GimpUnitMenu),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_unit_menu_init,
      };

      gum_type = g_type_register_static (GTK_TYPE_OPTION_MENU,
                                         "GimpUnitMenu",
                                         &gum_info, 0);
    }
  
  return gum_type;
}

static void
gimp_unit_menu_class_init (GimpUnitMenuClass *klass)
{
  GObjectClass *object_class;

  object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  gimp_unit_menu_signals[UNIT_CHANGED] = 
    g_signal_new ("unit_changed",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpUnitMenuClass, unit_changed),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  object_class->finalize = gimp_unit_menu_finalize;

  klass->unit_changed    = NULL;
}

static void
gimp_unit_menu_init (GimpUnitMenu *gum)
{
  gum->format       = NULL;
  gum->unit         = GIMP_UNIT_PIXEL;
  gum->show_pixels  = FALSE;
  gum->show_percent = FALSE;
  gum->selection    = NULL;
  gum->tv           = NULL;
}

static void
gimp_unit_menu_finalize (GObject *object)
{
  GimpUnitMenu *gum;

  g_return_if_fail (GIMP_IS_UNIT_MENU (object));

  gum = GIMP_UNIT_MENU (object);

  if (gum->format)
    {
      g_free (gum->format);
      gum->format = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/**
 * gimp_unit_menu_new:
 * @format:       A printf-like format string which is used to create the unit
 *                strings.
 * @unit:         The initially selected unit.
 * @show_pixels:  #TRUE if the unit menu should contain an item for
 *                GIMP_UNIT_PIXEL.
 * @show_percent: #TRUE in the unit menu should contain an item for
 *                GIMP_UNIT_PERCENT.
 * @show_custom:  #TRUE if the unit menu should contain a "More..." item for
 *                opening the user-defined-unit selection dialog.
 *
 * Creates a new #GimpUnitMenu widget.
 *
 * The @format string supports the following percent expansions:
 *
 * Returns: A pointer to the new #GimpUnitMenu widget.
 **/
GtkWidget *
gimp_unit_menu_new (const gchar *format,
		    GimpUnit     unit,
		    gboolean     show_pixels,
		    gboolean     show_percent,
		    gboolean     show_custom)
{
  GimpUnitMenu *gum;
  GtkWidget    *menu;
  GtkWidget    *menuitem;
  GimpUnit      u;

  g_return_val_if_fail (((unit >= GIMP_UNIT_PIXEL) &&
			 (unit < gimp_unit_get_number_of_units ())) ||
			 (unit == GIMP_UNIT_PERCENT), NULL);

  if ((unit >= gimp_unit_get_number_of_built_in_units ()) &&
      (unit != GIMP_UNIT_PERCENT))
    show_custom = TRUE;

  gum = g_object_new (GIMP_TYPE_UNIT_MENU, NULL);

  gum->format       = g_strdup (format);
  gum->show_pixels  = show_pixels;
  gum->show_percent = show_percent;

  menu = gtk_menu_new ();
  for (u = show_pixels ? GIMP_UNIT_PIXEL : GIMP_UNIT_INCH;
       u < gimp_unit_get_number_of_built_in_units ();
       u++)
    {
      /*  special cases "pixels" and "percent"  */
      if (u == GIMP_UNIT_INCH)
	{
	  if (show_percent)
	    {
	      menuitem =
		gtk_menu_item_new_with_label
		(gimp_unit_menu_build_string (format, GIMP_UNIT_PERCENT));
	      gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
	      g_object_set_data (G_OBJECT (menuitem), "gimp_unit_menu",
                                 GINT_TO_POINTER (GIMP_UNIT_PERCENT));
	      gtk_widget_show (menuitem);

	      g_signal_connect (G_OBJECT (menuitem), "activate",
                                G_CALLBACK (gimp_unit_menu_callback),
                                gum);
	    }

	  if (show_pixels || show_percent)
	    {
	      menuitem = gtk_menu_item_new ();
	      gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
	      gtk_widget_set_sensitive (menuitem, FALSE);
	      gtk_widget_show (menuitem);
	    }
	}

      menuitem =
	gtk_menu_item_new_with_label (gimp_unit_menu_build_string (format, u));
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
      g_object_set_data (G_OBJECT (menuitem), "gimp_unit_menu",
                         GINT_TO_POINTER (u));
      gtk_widget_show (menuitem);

      g_signal_connect (G_OBJECT (menuitem), "activate",
                        G_CALLBACK (gimp_unit_menu_callback),
                        gum);
    }

  if ((unit >= gimp_unit_get_number_of_built_in_units ()) &&
      (unit != GIMP_UNIT_PERCENT))
    {
      menuitem = gtk_menu_item_new ();
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
      gtk_widget_set_sensitive (menuitem, FALSE);
      gtk_widget_show (menuitem);
      
      menuitem =
	gtk_menu_item_new_with_label (gimp_unit_menu_build_string (format, unit));
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
      g_object_set_data (G_OBJECT (menuitem), "gimp_unit_menu", 
                         GINT_TO_POINTER (unit));
      gtk_widget_show (menuitem);

      g_signal_connect (G_OBJECT (menuitem), "activate",
                        G_CALLBACK (gimp_unit_menu_callback),
                        gum);
    }

  if (show_custom)
    {
      menuitem = gtk_menu_item_new ();
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
      gtk_widget_set_sensitive (menuitem, FALSE);
      gtk_widget_show (menuitem);

      menuitem =
	gtk_menu_item_new_with_label (_("More..."));
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
      g_object_set_data (G_OBJECT (menuitem), "gimp_unit_menu",
                         GINT_TO_POINTER (GIMP_UNIT_PERCENT + 1));
      gtk_widget_show (menuitem);

      g_signal_connect (G_OBJECT (menuitem), "activate",
                        G_CALLBACK (gimp_unit_menu_callback),
                        gum);
    }

  gtk_option_menu_set_menu (GTK_OPTION_MENU (gum), menu);

  gum->unit = unit;
  gtk_option_menu_set_history (GTK_OPTION_MENU (gum),
			       (unit == GIMP_UNIT_PIXEL) ? 0 :
			       ((unit == GIMP_UNIT_PERCENT) ?
				(show_pixels ? 1 : 0) :
				(((show_pixels || show_percent) ? 2 : 0) +
				 ((show_pixels && show_percent) ? 1 : 0) +
				 ((unit < GIMP_UNIT_END) ?
				  (unit - 1) : GIMP_UNIT_END)))); 

  return GTK_WIDGET (gum);
}

/**
 * gimp_unit_menu_set_unit:
 * @gum:  The unit menu you want to set the unit for.
 * @unit: The new unit.
 *
 * Sets a new #GimpUnit for the specified #GimpUnitMenu.
 **/
void
gimp_unit_menu_set_unit (GimpUnitMenu *gum,
			 GimpUnit      unit)
{
  GtkWidget *menuitem = NULL;
  GList     *items;
  gint       user_unit;

  g_return_if_fail (GIMP_IS_UNIT_MENU (gum));
  g_return_if_fail (((unit >= GIMP_UNIT_PIXEL) &&
		     ((unit > GIMP_UNIT_PIXEL) || gum->show_pixels) &&
		     (unit < gimp_unit_get_number_of_units ())) ||
		    ((unit == GIMP_UNIT_PERCENT) && gum->show_percent));

  if (unit == gum->unit)
    return;

  items = GTK_MENU_SHELL (GTK_OPTION_MENU (gum)->menu)->children;
  user_unit = (GIMP_UNIT_END +
	       (((gum->show_pixels || gum->show_percent) ? 2 : 0) +
		((gum->show_pixels && gum->show_percent) ? 1 : 0)));

  if ((unit >= GIMP_UNIT_END) && (unit != GIMP_UNIT_PERCENT))
    {
      if ((g_list_length (items) - 3) >= user_unit)
	{
	  gtk_widget_destroy (GTK_WIDGET (g_list_nth_data (items,
							   user_unit - 1)));
	  gtk_widget_destroy (GTK_WIDGET (g_list_nth_data (items,
							   user_unit - 1)));
	}

      menuitem = gtk_menu_item_new ();
      gtk_menu_shell_append (GTK_MENU_SHELL (GTK_OPTION_MENU (gum)->menu),
			     menuitem);
      gtk_widget_set_sensitive (menuitem, FALSE);
      gtk_menu_reorder_child (GTK_MENU (GTK_OPTION_MENU (gum)->menu),
			      menuitem, user_unit - 1);
      gtk_widget_show (menuitem);
      
      menuitem =
	gtk_menu_item_new_with_label (gimp_unit_menu_build_string (gum->format,
								   unit));
      gtk_menu_shell_append (GTK_MENU_SHELL (GTK_OPTION_MENU (gum)->menu),
			     menuitem);
      g_object_set_data (G_OBJECT (menuitem), "gimp_unit_menu",
                         GINT_TO_POINTER (unit));
      gtk_menu_reorder_child (GTK_MENU (GTK_OPTION_MENU (gum)->menu),
			      menuitem, user_unit);
      gtk_widget_show (menuitem);

      g_signal_connect (G_OBJECT (menuitem), "activate",
                        G_CALLBACK (gimp_unit_menu_callback),
                        gum);
    }

  gum->unit = unit;
  gtk_option_menu_set_history (GTK_OPTION_MENU (gum),
			       (unit == GIMP_UNIT_PIXEL) ? 0 :
			       ((unit == GIMP_UNIT_PERCENT) ?
				(gum->show_pixels ? 1 : 0) :
				(((gum->show_pixels ||
				   gum->show_percent) ? 2 : 0) +
				 ((gum->show_pixels &&
				   gum->show_percent) ? 1 : 0) +
				 ((unit < GIMP_UNIT_END) ?
				  (unit - 1) : GIMP_UNIT_END))));
}

/**
 * gimp_unit_menu_get_unit:
 * @gum: The unit menu you want to know the unit of.
 *
 * Returns the #GimpUnit the user has selected from the #GimpUnitMenu.
 *
 * Returns: The unit the user has selected.
 **/
GimpUnit
gimp_unit_menu_get_unit (GimpUnitMenu *gum)
{
  g_return_val_if_fail (GIMP_IS_UNIT_MENU (gum), GIMP_UNIT_INCH);

  return gum->unit;
}

/*  most of the next two functions is stolen from app/gdisplay.c  */
static gint
print (gchar       *buf,
       gint         len,
       gint         start,
       const gchar *fmt,
       ...)
{
  va_list args;
  gint printed;

  va_start (args, fmt);

  printed = g_vsnprintf (buf + start, len - start, fmt, args);
  if (printed < 0)
    printed = len - start;

  va_end (args);

  return printed;
}

static const gchar *
gimp_unit_menu_build_string (const gchar *format,
			     GimpUnit     unit)
{
  static gchar buffer[64];
  gint i = 0;

  while (i < (sizeof (buffer) - 1) && *format)
    {
      switch (*format)
	{
	case '%':
	  format++;
	  switch (*format)
	    {
	    case 0:
	      g_warning ("unit-menu-format string ended within %%-sequence");
	      break;
	      
	    case '%':
	      buffer[i++] = '%';
	      break;

	    case 'f': /* factor (how many units make up an inch) */
	      i += print (buffer, sizeof (buffer), i, "%f",
			  gimp_unit_get_factor (unit));
	      break;

	    case 'y': /* symbol ("''" for inch) */
	      i += print (buffer, sizeof (buffer), i, "%s",
			  gimp_unit_get_symbol (unit));
	      break;

	    case 'a': /* abbreviation */
	      i += print (buffer, sizeof (buffer), i, "%s",
			  gimp_unit_get_abbreviation (unit));
	      break;

	    case 's': /* singular */
	      i += print (buffer, sizeof (buffer), i, "%s",
			  gimp_unit_get_singular (unit));
	      break;

	    case 'p': /* plural */
	      i += print (buffer, sizeof (buffer), i, "%s",
			  gimp_unit_get_plural (unit));
	      break;

	    default:
	      g_warning ("gimp_unit_menu_build_string(): "
			 "unit-menu-format contains unknown format sequence "
			 "'%%%c'", *format);
	      break;
	    }
	  break;

	default:
	  buffer[i++] = *format;
	  break;
	}
      
      format++;
    }

  buffer[MIN (i, sizeof (buffer) - 1)] = 0;

  return buffer;
}

/*  private callback of gimp_unit_menu_create_selection ()  */
static void
gimp_unit_menu_selection_ok_callback (GtkWidget *widget,
				      gpointer   data)
{
  GimpUnitMenu     *gum;
  GimpUnit          unit;
  GtkTreeSelection *sel;
  GtkTreeModel     *model;
  GtkTreeIter       iter;
  GValue            val = { 0, };

  gum = GIMP_UNIT_MENU (data);

  sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (gum->tv));
  if (gum->selection && gtk_tree_selection_get_selected (sel, &model, &iter))
    {
      gtk_tree_model_get_value(model, &iter, 2, &val);
      unit = (GimpUnit) g_value_get_int (&val);
      g_value_unset (&val);

      gimp_unit_menu_set_unit (gum, unit);
      g_signal_emit (G_OBJECT (gum),
                     gimp_unit_menu_signals[UNIT_CHANGED], 0);

      gtk_widget_destroy (gum->selection);
    }
}

static void
gimp_unit_menu_selection_row_activated_callback (GtkTreeView       *tv,
						 GtkTreePath       *path,
						 GtkTreeViewColumn *column,
						 gpointer           data)
{
  gimp_unit_menu_selection_ok_callback (NULL, data);
}

/*  private function of gimp_unit_menu_callback ()  */
static void
gimp_unit_menu_create_selection (GimpUnitMenu *gum)
{
  GtkWidget        *vbox;
  GtkWidget        *scrolled_win;
  GtkListStore     *list;
  GtkTreeSelection *sel;
  GtkTreeIter       iter;
  GtkTreePath      *path;
  GimpUnit          unit;
  gint              num_units;

  gum->selection =
    gimp_dialog_new (_("Unit Selection"), "unit_selection",
		     gimp_standard_help_func, "dialogs/unit_selection.html",
		     GTK_WIN_POS_MOUSE,
		     FALSE, TRUE, FALSE,

		     GTK_STOCK_CANCEL, gtk_widget_destroy,
		     NULL, 1, NULL, FALSE, TRUE,

		     GTK_STOCK_OK, gimp_unit_menu_selection_ok_callback,
		     gum, NULL, NULL, TRUE, FALSE,

		     NULL);

  g_object_add_weak_pointer (G_OBJECT (gum->selection),
                             (gpointer) &gum->selection);

  g_signal_connect_object (G_OBJECT (gum), "destroy",
                           G_CALLBACK (gtk_widget_destroy),
                           G_OBJECT (gum->selection), G_CONNECT_SWAPPED);
  g_signal_connect_object (G_OBJECT (gum), "unmap",
                           G_CALLBACK (gtk_widget_destroy),
                           G_OBJECT (gum->selection), G_CONNECT_SWAPPED);

  /*  the main vbox  */
  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (gum->selection)->vbox), vbox);
  gtk_widget_show (vbox);

  /*  the selection list  */
  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_win),
				       GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
				  GTK_POLICY_NEVER,
				  GTK_POLICY_ALWAYS);
  gtk_container_add (GTK_CONTAINER (vbox), scrolled_win);
  gtk_widget_show (scrolled_win);

  list = gtk_list_store_new (NUM_COLUMNS, G_TYPE_STRING, G_TYPE_STRING,
			     G_TYPE_INT);
  gum->tv = gtk_tree_view_new_with_model (GTK_TREE_MODEL (list));
  g_object_unref (list);

  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (gum->tv),
					       -1, _("Unit"),
					       gtk_cell_renderer_text_new (),
					       "text", UNIT_COLUMN, NULL);
  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (gum->tv),
					       -1, _("Factor"),
					       gtk_cell_renderer_text_new (),
					       "text", FACTOR_COLUMN, NULL);

  /*  the unit lines  */
  num_units = gimp_unit_get_number_of_units ();
  for (unit = GIMP_UNIT_END; unit < num_units; unit++)
    {
      gtk_list_store_append (list, &iter);
      gtk_list_store_set (list, &iter,
			  UNIT_COLUMN,
			  gimp_unit_menu_build_string (gum->format, unit),
			  -1);
      gtk_list_store_set (list, &iter,
			  FACTOR_COLUMN,
			  gimp_unit_menu_build_string ("(%f)", unit),
			  -1);
      gtk_list_store_set (list, &iter, DATA_COLUMN, unit, -1);
    }

  gtk_widget_set_size_request (gum->tv, -1, 150);

  gtk_container_add (GTK_CONTAINER (scrolled_win), gum->tv);

  g_signal_connect (G_OBJECT (gum->tv), "row_activated",
                    G_CALLBACK (gimp_unit_menu_selection_row_activated_callback),
                    gum);

  gtk_widget_show (gum->tv);

  g_signal_connect (G_OBJECT (gum->tv), "destroy",
                    G_CALLBACK (gtk_widget_destroyed),
                    &gum->tv);

  gtk_widget_show (vbox);
  gtk_widget_show (gum->selection);

  if (gum->unit >= GIMP_UNIT_END)
    {
      path = gtk_tree_path_new ();
      gtk_tree_path_append_index (path, gum->unit - GIMP_UNIT_END);

      sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (gum->tv));
      gtk_tree_selection_select_path (sel, path);

      gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (gum->tv), path, NULL,
				    FALSE, 0.0, 0.0);
    }
}

static void
gimp_unit_menu_callback (GtkWidget *widget,
			 gpointer   data)
{
  GimpUnitMenu *gum;
  GimpUnit      new_unit;

  gum = data;
  new_unit = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (widget),
                                                  "gimp_unit_menu"));
  
  if (gum->unit == new_unit)
    return;

  /*  was "More..." selected?  */
  if (new_unit == (GIMP_UNIT_PERCENT + 1))
    {
      gtk_option_menu_set_history (GTK_OPTION_MENU (gum),
				   (gum->unit == GIMP_UNIT_PIXEL) ? 0 :
				   ((gum->unit == GIMP_UNIT_PERCENT) ?
				    (gum->show_pixels ? 1 : 0) :
				    ((gum->show_pixels ||
				      gum->show_percent ? 2 : 0) +
				     (gum->show_pixels &&
				      gum->show_percent ? 1 : 0) +
				     ((gum->unit < GIMP_UNIT_END) ?
				      gum->unit - 1 : GIMP_UNIT_END))));
      if (! gum->selection)
	gimp_unit_menu_create_selection (gum);
      return;
    }
  else if (gum->selection)
    {
      gtk_widget_destroy (gum->selection);
    }

  gimp_unit_menu_set_unit (gum, new_unit);
  g_signal_emit (G_OBJECT (gum),
                 gimp_unit_menu_signals[UNIT_CHANGED], 0);
}
