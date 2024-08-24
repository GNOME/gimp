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
 * <https://www.gnu.org/licenses/>.
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
 * @widget:            The widget you want to connect the help accelerator for.
 *                     Will be a #GtkWindow in most cases.
 * @tooltip: (nullable): The text for this widget's tooltip. For windows, you
 *                     usually want to set %NULL.
 * @help_func:         The function which will be called if the user presses "F1".
 * @help_id:           The @help_id which will be passed to @help_func.
 * @help_data:         The @help_data pointer which will be passed to @help_func.
 * @help_data_destroy: Destroy function for @help_data.
 *
 * Note that this function is automatically called by all libgimp dialog
 * constructors. You only have to call it for windows/dialogs you created
 * "manually".
 *
 * Most of the time, what you want to call for non-windows widgets is
 * simply [func@GimpUi.help_set_help_data]. Yet if you need to set up an
 * @help_func, call `gimp_help_connect` instead. Note that `gimp_help_set_help_data`
 * is implied, so you don't have to call it too.
 **/
void
gimp_help_connect (GtkWidget      *widget,
                   const gchar    *tooltip,
                   GimpHelpFunc    help_func,
                   const gchar    *help_id,
                   gpointer        help_data,
                   GDestroyNotify  help_data_destroy)
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

  gimp_help_set_help_data (widget, tooltip, help_id);

  g_object_set_data_full (G_OBJECT (widget), "gimp-help-data",
                          help_data, help_data_destroy);

  g_signal_connect (widget, "show-help",
                    G_CALLBACK (gimp_help_callback),
                    help_func);

  gtk_widget_add_events (widget, GDK_BUTTON_PRESS_MASK);
}

/**
 * gimp_help_set_help_data:
 * @widget:  The #GtkWidget you want to set a @tooltip and/or @help_id for.
 * @tooltip: (nullable): The text for this widget's tooltip (or %NULL).
 * @help_id: The @help_id for the #GtkTipsQuery tooltips inspector.
 *
 * The reason why we don't use gtk_widget_set_tooltip_text() is that
 * elements in the GIMP user interface should, if possible, also have
 * a @help_id set for context-sensitive help.
 *
 * This function can be called with %NULL for @tooltip. Use this feature
 * if you want to set a help link for a widget which shouldn't have
 * a visible tooltip.
 **/
