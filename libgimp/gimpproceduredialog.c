/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpproceduredialog.c
 * Copyright (C) 2019 Michael Natterer <mitch@gimp.org>
 * Copyright (C) 2020 Jehan
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "libgimp/gimppropwidgets.h"

#include "gimp.h"
#include "gimppropwidgets.h"
#include "gimpui.h"

#include "gimpprocedureconfig-private.h"

#include "libgimp-intl.h"


enum
{
  PROP_0,
  PROP_PROCEDURE,
  PROP_CONFIG,

  /* From here the overridden properties. */
  PROP_TITLE,

  N_PROPS
};

#define RESPONSE_RESET 1


typedef struct _GimpProcedureDialogPrivate
{
  GimpProcedure       *procedure;
  GimpProcedureConfig *config;
  GimpProcedureConfig *initial_config;

  GtkWidget           *ok_button;
  GtkWidget           *reset_popover;
  GtkWidget           *load_settings_button;

  GHashTable          *widgets;
  GHashTable          *mnemonics;
  GHashTable          *core_mnemonics;
  GtkSizeGroup        *label_group;

  GHashTable          *sensitive_data;

  gboolean             fill_started;
  gboolean             fill_ended;
} GimpProcedureDialogPrivate;

typedef struct GimpProcedureDialogSensitiveData
{
  gboolean  sensitive;

  GObject  *config;
  gchar    *config_property;
  gboolean  config_invert;
} GimpProcedureDialogSensitiveData;

typedef struct GimpProcedureDialogSensitiveData2
{
  GimpProcedureDialog *dialog;
  gchar               *widget_property;

  GimpValueArray      *values;
  gboolean             in_values;
} GimpProcedureDialogSensitiveData2;


static GObject   * gimp_procedure_dialog_constructor            (GType                  type,
                                                                 guint                  n_construct_properties,
                                                                 GObjectConstructParam *construct_properties);
static void        gimp_procedure_dialog_constructed            (GObject               *object);
static void        gimp_procedure_dialog_dispose                (GObject               *object);
static void        gimp_procedure_dialog_set_property           (GObject               *object,
                                                                 guint                  property_id,
                                                                 const GValue          *value,
                                                                 GParamSpec            *pspec);
static void        gimp_procedure_dialog_get_property           (GObject               *object,
                                                                 guint                  property_id,
                                                                 GValue                *value,
                                                                 GParamSpec            *pspec);

static void        gimp_procedure_dialog_real_fill_start        (GimpProcedureDialog *dialog,
                                                                 GimpProcedure       *procedure,
                                                                 GimpProcedureConfig *config);
static void        gimp_procedure_dialog_real_fill_end          (GimpProcedureDialog *dialog,
                                                                 GimpProcedure       *procedure,
                                                                 GimpProcedureConfig *config);
static void        gimp_procedure_dialog_real_fill_list         (GimpProcedureDialog   *dialog,
                                                                 GimpProcedure         *procedure,
                                                                 GimpProcedureConfig   *config,
                                                                 GList                 *properties);

static void        gimp_procedure_dialog_reset_initial          (GtkWidget             *button,
                                                                 GimpProcedureDialog   *dialog);
static void        gimp_procedure_dialog_reset_factory          (GtkWidget             *button,
                                                                 GimpProcedureDialog   *dialog);
static void        gimp_procedure_dialog_load_defaults          (GtkWidget             *button,
                                                                 GimpProcedureDialog   *dialog);
static void        gimp_procedure_dialog_save_defaults          (GtkWidget             *button,
                                                                 GimpProcedureDialog   *dialog);

static gboolean    gimp_procedure_dialog_check_mnemonic         (GimpProcedureDialog  *dialog,
                                                                  GtkWidget            *widget,
                                                                  const gchar          *id,
                                                                  const gchar          *core_id);
static GtkWidget * gimp_procedure_dialog_fill_container_list    (GimpProcedureDialog  *dialog,
                                                                 const gchar          *container_id,
                                                                 GtkContainer         *container,
                                                                 GList                *properties);

static void        gimp_procedure_dialog_set_sensitive_if_in_cb (GObject                           *config,
                                                                 GParamSpec                        *param_spec,
                                                                 GimpProcedureDialogSensitiveData2 *data);
static void        gimp_procedure_dialog_sensitive_data_free    (GimpProcedureDialogSensitiveData  *data);
static void        gimp_procedure_dialog_sensitive_cb_data_free (GimpProcedureDialogSensitiveData2 *data);


G_DEFINE_TYPE_WITH_PRIVATE (GimpProcedureDialog, gimp_procedure_dialog,
                            GIMP_TYPE_DIALOG)

#define parent_class gimp_procedure_dialog_parent_class

static GParamSpec *props[N_PROPS] = { NULL, };


static void
gimp_procedure_dialog_class_init (GimpProcedureDialogClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructor  = gimp_procedure_dialog_constructor;
  object_class->constructed  = gimp_procedure_dialog_constructed;
  object_class->dispose      = gimp_procedure_dialog_dispose;
  object_class->get_property = gimp_procedure_dialog_get_property;
  object_class->set_property = gimp_procedure_dialog_set_property;

  klass->fill_start          = gimp_procedure_dialog_real_fill_start;
  klass->fill_end            = gimp_procedure_dialog_real_fill_end;
  klass->fill_list           = gimp_procedure_dialog_real_fill_list;

  props[PROP_PROCEDURE] =
    g_param_spec_object ("procedure",
                         "Procedure",
                         "The GimpProcedure this dialog is used with",
                         GIMP_TYPE_PROCEDURE,
                         GIMP_PARAM_READWRITE |
                         G_PARAM_CONSTRUCT_ONLY);

  props[PROP_CONFIG] =
    g_param_spec_object ("config",
                         "Config",
                         "The GimpProcedureConfig this dialog is editing",
                         GIMP_TYPE_PROCEDURE_CONFIG,
                         GIMP_PARAM_READWRITE |
                         G_PARAM_CONSTRUCT);

  g_object_class_install_properties (object_class, N_PROPS - 1, props);

  /**
   * GimpProcedureDialog:title:
   *
   * Overrides the "title" property of #GtkWindow.
   * When %NULL, the title is taken from the #GimpProcedure menu label.
   *
   * Since: 3.0
   */
  g_object_class_override_property (object_class, PROP_TITLE, "title");
}

static void
gimp_procedure_dialog_init (GimpProcedureDialog *dialog)
{
  GimpProcedureDialogPrivate *priv;

  priv = gimp_procedure_dialog_get_instance_private (dialog);

  priv->widgets        = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_object_unref);
  priv->mnemonics      = g_hash_table_new_full (g_direct_hash, NULL, NULL, g_free);
  priv->core_mnemonics = g_hash_table_new_full (g_direct_hash, NULL, NULL, g_free);
  priv->label_group    = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  priv->sensitive_data = g_hash_table_new_full (g_str_hash, g_str_equal, g_free,
                                                (GDestroyNotify) gimp_procedure_dialog_sensitive_data_free);
  priv->fill_started   = FALSE;
  priv->fill_ended     = FALSE;
}

static GObject *
gimp_procedure_dialog_constructor (GType                  type,
                                   guint                  n_construct_properties,
                                   GObjectConstructParam *construct_properties)
{
  gboolean use_header_bar        = FALSE;
  gboolean use_header_bar_edited = FALSE;
  gboolean help_func_edited      = FALSE;

  if (gtk_settings_get_default () != NULL)
    g_object_get (gtk_settings_get_default (),
                  "gtk-dialogs-use-header", &use_header_bar,
                  NULL);
  else
    g_warning ("%s: no default screen. Did you call gimp_ui_init()?", G_STRFUNC);


  for (guint i = 0; i < n_construct_properties; i++)
    {
      /* We need to override the default values of some properties and
       * can't do it in _init() or _constructed() because it's too late
       * for G_PARAM_CONSTRUCT_ONLY properties.
       * Moreover we don't do it in _new() because we need this to work
       * also in bindings sometimes using only constructors to
       * initialize their object, hence we can't rely on our _new()
       * function (relying on _new() functions doing more than the
       * constructor is now discouraged by GTK/GLib developers).
       */
      GObjectConstructParam property;

      property = construct_properties[i];
      if (! use_header_bar_edited &&
          g_strcmp0 (g_param_spec_get_name (property.pspec),
                     "use-header-bar") == 0)
        {
          if (g_value_get_int (property.value) == -1)
            g_value_set_int (property.value, (gint) use_header_bar);

          use_header_bar_edited = TRUE;
        }
      else if (! help_func_edited &&
               g_strcmp0 (g_param_spec_get_name (property.pspec),
                          "help-func") == 0)
        {
          if (g_value_get_pointer (property.value) == NULL)
            g_value_set_pointer (property.value,
                                 gimp_standard_help_func);

          help_func_edited = TRUE;
        }

      if (use_header_bar_edited && help_func_edited)
        break;
    }

  return G_OBJECT_CLASS (parent_class)->constructor (type,
                                                     n_construct_properties,
                                                     construct_properties);
}

static void
gimp_procedure_dialog_constructed (GObject *object)
{
  GimpProcedureDialog        *dialog;
  GimpProcedureDialogPrivate *priv;
  GimpProcedure              *procedure;
  const gchar                *help_id;
  const gchar                *ok_label;
  GtkWidget                  *hbox;
  GtkWidget                  *button;
  GtkWidget                  *widget;
  GtkWidget                  *box;
  GtkWidget                  *content_area;
  gchar                      *role;

  dialog    = GIMP_PROCEDURE_DIALOG (object);
  priv      = gimp_procedure_dialog_get_instance_private (dialog);
  procedure = priv->procedure;

  role    = g_strdup_printf ("gimp-%s", gimp_procedure_get_name (procedure));
  help_id = gimp_procedure_get_help_id (procedure);
  g_object_set (object,
                "role",    role,
                "help-id", help_id,
                /* This may seem weird, but this is actually because we
                 * are overriding this property and if the title is NULL
                 * in particular, we create one out of the procedure's
                 * menu label. So we force the reset this way.
                 */
                "title",   gtk_window_get_title (GTK_WINDOW (dialog)),
                NULL);
  g_free (role);

  /* Normally, we would call the parents constructed as soon as possible.
   * However, gimp_dialog_constructed needs the help-id to already be set, or
   * else the help button in legacy plug-ins doesn't show up. Since we only
   * set a few object properties, that seems safe to do here. */
  G_OBJECT_CLASS (parent_class)->constructed (object);

  if (GIMP_IS_LOAD_PROCEDURE (procedure))
    ok_label = _("_Open");
  else if (GIMP_IS_EXPORT_PROCEDURE (procedure))
    ok_label = _("_Export");
  else
    ok_label = _("_OK");

  /* Reset button packaged with a down-arrow icon to show it pops up
   * more choices.
   */
  button = gtk_button_new ();
  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);

  widget = gtk_label_new_with_mnemonic (_("_Reset"));
  gtk_label_set_mnemonic_widget (GTK_LABEL (widget), button);
  gtk_box_pack_start (GTK_BOX (box), widget, FALSE, FALSE, 1);
  gtk_widget_show (widget);

  widget = gtk_image_new_from_icon_name (GIMP_ICON_GO_DOWN, GTK_ICON_SIZE_MENU);
  gtk_box_pack_start (GTK_BOX (box), widget, FALSE, FALSE, 1);
  gtk_widget_show (widget);

  gtk_container_add (GTK_CONTAINER (button), box);
  gtk_widget_show (box);

  gtk_dialog_add_action_widget (GTK_DIALOG (dialog), button, RESPONSE_RESET);
  gtk_widget_show (button);
  gimp_procedure_dialog_check_mnemonic (GIMP_PROCEDURE_DIALOG (dialog), button, NULL, "reset");

  /* Cancel and OK buttons. */
  button = gimp_dialog_add_button (GIMP_DIALOG (dialog),
                                   _("_Cancel"), GTK_RESPONSE_CANCEL);
  gimp_procedure_dialog_check_mnemonic (GIMP_PROCEDURE_DIALOG (dialog), button, NULL, "cancel");
  button = gimp_dialog_add_button (GIMP_DIALOG (dialog),
                                   ok_label, GTK_RESPONSE_OK);
  priv->ok_button = button;
  gimp_procedure_dialog_check_mnemonic (GIMP_PROCEDURE_DIALOG (dialog), button, NULL, "ok");
  /* OK button is the default action and has focus from start.
   * This allows to just accept quickly whatever default values.
   */
  gtk_widget_set_can_default (button, TRUE);
  gtk_widget_grab_focus (button);
  gtk_widget_grab_default (button);

  gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                            GTK_RESPONSE_OK,
                                            RESPONSE_RESET,
                                            GTK_RESPONSE_CANCEL,
                                            -1);

  gimp_window_set_transient (GTK_WINDOW (dialog));

  /* Main content area. */
  content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
  gtk_container_set_border_width (GTK_CONTAINER (content_area), 12);
  gtk_box_set_spacing (GTK_BOX (content_area), 3);

  /* Bottom box buttons with small additional padding. */
  hbox = gtk_button_box_new (GTK_ORIENTATION_HORIZONTAL);
  gtk_box_set_spacing (GTK_BOX (hbox), 6);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (hbox), GTK_BUTTONBOX_START);
  gtk_box_pack_end (GTK_BOX (content_area), hbox, FALSE, FALSE, 0);
  gtk_container_child_set (GTK_CONTAINER (content_area), hbox,
                           "padding", 3, NULL);
  gtk_widget_show (hbox);

  button = gtk_button_new_with_mnemonic (_("_Load Saved Settings"));
  gtk_widget_set_tooltip_text (button, _("Load settings saved with \"Save Settings\" button"));
  gimp_procedure_dialog_check_mnemonic (GIMP_PROCEDURE_DIALOG (dialog), button, NULL, "load-defaults");
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (gimp_procedure_dialog_load_defaults),
                    dialog);
  gtk_widget_set_sensitive (button,
                            gimp_procedure_config_has_default (priv->config));
  priv->load_settings_button = button;

  button = gtk_button_new_with_mnemonic (_("_Save Settings"));
  gtk_widget_set_tooltip_text (button, _("Store current settings for later reuse"));
  gimp_procedure_dialog_check_mnemonic (GIMP_PROCEDURE_DIALOG (dialog), button, NULL, "save-defaults");
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (gimp_procedure_dialog_save_defaults),
                    dialog);
}

