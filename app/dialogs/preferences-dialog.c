/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "dialogs-types.h"

#include "config/gimprc.h"

#include "core/gimp.h"
#include "core/gimptemplate.h"

#include "plug-in/gimppluginmanager.h"

#include "widgets/gimpaction-history.h"
#include "widgets/gimpcolorpanel.h"
#include "widgets/gimpcontainercombobox.h"
#include "widgets/gimpcontainerview.h"
#include "widgets/gimpcontrollerlist.h"
#include "widgets/gimpdevices.h"
#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpgrideditor.h"
#include "widgets/gimphelp.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpiconsizescale.h"
#include "widgets/gimplanguagecombobox.h"
#include "widgets/gimpmessagebox.h"
#include "widgets/gimpmessagedialog.h"
#include "widgets/gimppluginview.h"
#include "widgets/gimpprefsbox.h"
#include "widgets/gimppropwidgets.h"
#include "widgets/gimpstrokeeditor.h"
#include "widgets/gimptemplateeditor.h"
#include "widgets/gimptooleditor.h"
#include "widgets/gimpwidgets-utils.h"

#include "menus/menus.h"

#include "tools/gimp-tools.h"

#include "gui/session.h"
#include "gui/icon-themes.h"
#include "gui/themes.h"

#include "preferences-dialog.h"
#include "preferences-dialog-utils.h"
#include "resolution-calibrate-dialog.h"

#include "gimp-intl.h"


#define RESPONSE_RESET 1


/*  preferences local functions  */

static GtkWidget * prefs_dialog_new                (Gimp       *gimp,
                                                    GimpConfig *config);
static void        prefs_config_notify             (GObject    *config,
                                                    GParamSpec *param_spec,
                                                    GObject    *config_copy);
static void        prefs_config_copy_notify        (GObject    *config_copy,
                                                    GParamSpec *param_spec,
                                                    GObject    *config);
static void        prefs_response                  (GtkWidget  *widget,
                                                    gint        response_id,
                                                    GtkWidget  *dialog);

static void        prefs_message                   (GtkMessageType  type,
                                                    gboolean        destroy,
                                                    const gchar    *message);

static void   prefs_color_management_reset         (GtkWidget    *widget,
                                                    GObject      *config);
static void   prefs_dialog_defaults_reset          (GtkWidget    *widget,
                                                    GObject      *config);

static void   prefs_import_raw_procedure_callback  (GtkWidget    *widget,
                                                    GObject      *config);
static void   prefs_resolution_source_callback     (GtkWidget    *widget,
                                                    GObject      *config);
static void   prefs_resolution_calibrate_callback  (GtkWidget    *widget,
                                                    GtkWidget    *entry);
static void   prefs_input_devices_dialog           (GtkWidget    *widget,
                                                    Gimp         *gimp);
static void   prefs_keyboard_shortcuts_dialog      (GtkWidget    *widget,
                                                    Gimp         *gimp);
static void   prefs_menus_save_callback            (GtkWidget    *widget,
                                                    Gimp         *gimp);
static void   prefs_menus_clear_callback           (GtkWidget    *widget,
                                                    Gimp         *gimp);
static void   prefs_menus_remove_callback          (GtkWidget    *widget,
                                                    Gimp         *gimp);
static void   prefs_session_save_callback          (GtkWidget    *widget,
                                                    Gimp         *gimp);
static void   prefs_session_clear_callback         (GtkWidget    *widget,
                                                    Gimp         *gimp);
static void   prefs_devices_save_callback          (GtkWidget    *widget,
                                                    Gimp         *gimp);
static void   prefs_devices_clear_callback         (GtkWidget    *widget,
                                                    Gimp         *gimp);
static void   prefs_search_clear_callback          (GtkWidget    *widget,
                                                    Gimp         *gimp);
static void   prefs_tool_options_save_callback     (GtkWidget    *widget,
                                                    Gimp         *gimp);
static void   prefs_tool_options_clear_callback    (GtkWidget    *widget,
                                                    Gimp         *gimp);
static void   prefs_help_language_change_callback  (GtkComboBox  *combo,
                                                    Gimp         *gimp);
static void   prefs_help_language_change_callback2 (GtkComboBox  *combo,
                                                    GtkContainer *box);


/*  private variables  */

static GtkWidget *prefs_dialog = NULL;
static GtkWidget *tool_editor  = NULL;


/*  public function  */

GtkWidget *
preferences_dialog_create (Gimp *gimp)
{
  GimpConfig *config;
  GimpConfig *config_copy;
  GimpConfig *config_orig;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  if (prefs_dialog)
    return prefs_dialog;

  /*  turn off autosaving while the prefs dialog is open  */
  gimp_rc_set_autosave (GIMP_RC (gimp->edit_config), FALSE);

  config       = GIMP_CONFIG (gimp->edit_config);
  config_copy  = gimp_config_duplicate (config);
  config_orig  = gimp_config_duplicate (config);

  g_signal_connect_object (config, "notify",
                           G_CALLBACK (prefs_config_notify),
                           config_copy, 0);
  g_signal_connect_object (config_copy, "notify",
                           G_CALLBACK (prefs_config_copy_notify),
                           config, 0);

  prefs_dialog = prefs_dialog_new (gimp, config_copy);

  g_object_add_weak_pointer (G_OBJECT (prefs_dialog),
                             (gpointer) &prefs_dialog);

  g_object_set_data (G_OBJECT (prefs_dialog), "gimp", gimp);

  g_object_set_data_full (G_OBJECT (prefs_dialog), "config-copy", config_copy,
                          (GDestroyNotify) g_object_unref);
  g_object_set_data_full (G_OBJECT (prefs_dialog), "config-orig", config_orig,
                          (GDestroyNotify) g_object_unref);

  return prefs_dialog;
}


/*  private functions  */

static void
prefs_config_notify (GObject    *config,
                     GParamSpec *param_spec,
                     GObject    *config_copy)
{
  GValue global_value = G_VALUE_INIT;
  GValue copy_value   = G_VALUE_INIT;

  g_value_init (&global_value, param_spec->value_type);
  g_value_init (&copy_value,   param_spec->value_type);

  g_object_get_property (config,      param_spec->name, &global_value);
  g_object_get_property (config_copy, param_spec->name, &copy_value);

  if (g_param_values_cmp (param_spec, &global_value, &copy_value))
    {
      g_signal_handlers_block_by_func (config_copy,
                                       prefs_config_copy_notify,
                                       config);

      g_object_set_property (config_copy, param_spec->name, &global_value);

      g_signal_handlers_unblock_by_func (config_copy,
                                         prefs_config_copy_notify,
                                         config);
    }

  g_value_unset (&global_value);
  g_value_unset (&copy_value);
}

static void
prefs_config_copy_notify (GObject    *config_copy,
                          GParamSpec *param_spec,
                          GObject    *config)
{
  GValue copy_value   = G_VALUE_INIT;
  GValue global_value = G_VALUE_INIT;

  g_value_init (&copy_value,   param_spec->value_type);
  g_value_init (&global_value, param_spec->value_type);

  g_object_get_property (config_copy, param_spec->name, &copy_value);
  g_object_get_property (config,      param_spec->name, &global_value);

  if (g_param_values_cmp (param_spec, &copy_value, &global_value))
    {
      if (param_spec->flags & GIMP_CONFIG_PARAM_CONFIRM)
        {
#ifdef GIMP_CONFIG_DEBUG
          g_print ("NOT Applying prefs change of '%s' to edit_config "
                   "because it needs confirmation\n",
                   param_spec->name);
#endif
        }
      else
        {
#ifdef GIMP_CONFIG_DEBUG
          g_print ("Applying prefs change of '%s' to edit_config\n",
                   param_spec->name);
#endif
          g_signal_handlers_block_by_func (config,
                                           prefs_config_notify,
                                           config_copy);

          g_object_set_property (config, param_spec->name, &copy_value);

          g_signal_handlers_unblock_by_func (config,
                                             prefs_config_notify,
                                             config_copy);
        }
    }

  g_value_unset (&copy_value);
  g_value_unset (&global_value);
}

static void
prefs_response (GtkWidget *widget,
                gint       response_id,
                GtkWidget *dialog)
{
  Gimp *gimp = g_object_get_data (G_OBJECT (dialog), "gimp");

  switch (response_id)
    {
    case RESPONSE_RESET:
      {
        GtkWidget *confirm;

        confirm = gimp_message_dialog_new (_("Reset All Preferences"),
                                           GIMP_ICON_DIALOG_QUESTION,
                                           dialog,
                                           GTK_DIALOG_MODAL |
                                           GTK_DIALOG_DESTROY_WITH_PARENT,
                                           gimp_standard_help_func, NULL,

                                           _("_Cancel"), GTK_RESPONSE_CANCEL,
                                           _("_Reset"),  GTK_RESPONSE_OK,

                                           NULL);

        gtk_dialog_set_alternative_button_order (GTK_DIALOG (confirm),
                                                 GTK_RESPONSE_OK,
                                                 GTK_RESPONSE_CANCEL,
                                                 -1);

        gimp_message_box_set_primary_text (GIMP_MESSAGE_DIALOG (confirm)->box,
                                           _("Do you really want to reset all "
                                             "preferences to default values?"));

        if (gimp_dialog_run (GIMP_DIALOG (confirm)) == GTK_RESPONSE_OK)
          {
            GimpConfig *config_copy;

            config_copy = g_object_get_data (G_OBJECT (dialog), "config-copy");

            gimp_config_reset (config_copy);
            gimp_rc_load_system (GIMP_RC (config_copy));

            /* don't use the default value if there is no help browser */
            if (! gimp_help_browser_is_installed (gimp))
              {
                g_object_set (config_copy,
                              "help-browser", GIMP_HELP_BROWSER_WEB_BROWSER,
                              NULL);
              }
          }

        gtk_widget_destroy (confirm);

        return;
      }
      break;

    case GTK_RESPONSE_OK:
      {
        GObject *config_copy;
        GList   *restart_diff;
        GList   *confirm_diff;
        GList   *list;

        config_copy = g_object_get_data (G_OBJECT (dialog), "config-copy");

        /*  destroy config_orig  */
        g_object_set_data (G_OBJECT (dialog), "config-orig", NULL);

        gtk_widget_set_sensitive (GTK_WIDGET (dialog), FALSE);

        confirm_diff = gimp_config_diff (G_OBJECT (gimp->edit_config),
                                         config_copy,
                                         GIMP_CONFIG_PARAM_CONFIRM);

        g_object_freeze_notify (G_OBJECT (gimp->edit_config));

        for (list = confirm_diff; list; list = g_list_next (list))
          {
            GParamSpec *param_spec = list->data;
            GValue      value      = G_VALUE_INIT;

            g_value_init (&value, param_spec->value_type);

            g_object_get_property (config_copy,
                                   param_spec->name, &value);
            g_object_set_property (G_OBJECT (gimp->edit_config),
                                   param_spec->name, &value);

            g_value_unset (&value);
          }

        g_object_thaw_notify (G_OBJECT (gimp->edit_config));

        g_list_free (confirm_diff);

        gimp_rc_save (GIMP_RC (gimp->edit_config));

        /*  spit out a solely informational warning about changed values
         *  which need restart
         */
        restart_diff = gimp_config_diff (G_OBJECT (gimp->edit_config),
                                         G_OBJECT (gimp->config),
                                         GIMP_CONFIG_PARAM_RESTART);

        if (restart_diff)
          {
            GString *string;

            string = g_string_new (_("You will have to restart GIMP for "
                                     "the following changes to take effect:"));
            g_string_append (string, "\n\n");

            for (list = restart_diff; list; list = g_list_next (list))
              {
                GParamSpec *param_spec = list->data;

                g_string_append_printf (string, "%s\n", param_spec->name);
              }

            prefs_message (GTK_MESSAGE_INFO, FALSE, string->str);

            g_string_free (string, TRUE);
          }

        g_list_free (restart_diff);
      }
      break;

    default:
      {
        GObject *config_orig;
        GList   *diff;
        GList   *list;

        config_orig = g_object_get_data (G_OBJECT (dialog), "config-orig");

        /*  destroy config_copy  */
        g_object_set_data (G_OBJECT (dialog), "config-copy", NULL);

        gtk_widget_set_sensitive (GTK_WIDGET (dialog), FALSE);

        diff = gimp_config_diff (G_OBJECT (gimp->edit_config),
                                 config_orig,
                                 GIMP_CONFIG_PARAM_SERIALIZE);

        g_object_freeze_notify (G_OBJECT (gimp->edit_config));

        for (list = diff; list; list = g_list_next (list))
          {
            GParamSpec *param_spec = list->data;
            GValue      value      = G_VALUE_INIT;

            g_value_init (&value, param_spec->value_type);

            g_object_get_property (config_orig,
                                   param_spec->name, &value);
            g_object_set_property (G_OBJECT (gimp->edit_config),
                                   param_spec->name, &value);

            g_value_unset (&value);
          }

        gimp_tool_editor_revert_changes (GIMP_TOOL_EDITOR (tool_editor));

        g_object_thaw_notify (G_OBJECT (gimp->edit_config));

        g_list_free (diff);
      }

      tool_editor = NULL;
    }

  /*  enable autosaving again  */
  gimp_rc_set_autosave (GIMP_RC (gimp->edit_config), TRUE);

  gtk_widget_destroy (dialog);
}

static void
prefs_color_management_reset (GtkWidget *widget,
                              GObject   *config)
{
  GimpCoreConfig *core_config = GIMP_CORE_CONFIG (config);

  gimp_config_reset (GIMP_CONFIG (core_config->color_management));
  gimp_config_reset_property (config, "color-profile-policy");
  gimp_config_reset_property (config, "filter-tool-show-color-options");
}

