/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpfilechooser.h
 * Copyright (C) 2025 Jehan
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

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"

#include "gimpicons.h"
#include "gimpwidgetstypes.h"

#include "gimpfilechooser.h"

#include "libgimp/libgimp-intl.h"


/**
 * SECTION: gimpfilechooser
 * @title: GimpFileChooser
 * @short_description: A widget allowing to select a file.
 *
 * The chooser contains an optional label and other interface allowing
 * to select files for different use cases.
 *
 * Since: 3.0
 **/


enum
{
  PROP_0,
  PROP_ACTION,
  PROP_LABEL,
  PROP_TITLE,
  PROP_FILE,
  N_PROPS
};

struct _GimpFileChooser
{
  GtkBox                 parent_instance;

  GFile                 *file;
  gchar                 *title;
  gchar                 *label;

  GimpFileChooserAction  action;
  GtkWidget             *label_widget;
  GtkWidget             *button;
  GtkWidget             *entry;

  GtkWidget             *dialog;

  gboolean               invalid_file;
};


static void             gimp_file_chooser_constructed             (GObject             *object);
static void             gimp_file_chooser_finalize                (GObject             *object);

static void             gimp_file_chooser_set_property            (GObject             *object,
                                                                   guint                property_id,
                                                                   const GValue        *value,
                                                                   GParamSpec          *pspec);
static void             gimp_file_chooser_get_property            (GObject             *object,
                                                                   guint                property_id,
                                                                   GValue              *value,
                                                                   GParamSpec          *pspec);

static void            gimp_file_chooser_button_selection_changed (GtkFileChooser      *widget,
                                                                   GimpFileChooser     *chooser);
static void            gimp_file_chooser_dialog_response          (GtkDialog           *dialog,
                                                                   gint                 response_id,
                                                                   GimpFileChooser     *chooser);
static void            gimp_file_chooser_button_clicked           (GtkButton           *button,
                                                                   GimpFileChooser     *chooser);
static void            gimp_file_chooser_entry_text_notify        (GtkEntry            *entry,
                                                                   const GParamSpec    *pspec,
                                                                   GimpFileChooser     *chooser);


static GParamSpec *file_button_props[N_PROPS] = { NULL, };

/* Note: I initially wanted to implement the GtkFileChooser interface,
 * but it looks like GTK made GtkFileChooserIface private. So it can't
 * be implemented outside of core GTK widgets.
 */
G_DEFINE_FINAL_TYPE (GimpFileChooser, gimp_file_chooser, GTK_TYPE_BOX)


static void
gimp_file_chooser_class_init (GimpFileChooserClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = gimp_file_chooser_constructed;
  object_class->finalize     = gimp_file_chooser_finalize;
  object_class->set_property = gimp_file_chooser_set_property;
  object_class->get_property = gimp_file_chooser_get_property;

  /**
   * GimpFileChooser:action:
   *
   * The action determining the chooser UI.
   *
   * Since: 3.0
   */
  file_button_props[PROP_ACTION] =
    g_param_spec_enum ("action",
                       "Action",
                       "The action determining the chooser UI",
                       GIMP_TYPE_FILE_CHOOSER_ACTION,
                       GTK_FILE_CHOOSER_ACTION_OPEN,
                       GIMP_PARAM_READWRITE |
                       G_PARAM_EXPLICIT_NOTIFY);

  /**
   * GimpFileChooser:label:
   *
   * Label text with mnemonic.
   *
   * Since: 3.0
   */
  file_button_props[PROP_LABEL] =
    g_param_spec_string ("label",
                         "Label",
                         "The label to be used next to the button",
                         NULL,
                         GIMP_PARAM_READWRITE |
                         G_PARAM_EXPLICIT_NOTIFY);

  /**
   * GimpFileChooser:title:
   *
   * The title to be used for the file selection popup dialog.
   * If %NULL, the "label" property is used instead.
   *
   * Since: 3.0
   */
  file_button_props[PROP_TITLE] =
    g_param_spec_string ("title",
                         "Title",
                         "The title to be used for the file selection popup dialog "
                         "and as placeholder text in file entry.",
                         "File Selection",
                         GIMP_PARAM_READWRITE |
                         G_PARAM_EXPLICIT_NOTIFY);

  /**
   * GimpFileChooser:file:
   *
   * The currently selected file.
   *
   * Since: 3.0
   */
  file_button_props[PROP_FILE] =
    gimp_param_spec_file ("file", "File",
                          "The currently selected file",
                          GIMP_FILE_CHOOSER_ACTION_ANY,
                          TRUE, NULL,
                          GIMP_PARAM_READWRITE |
                          G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class,
                                     N_PROPS, file_button_props);
}