static void
gimp_procedure_dialog_dispose (GObject *object)
{
  GimpProcedureDialog        *dialog = GIMP_PROCEDURE_DIALOG (object);
  GimpProcedureDialogPrivate *priv;

  priv = gimp_procedure_dialog_get_instance_private (dialog);

  g_clear_object (&priv->procedure);
  g_clear_object (&priv->config);
  g_clear_object (&priv->initial_config);

  g_clear_pointer (&priv->reset_popover, gtk_widget_destroy);

  g_clear_pointer (&priv->widgets, g_hash_table_destroy);
  g_clear_pointer (&priv->mnemonics, g_hash_table_destroy);
  g_clear_pointer (&priv->core_mnemonics, g_hash_table_destroy);

  g_clear_pointer (&priv->sensitive_data, g_hash_table_destroy);

  g_clear_object (&priv->label_group);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_procedure_dialog_set_property (GObject      *object,
                                    guint         property_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  GimpProcedureDialog        *dialog = GIMP_PROCEDURE_DIALOG (object);
  GimpProcedureDialogPrivate *priv;

  priv = gimp_procedure_dialog_get_instance_private (dialog);

  switch (property_id)
    {
    case PROP_PROCEDURE:
      priv->procedure = g_value_dup_object (value);
      break;

    case PROP_CONFIG:
      priv->config = g_value_dup_object (value);

      if (priv->config)
        priv->initial_config =
          gimp_config_duplicate (GIMP_CONFIG (priv->config));
      break;

    case PROP_TITLE:
        {
          GtkWidget   *bogus = NULL;
          const gchar *title;

          title = g_value_get_string (value);

          if (title == NULL)
            {
              /* Use the procedure menu label, but remove mnemonic
               * underscore. Ugly yet must reliable way as GTK does not
               * expose a function to do this from a string (and better
               * not to copy-paste the internal function from GTK code).
               */
              bogus = gtk_label_new (NULL);
              gtk_label_set_markup_with_mnemonic (GTK_LABEL (g_object_ref_sink (bogus)),
                                                  gimp_procedure_get_menu_label (priv->procedure));
              title = gtk_label_get_text (GTK_LABEL (bogus));
            }
          if (title != NULL)
            gtk_window_set_title (GTK_WINDOW (dialog), title);

          g_clear_object (&bogus);
        }
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_procedure_dialog_get_property (GObject    *object,
                                    guint       property_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  GimpProcedureDialog        *dialog = GIMP_PROCEDURE_DIALOG (object);
  GimpProcedureDialogPrivate *priv;

  priv = gimp_procedure_dialog_get_instance_private (dialog);
  switch (property_id)
    {
    case PROP_PROCEDURE:
      g_value_set_object (value, priv->procedure);
      break;

    case PROP_CONFIG:
      g_value_set_object (value, priv->config);
      break;

    case PROP_TITLE:
      g_value_set_string (value,
                          gtk_window_get_title (GTK_WINDOW (dialog)));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_procedure_dialog_real_fill_start (GimpProcedureDialog *dialog,
                                       GimpProcedure       *procedure,
                                       GimpProcedureConfig *config)
{
}

static void
gimp_procedure_dialog_real_fill_end (GimpProcedureDialog *dialog,
                                     GimpProcedure       *procedure,
                                     GimpProcedureConfig *config)
{
}

static void
gimp_procedure_dialog_real_fill_list (GimpProcedureDialog *dialog,
                                      GimpProcedure       *procedure,
                                      GimpProcedureConfig *config,
                                      GList               *properties)
{
  GtkWidget *content_area;
  GList     *iter;

  content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

  for (iter = properties; iter; iter = iter->next)
    {
      GtkWidget *widget;

      widget = gimp_procedure_dialog_get_widget (dialog, iter->data, G_TYPE_NONE);
      if (widget)
        {
          gtk_box_pack_start (GTK_BOX (content_area), widget, TRUE, TRUE, 0);
          gtk_widget_show (widget);
        }
    }
}

/**
 * gimp_procedure_dialog_new:
 * @procedure: the associated #GimpProcedure.
 * @config:    a #GimpProcedureConfig from which properties will be
 *             turned into widgets.
 * @title: (nullable): a dialog title.
 *
 * Creates a new dialog for @procedure using widgets generated from
 * properties of @config.
 * A %NULL title will only be accepted if a menu label was set with
 * gimp_procedure_set_menu_label() (this menu label will then be used as
 * dialog title instead). If neither an explicit label nor a @procedure
 * menu label was set, the call will fail.
 *
 * As for all #GtkWindow, the returned #GimpProcedureDialog object is
 * owned by GTK and its initial reference is stored in an internal list
 * of top-level windows. To delete the dialog, call
 * gtk_widget_destroy().
 *
 * Returns: (transfer none): the newly created #GimpProcedureDialog.
 */
GtkWidget *
gimp_procedure_dialog_new (GimpProcedure       *procedure,
                           GimpProcedureConfig *config,
                           const gchar         *title)
{
  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);
  g_return_val_if_fail (GIMP_IS_PROCEDURE_CONFIG (config), NULL);
  g_return_val_if_fail (gimp_procedure_config_get_procedure (config) ==
                        procedure, NULL);
  g_return_val_if_fail (title != NULL || gimp_procedure_get_menu_label (procedure), NULL);

  return g_object_new (GIMP_TYPE_PROCEDURE_DIALOG,
                       "procedure", procedure,
                       "config",    config,
                       "title",     title,
                       NULL);
}

/**
 * gimp_procedure_dialog_set_ok_label:
 * @dialog:   the associated #GimpProcedureDialog.
 * @ok_label: a label to replace the OK button's text.
 *
 * Changes the "OK" button's label of @dialog to @ok_label.
 */
void
gimp_procedure_dialog_set_ok_label (GimpProcedureDialog *dialog,
                                    const gchar         *ok_label)
{
  GimpProcedureDialogPrivate *priv;

  priv = gimp_procedure_dialog_get_instance_private (dialog);

  if (ok_label == NULL)
    {
      GimpProcedure *procedure = priv->procedure;

      if (GIMP_IS_LOAD_PROCEDURE (procedure))
        ok_label = _("_Open");
      else if (GIMP_IS_EXPORT_PROCEDURE (procedure))
        ok_label = _("_Export");
      else
        ok_label = _("_OK");
    }

  gtk_button_set_label (GTK_BUTTON (priv->ok_button), ok_label);
  gimp_procedure_dialog_check_mnemonic (GIMP_PROCEDURE_DIALOG (dialog),
                                        priv->ok_button, NULL, "ok");
}

/**
 * gimp_procedure_dialog_get_widget:
 * @dialog:      the associated #GimpProcedureDialog.
 * @property:    name of the property to build a widget for. It must be
 *               a property of the #GimpProcedure @dialog has been
 *               created for.
 * @widget_type: alternative widget type. %G_TYPE_NONE will create the
 *               default type of widget for the associated property
 *               type.
 *
 * Creates a new #GtkWidget for @property according to the property
 * type. The following types are possible:
 *
 * - %G_TYPE_PARAM_BOOLEAN:
 *     * %GTK_TYPE_CHECK_BUTTON (default)
 *     * %GTK_TYPE_SWITCH
 * - %G_TYPE_PARAM_INT, %G_TYPE_PARAM_UINT, or %G_TYPE_PARAM_DOUBLE:
 *     * %GIMP_TYPE_LABEL_SPIN (default): a spin button with a label.
 *     * %GIMP_TYPE_SCALE_ENTRY: a scale entry with label.
 *     * %GIMP_TYPE_SPIN_SCALE: a spin scale with label embedded.
 *     * %GIMP_TYPE_SPIN_BUTTON: a spin button with no label.
 * - %G_TYPE_PARAM_STRING:
 *     * %GIMP_TYPE_LABEL_ENTRY (default): an entry with a label.
 *     * %GTK_TYPE_ENTRY: an entry with no label.
 *     * %GTK_TYPE_TEXT_VIEW: a text view with no label.
 * - %GIMP_TYPE_CHOICE (default will depend on the number of choices):
 *     * %GTK_TYPE_COMBO_BOX: a combo box displaying every
 *       choice.
 *     * %GIMP_TYPE_INT_RADIO_FRAME: a frame with radio buttons.
 * - %GEGL_TYPE_COLOR:
 *     * %GIMP_TYPE_LABEL_COLOR (default): a color button with a label.
 *         Please use gimp_procedure_dialog_get_color_widget() for a
 *         non-editable color area with a label.
 *     * %GIMP_TYPE_COLOR_BUTTON: a color button with no label.
 *     * %GIMP_TYPE_COLOR_AREA: a color area with no label.
 * - %GIMP_TYPE_PARAM_FILE:
 *     * %GTK_FILE_CHOOSER_BUTTON (default): generic file chooser widget
 *       using the action mode of the param spec.
 *       Note that it won't work with a [enum@Gimp.FileChooserAction.ANY]
 *       action. If you intend to display a widget for a file param
 *       spec, you should always set it to a more specific action.
 *       See [method@Gimp.Procedure.add_file_argument].
 * - %G_TYPE_PARAM_UNIT:
 *     * %GIMP_TYPE_UNIT_COMBO_BOX
 *
 * If the @widget_type is not supported for the actual type of
 * @property, the function will fail. To keep the default, set to
 * %G_TYPE_NONE.
 *
 * If a widget has already been created for this procedure, it will be
 * returned instead (even if with a different @widget_type).
 *
 * Returns: (transfer none): the #GtkWidget representing @property. The
 *                           object belongs to @dialog and must not be
 *                           freed.
 */
GtkWidget *
gimp_procedure_dialog_get_widget (GimpProcedureDialog *dialog,
                                  const gchar         *property,
                                  GType                widget_type)
{
  GimpProcedureDialogPrivate       *priv;
  GtkWidget                        *widget = NULL;
  GtkWidget                        *label  = NULL;
  GimpProcedureDialogSensitiveData *binding;
  GParamSpec                       *pspec;

  g_return_val_if_fail (property != NULL, NULL);

  priv = gimp_procedure_dialog_get_instance_private (dialog);

  /* First check if it already exists. */
  widget = g_hash_table_lookup (priv->widgets, property);

  if (widget)
    return widget;

  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (priv->config),
                                        property);
  if (! pspec)
    {
      g_warning ("%s: parameter %s does not exist.",
                 G_STRFUNC, property);
      return NULL;
    }

  if (G_PARAM_SPEC_TYPE (pspec) == G_TYPE_PARAM_BOOLEAN)
    {
      if (widget_type == G_TYPE_NONE || widget_type == GTK_TYPE_CHECK_BUTTON)
        widget = gimp_prop_check_button_new (G_OBJECT (priv->config),
                                             property,
                                             g_param_spec_get_nick (pspec));
      else if (widget_type == GTK_TYPE_SWITCH)
        widget = gimp_prop_switch_new (G_OBJECT (priv->config),
                                       property,
                                       g_param_spec_get_nick (pspec),
                                       &label, NULL);
    }
  else if (G_PARAM_SPEC_TYPE (pspec) == G_TYPE_PARAM_INT  ||
           G_PARAM_SPEC_TYPE (pspec) == G_TYPE_PARAM_UINT ||
           G_PARAM_SPEC_TYPE (pspec) == G_TYPE_PARAM_DOUBLE)
    {
      gdouble minimum;
      gdouble maximum;
      gdouble step   = 0.0;
      gdouble page   = 0.0;
      gint    digits = 0;

      if (G_PARAM_SPEC_TYPE (pspec) == G_TYPE_PARAM_INT)
        {
          GParamSpecInt *pspecint = (GParamSpecInt *) pspec;

          minimum = (gdouble) pspecint->minimum;
          maximum = (gdouble) pspecint->maximum;
        }
      else if (G_PARAM_SPEC_TYPE (pspec) == G_TYPE_PARAM_UINT)
        {
          GParamSpecUInt *pspecuint = (GParamSpecUInt *) pspec;

          minimum = (gdouble) pspecuint->minimum;
          maximum = (gdouble) pspecuint->maximum;
        }
      else /* G_TYPE_PARAM_DOUBLE */
        {
          GParamSpecDouble *pspecdouble = (GParamSpecDouble *) pspec;

          minimum = pspecdouble->minimum;
          maximum = pspecdouble->maximum;
        }
      gimp_range_estimate_settings (minimum, maximum, &step, &page, &digits);

      if (widget_type == G_TYPE_NONE || widget_type == GIMP_TYPE_LABEL_SPIN)
        {
          widget = gimp_prop_label_spin_new (G_OBJECT (priv->config),
                                             property, digits);
        }
      else if (widget_type == GIMP_TYPE_SCALE_ENTRY)
        {
          widget = gimp_prop_scale_entry_new (G_OBJECT (priv->config),
                                              property,
                                              g_param_spec_get_nick (pspec),
                                              1.0, FALSE, 0.0, 0.0);
        }
      else if (widget_type == GIMP_TYPE_SPIN_SCALE)
        {
          widget = gimp_prop_spin_scale_new (G_OBJECT (priv->config),
                                             property, step, page, digits);
        }
      else if (widget_type == GIMP_TYPE_SPIN_BUTTON)
        {
          /* Just some spin button without label. */
          widget = gimp_prop_spin_button_new (G_OBJECT (priv->config),
                                              property, step, page, digits);
        }
    }
  else if (G_PARAM_SPEC_TYPE (pspec) == G_TYPE_PARAM_STRING)
    {
      if (widget_type == G_TYPE_NONE || widget_type == GIMP_TYPE_LABEL_ENTRY)
        {
          widget = gimp_prop_label_entry_new (G_OBJECT (priv->config),
                                              property, -1);
        }
      else if (widget_type == GTK_TYPE_TEXT_VIEW)
        {
          GtkTextBuffer *buffer;

          /* Text view with no label. */
          buffer = gimp_prop_text_buffer_new (G_OBJECT (priv->config),
                                              property, -1);
          widget = gtk_text_view_new_with_buffer (buffer);
          gtk_text_view_set_top_margin (GTK_TEXT_VIEW (widget), 3);
          gtk_text_view_set_bottom_margin (GTK_TEXT_VIEW (widget), 3);
          gtk_text_view_set_left_margin (GTK_TEXT_VIEW (widget), 3);
          gtk_text_view_set_right_margin (GTK_TEXT_VIEW (widget), 3);
          gtk_widget_set_hexpand (widget, TRUE);
          g_object_unref (buffer);
        }
      else if (widget_type == GTK_TYPE_ENTRY)
        {
          /* Entry with no label. */
          widget = gimp_prop_entry_new (G_OBJECT (priv->config),
                                        property, -1);
        }
    }
  else if (G_PARAM_SPEC_TYPE (pspec) == GIMP_TYPE_PARAM_COLOR ||
           G_PARAM_SPEC_TYPE (pspec) == GEGL_TYPE_PARAM_COLOR)
    {
      gboolean has_alpha = TRUE;

      if (G_PARAM_SPEC_TYPE (pspec) == GIMP_TYPE_PARAM_COLOR)
        has_alpha = gimp_param_spec_color_has_alpha (pspec);

      if (widget_type == G_TYPE_NONE || widget_type == GIMP_TYPE_LABEL_COLOR)
        {
          widget = gimp_prop_label_color_new (G_OBJECT (priv->config),
                                              property, TRUE);

          if (! has_alpha)
            {
              GtkWidget *color_button;

              color_button =
                gimp_label_color_get_color_widget (GIMP_LABEL_COLOR (widget));
              gimp_color_button_set_type (GIMP_COLOR_BUTTON (color_button),
                                          GIMP_COLOR_AREA_FLAT);
            }
        }
      else if (widget_type == GIMP_TYPE_COLOR_AREA)
        {
          widget = gimp_prop_color_area_new (G_OBJECT (priv->config),
                                             property, 20, 20,
                                             has_alpha ?
                                             GIMP_COLOR_AREA_SMALL_CHECKS :
                                             GIMP_COLOR_AREA_FLAT);
          gtk_widget_set_vexpand (widget, FALSE);
          gtk_widget_set_hexpand (widget, FALSE);
        }
      else if (widget_type == GIMP_TYPE_COLOR_BUTTON)
        {
          widget = gimp_prop_color_select_new (G_OBJECT (priv->config),
                                               property, 20, 20,
                                               has_alpha ?
                                               GIMP_COLOR_AREA_SMALL_CHECKS :
                                               GIMP_COLOR_AREA_FLAT);
          gtk_widget_set_vexpand (widget, FALSE);
          gtk_widget_set_hexpand (widget, FALSE);
        }
    }
  else if (GIMP_IS_PARAM_SPEC_FILE (pspec))
    {
      widget = gimp_prop_file_chooser_new (G_OBJECT (priv->config), property, NULL, NULL);
      label  = gimp_file_chooser_get_label_widget (GIMP_FILE_CHOOSER (widget));
    }
  else if (G_PARAM_SPEC_TYPE (pspec) == GIMP_TYPE_PARAM_CHOICE)
    {
      GType real_widget_type = widget_type;

      if (real_widget_type == G_TYPE_NONE)
        {
          GimpChoice *choice = gimp_param_spec_choice_get_choice (pspec);
          gint        n_choices;

          n_choices = g_list_length (gimp_choice_list_nicks (choice));
          if (n_choices > 3)
            real_widget_type = GTK_TYPE_COMBO_BOX;
          else
            real_widget_type = GIMP_TYPE_INT_RADIO_FRAME;
        }

      if (real_widget_type == GTK_TYPE_COMBO_BOX)
        {
          widget = gimp_prop_choice_combo_box_new (G_OBJECT (priv->config), property);
          gtk_widget_set_vexpand (widget, FALSE);
          gtk_widget_set_hexpand (widget, TRUE);
          widget = gimp_label_string_widget_new (g_param_spec_get_nick (pspec), widget);
        }
      else if (real_widget_type == GIMP_TYPE_INT_RADIO_FRAME)
        {
          widget = gimp_prop_choice_radio_frame_new (G_OBJECT (priv->config), property);
          gtk_widget_set_vexpand (widget, FALSE);
          gtk_widget_set_hexpand (widget, TRUE);
        }
    }
  /* GimpResource subclasses */
  /* FUTURE: title the chooser more specifically, with a prefix that is the nick of the property. */
  else if (G_IS_PARAM_SPEC_OBJECT (pspec) && pspec->value_type == GIMP_TYPE_BRUSH)
    {
      widget = gimp_prop_brush_chooser_new (G_OBJECT (priv->config), property, _("Brush Chooser"));
    }
  else if (G_IS_PARAM_SPEC_OBJECT (pspec) && pspec->value_type == GIMP_TYPE_FONT)
    {
      widget = gimp_prop_font_chooser_new (G_OBJECT (priv->config), property, _("Font Chooser"));
    }
  else if (G_IS_PARAM_SPEC_OBJECT (pspec) && pspec->value_type == GIMP_TYPE_GRADIENT)
    {
      widget = gimp_prop_gradient_chooser_new (G_OBJECT (priv->config), property, _("Gradient Chooser"));
    }
  else if (G_IS_PARAM_SPEC_OBJECT (pspec) && pspec->value_type == GIMP_TYPE_PALETTE)
    {
      widget = gimp_prop_palette_chooser_new (G_OBJECT (priv->config), property, _("Palette Chooser"));
    }
  else if (G_IS_PARAM_SPEC_OBJECT (pspec) && pspec->value_type == GIMP_TYPE_PATTERN)
    {
      widget = gimp_prop_pattern_chooser_new (G_OBJECT (priv->config), property, _("Pattern Chooser"));
    }
  else if (G_IS_PARAM_SPEC_OBJECT (pspec) && (pspec->value_type == GIMP_TYPE_DRAWABLE ||
                                              pspec->value_type == GIMP_TYPE_LAYER    ||
                                              pspec->value_type == GIMP_TYPE_CHANNEL))
    {
      widget = gimp_prop_drawable_chooser_new (G_OBJECT (priv->config), property, NULL);
    }
  else if (G_PARAM_SPEC_TYPE (pspec) == G_TYPE_PARAM_ENUM)
    {
      GimpIntStore *store;

      store = (GimpIntStore *) gimp_enum_store_new (G_PARAM_SPEC_VALUE_TYPE (pspec));

      widget = gimp_prop_int_combo_box_new (G_OBJECT (priv->config),
                                            property,
                                            store);
      gtk_widget_set_vexpand (widget, FALSE);
      gtk_widget_set_hexpand (widget, TRUE);
      widget = gimp_label_int_widget_new (g_param_spec_get_nick (pspec),
                                          widget);
    }
  else if (G_PARAM_SPEC_TYPE (pspec) == GIMP_TYPE_PARAM_UNIT)
    {
      widget = gimp_prop_unit_combo_box_new (G_OBJECT (priv->config), property);
    }
  else
    {
      g_warning ("%s: parameter %s has non supported type %s for value type %s",
                 G_STRFUNC, property,
                 G_PARAM_SPEC_TYPE_NAME (pspec),
                 g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec)));
      return NULL;
    }

  if (! widget)
    {
      g_warning ("%s: widget type %s not supported for parameter '%s' of type %s",
                 G_STRFUNC, g_type_name (widget_type),
                 property, G_PARAM_SPEC_TYPE_NAME (pspec));
      return NULL;
    }
  else
    {
      const gchar *tooltip = g_param_spec_get_blurb (pspec);

      if (tooltip)
        gimp_help_set_help_data (widget, tooltip, NULL);

      if (label == NULL)
        {
          if (GIMP_IS_LABELED (widget))
            label = gimp_labeled_get_label (GIMP_LABELED (widget));
          else if (GIMP_IS_RESOURCE_CHOOSER (widget))
            label = gimp_resource_chooser_get_label (GIMP_RESOURCE_CHOOSER (widget));
          else if (GIMP_IS_DRAWABLE_CHOOSER (widget))
            label = gimp_drawable_chooser_get_label (GIMP_DRAWABLE_CHOOSER (widget));
        }

      if (label != NULL)
        {
          gtk_size_group_add_widget (priv->label_group, label);

          /* Make sure all labels have consistent alignment and margin. */
          gtk_label_set_xalign (GTK_LABEL (label), 0.0);
          gtk_widget_set_margin_end (GTK_WIDGET (label), 4);
          gtk_widget_set_margin_start (GTK_WIDGET (label), 0);
          gtk_widget_set_margin_top (GTK_WIDGET (label), 0);
          gtk_widget_set_margin_bottom (GTK_WIDGET (label), 0);
        }
    }

  if ((binding = g_hash_table_lookup (priv->sensitive_data, property)))
    {
      if (binding->config)
        {
          g_object_bind_property (binding->config, binding->config_property,
                                  widget, "sensitive",
                                  G_BINDING_SYNC_CREATE |
                                  (binding->config_invert ? G_BINDING_INVERT_BOOLEAN : 0));
        }
      else
        {
          gtk_widget_set_sensitive (widget, binding->sensitive);
        }

      g_hash_table_remove (priv->sensitive_data, property);
    }

  g_return_val_if_fail (g_hash_table_lookup_extended (priv->widgets, property, NULL, NULL) == FALSE, NULL);

  gimp_procedure_dialog_check_mnemonic (dialog, widget, property, NULL);
  g_hash_table_insert (priv->widgets, g_strdup (property), widget);
  if (g_object_is_floating (widget))
    g_object_ref_sink (widget);

  return widget;
}

/**
 * gimp_procedure_dialog_get_color_widget:
 * @dialog:   the associated #GimpProcedureDialog.
 * @property: name of the #GeglColor property to build a widget for. It
 *            must be a property of the #GimpProcedure @dialog has been
 *            created for.
 * @editable: whether the color can be edited or is only for display.
 * @type:     the #GimpColorAreaType.
 *
 * Creates a new widget for @property which must necessarily be a
 * #GeglColor property.
 * This must be used instead of gimp_procedure_dialog_get_widget() when
 * you want a #GimpLabelColor which is not customizable for a color
 * property, or when to set a specific @type.
 *
 * If a widget has already been created for this procedure, it will be
 * returned instead (whatever its actual widget type).
 *
 * Returns: (transfer none): a #GimpLabelColor representing @property.
 *                           The object belongs to @dialog and must not
 *                           be freed.
 */
GtkWidget *
gimp_procedure_dialog_get_color_widget (GimpProcedureDialog *dialog,
                                        const gchar         *property,
                                        gboolean             editable,
                                        GimpColorAreaType    type)
{
  GimpProcedureDialogPrivate *priv;
  GtkWidget                  *widget    = NULL;
  GParamSpec                 *pspec;
  gboolean                    has_alpha = TRUE;

  g_return_val_if_fail (property != NULL, NULL);

  priv = gimp_procedure_dialog_get_instance_private (dialog);

  /* First check if it already exists. */
  widget = g_hash_table_lookup (priv->widgets, property);

  if (widget)
    return widget;

  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (priv->config),
                                        property);
  if (! pspec)
    {
      g_warning ("%s: parameter %s does not exist.",
                 G_STRFUNC, property);
      return NULL;
    }

  if (G_PARAM_SPEC_TYPE (pspec) == GIMP_TYPE_PARAM_COLOR)
    has_alpha = gimp_param_spec_color_has_alpha (pspec);

  if (G_PARAM_SPEC_TYPE (pspec) == GIMP_TYPE_PARAM_COLOR ||
      G_PARAM_SPEC_TYPE (pspec) == GEGL_TYPE_PARAM_COLOR)
    {
      widget = gimp_prop_label_color_new (G_OBJECT (priv->config),
                                          property, editable);

      gtk_widget_set_vexpand (widget, FALSE);
      gtk_widget_set_hexpand (widget, FALSE);
    }

  if (! widget)
    {
      g_warning ("%s: parameter '%s' of type %s not suitable as color widget",
                 G_STRFUNC, property, G_PARAM_SPEC_TYPE_NAME (pspec));
      return NULL;
    }
  else if (GIMP_IS_LABELED (widget))
    {
      GtkWidget   *label   = gimp_labeled_get_label (GIMP_LABELED (widget));
      const gchar *tooltip = g_param_spec_get_blurb (pspec);

      gtk_size_group_add_widget (priv->label_group, label);
      if (tooltip)
        gimp_help_set_help_data (label, tooltip, NULL);
    }

  if (! has_alpha)
    {
      GtkWidget *color_button;

      color_button =
        gimp_label_color_get_color_widget (GIMP_LABEL_COLOR (widget));
      gimp_color_button_set_type (GIMP_COLOR_BUTTON (color_button),
                                  GIMP_COLOR_AREA_FLAT);
    }

  gimp_procedure_dialog_check_mnemonic (dialog, widget, property, NULL);
  g_hash_table_insert (priv->widgets, g_strdup (property), widget);
  if (g_object_is_floating (widget))
    g_object_ref_sink (widget);

  return widget;
}