static void
prefs_dialog_defaults_reset (GtkWidget *widget,
                             GObject   *config)
{
  GParamSpec **pspecs;
  guint        n_pspecs;
  guint        i;

  pspecs = g_object_class_list_properties (G_OBJECT_GET_CLASS (config),
                                           &n_pspecs);

  g_object_freeze_notify (config);

  for (i = 0; i < n_pspecs; i++)
    {
      GParamSpec *pspec = pspecs[i];

      if (pspec->owner_type == GIMP_TYPE_DIALOG_CONFIG)
        gimp_config_reset_property (config, pspec->name);
    }

  gimp_config_reset_property (config, "filter-tool-max-recent");
  gimp_config_reset_property (config, "filter-tool-use-last-settings");
  gimp_config_reset_property (config, "filter-tool-show-color-options");

  g_object_thaw_notify (config);

  g_free (pspecs);
}

static void
prefs_template_select_callback (GimpContainerView *view,
                                GimpTemplate      *template,
                                gpointer           insert_data,
                                GimpTemplate      *edit_template)
{
  if (template)
    {
      /*  make sure the resolution values are copied first (see bug #546924)  */
      gimp_config_sync (G_OBJECT (template), G_OBJECT (edit_template),
                        GIMP_TEMPLATE_PARAM_COPY_FIRST);
      gimp_config_sync (G_OBJECT (template), G_OBJECT (edit_template),
                        0);
    }
}

static void
prefs_import_raw_procedure_callback (GtkWidget *widget,
                                     GObject   *config)
{
  gchar *raw_plug_in;

  raw_plug_in = gimp_plug_in_view_get_plug_in (GIMP_PLUG_IN_VIEW (widget));

  g_object_set (config,
                "import-raw-plug-in", raw_plug_in,
                NULL);

  g_free (raw_plug_in);
}

static void
prefs_resolution_source_callback (GtkWidget *widget,
                                  GObject   *config)
{
  gdouble  xres;
  gdouble  yres;
  gboolean from_gdk;

  from_gdk = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));

  if (from_gdk)
    {
      gimp_get_monitor_resolution (gtk_widget_get_screen (widget),
                                   gimp_widget_get_monitor (widget),
                                   &xres, &yres);
    }
  else
    {
      GimpSizeEntry *entry = g_object_get_data (G_OBJECT (widget),
                                                "monitor_resolution_sizeentry");

      g_return_if_fail (GIMP_IS_SIZE_ENTRY (entry));

      xres = gimp_size_entry_get_refval (entry, 0);
      yres = gimp_size_entry_get_refval (entry, 1);
    }

  g_object_set (config,
                "monitor-xresolution",                      xres,
                "monitor-yresolution",                      yres,
                "monitor-resolution-from-windowing-system", from_gdk,
                NULL);
}

static void
prefs_resolution_calibrate_callback (GtkWidget *widget,
                                     GtkWidget *entry)
{
  GtkWidget   *dialog;
  GtkWidget   *prefs_box;
  const gchar *icon_name;

  dialog = gtk_widget_get_toplevel (entry);

  prefs_box = g_object_get_data (G_OBJECT (dialog), "prefs-box");
  icon_name = gimp_prefs_box_get_current_icon_name (GIMP_PREFS_BOX (prefs_box));

  resolution_calibrate_dialog (entry, icon_name);
}

static void
prefs_input_devices_dialog (GtkWidget *widget,
                            Gimp      *gimp)
{
  gimp_dialog_factory_dialog_raise (gimp_dialog_factory_get_singleton (),
                                    gtk_widget_get_screen (widget),
                                    gimp_widget_get_monitor (widget),
                                    "gimp-input-devices-dialog", 0);
}

static void
prefs_keyboard_shortcuts_dialog (GtkWidget *widget,
                                 Gimp      *gimp)
{
  gimp_dialog_factory_dialog_raise (gimp_dialog_factory_get_singleton (),
                                    gtk_widget_get_screen (widget),
                                    gimp_widget_get_monitor (widget),
                                    "gimp-keyboard-shortcuts-dialog", 0);
}

static void
prefs_menus_save_callback (GtkWidget *widget,
                           Gimp      *gimp)
{
  GtkWidget *clear_button;

  menus_save (gimp, TRUE);

  clear_button = g_object_get_data (G_OBJECT (widget), "clear-button");

  if (clear_button)
    gtk_widget_set_sensitive (clear_button, TRUE);
}

static void
prefs_menus_clear_callback (GtkWidget *widget,
                            Gimp      *gimp)
{
  GError *error = NULL;

  if (! menus_clear (gimp, &error))
    {
      prefs_message (GTK_MESSAGE_ERROR, TRUE, error->message);
      g_clear_error (&error);
    }
  else
    {
      gtk_widget_set_sensitive (widget, FALSE);

      prefs_message (GTK_MESSAGE_INFO, TRUE,
                     _("Your keyboard shortcuts will be reset to "
                       "default values the next time you start GIMP."));
    }
}

static void
prefs_menus_remove_callback (GtkWidget *widget,
                             Gimp      *gimp)
{
  GtkWidget *dialog;

  dialog = gimp_message_dialog_new (_("Remove all Keyboard Shortcuts"),
                                    GIMP_ICON_DIALOG_QUESTION,
                                    gtk_widget_get_toplevel (widget),
                                    GTK_DIALOG_MODAL |
                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                    gimp_standard_help_func, NULL,

                                    _("_Cancel"), GTK_RESPONSE_CANCEL,
                                    _("Cl_ear"),  GTK_RESPONSE_OK,

                                    NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  g_signal_connect_object (gtk_widget_get_toplevel (widget), "unmap",
                           G_CALLBACK (gtk_widget_destroy),
                           dialog, G_CONNECT_SWAPPED);

  gimp_message_box_set_primary_text (GIMP_MESSAGE_DIALOG (dialog)->box,
                                     _("Do you really want to remove all "
                                       "keyboard shortcuts from all menus?"));

  if (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK)
    {
      menus_remove (gimp);
    }

  gtk_widget_destroy (dialog);
}

static void
prefs_session_save_callback (GtkWidget *widget,
                             Gimp      *gimp)
{
  GtkWidget *clear_button;

  session_save (gimp, TRUE);

  clear_button = g_object_get_data (G_OBJECT (widget), "clear-button");

  if (clear_button)
    gtk_widget_set_sensitive (clear_button, TRUE);
}

static void
prefs_session_clear_callback (GtkWidget *widget,
                              Gimp      *gimp)
{
  GError *error = NULL;

  if (! session_clear (gimp, &error))
    {
      prefs_message (GTK_MESSAGE_ERROR, TRUE, error->message);
      g_clear_error (&error);
    }
  else
    {
      gtk_widget_set_sensitive (widget, FALSE);

      prefs_message (GTK_MESSAGE_INFO, TRUE,
                     _("Your window setup will be reset to "
                       "default values the next time you start GIMP."));
    }
}

static void
prefs_devices_save_callback (GtkWidget *widget,
                             Gimp      *gimp)
{
  GtkWidget *clear_button;

  gimp_devices_save (gimp, TRUE);

  clear_button = g_object_get_data (G_OBJECT (widget), "clear-button");

  if (clear_button)
    gtk_widget_set_sensitive (clear_button, TRUE);
}

static void
prefs_devices_clear_callback (GtkWidget *widget,
                              Gimp      *gimp)
{
  GError *error = NULL;

  if (! gimp_devices_clear (gimp, &error))
    {
      prefs_message (GTK_MESSAGE_ERROR, TRUE, error->message);
      g_clear_error (&error);
    }
  else
    {
      gtk_widget_set_sensitive (widget, FALSE);

      prefs_message (GTK_MESSAGE_INFO, TRUE,
                     _("Your input device settings will be reset to "
                       "default values the next time you start GIMP."));
    }
}

static void
prefs_search_clear_callback (GtkWidget *widget,
                             Gimp      *gimp)
{
  gimp_action_history_clear (gimp);
}

static void
prefs_tool_options_save_callback (GtkWidget *widget,
                                  Gimp      *gimp)
{
  GtkWidget *clear_button;

  gimp_tools_save (gimp, TRUE, TRUE);

  clear_button = g_object_get_data (G_OBJECT (widget), "clear-button");

  if (clear_button)
    gtk_widget_set_sensitive (clear_button, TRUE);
}

static void
prefs_tool_options_clear_callback (GtkWidget *widget,
                                   Gimp      *gimp)
{
  GError *error = NULL;

  if (! gimp_tools_clear (gimp, &error))
    {
      prefs_message (GTK_MESSAGE_ERROR, TRUE, error->message);
      g_clear_error (&error);
    }
  else
    {
      gtk_widget_set_sensitive (widget, FALSE);

      prefs_message (GTK_MESSAGE_INFO, TRUE,
                     _("Your tool options will be reset to "
                       "default values the next time you start GIMP."));
    }
}

static void
prefs_help_language_change_callback (GtkComboBox *combo,
                                     Gimp        *gimp)
{
  gchar *help_locales = NULL;
  gchar *code;

  code = gimp_language_combo_box_get_code (GIMP_LANGUAGE_COMBO_BOX (combo));
  if (code && g_strcmp0 ("", code) != 0)
    {
      help_locales = g_strdup_printf ("%s:", code);
    }
  g_object_set (gimp->config,
                "help-locales", help_locales? help_locales : "",
                NULL);
  g_free (code);
  if (help_locales)
    g_free (help_locales);
}

static void
prefs_help_language_change_callback2 (GtkComboBox  *combo,
                                      GtkContainer *box)
{
  Gimp        *gimp;
  GtkLabel    *label = NULL;
  GtkImage    *icon  = NULL;
  GList       *children;
  GList       *iter;
  const gchar *text;
  const gchar *icon_name;

  gimp = g_object_get_data (G_OBJECT (box), "gimp");
  children = gtk_container_get_children (box);
  for (iter = children; iter; iter = iter->next)
    {
      if (GTK_IS_LABEL (iter->data))
        {
          label = iter->data;
        }
      else if (GTK_IS_IMAGE (iter->data))
        {
          icon = iter->data;
        }
    }
  if (gimp_help_user_manual_is_installed (gimp))
    {
      text = _("There's a local installation of the user manual.");
      icon_name = GIMP_ICON_DIALOG_INFORMATION;
    }
  else
    {
      text = _("The user manual is not installed locally.");
      icon_name = GIMP_ICON_DIALOG_WARNING;
    }
  if (label)
    {
      gtk_label_set_text (label, text);
    }
  if (icon)
    {
      gtk_image_set_from_icon_name (icon, icon_name,
                                    GTK_ICON_SIZE_BUTTON);
    }

  g_list_free (children);
}

static void
prefs_format_string_select_callback (GtkTreeSelection *sel,
                                     GtkEntry         *entry)
{
  GtkTreeModel *model;
  GtkTreeIter   iter;

  if (gtk_tree_selection_get_selected (sel, &model, &iter))
    {
      GValue val = G_VALUE_INIT;

      gtk_tree_model_get_value (model, &iter, 1, &val);
      gtk_entry_set_text (entry, g_value_get_string (&val));
      g_value_unset (&val);
    }
}

static void
prefs_theme_select_callback (GtkTreeSelection *sel,
                             Gimp             *gimp)
{
  GtkTreeModel *model;
  GtkTreeIter   iter;

  if (gtk_tree_selection_get_selected (sel, &model, &iter))
    {
      GValue val = G_VALUE_INIT;

      gtk_tree_model_get_value (model, &iter, 0, &val);
      g_object_set_property (G_OBJECT (gimp->config), "theme", &val);
      g_value_unset (&val);
    }
}

static void
prefs_theme_reload_callback (GtkWidget *button,
                             Gimp      *gimp)
{
  g_object_notify (G_OBJECT (gimp->config), "theme");
}

static void
prefs_icon_theme_select_callback (GtkTreeSelection *sel,
                                  Gimp             *gimp)
{
  GtkTreeModel *model;
  GtkTreeIter   iter;

  if (gtk_tree_selection_get_selected (sel, &model, &iter))
    {
      GValue val = G_VALUE_INIT;

      gtk_tree_model_get_value (model, &iter, 1, &val);
      g_object_set_property (G_OBJECT (gimp->config), "icon-theme", &val);
      g_value_unset (&val);
    }
}

static void
prefs_canvas_padding_color_changed (GtkWidget *button,
                                    GtkWidget *combo)
{
  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (combo),
                                 GIMP_CANVAS_PADDING_MODE_CUSTOM);
}

static void
prefs_display_options_frame_add (Gimp         *gimp,
                                 GObject      *object,
                                 const gchar  *label,
                                 GtkContainer *parent)
{
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *checks_vbox;
  GtkWidget *table;
  GtkWidget *combo;
  GtkWidget *button;

  vbox = prefs_frame_new (label, parent, FALSE);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  checks_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_box_pack_start (GTK_BOX (hbox), checks_vbox, TRUE, TRUE, 0);
  gtk_widget_show (checks_vbox);

#ifndef GDK_WINDOWING_QUARTZ
  prefs_check_button_add (object, "show-menubar",
                          _("Show _menubar"),
                          GTK_BOX (checks_vbox));
#endif /* !GDK_WINDOWING_QUARTZ */
  prefs_check_button_add (object, "show-rulers",
                          _("Show _rulers"),
                          GTK_BOX (checks_vbox));
  prefs_check_button_add (object, "show-scrollbars",
                          _("Show scroll_bars"),
                          GTK_BOX (checks_vbox));
  prefs_check_button_add (object, "show-statusbar",
                          _("Show s_tatusbar"),
                          GTK_BOX (checks_vbox));

  checks_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_box_pack_start (GTK_BOX (hbox), checks_vbox, TRUE, TRUE, 0);
  gtk_widget_show (checks_vbox);

  prefs_check_button_add (object, "show-selection",
                          _("Show s_election"),
                          GTK_BOX (checks_vbox));
  prefs_check_button_add (object, "show-layer-boundary",
                          _("Show _layer boundary"),
                          GTK_BOX (checks_vbox));
  prefs_check_button_add (object, "show-guides",
                          _("Show _guides"),
                          GTK_BOX (checks_vbox));
  prefs_check_button_add (object, "show-grid",
                          _("Show gri_d"),
                          GTK_BOX (checks_vbox));

  table = prefs_table_new (2, GTK_CONTAINER (vbox));

  combo = prefs_enum_combo_box_add (object, "padding-mode", 0, 0,
                                    _("Canvas _padding mode:"),
                                    GTK_TABLE (table), 0,
                                    NULL);

  button = prefs_color_button_add (object, "padding-color",
                                   _("Custom p_adding color:"),
                                   _("Select Custom Canvas Padding Color"),
                                   GTK_TABLE (table), 1, NULL,
                                   gimp_get_user_context (gimp));

  g_signal_connect (button, "color-changed",
                    G_CALLBACK (prefs_canvas_padding_color_changed),
                    combo);
}

