/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpquerybox.c
 * Copyright (C) 1999-2000 Michael Natterer <mitch@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"

#include "gimpwidgetstypes.h"

#include "gimpdialog.h"
#include "gimpquerybox.h"
#include "gimpsizeentry.h"
#include "gimpwidgets.h"

#include "libgimp/libgimp-intl.h"


/**
 * SECTION: gimpquerybox
 * @title: GimpQueryBox
 * @short_description: Some simple dialogs to enter a single int,
 *                     double, string or boolean value.
 * @see_also: #GimpSizeEntry, #GimpUnitMenu
 *
 * These functions provide simple dialogs for entering a single
 * string, integer, double, boolean or pixel size value.
 *
 * They return a pointer to a #GtkDialog which has to be shown with
 * gtk_widget_show() by the caller.
 *
 * The dialogs contain an entry widget for the kind of value they ask
 * for and "OK" and "Cancel" buttons. On "Cancel", all query boxes
 * except the boolean one silently destroy themselves. On "OK" the
 * user defined callback function is called and returns the entered
 * value.
 **/


/*
 *  String, integer, double and size query boxes
 */

typedef struct _QueryBox QueryBox;

struct _QueryBox
{
  GtkWidget *qbox;
  GtkWidget *vbox;
  GtkWidget *entry;
  GObject   *object;
  gulong     response_handler;
  GCallback  callback;
  gpointer   callback_data;
};


static QueryBox * create_query_box             (const gchar   *title,
                                                GtkWidget     *parent,
                                                GimpHelpFunc   help_func,
                                                const gchar   *help_id,
                                                GCallback      response_callback,
                                                const gchar   *icon_name,
                                                const gchar   *message,
                                                const gchar   *ok_button,
                                                const gchar   *cancel_button,
                                                GObject       *object,
                                                const gchar   *signal,
                                                GCallback      callback,
                                                gpointer       callback_data);

static void       query_box_disconnect         (QueryBox      *query_box);
static void       query_box_destroy            (QueryBox      *query_box);

static void       string_query_box_response    (GtkWidget     *widget,
                                                gint           response_id,
                                                QueryBox      *query_box);
static void       int_query_box_response       (GtkWidget     *widget,
                                                gint           response_id,
                                                QueryBox      *query_box);
static void       double_query_box_response    (GtkWidget     *widget,
                                                gint           response_id,
                                                QueryBox      *query_box);
static void       size_query_box_response      (GtkWidget     *widget,
                                                gint           response_id,
                                                QueryBox      *query_box);
static void       boolean_query_box_response   (GtkWidget     *widget,
                                                gint           response_id,
                                                QueryBox      *query_box);

static void       query_box_cancel_callback    (QueryBox      *query_box);


/*
 *  create a generic query box without any entry widget
 */