void
gimp_help_set_help_data (GtkWidget   *widget,
                         const gchar *tooltip,
                         const gchar *help_id)
{
  g_return_if_fail (GTK_IS_WIDGET (widget));

  gtk_widget_set_tooltip_text (widget, tooltip);

  if (GTK_IS_MENU_ITEM (widget))
    gimp_help_menu_item_set_tooltip (widget, tooltip, help_id);

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

  gtk_widget_set_tooltip_markup (widget, tooltip);

  if (GTK_IS_MENU_ITEM (widget))
    gimp_help_menu_item_set_tooltip (widget, tooltip, help_id);

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
 * Returns: The #GQuark.
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

  g_object_set (widget, "has-tooltip", FALSE, NULL);

  g_signal_handlers_disconnect_by_func (widget,
                                        gimp_help_menu_item_query_tooltip,
                                        NULL);
  if (tooltip && help_id)
    g_signal_connect (widget, "query-tooltip",
                      G_CALLBACK (gimp_help_menu_item_query_tooltip),
                      NULL);
  if (tooltip)
    g_object_set (widget, "has-tooltip", TRUE, NULL);
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
      GdkDisplay    *display;
      GtkWidget     *invisible;
      GdkCursor     *cursor;
      GdkGrabStatus  status;

      invisible = gtk_invisible_new_for_screen (gtk_widget_get_screen (widget));
      gtk_widget_show (invisible);

      display = gtk_widget_get_display (invisible);

      cursor = gdk_cursor_new_for_display (display, GDK_QUESTION_ARROW);

      status = gdk_seat_grab (gdk_display_get_default_seat (display),
                              gtk_widget_get_window (invisible),
                              GDK_SEAT_CAPABILITY_ALL, TRUE,
                              cursor,
                              NULL, NULL, NULL);

      g_object_unref (cursor);

      if (status != GDK_GRAB_SUCCESS)
        {
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

/* find widget code shamelessly stolen from gtkinspector */

typedef struct
{
  gint x;
  gint y;
  gboolean found;
  gboolean first;
  GtkWidget *res_widget;
} FindWidgetData;

static void
find_widget (GtkWidget      *widget,
             FindWidgetData *data)
{
  GtkAllocation new_allocation;
  gint x_offset = 0;
  gint y_offset = 0;

  gtk_widget_get_allocation (widget, &new_allocation);

  if (data->found || !gtk_widget_get_mapped (widget))
    return;

  /* Note that in the following code, we only count the
   * position as being inside a WINDOW widget if it is inside
   * widget->window; points that are outside of widget->window
   * but within the allocation are not counted. This is consistent
   * with the way we highlight drag targets.
   */
  if (gtk_widget_get_has_window (widget))
    {
      new_allocation.x = 0;
      new_allocation.y = 0;
    }

  if (gtk_widget_get_parent (widget) && !data->first)
    {
      GdkWindow *window;

      window = gtk_widget_get_window (widget);
      while (window != gtk_widget_get_window (gtk_widget_get_parent (widget)))
        {
          gint tx, ty, twidth, theight;

          if (window == NULL)
            return;

          twidth = gdk_window_get_width (window);
          theight = gdk_window_get_height (window);

          if (new_allocation.x < 0)
            {
              new_allocation.width += new_allocation.x;
              new_allocation.x = 0;
            }
          if (new_allocation.y < 0)
            {
              new_allocation.height += new_allocation.y;
              new_allocation.y = 0;
            }

          if (new_allocation.x + new_allocation.width > twidth)
            new_allocation.width = twidth - new_allocation.x;
          if (new_allocation.y + new_allocation.height > theight)
            new_allocation.height = theight - new_allocation.y;

          gdk_window_get_position (window, &tx, &ty);
          new_allocation.x += tx;
          x_offset += tx;
          new_allocation.y += ty;
          y_offset += ty;

          window = gdk_window_get_parent (window);
        }
    }

  if ((data->x >= new_allocation.x) && (data->y >= new_allocation.y) &&
      (data->x < new_allocation.x + new_allocation.width) &&
      (data->y < new_allocation.y + new_allocation.height))
    {
      /* First, check if the drag is in a valid drop site in
       * one of our children
       */
      if (GTK_IS_CONTAINER (widget))
        {
          FindWidgetData new_data = *data;

          new_data.x -= x_offset;
          new_data.y -= y_offset;
          new_data.found = FALSE;
          new_data.first = FALSE;

          gtk_container_forall (GTK_CONTAINER (widget),
                                (GtkCallback)find_widget,
                                &new_data);

          data->found = new_data.found;
          if (data->found)
            data->res_widget = new_data.res_widget;
        }

      /* If not, and this widget is registered as a drop site, check to
       * emit "drag_motion" to check if we are actually in
       * a drop site.
       */
      if (!data->found)
        {
          data->found = TRUE;
          data->res_widget = widget;
        }
    }
}

static GtkWidget *
find_widget_at_pointer (GdkDevice *device)
{
  GtkWidget *widget = NULL;
  GdkWindow *pointer_window;
  gint x, y;
  FindWidgetData data;

  pointer_window = gdk_device_get_window_at_position (device, NULL, NULL);

  if (pointer_window)
    {
      gpointer widget_ptr;

      gdk_window_get_user_data (pointer_window, &widget_ptr);
      widget = widget_ptr;
    }

  if (widget)
    {
      gdk_window_get_device_position (gtk_widget_get_window (widget),
                                      device, &x, &y, NULL);

      data.x = x;
      data.y = y;
      data.found = FALSE;
      data.first = TRUE;

      find_widget (widget, &data);
      if (data.found)
        return data.res_widget;

      return widget;
    }

  return NULL;
}

static gboolean
gimp_context_help_button_press (GtkWidget      *widget,
                                GdkEventButton *bevent,
                                gpointer        data)
{
  GdkDisplay *display      = gtk_widget_get_display (widget);
  GdkSeat    *seat         = gdk_display_get_default_seat (display);
  GdkDevice  *device       = gdk_seat_get_pointer (seat);
  GtkWidget  *event_widget = find_widget_at_pointer (device);

  if (event_widget && bevent->button == 1 && bevent->type == GDK_BUTTON_PRESS)
    {
      gtk_grab_remove (widget);
      gdk_seat_ungrab (seat);
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
      gdk_seat_ungrab (gdk_display_get_default_seat (display));
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