static void
prefs_behavior_options_frame_add (Gimp         *gimp,
                                  GObject      *object,
                                  const gchar  *label,
                                  GtkContainer *parent)
{
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *checks_vbox;

  vbox = prefs_frame_new (label, parent, FALSE);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  checks_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_box_pack_start (GTK_BOX (hbox), checks_vbox, TRUE, TRUE, 0);
  gtk_widget_show (checks_vbox);

  prefs_check_button_add (object, "snap-to-guides",
                          _("Snap to Guides"),
                          GTK_BOX (checks_vbox));
  prefs_check_button_add (object, "snap-to-grid",
                          _("Snap to Grid"),
                          GTK_BOX (checks_vbox));

  checks_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_box_pack_start (GTK_BOX (hbox), checks_vbox, TRUE, TRUE, 0);
  gtk_widget_show (checks_vbox);

  prefs_check_button_add (object, "snap-to-canvas",
                          _("Snap to Canvas Edges"),
                          GTK_BOX (checks_vbox));
  prefs_check_button_add (object, "snap-to-path",
                          _("Snap to Active Path"),
                          GTK_BOX (checks_vbox));
}

static void
prefs_help_func (const gchar *help_id,
                 gpointer     help_data)
{
  GtkWidget *prefs_box;

  prefs_box = g_object_get_data (G_OBJECT (help_data), "prefs-box");

  help_id = gimp_prefs_box_get_current_help_id (GIMP_PREFS_BOX (prefs_box));

  gimp_standard_help_func (help_id, NULL);
}

static void
prefs_message (GtkMessageType  type,
               gboolean        destroy_with_parent,
               const gchar    *message)
{
  GtkWidget *dialog;

  dialog = gtk_message_dialog_new (GTK_WINDOW (prefs_dialog),
                                   destroy_with_parent ?
                                   GTK_DIALOG_DESTROY_WITH_PARENT : 0,
                                   type, GTK_BUTTONS_OK,
                                   "%s", message);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (gtk_widget_destroy),
                    NULL);
  gtk_widget_show (dialog);
}

static GtkWidget *
prefs_dialog_new (Gimp       *gimp,
                  GimpConfig *config)
{
  GtkWidget         *dialog;
  GtkTreeIter        top_iter;
  GtkTreeIter        child_iter;

  GtkWidget         *prefs_box;
  GtkSizeGroup      *size_group = NULL;
  GtkWidget         *vbox;
  GtkWidget         *hbox;
  GtkWidget         *vbox2;
  GtkWidget         *vbox3;
  GtkWidget         *button;
  GtkWidget         *button2;
  GtkWidget         *table;
  GtkWidget         *label;
  GtkWidget         *entry;
  GtkWidget         *calibrate_button;
  GSList            *group;
  GtkWidget         *editor;
  gint               i;

  GObject           *object;
  GimpCoreConfig    *core_config;
  GimpDisplayConfig *display_config;
  GList             *manuals;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (GIMP_IS_CONFIG (config), NULL);

  object         = G_OBJECT (config);
  core_config    = GIMP_CORE_CONFIG (config);
  display_config = GIMP_DISPLAY_CONFIG (config);

  dialog = gimp_dialog_new (_("Preferences"), "gimp-preferences",
                            NULL, 0,
                            prefs_help_func,
                            GIMP_HELP_PREFS_DIALOG,

                            _("_Reset"),  RESPONSE_RESET,
                            _("_Cancel"), GTK_RESPONSE_CANCEL,
                            _("_OK"),     GTK_RESPONSE_OK,

                            NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           RESPONSE_RESET,
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (prefs_response),
                    dialog);

  /* The prefs box */
  prefs_box = gimp_prefs_box_new ();
  gtk_container_set_border_width (GTK_CONTAINER (prefs_box), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      prefs_box, TRUE, TRUE, 0);
  gtk_widget_show (prefs_box);

  g_object_set_data (G_OBJECT (dialog), "prefs-box", prefs_box);


  /**********************/
  /*  System Resources  */
  /**********************/
  vbox = gimp_prefs_box_add_page (GIMP_PREFS_BOX (prefs_box),
                                  "gimp-prefs-system-resources",
                                  _("System Resources"),
                                  _("System Resources"),
                                  GIMP_HELP_PREFS_SYSTEM_RESOURCES,
                                  NULL,
                                  &top_iter);

  size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  vbox2 = prefs_frame_new (_("Resource Consumption"),
                           GTK_CONTAINER (vbox), FALSE);

#ifdef ENABLE_MP
  table = prefs_table_new (5, GTK_CONTAINER (vbox2));
#else
  table = prefs_table_new (4, GTK_CONTAINER (vbox2));
#endif /* ENABLE_MP */

  prefs_spin_button_add (object, "undo-levels", 1.0, 5.0, 0,
                         _("Minimal number of _undo levels:"),
                         GTK_TABLE (table), 0, size_group);
  prefs_memsize_entry_add (object, "undo-size",
                           _("Maximum undo _memory:"),
                           GTK_TABLE (table), 1, size_group);
  prefs_memsize_entry_add (object, "tile-cache-size",
                           _("Tile cache _size:"),
                           GTK_TABLE (table), 2, size_group);
  prefs_memsize_entry_add (object, "max-new-image-size",
                           _("Maximum _new image size:"),
                           GTK_TABLE (table), 3, size_group);

#ifdef ENABLE_MP
  prefs_spin_button_add (object, "num-processors", 1.0, 4.0, 0,
                         _("Number of _threads to use:"),
                         GTK_TABLE (table), 4, size_group);
#endif /* ENABLE_MP */

  /*  Hardware Acceleration  */
  vbox2 = prefs_frame_new (_("Hardware Acceleration"), GTK_CONTAINER (vbox),
                           FALSE);

  prefs_check_button_add (object, "use-opencl",
                          _("Use OpenCL"),
                          GTK_BOX (vbox2));

  /*  Image Thumbnails  */
  vbox2 = prefs_frame_new (_("Image Thumbnails"), GTK_CONTAINER (vbox), FALSE);

  table = prefs_table_new (2, GTK_CONTAINER (vbox2));

  prefs_enum_combo_box_add (object, "thumbnail-size", 0, 0,
                            _("Size of _thumbnails:"),
                            GTK_TABLE (table), 0, size_group);

  prefs_memsize_entry_add (object, "thumbnail-filesize-limit",
                           _("Maximum _filesize for thumbnailing:"),
                           GTK_TABLE (table), 1, size_group);

  g_object_unref (size_group);
  size_group = NULL;

  /*  Document History  */
  vbox2 = prefs_frame_new (_("Document History"), GTK_CONTAINER (vbox), FALSE);

  prefs_check_button_add (object, "save-document-history",
                          _("Keep record of used files in the Recent Documents list"),
                          GTK_BOX (vbox2));


  /***************/
  /*  Debugging  */
  /***************/
  /* No debugging preferences are needed on win32. Either GIMP has been
   * built with DrMinGW support (HAVE_EXCHNDL) or not. If it has, then
   * the backtracing is enabled and can't be disabled. It assume it will
   * work only upon a crash.
   */
#ifndef G_OS_WIN32
  vbox = gimp_prefs_box_add_page (GIMP_PREFS_BOX (prefs_box),
                                  "gimp-wilber-eek", /* TODO: icon needed. */
                                  _("Debugging"),
                                  _("Debugging"),
                                  GIMP_HELP_PREFS_DEBUGGING,
                                  NULL,
                                  &top_iter);

  hbox = g_object_new (GIMP_TYPE_HINT_BOX,
                       "icon-name", GIMP_ICON_DIALOG_WARNING,
                       "hint",      _("We hope you will never need these "
                                      "settings, but as all software, GIMP "
                                      "has bugs, and crashes can occur. If it "
                                      "happens, you can help us by reporting "
                                      "bugs."),
                        NULL);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  vbox2 = prefs_frame_new (_("Bug Reporting"),
                           GTK_CONTAINER (vbox), FALSE);

  size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  table = prefs_table_new (1, GTK_CONTAINER (vbox2));

  button = prefs_enum_combo_box_add (object, "debug-policy", 0, 0,
                                     _("Debug _policy:"),
                                     GTK_TABLE (table), 0, size_group);

  /* Check existence of gdb or lldb to activate the preference, as a
   * good hint of its prerequisite, unless backtrace() API exists, in
   * which case the feature is always available.
   */
  hbox = NULL;
  if (! gimp_stack_trace_available (TRUE))
    {
#ifndef HAVE_EXECINFO_H
      hbox = prefs_hint_box_new (GIMP_ICON_DIALOG_WARNING,
                                 _("This feature requires \"gdb\" or \"lldb\" installed on your system."));
      gtk_widget_set_sensitive (button, FALSE);
#else
      hbox = prefs_hint_box_new (GIMP_ICON_DIALOG_WARNING,
                                 _("This feature is more efficient with \"gdb\" or \"lldb\" installed on your system."));
#endif /* ! HAVE_EXECINFO_H */
    }
  if (hbox)
    gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);