/**
 * gimp_procedure_dialog_get_coordinates:
 * @dialog:            the associated #GimpProcedureDialog.
 * @coordinates_id:    Identifier for #GimpCoordinates widget.
 * @x_property:        Name of int or double property for X coordinate.
 * @y_property:        Name of int or double property for Y coordinate.
 * @unit_property:     Name of unit property.
 * @unit_format:       A printf-like unit-format string as is used with
 *                     gimp_unit_menu_new().
 * @update_policy:     How the automatic pixel <-> real-world-unit
 *                     calculations should be done.
 * @x_resolution:      The resolution (in dpi) for the X coordinate.
 * @y_resolution:      The resolution (in dpi) for the Y coordinate.
 *
 * Creates a new #GimpCoordinates for @x_property and @y_property which
 * must necessarily be an integer or double property.
 * The associated @unit_property must be a GimpUnit property.
 *
 * If a widget has already been created for this procedure, it will be
 * returned instead (whatever its actual widget type).
 *
 * Returns: (transfer none): the #GtkWidget representing @coordinates_id.
 *                           The object belongs to @dialog and must not be
 *                           freed.
 */
GtkWidget *
gimp_procedure_dialog_get_coordinates (GimpProcedureDialog       *dialog,
                                       const gchar               *coordinates_id,
                                       const gchar               *x_property,
                                       const gchar               *y_property,
                                       const gchar               *unit_property,
                                       const gchar               *unit_format,
                                       GimpSizeEntryUpdatePolicy  update_policy,
                                       gdouble                    x_resolution,
                                       gdouble                    y_resolution)
{
  GimpProcedureDialogPrivate *priv;
  GtkWidget                  *widget = NULL;
  GtkWidget                  *label  = NULL;
  GParamSpec                 *pspec_x;
  GParamSpec                 *pspec_y;
  GParamSpec                 *pspec_unit;

  g_return_val_if_fail (GIMP_IS_PROCEDURE_DIALOG (dialog), NULL);
  g_return_val_if_fail (coordinates_id != NULL, NULL);
  g_return_val_if_fail (x_property != NULL, NULL);
  g_return_val_if_fail (y_property != NULL, NULL);
  g_return_val_if_fail (unit_property != NULL, NULL);

  priv       = gimp_procedure_dialog_get_instance_private (dialog);
  pspec_x    = g_object_class_find_property (G_OBJECT_GET_CLASS (priv->config),
                                             x_property);
  pspec_y    = g_object_class_find_property (G_OBJECT_GET_CLASS (priv->config),
                                             y_property);
  pspec_unit = g_object_class_find_property (G_OBJECT_GET_CLASS (priv->config),
                                             unit_property);

  if (! pspec_x)
    {
      g_warning ("%s: parameter %s does not exist.",
                 G_STRFUNC, x_property);
      return NULL;
    }
  if (! pspec_y)
    {
      g_warning ("%s: parameter %s does not exist.",
                 G_STRFUNC, y_property);
      return NULL;
    }
  if (! pspec_unit)
    {
      g_warning ("%s: unit parameter %s does not exist.",
                 G_STRFUNC, unit_property);
      return NULL;
    }

  g_return_val_if_fail (G_PARAM_SPEC_TYPE (pspec_x) == G_TYPE_PARAM_INT  ||
                        G_PARAM_SPEC_TYPE (pspec_x) == G_TYPE_PARAM_UINT ||
                        G_PARAM_SPEC_TYPE (pspec_x) == G_TYPE_PARAM_DOUBLE, NULL);
  g_return_val_if_fail (G_PARAM_SPEC_TYPE (pspec_y) == G_TYPE_PARAM_INT  ||
                        G_PARAM_SPEC_TYPE (pspec_y) == G_TYPE_PARAM_UINT ||
                        G_PARAM_SPEC_TYPE (pspec_y) == G_TYPE_PARAM_DOUBLE, NULL);
  g_return_val_if_fail (G_PARAM_SPEC_TYPE (pspec_unit) == GIMP_TYPE_PARAM_UNIT, NULL);

  /* First check if it already exists. */
  widget = g_hash_table_lookup (priv->widgets, coordinates_id);

  if (widget)
    return widget;

  widget = gimp_prop_coordinates_new (G_OBJECT (priv->config), x_property,
                                      y_property, unit_property, unit_format,
                                      update_policy, x_resolution,
                                      y_resolution, TRUE);
  /* Add labels */
  label = gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (widget),
                                        g_param_spec_get_nick (pspec_x), 0, 1, 0.0);
  gtk_widget_set_margin_end (label, 6);
  label = gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (widget),
                                        g_param_spec_get_nick (pspec_y), 0, 2, 0.0);
  gtk_widget_set_margin_end (label, 6);

  g_hash_table_insert (priv->widgets, g_strdup (coordinates_id), widget);
  if (g_object_is_floating (widget))
    g_object_ref_sink (widget);

  return widget;
}