static QueryBox *
create_query_box (const gchar   *title,
                  GtkWidget     *parent,
                  GimpHelpFunc   help_func,
                  const gchar   *help_id,
                  GCallback      response_callback,
                  const gchar   *icon_name,
                  const gchar   *message,
                  const gchar   *ok_button,
                  const gchar   *cancel_button,
                  GObject       *object,
                  const gchar   *signal,
                  GCallback      callback,
                  gpointer       callback_data)
{
  QueryBox  *query_box;
  GtkWidget *hbox = NULL;
  GtkWidget *label;

  /*  make sure the object / signal passed are valid
   */
  g_return_val_if_fail (parent == NULL || GTK_IS_WIDGET (parent), NULL);
  g_return_val_if_fail (object == NULL || G_IS_OBJECT (object), NULL);
  g_return_val_if_fail (object == NULL || signal != NULL, NULL);

  query_box = g_slice_new0 (QueryBox);

  query_box->qbox = gimp_dialog_new (title, "gimp-query-box",
                                     parent, 0,
                                     help_func, help_id,

                                     cancel_button, GTK_RESPONSE_CANCEL,
                                     ok_button,     GTK_RESPONSE_OK,

                                     NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (query_box->qbox),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  query_box->response_handler =
    g_signal_connect (query_box->qbox, "response",
                      G_CALLBACK (response_callback),
                      query_box);

  g_signal_connect (query_box->qbox, "destroy",
                    G_CALLBACK (gtk_widget_destroyed),
                    &query_box->qbox);

  /*  if we are associated with an object, connect to the provided signal
   */
  if (object)
    {
      GClosure *closure;

      closure = g_cclosure_new_swap (G_CALLBACK (query_box_cancel_callback),
                                     query_box, NULL);
      g_object_watch_closure (G_OBJECT (query_box->qbox), closure);

      g_signal_connect_closure (object, signal, closure, FALSE);
    }

  if (icon_name)
    {
      GtkWidget *content_area;
      GtkWidget *image;

      content_area = gtk_dialog_get_content_area (GTK_DIALOG (query_box->qbox));

      hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
      gtk_container_set_border_width (GTK_CONTAINER (hbox), 12);
      gtk_box_pack_start (GTK_BOX (content_area), hbox, TRUE, TRUE, 0);
      gtk_widget_show (hbox);

      image = gtk_image_new_from_icon_name (icon_name, GTK_ICON_SIZE_DIALOG);
      gtk_misc_set_alignment (GTK_MISC (image), 0.5, 0.0);
      gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
      gtk_widget_show (image);
    }

  query_box->vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);

  g_object_set_data (G_OBJECT (query_box->qbox), "gimp-query-box-vbox",
                     query_box->vbox);

  if (hbox)
    {
      gtk_box_pack_start (GTK_BOX (hbox), query_box->vbox, FALSE, FALSE, 0);
    }
  else
    {
      GtkWidget *content_area;

      content_area = gtk_dialog_get_content_area (GTK_DIALOG (query_box->qbox));

      gtk_container_set_border_width (GTK_CONTAINER (query_box->vbox), 12);
      gtk_box_pack_start (GTK_BOX (content_area), query_box->vbox,
                          TRUE, TRUE, 0);
    }

  gtk_widget_show (query_box->vbox);

  if (message)
    {
      label = gtk_label_new (message);
      gtk_label_set_xalign (GTK_LABEL (label), 0.0);
      gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
      gtk_box_pack_start (GTK_BOX (query_box->vbox), label, FALSE, FALSE, 0);
      gtk_widget_show (label);
    }

  query_box->entry         = NULL;
  query_box->object        = object;
  query_box->callback      = callback;
  query_box->callback_data = callback_data;

  return query_box;
}

/**
 * gimp_query_string_box:
 * @title:     The query box dialog's title.
 * @parent:    The dialog's parent widget.
 * @help_func: The help function to show this dialog's help page.
 * @help_id:   A string identifying this dialog's help page.
 * @message:   A string which will be shown above the dialog's entry widget.
 * @initial:   The initial value.
 * @object:    The object this query box is associated with.
 * @signal:    The object's signal which will cause the query box to be closed.
 * @callback:  The function which will be called when the user selects "OK".
 * @data:      The callback's user data.
 *
 * Creates a new #GtkDialog that queries the user for a string value.
 *
 * Returns: A pointer to the new #GtkDialog.
 **/
GtkWidget *
gimp_query_string_box (const gchar             *title,
                       GtkWidget               *parent,
                       GimpHelpFunc             help_func,
                       const gchar             *help_id,
                       const gchar             *message,
                       const gchar             *initial,
                       GObject                 *object,
                       const gchar             *signal,
                       GimpQueryStringCallback  callback,
                       gpointer                 data)
{
  QueryBox  *query_box;
  GtkWidget *entry;

  query_box = create_query_box (title, parent, help_func, help_id,
                                G_CALLBACK (string_query_box_response),
                                "dialog-question",
                                message,
                                _("_OK"), _("_Cancel"),
                                object, signal,
                                G_CALLBACK (callback), data);

  if (! query_box)
    return NULL;

  entry = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (entry), initial ? initial : "");
  gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);
  gtk_box_pack_start (GTK_BOX (query_box->vbox), entry, FALSE, FALSE, 0);
  gtk_widget_grab_focus (entry);
  gtk_widget_show (entry);

  query_box->entry = entry;

  return query_box->qbox;
}

