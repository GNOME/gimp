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

#include "gimpdialog.h"
#include "gimpunitmenu.h"

#include "libgimp-intl.h"

/*  private functions  */
static const gchar * gimp_unit_menu_build_string (gchar     *format,
						  GimpUnit   unit);
static void          gimp_unit_menu_callback     (GtkWidget *widget,
						  gpointer   data);

enum
{
  UNIT_CHANGED,
  LAST_SIGNAL
};

static guint gimp_unit_menu_signals[LAST_SIGNAL] = { 0 };

static GtkOptionMenuClass *parent_class = NULL;

static void
gimp_unit_menu_destroy (GtkObject *object)
{
  GimpUnitMenu *gum;

  g_return_if_fail (object != NULL);
  g_return_if_fail (GIMP_IS_UNIT_MENU (object));

  gum = GIMP_UNIT_MENU (object);

  if (gum->format)
    g_free (gum->format);

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
gimp_unit_menu_class_init (GimpUnitMenuClass *class)
{
  GtkObjectClass *object_class;

  object_class = (GtkObjectClass*) class;

  parent_class = gtk_type_class (gtk_option_menu_get_type ());

  gimp_unit_menu_signals[UNIT_CHANGED] = 
    gtk_signal_new ("unit_changed",
		    GTK_RUN_FIRST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (GimpUnitMenuClass,
				       unit_changed),
		    gtk_signal_default_marshaller, GTK_TYPE_NONE, 0);

  gtk_object_class_add_signals (object_class, gimp_unit_menu_signals, 
				LAST_SIGNAL);

  class->unit_changed = NULL;

  object_class->destroy = gimp_unit_menu_destroy;
}

static void
gimp_unit_menu_init (GimpUnitMenu *gum)
{
  gum->format       = NULL;
  gum->unit         = GIMP_UNIT_PIXEL;
  gum->show_pixels  = FALSE;
  gum->show_percent = FALSE;
  gum->selection    = NULL;
  gum->clist        = NULL;
}

GtkType
gimp_unit_menu_get_type (void)
{
  static GtkType gum_type = 0;

  if (!gum_type)
    {
      GtkTypeInfo gum_info =
      {
	"GimpUnitMenu",
	sizeof (GimpUnitMenu),
	sizeof (GimpUnitMenuClass),
	(GtkClassInitFunc) gimp_unit_menu_class_init,
	(GtkObjectInitFunc) gimp_unit_menu_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL
      };

      gum_type = gtk_type_unique (gtk_option_menu_get_type (), &gum_info);
    }
  
  return gum_type;
}

/**
 * gimp_unit_menu_new:
 * @format: A printf-like format string which is used to create the unit
 *          strings.
 * @unit: The initially selected unit.
 * @show_pixels: #TRUE if the unit menu should contain an item for
 *               GIMP_UNIT_PIXEL.
 * @show_percent: #TRUE in the unit menu should contain an item for
 *                GIMP_UNIT_PERCENT.
 * @show_custom: #TRUE if the unit menu should contain a "More..." item for
 *               opening the user-defined-unit selection dialog.
 *
 * Creates a new #GimpUnitMenu widget.
 *
 * The @format string supports the following percent expansions:
 *
 * Returns: A pointer to the new #GimpUnitMenu widget.
 *
 */
GtkWidget *
gimp_unit_menu_new (gchar    *format,
		    GimpUnit  unit,
		    gboolean  show_pixels,
		    gboolean  show_percent,
		    gboolean  show_custom)
{
  GimpUnitMenu *gum;
  GtkWidget    *menu;
  GtkWidget    *menuitem;
  GimpUnit      u;

  g_return_val_if_fail ((unit >= GIMP_UNIT_PIXEL) &&
			(unit < gimp_unit_get_number_of_units ()) ||
			(unit == GIMP_UNIT_PERCENT), NULL);

  if ((unit >= gimp_unit_get_number_of_built_in_units ()) &&
      (unit != GIMP_UNIT_PERCENT))
    show_custom = TRUE;

  gum = gtk_type_new (gimp_unit_menu_get_type ());

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
	      gtk_menu_append (GTK_MENU (menu), menuitem);
	      gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
				  GTK_SIGNAL_FUNC (gimp_unit_menu_callback),
				  gum);
	      gtk_object_set_data (GTK_OBJECT (menuitem), "gimp_unit_menu",
				   (gpointer) GIMP_UNIT_PERCENT);
	      gtk_widget_show (menuitem);
	    }

	  if (show_pixels || show_percent)
	    {
	      menuitem = gtk_menu_item_new ();
	      gtk_menu_append (GTK_MENU (menu), menuitem);
	      gtk_widget_show (menuitem);
	    }
	}

      menuitem =
	gtk_menu_item_new_with_label (gimp_unit_menu_build_string (format, u));
      gtk_menu_append (GTK_MENU (menu), menuitem);
      gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
			  GTK_SIGNAL_FUNC (gimp_unit_menu_callback),
			  gum);
      gtk_object_set_data (GTK_OBJECT (menuitem), "gimp_unit_menu",
			   (gpointer) u);
      gtk_widget_show (menuitem);
    }

  if ((unit >= gimp_unit_get_number_of_built_in_units ()) &&
      (unit != GIMP_UNIT_PERCENT))
    {
      menuitem = gtk_menu_item_new ();
      gtk_menu_append (GTK_MENU (menu), menuitem);
      gtk_widget_show (menuitem);
      
      menuitem =
	gtk_menu_item_new_with_label (gimp_unit_menu_build_string (format, unit));
      gtk_menu_append (GTK_MENU (menu), menuitem);
      gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
			  GTK_SIGNAL_FUNC (gimp_unit_menu_callback),
			  gum);
      gtk_object_set_data (GTK_OBJECT (menuitem), "gimp_unit_menu",
			   (gpointer) unit);
      gtk_widget_show (menuitem);
    }

  if (show_custom)
    {
      menuitem = gtk_menu_item_new ();
      gtk_menu_append (GTK_MENU (menu), menuitem);
      gtk_widget_show (menuitem);

      menuitem =
	gtk_menu_item_new_with_label (_("More..."));
      gtk_menu_append (GTK_MENU (menu), menuitem);
      gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
			  GTK_SIGNAL_FUNC (gimp_unit_menu_callback),
			  gum);
      gtk_object_set_data (GTK_OBJECT (menuitem), "gimp_unit_menu",
			   (gpointer) (GIMP_UNIT_PERCENT + 1));
      gtk_widget_show (menuitem);
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
 * @gum: The unit menu you want to set the unit for.
 * @unit: The new unit.
 *
 * Sets a new #GimpUnit for the specified #GimpUnitMenu.
 *
 */