static void
gimp_file_chooser_init (GimpFileChooser *chooser)
{
  gtk_orientable_set_orientation (GTK_ORIENTABLE (chooser),
                                  GTK_ORIENTATION_HORIZONTAL);
  gtk_box_set_spacing (GTK_BOX (chooser), 6);

  chooser->action = GIMP_FILE_CHOOSER_ACTION_OPEN;
  chooser->button = NULL;
  chooser->entry  = NULL;
  chooser->dialog = NULL;
  chooser->file   = NULL;

  chooser->invalid_file = FALSE;
}

static void
gimp_file_chooser_constructed (GObject *object)
{
  GimpFileChooser *chooser = GIMP_FILE_CHOOSER (object);

  chooser->label_widget = gtk_label_new (NULL);
  gtk_box_pack_start (GTK_BOX (chooser), chooser->label_widget, FALSE, FALSE, 0);
  if (chooser->label)
    gtk_label_set_text_with_mnemonic (GTK_LABEL (chooser->label_widget), chooser->label);
  gtk_label_set_xalign (GTK_LABEL (chooser->label_widget), 0.0);
  gtk_widget_set_visible (chooser->label_widget, chooser->label != NULL);

  gimp_file_chooser_set_action (chooser, chooser->action);

  G_OBJECT_CLASS (gimp_file_chooser_parent_class)->constructed (object);
}

static void
gimp_file_chooser_finalize (GObject *object)
{
  GimpFileChooser *chooser = GIMP_FILE_CHOOSER (object);

  g_clear_pointer (&chooser->title, g_free);
  g_clear_pointer (&chooser->label, g_free);
  g_clear_object (&chooser->file);

  G_OBJECT_CLASS (gimp_file_chooser_parent_class)->finalize (object);
}