#endif /* ! G_OS_WIN32 */

  /**********************/
  /*  Color Management  */
  /**********************/
  vbox = gimp_prefs_box_add_page (GIMP_PREFS_BOX (prefs_box),
                                  "gimp-prefs-color-management",
                                  _("Color Management"),
                                  _("Color Management"),
                                  GIMP_HELP_PREFS_COLOR_MANAGEMENT,
                                  NULL,
                                  &top_iter);

  gimp_prefs_box_set_page_scrollable (GIMP_PREFS_BOX (prefs_box), vbox, TRUE);

  button = gimp_prefs_box_set_page_resettable (GIMP_PREFS_BOX (prefs_box),
                                               vbox,
                                               _("Reset Color Management"));
  g_signal_connect (button, "clicked",
                    G_CALLBACK (prefs_color_management_reset),
                    config);

  {
    GObject      *color_config = G_OBJECT (core_config->color_management);
    GtkListStore *store;
    gchar        *filename;
    gint          row = 0;

    filename = gimp_personal_rc_file ("profilerc");
    store = gimp_color_profile_store_new (filename);
    g_free (filename);

    gimp_color_profile_store_add_file (GIMP_COLOR_PROFILE_STORE (store),
                                       NULL, NULL);

    size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

    table = prefs_table_new (1, GTK_CONTAINER (vbox));

    prefs_enum_combo_box_add (color_config, "mode", 0, 0,
                              _("Image display _mode:"),
                              GTK_TABLE (table), row++, NULL);

    /*  Color Managed Display  */
    vbox2 = prefs_frame_new (_("Color Managed Display"), GTK_CONTAINER (vbox),
                             FALSE);

    table = prefs_table_new (4, GTK_CONTAINER (vbox2));
    row = 0;

    prefs_profile_combo_box_add (color_config,
                                 "display-profile",
                                 store,
                                 _("Select Monitor Color Profile"),
                                 _("_Monitor profile:"),
                                 GTK_TABLE (table), row++, size_group,
                                 object, "color-profile-path");

    button = gimp_prop_check_button_new (color_config,
                                         "display-profile-from-gdk",
                                         _("_Try to use the system monitor "
                                           "profile"));
    gtk_table_attach_defaults (GTK_TABLE (table),
                               button, 1, 2, row, row + 1);
    gtk_widget_show (button);
    row++;

    prefs_enum_combo_box_add (color_config,
                              "display-rendering-intent", 0, 0,
                              _("_Rendering intent:"),
                              GTK_TABLE (table), row++, size_group);

    button = gimp_prop_check_button_new (color_config,
                                         "display-use-black-point-compensation",
                                         _("Use _black point compensation"));
    gtk_table_attach_defaults (GTK_TABLE (table),
                               button, 1, 2, row, row + 1);
    gtk_widget_show (button);
    row++;

    prefs_boolean_combo_box_add (color_config,
                                 "display-optimize",
                                 _("Speed"),
                                 _("Precision / Color Fidelity"),
                                 _("_Optimize image display for:"),
                                 GTK_TABLE (table), row++, size_group);

    /*  Print Simulation (Soft-proofing)  */
    vbox2 = prefs_frame_new (_("Soft-Proofing"),
                             GTK_CONTAINER (vbox),
                             FALSE);

    table = prefs_table_new (4, GTK_CONTAINER (vbox2));
    row = 0;

    prefs_profile_combo_box_add (color_config,
                                 "printer-profile",
                                 store,
                                 _("Select Soft-Proofing Color Profile"),
                                 _("_Soft-proofing profile:"),
                                 GTK_TABLE (table), row++, size_group,
                                 object, "color-profile-path");

    prefs_enum_combo_box_add (color_config,
                              "simulation-rendering-intent", 0, 0,
                              _("Re_ndering intent:"),
                              GTK_TABLE (table), row++, size_group);

    button = gimp_prop_check_button_new (color_config,
                                         "simulation-use-black-point-compensation",
                                         _("Use black _point compensation"));
    gtk_table_attach_defaults (GTK_TABLE (table),
                               button, 1, 2, row, row + 1);
    gtk_widget_show (button);
    row++;

    prefs_boolean_combo_box_add (color_config,
                                 "simulation-optimize",
                                 _("Speed"),
                                 _("Precision / Color Fidelity"),
                                 _("O_ptimize soft-proofing for:"),
                                 GTK_TABLE (table), row++, size_group);

    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_table_attach_defaults (GTK_TABLE (table), hbox, 1, 2, row, row + 1);
    gtk_widget_show (hbox);
    row++;

    button = gimp_prop_check_button_new (color_config, "simulation-gamut-check",
                                         _("Mark out of gamut colors"));
    gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
    gtk_widget_show (button);

    button = gimp_prop_color_button_new (color_config, "out-of-gamut-color",
                                         _("Select Warning Color"),
                                         PREFS_COLOR_BUTTON_WIDTH,
                                         PREFS_COLOR_BUTTON_HEIGHT,
                                         GIMP_COLOR_AREA_FLAT);
    gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
    gtk_widget_show (button);

    gimp_color_panel_set_context (GIMP_COLOR_PANEL (button),
                                  gimp_get_user_context (gimp));

    /*  Preferred profiles  */
    vbox2 = prefs_frame_new (_("Preferred Profiles"), GTK_CONTAINER (vbox),
                             FALSE);

    table = prefs_table_new (3, GTK_CONTAINER (vbox2));
    row = 0;

    prefs_profile_combo_box_add (color_config,
                                 "rgb-profile",
                                 store,
                                 _("Select Preferred RGB Color Profile"),
                                 _("_RGB profile:"),
                                 GTK_TABLE (table), row++, size_group,
                                 object, "color-profile-path");

    prefs_profile_combo_box_add (color_config,
                                 "gray-profile",
                                 store,
                                 _("Select Preferred Grayscale Color Profile"),
                                 _("_Grayscale profile:"),
                                 GTK_TABLE (table), row++, size_group,
                                 object, "color-profile-path");

    prefs_profile_combo_box_add (color_config,
                                 "cmyk-profile",
                                 store,
                                 _("Select CMYK Color Profile"),
                                 _("_CMYK profile:"),
                                 GTK_TABLE (table), row++, size_group,
                                 object, "color-profile-path");

    /*  Policies  */
    vbox2 = prefs_frame_new (_("Policies"), GTK_CONTAINER (vbox),
                             FALSE);
    table = prefs_table_new (1, GTK_CONTAINER (vbox2));

    button = prefs_enum_combo_box_add (object, "color-profile-policy", 0, 0,
                                       _("File Open behaviour:"),
                                       GTK_TABLE (table), 0, size_group);

    /*  Filter Dialogs  */
    vbox2 = prefs_frame_new (_("Filter Dialogs"), GTK_CONTAINER (vbox),
                             FALSE);

    button = prefs_check_button_add (object, "filter-tool-show-color-options",
                                     _("Show advanced color options"),
                                     GTK_BOX (vbox2));

    g_object_unref (size_group);

    g_object_unref (store);
  }


  /***************************/
  /*  Image Import / Export  */
  /***************************/
  vbox = gimp_prefs_box_add_page (GIMP_PREFS_BOX (prefs_box),
                                  "gimp-prefs-import-export",
                                  _("Image Import & Export"),
                                  _("Image Import & Export"),
                                  GIMP_HELP_PREFS_IMPORT_EXPORT,
                                  NULL,
                                  &top_iter);

  /*  Import Policies  */
  vbox2 = prefs_frame_new (_("Import Policies"),
                           GTK_CONTAINER (vbox), FALSE);

  button = prefs_check_button_add (object, "import-promote-float",
                                   _("Promote imported images to "
                                     "_floating point precision"),
                                   GTK_BOX (vbox2));

  vbox3 = prefs_frame_new (NULL, GTK_CONTAINER (vbox2), FALSE);
  button = prefs_check_button_add (object, "import-promote-dither",
                                   _("Dither images when promoting to "
                                     "floating point"),
                                   GTK_BOX (vbox3));

  button = prefs_check_button_add (object, "import-add-alpha",
                                   _("Add an alpha channel to imported images"),
                                   GTK_BOX (vbox2));

  table = prefs_table_new (1, GTK_CONTAINER (vbox2));
  button = prefs_enum_combo_box_add (object, "color-profile-policy", 0, 0,
                                     _("Color profile policy:"),
                                     GTK_TABLE (table), 0, NULL);

  /*  Export Policies  */
  vbox2 = prefs_frame_new (_("Export Policies"),
                           GTK_CONTAINER (vbox), FALSE);

  button = prefs_check_button_add (object, "export-metadata-exif",
                                   _("Export Exif metadata by default when available"),
                                   GTK_BOX (vbox2));
  button = prefs_check_button_add (object, "export-metadata-xmp",
                                   _("Export XMP metadata by default when available"),
                                   GTK_BOX (vbox2));
  button = prefs_check_button_add (object, "export-metadata-iptc",
                                   _("Export IPTC metadata by default when available"),
                                   GTK_BOX (vbox2));
  hbox = prefs_hint_box_new (GIMP_ICON_DIALOG_WARNING,
                             _("Metadata can contain sensitive information."));
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);


  /*  Raw Image Importer  */
  vbox2 = prefs_frame_new (_("Raw Image Importer"),
                           GTK_CONTAINER (vbox), TRUE);

  {
    GtkWidget *scrolled_window;
    GtkWidget *view;

    scrolled_window = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
                                         GTK_SHADOW_IN);
    gtk_box_pack_start (GTK_BOX (vbox2), scrolled_window, TRUE, TRUE, 0);
    gtk_widget_show (scrolled_window);

    view = gimp_plug_in_view_new (gimp->plug_in_manager->raw_load_procs);
    gimp_plug_in_view_set_plug_in (GIMP_PLUG_IN_VIEW (view),
                                   core_config->import_raw_plug_in);
    gtk_container_add (GTK_CONTAINER (scrolled_window), view);
    gtk_widget_show (view);

    g_signal_connect (view, "changed",
                      G_CALLBACK (prefs_import_raw_procedure_callback),
                      config);
  }

  /****************/
  /*  Playground  */
  /****************/
  if (gimp->show_playground)
    {
      vbox = gimp_prefs_box_add_page (GIMP_PREFS_BOX (prefs_box),
                                      "gimp-prefs-playground",
                                      _("Experimental Playground"),
                                      _("Playground"),
                                      GIMP_HELP_PREFS_PLAYGROUND,
                                      NULL,
                                      &top_iter);

      hbox = g_object_new (GIMP_TYPE_HINT_BOX,
                           "icon-name", GIMP_ICON_DIALOG_WARNING,
                           "hint",      _("These features are unfinished, buggy "
                                          "and may crash GIMP. It is unadvised to "
                                          "use them unless you really know what "
                                          "you are doing or you intend to contribute "
                                          "patches."),
                           NULL);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
      gtk_widget_show (hbox);

      vbox2 = prefs_frame_new (_("Insane Options"),
                               GTK_CONTAINER (vbox), TRUE);

      button = prefs_check_button_add (object, "playground-npd-tool",
                                       _("_N-Point Deformation tool"),
                                       GTK_BOX (vbox2));
      button = prefs_check_button_add (object, "playground-seamless-clone-tool",
                                       _("_Seamless Clone tool"),
                                       GTK_BOX (vbox2));
    }


  /******************/
  /*  Tool Options  */
  /******************/
  vbox = gimp_prefs_box_add_page (GIMP_PREFS_BOX (prefs_box),
                                  "gimp-prefs-tool-options",
                                  C_("preferences", "Tool Options"),
                                  C_("preferences", "Tool Options"),
                                  GIMP_HELP_PREFS_TOOL_OPTIONS,
                                  NULL,
                                  &top_iter);

  size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  /*  General  */
  vbox2 = prefs_frame_new (_("General"), GTK_CONTAINER (vbox), FALSE);

  prefs_check_button_add (object, "save-tool-options",
                          _("_Save tool options on exit"),
                          GTK_BOX (vbox2));

  button = prefs_button_add (GIMP_ICON_DOCUMENT_SAVE,
                             _("Save Tool Options _Now"),
                             GTK_BOX (vbox2));
  g_signal_connect (button, "clicked",
                    G_CALLBACK (prefs_tool_options_save_callback),
                    gimp);

  button2 = prefs_button_add (GIMP_ICON_RESET,
                              _("_Reset Saved Tool Options to "
                                "Default Values"),
                              GTK_BOX (vbox2));
  g_signal_connect (button2, "clicked",
                    G_CALLBACK (prefs_tool_options_clear_callback),
                    gimp);

  g_object_set_data (G_OBJECT (button), "clear-button", button2);

  /*  Scaling  */
  vbox2 = prefs_frame_new (_("Scaling"), GTK_CONTAINER (vbox), FALSE);
  table = prefs_table_new (1, GTK_CONTAINER (vbox2));

  prefs_enum_combo_box_add (object, "interpolation-type", 0, 0,
                            _("Default _interpolation:"),
                            GTK_TABLE (table), 0, size_group);

  g_object_unref (size_group);
  size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  /*  Global Brush, Pattern, ...  */
  vbox2 = prefs_frame_new (_("Paint Options Shared Between Tools"),
                           GTK_CONTAINER (vbox), FALSE);

  prefs_check_button_add_with_icon (object, "global-brush",
                                    _("_Brush"),    GIMP_ICON_BRUSH,
                                    GTK_BOX (vbox2), size_group);
  prefs_check_button_add_with_icon (object, "global-dynamics",
                                    _("_Dynamics"), GIMP_ICON_DYNAMICS,
                                    GTK_BOX (vbox2), size_group);
  prefs_check_button_add_with_icon (object, "global-pattern",
                                    _("_Pattern"),  GIMP_ICON_PATTERN,
                                    GTK_BOX (vbox2), size_group);
  prefs_check_button_add_with_icon (object, "global-gradient",
                                    _("_Gradient"), GIMP_ICON_GRADIENT,
                                    GTK_BOX (vbox2), size_group);

  /*  Move Tool */
  vbox2 = prefs_frame_new (_("Move Tool"),
                           GTK_CONTAINER (vbox), FALSE);

  prefs_check_button_add_with_icon (object, "move-tool-changes-active",
                                    _("Set layer or path as active"),
                                    GIMP_ICON_TOOL_MOVE,
                                    GTK_BOX (vbox2), size_group);

  g_object_unref (size_group);
  size_group = NULL;


  /*******************/
  /*  Default Image  */
  /*******************/
  vbox = gimp_prefs_box_add_page (GIMP_PREFS_BOX (prefs_box),
                                  "gimp-prefs-new-image",
                                  _("Default New Image"),
                                  _("Default Image"),
                                  GIMP_HELP_PREFS_NEW_IMAGE,
                                  NULL,
                                  &top_iter);

  gimp_prefs_box_set_page_scrollable (GIMP_PREFS_BOX (prefs_box), vbox, TRUE);

  table = prefs_table_new (1, GTK_CONTAINER (vbox));

  {
    GtkWidget *combo;

    combo = gimp_container_combo_box_new (gimp->templates,
                                          gimp_get_user_context (gimp),
                                          16, 0);
    gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                               _("_Template:"),  0.0, 0.5,
                               combo, 1, FALSE);

    gimp_container_view_select_item (GIMP_CONTAINER_VIEW (combo), NULL);

    g_signal_connect (combo, "select-item",
                      G_CALLBACK (prefs_template_select_callback),
                      core_config->default_image);
  }

  editor = gimp_template_editor_new (core_config->default_image, gimp, FALSE);
  gimp_template_editor_show_advanced (GIMP_TEMPLATE_EDITOR (editor), TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), editor, FALSE, FALSE, 0);
  gtk_widget_show (editor);

  /*  Quick Mask Color */
  vbox2 = prefs_frame_new (_("Quick Mask"), GTK_CONTAINER (vbox), FALSE);
  table = prefs_table_new (1, GTK_CONTAINER (vbox2));

  prefs_color_button_add (object, "quick-mask-color",
                          _("Quick Mask color:"),
                          _("Set the default Quick Mask color"),
                          GTK_TABLE (table), 0, NULL,
                          gimp_get_user_context (gimp));


  /**********************************/
  /*  Default Image / Default Grid  */
  /**********************************/
  vbox = gimp_prefs_box_add_page (GIMP_PREFS_BOX (prefs_box),
                                  "gimp-prefs-default-grid",
                                  _("Default Image Grid"),
                                  _("Default Grid"),
                                  GIMP_HELP_PREFS_DEFAULT_GRID,
                                  &top_iter,
                                  &child_iter);

  /*  Grid  */
  editor = gimp_grid_editor_new (core_config->default_grid,
                                 gimp_get_user_context (gimp),
                                 gimp_template_get_resolution_x (core_config->default_image),
                                 gimp_template_get_resolution_y (core_config->default_image));
  gtk_box_pack_start (GTK_BOX (vbox), editor, TRUE, TRUE, 0);
  gtk_widget_show (editor);


  /***************/
  /*  Interface  */
  /***************/
  vbox = gimp_prefs_box_add_page (GIMP_PREFS_BOX (prefs_box),
                                  "gimp-prefs-interface",
                                  _("User Interface"),
                                  _("Interface"),
                                  GIMP_HELP_PREFS_INTERFACE,
                                  NULL,
                                  &top_iter);

  /*  Language  */

  /*  Only add the language entry if the iso-codes package is available.  */
