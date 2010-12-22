/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimphelpui.c
 * Copyright (C) 2000-2003 Michael Natterer <mitch@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "gimpwidgets.h"
#include "gimpwidgets-private.h"

#include "libgimp/libgimp-intl.h"


/**
 * SECTION: gimphelpui
 * @title: GimpHelpUI
 * @short_description: Functions for setting tooltip and help identifier
 *                     used by the GIMP help system.
 *
 * Functions for setting tooltip and help identifier used by the GIMP
 * help system.
 **/


typedef enum
{
  GIMP_WIDGET_HELP_TOOLTIP    = GTK_WIDGET_HELP_TOOLTIP,
  GIMP_WIDGET_HELP_WHATS_THIS = GTK_WIDGET_HELP_WHATS_THIS,
  GIMP_WIDGET_HELP_TYPE_HELP  = 0xff
} GimpWidgetHelpType;


/*  local variables  */

static gboolean tooltips_enabled       = TRUE;
static gboolean tooltips_enable_called = FALSE;


/*  local function prototypes  */

static const gchar * gimp_help_get_help_data        (GtkWidget      *widget,
                                                     GtkWidget     **help_widget,
                                                     gpointer       *ret_data);
static gboolean   gimp_help_callback                (GtkWidget      *widget,
                                                     GimpWidgetHelpType help_type,
                                                     GimpHelpFunc    help_func);

static void       gimp_help_menu_item_set_tooltip   (GtkWidget      *widget,
                                                     const gchar    *tooltip,
                                                     const gchar    *help_id);
static gboolean   gimp_help_menu_item_query_tooltip (GtkWidget      *widget,
                                                     gint            x,
                                                     gint            y,
                                                     gboolean        keyboard_mode,
                                                     GtkTooltip     *tooltip);
static gboolean   gimp_context_help_idle_start      (gpointer        widget);
static gboolean   gimp_context_help_button_press    (GtkWidget      *widget,
                                                     GdkEventButton *bevent,
                                                     gpointer        data);
static gboolean   gimp_context_help_key_press       (GtkWidget      *widget,
                                                     GdkEventKey    *kevent,
                                                     gpointer        data);
static gboolean   gimp_context_help_idle_show_help  (gpointer        data);


/*  public functions  */

/**
 * gimp_help_enable_tooltips:
 *
 * Enable tooltips to be shown in the GIMP user interface.
 *
 * As a plug-in author, you don't need to care about this as this
 * function is called for you from gimp_ui_init(). This ensures that
 * the user setting from the GIMP preferences dialog is respected in
 * all plug-in dialogs.
 **/
void
gimp_help_enable_tooltips (void)
{
  if (! tooltips_enable_called)
    {
      tooltips_enable_called = TRUE;
      tooltips_enabled       = TRUE;
    }
}

/**
 * gimp_help_disable_tooltips:
 *
 * Disable tooltips to be shown in the GIMP user interface.
 *
 * As a plug-in author, you don't need to care about this as this
 * function is called for you from gimp_ui_init(). This ensures that
 * the user setting from the GIMP preferences dialog is respected in
 * all plug-in dialogs.
 **/
void
gimp_help_disable_tooltips (void)
{
  if (! tooltips_enable_called)
    {
      tooltips_enable_called = TRUE;
      tooltips_enabled       = FALSE;
    }
}

/**
 * gimp_standard_help_func:
 * @help_id:   A unique help identifier.
 * @help_data: The @help_data passed to gimp_help_connect().
 *
 * This is the standard GIMP help function which does nothing but calling
 * gimp_help(). It is the right function to use in almost all cases.
 **/
void
gimp_standard_help_func (const gchar *help_id,
                         gpointer     help_data)
{
  if (! _gimp_standard_help_func)
    {
      g_warning ("%s: you must call gimp_widgets_init() before using "
                 "the help system", G_STRFUNC);
      return;
    }

  (* _gimp_standard_help_func) (help_id, help_data);
}