/**
 * gimp_procedure_dialog_get_int_combo:
 * @dialog:   the associated #GimpProcedureDialog.
 * @property: name of the int property to build a combo for. It must be
 *            a property of the #GimpProcedure @dialog has been created
 *            for.
 * @store: (transfer full): the #GimpIntStore which will be used.
 *
 * Creates a new #GimpLabelIntWidget for @property which must
 * necessarily be an integer or boolean property.
 * This must be used instead of gimp_procedure_dialog_get_widget() when
 * you want to create a combo box from an integer property.
 *
 * If a widget has already been created for this procedure, it will be
 * returned instead (whatever its actual widget type).
 *
 * Returns: (transfer none): the #GtkWidget representing @property. The
 *                           object belongs to @dialog and must not be
 *                           freed.
 */
GtkWidget *
gimp_procedure_dialog_get_int_combo (GimpProcedureDialog *dialog,
                                     const gchar         *property,
                                     GimpIntStore        *store)
{
  GimpProcedureDialogPrivate *priv;
  GtkWidget                  *widget = NULL;
  GParamSpec                 *pspec;

  g_return_val_if_fail (GIMP_IS_PROCEDURE_DIALOG (dialog), NULL);
  g_return_val_if_fail (property != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_INT_STORE (store), NULL);

  priv = gimp_procedure_dialog_get_instance_private (dialog);

  /* First check if it already exists. */
  widget = g_hash_table_lookup (priv->widgets, property);

  if (widget)
    {
      g_object_unref (store);
      return widget;
    }

  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (priv->config),
                                        property);
  if (! pspec)
    {
      g_warning ("%s: parameter %s does not exist.", G_STRFUNC, property);
      g_object_unref (store);
      return NULL;
    }

  if (G_PARAM_SPEC_TYPE (pspec) == G_TYPE_PARAM_BOOLEAN ||
      G_PARAM_SPEC_TYPE (pspec) == G_TYPE_PARAM_INT)
    {
      widget = gimp_prop_int_combo_box_new (G_OBJECT (priv->config),
                                            property, store);
      gtk_widget_set_vexpand (widget, FALSE);
      gtk_widget_set_hexpand (widget, TRUE);
      widget = gimp_label_int_widget_new (g_param_spec_get_nick (pspec),
                                          widget);
    }
  g_object_unref (store);

  if (! widget)
    {
      g_warning ("%s: parameter '%s' of type %s not suitable as GimpIntComboBox",
                 G_STRFUNC, property, G_PARAM_SPEC_TYPE_NAME (pspec));
      return NULL;
    }
  else
    {
      const gchar *tooltip = g_param_spec_get_blurb (pspec);
      if (tooltip)
        gimp_help_set_help_data (widget, tooltip, NULL);
      if (GIMP_IS_LABELED (widget))
        {
          GtkWidget *label = gimp_labeled_get_label (GIMP_LABELED (widget));

          gtk_size_group_add_widget (priv->label_group, label);
        }
    }

  gimp_procedure_dialog_check_mnemonic (dialog, widget, property, NULL);
  g_hash_table_insert (priv->widgets, g_strdup (property), widget);
  if (g_object_is_floating (widget))
    g_object_ref_sink (widget);

  return widget;
}

/**
 * gimp_procedure_dialog_get_int_radio:
 * @dialog:   the associated #GimpProcedureDialog.
 * @property: name of the int property to build radio buttons for. It
 *            must be a property of the #GimpProcedure @dialog has been
 *            created for.
 * @store: (transfer full): the #GimpIntStore which will be used.
 *
 * Creates a new #GimpLabelIntRadioFrame for @property which must
 * necessarily be an integer, enum or boolean property.
 * This must be used instead of gimp_procedure_dialog_get_widget() when
 * you want to create a group of %GtkRadioButton-s from an integer
 * property.
 *
 * If a widget has already been created for this procedure, it will be
 * returned instead (whatever its actual widget type).
 *
 * Returns: (transfer none): the #GtkWidget representing @property. The
 *                           object belongs to @dialog and must not be
 *                           freed.
 */
GtkWidget *
gimp_procedure_dialog_get_int_radio (GimpProcedureDialog *dialog,
                                     const gchar         *property,
                                     GimpIntStore        *store)
{
  GimpProcedureDialogPrivate *priv;
  GtkWidget                  *widget = NULL;
  GParamSpec                 *pspec;

  g_return_val_if_fail (GIMP_IS_PROCEDURE_DIALOG (dialog), NULL);
  g_return_val_if_fail (property != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_INT_STORE (store), NULL);

  priv = gimp_procedure_dialog_get_instance_private (dialog);

  /* First check if it already exists. */
  widget = g_hash_table_lookup (priv->widgets, property);

  if (widget)
    {
      g_object_unref (store);
      return widget;
    }

  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (priv->config),
                                        property);
  if (! pspec)
    {
      g_warning ("%s: parameter %s does not exist.", G_STRFUNC, property);
      g_object_unref (store);
      return NULL;
    }

  if (G_PARAM_SPEC_TYPE (pspec) == G_TYPE_PARAM_BOOLEAN ||
      G_PARAM_SPEC_TYPE (pspec) == G_TYPE_PARAM_INT)
    {
      widget = gimp_prop_int_radio_frame_new (G_OBJECT (priv->config),
                                              property, NULL, store);
      gtk_widget_set_vexpand (widget, FALSE);
      gtk_widget_set_hexpand (widget, TRUE);
    }
  g_object_unref (store);

  if (! widget)
    {
      g_warning ("%s: parameter '%s' of type %s not suitable as GimpIntRadioFrame",
                 G_STRFUNC, property, G_PARAM_SPEC_TYPE_NAME (pspec));
      return NULL;
    }

  gimp_procedure_dialog_check_mnemonic (dialog, widget, property, NULL);
  g_hash_table_insert (priv->widgets, g_strdup (property), widget);
  if (g_object_is_floating (widget))
    g_object_ref_sink (widget);

  return widget;
}

/**
 * gimp_procedure_dialog_get_spin_scale:
 * @dialog:   the associated #GimpProcedureDialog.
 * @property: name of the int or double property to build a
 *            #GimpSpinScale for. It must be a property of the
 *            #GimpProcedure @dialog has been created for.
 * @factor:   a display factor for the range shown by the widget.
 *            It must be set to 1.0 for integer properties.
 *
 * Creates a new #GimpSpinScale for @property which must necessarily be
 * an integer or double property.
 * This can be used instead of gimp_procedure_dialog_get_widget() in
 * particular if you want to tweak the display factor. A typical example
 * is showing a [0.0, 1.0] range as [0.0, 100.0] instead (@factor = 100.0).
 *
 * If a widget has already been created for this procedure, it will be
 * returned instead (whatever its actual widget type).
 *
 * Returns: (transfer none): the #GtkWidget representing @property. The
 *                           object belongs to @dialog and must not be
 *                           freed.
 */
GtkWidget *
gimp_procedure_dialog_get_spin_scale (GimpProcedureDialog *dialog,
                                      const gchar         *property,
                                      gdouble              factor)
{
  GimpProcedureDialogPrivate *priv;
  GtkWidget                  *widget = NULL;
  GParamSpec                 *pspec;
  gdouble                     minimum;
  gdouble                     maximum;
  gdouble                     step   = 0.0;
  gdouble                     page   = 0.0;
  gint                        digits = 0;

  g_return_val_if_fail (GIMP_IS_PROCEDURE_DIALOG (dialog), NULL);
  g_return_val_if_fail (property != NULL, NULL);

  priv  = gimp_procedure_dialog_get_instance_private (dialog);
  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (priv->config),
                                        property);

  if (! pspec)
    {
      g_warning ("%s: parameter %s does not exist.",
                 G_STRFUNC, property);
      return NULL;
    }

  g_return_val_if_fail (G_PARAM_SPEC_TYPE (pspec) == G_TYPE_PARAM_DOUBLE  ||
                        ((G_PARAM_SPEC_TYPE (pspec) == G_TYPE_PARAM_UINT  ||
                          G_PARAM_SPEC_TYPE (pspec) == G_TYPE_PARAM_INT)  &&
                         factor == 1.0), NULL);

  /* First check if it already exists. */
  widget = g_hash_table_lookup (priv->widgets, property);

  if (widget)
    return widget;

  if (G_PARAM_SPEC_TYPE (pspec) == G_TYPE_PARAM_INT)
    {
      GParamSpecInt *pspecint = (GParamSpecInt *) pspec;

      minimum = (gdouble) pspecint->minimum;
      maximum = (gdouble) pspecint->maximum;
    }
  else if (G_PARAM_SPEC_TYPE (pspec) == G_TYPE_PARAM_UINT)
    {
      GParamSpecUInt *pspecuint = (GParamSpecUInt *) pspec;

      minimum = (gdouble) pspecuint->minimum;
      maximum = (gdouble) pspecuint->maximum;
    }
  else /* G_TYPE_PARAM_DOUBLE */
    {
      GParamSpecDouble *pspecdouble = (GParamSpecDouble *) pspec;

      minimum = pspecdouble->minimum;
      maximum = pspecdouble->maximum;
    }
  gimp_range_estimate_settings (minimum * factor, maximum * factor, &step, &page, &digits);

  widget = gimp_prop_spin_scale_new (G_OBJECT (priv->config),
                                     property, step, page, digits);
  if (G_PARAM_SPEC_TYPE (pspec) == G_TYPE_PARAM_DOUBLE)
    gimp_prop_widget_set_factor (widget, factor, step, page, digits);

  gimp_procedure_dialog_check_mnemonic (dialog, widget, property, NULL);
  g_hash_table_insert (priv->widgets, g_strdup (property), widget);
  if (g_object_is_floating (widget))
    g_object_ref_sink (widget);

  return widget;
}

/**
 * gimp_procedure_dialog_get_scale_entry:
 * @dialog:   the associated #GimpProcedureDialog.
 * @property: name of the int property to build a combo for. It must be
 *            a property of the #GimpProcedure @dialog has been created
 *            for.
 * @factor:   a display factor for the range shown by the widget.
 *
 * Creates a new #GimpScaleEntry for @property which must necessarily be
 * an integer or double property.
 * This can be used instead of gimp_procedure_dialog_get_widget() in
 * particular if you want to tweak the display factor. A typical example
 * is showing a [0.0, 1.0] range as [0.0, 100.0] instead (@factor = 100.0).
 *
 * If a widget has already been created for this procedure, it will be
 * returned instead (whatever its actual widget type).
 *
 * Returns: (transfer none): the #GtkWidget representing @property. The
 *                           object belongs to @dialog and must not be
 *                           freed.
 */