/**
 * gimp_query_int_box:
 * @title:     The query box dialog's title.
 * @parent:    The dialog's parent widget.
 * @help_func: The help function to show this dialog's help page.
 * @help_id:   A string identifying this dialog's help page.
 * @message:   A string which will be shown above the dialog's entry widget.
 * @initial:   The initial value.
 * @lower:     The lower boundary of the range of possible values.
 * @upper:     The upper boundray of the range of possible values.
 * @object:    The object this query box is associated with.
 * @signal:    The object's signal which will cause the query box to be closed.
 * @callback:  The function which will be called when the user selects "OK".
 * @data:      The callback's user data.
 *
 * Creates a new #GtkDialog that queries the user for an integer value.
 *
 * Returns: A pointer to the new #GtkDialog.
 **/
GtkWidget *
gimp_query_int_box (const gchar          *title,
                    GtkWidget            *parent,
                    GimpHelpFunc          help_func,
                    const gchar          *help_id,
                    const gchar          *message,
                    gint                  initial,
                    gint                  lower,
                    gint                  upper,
                    GObject              *object,
                    const gchar          *signal,
                    GimpQueryIntCallback  callback,
                    gpointer              data)
{
  QueryBox      *query_box;
  GtkWidget     *spinbutton;
  GtkAdjustment *adjustment;

  query_box = create_query_box (title, parent, help_func, help_id,
                                G_CALLBACK (int_query_box_response),
                                "dialog-question",
                                message,
                                _("_OK"), _("_Cancel"),
                                object, signal,
                                G_CALLBACK (callback), data);

  if (! query_box)
    return NULL;

  adjustment = (GtkAdjustment *)
    gtk_adjustment_new (initial, lower, upper, 1, 10, 0);
  spinbutton = gtk_spin_button_new (adjustment, 1.0, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_entry_set_activates_default (GTK_ENTRY (spinbutton), TRUE);
  gtk_box_pack_start (GTK_BOX (query_box->vbox), spinbutton, FALSE, FALSE, 0);
  gtk_widget_grab_focus (spinbutton);
  gtk_widget_show (spinbutton);

  query_box->entry = spinbutton;

  return query_box->qbox;
}

/**
 * gimp_query_double_box:
 * @title:     The query box dialog's title.
 * @parent:    The dialog's parent widget.
 * @help_func: The help function to show this dialog's help page.
 * @help_id:   A string identifying this dialog's help page.
 * @message:   A string which will be shown above the dialog's entry widget.
 * @initial:   The initial value.
 * @lower:     The lower boundary of the range of possible values.
 * @upper:     The upper boundray of the range of possible values.
 * @digits:    The number of decimal digits the #GtkSpinButton will provide.
 * @object:    The object this query box is associated with.
 * @signal:    The object's signal which will cause the query box to be closed.
 * @callback:  The function which will be called when the user selects "OK".
 * @data:      The callback's user data.
 *
 * Creates a new #GtkDialog that queries the user for a double value.
 *
 * Returns: A pointer to the new #GtkDialog.
 **/
GtkWidget *
gimp_query_double_box (const gchar             *title,
                       GtkWidget               *parent,
                       GimpHelpFunc             help_func,
                       const gchar             *help_id,
                       const gchar             *message,
                       gdouble                  initial,
                       gdouble                  lower,
                       gdouble                  upper,
                       gint                     digits,
                       GObject                 *object,
                       const gchar             *signal,
                       GimpQueryDoubleCallback  callback,
                       gpointer                 data)
{
  QueryBox      *query_box;
  GtkWidget     *spinbutton;
  GtkAdjustment *adjustment;

  query_box = create_query_box (title, parent, help_func, help_id,
                                G_CALLBACK (double_query_box_response),
                                "dialog-question",
                                message,
                                _("_OK"), _("_Cancel"),
                                object, signal,
                                G_CALLBACK (callback), data);

  if (! query_box)
    return NULL;

  adjustment = (GtkAdjustment *)
    gtk_adjustment_new (initial, lower, upper, 1, 10, 0);
  spinbutton = gtk_spin_button_new (adjustment, 1.0, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_entry_set_activates_default (GTK_ENTRY (spinbutton), TRUE);
  gtk_box_pack_start (GTK_BOX (query_box->vbox), spinbutton, FALSE, FALSE, 0);
  gtk_widget_grab_focus (spinbutton);
  gtk_widget_show (spinbutton);

  query_box->entry = spinbutton;

  return query_box->qbox;
}

/**
 * gimp_query_size_box:
 * @title:       The query box dialog's title.
 * @parent:      The dialog's parent widget.
 * @help_func:   The help function to show this dialog's help page.
 * @help_id:     A string identifying this dialog's help page.
 * @message:     A string which will be shown above the dialog's entry widget.
 * @initial:     The initial value.
 * @lower:       The lower boundary of the range of possible values.
 * @upper:       The upper boundray of the range of possible values.
 * @digits:      The number of decimal digits the #GimpSizeEntry provide in
 *               "pixel" mode.
 * @unit:        The unit initially shown by the #GimpUnitMenu.
 * @resolution:  The resolution (in dpi) which will be used for pixel/unit
 *               calculations.
 * @dot_for_dot: %TRUE if the #GimpUnitMenu's initial unit should be "pixels".
 * @object:      The object this query box is associated with.
 * @signal:      The object's signal which will cause the query box
 *               to be closed.
 * @callback:    The function which will be called when the user selects "OK".
 * @data:        The callback's user data.
 *
 * Creates a new #GtkDialog that queries the user for a size using a
 * #GimpSizeEntry.
 *
 * Returns: A pointer to the new #GtkDialog.
 **/
GtkWidget *
gimp_query_size_box (const gchar           *title,
                     GtkWidget             *parent,
                     GimpHelpFunc           help_func,
                     const gchar           *help_id,
                     const gchar           *message,
                     gdouble                initial,
                     gdouble                lower,
                     gdouble                upper,
                     gint                   digits,
                     GimpUnit               unit,
                     gdouble                resolution,
                     gboolean               dot_for_dot,
                     GObject               *object,
                     const gchar           *signal,
                     GimpQuerySizeCallback  callback,
                     gpointer               data)
{
  QueryBox  *query_box;
  GtkWidget *sizeentry;
  GtkWidget *spinbutton;

  query_box = create_query_box (title, parent, help_func, help_id,
                                G_CALLBACK (size_query_box_response),
                                "dialog-question",
                                message,
                                _("_OK"), _("_Cancel"),
                                object, signal,
                                G_CALLBACK (callback), data);

  if (! query_box)
    return NULL;

  sizeentry = gimp_size_entry_new (1, unit, "%p", TRUE, FALSE, FALSE, 12,
                                   GIMP_SIZE_ENTRY_UPDATE_SIZE);
  if (dot_for_dot)
    gimp_size_entry_set_unit (GIMP_SIZE_ENTRY (sizeentry), GIMP_UNIT_PIXEL);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (sizeentry), 0,
                                  resolution, FALSE);
  gimp_size_entry_set_refval_digits (GIMP_SIZE_ENTRY (sizeentry), 0, digits);
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (sizeentry), 0,
                                         lower, upper);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (sizeentry), 0, initial);

  spinbutton = gimp_size_entry_get_help_widget (GIMP_SIZE_ENTRY (sizeentry), 0);
  gtk_entry_set_activates_default (GTK_ENTRY (spinbutton), TRUE);

  gtk_box_pack_start (GTK_BOX (query_box->vbox), sizeentry, FALSE, FALSE, 0);
  gimp_size_entry_grab_focus (GIMP_SIZE_ENTRY (sizeentry));
  gtk_widget_show (sizeentry);

  query_box->entry = sizeentry;

  return query_box->qbox;
}

