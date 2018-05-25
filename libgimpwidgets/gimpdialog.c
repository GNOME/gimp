/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdialog.c
 * Copyright (C) 2000-2003 Michael Natterer <mitch@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "gimpwidgetstypes.h"

#include "gimpdialog.h"
#include "gimphelpui.h"

#include "libgimp/libgimp-intl.h"


/**
 * SECTION: gimpdialog
 * @title: GimpDialog
 * @short_description: Constructors for #GtkDialog's and action_areas as
 *                     well as other dialog-related stuff.
 *
 * Constructors for #GtkDialog's and action_areas as well as other
 * dialog-related stuff.
 **/


enum
{
  PROP_0,
  PROP_HELP_FUNC,
  PROP_HELP_ID,
  PROP_PARENT
};


struct _GimpDialogPrivate
{
  GimpHelpFunc  help_func;
  gchar        *help_id;
  GtkWidget    *help_button;
};

#define GET_PRIVATE(obj) (((GimpDialog *) (obj))->priv)


static void       gimp_dialog_constructed  (GObject      *object);
static void       gimp_dialog_dispose      (GObject      *object);
static void       gimp_dialog_finalize     (GObject      *object);
static void       gimp_dialog_set_property (GObject      *object,
                                            guint         property_id,
                                            const GValue *value,
                                            GParamSpec   *pspec);
static void       gimp_dialog_get_property (GObject      *object,
                                            guint         property_id,
                                            GValue       *value,
                                            GParamSpec   *pspec);

static void       gimp_dialog_hide         (GtkWidget    *widget);
static gboolean   gimp_dialog_delete_event (GtkWidget    *widget,
                                            GdkEventAny  *event);

static void       gimp_dialog_close        (GtkDialog    *dialog);

static void       gimp_dialog_response     (GtkDialog    *dialog,
                                            gint          response_id);


G_DEFINE_TYPE (GimpDialog, gimp_dialog, GTK_TYPE_DIALOG)

#define parent_class gimp_dialog_parent_class

static gboolean show_help_button = TRUE;