GtkWidget *
gimp_procedure_dialog_get_scale_entry (GimpProcedureDialog *dialog,
                                       const gchar         *property,
                                       gdouble              factor)
{
  GimpProcedureDialogPrivate *priv;
  GtkWidget                  *widget = NULL;
  GParamSpec                 *pspec;

  g_return_val_if_fail (GIMP_IS_PROCEDURE_DIALOG (dialog), NULL);
  g_return_val_if_fail (property != NULL, NULL);

  priv  = gimp_procedure_dialog_get_instance_private (dialog);
  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (priv->config),
                                        property);

  if (! pspec)
    {
      g_warning ("%s: parameter %s does not exist.",
                 G_STRFUNC, property);
      return NULL;
    }

  g_return_val_if_fail (G_PARAM_SPEC_TYPE (pspec) == G_TYPE_PARAM_INT  ||
                        G_PARAM_SPEC_TYPE (pspec) == G_TYPE_PARAM_UINT ||
                        G_PARAM_SPEC_TYPE (pspec) == G_TYPE_PARAM_DOUBLE, NULL);

  /* First check if it already exists. */
  widget = g_hash_table_lookup (priv->widgets, property);

  if (widget)
    return widget;

  widget = gimp_prop_scale_entry_new (G_OBJECT (priv->config),
                                      property,
                                      g_param_spec_get_nick (pspec),
                                      factor, FALSE, 0.0, 0.0);

  gtk_size_group_add_widget (priv->label_group,
                             gimp_labeled_get_label (GIMP_LABELED (widget)));

  gimp_procedure_dialog_check_mnemonic (dialog, widget, property, NULL);
  g_hash_table_insert (priv->widgets, g_strdup (property), widget);
  if (g_object_is_floating (widget))
    g_object_ref_sink (widget);

  return widget;
}

/**
 * gimp_procedure_dialog_get_size_entry:
 * @dialog:            the associated #GimpProcedureDialog.
 * @property:          name of the int property to build an entry for.
 *                     It must be a property of the #GimpProcedure @dialog
 *                     has been created for.
 * @property_is_pixel: when %TRUE, the property value is in pixels,
 *                     and in the selected unit otherwise.
 * @unit_property:     name of unit property.
 * @unit_format:       a printf-like unit-format string used for unit
 *                     labels.
 * @update_policy:     how the automatic pixel <-> real-world-unit
 *                     calculations should be done.
 * @resolution:        the resolution (in dpi) for the field.
 *
 * Creates a new #GimpSizeEntry for @property which must necessarily be
 * an integer or double property. The associated @unit_property must be
 * a GimpUnit or integer property.
 *
 * If a widget has already been created for this procedure, it will be
 * returned instead (whatever its actual widget type).
 *
 * Returns: (transfer none): the #GtkWidget representing @property. The
 *                           object belongs to @dialog and must not be
 *                           freed.
 */
GtkWidget *
gimp_procedure_dialog_get_size_entry (GimpProcedureDialog       *dialog,
                                      const gchar               *property,
                                      gboolean                   property_is_pixel,
                                      const gchar               *unit_property,
                                      const gchar               *unit_format,
                                      GimpSizeEntryUpdatePolicy  update_policy,
                                      gdouble                    resolution)
{
  GimpProcedureDialogPrivate *priv;
  GtkWidget                  *widget = NULL;
  GtkWidget                  *label  = NULL;
  GtkSizeGroup               *group;
  GParamSpec                 *pspec;
  GParamSpec                 *pspec_unit;

  g_return_val_if_fail (GIMP_IS_PROCEDURE_DIALOG (dialog), NULL);
  g_return_val_if_fail (property != NULL, NULL);
  g_return_val_if_fail (unit_property != NULL, NULL);

  priv       = gimp_procedure_dialog_get_instance_private (dialog);
  pspec      = g_object_class_find_property (G_OBJECT_GET_CLASS (priv->config),
                                             property);
  pspec_unit = g_object_class_find_property (G_OBJECT_GET_CLASS (priv->config),
                                             unit_property);

  if (! pspec)
    {
      g_warning ("%s: parameter %s does not exist.",
                 G_STRFUNC, property);
      return NULL;
    }
  if (! pspec_unit)
    {
      g_warning ("%s: unit parameter %s does not exist.",
                 G_STRFUNC, unit_property);
      return NULL;
    }

  g_return_val_if_fail (G_PARAM_SPEC_TYPE (pspec) == G_TYPE_PARAM_INT  ||
                        G_PARAM_SPEC_TYPE (pspec) == G_TYPE_PARAM_UINT ||
                        G_PARAM_SPEC_TYPE (pspec) == G_TYPE_PARAM_DOUBLE, NULL);
  g_return_val_if_fail (G_PARAM_SPEC_TYPE (pspec_unit) == GIMP_TYPE_PARAM_UNIT, NULL);

  /* First check if it already exists. */
  widget = g_hash_table_lookup (priv->widgets, property);

  if (widget)
    return widget;

  widget = gimp_prop_size_entry_new (G_OBJECT (priv->config), property,
                                     property_is_pixel, unit_property,
                                     unit_format, update_policy, resolution);
  /* Add label */
  label = gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (widget),
                                        g_param_spec_get_nick (pspec), 1, 0, 0.0);
  gtk_widget_set_margin_end (label, 6);

  group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  gtk_size_group_add_widget (group, label);
  g_object_unref (group);

  g_hash_table_insert (priv->widgets, g_strdup (property), widget);
  if (g_object_is_floating (widget))
    g_object_ref_sink (widget);

  return widget;
}

/**
 * gimp_procedure_dialog_get_drawable_preview:
 * @dialog:     the #GimpProcedureDialog.
 * @preview_id: the ID of #GimpDrawablePreview.
 * @drawable:   the #GimpDrawable.
 *
 * Gets or creates a new #GimpDrawablePreview for @drawable.
 * If a widget with the @preview_id has already been created for
 * this procedure, it will be returned instead.
 *
 * The @preview_id ID can later be used together with property names
 * to be packed in other containers or inside @dialog itself.

 * Returns: (transfer none): the #GtkWidget representing @preview_id. The
 *                           object belongs to @dialog and must not be
 *                           freed.
 */
GtkWidget *
gimp_procedure_dialog_get_drawable_preview (GimpProcedureDialog *dialog,
                                            const gchar         *preview_id,
                                            GimpDrawable        *drawable)
{
  GimpProcedureDialogPrivate *priv = gimp_procedure_dialog_get_instance_private (dialog);
  GtkWidget                  *w    = g_hash_table_lookup (priv->widgets, preview_id);

  if (w != NULL)
    {
      g_warning ("%s: preview_from_drawable identifier '%s' was already configured.",
                 G_STRFUNC, preview_id);
      return w;
    }

  w = gimp_drawable_preview_new_from_drawable (drawable);

  g_hash_table_insert (priv->widgets, g_strdup (preview_id), w);
  if (g_object_is_floating (w))
    g_object_ref_sink (w);

  return w;
}

/**
 * gimp_procedure_dialog_get_label:
 * @dialog:    the #GimpProcedureDialog.
 * @label_id:  the label for the #GtkLabel.
 * @text:      the text for the label.
 * @is_markup: whether @text is formatted with Pango markup.
 * @with_mnemonic: whether @text contains a mnemonic character.
 *
 * Creates a new #GtkLabel with @text. It can be useful for packing
 * textual information in between property settings.
 *
 * If @label_id is an existing string property of the #GimpProcedureConfig
 * associated to @dialog, then it will sync to the property value. In this case,
 * @text should be %NULL.
 *
 * If @label_id is a unique ID which is neither the name of a property of the
 * #GimpProcedureConfig associated to @dialog, nor is it the ID of any
 * previously created label or container, it will be initialized to @text. This
 * ID can later be used together with property names to be packed in other
 * containers or inside @dialog itself.
 *
 * Returns: (transfer none): the #GtkWidget representing @label_id. The
 *                           object belongs to @dialog and must not be
 *                           freed.
 */
GtkWidget *
gimp_procedure_dialog_get_label (GimpProcedureDialog *dialog,
                                 const gchar         *label_id,
                                 const gchar         *text,
                                 gboolean             is_markup,
                                 gboolean             with_mnemonic)
{
  GimpProcedureDialogPrivate *priv;
  GtkWidget                  *label;
  GParamSpec                 *pspec;

  g_return_val_if_fail (label_id != NULL, NULL);

  priv  = gimp_procedure_dialog_get_instance_private (dialog);
  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (priv->config),
                                        label_id);
  if (pspec != NULL && G_PARAM_SPEC_TYPE (pspec) != G_TYPE_PARAM_STRING)
    {
      g_warning ("%s: label identifier '%s' must either not already exist or "
                 "be an existing string property.",
                 G_STRFUNC, label_id);
      return NULL;
    }

  if ((label = g_hash_table_lookup (priv->widgets, label_id)))
    {
      g_warning ("%s: label identifier '%s' was already configured.",
                 G_STRFUNC, label_id);
      return label;
    }

  label = gtk_label_new (NULL);
  g_object_set (label,
                "use-markup",    is_markup,
                "use-underline", with_mnemonic,
                NULL);
  if (pspec != NULL)
    g_object_bind_property (priv->config, label_id,
                            label,                "label",
                            G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
  else
    g_object_set (label, "label", text, NULL);

  g_hash_table_insert (priv->widgets, g_strdup (label_id), label);
  if (g_object_is_floating (label))
    g_object_ref_sink (label);

  return label;
}

/**
 * gimp_procedure_dialog_fill:
 * @dialog: the #GimpProcedureDialog.
 * @...: a %NULL-terminated list of property names.
 *
 * Populate @dialog with the widgets corresponding to every listed
 * properties. If the list is empty, @dialog will be filled by the whole
 * list of properties of the associated #GimpProcedure, in the defined
 * order:
 * |[<!-- language="C" -->
 * gimp_procedure_dialog_fill (dialog, NULL);
 * ]|
 * Nevertheless if you only wish to display a partial list of
 * properties, or if you wish to change the display order, then you have
 * to give an explicit list:
 * |[<!-- language="C" -->
 * gimp_procedure_dialog_fill (dialog, "property-1", "property-2", NULL);
 * ]|
 *
 * Note: you do not have to call gimp_procedure_dialog_get_widget() on
 * every property before calling this function unless you want a given
 * property to be represented by an alternative widget type. By default,
 * each property will get a default representation according to its
 * type.
 */
void
gimp_procedure_dialog_fill (GimpProcedureDialog *dialog,
                            ...)
{
  const gchar *prop_name;
  GList       *list      = NULL;
  va_list      va_args;

  g_return_if_fail (GIMP_IS_PROCEDURE_DIALOG (dialog));

  va_start (va_args, dialog);

  while ((prop_name = va_arg (va_args, const gchar *)))
    list = g_list_prepend (list, (gpointer) prop_name);

  va_end (va_args);

  list = g_list_reverse (list);
  gimp_procedure_dialog_fill_list (dialog, list);
  if (list)
    g_list_free (list);
}

/**
 * gimp_procedure_dialog_fill_list: (rename-to gimp_procedure_dialog_fill)
 * @dialog: the #GimpProcedureDialog.
 * @properties: (nullable) (element-type gchar*): the list of property names.
 *
 * Populate @dialog with the widgets corresponding to every listed
 * properties. If the list is %NULL, @dialog will be filled by the whole
 * list of properties of the associated #GimpProcedure, in the defined
 * order:
 * |[<!-- language="C" -->
 * gimp_procedure_dialog_fill_list (dialog, NULL);
 * ]|
 * Nevertheless if you only wish to display a partial list of
 * properties, or if you wish to change the display order, then you have
 * to give an explicit list:
 * |[<!-- language="C" -->
 * gimp_procedure_dialog_fill (dialog, "property-1", "property-2", NULL);
 * ]|
 *
 * Note: you do not have to call gimp_procedure_dialog_get_widget() on
 * every property before calling this function unless you want a given
 * property to be represented by an alternative widget type. By default,
 * each property will get a default representation according to its
 * type.
 */
void
gimp_procedure_dialog_fill_list (GimpProcedureDialog *dialog,
                                 GList               *properties)
{
  GimpProcedureDialogPrivate *priv;
  gboolean                    free_properties = FALSE;

  priv = gimp_procedure_dialog_get_instance_private (dialog);

  if (! priv->fill_started &&
      GIMP_PROCEDURE_DIALOG_GET_CLASS (dialog)->fill_start)
    {
      GIMP_PROCEDURE_DIALOG_GET_CLASS (dialog)->fill_start (dialog,
                                                            priv->procedure,
                                                            priv->config);
      priv->fill_started = TRUE;
    }

  if (! properties)
    {
      GParamSpec **pspecs;
      guint        n_pspecs;
      guint        i;

      pspecs = g_object_class_list_properties (G_OBJECT_GET_CLASS (priv->config),
                                               &n_pspecs);

      for (i = 0; i < n_pspecs; i++)
        {
          const gchar *prop_name;
          GParamSpec  *pspec = pspecs[i];

          /*  skip our own properties  */
          if (pspec->owner_type == GIMP_TYPE_PROCEDURE_CONFIG)
            continue;

          prop_name  = g_param_spec_get_name (pspec);
          properties = g_list_prepend (properties, (gpointer) prop_name);
        }

      properties = g_list_reverse (properties);

      if (properties)
        free_properties = TRUE;

      g_free (pspecs);
    }

  GIMP_PROCEDURE_DIALOG_GET_CLASS (dialog)->fill_list (dialog,
                                                       priv->procedure,
                                                       priv->config,
                                                       properties);

  if (free_properties)
    g_list_free (properties);
}