/**
 * gimp_query_boolean_box:
 * @title:        The query box dialog's title.
 * @parent:       The dialog's parent widget.
 * @help_func:    The help function to show this dialog's help page.
 * @help_id:      A string identifying this dialog's help page.
 * @icon_name:    An icon name to specify an icon to appear on the left
 *                on the dialog's message.
 * @message:      A string which will be shown in the query box.
 * @true_button:  The string to be shown in the dialog's left button.
 * @false_button: The string to be shown in the dialog's right button.
 * @object:       The object this query box is associated with.
 * @signal:       The object's signal which will cause the query box
 *                to be closed.
 * @callback:     The function which will be called when the user clicks one
 *                of the buttons.
 * @data:         The callback's user data.
 *
 * Creates a new #GtkDialog that asks the user to do a boolean decision.
 *
 * Returns: A pointer to the new #GtkDialog.
 **/
GtkWidget *
gimp_query_boolean_box (const gchar              *title,
                        GtkWidget                *parent,
                        GimpHelpFunc              help_func,
                        const gchar              *help_id,
                        const gchar              *icon_name,
                        const gchar              *message,
                        const gchar              *true_button,
                        const gchar              *false_button,
                        GObject                  *object,
                        const gchar              *signal,
                        GimpQueryBooleanCallback  callback,
                        gpointer                  data)
{
  QueryBox  *query_box;

  query_box = create_query_box (title, parent, help_func, help_id,
                                G_CALLBACK (boolean_query_box_response),
                                icon_name,
                                message,
                                true_button, false_button,
                                object, signal,
                                G_CALLBACK (callback), data);

  if (! query_box)
    return NULL;

  return query_box->qbox;
}