static void
gimp_file_chooser_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  GimpFileChooser *chooser = GIMP_FILE_CHOOSER (object);

  switch (property_id)
    {
    case PROP_ACTION:
      gimp_file_chooser_set_action (chooser, g_value_get_enum (value));
      break;

    case PROP_LABEL:
      gimp_file_chooser_set_label (chooser, g_value_get_string (value));
      break;

    case PROP_TITLE:
      gimp_file_chooser_set_title (chooser, g_value_get_string (value));
      break;

    case PROP_FILE:
      gimp_file_chooser_set_file (chooser, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_file_chooser_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  GimpFileChooser *chooser = GIMP_FILE_CHOOSER (object);

  switch (property_id)
    {
    case PROP_ACTION:
      g_value_set_enum (value, chooser->action);
      break;

    case PROP_LABEL:
      g_value_set_string (value, chooser->label);
      break;

    case PROP_TITLE:
      g_value_set_string (value, chooser->title);
      break;

    case PROP_FILE:
      g_value_set_object (value, chooser->file);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

/**
 * gimp_file_chooser_new:
 * @action: the action determining the UI created for this widget.
 * @label: (nullable): Label or %NULL for no label.
 * @title: (nullable): Title of the dialog to use or %NULL to reuse @label.
 * @file:  (nullable): Initial file.
 *
 * Creates a new #GtkWidget that lets a user choose a file according to
 * @action.
 *
 * [enum@Gimp.FileChooserAction.ANY] is not a valid value for @action.
 *
 * Returns: A [class@GimpUi.FileChooser].
 *
 * Since: 3.0
 */
GtkWidget *
gimp_file_chooser_new (GimpFileChooserAction  action,
                       const gchar           *label,
                       const gchar           *title,
                       GFile                 *file)
{
  GtkWidget *chooser;

  g_return_val_if_fail (action != GIMP_FILE_CHOOSER_ACTION_ANY, NULL);
  g_return_val_if_fail (file == NULL || G_IS_FILE (file), NULL);

  chooser = g_object_new (GIMP_TYPE_FILE_CHOOSER,
                          "action", action,
                          "label",  label,
                          "title",  title,
                          "file",   file,
                          NULL);

  return chooser;
}

/**
 * gimp_file_chooser_get_action:
 * @chooser: A #GimpFileChooser
 *
 * Gets the current action.
 *
 * Returns: the action which determined the UI of @chooser.
 *
 * Since: 3.0
 */
GimpFileChooserAction
gimp_file_chooser_get_action (GimpFileChooser *chooser)
{
  g_return_val_if_fail (GIMP_IS_FILE_CHOOSER (chooser), GIMP_FILE_CHOOSER_ACTION_ANY);

  return chooser->action;
}

/**
 * gimp_file_chooser_set_action:
 * @chooser: A #GimpFileChooser
 * @action: Action to set.
 *
 * Changes how @chooser is set to select a file. It may completely
 * change the internal widget structure so you should not depend on a
 * specific widget composition.
 *
 * Warning: with GTK deprecations, we may have soon to change the
 * internal implementation. So this is all the more reason for you not
 * to rely on specific child widgets being present (e.g.: we use
 * currently [class@Gtk.FileChooserButton] internally but it was removed
 * in GTK4 so we will eventually replace it by custom code). We will
 * also likely move to native file dialogs at some point.
 *
 * [enum@Gimp.FileChooserAction.ANY] is not a valid value for @action.
 *
 * Since: 3.0
 */
void
gimp_file_chooser_set_action (GimpFileChooser       *chooser,
                              GimpFileChooserAction  action)
{
  gchar *uri_path = NULL;

  g_return_if_fail (GIMP_IS_FILE_CHOOSER (chooser));
  g_return_if_fail (action != GIMP_FILE_CHOOSER_ACTION_ANY);

  if (chooser->button)
    gtk_widget_destroy (chooser->button);
  if (chooser->entry)
    gtk_widget_destroy (chooser->entry);
  if (chooser->dialog)
    gtk_widget_destroy (chooser->dialog);
  chooser->button = NULL;
  chooser->entry  = NULL;
  chooser->dialog = NULL;

  switch (action)
    {
    case GIMP_FILE_CHOOSER_ACTION_OPEN:
    case GIMP_FILE_CHOOSER_ACTION_SELECT_FOLDER:
      chooser->button = gtk_file_chooser_button_new (chooser->title, (GtkFileChooserAction) action);
      gtk_box_pack_start (GTK_BOX (chooser), chooser->button, FALSE, FALSE, 0);
      g_signal_connect (chooser->button, "selection-changed",
                        G_CALLBACK (gimp_file_chooser_button_selection_changed),
                        chooser);
      gtk_widget_set_visible (chooser->button, TRUE);

      gtk_label_set_mnemonic_widget (GTK_LABEL (chooser->label_widget), chooser->button);
      break;
    case GIMP_FILE_CHOOSER_ACTION_SAVE:
    case GIMP_FILE_CHOOSER_ACTION_CREATE_FOLDER:
      chooser->entry = gtk_entry_new ();
      gtk_box_pack_start (GTK_BOX (chooser), chooser->entry, TRUE, TRUE, 0);
      if (chooser->file)
        {
          uri_path = g_file_get_path (chooser->file);
          if (! uri_path)
            uri_path = g_file_get_uri (chooser->file);
        }
      if (! uri_path)
        uri_path = g_strdup ("");
      gtk_entry_set_text (GTK_ENTRY (chooser->entry), uri_path);
      g_signal_connect (chooser->entry, "notify::text",
                        G_CALLBACK (gimp_file_chooser_entry_text_notify),
                        chooser);
      gtk_entry_set_placeholder_text (GTK_ENTRY (chooser->entry), chooser->title);
      gtk_widget_set_visible (chooser->entry, TRUE);

      chooser->button = gtk_button_new_from_icon_name (GIMP_ICON_FILE_MANAGER, GTK_ICON_SIZE_SMALL_TOOLBAR);
      gtk_box_pack_start (GTK_BOX (chooser), chooser->button, FALSE, FALSE, 0);
      gtk_widget_set_visible (chooser->button, TRUE);
      g_signal_connect (chooser->button, "clicked",
                        G_CALLBACK (gimp_file_chooser_button_clicked),
                        chooser);

      gtk_label_set_mnemonic_widget (GTK_LABEL (chooser->label_widget), chooser->entry);
      break;
    case GIMP_FILE_CHOOSER_ACTION_ANY:
      g_return_if_reached ();
    }

  chooser->action = action;
  gimp_param_spec_file_set_action (file_button_props[PROP_FILE], action);

  g_object_notify_by_pspec (G_OBJECT (chooser), file_button_props[PROP_ACTION]);

  g_free (uri_path);
}

/**
 * gimp_file_chooser_get_file:
 * @chooser: A #GimpFileChooser
 *
 * Gets the currently selected file.
 *
 * Returns: (transfer none): an internal copy of the file which must not be freed.
 *
 * Since: 3.0
 */
GFile *
gimp_file_chooser_get_file (GimpFileChooser *chooser)
{
  g_return_val_if_fail (GIMP_IS_FILE_CHOOSER (chooser), NULL);

  return chooser->file;
}

/**
 * gimp_file_chooser_set_file:
 * @chooser: A #GimpFileChooser
 * @file: File to set.
 *
 * Sets the currently selected file.
 *
 * Since: 3.0
 */
void
gimp_file_chooser_set_file (GimpFileChooser *chooser,
                            GFile           *file)
{
  GFile *current_file = NULL;
  gchar *uri_path     = NULL;

  g_return_if_fail (GIMP_IS_FILE_CHOOSER (chooser));
  g_return_if_fail (file == NULL || G_IS_FILE (file));

  current_file = chooser->file ? g_object_ref (chooser->file) : NULL;
  g_clear_object (&chooser->file);
  chooser->file = file ? g_object_ref (file) : NULL;

  if ((current_file != NULL && file == NULL) ||
      (file != NULL && current_file == NULL) ||
      (file != NULL && current_file != NULL && ! g_file_equal (file, current_file)))
    {
      switch (chooser->action)
        {
        case GTK_FILE_CHOOSER_ACTION_OPEN:
        case GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER:
          gtk_file_chooser_set_file (GTK_FILE_CHOOSER (chooser->button), file, NULL);
          break;
        case GTK_FILE_CHOOSER_ACTION_SAVE:
        case GTK_FILE_CHOOSER_ACTION_CREATE_FOLDER:
          if (chooser->dialog)
            gtk_file_chooser_set_file (GTK_FILE_CHOOSER (chooser->dialog), file, NULL);

          if (file)
            {
              uri_path = g_file_get_path (file);
              if (! uri_path)
                uri_path = g_file_get_uri (file);
            }
          if (! uri_path)
            uri_path = g_strdup ("");
          g_signal_handlers_block_by_func (chooser->entry,
                                           G_CALLBACK (gimp_file_chooser_entry_text_notify),
                                           chooser);
          if (! chooser->invalid_file)
            gtk_entry_set_text (GTK_ENTRY (chooser->entry), uri_path);
          g_signal_handlers_unblock_by_func (chooser->entry,
                                             G_CALLBACK (gimp_file_chooser_entry_text_notify),
                                             chooser);
          break;
        case GIMP_FILE_CHOOSER_ACTION_ANY:
          g_return_if_reached ();
        }
    }

  g_object_notify_by_pspec (G_OBJECT (chooser), file_button_props[PROP_FILE]);

  g_clear_object (&current_file);
  g_free (uri_path);
}

/**
 * gimp_file_chooser_get_label:
 * @chooser: A #GimpFileChooser
 *
 * Gets the current label text. A %NULL label means that the label
 * widget is hidden.
 *
 * Note: the label text may contain a mnemonic.
 *
 * Returns: (nullable): the label set.
 *
 * Since: 3.0
 */
const gchar *
gimp_file_chooser_get_label (GimpFileChooser *chooser)
{
  g_return_val_if_fail (GIMP_IS_FILE_CHOOSER (chooser), NULL);

  return chooser->label;
}

/**
 * gimp_file_chooser_set_label:
 * @chooser:          A [class@FileChooser].
 * @text: (nullable): Label text.
 *
 * Set the label text with mnemonic.
 *
 * Setting a %NULL label text will hide the label widget.
 *
 * Since: 3.0
 */
void
gimp_file_chooser_set_label (GimpFileChooser *chooser,
                             const gchar     *text)
{
  g_return_if_fail (GIMP_IS_FILE_CHOOSER (chooser));

  g_free (chooser->label);
  chooser->label = g_strdup (text);
  gtk_widget_set_visible (chooser->label_widget, text != NULL);

  if (chooser->label_widget)
    {
      if (text != NULL)
        gtk_label_set_text_with_mnemonic (GTK_LABEL (chooser->label_widget), text);

      gtk_widget_set_visible (chooser->label_widget, text != NULL);
    }

  g_object_notify_by_pspec (G_OBJECT (chooser), file_button_props[PROP_LABEL]);
}

/**
 * gimp_file_chooser_get_title:
 * @chooser: A #GimpFileChooser
 *
 * Gets the text currently used for the file dialog's title and for
 * entry's placeholder text.
 *
 * A %NULL value means that the file dialog uses default title and the
 * entry has no placeholder text.
 *
 * Returns: (nullable): the text used for the title of @chooser's dialog.
 *
 * Since: 3.0
 */
const gchar *
gimp_file_chooser_get_title (GimpFileChooser *chooser)
{
  g_return_val_if_fail (GIMP_IS_FILE_CHOOSER (chooser), NULL);

  return chooser->title;
}

/**
 * gimp_file_chooser_set_title:
 * @chooser:          A [class@FileChooser].
 * @text: (nullable): Dialog's title text.
 *
 * Set the text to be used for the file dialog's title and for entry's
 * placeholder text.
 *
 * Setting a %NULL title @text will mean that the file dialog will use a
 * generic title and there will be no placeholder text in the entry.
 *
 * Since: 3.0
 */
void
gimp_file_chooser_set_title (GimpFileChooser *chooser,
                             const gchar     *text)
{
  g_return_if_fail (GIMP_IS_FILE_CHOOSER (chooser));

  g_free (chooser->title);
  chooser->title = g_strdup (text);

  if (chooser->dialog)
    gtk_window_set_title (GTK_WINDOW (chooser->dialog), chooser->title);

  if (chooser->entry)
    gtk_entry_set_placeholder_text (GTK_ENTRY (chooser->entry), chooser->title);

  g_object_notify_by_pspec (G_OBJECT (chooser), file_button_props[PROP_TITLE]);
}

/**
 * gimp_file_chooser_get_label_widget:
 * @chooser: A [class@FileChooser].
 *
 * Returns the label widget. This can be useful for instance when
 * aligning dialog's widgets with a [class@Gtk.SizeGroup].
 *
 * Returns: (transfer none): the [class@Gtk.Widget] showing the label text.
 *
 * Since: 3.0
 */
GtkWidget *
gimp_file_chooser_get_label_widget (GimpFileChooser *chooser)
{
  g_return_val_if_fail (GIMP_IS_FILE_CHOOSER (chooser), NULL);

  return chooser->label_widget;
}


/* Private Functions */

static void
gimp_file_chooser_button_selection_changed (GtkFileChooser  *widget,
                                            GimpFileChooser *chooser)
{
  GFile *file;

  file = gtk_file_chooser_get_file (widget);
  g_signal_handlers_block_by_func (chooser->button,
                                   G_CALLBACK (gimp_file_chooser_button_selection_changed),
                                   chooser);
  gimp_file_chooser_set_file (chooser, file);
  g_signal_handlers_unblock_by_func (chooser->button,
                                     G_CALLBACK (gimp_file_chooser_button_selection_changed),
                                     chooser);
  g_clear_object (&file);
}

static void
gimp_file_chooser_dialog_response (GtkDialog       *dialog,
                                   gint             response_id,
                                   GimpFileChooser *chooser)
{
  GFile *file = NULL;

  switch (response_id)
    {
    case GTK_RESPONSE_OK:
      file = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (dialog));
      gimp_file_chooser_set_file (chooser, file);
      break;
    case GTK_RESPONSE_CANCEL:
    case GTK_RESPONSE_DELETE_EVENT:
    default:
      break;
    }

  gtk_widget_destroy (GTK_WIDGET (dialog));
  chooser->dialog = NULL;

  g_clear_object (&file);
}

static void
gimp_file_chooser_button_clicked (GtkButton       *button,
                                  GimpFileChooser *chooser)
{
  GtkWidget *toplevel;

  if (chooser->dialog)
    {
      gtk_window_present (GTK_WINDOW (chooser->dialog));
      return;
    }

  toplevel = gtk_widget_get_toplevel (GTK_WIDGET (chooser));
  chooser->dialog = gtk_file_chooser_dialog_new ((const gchar *) chooser->title,
                                                 GTK_WINDOW (toplevel),
                                                 (GtkFileChooserAction) chooser->action,
                                                 _("_OK"),     GTK_RESPONSE_OK,
                                                 _("_Cancel"), GTK_RESPONSE_CANCEL,
                                                 NULL);
  if (chooser->file)
    gtk_file_chooser_set_file (GTK_FILE_CHOOSER (chooser->dialog), chooser->file, NULL);

  g_signal_connect (chooser->dialog, "response",
                    G_CALLBACK (gimp_file_chooser_dialog_response),
                    chooser);
  gtk_widget_set_visible (chooser->dialog, TRUE);
}

static void
gimp_file_chooser_entry_text_notify (GtkEntry         *entry,
                                     const GParamSpec *pspec,
                                     GimpFileChooser  *chooser)
{
  GParamSpec *chooser_pspec;
  GFile      *file;
  GValue      value = G_VALUE_INIT;

  chooser_pspec = file_button_props[PROP_FILE];
  file  = g_file_parse_name (gtk_entry_get_text (entry));
  g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (chooser_pspec));
  g_value_set_object (&value, G_OBJECT (file));

  if (! g_param_value_validate (chooser_pspec, &value))
    {
      gimp_file_chooser_set_file (chooser, file);
      gtk_entry_set_icon_from_icon_name (entry, GTK_ENTRY_ICON_SECONDARY, NULL);
    }
  else
    {
      chooser->invalid_file = TRUE;
      gimp_file_chooser_set_file (chooser, NULL);
      chooser->invalid_file = FALSE;
      /* XXX When not validating, I initially wanted to set the entry
       * borders to the error_color from the current theme. But I
       * settled with a simpler icon, which works well too IMO.
       */
      gtk_entry_set_icon_from_icon_name (entry, GTK_ENTRY_ICON_SECONDARY, GIMP_ICON_WILBER_EEK);
    }

  g_value_unset (&value);
  g_clear_object (&file);
}