/**
 * gimp_procedure_dialog_fill_box:
 * @dialog:         the #GimpProcedureDialog.
 * @container_id:   a container identifier.
 * @first_property: the first property name.
 * @...:            a %NULL-terminated list of other property names.
 *
 * Creates and populates a new #GtkBox with widgets corresponding to
 * every listed properties. If the list is empty, the created box will
 * be filled by the whole list of properties of the associated
 * #GimpProcedure, in the defined order. This is similar of how
 * gimp_procedure_dialog_fill() works except that it creates a new
 * widget which is not inside @dialog itself.
 *
 * The @container_id must be a unique ID which is neither the name of a
 * property of the #GimpProcedureConfig associated to @dialog, nor is it
 * the ID of any previously created container. This ID can later be used
 * together with property names to be packed in other containers or
 * inside @dialog itself.
 *
 * Returns: (transfer none): the #GtkBox representing @property. The
 *                           object belongs to @dialog and must not be
 *                           freed.
 */
GtkWidget *
gimp_procedure_dialog_fill_box (GimpProcedureDialog *dialog,
                                const gchar         *container_id,
                                const gchar         *first_property,
                                ...)
{
  const gchar *prop_name = first_property;
  GtkWidget   *box;
  GList       *list      = NULL;
  va_list      va_args;

  g_return_val_if_fail (GIMP_IS_PROCEDURE_DIALOG (dialog), NULL);
  g_return_val_if_fail (container_id != NULL, NULL);

  if (first_property)
    {
      va_start (va_args, first_property);

      do
        list = g_list_prepend (list, (gpointer) prop_name);
      while ((prop_name = va_arg (va_args, const gchar *)));

      va_end (va_args);
    }

  list = g_list_reverse (list);
  box = gimp_procedure_dialog_fill_box_list (dialog, container_id, list);
  if (list)
    g_list_free (list);

  return box;
}

/**
 * gimp_procedure_dialog_fill_box_list: (rename-to gimp_procedure_dialog_fill_box)
 * @dialog:        the #GimpProcedureDialog.
 * @container_id:  a container identifier.
 * @properties: (nullable) (element-type gchar*): the list of property names.
 *
 * Creates and populates a new #GtkBox with widgets corresponding to
 * every listed @properties. If the list is empty, the created box will
 * be filled by the whole list of properties of the associated
 * #GimpProcedure, in the defined order. This is similar of how
 * gimp_procedure_dialog_fill() works except that it creates a new
 * widget which is not inside @dialog itself.
 *
 * The @container_id must be a unique ID which is neither the name of a
 * property of the #GimpProcedureConfig associated to @dialog, nor is it
 * the ID of any previously created container. This ID can later be used
 * together with property names to be packed in other containers or
 * inside @dialog itself.
 *
 * Returns: (transfer none): the #GtkBox representing @property. The
 *                           object belongs to @dialog and must not be
 *                           freed.
 */
GtkWidget *
gimp_procedure_dialog_fill_box_list (GimpProcedureDialog *dialog,
                                     const gchar         *container_id,
                                     GList               *properties)
{
  g_return_val_if_fail (container_id != NULL, NULL);

  return gimp_procedure_dialog_fill_container_list (dialog, container_id,
                                                    GTK_CONTAINER (gtk_box_new (GTK_ORIENTATION_VERTICAL, 2)),
                                                    properties);
}

/**
 * gimp_procedure_dialog_fill_flowbox:
 * @dialog:         the #GimpProcedureDialog.
 * @container_id:   a container identifier.
 * @first_property: the first property name.
 * @...:            a %NULL-terminated list of other property names.
 *
 * Creates and populates a new #GtkFlowBox with widgets corresponding to
 * every listed properties. If the list is empty, the created flowbox
 * will be filled by the whole list of properties of the associated
 * #GimpProcedure, in the defined order. This is similar of how
 * gimp_procedure_dialog_fill() works except that it creates a new
 * widget which is not inside @dialog itself.
 *
 * The @container_id must be a unique ID which is neither the name of a
 * property of the #GimpProcedureConfig associated to @dialog, nor is it
 * the ID of any previously created container. This ID can later be used
 * together with property names to be packed in other containers or
 * inside @dialog itself.
 *
 * Returns: (transfer none): the #GtkFlowBox representing @property. The
 *                           object belongs to @dialog and must not be
 *                           freed.
 */
GtkWidget *
gimp_procedure_dialog_fill_flowbox (GimpProcedureDialog *dialog,
                                    const gchar         *container_id,
                                    const gchar         *first_property,
                                    ...)
{
  const gchar *prop_name = first_property;
  GtkWidget   *flowbox;
  GList       *list      = NULL;
  va_list      va_args;

  g_return_val_if_fail (GIMP_IS_PROCEDURE_DIALOG (dialog), NULL);
  g_return_val_if_fail (container_id != NULL, NULL);

  if (first_property)
    {
      va_start (va_args, first_property);

      do
        list = g_list_prepend (list, (gpointer) prop_name);
      while ((prop_name = va_arg (va_args, const gchar *)));

      va_end (va_args);
    }

  list = g_list_reverse (list);
  flowbox = gimp_procedure_dialog_fill_flowbox_list (dialog, container_id, list);
  if (list)
    g_list_free (list);

  return flowbox;
}

/**
 * gimp_procedure_dialog_fill_flowbox_list: (rename-to gimp_procedure_dialog_fill_flowbox)
 * @dialog:        the #GimpProcedureDialog.
 * @container_id:  a container identifier.
 * @properties: (nullable) (element-type gchar*): the list of property names.
 *
 * Creates and populates a new #GtkFlowBox with widgets corresponding to
 * every listed @properties. If the list is empty, the created flowbox
 * will be filled by the whole list of properties of the associated
 * #GimpProcedure, in the defined order. This is similar of how
 * gimp_procedure_dialog_fill() works except that it creates a new
 * widget which is not inside @dialog itself.
 *
 * The @container_id must be a unique ID which is neither the name of a
 * property of the #GimpProcedureConfig associated to @dialog, nor is it
 * the ID of any previously created container. This ID can later be used
 * together with property names to be packed in other containers or
 * inside @dialog itself.
 *
 * Returns: (transfer none): the #GtkFlowBox representing @property. The
 *                           object belongs to @dialog and must not be
 *                           freed.
 */
GtkWidget *
gimp_procedure_dialog_fill_flowbox_list (GimpProcedureDialog *dialog,
                                         const gchar         *container_id,
                                         GList               *properties)
{
  g_return_val_if_fail (container_id != NULL, NULL);

  return gimp_procedure_dialog_fill_container_list (dialog, container_id,
                                                    GTK_CONTAINER (gtk_flow_box_new ()),
                                                    properties);
}


/**
 * gimp_procedure_dialog_fill_frame:
 * @dialog:        the #GimpProcedureDialog.
 * @container_id:  a container identifier.
 * @title_id: (nullable): the identifier for the title widget.
 * @invert_title:  whether to use the opposite value of @title_id if it
 *                 represents a boolean widget.
 * @contents_id: (nullable): the identifier for the contents.
 *
 * Creates a new #GtkFrame and packs @title_id as its title and
 * @contents_id as its child.
 * If @title_id represents a boolean property, its value will be used to
 * renders @contents_id sensitive or not. If @invert_title is TRUE, then
 * sensitivity binding is inverted.
 *
 * The @container_id must be a unique ID which is neither the name of a
 * property of the #GimpProcedureConfig associated to @dialog, nor is it
 * the ID of any previously created container. This ID can later be used
 * together with property names to be packed in other containers or
 * inside @dialog itself.
 *
 * Returns: (transfer none): the #GtkWidget representing @container_id. The
 *                           object belongs to @dialog and must not be
 *                           freed.
 */
GtkWidget *
gimp_procedure_dialog_fill_frame (GimpProcedureDialog *dialog,
                                  const gchar         *container_id,
                                  const gchar         *title_id,
                                  gboolean             invert_title,
                                  const gchar         *contents_id)
{
  GimpProcedureDialogPrivate *priv;
  GtkWidget                  *frame;
  GtkWidget                  *contents = NULL;
  GtkWidget                  *title    = NULL;

  g_return_val_if_fail (container_id != NULL, NULL);

  priv = gimp_procedure_dialog_get_instance_private (dialog);

  if (g_object_class_find_property (G_OBJECT_GET_CLASS (priv->config),
                                    container_id))
    {
      g_warning ("%s: frame identifier '%s' cannot be an existing property name.",
                 G_STRFUNC, container_id);
      return NULL;
    }

  if ((frame = g_hash_table_lookup (priv->widgets, container_id)))
    {
      g_warning ("%s: frame identifier '%s' was already configured.",
                 G_STRFUNC, container_id);
      return frame;
    }

  frame = gimp_frame_new (NULL);

  if (contents_id)
    {
      contents = gimp_procedure_dialog_get_widget (dialog, contents_id, G_TYPE_NONE);
      if (! contents)
        {
          g_warning ("%s: no property or configured widget with identifier '%s'.",
                     G_STRFUNC, contents_id);
          return frame;
        }

      gtk_container_add (GTK_CONTAINER (frame), contents);
      gtk_widget_show (contents);
    }

  if (title_id)
    {
      title = gimp_procedure_dialog_get_widget (dialog, title_id, G_TYPE_NONE);
      if (! title)
        {
          g_warning ("%s: no property or configured widget with identifier '%s'.",
                     G_STRFUNC, title_id);
          return frame;
        }

      gtk_frame_set_label_widget (GTK_FRAME (frame), title);
      gtk_widget_show (title);

      if (contents && (GTK_IS_CHECK_BUTTON (title) || GTK_IS_SWITCH (title)))
        {
          GBindingFlags flags = G_BINDING_SYNC_CREATE;

          if (invert_title)
            flags |= G_BINDING_INVERT_BOOLEAN;

          g_object_bind_property (title,    "active",
                                  contents, "sensitive",
                                  flags);
        }
    }

  g_hash_table_insert (priv->widgets, g_strdup (container_id), frame);
  if (g_object_is_floating (frame))
    g_object_ref_sink (frame);

  return frame;
}


/**
 * gimp_procedure_dialog_fill_expander:
 * @dialog:        the #GimpProcedureDialog.
 * @container_id:  a container identifier.
 * @title_id: (nullable): the identifier for the title widget.
 * @invert_title:  whether to use the opposite value of @title_id if it
 *                 represents a boolean widget.
 * @contents_id: (nullable): the identifier for the contents.
 *
 * Creates a new #GtkExpander and packs @title_id as its title
 * and @contents_id as content.
 * If @title_id represents a boolean property, its value will be used to
 * expand the #GtkExpander. If @invert_title is TRUE, then expand binding is
 * inverted.
 *
 * The @container_id must be a unique ID which is neither the name of a
 * property of the #GimpProcedureConfig associated to @dialog, nor is it
 * the ID of any previously created container. This ID can later be used
 * together with property names to be packed in other containers or
 * inside @dialog itself.
 *
 * Returns: (transfer none): the #GtkWidget representing @container_id. The
 *                           object belongs to @dialog and must not be
 *                           freed.
 */
GtkWidget *
gimp_procedure_dialog_fill_expander (GimpProcedureDialog *dialog,
                                     const gchar         *container_id,
                                     const gchar         *title_id,
                                     gboolean             invert_title,
                                     const gchar         *contents_id)
{
  GimpProcedureDialogPrivate *priv;
  GtkWidget                  *expander;
  GtkWidget                  *contents = NULL;
  GtkWidget                  *title    = NULL;

  g_return_val_if_fail (container_id != NULL, NULL);

  priv = gimp_procedure_dialog_get_instance_private (dialog);

  if (g_object_class_find_property (G_OBJECT_GET_CLASS (priv->config),
                                    container_id))
    {
      g_warning ("%s: expander identifier '%s' cannot be an existing property name.",
                 G_STRFUNC, container_id);
      return NULL;
    }

  if ((expander = g_hash_table_lookup (priv->widgets, container_id)))
    {
      g_warning ("%s: expander identifier '%s' was already configured.",
                 G_STRFUNC, container_id);
      return expander;
    }

  expander = gtk_expander_new (NULL);

  if (contents_id)
    {
      contents = gimp_procedure_dialog_get_widget (dialog, contents_id, G_TYPE_NONE);
      if (! contents)
        {
          g_warning ("%s: no property or configured widget with identifier '%s'.",
                     G_STRFUNC, contents_id);
          return expander;
        }

      gtk_container_add (GTK_CONTAINER (expander), contents);
      gtk_widget_show (contents);
    }

  if (title_id)
    {
      title = gimp_procedure_dialog_get_widget (dialog, title_id, G_TYPE_NONE);
      if (! title)
        {
          g_warning ("%s: no property or configured widget with identifier '%s'.",
                     G_STRFUNC, title_id);
          return expander;
        }

      gtk_expander_set_label_widget (GTK_EXPANDER (expander), title);
      gtk_expander_set_resize_toplevel (GTK_EXPANDER (expander), TRUE);
      gtk_widget_show (title);
      g_object_bind_property (title,    "sensitive",
                              expander, "sensitive",
                              G_BINDING_SYNC_CREATE);

      if (contents && (GTK_IS_CHECK_BUTTON (title) || GTK_IS_SWITCH (title)))
        {
          GBindingFlags flags = G_BINDING_SYNC_CREATE;
          gboolean      active;

          /* Workaround for connecting check button state to expanded state of
           * GtkExpander. This is required as GtkExpander do not pass click
           * events to label widget.
           * Please see https://bugzilla.gnome.org/show_bug.cgi?id=705971
           */
          if (invert_title)
            flags |= G_BINDING_INVERT_BOOLEAN;

          g_object_get (title, "active", &active, NULL);
          gtk_expander_set_expanded (GTK_EXPANDER (expander),
                                     invert_title ?  ! active : active);
          g_object_bind_property (expander, "expanded",
                                  title,    "active",
                                  flags);
        }
    }

  g_hash_table_insert (priv->widgets, g_strdup (container_id), expander);
  if (g_object_is_floating (expander))
    g_object_ref_sink (expander);

  return expander;
}