/*
 *  private functions
 */

static void
query_box_disconnect (QueryBox *query_box)
{
  gtk_widget_set_sensitive (query_box->qbox, FALSE);

  /*  disconnect the response callback to avoid that it may be run twice  */
  if (query_box->response_handler)
    {
      g_signal_handler_disconnect (query_box->qbox,
                                   query_box->response_handler);

      query_box->response_handler = 0;
    }

  /*  disconnect, if we are connected to some signal  */
  if (query_box->object)
    g_signal_handlers_disconnect_by_func (query_box->object,
                                          query_box_cancel_callback,
                                          query_box);
}

static void
query_box_destroy (QueryBox *query_box)
{
  /*  Destroy the box  */
  if (query_box->qbox)
    gtk_widget_destroy (query_box->qbox);

  g_slice_free (QueryBox, query_box);
}

static void
string_query_box_response (GtkWidget *widget,
                           gint       response_id,
                           QueryBox  *query_box)
{
  const gchar *string;

  query_box_disconnect (query_box);

  /*  Get the entry data  */
  string = gtk_entry_get_text (GTK_ENTRY (query_box->entry));

  /*  Call the user defined callback  */
  if (response_id == GTK_RESPONSE_OK)
    (* (GimpQueryStringCallback) query_box->callback) (query_box->qbox,
                                                       string,
                                                       query_box->callback_data);

  query_box_destroy (query_box);
}

static void
int_query_box_response (GtkWidget *widget,
                        gint       response_id,
                        QueryBox  *query_box)
{
  gint value;

  query_box_disconnect (query_box);

  /*  Get the spinbutton data  */
  value = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (query_box->entry));

  /*  Call the user defined callback  */
  if (response_id == GTK_RESPONSE_OK)
    (* (GimpQueryIntCallback) query_box->callback) (query_box->qbox,
                                                    value,
                                                    query_box->callback_data);

  query_box_destroy (query_box);
}

static void
double_query_box_response (GtkWidget *widget,
                           gint       response_id,
                           QueryBox  *query_box)
{
  gdouble value;

  query_box_disconnect (query_box);

  /*  Get the spinbutton data  */
  value = gtk_spin_button_get_value (GTK_SPIN_BUTTON (query_box->entry));

  /*  Call the user defined callback  */
  if (response_id == GTK_RESPONSE_OK)
    (* (GimpQueryDoubleCallback) query_box->callback) (query_box->qbox,
                                                       value,
                                                       query_box->callback_data);

  query_box_destroy (query_box);
}

static void
size_query_box_response (GtkWidget *widget,
                         gint       response_id,
                         QueryBox  *query_box)
{
  gdouble  size;
  GimpUnit unit;

  query_box_disconnect (query_box);

  /*  Get the sizeentry data  */
  size = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (query_box->entry), 0);
  unit = gimp_size_entry_get_unit (GIMP_SIZE_ENTRY (query_box->entry));

  /*  Call the user defined callback  */
  if (response_id == GTK_RESPONSE_OK)
    (* (GimpQuerySizeCallback) query_box->callback) (query_box->qbox,
                                                     size,
                                                     unit,
                                                     query_box->callback_data);

  query_box_destroy (query_box);
}

static void
boolean_query_box_response (GtkWidget *widget,
                            gint       response_id,
                            QueryBox  *query_box)
{
  query_box_disconnect (query_box);

  /*  Call the user defined callback  */
  (* (GimpQueryBooleanCallback) query_box->callback) (query_box->qbox,
                                                      (response_id ==
                                                       GTK_RESPONSE_OK),
                                                      query_box->callback_data);

  query_box_destroy (query_box);
}

static void
query_box_cancel_callback (QueryBox *query_box)
{
  query_box_disconnect (query_box);
  query_box_destroy (query_box);
}
