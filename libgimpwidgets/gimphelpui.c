/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimphelpui.c
 * Copyright (C) 2000 Michael Natterer <mitch@gimp.org>
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

#include <gdk/gdkkeysyms.h>

#include <gtk/gtk.h>

#ifdef __GNUC__
#warning GTK_DISABLE_DEPRECATED
#endif
#undef GTK_DISABLE_DEPRECATED

#include <gtk/gtktipsquery.h>

#define GTK_DISABLE_DEPRECATED

#include "gimpwidgetstypes.h"

#include "gimpdialog.h"
#include "gimphelpui.h"


typedef enum
{
  GIMP_WIDGET_HELP_TYPE_HELP = 0xff
} GimpWidgetHelpType;


/*  local function prototypes  */

static const gchar * gimp_help_get_help_data         (GtkWidget      *widget,
                                                      GtkWidget     **help_widget);
static gboolean gimp_help_callback                   (GtkWidget      *widget,
                                                      GtkWidgetHelpType help_type,
                                                      GimpHelpFunc    help_func);
static gboolean gimp_help_tips_query_idle_show_help  (gpointer        data);
static gboolean gimp_help_tips_query_widget_selected (GtkWidget      *tips_query,
                                                      GtkWidget      *widget,
                                                      const gchar    *tip_text,
                                                      const gchar    *tip_private,
                                                      GdkEventButton *event,
                                                      gpointer        func_data);
static gboolean gimp_help_tips_query_idle_start      (gpointer        tips_query);


/*  local variables  */

static GtkTooltips *tool_tips  = NULL;
static GtkWidget   *tips_query = NULL;


/*  public functions  */

/**
 * gimp_help_init:
 *
 * This function initializes GIMP's help system.
 *
 * Currently it only creates a #GtkTooltips object with gtk_tooltips_new()
 * which will be used by gimp_help_set_help_data().
 **/
void
gimp_help_init (void)
{
  tool_tips = gtk_tooltips_new ();

  /* take ownership of the tooltips */
  g_object_ref (tool_tips);
  gtk_object_sink (GTK_OBJECT (tool_tips));
}

/**
 * gimp_help_free:
 *
 * This function frees the memory used by the #GtkTooltips created by
 * gimp_help_init().
 **/
void
gimp_help_free (void)
{
  g_object_unref (tool_tips);
  tool_tips = NULL;

  tips_query->parent = NULL;
  gtk_widget_destroy (tips_query);
  tips_query = NULL;
}

/**
 * gimp_help_enable_tooltips:
 *
 * This function calls gtk_tooltips_enable().
 **/
void
gimp_help_enable_tooltips (void)
{
  gtk_tooltips_enable (tool_tips);
}

/**
 * gimp_help_disable_tooltips:
 *
 * This function calls gtk_tooltips_disable().
 **/
void
gimp_help_disable_tooltips (void)
{
  gtk_tooltips_disable (tool_tips);
}

/**
 * gimp_help_connect:
 * @widget: The widget you want to connect the help accelerator for. Will
 *          be a #GtkWindow in most cases.
 * @help_func: The function which will be called if the user presses "F1".
 * @help_data: The data pointer which will be passed to @help_func.
 *
 * Note that this function is automatically called by all libgimp dialog
 * constructors. You only have to call it for windows/dialogs you created
 * "manually".
 **/
void
gimp_help_connect (GtkWidget    *widget,
		   GimpHelpFunc  help_func,
		   const gchar  *help_data)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));
  g_return_if_fail (help_func != NULL);

  /*  set up the help signals and tips query widget
   */
  if (! tips_query)
    {
      GtkBindingSet *binding_set;

      binding_set =
        gtk_binding_set_by_class (g_type_class_peek (GTK_TYPE_WIDGET));

      gtk_binding_entry_add_signal (binding_set, GDK_F1, 0,
				    "show_help", 1,
				    GTK_TYPE_WIDGET_HELP_TYPE,
				    GIMP_WIDGET_HELP_TYPE_HELP);
      gtk_binding_entry_add_signal (binding_set, GDK_KP_F1, 0,
				    "show_help", 1,
				    GTK_TYPE_WIDGET_HELP_TYPE,
				    GIMP_WIDGET_HELP_TYPE_HELP);

      tips_query = gtk_tips_query_new ();

      g_object_set (tips_query,
                    "emit_always", TRUE,
                    NULL);

      g_signal_connect (tips_query, "widget_selected",
			G_CALLBACK (gimp_help_tips_query_widget_selected),
			NULL);

      /*  FIXME: EEEEEEEEEEEEEEEEEEEEK, this is very ugly and forbidden...
       *  does anyone know a way to do this tips query stuff without
       *  having to attach to some parent widget???
       */
      tips_query->parent = widget;
      gtk_widget_realize (tips_query);
    }

  gimp_help_set_help_data (widget, NULL, help_data);

  g_signal_connect (widget, "show_help",
                    G_CALLBACK (gimp_help_callback),
                    help_func);

  gtk_widget_add_events (widget, GDK_BUTTON_PRESS_MASK);
}