/**
 * gimp_procedure_dialog_fill_scrolled_window:
 * @dialog:         the #GimpProcedureDialog.
 * @container_id:   a container identifier.
 * @contents_id:    The identifier for the contents.
 *
 * Creates and populates a new #GtkScrolledWindow with a widget corresponding
 * to the declared content id.
 *
 * The @container_id must be a unique ID which is neither the name of a
 * property of the #GimpProcedureConfig associated to @dialog, nor is it
 * the ID of any previously created container. This ID can later be used
 * together with property names to be packed in other containers or
 * inside @dialog itself.
 *
 * Returns: (transfer none): the #GtkScrolledWindow representing @contents_id.
 *                           The object belongs to @dialog and must not be
 *                           freed.
 */
GtkWidget *
gimp_procedure_dialog_fill_scrolled_window (GimpProcedureDialog *dialog,
                                            const gchar         *container_id,
                                            const gchar         *contents_id)
{
  GtkWidget *scrolled_window;
  GList     *single_list = NULL;

  g_return_val_if_fail (GIMP_IS_PROCEDURE_DIALOG (dialog), NULL);
  g_return_val_if_fail (container_id != NULL, NULL);
  /* GtkScrolledWindow can only have one child */
  g_return_val_if_fail (contents_id != NULL, NULL);

  single_list = g_list_prepend (single_list, (gpointer) contents_id);

  scrolled_window =
    gimp_procedure_dialog_fill_container_list (dialog, container_id,
                                               GTK_CONTAINER (gtk_scrolled_window_new (NULL, NULL)),
                                               single_list);

  if (single_list)
    g_list_free (single_list);

  return scrolled_window;
}

/**
 * gimp_procedure_dialog_fill_notebook:
 * @dialog:       the #GimpProcedureDialog.
 * @container_id: a container identifier.
 * @label_id:     the first page's label.
 * @page_id:      the first page's contents.
 * @...:          a %NULL-terminated list of other property names.
 *
 * Creates and populates a new #GtkNotebook with widgets corresponding to every
 * listed properties.
 * This is similar of how gimp_procedure_dialog_fill() works except that it
 * creates a new widget which is not inside @dialog itself.
 *
 * The @container_id must be a unique ID which is neither the name of a
 * property of the #GimpProcedureConfig associated to @dialog, nor is it
 * the ID of any previously created container. This ID can later be used
 * together with property names to be packed in other containers or
 * inside @dialog itself.
 *
 * Returns: (transfer none): the #GtkNotebook representing @property. The
 *                           object belongs to @dialog and must not be
 *                           freed.
 */
GtkWidget *
gimp_procedure_dialog_fill_notebook (GimpProcedureDialog *dialog,
                                     const gchar         *container_id,
                                     const gchar         *label_id,
                                     const gchar         *page_id,
                                     ...)
{
  GtkWidget *notebook;
  GList     *label_list = NULL;
  GList     *page_list  = NULL;
  va_list    va_args;

  g_return_val_if_fail (GIMP_IS_PROCEDURE_DIALOG (dialog), NULL);
  g_return_val_if_fail (container_id != NULL, NULL);
  g_return_val_if_fail (label_id != NULL && page_id != NULL, NULL);

  va_start (va_args, page_id);

  do
    {
      label_list = g_list_prepend (label_list, (gpointer) label_id);
      page_list  = g_list_prepend (page_list, (gpointer) page_id);

      label_id = va_arg (va_args, const gchar *);
      page_id  = va_arg (va_args, const gchar *);
    }
  while (label_id != NULL && page_id != NULL);

  va_end (va_args);

  label_list = g_list_reverse (label_list);
  page_list  = g_list_reverse (page_list);
  notebook = gimp_procedure_dialog_fill_notebook_list (dialog, container_id, label_list, page_list);
  g_list_free (label_list);
  g_list_free (page_list);

  return notebook;
}

/**
 * gimp_procedure_dialog_fill_notebook_list: (rename-to gimp_procedure_dialog_fill_notebook)
 * @dialog:       the #GimpProcedureDialog.
 * @container_id: a container identifier.
 * @label_list: (not nullable) (element-type gchar*): the list of label IDs.
 * @page_list:  (not nullable) (element-type gchar*): the list of page IDs.
 *
 * Creates and populates a new #GtkNotebook with widgets corresponding to every
 * listed properties.
 * This is similar of how gimp_procedure_dialog_fill_list() works except that it
 * creates a new widget which is not inside @dialog itself.
 *
 * The @container_id must be a unique ID which is neither the name of a
 * property of the #GimpProcedureConfig associated to @dialog, nor is it
 * the ID of any previously created container. This ID can later be used
 * together with property names to be packed in other containers or
 * inside @dialog itself.
 *
 * Returns: (transfer none): the #GtkNotebook representing @property. The
 *                           object belongs to @dialog and must not be
 *                           freed.
 */
GtkWidget *
gimp_procedure_dialog_fill_notebook_list (GimpProcedureDialog *dialog,
                                          const gchar         *container_id,
                                          GList               *label_list,
                                          GList               *page_list)
{
  GimpProcedureDialogPrivate *priv;
  GtkWidget                  *notebook;
  GList                      *iter_label = label_list;
  GList                      *iter_page  = page_list;

  g_return_val_if_fail (container_id != NULL, NULL);
  g_return_val_if_fail (g_list_length (label_list) > 0 &&
                        g_list_length (label_list) == g_list_length (page_list), NULL);

  priv = gimp_procedure_dialog_get_instance_private (dialog);

  if (g_object_class_find_property (G_OBJECT_GET_CLASS (priv->config),
                                    container_id))
    {
      g_warning ("%s: container identifier '%s' cannot be an existing property name.",
                 G_STRFUNC, container_id);
      return NULL;
    }

  if (g_hash_table_lookup (priv->widgets, container_id))
    {
      g_warning ("%s: container identifier '%s' was already configured.",
                 G_STRFUNC, container_id);
      return g_hash_table_lookup (priv->widgets, container_id);
    }

  notebook = gtk_notebook_new ();
  g_object_ref_sink (notebook);

  for (; iter_label; iter_label = iter_label->next, iter_page = iter_page->next)
    {
      GtkWidget *label;
      GtkWidget *page;

      label = gimp_procedure_dialog_get_widget (dialog, iter_label->data, G_TYPE_NONE);
      page  = gimp_procedure_dialog_get_widget (dialog, iter_page->data, G_TYPE_NONE);
      if (label != NULL && page != NULL)
        {
          gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page, label);
          gtk_widget_show (label);
          gtk_widget_show (page);
        }
    }

  g_hash_table_insert (priv->widgets, g_strdup (container_id), notebook);

  return notebook;
}

/**
 * gimp_procedure_dialog_fill_paned:
 * @dialog:       the #GimpProcedureDialog.
 * @container_id: a container identifier.
 * @child1_id:    (nullable): the first child's ID.
 * @child2_id:    (nullable): the second child's ID.
 *
 * Creates and populates a new #GtkPaned containing widgets corresponding to
 * @child1_id and @child2_id.
 * This is similar of how gimp_procedure_dialog_fill() works except that it
 * creates a new widget which is not inside @dialog itself.
 *
 * The @container_id must be a unique ID which is neither the name of a
 * property of the #GimpProcedureConfig associated to @dialog, nor is it
 * the ID of any previously created container. This ID can later be used
 * together with property names to be packed in other containers or
 * inside @dialog itself.
 *
 * Returns: (transfer none): the #GtkPaned representing @property. The
 *                           object belongs to @dialog and must not be
 *                           freed.
 */
GtkWidget *
gimp_procedure_dialog_fill_paned (GimpProcedureDialog *dialog,
                                  const gchar         *container_id,
                                  GtkOrientation       orientation,
                                  const gchar         *child1_id,
                                  const gchar         *child2_id)
{
  GimpProcedureDialogPrivate *priv;
  GtkWidget                  *paned;
  GtkWidget                  *child1;
  GtkWidget                  *child2;

  g_return_val_if_fail (container_id != NULL, NULL);

  priv = gimp_procedure_dialog_get_instance_private (dialog);

  if (g_object_class_find_property (G_OBJECT_GET_CLASS (priv->config),
                                    container_id))
    {
      g_warning ("%s: container identifier '%s' cannot be an existing property name.",
                 G_STRFUNC, container_id);
      return NULL;
    }

  if (g_hash_table_lookup (priv->widgets, container_id))
    {
      g_warning ("%s: container identifier '%s' was already configured.",
                 G_STRFUNC, container_id);
      return g_hash_table_lookup (priv->widgets, container_id);
    }

  paned = gtk_paned_new (orientation);
  g_object_ref_sink (paned);

  if (child1_id != NULL)
    {
      child1 = gimp_procedure_dialog_get_widget (dialog, child1_id, G_TYPE_NONE);
      gtk_paned_pack1 (GTK_PANED (paned), child1, TRUE, FALSE);
      gtk_widget_show (child1);
    }
  if (child2_id != NULL)
    {
      child2 = gimp_procedure_dialog_get_widget (dialog, child2_id, G_TYPE_NONE);
      gtk_paned_pack2 (GTK_PANED (paned), child2, TRUE, FALSE);
      gtk_widget_show (child2);
    }

  g_hash_table_insert (priv->widgets, g_strdup (container_id), paned);

  return paned;
}

/**
 * gimp_procedure_dialog_set_sensitive:
 * @dialog:          the #GimpProcedureDialog.
 * @property:        name of a property of the #GimpProcedure @dialog
 *                   has been created for.
 * @sensitive:       whether the widget associated to @property should
 *                   be sensitive.
 * @config:          (nullable): an optional config object.
 * @config_property: (nullable): name of a property of @config.
 * @config_invert:   whether to negate the value of @config_property.
 *
 * Sets sensitivity of the widget associated to @property in @dialog. If
 * @config is %NULL, then it is set to the value of @sensitive.
 * Otherwise @sensitive is ignored and sensitivity is bound to the value
 * of @config_property of @config (or the negation of this value
 * if @config_reverse is %TRUE).
 */
void
gimp_procedure_dialog_set_sensitive (GimpProcedureDialog *dialog,
                                     const gchar         *property,
                                     gboolean             sensitive,
                                     GObject             *config,
                                     const gchar         *config_property,
                                     gboolean             config_invert)
{
  GimpProcedureDialogPrivate *priv;
  GtkWidget                  *widget = NULL;
  GParamSpec                 *pspec;

  g_return_if_fail (GIMP_IS_PROCEDURE_DIALOG (dialog));
  g_return_if_fail (property != NULL);
  g_return_if_fail (config == NULL || config_property != NULL);

  priv   = gimp_procedure_dialog_get_instance_private (dialog);
  widget = g_hash_table_lookup (priv->widgets, property);

  if (! widget)
    {
      pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (priv->config),
                                            property);
      if (! pspec)
        {
          g_warning ("%s: parameter %s does not exist on the GimpProcedure.",
                     G_STRFUNC, property);
          return;
        }
    }

  if (config)
    {
      pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (config),
                                            config_property);
      if (! pspec)
        {
          g_warning ("%s: parameter %s does not exist on the config object.",
                     G_STRFUNC, config_property);
          return;
        }
    }

  if (widget)
    {
      if (config)
        {
          g_object_bind_property (config, config_property,
                                  widget, "sensitive",
                                  G_BINDING_SYNC_CREATE | (config_invert ? G_BINDING_INVERT_BOOLEAN : 0));
        }
      else
        {
          gtk_widget_set_sensitive (widget, sensitive);
        }
    }
  else
    {
      /* Set for later creation. */
      GimpProcedureDialogSensitiveData *data;

      data = g_slice_new0 (GimpProcedureDialogSensitiveData);

      data->sensitive = sensitive;
      if (config)
        {
          data->config          = g_object_ref (config);
          data->config_property = g_strdup (config_property);
          data->config_invert   = config_invert;
        }

      g_hash_table_insert (priv->sensitive_data, g_strdup (property), data);
    }
}

/**
 * gimp_procedure_dialog_set_sensitive_if_in:
 * @dialog:          the #GimpProcedureDialog.
 * @property:        name of a property of the #GimpProcedure @dialog
 *                   has been created for.
 * @config:          (nullable): an optional config object (if %NULL,
 *                   @property's config will be used).
 * @config_property: name of a property of @config.
 * @values:          (not nullable) (transfer full):
 *                   an array of GValues which could be values of @config_property.
 * @in_values:       whether @property should be sensitive when @config_property
 *                   is one of @values, or the opposite.
 *
 * Sets sensitivity of the widget associated to @property in @dialog if the
 * value of @config_property in @config is equal to one of @values.
 *
 * If @config is %NULL, then the configuration object of @dialog is used.
 *
 * If @in_values is FALSE, then the widget is set sensitive if the value of
 * @config_property is **not** in @values.
 */
void
gimp_procedure_dialog_set_sensitive_if_in (GimpProcedureDialog *dialog,
                                           const gchar         *property,
                                           GObject             *config,
                                           const gchar         *config_property,
                                           GimpValueArray      *values,
                                           gboolean             in_values)
{
  GimpProcedureDialogPrivate        *priv;
  GimpProcedureDialogSensitiveData2 *data;
  GParamSpec                        *pspec;
  gchar                             *signal_name;

  g_return_if_fail (GIMP_IS_PROCEDURE_DIALOG (dialog));
  g_return_if_fail (property != NULL);
  g_return_if_fail (config_property != NULL);
  g_return_if_fail (values != NULL);

  priv  = gimp_procedure_dialog_get_instance_private (dialog);
  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (priv->config),
                                        property);
  if (! pspec)
    {
      g_warning ("%s: parameter %s does not exist on the GimpProcedure.",
                 G_STRFUNC, property);
      return;
    }

  if (! config)
    config = G_OBJECT (priv->config);

  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (config),
                                        config_property);
  if (! pspec)
    {
      g_warning ("%s: parameter %s does not exist on the config object.",
                 G_STRFUNC, config_property);
      return;
    }

  data = g_new (GimpProcedureDialogSensitiveData2, 1);
  data->dialog           = dialog;
  data->widget_property  = g_strdup (property);
  data->values           = values;
  data->in_values        = in_values;

  signal_name = g_strconcat ("notify::", config_property, NULL);

  g_signal_connect_data (config, signal_name,
                         G_CALLBACK (gimp_procedure_dialog_set_sensitive_if_in_cb),
                         data,
                         (GClosureNotify) gimp_procedure_dialog_sensitive_cb_data_free,
                         0);
  gimp_procedure_dialog_set_sensitive_if_in_cb (config, pspec, data);
  g_free (signal_name);
}