/**
 * gimp_help_connect:
 * @widget: The widget you want to connect the help accelerator for. Will
 *          be a #GtkWindow in most cases.
 * @help_func: The function which will be called if the user presses "F1".
 * @help_id:   The @help_id which will be passed to @help_func.
 * @help_data: The @help_data pointer which will be passed to @help_func.
 *
 * Note that this function is automatically called by all libgimp dialog
 * constructors. You only have to call it for windows/dialogs you created
 * "manually".
 **/
void
gimp_help_connect (GtkWidget    *widget,
                   GimpHelpFunc  help_func,
                   const gchar  *help_id,
                   gpointer      help_data)
{
  static gboolean initialized = FALSE;

  g_return_if_fail (GTK_IS_WIDGET (widget));
  g_return_if_fail (help_func != NULL);

  /*  set up the help signals
   */
  if (! initialized)
    {
      GtkBindingSet *binding_set;

      binding_set =
        gtk_binding_set_by_class (g_type_class_peek (GTK_TYPE_WIDGET));

      gtk_binding_entry_add_signal (binding_set, GDK_KEY_F1, 0,
                                    "show-help", 1,
                                    GTK_TYPE_WIDGET_HELP_TYPE,
                                    GIMP_WIDGET_HELP_TYPE_HELP);
      gtk_binding_entry_add_signal (binding_set, GDK_KEY_KP_F1, 0,
                                    "show-help", 1,
                                    GTK_TYPE_WIDGET_HELP_TYPE,
                                    GIMP_WIDGET_HELP_TYPE_HELP);

      initialized = TRUE;
    }

  gimp_help_set_help_data (widget, NULL, help_id);

  g_object_set_data (G_OBJECT (widget), "gimp-help-data", help_data);

  g_signal_connect (widget, "show-help",
                    G_CALLBACK (gimp_help_callback),
                    help_func);

  gtk_widget_add_events (widget, GDK_BUTTON_PRESS_MASK);
}

/**
 * gimp_help_set_help_data:
 * @widget:  The #GtkWidget you want to set a @tooltip and/or @help_id for.
 * @tooltip: The text for this widget's tooltip (or %NULL).
 * @help_id: The @help_id for the #GtkTipsQuery tooltips inspector.
 *
 * The reason why we don't use gtk_widget_set_tooltip_text() is that
 * elements in the GIMP user interface should, if possible, also have
 * a @help_id set for context-sensitive help.
 *
 * This function can be called with #NULL for @tooltip. Use this feature
 * if you want to set a help link for a widget which shouldn't have
 * a visible tooltip.
 **/
void
gimp_help_set_help_data (GtkWidget   *widget,
                         const gchar *tooltip,
                         const gchar *help_id)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  if (tooltips_enabled)
    {
      gtk_widget_set_tooltip_text (widget, tooltip);

      if (GTK_IS_MENU_ITEM (widget))
        gimp_help_menu_item_set_tooltip (widget, tooltip, help_id);
    }

  g_object_set_qdata (G_OBJECT (widget), GIMP_HELP_ID, (gpointer) help_id);
}

/**
 * gimp_help_set_help_data_with_markup:
 * @widget:  The #GtkWidget you want to set a @tooltip and/or @help_id for.
 * @tooltip: The markup for this widget's tooltip (or %NULL).
 * @help_id: The @help_id for the #GtkTipsQuery tooltips inspector.
 *
 * Just like gimp_help_set_help_data(), but supports to pass text
 * which is marked up with <link linkend="PangoMarkupFormat">Pango
 * text markup language</link>.
 *
 * Since: 2.6
 **/
void
gimp_help_set_help_data_with_markup (GtkWidget   *widget,
                                     const gchar *tooltip,
                                     const gchar *help_id)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  if (tooltips_enabled)
    {
      gtk_widget_set_tooltip_markup (widget, tooltip);

      if (GTK_IS_MENU_ITEM (widget))
        gimp_help_menu_item_set_tooltip (widget, tooltip, help_id);
    }

  g_object_set_qdata (G_OBJECT (widget), GIMP_HELP_ID, (gpointer) help_id);
}