void
gimp_unit_menu_set_unit (GimpUnitMenu *gum,
			 GimpUnit      unit)
{
  GtkWidget *menuitem = NULL;
  GList     *items;
  gint       user_unit;

  g_return_if_fail (gum != NULL);
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
      gtk_menu_append (GTK_MENU (GTK_OPTION_MENU (gum)->menu), menuitem);
      gtk_menu_reorder_child (GTK_MENU (GTK_OPTION_MENU (gum)->menu),
			      menuitem, user_unit - 1);
      gtk_widget_show (menuitem);
      
      menuitem =
	gtk_menu_item_new_with_label (gimp_unit_menu_build_string (gum->format,
								   unit));
      gtk_menu_append (GTK_MENU (GTK_OPTION_MENU (gum)->menu), menuitem);
      gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
			  GTK_SIGNAL_FUNC (gimp_unit_menu_callback),
			  gum);
      gtk_object_set_data (GTK_OBJECT (menuitem), "gimp_unit_menu",
			   (gpointer) unit);
      gtk_menu_reorder_child (GTK_MENU (GTK_OPTION_MENU (gum)->menu),
			      menuitem, user_unit);
      gtk_widget_show (menuitem);
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
 *
 */
GimpUnit
gimp_unit_menu_get_unit (GimpUnitMenu *gum)
{
  g_return_val_if_fail (gum != NULL, GIMP_UNIT_INCH);
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
gimp_unit_menu_build_string (gchar    *format,
			     GimpUnit  unit)
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
gimp_unit_menu_selection_select_callback (GtkWidget *widget,
					  gpointer   data)
{
  GimpUnitMenu *gum;
  GimpUnit      unit;

  gum = GIMP_UNIT_MENU (data);

  if (gum->selection && GTK_CLIST (gum->clist)->selection)
    {
      unit = (GimpUnit)
	gtk_clist_get_row_data (GTK_CLIST (gum->clist),
				(gint) (GTK_CLIST (gum->clist)->selection->data));
      gimp_unit_menu_set_unit (gum, unit);
      gtk_signal_emit (GTK_OBJECT (gum),
		       gimp_unit_menu_signals[UNIT_CHANGED]);

      gtk_widget_destroy (gum->selection);
    }
}

/*  private function of gimp_unit_menu_callback ()  */
static void
gimp_unit_menu_create_selection (GimpUnitMenu *gum)
{
  GtkWidget *vbox;
  GtkWidget *scrolled_win;
  gchar     *titles[2];
  gchar     *row[2];
  GimpUnit   unit;
  gint       num_units;
  gint       unit_width;
  gint       factor_width;

  gum->selection =
    gimp_dialog_new (_("Unit Selection"), "unit_selection",
		     gimp_standard_help_func, "dialogs/unit_selection.html",
		     GTK_WIN_POS_MOUSE,
		     FALSE, TRUE, FALSE,

		     _("Select"), gimp_unit_menu_selection_select_callback,
		     gum, NULL, NULL, TRUE, FALSE,
		     _("Close"), gtk_widget_destroy,
		     NULL, 1, NULL, FALSE, TRUE,

		     NULL);

  gtk_signal_connect (GTK_OBJECT (gum->selection), "destroy",
                      GTK_SIGNAL_FUNC (gtk_widget_destroyed),
                      &gum->selection);

  gtk_signal_connect_object_while_alive (GTK_OBJECT (gum), "destroy",
					 GTK_SIGNAL_FUNC (gtk_widget_destroy),
					 GTK_OBJECT (gum->selection));
  gtk_signal_connect_object_while_alive (GTK_OBJECT (gum), "unmap",
					 GTK_SIGNAL_FUNC (gtk_widget_destroy),
					 GTK_OBJECT (gum->selection));

  /*  the main vbox  */
  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (gum->selection)->vbox), vbox);
  gtk_widget_show (vbox);

  /*  the selection list  */
  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
				  GTK_POLICY_NEVER,
				  GTK_POLICY_ALWAYS);
  gtk_container_add (GTK_CONTAINER (vbox), scrolled_win);
  gtk_widget_show (scrolled_win);

  titles[0] = _("Unit");
  titles[1] = _("Factor");
  gum->clist = gtk_clist_new_with_titles (2, titles);
  gtk_clist_set_shadow_type (GTK_CLIST (gum->clist), GTK_SHADOW_IN);
  gtk_clist_set_selection_mode (GTK_CLIST (gum->clist), GTK_SELECTION_BROWSE);
  gtk_clist_column_titles_passive (GTK_CLIST (gum->clist));
  /*  the unit lines  */
  num_units = gimp_unit_get_number_of_units ();
  for (unit = GIMP_UNIT_END; unit < num_units; unit++)
    {
      row[0] = g_strdup (gimp_unit_menu_build_string (gum->format, unit));
      row[1] = g_strdup (gimp_unit_menu_build_string ("(%f)", unit));

      gtk_clist_append (GTK_CLIST (gum->clist), row);
      gtk_clist_set_row_data (GTK_CLIST (gum->clist),
			      unit - GIMP_UNIT_END,
			      (gpointer) unit);

      g_free (row[0]);
      g_free (row[1]);
    }

  unit_width   = gtk_clist_optimal_column_width (GTK_CLIST (gum->clist), 0);
  factor_width = gtk_clist_optimal_column_width (GTK_CLIST (gum->clist), 1);

  gtk_clist_set_column_width (GTK_CLIST (gum->clist), 0, unit_width);
  gtk_clist_set_column_width (GTK_CLIST (gum->clist), 1, factor_width);

  gtk_widget_set_usize (gum->clist, -1, 150);

  gtk_container_add (GTK_CONTAINER (scrolled_win), gum->clist);
  gtk_widget_show (gum->clist);

  gtk_signal_connect (GTK_OBJECT (gum->clist), "destroy",
                      GTK_SIGNAL_FUNC (gtk_widget_destroyed),
                      &gum->clist);

  gtk_widget_show (vbox);
  gtk_widget_show (gum->selection);

  if (gum->unit >= GIMP_UNIT_END)
    {
      gtk_clist_select_row (GTK_CLIST (gum->clist), gum->unit - GIMP_UNIT_END, 0);
      gtk_clist_moveto (GTK_CLIST (gum->clist), gum->unit - GIMP_UNIT_END,
			0, 0.0, 0.0); 
    }
}

static void
gimp_unit_menu_callback (GtkWidget *widget,
			 gpointer   data)
{
  GimpUnitMenu *gum;
  GimpUnit      new_unit;

  gum = data;
  new_unit = (guint) gtk_object_get_data (GTK_OBJECT (widget),
					  "gimp_unit_menu"); 
  
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
  gtk_signal_emit (GTK_OBJECT (gum),
		   gimp_unit_menu_signals[UNIT_CHANGED]);
}