/**
 * gimp_procedure_dialog_run:
 * @dialog: the #GimpProcedureDialog.
 *
 * Show @dialog and only returns when the user finished interacting with
 * it (either validating choices or canceling).
 *
 * Returns: %TRUE if the dialog was validated, %FALSE otherwise.
 */
gboolean
gimp_procedure_dialog_run (GimpProcedureDialog *dialog)
{
  GimpProcedureDialogPrivate *priv;

  g_return_val_if_fail (GIMP_IS_PROCEDURE_DIALOG (dialog), FALSE);

  priv = gimp_procedure_dialog_get_instance_private (dialog);

  if (! priv->fill_started &&
      GIMP_PROCEDURE_DIALOG_GET_CLASS (dialog)->fill_start)
    {
      GIMP_PROCEDURE_DIALOG_GET_CLASS (dialog)->fill_start (dialog,
                                                            priv->procedure,
                                                            priv->config);
      priv->fill_started = TRUE;
    }

  if (! priv->fill_ended &&
      GIMP_PROCEDURE_DIALOG_GET_CLASS (dialog)->fill_end)
    {
      GIMP_PROCEDURE_DIALOG_GET_CLASS (dialog)->fill_end (dialog,
                                                          priv->procedure,
                                                          priv->config);
      priv->fill_ended = TRUE;
    }

  while (TRUE)
    {
      gint response = gimp_dialog_run (GIMP_DIALOG (dialog));

      if (response == RESPONSE_RESET)
        {
          if (! priv->reset_popover)
            {
              GtkWidget *button;
              GtkWidget *vbox;

              button = gtk_dialog_get_widget_for_response (GTK_DIALOG (dialog),
                                                           response);

              priv->reset_popover = gtk_popover_new (button);

              vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);
              gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
              gtk_container_add (GTK_CONTAINER (priv->reset_popover),
                                 vbox);
              gtk_widget_show (vbox);

              button = gtk_button_new_with_mnemonic (_("Reset to _Initial "
                                                       "Values"));
              gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
              gtk_widget_show (button);

              g_signal_connect (button, "clicked",
                                G_CALLBACK (gimp_procedure_dialog_reset_initial),
                                dialog);

              button = gtk_button_new_with_mnemonic (_("Reset to _Factory "
                                                       "Defaults"));
              gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
              gtk_widget_show (button);

              g_signal_connect (button, "clicked",
                                G_CALLBACK (gimp_procedure_dialog_reset_factory),
                                dialog);
            }

          gtk_popover_popup (GTK_POPOVER (priv->reset_popover));
        }
      else
        {
          return response == GTK_RESPONSE_OK;
        }
    }
}


/*  private functions  */

static void
gimp_procedure_dialog_reset_initial (GtkWidget           *button,
                                     GimpProcedureDialog *dialog)
{
  GimpProcedureDialogPrivate *priv;

  priv = gimp_procedure_dialog_get_instance_private (dialog);

  gimp_config_copy (GIMP_CONFIG (priv->initial_config),
                    GIMP_CONFIG (priv->config),
                    0);

  gtk_popover_popdown (GTK_POPOVER (priv->reset_popover));
}

static void
gimp_procedure_dialog_reset_factory (GtkWidget           *button,
                                     GimpProcedureDialog *dialog)
{
  GimpProcedureDialogPrivate *priv;

  priv = gimp_procedure_dialog_get_instance_private (dialog);

  gimp_config_reset (GIMP_CONFIG (priv->config));

  gtk_popover_popdown (GTK_POPOVER (priv->reset_popover));
}

static void
gimp_procedure_dialog_load_defaults (GtkWidget           *button,
                                     GimpProcedureDialog *dialog)
{
  GimpProcedureDialogPrivate *priv;
  GError                     *error = NULL;

  priv = gimp_procedure_dialog_get_instance_private (dialog);

  if (! gimp_procedure_config_load_default (priv->config, &error))
    {
      if (error)
        {
          g_printerr ("Loading default values from disk failed: %s\n",
                      error->message);
          g_clear_error (&error);
        }
      else
        {
          g_printerr ("No default values found on disk\n");
        }
    }
}

static void
gimp_procedure_dialog_save_defaults (GtkWidget           *button,
                                     GimpProcedureDialog *dialog)
{
  GimpProcedureDialogPrivate *priv;
  GError                     *error = NULL;

  priv = gimp_procedure_dialog_get_instance_private (dialog);

  if (! gimp_procedure_config_save_default (priv->config, &error))
    {
      g_printerr ("Saving default values to disk failed: %s\n",
                  error->message);
      g_clear_error (&error);
    }
  gtk_widget_set_sensitive (priv->load_settings_button,
                            gimp_procedure_config_has_default (priv->config));
}

static gboolean
gimp_procedure_dialog_check_mnemonic (GimpProcedureDialog *dialog,
                                      GtkWidget           *widget,
                                      const gchar         *id,
                                      const gchar         *core_id)
{
  GimpProcedureDialogPrivate *priv;
  GtkWidget                  *label    = NULL;
  gchar                      *duplicate;
  gboolean                    success  = TRUE;
  guint                       mnemonic = GDK_KEY_VoidSymbol;

  g_return_val_if_fail ((id && ! core_id) || (core_id && ! id), FALSE);

  priv = gimp_procedure_dialog_get_instance_private (dialog);

  if (GIMP_IS_LABELED (widget))
    {
      label = gimp_labeled_get_label (GIMP_LABELED (widget));
    }
  else if (GIMP_IS_RESOURCE_CHOOSER (widget))
    {
      label = gimp_resource_chooser_get_label (GIMP_RESOURCE_CHOOSER (widget));
    }
  else if (GIMP_IS_DRAWABLE_CHOOSER (widget))
    {
      label = gimp_drawable_chooser_get_label (GIMP_DRAWABLE_CHOOSER (widget));
    }
  else if (GIMP_IS_FILE_CHOOSER (widget))
    {
      label = gimp_file_chooser_get_label_widget (GIMP_FILE_CHOOSER (widget));
    }
  else
    {
      GList *labels = gtk_widget_list_mnemonic_labels (widget);

      if (g_list_length (labels) >= 1)
        {
          if (g_list_length (labels) > 1)
            g_printerr ("Procedure '%s': %d mnemonics for property %s. Too much?\n",
                        gimp_procedure_get_name (priv->procedure),
                        g_list_length (labels),
                        id ? id : core_id);

          label = labels->data;
        }

      g_list_free (labels);
    }

  if (label)
    mnemonic = gtk_label_get_mnemonic_keyval (GTK_LABEL (label));
  else if (GIMP_IS_SPIN_SCALE (widget))
    mnemonic = gimp_spin_scale_get_mnemonic_keyval (GIMP_SPIN_SCALE (widget));

  if (mnemonic != GDK_KEY_VoidSymbol)
    {
      duplicate = g_hash_table_lookup (priv->core_mnemonics, GINT_TO_POINTER (mnemonic));
      if (duplicate && g_strcmp0 (duplicate, id ? id : core_id) != 0)
        {
          g_printerr ("Procedure '%s': duplicate mnemonic %s for label of property %s and dialog button %s\n",
                      gimp_procedure_get_name (priv->procedure),
                      gdk_keyval_name (mnemonic), id, duplicate);
          success = FALSE;
        }

      if (success)
        {
          duplicate = g_hash_table_lookup (priv->mnemonics, GINT_TO_POINTER (mnemonic));
          if (duplicate && g_strcmp0 (duplicate, id ? id : core_id) != 0)
            {
              g_printerr ("Procedure '%s': duplicate mnemonic %s for label of properties %s and %s\n",
                          gimp_procedure_get_name (priv->procedure),
                          gdk_keyval_name (mnemonic), id, duplicate);
              success = FALSE;
            }
          else if (! duplicate)
            {
              if (id)
                g_hash_table_insert (priv->mnemonics, GINT_TO_POINTER (mnemonic), g_strdup (id));
              else
                g_hash_table_insert (priv->core_mnemonics, GINT_TO_POINTER (mnemonic), g_strdup (core_id));
            }
        }
    }
  else
    {
      g_printerr ("Procedure '%s': no mnemonic for property %s\n",
                  gimp_procedure_get_name (priv->procedure),
                  id ? id : core_id);
      success = FALSE;
    }

  return success;
}

/**
 * gimp_procedure_dialog_fill_container_list:
 * @dialog:        the #GimpProcedureDialog.
 * @container_id:  a container identifier.
 * @container: (transfer full): The new container that should be used if none
 *                              exists yet
 * @properties: (nullable) (element-type gchar*): the list of property names.
 *
 * A generic function to be used by various public functions
 * gimp_procedure_dialog_fill_*_list(). Note in particular that
 * @container is taken over by this function which may return it or not.
 * @container is assumed to be a floating GtkContainer (i.e. newly
 * created widget without a parent yet).
 * If the object returns a different object (because @container_id
 * already represents another widget) or %NULL, the function takes care
 * of freeing @container. Calling code must therefore not reuse the
 * pointer anymore.
 */
static GtkWidget *
gimp_procedure_dialog_fill_container_list (GimpProcedureDialog *dialog,
                                           const gchar         *container_id,
                                           GtkContainer        *container,
                                           GList               *properties)
{
  GimpProcedureDialogPrivate *priv;
  GList                      *iter;
  gboolean                    free_properties = FALSE;
  GtkSizeGroup               *sz_group;

  g_return_val_if_fail (container_id != NULL, NULL);
  g_return_val_if_fail (GTK_IS_CONTAINER (container), NULL);
  g_return_val_if_fail (g_object_is_floating (G_OBJECT (container)), NULL);

  priv = gimp_procedure_dialog_get_instance_private (dialog);

  g_object_ref_sink (container);
  if (g_object_class_find_property (G_OBJECT_GET_CLASS (priv->config),
                                    container_id))
    {
      g_warning ("%s: container identifier '%s' cannot be an existing property name.",
                 G_STRFUNC, container_id);
      g_object_unref (container);
      return NULL;
    }

  if (g_hash_table_lookup (priv->widgets, container_id))
    {
      g_warning ("%s: container identifier '%s' was already configured.",
                 G_STRFUNC, container_id);
      g_object_unref (container);
      return g_hash_table_lookup (priv->widgets, container_id);
    }

  if (! properties)
    {
      GParamSpec **pspecs;
      guint        n_pspecs;
      guint        i;

      pspecs = g_object_class_list_properties (G_OBJECT_GET_CLASS (priv->config),
                                               &n_pspecs);

      for (i = 0; i < n_pspecs; i++)
        {
          const gchar *prop_name;
          GParamSpec  *pspec = pspecs[i];

          /*  skip our own properties  */
          if (pspec->owner_type == GIMP_TYPE_PROCEDURE_CONFIG)
            continue;

          prop_name  = g_param_spec_get_name (pspec);
          properties = g_list_prepend (properties, (gpointer) prop_name);
        }

      properties = g_list_reverse (properties);

      if (properties)
        free_properties = TRUE;

      g_free (pspecs);
    }

  sz_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  for (iter = properties; iter; iter = iter->next)
    {
      GtkWidget *widget;

      widget = gimp_procedure_dialog_get_widget (dialog, iter->data, G_TYPE_NONE);
      if (widget)
        {
          gtk_container_add (container, widget);
          if (GIMP_IS_LABELED (widget))
            {
              GtkWidget *label = gimp_labeled_get_label (GIMP_LABELED (widget));
              gtk_size_group_remove_widget (priv->label_group, label);
              gtk_size_group_add_widget (sz_group, label);
            }
          gtk_widget_show (widget);
        }
    }
  g_clear_object (&sz_group);

  if (free_properties)
    g_list_free (properties);

  g_hash_table_insert (priv->widgets, g_strdup (container_id), container);
  if (g_object_is_floating (container))
    g_object_ref_sink (container);

  return GTK_WIDGET (container);
}

static void
gimp_procedure_dialog_set_sensitive_if_in_cb (GObject                           *config,
                                              GParamSpec                        *param_spec,
                                              GimpProcedureDialogSensitiveData2 *data)
{
  GimpProcedureDialogPrivate *priv;
  GimpProcedureDialog        *dialog = data->dialog;
  GtkWidget                  *widget;

  priv   = gimp_procedure_dialog_get_instance_private (dialog);
  widget = g_hash_table_lookup (priv->widgets, data->widget_property);

  if (widget)
    {
      GValue   param_value = G_VALUE_INIT;
      gboolean sensitive;
      gint     n_values    = gimp_value_array_length (data->values);

      g_value_init (&param_value, param_spec->value_type);
      g_object_get_property (config, param_spec->name, &param_value);

      sensitive = (! data->in_values);
      for (gint i = 0; i < n_values; i++)
        {
          GValue *value;

          value = gimp_value_array_index (data->values, i);

          if (g_param_values_cmp (param_spec, &param_value, value) == 0)
            {
              sensitive = data->in_values;
              break;
            }
        }
      gtk_widget_set_sensitive (widget, sensitive);
      g_value_unset (&param_value);
    }
  else
    {
      g_printerr ("gimp_procedure_dialog_set_sensitive_if_in: "
                  "no widget was created for property \"%s\".\n",
                  data->widget_property);
    }
}

static void
gimp_procedure_dialog_sensitive_data_free (GimpProcedureDialogSensitiveData *data)
{
  g_free (data->config_property);
  g_clear_object (&data->config);

  g_slice_free (GimpProcedureDialogSensitiveData, data);
}

static void
gimp_procedure_dialog_sensitive_cb_data_free (GimpProcedureDialogSensitiveData2 *data)
{
  g_free (data->widget_property);
  gimp_value_array_unref (data->values);

  g_free (data);
}