/**
 * gimp_context_help:
 * @widget: Any #GtkWidget on the screen.
 *
 * This function invokes the context help inspector.
 *
 * The mouse cursor will turn turn into a question mark and the user can
 * click on any widget of the application which started the inspector.
 *
 * If the widget the user clicked on has a @help_id string attached
 * (see gimp_help_set_help_data()), the corresponding help page will
 * be displayed. Otherwise the help system will ascend the widget hierarchy
 * until it finds an attached @help_id string (which should be the
 * case at least for every window/dialog).
 **/
void
gimp_context_help (GtkWidget *widget)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  gimp_help_callback (widget, GIMP_WIDGET_HELP_WHATS_THIS, NULL);
}

/**
 * gimp_help_id_quark:
 *
 * This function returns the #GQuark which should be used as key when
 * attaching help IDs to widgets and objects.
 *
 * Return value: The #GQuark.
 *
 * Since: 2.2
 **/
GQuark
gimp_help_id_quark (void)
{
  static GQuark quark = 0;

  if (! quark)
    quark = g_quark_from_static_string ("gimp-help-id");

  return quark;
}


/*  private functions  */

static const gchar *
gimp_help_get_help_data (GtkWidget  *widget,
                         GtkWidget **help_widget,
                         gpointer   *ret_data)
{
  const gchar *help_id   = NULL;
  gpointer     help_data = NULL;

  for (; widget; widget = gtk_widget_get_parent (widget))
    {
      help_id   = g_object_get_qdata (G_OBJECT (widget), GIMP_HELP_ID);
      help_data = g_object_get_data (G_OBJECT (widget), "gimp-help-data");

      if (help_id)
        {
          if (help_widget)
            *help_widget = widget;

          if (ret_data)
            *ret_data = help_data;

          return help_id;
        }
    }

  if (help_widget)
    *help_widget = NULL;

  if (ret_data)
    *ret_data = NULL;

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
          help_func (g_object_get_qdata (G_OBJECT (widget), GIMP_HELP_ID),
                     g_object_get_data (G_OBJECT (widget), "gimp-help-data"));
        }
      return TRUE;

    case GIMP_WIDGET_HELP_WHATS_THIS:
      g_idle_add (gimp_context_help_idle_start, widget);
      return TRUE;

    default:
      break;
    }

  return FALSE;
}

static void
gimp_help_menu_item_set_tooltip (GtkWidget   *widget,
                                 const gchar *tooltip,
                                 const gchar *help_id)
{
  g_return_if_fail (GTK_IS_MENU_ITEM (widget));

  if (tooltip && help_id)
    {
      g_object_set (widget, "has-tooltip", TRUE, NULL);

      g_signal_connect (widget, "query-tooltip",
                        G_CALLBACK (gimp_help_menu_item_query_tooltip),
                        NULL);
    }
  else if (! tooltip)
    {
      g_object_set (widget, "has-tooltip", FALSE, NULL);

      g_signal_handlers_disconnect_by_func (widget,
                                            gimp_help_menu_item_query_tooltip,
                                            NULL);
    }
}

static gboolean
gimp_help_menu_item_query_tooltip (GtkWidget  *widget,
                                   gint        x,
                                   gint        y,
                                   gboolean    keyboard_mode,
                                   GtkTooltip *tooltip)
{
  GtkWidget *vbox;
  GtkWidget *label;
  gchar     *text;
  gboolean   use_markup = TRUE;

  text = gtk_widget_get_tooltip_markup (widget);

  if (! text)
    {
      text = gtk_widget_get_tooltip_text (widget);
      use_markup = FALSE;
    }

  if (! text)
    return FALSE;

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);

  label = gtk_label_new (text);
  gtk_label_set_use_markup (GTK_LABEL (label), use_markup);
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, TRUE, 0);
  gtk_widget_show (label);

  g_free (text);

  label = gtk_label_new (_("Press F1 for more help"));
  gimp_label_set_attributes (GTK_LABEL (label),
                             PANGO_ATTR_STYLE, PANGO_STYLE_ITALIC,
                             PANGO_ATTR_SCALE, PANGO_SCALE_SMALL,
                             -1);
  gtk_label_set_xalign (GTK_LABEL (label), 1.0);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  gtk_tooltip_set_custom (tooltip, vbox);

  return TRUE;
}