static void
gimp_dialog_class_init (GimpDialogClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkDialogClass *dialog_class = GTK_DIALOG_CLASS (klass);

  object_class->constructed  = gimp_dialog_constructed;
  object_class->dispose      = gimp_dialog_dispose;
  object_class->finalize     = gimp_dialog_finalize;
  object_class->set_property = gimp_dialog_set_property;
  object_class->get_property = gimp_dialog_get_property;

  widget_class->hide         = gimp_dialog_hide;
  widget_class->delete_event = gimp_dialog_delete_event;

  dialog_class->close        = gimp_dialog_close;

  /**
   * GimpDialog:help-func:
   *
   * Since: 2.2
   **/
  g_object_class_install_property (object_class, PROP_HELP_FUNC,
                                   g_param_spec_pointer ("help-func",
                                                         "Help Func",
                                                         "The help function to call when F1 is hit",
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));

  /**
   * GimpDialog:help-id:
   *
   * Since: 2.2
   **/
  g_object_class_install_property (object_class, PROP_HELP_ID,
                                   g_param_spec_string ("help-id",
                                                        "Help ID",
                                                        "The help ID to pass to help-func",
                                                        NULL,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  /**
   * GimpDialog:parent:
   *
   * Since: 2.8
   **/
  g_object_class_install_property (object_class, PROP_PARENT,
                                   g_param_spec_object ("parent",
                                                        "Parent",
                                                        "The dialog's parent widget",
                                                        GTK_TYPE_WIDGET,
                                                        GIMP_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_type_class_add_private (klass, sizeof (GimpDialogPrivate));
}

static void
gimp_dialog_init (GimpDialog *dialog)
{
  dialog->priv = G_TYPE_INSTANCE_GET_PRIVATE (dialog,
                                              GIMP_TYPE_DIALOG,
                                              GimpDialogPrivate);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (gimp_dialog_response),
                    NULL);
}

static void
gimp_dialog_constructed (GObject *object)
{
  GimpDialogPrivate *private = GET_PRIVATE (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  if (private->help_func)
    gimp_help_connect (GTK_WIDGET (object),
                       private->help_func, private->help_id,
                       object);

  if (show_help_button && private->help_func && private->help_id)
    {
      private->help_button = gtk_dialog_add_button (GTK_DIALOG (object),
                                                    ("_Help"),
                                                    GTK_RESPONSE_HELP);
    }
}

static void
gimp_dialog_dispose (GObject *object)
{
  GdkDisplay *display = NULL;

  if (g_main_depth () == 0)
    {
      display = gtk_widget_get_display (GTK_WIDGET (object));
      g_object_ref (display);
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);

  if (display)
    {
      gdk_display_flush (display);
      g_object_unref (display);
    }
}

static void
gimp_dialog_finalize (GObject *object)
{
  GimpDialogPrivate *private = GET_PRIVATE (object);

  g_clear_pointer (&private->help_id, g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_dialog_set_property (GObject      *object,
                          guint         property_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  GimpDialogPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_HELP_FUNC:
      private->help_func = g_value_get_pointer (value);
      break;

    case PROP_HELP_ID:
      g_free (private->help_id);
      private->help_id = g_value_dup_string (value);
      gimp_help_set_help_data (GTK_WIDGET (object), NULL, private->help_id);
      break;

    case PROP_PARENT:
      {
        GtkWidget *parent = g_value_get_object (value);

        if (parent)
          {
            if (GTK_IS_WINDOW (parent))
              {
                gtk_window_set_transient_for (GTK_WINDOW (object),
                                              GTK_WINDOW (parent));
              }
            else
              {
                gtk_window_set_screen (GTK_WINDOW (object),
                                       gtk_widget_get_screen (parent));
                gtk_window_set_position (GTK_WINDOW (object),
                                         GTK_WIN_POS_MOUSE);
              }
          }
      }
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_dialog_get_property (GObject    *object,
                          guint       property_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  GimpDialogPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_HELP_FUNC:
      g_value_set_pointer (value, private->help_func);
      break;

    case PROP_HELP_ID:
      g_value_set_string (value, private->help_id);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_dialog_hide (GtkWidget *widget)
{
  /*  set focus to NULL so focus_out callbacks are invoked synchronously  */
  gtk_window_set_focus (GTK_WINDOW (widget), NULL);

  GTK_WIDGET_CLASS (parent_class)->hide (widget);
}

static gboolean
gimp_dialog_delete_event (GtkWidget   *widget,
                          GdkEventAny *event)
{
  return TRUE;
}

static void
gimp_dialog_close (GtkDialog *dialog)
{
  /* Synthesize delete_event to close dialog. */

  GtkWidget *widget = GTK_WIDGET (dialog);

  if (gtk_widget_get_window (widget))
    {
      GdkEvent *event = gdk_event_new (GDK_DELETE);

      event->any.window     = g_object_ref (gtk_widget_get_window (widget));
      event->any.send_event = TRUE;

      gtk_main_do_event (event);
      gdk_event_free (event);
    }
}

static void
gimp_dialog_response (GtkDialog *dialog,
                      gint       response_id)
{
  GimpDialogPrivate *private = GET_PRIVATE (dialog);
  GtkWidget         *widget  = gtk_dialog_get_widget_for_response (dialog,
                                                                   response_id);

  if (widget &&
      (! GTK_IS_BUTTON (widget) ||
       gtk_widget_get_focus_on_click (widget)))
    {
      gtk_widget_grab_focus (widget);
    }

  /*  if our own help button was activated, abort "response" and
   *  call our help callback.
   */
  if (response_id == GTK_RESPONSE_HELP &&
      widget      == private->help_button)
    {
      g_signal_stop_emission_by_name (dialog, "response");

      if (private->help_func)
        private->help_func (private->help_id, dialog);
    }
}


/**
 * gimp_dialog_new:
 * @title:        The dialog's title which will be set with
 *                gtk_window_set_title().
 * @role:         The dialog's @role which will be set with
 *                gtk_window_set_role().
 * @parent:       The @parent widget of this dialog.
 * @flags:        The @flags (see the #GtkDialog documentation).
 * @help_func:    The function which will be called if the user presses "F1".
 * @help_id:      The help_id which will be passed to @help_func.
 * @...:          A %NULL-terminated @va_list destribing the
 *                action_area buttons.
 *
 * Creates a new @GimpDialog widget.
 *
 * This function simply packs the action_area arguments passed in "..."
 * into a @va_list variable and passes everything to gimp_dialog_new_valist().
 *
 * For a description of the format of the @va_list describing the
 * action_area buttons see gtk_dialog_new_with_buttons().
 *
 * Returns: A #GimpDialog.
 **/
GtkWidget *
gimp_dialog_new (const gchar    *title,
                 const gchar    *role,
                 GtkWidget      *parent,
                 GtkDialogFlags  flags,
                 GimpHelpFunc    help_func,
                 const gchar    *help_id,
                 ...)
{
  GtkWidget *dialog;
  va_list    args;

  g_return_val_if_fail (parent == NULL || GTK_IS_WIDGET (parent), NULL);
  g_return_val_if_fail (title != NULL, NULL);
  g_return_val_if_fail (role != NULL, NULL);

  va_start (args, help_id);

  dialog = gimp_dialog_new_valist (title, role,
                                   parent, flags,
                                   help_func, help_id,
                                   args);

  va_end (args);

  return dialog;
}

/**
 * gimp_dialog_new_valist:
 * @title:        The dialog's title which will be set with
 *                gtk_window_set_title().
 * @role:         The dialog's @role which will be set with
 *                gtk_window_set_role().
 * @parent:       The @parent widget of this dialog or %NULL.
 * @flags:        The @flags (see the #GtkDialog documentation).
 * @help_func:    The function which will be called if the user presses "F1".
 * @help_id:      The help_id which will be passed to @help_func.
 * @args:         A @va_list destribing the action_area buttons.
 *
 * Creates a new @GimpDialog widget. If a GtkWindow is specified as
 * @parent then the dialog will be made transient for this window.
 *
 * For a description of the format of the @va_list describing the
 * action_area buttons see gtk_dialog_new_with_buttons().
 *
 * Returns: A #GimpDialog.
 **/
GtkWidget *
gimp_dialog_new_valist (const gchar    *title,
                        const gchar    *role,
                        GtkWidget      *parent,
                        GtkDialogFlags  flags,
                        GimpHelpFunc    help_func,
                        const gchar    *help_id,
                        va_list         args)
{
  GtkWidget *dialog;
  gboolean   use_header_bar;

  g_return_val_if_fail (title != NULL, NULL);
  g_return_val_if_fail (role != NULL, NULL);
  g_return_val_if_fail (parent == NULL || GTK_IS_WIDGET (parent), NULL);

  g_object_get (gtk_settings_get_default (),
                "gtk-dialogs-use-header", &use_header_bar,
                NULL);

  dialog = g_object_new (GIMP_TYPE_DIALOG,
                         "title",          title,
                         "role",           role,
                         "modal",          (flags & GTK_DIALOG_MODAL),
                         "help-func",      help_func,
                         "help-id",        help_id,
                         "parent",         parent,
                         "use-header-bar", use_header_bar,
                         NULL);

  if (parent)
    {
      if (flags & GTK_DIALOG_DESTROY_WITH_PARENT)
        g_signal_connect_object (parent, "destroy",
                                 G_CALLBACK (gimp_dialog_close),
                                 dialog, G_CONNECT_SWAPPED);
    }

  gimp_dialog_add_buttons_valist (GIMP_DIALOG (dialog), args);

  return dialog;
}

/**
 * gimp_dialog_add_button:
 * @dialog: The @dialog to add a button to.
 * @button_text: text of button, or stock ID.
 * @response_id: response ID for the button.
 *
 * This function is essentially the same as gtk_dialog_add_button()
 * except it ensures there is only one help button and automatically
 * sets the RESPONSE_OK widget as the default response.
 *
 * Return value: the button widget that was added.
 **/
GtkWidget *
gimp_dialog_add_button (GimpDialog  *dialog,
                        const gchar *button_text,
                        gint         response_id)
{
  GtkWidget *button;
  gboolean   use_header_bar;

  /*  hide the automatically added help button if another one is added  */
  if (response_id == GTK_RESPONSE_HELP)
    {
      GimpDialogPrivate *private = GET_PRIVATE (dialog);

      if (private->help_button)
        {
          gtk_widget_destroy (private->help_button);
          private->help_button = NULL;
        }
    }

  button = gtk_dialog_add_button (GTK_DIALOG (dialog), button_text,
                                  response_id);

  g_object_get (dialog,
                "use-header-bar", &use_header_bar,
                NULL);

  if (use_header_bar &&
      (response_id == GTK_RESPONSE_OK     ||
       response_id == GTK_RESPONSE_CANCEL ||
       response_id == GTK_RESPONSE_CLOSE))
    {
      GtkWidget *header = gtk_dialog_get_header_bar (GTK_DIALOG (dialog));

      if (response_id == GTK_RESPONSE_OK)
        gtk_dialog_set_default_response (GTK_DIALOG (dialog),
                                         GTK_RESPONSE_OK);

      gtk_container_child_set (GTK_CONTAINER (header), button,
                               "position", 0,
                               NULL);
    }

  return button;
}

/**
 * gimp_dialog_add_buttons:
 * @dialog: The @dialog to add buttons to.
 * @...: button_text-response_id pairs.
 *
 * This function is essentially the same as gtk_dialog_add_buttons()
 * except it calls gimp_dialog_add_button() instead of gtk_dialog_add_button()
 **/
void
gimp_dialog_add_buttons (GimpDialog *dialog,
                         ...)
{
  va_list args;

  va_start (args, dialog);

  gimp_dialog_add_buttons_valist (dialog, args);

  va_end (args);
}

/**
 * gimp_dialog_add_buttons_valist:
 * @dialog: The @dialog to add buttons to.
 * @args:   The buttons as va_list.
 *
 * This function is essentially the same as gimp_dialog_add_buttons()
 * except it takes a va_list instead of '...'
 **/
void
gimp_dialog_add_buttons_valist (GimpDialog *dialog,
                                va_list     args)
{
  const gchar *button_text;
  gint         response_id;

  g_return_if_fail (GIMP_IS_DIALOG (dialog));

  while ((button_text = va_arg (args, const gchar *)))
    {
      response_id = va_arg (args, gint);

      gimp_dialog_add_button (dialog, button_text, response_id);
    }
}


typedef struct
{
  GtkDialog *dialog;
  gint       response_id;
  GMainLoop *loop;
  gboolean   destroyed;
} RunInfo;

static void
run_shutdown_loop (RunInfo *ri)
{
  if (g_main_loop_is_running (ri->loop))
    g_main_loop_quit (ri->loop);
}

static void
run_unmap_handler (GtkDialog *dialog,
                   RunInfo   *ri)
{
  run_shutdown_loop (ri);
}

static void
run_response_handler (GtkDialog *dialog,
                      gint       response_id,
                      RunInfo   *ri)
{
  ri->response_id = response_id;

  run_shutdown_loop (ri);
}

static gint
run_delete_handler (GtkDialog   *dialog,
                    GdkEventAny *event,
                    RunInfo     *ri)
{
  run_shutdown_loop (ri);

  return TRUE; /* Do not destroy */
}

static void
run_destroy_handler (GtkDialog *dialog,
                     RunInfo   *ri)
{
  /* shutdown_loop will be called by run_unmap_handler */

  ri->destroyed = TRUE;
}

/**
 * gimp_dialog_run:
 * @dialog: a #GimpDialog
 *
 * This function does exactly the same as gtk_dialog_run() except it
 * does not make the dialog modal while the #GMainLoop is running.
 *
 * Return value: response ID
 **/
gint
gimp_dialog_run (GimpDialog *dialog)
{
  RunInfo ri = { NULL, GTK_RESPONSE_NONE, NULL };
  gulong  response_handler;
  gulong  unmap_handler;
  gulong  destroy_handler;
  gulong  delete_handler;

  g_return_val_if_fail (GIMP_IS_DIALOG (dialog), -1);

  g_object_ref (dialog);

  gtk_window_present (GTK_WINDOW (dialog));

  response_handler = g_signal_connect (dialog, "response",
                                       G_CALLBACK (run_response_handler),
                                       &ri);
  unmap_handler    = g_signal_connect (dialog, "unmap",
                                       G_CALLBACK (run_unmap_handler),
                                       &ri);
  delete_handler   = g_signal_connect (dialog, "delete-event",
                                       G_CALLBACK (run_delete_handler),
                                       &ri);
  destroy_handler  = g_signal_connect (dialog, "destroy",
                                       G_CALLBACK (run_destroy_handler),
                                       &ri);

  ri.loop = g_main_loop_new (NULL, FALSE);

  g_main_loop_run (ri.loop);

  g_main_loop_unref (ri.loop);

  ri.loop      = NULL;
  ri.destroyed = FALSE;

  if (!ri.destroyed)
    {
      g_signal_handler_disconnect (dialog, response_handler);
      g_signal_handler_disconnect (dialog, unmap_handler);
      g_signal_handler_disconnect (dialog, delete_handler);
      g_signal_handler_disconnect (dialog, destroy_handler);
    }

  g_object_unref (dialog);

  return ri.response_id;
}

/**
 * gimp_dialogs_show_help_button:
 * @show: whether a help button should be added when creating a GimpDialog
 *
 * This function is for internal use only.
 *
 * Since: 2.2
 **/
void
gimp_dialogs_show_help_button (gboolean  show)
{
  show_help_button = show ? TRUE : FALSE;
}