/**
 * gimp_help_set_help_data:
 * @widget: The #GtkWidget you want to set a @tooltip and/or @help_data for.
 * @tooltip: The text for this widget's tooltip.
 * @help_data: The @help_data for the #GtkTipsQuery tooltips inspector.
 *
 * The reason why we don't use gtk_tooltips_set_tip() is that it's
 * impossible to set a @private_tip (aka @help_data) without a visible
 * @tooltip.
 *
 * This function can be called with @tooltip == #NULL. Use this feature
 * if you want to set a HTML help link for a widget which shouldn't have
 * a visible tooltip.
 *
 * You can e.g. set a @help_data string to a complete HTML page for a
 * container widget (e.g. a #GtkBox). For the widgets inside the box
 * you can set HTML anchors which point inside the container widget's
 * help page by setting @help_data strings starting with "#".
 *
 * If the tooltips inspector (Shift + "F1") is invoked and the user
 * clicks on one of the widgets which only contain a "#" link, the
 * help system will automatically ascend the widget hierarchy until it
 * finds another widget with @help_data attached and concatenates both
 * to a complete help path.
 **/
void
gimp_help_set_help_data (GtkWidget   *widget,
			 const gchar *tooltip,
			 const gchar *help_data)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  if (tooltip)
    {
      gtk_tooltips_set_tip (tool_tips, widget, tooltip, help_data);
    }
  else if (help_data)
    {
      g_object_set_data (G_OBJECT (widget), "gimp_help_data",
                         (gpointer) help_data);
    }
}

/**
 * gimp_context_help:
 *
 * This function invokes the #GtkTipsQuery tooltips inspector.
 *
 * The mouse cursor will turn turn into a question mark and the user can
 * click on any widget of the application which started the inspector.
 *
 * If the widget the user clicked on has a @help_data string attached
 * (see gimp_help_set_help_data()), the corresponding HTML page will
 * be displayed. Otherwise the help system will ascend the widget hierarchy
 * until it finds an attached @help_data string (which should be the
 * case at least for every window/dialog).
 **/
void
gimp_context_help (void)
{
  if (tips_query)
    gimp_help_callback (NULL, GTK_WIDGET_HELP_WHATS_THIS, NULL);
}


/*  private functions  */

static const gchar *
gimp_help_get_help_data (GtkWidget  *widget,
                         GtkWidget **help_widget)
{
  GtkTooltipsData *tooltips_data;
  gchar           *help_data = NULL;

  for (; widget; widget = widget->parent)
    {
      if ((tooltips_data = gtk_tooltips_data_get (widget)) &&
	  tooltips_data->tip_private)
	{
	  help_data = tooltips_data->tip_private;
	}
      else
	{
	  help_data = g_object_get_data (G_OBJECT (widget), "gimp_help_data");
	}

      if (help_data)
        {
          if (help_widget)
            *help_widget = widget;

          return (const gchar *) help_data;
        }
    }

  return NULL;
}

static gboolean
gimp_help_callback (GtkWidget          *widget,
		    GimpWidgetHelpType  help_type,
		    GimpHelpFunc        help_func)
{
  switch (help_type)
    {
    case GIMP_WIDGET_HELP_TYPE_HELP:
      if (help_func)
        {
          gchar *help_data;

          help_data = g_object_get_data (G_OBJECT (widget), "gimp_help_data");

	  (* help_func) (help_data);
        }
      return TRUE;

    case GTK_WIDGET_HELP_WHATS_THIS:
      if (! GTK_TIPS_QUERY (tips_query)->in_query)
	g_idle_add (gimp_help_tips_query_idle_start, tips_query);
      return TRUE;

    default:
      break;
    }

  return FALSE;
}

/*  Do all the actual GtkTipsQuery calls in idle functions and check for
 *  some widget holding a grab before starting the query because strange
 *  things happen if (1) the help browser pops up while the query has
 *  grabbed the pointer or (2) the query grabs the pointer while some
 *  other part of the gimp has grabbed it (e.g. a tool, eek)
 */

static gboolean
gimp_help_tips_query_idle_show_help (gpointer data)
{
  GtkWidget   *help_widget;
  const gchar *help_data = NULL;

  help_data = gimp_help_get_help_data (GTK_WIDGET (data), &help_widget);

  if (help_data)
    {
      if (help_data[0] == '#')
        {
          const gchar *help_index;

          help_index = help_data;
          help_data  = gimp_help_get_help_data (help_widget->parent, NULL);

          if (help_data)
            {
              gchar *help_text;

              help_text = g_strconcat (help_data, help_index, NULL);
              gimp_standard_help_func (help_text);
              g_free (help_text);
            }
        }
      else
        {
          gimp_standard_help_func (help_data);
        }
    }

  return FALSE;
}

static gboolean
gimp_help_tips_query_widget_selected (GtkWidget      *tips_query,
				      GtkWidget      *widget,
				      const gchar    *tip_text,
				      const gchar    *tip_private,
				      GdkEventButton *event,
				      gpointer        func_data)
{
  if (widget)
    g_idle_add (gimp_help_tips_query_idle_show_help, widget);

  return TRUE;
}

static gboolean
gimp_help_tips_query_idle_start (gpointer tips_query)
{
  if (! gtk_grab_get_current ())
    gtk_tips_query_start_query (GTK_TIPS_QUERY (tips_query));

  return FALSE;
}