/*  Do all the actual context help calls in idle functions and check for
 *  some widget holding a grab before starting the query because strange
 *  things happen if (1) the help browser pops up while the query has
 *  grabbed the pointer or (2) the query grabs the pointer while some
 *  other part of GIMP has grabbed it (e.g. a tool, eek)
 */

static gboolean
gimp_context_help_idle_start (gpointer widget)
{
  if (! gtk_grab_get_current ())
    {
      GtkWidget     *invisible;
      GdkCursor     *cursor;
      GdkGrabStatus  status;

      invisible = gtk_invisible_new_for_screen (gtk_widget_get_screen (widget));
      gtk_widget_show (invisible);

      cursor = gdk_cursor_new_for_display (gtk_widget_get_display (invisible),
                                           GDK_QUESTION_ARROW);

      status = gdk_pointer_grab (gtk_widget_get_window (invisible), TRUE,
                                 GDK_BUTTON_PRESS_MASK   |
                                 GDK_BUTTON_RELEASE_MASK |
                                 GDK_ENTER_NOTIFY_MASK   |
                                 GDK_LEAVE_NOTIFY_MASK,
                                 NULL, cursor,
                                 GDK_CURRENT_TIME);

      g_object_unref (cursor);

      if (status != GDK_GRAB_SUCCESS)
        {
          gtk_widget_destroy (invisible);
          return FALSE;
        }

      if (gdk_keyboard_grab (gtk_widget_get_window (invisible), TRUE,
                             GDK_CURRENT_TIME) != GDK_GRAB_SUCCESS)
        {
          gdk_display_pointer_ungrab (gtk_widget_get_display (invisible),
                                      GDK_CURRENT_TIME);
          gtk_widget_destroy (invisible);
          return FALSE;
        }

      gtk_grab_add (invisible);

      g_signal_connect (invisible, "button-press-event",
                        G_CALLBACK (gimp_context_help_button_press),
                        NULL);
      g_signal_connect (invisible, "key-press-event",
                        G_CALLBACK (gimp_context_help_key_press),
                        NULL);
    }

  return FALSE;
}

static gboolean
gimp_context_help_button_press (GtkWidget      *widget,
                                GdkEventButton *bevent,
                                gpointer        data)
{
  GtkWidget *event_widget = gtk_get_event_widget ((GdkEvent *) bevent);

  if (event_widget && bevent->button == 1 && bevent->type == GDK_BUTTON_PRESS)
    {
      GdkDisplay *display = gtk_widget_get_display (widget);

      gtk_grab_remove (widget);
      gdk_display_keyboard_ungrab (display, bevent->time);
      gdk_display_pointer_ungrab (display, bevent->time);
      gtk_widget_destroy (widget);

      if (event_widget != widget)
        g_idle_add (gimp_context_help_idle_show_help, event_widget);
    }

  return TRUE;
}

static gboolean
gimp_context_help_key_press (GtkWidget   *widget,
                             GdkEventKey *kevent,
                             gpointer     data)
{
  if (kevent->keyval == GDK_KEY_Escape)
    {
      GdkDisplay *display = gtk_widget_get_display (widget);

      gtk_grab_remove (widget);
      gdk_display_keyboard_ungrab (display, kevent->time);
      gdk_display_pointer_ungrab (display, kevent->time);
      gtk_widget_destroy (widget);
    }

  return TRUE;
}

static gboolean
gimp_context_help_idle_show_help (gpointer data)
{
  GtkWidget   *help_widget;
  const gchar *help_id   = NULL;
  gpointer     help_data = NULL;

  help_id = gimp_help_get_help_data (GTK_WIDGET (data), &help_widget,
                                     &help_data);

  if (help_id)
    gimp_standard_help_func (help_id, help_data);

  return FALSE;
}