#ifdef HAVE_ISO_CODES
  vbox2 = prefs_frame_new (_("Language"), GTK_CONTAINER (vbox), FALSE);

  prefs_language_combo_box_add (object, "language", GTK_BOX (vbox2));
#endif

  /*  Previews  */
  vbox2 = prefs_frame_new (_("Previews"), GTK_CONTAINER (vbox), FALSE);

  prefs_check_button_add (object, "layer-previews",
                          _("_Enable layer & channel previews"),
                          GTK_BOX (vbox2));

  table = prefs_table_new (3, GTK_CONTAINER (vbox2));

  prefs_enum_combo_box_add (object, "layer-preview-size", 0, 0,
                            _("_Default layer & channel preview size:"),
                            GTK_TABLE (table), 0, size_group);
  prefs_enum_combo_box_add (object, "undo-preview-size", 0, 0,
                            _("_Undo preview size:"),
                            GTK_TABLE (table), 1, size_group);
  prefs_enum_combo_box_add (object, "navigation-preview-size", 0, 0,
                            _("Na_vigation preview size:"),
                            GTK_TABLE (table), 2, size_group);

  /* Keyboard Shortcuts */
  vbox2 = prefs_frame_new (_("Keyboard Shortcuts"),
                           GTK_CONTAINER (vbox), FALSE);

  prefs_check_button_add (object, "can-change-accels",
                          _("_Use dynamic keyboard shortcuts"),
                          GTK_BOX (vbox2));

  button = prefs_button_add (GIMP_ICON_PREFERENCES_SYSTEM,
                             _("Configure _Keyboard Shortcuts..."),
                             GTK_BOX (vbox2));
  g_signal_connect (button, "clicked",
                    G_CALLBACK (prefs_keyboard_shortcuts_dialog),
                    gimp);

  prefs_check_button_add (object, "save-accels",
                          _("_Save keyboard shortcuts on exit"),
                          GTK_BOX (vbox2));

  button = prefs_button_add (GIMP_ICON_DOCUMENT_SAVE,
                             _("Save Keyboard Shortcuts _Now"),
                             GTK_BOX (vbox2));
  g_signal_connect (button, "clicked",
                    G_CALLBACK (prefs_menus_save_callback),
                    gimp);

  button2 = prefs_button_add (GIMP_ICON_RESET,
                              _("_Reset Keyboard Shortcuts to Default Values"),
                              GTK_BOX (vbox2));
  g_signal_connect (button2, "clicked",
                    G_CALLBACK (prefs_menus_clear_callback),
                    gimp);

  g_object_set_data (G_OBJECT (button), "clear-button", button2);

  button = prefs_button_add (GIMP_ICON_EDIT_CLEAR,
                             _("Remove _All Keyboard Shortcuts"),
                             GTK_BOX (vbox2));
  g_signal_connect (button, "clicked",
                    G_CALLBACK (prefs_menus_remove_callback),
                    gimp);


  /***********************/
  /*  Interface / Theme  */
  /***********************/
  vbox = gimp_prefs_box_add_page (GIMP_PREFS_BOX (prefs_box),
                                  "gimp-prefs-theme",
                                  _("Theme"),
                                  _("Theme"),
                                  GIMP_HELP_PREFS_THEME,
                                  &top_iter,
                                  &child_iter);

  vbox2 = prefs_frame_new (_("Select Theme"), GTK_CONTAINER (vbox), TRUE);

  {
    GtkWidget         *scrolled_win;
    GtkListStore      *list_store;
    GtkWidget         *view;
    GtkTreeSelection  *sel;
    gchar            **themes;
    gint               n_themes;
    gint               i;

    scrolled_win = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_set_size_request (scrolled_win, -1, 80);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_win),
                                         GTK_SHADOW_IN);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
                                    GTK_POLICY_NEVER,
                                    GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start (GTK_BOX (vbox2), scrolled_win, TRUE, TRUE, 0);
    gtk_widget_show (scrolled_win);

    list_store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);

    view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (list_store));
    gtk_container_add (GTK_CONTAINER (scrolled_win), view);
    gtk_widget_show (view);

    g_object_unref (list_store);

    gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (view), 0,
                                                 _("Theme"),
                                                 gtk_cell_renderer_text_new (),
                                                 "text", 0,
                                                 NULL);
    gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (view), 1,
                                                 _("Folder"),
                                                 gtk_cell_renderer_text_new (),
                                                 "text", 1,
                                                 NULL);

    sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));

    themes = themes_list_themes (gimp, &n_themes);

    for (i = 0; i < n_themes; i++)
      {
        GtkTreeIter  iter;
        GFile       *theme_dir = themes_get_theme_dir (gimp, themes[i]);

        gtk_list_store_append (list_store, &iter);
        gtk_list_store_set (list_store, &iter,
                            0, themes[i],
                            1, gimp_file_get_utf8_name (theme_dir),
                            -1);

        if (GIMP_GUI_CONFIG (object)->theme &&
            ! strcmp (GIMP_GUI_CONFIG (object)->theme, themes[i]))
          {
            GtkTreePath *path;

            path = gtk_tree_model_get_path (GTK_TREE_MODEL (list_store), &iter);

            gtk_tree_view_set_cursor (GTK_TREE_VIEW (view), path, NULL, FALSE);
            gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (view), path,
                                          NULL, FALSE, 0.0, 0.0);

            gtk_tree_path_free (path);
          }
      }

    g_strfreev (themes);

    g_signal_connect (sel, "changed",
                      G_CALLBACK (prefs_theme_select_callback),
                      gimp);
  }

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  button = prefs_button_add (GIMP_ICON_VIEW_REFRESH,
                             _("Reload C_urrent Theme"),
                             GTK_BOX (hbox));
  g_signal_connect (button, "clicked",
                    G_CALLBACK (prefs_theme_reload_callback),
                    gimp);


  /****************************/
  /*  Interface / Icon Theme  */
  /****************************/
  vbox = gimp_prefs_box_add_page (GIMP_PREFS_BOX (prefs_box),
                                  "gimp-prefs-icon-theme",
                                  _("Icon Theme"),
                                  _("Icon Theme"),
                                  GIMP_HELP_PREFS_ICON_THEME,
                                  &top_iter,
                                  &child_iter);

  vbox2 = prefs_frame_new (_("Select an Icon Theme"), GTK_CONTAINER (vbox), TRUE);

  {
    GtkWidget         *scrolled_win;
    GtkWidget         *icon_size_scale;
    GtkListStore      *list_store;
    GtkWidget         *view;
    GtkTreeSelection  *sel;
    gchar            **icon_themes;
    gint               n_icon_themes;
    gint               i;

    scrolled_win = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_set_size_request (scrolled_win, -1, 80);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_win),
                                         GTK_SHADOW_IN);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
                                    GTK_POLICY_NEVER,
                                    GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start (GTK_BOX (vbox2), scrolled_win, TRUE, TRUE, 0);
    gtk_widget_show (scrolled_win);

    list_store = gtk_list_store_new (3, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING);

    view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (list_store));
    gtk_container_add (GTK_CONTAINER (scrolled_win), view);
    gtk_widget_show (view);

    g_object_unref (list_store);

    gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (view), 0,
                                                 NULL,
                                                 gtk_cell_renderer_pixbuf_new (),
                                                 "pixbuf", 0,
                                                 NULL);
    gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (view), 1,
                                                 _("Icon Theme"),
                                                 gtk_cell_renderer_text_new (),
                                                 "text", 1,
                                                 NULL);
    gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (view), 2,
                                                 _("Folder"),
                                                 gtk_cell_renderer_text_new (),
                                                 "text", 2,
                                                 NULL);

    sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));

    icon_themes = icon_themes_list_themes (gimp, &n_icon_themes);

    for (i = 0; i < n_icon_themes; i++)
      {
        GtkTreeIter   iter;
        GFile        *icon_theme_dir = icon_themes_get_theme_dir (gimp, icon_themes[i]);
        GFile        *icon_theme_search_path = g_file_get_parent (icon_theme_dir);
        GtkIconTheme *theme;
        gchar        *example;
        GdkPixbuf    *pixbuf;

        theme = gtk_icon_theme_new ();
        gtk_icon_theme_prepend_search_path (theme, gimp_file_get_utf8_name(icon_theme_search_path));
        g_object_unref (icon_theme_search_path);
        gtk_icon_theme_set_custom_theme (theme, icon_themes[i]);

        example = gtk_icon_theme_get_example_icon_name (theme);
        if (! example)
          {
            /* If the icon theme didn't explicitly specify an example
             * icon, try "gimp-wilber".
             */
            example = g_strdup ("gimp-wilber");
          }
        pixbuf = gtk_icon_theme_load_icon (theme, example, 16, 0, NULL);

        gtk_list_store_append (list_store, &iter);
        gtk_list_store_set (list_store, &iter,
                            0, pixbuf,
                            1, icon_themes[i],
                            2, gimp_file_get_utf8_name (icon_theme_dir),
                            -1);
        g_object_unref (theme);
        g_object_unref (pixbuf);
        g_free (example);

        if (GIMP_GUI_CONFIG (object)->icon_theme &&
            ! strcmp (GIMP_GUI_CONFIG (object)->icon_theme, icon_themes[i]))
          {
            GtkTreePath *path;

            path = gtk_tree_model_get_path (GTK_TREE_MODEL (list_store), &iter);

            gtk_tree_view_set_cursor (GTK_TREE_VIEW (view), path, NULL, FALSE);
            gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (view), path,
                                          NULL, FALSE, 0.0, 0.0);

            gtk_tree_path_free (path);
          }
      }

    g_strfreev (icon_themes);

    g_signal_connect (sel, "changed",
                      G_CALLBACK (prefs_icon_theme_select_callback),
                      gimp);

    icon_size_scale = gimp_icon_size_scale_new (gimp);
    gtk_box_pack_start (GTK_BOX (vbox), icon_size_scale, FALSE, FALSE, 0);
    gtk_widget_show (icon_size_scale);
  }

  /*************************/
  /*  Interface / Toolbox  */
  /*************************/
  vbox = gimp_prefs_box_add_page (GIMP_PREFS_BOX (prefs_box),
                                  "gimp-prefs-toolbox",
                                  _("Toolbox"),
                                  _("Toolbox"),
                                  GIMP_HELP_PREFS_TOOLBOX,
                                  &top_iter,
                                  &child_iter);

  size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  /*  Appearance  */
  vbox2 = prefs_frame_new (_("Appearance"),
                           GTK_CONTAINER (vbox), FALSE);

  prefs_check_button_add_with_icon (object, "toolbox-wilber",
                                    _("Show GIMP _logo (drag-and-drop target)"),
                                    GIMP_ICON_WILBER,
                                    GTK_BOX (vbox2), size_group);
  prefs_check_button_add_with_icon (object, "toolbox-color-area",
                                    _("Show _foreground & background color"),
                                    GIMP_ICON_COLORS_DEFAULT,
                                    GTK_BOX (vbox2), size_group);
  prefs_check_button_add_with_icon (object, "toolbox-foo-area",
                                    _("Show active _brush, pattern & gradient"),
                                    GIMP_ICON_BRUSH,
                                    GTK_BOX (vbox2), size_group);
  prefs_check_button_add_with_icon (object, "toolbox-image-area",
                                    _("Show active _image"),
                                    GIMP_ICON_IMAGE,
                                    GTK_BOX (vbox2), size_group);

  g_object_unref (size_group);
  size_group = NULL;

  /* Tool Editor */
  vbox2 = prefs_frame_new (_("Tools Configuration"),
                           GTK_CONTAINER (vbox), TRUE);
  tool_editor = gimp_tool_editor_new (gimp->tool_info_list, gimp->user_context,
                                      gimp_tools_get_default_order (gimp),
                                      GIMP_VIEW_SIZE_SMALL, 1);

  gtk_box_pack_start (GTK_BOX (vbox2), tool_editor, TRUE, TRUE, 0);
  gtk_widget_show (tool_editor);


  /*********************************/
  /*  Interface / Dialog Defaults  */
  /*********************************/
  vbox = gimp_prefs_box_add_page (GIMP_PREFS_BOX (prefs_box),
                                  /* FIXME need an icon */
                                  "gimp-prefs-controllers",
                                  _("Dialog Defaults"),
                                  _("Dialog Defaults"),
                                  GIMP_HELP_PREFS_DIALOG_DEFAULTS,
                                  &top_iter,
                                  &child_iter);

  gimp_prefs_box_set_page_scrollable (GIMP_PREFS_BOX (prefs_box), vbox, TRUE);

  button = gimp_prefs_box_set_page_resettable (GIMP_PREFS_BOX (prefs_box),
                                               vbox,
                                               _("Reset Dialog Defaults"));
  g_signal_connect (button, "clicked",
                    G_CALLBACK (prefs_dialog_defaults_reset),
                    config);

  size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  /*  Color profile import dialog  */
  vbox2 = prefs_frame_new (_("Color Profile Import Dialog"), GTK_CONTAINER (vbox),
                           FALSE);
  table = prefs_table_new (1, GTK_CONTAINER (vbox2));

  button = prefs_enum_combo_box_add (object, "color-profile-policy", 0, 0,
                                     _("Color profile policy:"),
                                     GTK_TABLE (table), 0, size_group);

  /*  All color profile chooser dialogs  */
  vbox2 = prefs_frame_new (_("Color Profile File Dialogs"), GTK_CONTAINER (vbox),
                           FALSE);
  table = prefs_table_new (1, GTK_CONTAINER (vbox2));

  prefs_file_chooser_button_add (object, "color-profile-path",
                                 _("Profile folder:"),
                                 _("Select Default Folder for Color Profiles"),
                                 GTK_TABLE (table), 0, size_group);

  /*  Convert to Color Profile Dialog  */
  vbox2 = prefs_frame_new (_("Convert to Color Profile Dialog"),
                           GTK_CONTAINER (vbox), FALSE);
  table = prefs_table_new (1, GTK_CONTAINER (vbox2));

  prefs_enum_combo_box_add (object, "image-convert-profile-intent", 0, 0,
                            _("Rendering intent:"),
                            GTK_TABLE (table), 0, size_group);

  prefs_check_button_add (object, "image-convert-profile-black-point-compensation",
                          _("Black point compensation"),
                          GTK_BOX (vbox2));

  /*  Convert Precision Dialog  */
  vbox2 = prefs_frame_new (_("Precision Conversion Dialog"),
                           GTK_CONTAINER (vbox), FALSE);
  table = prefs_table_new (3, GTK_CONTAINER (vbox2));

  prefs_enum_combo_box_add (object,
                            "image-convert-precision-layer-dither-method",
                            0, 0,
                            _("Dither layers:"),
                            GTK_TABLE (table), 0, size_group);
  prefs_enum_combo_box_add (object,
                            "image-convert-precision-text-layer-dither-method",
                            0, 0,
                            _("Dither text layers:"),
                            GTK_TABLE (table), 1, size_group);
  prefs_enum_combo_box_add (object,
                            "image-convert-precision-channel-dither-method",
                            0, 0,
                            _("Dither channels/masks:"),
                            GTK_TABLE (table), 2, size_group);

  /*  Convert Indexed Dialog  */
  vbox2 = prefs_frame_new (_("Indexed Conversion Dialog"),
                           GTK_CONTAINER (vbox), FALSE);
  table = prefs_table_new (2, GTK_CONTAINER (vbox2));

  prefs_enum_combo_box_add (object, "image-convert-indexed-palette-type", 0, 0,
                            _("Colormap:"),
                            GTK_TABLE (table), 0, size_group);
  prefs_spin_button_add (object, "image-convert-indexed-max-colors", 1.0, 8.0, 0,
                         _("Maximum number of colors:"),
                         GTK_TABLE (table), 1, size_group);

  prefs_check_button_add (object, "image-convert-indexed-remove-duplicates",
                          _("Remove unused and duplicate colors "
                            "from colormap"),
                          GTK_BOX (vbox2));

  table = prefs_table_new (1, GTK_CONTAINER (vbox2));
  prefs_enum_combo_box_add (object, "image-convert-indexed-dither-type", 0, 0,
                            _("Color dithering:"),
                            GTK_TABLE (table), 0, size_group);

  prefs_check_button_add (object, "image-convert-indexed-dither-alpha",
                          _("Enable dithering of transparency"),
                          GTK_BOX (vbox2));
  prefs_check_button_add (object, "image-convert-indexed-dither-text-layers",
                          _("Enable dithering of text layers"),
                          GTK_BOX (vbox2));

  /*  Filter Dialogs  */
  vbox2 = prefs_frame_new (_("Filter Dialogs"),
                           GTK_CONTAINER (vbox), FALSE);
  table = prefs_table_new (1, GTK_CONTAINER (vbox2));

  prefs_spin_button_add (object, "filter-tool-max-recent", 1.0, 8.0, 0,
                         _("Keep recent settings:"),
                         GTK_TABLE (table), 1, size_group);

  button = prefs_check_button_add (object, "filter-tool-use-last-settings",
                                   _("Default to the last used settings"),
                                   GTK_BOX (vbox2));
  button = prefs_check_button_add (object, "filter-tool-show-color-options",
                                   _("Show advanced color options"),
                                   GTK_BOX (vbox2));

  /*  Canvas Size Dialog  */
  vbox2 = prefs_frame_new (_("Canvas Size Dialog"),
                           GTK_CONTAINER (vbox), FALSE);
  table = prefs_table_new (2, GTK_CONTAINER (vbox2));

  prefs_enum_combo_box_add (object, "image-resize-fill-type", 0, 0,
                            _("Fill with:"),
                            GTK_TABLE (table), 0, size_group);
  prefs_enum_combo_box_add (object, "image-resize-layer-set", 0, 0,
                            _("Resize layers:"),
                            GTK_TABLE (table), 1, size_group);

  prefs_check_button_add (object, "image-resize-resize-text-layers",
                          _("Resize text layers"),
                          GTK_BOX (vbox2));

  /*  New Layer Dialog  */
  vbox2 = prefs_frame_new (_("New Layer Dialog"),
                           GTK_CONTAINER (vbox), FALSE);
  table = prefs_table_new (2, GTK_CONTAINER (vbox2));

  prefs_entry_add (object, "layer-new-name",
                   _("Layer name:"),
                   GTK_TABLE (table), 0, size_group);

  prefs_enum_combo_box_add (object, "layer-new-fill-type", 0, 0,
                            _("Fill type:"),
                            GTK_TABLE (table), 1, size_group);

  /*  Layer Boundary Size Dialog  */
  vbox2 = prefs_frame_new (_("Layer Boundary Size Dialog"),
                           GTK_CONTAINER (vbox), FALSE);
  table = prefs_table_new (1, GTK_CONTAINER (vbox2));

  prefs_enum_combo_box_add (object, "layer-resize-fill-type", 0, 0,
                            _("Fill with:"),
                            GTK_TABLE (table), 0, size_group);

  /*  Add Layer Mask Dialog  */
  vbox2 = prefs_frame_new (_("Add Layer Mask Dialog"),
                           GTK_CONTAINER (vbox), FALSE);
  table = prefs_table_new (1, GTK_CONTAINER (vbox2));

  prefs_enum_combo_box_add (object, "layer-add-mask-type", 0, 0,
                            _("Layer mask type:"),
                            GTK_TABLE (table), 0, size_group);

  prefs_check_button_add (object, "layer-add-mask-invert",
                          _("Invert mask"),
                          GTK_BOX (vbox2));

  /*  Merge Layers Dialog  */
  vbox2 = prefs_frame_new (_("Merge Layers Dialog"),
                           GTK_CONTAINER (vbox), FALSE);
  table = prefs_table_new (1, GTK_CONTAINER (vbox2));

  prefs_enum_combo_box_add (object, "layer-merge-type",
                            GIMP_EXPAND_AS_NECESSARY,
                            GIMP_CLIP_TO_BOTTOM_LAYER,
                            _("Merged layer size:"),
                            GTK_TABLE (table), 0, size_group);

  prefs_check_button_add (object, "layer-merge-active-group-only",
                          _("Merge within active group only"),
                          GTK_BOX (vbox2));
  prefs_check_button_add (object, "layer-merge-discard-invisible",
                          _("Discard invisible layers"),
                          GTK_BOX (vbox2));

  /*  New Channel Dialog  */
  vbox2 = prefs_frame_new (_("New Channel Dialog"),
                           GTK_CONTAINER (vbox), FALSE);
  table = prefs_table_new (2, GTK_CONTAINER (vbox2));

  prefs_entry_add (object, "channel-new-name",
                   _("Channel name:"),
                   GTK_TABLE (table), 0, size_group);

  prefs_color_button_add (object, "channel-new-color",
                          _("Color and opacity:"),
                          _("Default New Channel Color and Opacity"),
                          GTK_TABLE (table), 1, size_group,
                          gimp_get_user_context (gimp));

  /*  New Path Dialog  */
  vbox2 = prefs_frame_new (_("New Path Dialog"),
                           GTK_CONTAINER (vbox), FALSE);
  table = prefs_table_new (1, GTK_CONTAINER (vbox2));

  prefs_entry_add (object, "path-new-name",
                   _("Path name:"),
                   GTK_TABLE (table), 0, size_group);

  /*  Export Path Dialog  */
  vbox2 = prefs_frame_new (_("Export Paths Dialog"),
                           GTK_CONTAINER (vbox), FALSE);
  table = prefs_table_new (1, GTK_CONTAINER (vbox2));

  prefs_file_chooser_button_add (object, "path-export-path",
                                 _("Export folder:"),
                                 _("Select Default Folder for Exporting Paths"),
                                 GTK_TABLE (table), 0, size_group);

  prefs_check_button_add (object, "path-export-active-only",
                          _("Export the active path only"),
                          GTK_BOX (vbox2));

  /*  Import Path Dialog  */
  vbox2 = prefs_frame_new (_("Import Paths Dialog"),
                           GTK_CONTAINER (vbox), FALSE);
  table = prefs_table_new (1, GTK_CONTAINER (vbox2));

  prefs_file_chooser_button_add (object, "path-import-path",
                                 _("Import folder:"),
                                 _("Select Default Folder for Importing Paths"),
                                 GTK_TABLE (table), 0, size_group);

  prefs_check_button_add (object, "path-import-merge",
                          _("Merge imported paths"),
                          GTK_BOX (vbox2));
  prefs_check_button_add (object, "path-import-scale",
                          _("Scale imported paths"),
                          GTK_BOX (vbox2));

  /*  Feather Selection Dialog  */
  vbox2 = prefs_frame_new (_("Feather Selection Dialog"),
                           GTK_CONTAINER (vbox), FALSE);
  table = prefs_table_new (1, GTK_CONTAINER (vbox2));

  prefs_spin_button_add (object, "selection-feather-radius", 1.0, 10.0, 2,
                         _("Feather radius:"),
                         GTK_TABLE (table), 0, size_group);

  /*  Grow Selection Dialog  */
  vbox2 = prefs_frame_new (_("Grow Selection Dialog"),
                           GTK_CONTAINER (vbox), FALSE);
  table = prefs_table_new (1, GTK_CONTAINER (vbox2));

  prefs_spin_button_add (object, "selection-grow-radius", 1.0, 10.0, 0,
                         _("Grow radius:"),
                         GTK_TABLE (table), 0, size_group);

  /*  Shrink Selection Dialog  */
  vbox2 = prefs_frame_new (_("Shrink Selection Dialog"),
                           GTK_CONTAINER (vbox), FALSE);
  table = prefs_table_new (1, GTK_CONTAINER (vbox2));

  prefs_spin_button_add (object, "selection-shrink-radius", 1.0, 10.0, 0,
                         _("Shrink radius:"),
                         GTK_TABLE (table), 0, size_group);

  prefs_check_button_add (object, "selection-shrink-edge-lock",
                          _("Selected areas continue outside the image"),
                          GTK_BOX (vbox2));

  /*  Border Selection Dialog  */
  vbox2 = prefs_frame_new (_("Border Selection Dialog"),
                           GTK_CONTAINER (vbox), FALSE);
  table = prefs_table_new (2, GTK_CONTAINER (vbox2));

  prefs_spin_button_add (object, "selection-border-radius", 1.0, 10.0, 0,
                         _("Border radius:"),
                         GTK_TABLE (table), 0, size_group);

  prefs_enum_combo_box_add (object, "selection-border-style", 0, 0,
                            _("Border style:"),
                            GTK_TABLE (table), 1, size_group);

  prefs_check_button_add (object, "selection-border-edge-lock",
                          _("Selected areas continue outside the image"),
                          GTK_BOX (vbox2));

  /*  Fill Options Dialog  */
  vbox2 = prefs_frame_new (_("Fill Selection Outline & Fill Path Dialogs"),
                           GTK_CONTAINER (vbox), FALSE);

  table = gimp_fill_editor_new (GIMP_DIALOG_CONFIG (object)->fill_options,
                                FALSE);
  gtk_box_pack_start (GTK_BOX (vbox2), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /*  Stroke Options Dialog  */
  vbox2 = prefs_frame_new (_("Stroke Selection & Stroke Path Dialogs"),
                           GTK_CONTAINER (vbox), FALSE);

  /* The stroke line width physical values could be based on either the
   * x or y resolution, some average, or whatever which makes a bit of
   * sense. There is no perfect answer. The actual stroke dialog though
   * uses the y resolution on the opened image. So using the y resolution
   * of the default image seems like the best compromise in the preferences.
   */
  table = gimp_stroke_editor_new (GIMP_DIALOG_CONFIG (object)->stroke_options,
                                  gimp_template_get_resolution_y (core_config->default_image),
                                  FALSE);
  gtk_box_pack_start (GTK_BOX (vbox2), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  g_object_unref (size_group);


  /*****************************/
  /*  Interface / Help System  */
  /*****************************/
  vbox = gimp_prefs_box_add_page (GIMP_PREFS_BOX (prefs_box),
                                  "gimp-prefs-help-system",
                                  _("Help System"),
                                  _("Help System"),
                                  GIMP_HELP_PREFS_HELP,
                                  &top_iter,
                                  &child_iter);

  size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  /*  General  */
  vbox2 = prefs_frame_new (_("General"), GTK_CONTAINER (vbox), FALSE);

  prefs_check_button_add (object, "show-tooltips",
                          _("Show _tooltips"),
                          GTK_BOX (vbox2));
  prefs_check_button_add (object, "show-help-button",
                          _("Show help _buttons"),
                          GTK_BOX (vbox2));

  table = prefs_table_new (3, GTK_CONTAINER (vbox2));
  button = prefs_boolean_combo_box_add (object, "user-manual-online",
                                        _("Use the online version"),
                                        _("Use a locally installed copy"),
                                        _("User manual:"),
                                        GTK_TABLE (table), 0, size_group);
  gimp_help_set_help_data (button, NULL, NULL);

  manuals = gimp_help_get_installed_languages ();
  entry   = NULL;
  if (manuals != NULL)
    {
      gchar *help_locales = NULL;

      entry = gimp_language_combo_box_new (TRUE,
                                           _("User interface language"));

      g_object_get (config, "help-locales", &help_locales, NULL);
      if (help_locales && strlen (help_locales))
        {
          gchar *sep;

          sep = strchr (help_locales, ':');
          if (sep)
            *sep = '\0';
        }
      if (help_locales)
        {
          gimp_language_combo_box_set_code (GIMP_LANGUAGE_COMBO_BOX (entry),
                                            help_locales);
          g_free (help_locales);
        }
      else
        {
          gimp_language_combo_box_set_code (GIMP_LANGUAGE_COMBO_BOX (entry),
                                            "");
        }
      g_signal_connect (entry, "changed",
                        G_CALLBACK (prefs_help_language_change_callback),
                        gimp);
      gtk_table_attach_defaults (GTK_TABLE (table), entry, 1, 2, 1, 2);
      gtk_widget_show (entry);
    }

  if (gimp_help_user_manual_is_installed (gimp))
    {
      hbox = prefs_hint_box_new (GIMP_ICON_DIALOG_INFORMATION,
                                 _("There's a local installation "
                                   "of the user manual."));
    }
  else
    {
      hbox = prefs_hint_box_new (GIMP_ICON_DIALOG_WARNING,
                                 _("The user manual is not installed "
                                   "locally."));
    }
  if (manuals)
    {
      g_object_set_data (G_OBJECT (hbox), "gimp", gimp);
      g_signal_connect (entry, "changed",
                        G_CALLBACK (prefs_help_language_change_callback2),
                        hbox);
      g_list_free_full (manuals, g_free);
    }

  gtk_table_attach_defaults (GTK_TABLE (table), hbox, 1, 2, 2, 3);
  gtk_widget_show (hbox);

  /*  Help Browser  */
#ifdef HAVE_WEBKIT
  /*  If there is no webkit available, assume we are on a platform
   *  that doesn't use the help browser, so don't bother showing
   *  the combo.
   */
  vbox2 = prefs_frame_new (_("Help Browser"), GTK_CONTAINER (vbox), FALSE);

  if (gimp_help_browser_is_installed (gimp))
    {
      table = prefs_table_new (1, GTK_CONTAINER (vbox2));

      button = prefs_enum_combo_box_add (object, "help-browser", 0, 0,
                                         _("H_elp browser to use:"),
                                         GTK_TABLE (table), 0, size_group);
    }
  else
    {
      hbox = prefs_hint_box_new (GIMP_ICON_DIALOG_WARNING,
                                 _("The GIMP help browser doesn't seem to "
                                   "be installed. Using the web browser "
                                   "instead."));
      gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);
      gtk_widget_show (hbox);

      g_object_set (config,
                    "help-browser", GIMP_HELP_BROWSER_WEB_BROWSER,
                    NULL);
    }
#else
  g_object_set (config,
                "help-browser", GIMP_HELP_BROWSER_WEB_BROWSER,
                NULL);
#endif /* HAVE_WEBKIT */

  /* Action Search */
  vbox2 = prefs_frame_new (_("Action Search"), GTK_CONTAINER (vbox), FALSE);
  table = prefs_table_new (1, GTK_CONTAINER (vbox2));

  prefs_check_button_add (object, "search-show-unavailable-actions",
                          _("Show _unavailable actions"),
                          GTK_BOX (vbox2));
  prefs_spin_button_add (object, "action-history-size", 1.0, 10.0, 0,
                         _("Maximum History Size:"),
                         GTK_TABLE (table), 0, size_group);

  button = prefs_button_add (GIMP_ICON_SHRED,
                             _("Clear Action History"),
                             GTK_BOX (vbox2));
  g_signal_connect (button, "clicked",
                    G_CALLBACK (prefs_search_clear_callback),
                    gimp);

  g_object_unref (size_group);
  size_group = NULL;


  /*************************/
  /*  Interface / Display  */
  /*************************/
  vbox = gimp_prefs_box_add_page (GIMP_PREFS_BOX (prefs_box),
                                  "gimp-prefs-display",
                                  _("Display"),
                                  _("Display"),
                                  GIMP_HELP_PREFS_DISPLAY,
                                  &top_iter,
                                  &child_iter);

  size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  /*  Transparency  */
  vbox2 = prefs_frame_new (_("Transparency"), GTK_CONTAINER (vbox), FALSE);
  table = prefs_table_new (2, GTK_CONTAINER (vbox2));

  prefs_enum_combo_box_add (object, "transparency-type", 0, 0,
                            _("_Check style:"),
                            GTK_TABLE (table), 0, size_group);
  prefs_enum_combo_box_add (object, "transparency-size", 0, 0,
                            _("Check _size:"),
                            GTK_TABLE (table), 1, size_group);

  vbox2 = prefs_frame_new (_("Monitor Resolution"),
                           GTK_CONTAINER (vbox), FALSE);

  {
    gchar *pixels_per_unit = g_strconcat (_("Pixels"), "/%s", NULL);

    entry = gimp_prop_coordinates_new (object,
                                       "monitor-xresolution",
                                       "monitor-yresolution",
                                       NULL,
                                       pixels_per_unit,
                                       GIMP_SIZE_ENTRY_UPDATE_RESOLUTION,
                                       0.0, 0.0,
                                       TRUE);

    g_free (pixels_per_unit);
  }

  gtk_table_set_col_spacings (GTK_TABLE (entry), 2);
  gtk_table_set_row_spacings (GTK_TABLE (entry), 2);

  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (entry),
                                _("Horizontal"), 0, 1, 0.0);
  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (entry),
                                _("Vertical"), 0, 2, 0.0);
  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (entry),
                                _("ppi"), 1, 4, 0.0);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);

  gtk_box_pack_start (GTK_BOX (hbox), entry, FALSE, FALSE, 24);
  gtk_widget_show (entry);
  gtk_widget_set_sensitive (entry, ! display_config->monitor_res_from_gdk);

  group = NULL;

  {
    gdouble  xres;
    gdouble  yres;
    gchar   *str;

    gimp_get_monitor_resolution (gdk_screen_get_default (), /* FIXME monitor */
                                 0, /* FIXME monitor */
                                 &xres, &yres);

    str = g_strdup_printf (_("_Detect automatically (currently %d × %d ppi)"),
                           ROUND (xres), ROUND (yres));

    button = gtk_radio_button_new_with_mnemonic (group, str);

    g_free (str);
  }

  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_object_set_data (G_OBJECT (button), "monitor_resolution_sizeentry", entry);

  g_signal_connect (button, "toggled",
                    G_CALLBACK (prefs_resolution_source_callback),
                    config);

  button = gtk_radio_button_new_with_mnemonic (group, _("_Enter manually"));
  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  if (! display_config->monitor_res_from_gdk)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  calibrate_button = gtk_button_new_with_mnemonic (_("C_alibrate..."));
  label = gtk_bin_get_child (GTK_BIN (calibrate_button));
  g_object_set (label,
                "margin-start", 4,
                "margin-end",   4,
                NULL);
  gtk_box_pack_start (GTK_BOX (hbox), calibrate_button, FALSE, FALSE, 0);
  gtk_widget_show (calibrate_button);
  gtk_widget_set_sensitive (calibrate_button,
                            ! display_config->monitor_res_from_gdk);

  g_object_bind_property (button, "active",
                          entry,  "sensitive",
                          G_BINDING_SYNC_CREATE);
  g_object_bind_property (button,           "active",
                          calibrate_button, "sensitive",
                          G_BINDING_SYNC_CREATE);

  g_signal_connect (calibrate_button, "clicked",
                    G_CALLBACK (prefs_resolution_calibrate_callback),
                    entry);

  g_object_unref (size_group);
  size_group = NULL;


  /***********************************/
  /*  Interface / Window Management  */
  /***********************************/
  vbox = gimp_prefs_box_add_page (GIMP_PREFS_BOX (prefs_box),
                                  "gimp-prefs-window-management",
                                  _("Window Management"),
                                  _("Window Management"),
                                  GIMP_HELP_PREFS_WINDOW_MANAGEMENT,
                                  &top_iter,
                                  &child_iter);

  vbox2 = prefs_frame_new (_("Window Manager Hints"),
                           GTK_CONTAINER (vbox), FALSE);

  table = prefs_table_new (1, GTK_CONTAINER (vbox2));

  prefs_enum_combo_box_add (object, "dock-window-hint", 0, 0,
                            _("Hint for _docks and toolbox:"),
                            GTK_TABLE (table), 1, size_group);

  vbox2 = prefs_frame_new (_("Focus"),
                           GTK_CONTAINER (vbox), FALSE);

  prefs_check_button_add (object, "activate-on-focus",
                          _("Activate the _focused image"),
                          GTK_BOX (vbox2));

  /* Window Positions */
  vbox2 = prefs_frame_new (_("Window Positions"), GTK_CONTAINER (vbox), FALSE);

  prefs_check_button_add (object, "save-session-info",
                          _("_Save window positions on exit"),
                          GTK_BOX (vbox2));
  prefs_check_button_add (object, "restore-monitor",
                          _("Open windows on the same _monitor they were open before"),
                          GTK_BOX (vbox2));

  button = prefs_button_add (GIMP_ICON_DOCUMENT_SAVE,
                             _("Save Window Positions _Now"),
                             GTK_BOX (vbox2));
  g_signal_connect (button, "clicked",
                    G_CALLBACK (prefs_session_save_callback),
                    gimp);

  button2 = prefs_button_add (GIMP_ICON_RESET,
                              _("_Reset Saved Window Positions to "
                                "Default Values"),
                              GTK_BOX (vbox2));
  g_signal_connect (button2, "clicked",
                    G_CALLBACK (prefs_session_clear_callback),
                    gimp);

  g_object_set_data (G_OBJECT (button), "clear-button", button2);


  /*******************/
  /*  Image Windows  */
  /*******************/
  vbox = gimp_prefs_box_add_page (GIMP_PREFS_BOX (prefs_box),
                                  "gimp-prefs-image-windows",
                                  _("Image Windows"),
                                  _("Image Windows"),
                                  GIMP_HELP_PREFS_IMAGE_WINDOW,
                                  NULL,
                                  &top_iter);

  size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  /*  General  */
  vbox2 = prefs_frame_new (_("General"), GTK_CONTAINER (vbox), FALSE);

  prefs_check_button_add (object, "default-dot-for-dot",
                          _("Use \"_Dot for dot\" by default"),
                          GTK_BOX (vbox2));

  table = prefs_table_new (1, GTK_CONTAINER (vbox2));

  prefs_spin_button_add (object, "marching-ants-speed", 1.0, 10.0, 0,
                         _("Marching _ants speed:"),
                         GTK_TABLE (table), 0, size_group);

  /*  Zoom & Resize Behavior  */
  vbox2 = prefs_frame_new (_("Zoom & Resize Behavior"),
                           GTK_CONTAINER (vbox), FALSE);

  prefs_check_button_add (object, "resize-windows-on-zoom",
                          _("Resize window on _zoom"),
                          GTK_BOX (vbox2));
  prefs_check_button_add (object, "resize-windows-on-resize",
                          _("Resize window on image _size change"),
                          GTK_BOX (vbox2));

  table = prefs_table_new (1, GTK_CONTAINER (vbox2));

  prefs_boolean_combo_box_add (object, "initial-zoom-to-fit",
                               _("Show entire image"),
                               "1:1",
                               _("Initial zoom _ratio:"),
                               GTK_TABLE (table), 0, size_group);

  /*  Space Bar  */
  vbox2 = prefs_frame_new (_("Space Bar"),
                           GTK_CONTAINER (vbox), FALSE);

  table = prefs_table_new (1, GTK_CONTAINER (vbox2));

  prefs_enum_combo_box_add (object, "space-bar-action", 0, 0,
                            _("_While space bar is pressed:"),
                            GTK_TABLE (table), 0, size_group);

  /*  Mouse Pointers  */
  vbox2 = prefs_frame_new (_("Mouse Pointers"),
                           GTK_CONTAINER (vbox), FALSE);

  prefs_check_button_add (object, "show-brush-outline",
                          _("Show _brush outline"),
                          GTK_BOX (vbox2));
  prefs_check_button_add (object, "show-paint-tool-cursor",
                          _("Show pointer for paint _tools"),
                          GTK_BOX (vbox2));

  table = prefs_table_new (2, GTK_CONTAINER (vbox2));

  prefs_enum_combo_box_add (object, "cursor-mode", 0, 0,
                            _("Pointer _mode:"),
                            GTK_TABLE (table), 0, size_group);
  prefs_enum_combo_box_add (object, "cursor-handedness", 0, 0,
                            _("Pointer _handedness:"),
                            GTK_TABLE (table), 1, NULL);

  g_object_unref (size_group);
  size_group = NULL;


  /********************************/
  /*  Image Windows / Appearance  */
  /********************************/
  vbox = gimp_prefs_box_add_page (GIMP_PREFS_BOX (prefs_box),
                                  "gimp-prefs-image-windows-appearance",
                                  _("Image Window Appearance"),
                                  _("Appearance"),
                                  GIMP_HELP_PREFS_IMAGE_WINDOW_APPEARANCE,
                                  &top_iter,
                                  &child_iter);

  prefs_display_options_frame_add (gimp,
                                   G_OBJECT (display_config->default_view),
                                   _("Default Appearance in Normal Mode"),
                                   GTK_CONTAINER (vbox));

  prefs_display_options_frame_add (gimp,
                                   G_OBJECT (display_config->default_fullscreen_view),
                                   _("Default Appearance in Fullscreen Mode"),
                                   GTK_CONTAINER (vbox));


  /****************************************************/
  /*  Image Windows / Image Title & Statusbar Format  */
  /****************************************************/
  vbox = gimp_prefs_box_add_page (GIMP_PREFS_BOX (prefs_box),
                                  "gimp-prefs-image-title",
                                  _("Image Title & Statusbar Format"),
                                  _("Title & Status"),
                                  GIMP_HELP_PREFS_IMAGE_WINDOW_TITLE,
                                  &top_iter,
                                  &child_iter);

  {
    const gchar *format_strings[] =
    {
      NULL,
      NULL,
      "%f-%p.%i (%t) %z%%",
      "%f-%p.%i (%t) %d:%s",
      "%f-%p.%i (%t) %wx%h",
      "%f-%p-%i (%t) %wx%h (%xx%y)"
    };

    const gchar *format_names[] =
    {
      N_("Current format"),
      N_("Default format"),
      N_("Show zoom percentage"),
      N_("Show zoom ratio"),
      N_("Show image size"),
      N_("Show drawable size")
    };

    struct
    {
      gchar       *current_setting;
      const gchar *default_setting;
      const gchar *title;
      const gchar *property_name;
    }
    formats[] =
    {
      { NULL, GIMP_CONFIG_DEFAULT_IMAGE_TITLE_FORMAT,
        N_("Image Title Format"),     "image-title-format"  },
      { NULL, GIMP_CONFIG_DEFAULT_IMAGE_STATUS_FORMAT,
        N_("Image Statusbar Format"), "image-status-format" }
    };

    gint format;

    gimp_assert (G_N_ELEMENTS (format_strings) == G_N_ELEMENTS (format_names));

    formats[0].current_setting = display_config->image_title_format;
    formats[1].current_setting = display_config->image_status_format;

    for (format = 0; format < G_N_ELEMENTS (formats); format++)
      {
        GtkWidget        *scrolled_win;
        GtkListStore     *list_store;
        GtkWidget        *view;
        GtkTreeSelection *sel;
        gint              i;

        format_strings[0] = formats[format].current_setting;
        format_strings[1] = formats[format].default_setting;

        vbox2 = prefs_frame_new (gettext (formats[format].title),
                                 GTK_CONTAINER (vbox), TRUE);

        entry = gimp_prop_entry_new (object, formats[format].property_name, 0);
        gtk_box_pack_start (GTK_BOX (vbox2), entry, FALSE, FALSE, 0);
        gtk_widget_show (entry);

        scrolled_win = gtk_scrolled_window_new (NULL, NULL);
        gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_win),
                                             GTK_SHADOW_IN);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
                                        GTK_POLICY_NEVER,
                                        GTK_POLICY_AUTOMATIC);
        gtk_box_pack_start (GTK_BOX (vbox2), scrolled_win, TRUE, TRUE, 0);
        gtk_widget_show (scrolled_win);

        list_store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);

        view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (list_store));
        gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (view), FALSE);
        gtk_container_add (GTK_CONTAINER (scrolled_win), view);
        gtk_widget_show (view);

        g_object_unref (list_store);

        gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (view), 0,
                                                     NULL,
                                                     gtk_cell_renderer_text_new (),
                                                     "text", 0,
                                                     NULL);
        gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (view), 1,
                                                     NULL,
                                                     gtk_cell_renderer_text_new (),
                                                     "text", 1,
                                                     NULL);

        sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));

        for (i = 0; i < G_N_ELEMENTS (format_strings); i++)
          {
            GtkTreeIter iter;

            gtk_list_store_append (list_store, &iter);
            gtk_list_store_set (list_store, &iter,
                                0, gettext (format_names[i]),
                                1, format_strings[i],
                                -1);

            if (i == 0)
              gtk_tree_selection_select_iter (sel, &iter);
          }

        g_signal_connect (sel, "changed",
                          G_CALLBACK (prefs_format_string_select_callback),
                          entry);
      }
  }

  /******************************/
  /*  Image Windows / Snapping  */
  /******************************/
  vbox = gimp_prefs_box_add_page (GIMP_PREFS_BOX (prefs_box),
                                  "gimp-prefs-image-windows-snapping",
                                  _("Image Window Snapping Behavior"),
                                  _("Snapping"),
                                  GIMP_HELP_PREFS_IMAGE_WINDOW_SNAPPING,
                                  &top_iter,
                                  &child_iter);

  prefs_behavior_options_frame_add (gimp,
                                    G_OBJECT (display_config->default_view),
                                    _("Default Behavior in Normal Mode"),
                                    GTK_CONTAINER (vbox));
  prefs_behavior_options_frame_add (gimp,
                                    G_OBJECT (display_config->default_fullscreen_view),
                                    _("Default Behavior in Fullscreen Mode"),
                                    GTK_CONTAINER (vbox));

  /*  Snapping Distance  */
  vbox2 = prefs_frame_new (_("General"),
                           GTK_CONTAINER (vbox), FALSE);
  table = prefs_table_new (1, GTK_CONTAINER (vbox2));

  prefs_spin_button_add (object, "snap-distance", 1.0, 5.0, 0,
                         _("_Snapping distance:"),
                         GTK_TABLE (table), 0, size_group);


  /*******************/
  /*  Input Devices  */
  /*******************/
  vbox = gimp_prefs_box_add_page (GIMP_PREFS_BOX (prefs_box),
                                  "gimp-prefs-input-devices",
                                  _("Input Devices"),
                                  _("Input Devices"),
                                  GIMP_HELP_PREFS_INPUT_DEVICES,
                                  NULL,
                                  &top_iter);

  /*  Extended Input Devices  */
  vbox2 = prefs_frame_new (_("Extended Input Devices"),
                           GTK_CONTAINER (vbox), FALSE);

  prefs_check_button_add (object, "devices-share-tool",
                          _("S_hare tool and tool options between input devices"),
                          GTK_BOX (vbox2));

  button = prefs_button_add (GIMP_ICON_PREFERENCES_SYSTEM,
                             _("Configure E_xtended Input Devices..."),
                             GTK_BOX (vbox2));
  g_signal_connect (button, "clicked",
                    G_CALLBACK (prefs_input_devices_dialog),
                    gimp);

  prefs_check_button_add (object, "save-device-status",
                          _("_Save input device settings on exit"),
                          GTK_BOX (vbox2));

  button = prefs_button_add (GIMP_ICON_DOCUMENT_SAVE,
                             _("Save Input Device Settings _Now"),
                             GTK_BOX (vbox2));
  g_signal_connect (button, "clicked",
                    G_CALLBACK (prefs_devices_save_callback),
                    gimp);

  button2 = prefs_button_add (GIMP_ICON_RESET,
                              _("_Reset Saved Input Device Settings to "
                                "Default Values"),
                              GTK_BOX (vbox2));
  g_signal_connect (button2, "clicked",
                    G_CALLBACK (prefs_devices_clear_callback),
                    gimp);

  g_object_set_data (G_OBJECT (button), "clear-button", button2);


  /****************************/
  /*  Additional Controllers  */
  /****************************/
  vbox = gimp_prefs_box_add_page (GIMP_PREFS_BOX (prefs_box),
                                  "gimp-prefs-controllers",
                                  _("Additional Input Controllers"),
                                  _("Input Controllers"),
                                  GIMP_HELP_PREFS_INPUT_CONTROLLERS,
                                  &top_iter,
                                  &child_iter);

  vbox2 = gimp_controller_list_new (gimp);
  gtk_box_pack_start (GTK_BOX (vbox), vbox2, TRUE, TRUE, 0);
  gtk_widget_show (vbox2);


  /*************/
  /*  Folders  */
  /*************/
  vbox = gimp_prefs_box_add_page (GIMP_PREFS_BOX (prefs_box),
                                  "gimp-prefs-folders",
                                  _("Folders"),
                                  _("Folders"),
                                  GIMP_HELP_PREFS_FOLDERS,
                                  NULL,
                                  &top_iter);

  {
    static const struct
    {
      const gchar *property_name;
      const gchar *label;
      const gchar *dialog_title;
    }
    dirs[] =
    {
      {
        "temp-path",
        N_("Temporary folder:"),
        N_("Select Folder for Temporary Files")
      },
      {
        "swap-path",
        N_("Swap folder:"),
        N_("Select Swap Folder")
      }
    };

    table = prefs_table_new (G_N_ELEMENTS (dirs) + 1, GTK_CONTAINER (vbox));

    for (i = 0; i < G_N_ELEMENTS (dirs); i++)
      {
        prefs_file_chooser_button_add (object, dirs[i].property_name,
                                       gettext (dirs[i].label),
                                       gettext (dirs[i].dialog_title),
                                       GTK_TABLE (table), i, NULL);
      }
  }


  /*********************/
  /* Folders / <paths> */
  /*********************/
  {
    static const struct
    {
      const gchar *tree_label;
      const gchar *label;
      const gchar *icon;
      const gchar *help_data;
      const gchar *fs_label;
      const gchar *path_property_name;
      const gchar *writable_property_name;
    }
    paths[] =
    {
      { N_("Brushes"), N_("Brush Folders"), "folders-brushes",
        GIMP_HELP_PREFS_FOLDERS_BRUSHES,
        N_("Select Brush Folders"),
        "brush-path", "brush-path-writable" },
      { N_("Dynamics"), N_("Dynamics Folders"), "folders-dynamics",
        GIMP_HELP_PREFS_FOLDERS_DYNAMICS,
        N_("Select Dynamics Folders"),
        "dynamics-path", "dynamics-path-writable" },
      { N_("Patterns"), N_("Pattern Folders"), "folders-patterns",
        GIMP_HELP_PREFS_FOLDERS_PATTERNS,
        N_("Select Pattern Folders"),
        "pattern-path", "pattern-path-writable" },
      { N_("Palettes"), N_("Palette Folders"), "folders-palettes",
        GIMP_HELP_PREFS_FOLDERS_PALETTES,
        N_("Select Palette Folders"),
        "palette-path", "palette-path-writable" },
      { N_("Gradients"), N_("Gradient Folders"), "folders-gradients",
        GIMP_HELP_PREFS_FOLDERS_GRADIENTS,
        N_("Select Gradient Folders"),
        "gradient-path", "gradient-path-writable" },
      { N_("Fonts"), N_("Font Folders"), "folders-fonts",
        GIMP_HELP_PREFS_FOLDERS_FONTS,
        N_("Select Font Folders"),
        "font-path", NULL },
      { N_("Tool Presets"), N_("Tool Preset Folders"), "folders-tool-presets",
        GIMP_HELP_PREFS_FOLDERS_TOOL_PRESETS,
        N_("Select Tool Preset Folders"),
        "tool-preset-path", "tool-preset-path-writable" },
      { N_("MyPaint Brushes"), N_("MyPaint Brush Folders"), "folders-mypaint-brushes",
        GIMP_HELP_PREFS_FOLDERS_MYPAINT_BRUSHES,
        N_("Select MyPaint Brush Folders"),
        "mypaint-brush-path", "mypaint-brush-path-writable" },
      { N_("Plug-ins"), N_("Plug-in Folders"), "folders-plug-ins",
        GIMP_HELP_PREFS_FOLDERS_PLUG_INS,
        N_("Select plug-in Folders"),
        "plug-in-path", NULL },
      { N_("Scripts"), N_("Script-Fu Folders"), "folders-scripts",
        GIMP_HELP_PREFS_FOLDERS_SCRIPTS,
        N_("Select Script-Fu Folders"),
        "script-fu-path", NULL },
      { N_("Modules"), N_("Module Folders"), "folders-modules",
        GIMP_HELP_PREFS_FOLDERS_MODULES,
        N_("Select Module Folders"),
        "module-path", NULL },
      { N_("Interpreters"), N_("Interpreter Folders"), "folders-interp",
        GIMP_HELP_PREFS_FOLDERS_INTERPRETERS,
        N_("Select Interpreter Folders"),
        "interpreter-path", NULL },
      { N_("Environment"), N_("Environment Folders"), "folders-environ",
        GIMP_HELP_PREFS_FOLDERS_ENVIRONMENT,
        N_("Select Environment Folders"),
        "environ-path", NULL },
      { N_("Themes"), N_("Theme Folders"), "folders-themes",
        GIMP_HELP_PREFS_FOLDERS_THEMES,
        N_("Select Theme Folders"),
        "theme-path", NULL },
      { N_("Icon Themes"), N_("Icon Theme Folders"), "folders-icon-themes",
        GIMP_HELP_PREFS_FOLDERS_ICON_THEMES,
        N_("Select Icon Theme Folders"),
        "icon-theme-path", NULL }
    };

    for (i = 0; i < G_N_ELEMENTS (paths); i++)
      {
        GtkWidget *editor;
        gchar     *icon_name;

        icon_name = g_strconcat ("gimp-prefs-", paths[i].icon, NULL);
        vbox = gimp_prefs_box_add_page (GIMP_PREFS_BOX (prefs_box),
                                        icon_name,
                                        gettext (paths[i].label),
                                        gettext (paths[i].tree_label),
                                        paths[i].help_data,
                                        &top_iter,
                                        &child_iter);
        g_free (icon_name);

        editor = gimp_prop_path_editor_new (object,
                                            paths[i].path_property_name,
                                            paths[i].writable_property_name,
                                            gettext (paths[i].fs_label));
        gtk_box_pack_start (GTK_BOX (vbox), editor, TRUE, TRUE, 0);
        gtk_widget_show (editor);
      }
  }

  {
    GtkWidget    *tv;
    GtkTreeModel *model;
    GtkTreePath  *path;

    tv = gimp_prefs_box_get_tree_view (GIMP_PREFS_BOX (prefs_box));
    gtk_tree_view_expand_all (GTK_TREE_VIEW (tv));

    /*  collapse the Folders subtree */
    model = gtk_tree_view_get_model (GTK_TREE_VIEW (tv));
    path = gtk_tree_model_get_path (model, &top_iter);
    gtk_tree_view_collapse_row (GTK_TREE_VIEW (tv), path);
    gtk_tree_path_free (path);
  }

  return dialog;
}
