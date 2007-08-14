/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "dialogs-types.h"

#include "config/gimprc.h"

#include "core/gimp.h"
#include "core/gimplist.h"
#include "core/gimptemplate.h"

#include "widgets/gimpcolorpanel.h"
#include "widgets/gimpcontainercombobox.h"
#include "widgets/gimpcontainerview.h"
#include "widgets/gimpcontrollerlist.h"
#include "widgets/gimpdeviceinfo.h"
#include "widgets/gimpdevices.h"
#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpgrideditor.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpmessagebox.h"
#include "widgets/gimpmessagedialog.h"
#include "widgets/gimpprofilechooserdialog.h"
#include "widgets/gimppropwidgets.h"
#include "widgets/gimptemplateeditor.h"
#include "widgets/gimpwidgets-utils.h"

#include "menus/menus.h"

#include "tools/gimp-tools.h"

#include "gui/session.h"
#include "gui/themes.h"

#include "preferences-dialog.h"
#include "resolution-calibrate-dialog.h"

#include "gimp-intl.h"


#define RESPONSE_RESET 1


/*  preferences local functions  */

static GtkWidget * prefs_dialog_new               (Gimp       *gimp,
                                                   GimpConfig *config);
static void        prefs_config_notify            (GObject    *config,
                                                   GParamSpec *param_spec,
                                                   GObject    *config_copy);
static void        prefs_config_copy_notify       (GObject    *config_copy,
                                                   GParamSpec *param_spec,
                                                   GObject    *config);
static void        prefs_response                 (GtkWidget  *widget,
                                                   gint        response_id,
                                                   GtkWidget  *dialog);

static void        prefs_message                  (GtkMessageType  type,
                                                   gboolean        destroy,
                                                   const gchar    *message);

static void   prefs_notebook_page_callback        (GtkNotebook      *notebook,
                                                   GtkNotebookPage  *page,
                                                   guint             page_num,
                                                   GtkTreeSelection *sel);
static void   prefs_resolution_source_callback    (GtkWidget  *widget,
                                                   GObject    *config);
static void   prefs_resolution_calibrate_callback (GtkWidget  *widget,
                                                   GtkWidget  *entry);
static void   prefs_input_devices_dialog          (GtkWidget  *widget,
                                                   Gimp       *gimp);
static void   prefs_input_dialog_able_callback    (GtkWidget  *widget,
                                                   GdkDevice  *device,
                                                   gpointer    data);
static void   prefs_menus_save_callback           (GtkWidget  *widget,
                                                   Gimp       *gimp);
static void   prefs_menus_clear_callback          (GtkWidget  *widget,
                                                   Gimp       *gimp);
static void   prefs_menus_remove_callback         (GtkWidget  *widget,
                                                   Gimp       *gimp);
static void   prefs_session_save_callback         (GtkWidget  *widget,
                                                   Gimp       *gimp);
static void   prefs_session_clear_callback        (GtkWidget  *widget,
                                                   Gimp       *gimp);
static void   prefs_devices_save_callback         (GtkWidget  *widget,
                                                   Gimp       *gimp);
static void   prefs_devices_clear_callback        (GtkWidget  *widget,
                                                   Gimp       *gimp);
static void   prefs_tool_options_save_callback    (GtkWidget  *widget,
                                                   Gimp       *gimp);
static void   prefs_tool_options_clear_callback   (GtkWidget  *widget,
                                                   Gimp       *gimp);


/*  private variables  */

static GtkWidget *prefs_dialog = NULL;


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
  GValue global_value = { 0, };
  GValue copy_value   = { 0, };

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
  GValue copy_value   = { 0, };
  GValue global_value = { 0, };

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
                                           GIMP_STOCK_QUESTION,
                                           dialog,
                                           GTK_DIALOG_MODAL |
                                           GTK_DIALOG_DESTROY_WITH_PARENT,
                                           gimp_standard_help_func, NULL,

                                           GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                           GIMP_STOCK_RESET, GTK_RESPONSE_OK,

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
            GValue      value      = { 0, };

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
            GValue      value      = { 0, };

            g_value_init (&value, param_spec->value_type);

            g_object_get_property (config_orig,
                                   param_spec->name, &value);
            g_object_set_property (G_OBJECT (gimp->edit_config),
                                   param_spec->name, &value);

            g_value_unset (&value);
          }

        g_object_thaw_notify (G_OBJECT (gimp->edit_config));

        g_list_free (diff);
      }
    }

  /*  enable autosaving again  */
  gimp_rc_set_autosave (GIMP_RC (gimp->edit_config), TRUE);

  gtk_widget_destroy (dialog);
}

static void
prefs_template_select_callback (GimpContainerView *view,
                                GimpTemplate      *template,
                                gpointer           insert_data,
                                GimpTemplate      *edit_template)
{
  if (template)
    gimp_config_sync (G_OBJECT (template), G_OBJECT (edit_template), 0);
}

static void
prefs_resolution_source_callback (GtkWidget *widget,
                                  GObject   *config)
{
  gdouble  xres;
  gdouble  yres;
  gboolean from_gdk;

  gimp_toggle_button_sensitive_update (GTK_TOGGLE_BUTTON (widget));

  from_gdk = GTK_TOGGLE_BUTTON (widget)->active;

  if (from_gdk)
    {
      gimp_get_screen_resolution (NULL, &xres, &yres);
    }
  else
    {
      GimpSizeEntry *entry = g_object_get_data (G_OBJECT (widget),
                                                "monitor_resolution_sizeentry");
      if (entry)
        {
          xres = gimp_size_entry_get_refval (entry, 0);
          yres = gimp_size_entry_get_refval (entry, 1);
        }
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
  GtkWidget *dialog;
  GtkWidget *notebook;
  GtkWidget *image;

  dialog = gtk_widget_get_toplevel (entry);

  notebook = g_object_get_data (G_OBJECT (dialog),   "notebook");
  image    = g_object_get_data (G_OBJECT (notebook), "image");

  resolution_calibrate_dialog (entry, gtk_image_get_pixbuf (GTK_IMAGE (image)));
}

static void
prefs_input_devices_dialog (GtkWidget *widget,
                            Gimp      *gimp)
{
  static GtkWidget *input_dialog = NULL;

  if (input_dialog)
    {
      gtk_window_present (GTK_WINDOW (input_dialog));
      return;
    }

  input_dialog = g_object_new (GTK_TYPE_INPUT_DIALOG,
                               "title", _("Configure Input Devices"),
                               NULL);

  g_object_add_weak_pointer (G_OBJECT (input_dialog),
                             (gpointer) &input_dialog);

  gtk_window_set_transient_for (GTK_WINDOW (input_dialog),
                                GTK_WINDOW (prefs_dialog));
  gtk_window_set_destroy_with_parent (GTK_WINDOW (input_dialog), TRUE);

  g_signal_connect_swapped (GTK_INPUT_DIALOG (input_dialog)->save_button,
                            "clicked",
                            G_CALLBACK (gimp_devices_save),
                            gimp);

  g_signal_connect_swapped (GTK_INPUT_DIALOG (input_dialog)->close_button,
                            "clicked",
                            G_CALLBACK (gtk_widget_destroy),
                            input_dialog);

  g_signal_connect (input_dialog, "enable_device",
                    G_CALLBACK (prefs_input_dialog_able_callback),
                    NULL);
  g_signal_connect (input_dialog, "disable_device",
                    G_CALLBACK (prefs_input_dialog_able_callback),
                    NULL);

  gtk_widget_show (input_dialog);
}

static void
prefs_input_dialog_able_callback (GtkWidget *widget,
                                  GdkDevice *device,
                                  gpointer   data)
{
  gimp_device_info_changed_by_device (device);
}

static void
prefs_keyboard_shortcuts_dialog (GtkWidget *widget,
                                 Gimp      *gimp)
{
  gimp_dialog_factory_dialog_raise (gimp_dialog_factory_from_name ("toplevel"),
                                    gtk_widget_get_screen (widget),
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
                                    GIMP_STOCK_QUESTION,
                                    gtk_widget_get_toplevel (widget),
                                    GTK_DIALOG_MODAL |
                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                    gimp_standard_help_func, NULL,

                                    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                    GTK_STOCK_CLEAR,  GTK_RESPONSE_OK,

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

static GtkWidget *
prefs_notebook_append_page (Gimp          *gimp,
                            GtkNotebook   *notebook,
                            const gchar   *notebook_label,
                            const gchar   *notebook_icon,
                            GtkTreeStore  *tree,
                            const gchar   *tree_label,
                            const gchar   *help_id,
                            GtkTreeIter   *parent,
                            GtkTreeIter   *iter,
                            gint           page_index)
{
  GtkWidget *event_box;
  GtkWidget *vbox;
  GdkPixbuf *pixbuf       = NULL;
  GdkPixbuf *small_pixbuf = NULL;

  event_box = gtk_event_box_new ();
  gtk_event_box_set_visible_window (GTK_EVENT_BOX (event_box), FALSE);
  gtk_notebook_append_page (notebook, event_box, NULL);
  gtk_widget_show (event_box);

  gimp_help_set_help_data (event_box, NULL, help_id);

  vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_add (GTK_CONTAINER (event_box), vbox);
  gtk_widget_show (vbox);

  if (notebook_icon)
    {
      gchar *basename;
      gchar *filename;

      basename = g_strconcat (notebook_icon, ".png", NULL);
      filename = themes_get_theme_file (gimp, "images", "preferences",
                                        basename, NULL);

      if (g_file_test (filename, G_FILE_TEST_IS_REGULAR))
        pixbuf = gdk_pixbuf_new_from_file (filename, NULL);

      g_free (filename);
      g_free (basename);

      basename = g_strconcat (notebook_icon, "-22.png", NULL);
      filename = themes_get_theme_file (gimp, "images", "preferences",
                                        basename, NULL);

      if (g_file_test (filename, G_FILE_TEST_IS_REGULAR))
        small_pixbuf = gdk_pixbuf_new_from_file (filename, NULL);
      else if (pixbuf)
        small_pixbuf = gdk_pixbuf_scale_simple (pixbuf,
                                                22, 22, GDK_INTERP_BILINEAR);

      g_free (filename);
      g_free (basename);
    }

  gtk_tree_store_append (tree, iter, parent);
  gtk_tree_store_set (tree, iter,
                      0, small_pixbuf,
                      1, tree_label ? tree_label : notebook_label,
                      2, page_index,
                      3, notebook_label,
                      4, pixbuf,
                      -1);

  if (pixbuf)
    g_object_unref (pixbuf);

  if (small_pixbuf)
    g_object_unref (small_pixbuf);

  return vbox;
}

static void
prefs_tree_select_callback (GtkTreeSelection *sel,
                            GtkNotebook      *notebook)
{
  GtkWidget    *label;
  GtkWidget    *image;
  GtkTreeModel *model;
  GtkTreeIter   iter;
  gchar        *text;
  GdkPixbuf    *pixbuf;
  gint          index;

  if (! gtk_tree_selection_get_selected (sel, &model, &iter))
    return;

  label = g_object_get_data (G_OBJECT (notebook), "label");
  image = g_object_get_data (G_OBJECT (notebook), "image");

  gtk_tree_model_get (model, &iter,
                      3, &text,
                      4, &pixbuf,
                      2, &index,
                      -1);

  gtk_label_set_text (GTK_LABEL (label), text);
  g_free (text);

  gtk_image_set_from_pixbuf (GTK_IMAGE (image), pixbuf);
  if (pixbuf)
    g_object_unref (pixbuf);

  g_signal_handlers_block_by_func (notebook,
                                   prefs_notebook_page_callback,
                                   sel);
  gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), index);
  g_signal_handlers_unblock_by_func (notebook,
                                     prefs_notebook_page_callback,
                                     sel);
}

static void
prefs_notebook_page_callback (GtkNotebook      *notebook,
                              GtkNotebookPage  *page,
                              guint             page_num,
                              GtkTreeSelection *sel)
{
  GtkTreeView  *view;
  GtkTreeModel *model;
  GtkTreeIter   iter;
  gboolean      iter_valid;

  view  = gtk_tree_selection_get_tree_view (sel);
  model = gtk_tree_view_get_model (view);

  for (iter_valid = gtk_tree_model_get_iter_first (model, &iter);
       iter_valid;
       iter_valid = gtk_tree_model_iter_next (model, &iter))
    {
      gint index;

      gtk_tree_model_get (model, &iter, 2, &index, -1);

      if (index == page_num)
        {
          gtk_tree_selection_select_iter (sel, &iter);
          return;
        }

      if (gtk_tree_model_iter_has_child (model, &iter))
        {
          gint num_children;
          gint i;

          num_children = gtk_tree_model_iter_n_children (model, &iter);

          for (i = 0; i < num_children; i++)
            {
              GtkTreeIter child_iter;

              gtk_tree_model_iter_nth_child (model, &child_iter, &iter, i);
              gtk_tree_model_get (model, &child_iter, 2, &index, -1);

              if (index == page_num)
                {
                  GtkTreePath *path;

                  path = gtk_tree_model_get_path (model, &child_iter);
                  gtk_tree_view_expand_to_path (view, path);
                  gtk_tree_selection_select_iter (sel, &child_iter);
                  return;
                }
            }
        }
    }
}

static void
prefs_format_string_select_callback (GtkTreeSelection *sel,
                                     GtkEntry         *entry)
{
  GtkTreeModel *model;
  GtkTreeIter   iter;

  if (gtk_tree_selection_get_selected (sel, &model, &iter))
    {
      GValue val = { 0, };

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
      GValue val = { 0, };

      gtk_tree_model_get_value (model, &iter, 0, &val);
      g_object_set (gimp->config, "theme", g_value_get_string (&val), NULL);
      g_value_unset (&val);
    }
}

static void
prefs_theme_reload_callback (GtkWidget *button,
                             Gimp      *gimp)
{
  g_object_notify (G_OBJECT (gimp->config), "theme");
}

static GtkWidget *
prefs_frame_new (const gchar  *label,
                 GtkContainer *parent,
                 gboolean      expand)
{
  GtkWidget *frame;
  GtkWidget *vbox;

  frame = gimp_frame_new (label);

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  if (GTK_IS_BOX (parent))
    gtk_box_pack_start (GTK_BOX (parent), frame, expand, expand, 0);
  else
    gtk_container_add (parent, frame);

  gtk_widget_show (frame);

  return vbox;
}

static GtkWidget *
prefs_table_new (gint          rows,
                 GtkContainer *parent)
{
  GtkWidget *table;

  table = gtk_table_new (rows, 2, FALSE);

  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);

  if (GTK_IS_BOX (parent))
    gtk_box_pack_start (GTK_BOX (parent), table, FALSE, FALSE, 0);
  else
    gtk_container_add (parent, table);

  gtk_widget_show (table);

  return table;
}

static void
prefs_profile_combo_dialog_response (GimpProfileChooserDialog *dialog,
                                     gint                      response,
                                     GimpColorProfileComboBox *combo)
{
  if (response == GTK_RESPONSE_ACCEPT)
    {
      gchar *filename;

      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

      if (filename)
        {
          gchar *label = gimp_profile_chooser_dialog_get_desc (dialog,
                                                               filename);

          gimp_color_profile_combo_box_set_active (combo, filename, label);

          g_free (label);
        }
    }

  gtk_widget_hide (GTK_WIDGET (dialog));
}

static void
prefs_profile_combo_changed (GimpColorProfileComboBox *combo,
                             GObject                  *config)
{
  gchar *filename = gimp_color_profile_combo_box_get_active (combo);

  g_object_set (config,
                g_object_get_data (G_OBJECT (combo), "property-name"), filename,
                NULL);

  g_free (filename);
}

static GtkWidget *
prefs_profile_combo_box_new (Gimp         *gimp,
                             GObject      *config,
                             GtkListStore *store,
                             const gchar  *label,
                             const gchar  *property_name)
{
  GtkWidget *dialog = gimp_profile_chooser_dialog_new (gimp, label);
  GtkWidget *combo;
  gchar     *filename;

  g_object_get (config, property_name, &filename, NULL);

  combo = gimp_color_profile_combo_box_new_with_model (dialog,
                                                       GTK_TREE_MODEL (store));

  g_object_set_data (G_OBJECT (combo),
                     "property-name", (gpointer) property_name);

  gimp_color_profile_combo_box_set_active (GIMP_COLOR_PROFILE_COMBO_BOX (combo),
                                           filename, NULL);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (prefs_profile_combo_dialog_response),
                    combo);

  g_signal_connect (combo, "changed",
                    G_CALLBACK (prefs_profile_combo_changed),
                    config);

  return combo;
}

static GtkWidget *
prefs_button_add (const gchar *stock_id,
                  const gchar *label,
                  GtkBox      *box)
{
  GtkWidget *button;
  GtkWidget *hbox;
  GtkWidget *image;
  GtkWidget *lab;

  button = gtk_button_new ();

  hbox = gtk_hbox_new (FALSE, 6);
  gtk_container_add (GTK_CONTAINER (button), hbox);
  gtk_widget_show (hbox);

  image = gtk_image_new_from_stock (stock_id, GTK_ICON_SIZE_BUTTON);
  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
  gtk_widget_show (image);

  lab = gtk_label_new_with_mnemonic (label);
  gtk_label_set_mnemonic_widget (GTK_LABEL (lab), button);
  gtk_box_pack_start (GTK_BOX (hbox), lab, TRUE, TRUE, 0);
  gtk_widget_show (lab);

  gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  return button;
}

static GtkWidget *
prefs_check_button_add (GObject     *config,
                        const gchar *property_name,
                        const gchar *label,
                        GtkBox      *vbox)
{
  GtkWidget *button;

  button = gimp_prop_check_button_new (config, property_name, label);

  if (button)
    {
      gtk_box_pack_start (vbox, button, FALSE, FALSE, 0);
      gtk_widget_show (button);
    }

  return button;
}

static GtkWidget *
prefs_check_button_add_with_icon (GObject      *config,
                                  const gchar  *property_name,
                                  const gchar  *label,
                                  const gchar  *stock_id,
                                  GtkBox       *vbox,
                                  GtkSizeGroup *group)
{
  GtkWidget *button;
  GtkWidget *hbox;
  GtkWidget *image;

  button = gimp_prop_check_button_new (config, property_name, label);
  if (!button)
    return NULL;

  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (vbox, hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  image = gtk_image_new_from_stock (stock_id, GTK_ICON_SIZE_BUTTON);
  gtk_misc_set_padding (GTK_MISC (image), 2, 2);
  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
  gtk_widget_show (image);

  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  if (group)
    gtk_size_group_add_widget (group, image);

  return button;
}

static GtkWidget *
prefs_widget_add_aligned (GtkWidget    *widget,
                          const gchar  *text,
                          GtkTable     *table,
                          gint          table_row,
                          gboolean      left_align,
                          GtkSizeGroup *group)
{
  GtkWidget *label = gimp_table_attach_aligned (table, 0, table_row,
                                                text, 0.0, 0.5,
                                                widget, 1, left_align);
  if (group)
    gtk_size_group_add_widget (group, label);

  return label;
}

static GtkWidget *
prefs_color_button_add (GObject      *config,
                        const gchar  *property_name,
                        const gchar  *label,
                        const gchar  *title,
                        GtkTable     *table,
                        gint          table_row,
                        GtkSizeGroup *group)
{
  GtkWidget *button = gimp_prop_color_button_new (config, property_name,
                                                  title,
                                                  60, 24,
                                                  GIMP_COLOR_AREA_FLAT);

  if (button)
    prefs_widget_add_aligned (button, label, table, table_row, TRUE, group);

  return button;
}

static GtkWidget *
prefs_enum_combo_box_add (GObject      *config,
                          const gchar  *property_name,
                          gint          minimum,
                          gint          maximum,
                          const gchar  *label,
                          GtkTable     *table,
                          gint          table_row,
                          GtkSizeGroup *group)
{
  GtkWidget *combo = gimp_prop_enum_combo_box_new (config, property_name,
                                                   minimum, maximum);

  if (combo)
    prefs_widget_add_aligned (combo, label, table, table_row, FALSE, group);

  return combo;
}

static GtkWidget *
prefs_boolean_combo_box_add (GObject      *config,
                             const gchar  *property_name,
                             const gchar  *true_text,
                             const gchar  *false_text,
                             const gchar  *label,
                             GtkTable     *table,
                             gint          table_row,
                             GtkSizeGroup *group)
{
  GtkWidget *combo = gimp_prop_boolean_combo_box_new (config, property_name,
                                                      true_text, false_text);

  if (combo)
    prefs_widget_add_aligned (combo, label, table, table_row, FALSE, group);

  return combo;
}

static GtkWidget *
prefs_spin_button_add (GObject      *config,
                       const gchar  *property_name,
                       gdouble       step_increment,
                       gdouble       page_increment,
                       gint          digits,
                       const gchar  *label,
                       GtkTable     *table,
                       gint          table_row,
                       GtkSizeGroup *group)
{
  GtkWidget *button = gimp_prop_spin_button_new (config, property_name,
                                                 step_increment,
                                                 page_increment,
                                                 digits);

  if (button)
    prefs_widget_add_aligned (button, label, table, table_row, TRUE, group);

  return button;
}

static GtkWidget *
prefs_memsize_entry_add (GObject      *config,
                         const gchar  *property_name,
                         const gchar  *label,
                         GtkTable     *table,
                         gint          table_row,
                         GtkSizeGroup *group)
{
  GtkWidget *entry = gimp_prop_memsize_entry_new (config, property_name);

  if (entry)
    prefs_widget_add_aligned (entry, label, table, table_row, TRUE, group);

  return entry;
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

  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  checks_vbox = gtk_vbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (hbox), checks_vbox, TRUE, TRUE, 0);
  gtk_widget_show (checks_vbox);

  prefs_check_button_add (object, "show-menubar",
                          _("Show _menubar"),
                          GTK_BOX (checks_vbox));
  prefs_check_button_add (object, "show-rulers",
                          _("Show _rulers"),
                          GTK_BOX (checks_vbox));
  prefs_check_button_add (object, "show-scrollbars",
                          _("Show scroll_bars"),
                          GTK_BOX (checks_vbox));
  prefs_check_button_add (object, "show-statusbar",
                          _("Show s_tatusbar"),
                          GTK_BOX (checks_vbox));

  checks_vbox = gtk_vbox_new (FALSE, 2);
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
                                   GTK_TABLE (table), 1, NULL);
  gimp_color_panel_set_context (GIMP_COLOR_PANEL (button),
                                gimp_get_user_context (gimp));

  g_signal_connect (button, "color-changed",
                    G_CALLBACK (prefs_canvas_padding_color_changed),
                    gtk_bin_get_child (GTK_BIN (combo)));
}

static void
prefs_help_func (const gchar *help_id,
                 gpointer     help_data)
{
  GtkWidget *notebook;
  GtkWidget *event_box;
  gint       page_num;

  notebook  = g_object_get_data (G_OBJECT (help_data), "notebook");
  page_num  = gtk_notebook_get_current_page (GTK_NOTEBOOK (notebook));
  event_box = gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook), page_num);

  help_id = g_object_get_data (G_OBJECT (event_box), "gimp-help-id");
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
                                   message);

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
  GtkWidget         *tv;
  GtkTreeStore      *tree;
  GtkTreeViewColumn *column;
  GtkCellRenderer   *cell;
  GtkTreeSelection  *sel;
  GtkTreePath       *path;
  GtkTreeIter        top_iter;
  GtkTreeIter        child_iter;
  gint               page_index;

  GtkSizeGroup      *size_group = NULL;
  GtkWidget         *frame;
  GtkWidget         *ebox;
  GtkWidget         *notebook;
  GtkWidget         *vbox;
  GtkWidget         *vbox2;
  GtkWidget         *hbox;
  GtkWidget         *button;
  GtkWidget         *button2;
  GtkWidget         *table;
  GtkWidget         *label;
  GtkWidget         *image;
  GtkWidget         *entry;
  GtkWidget         *calibrate_button;
  GSList            *group;
  GtkWidget         *editor;
  gint               i;

  GObject           *object;
  GimpCoreConfig    *core_config;
  GimpDisplayConfig *display_config;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (GIMP_IS_CONFIG (config), NULL);

  object         = G_OBJECT (config);
  core_config    = GIMP_CORE_CONFIG (config);
  display_config = GIMP_DISPLAY_CONFIG (config);

  dialog = gimp_dialog_new (_("Preferences"), "preferences",
                            NULL, 0,
                            prefs_help_func,
                            GIMP_HELP_PREFS_DIALOG,

                            GIMP_STOCK_RESET, RESPONSE_RESET,
                            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                            GTK_STOCK_OK,     GTK_RESPONSE_OK,

                            NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           RESPONSE_RESET,
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (prefs_response),
                    dialog);

  /* The main hbox */
  hbox = gtk_hbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), hbox);
  gtk_widget_show (hbox);

  /* The categories tree */
  frame = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (frame),
                                       GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (frame),
                                  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  tree = gtk_tree_store_new (5,
                             GDK_TYPE_PIXBUF, G_TYPE_STRING,
                             G_TYPE_INT, G_TYPE_STRING, GDK_TYPE_PIXBUF);
  tv = gtk_tree_view_new_with_model (GTK_TREE_MODEL (tree));
  g_object_unref (tree);

  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (tv), FALSE);

  column = gtk_tree_view_column_new ();

  cell = gtk_cell_renderer_pixbuf_new ();
  gtk_tree_view_column_pack_start (column, cell, FALSE);
  gtk_tree_view_column_set_attributes (column, cell, "pixbuf", 0, NULL);

  cell = gtk_cell_renderer_text_new ();
  gtk_tree_view_column_pack_start (column, cell, TRUE);
  gtk_tree_view_column_set_attributes (column, cell, "text", 1, NULL);

  gtk_tree_view_append_column (GTK_TREE_VIEW (tv), column);

  gtk_container_add (GTK_CONTAINER (frame), tv);

  vbox = gtk_vbox_new (FALSE, 12);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  ebox = gtk_event_box_new ();
  gtk_widget_set_state (ebox, GTK_STATE_SELECTED);
  gtk_box_pack_start (GTK_BOX (vbox), ebox, FALSE, TRUE, 0);
  gtk_widget_show (ebox);

  hbox = gtk_hbox_new (FALSE, 6);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 6);
  gtk_container_add (GTK_CONTAINER (ebox), hbox);
  gtk_widget_show (hbox);

  label = gtk_label_new (NULL);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gimp_label_set_attributes (GTK_LABEL (label),
                             PANGO_ATTR_SCALE,  PANGO_SCALE_LARGE,
                             PANGO_ATTR_WEIGHT, PANGO_WEIGHT_BOLD,
                             -1);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  image = gtk_image_new ();
  gtk_box_pack_end (GTK_BOX (hbox), image, FALSE, FALSE, 0);
  gtk_widget_show (image);

  /* The main preferences notebook */
  notebook = gtk_notebook_new ();
  gtk_notebook_set_show_tabs (GTK_NOTEBOOK (notebook), FALSE);
  gtk_notebook_set_show_border (GTK_NOTEBOOK (notebook), FALSE);
  gtk_box_pack_start (GTK_BOX (vbox), notebook, TRUE, TRUE, 0);

  g_object_set_data (G_OBJECT (dialog), "notebook", notebook);

  g_object_set_data (G_OBJECT (notebook), "label", label);
  g_object_set_data (G_OBJECT (notebook), "image", image);

  sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tv));
  g_signal_connect (sel, "changed",
                    G_CALLBACK (prefs_tree_select_callback),
                    notebook);
  g_signal_connect (notebook, "switch-page",
                    G_CALLBACK (prefs_notebook_page_callback),
                    sel);

  page_index = 0;


  /*****************/
  /*  Environment  */
  /*****************/
  vbox = prefs_notebook_append_page (gimp,
                                     GTK_NOTEBOOK (notebook),
                                     _("Environment"),
                                     "environment",
                                     GTK_TREE_STORE (tree),
                                     NULL,
                                     GIMP_HELP_PREFS_ENVIRONMENT,
                                     NULL,
                                     &top_iter,
                                     page_index++);

  /* select this page in the tree */
  gtk_tree_selection_select_iter (sel, &top_iter);

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
                         _("Number of _processors to use:"),
                         GTK_TABLE (table), 4, size_group);
#endif /* ENABLE_MP */

  /*  Image Thumbnails  */
  vbox2 = prefs_frame_new (_("Image Thumbnails"), GTK_CONTAINER (vbox), FALSE);

  table = prefs_table_new (2, GTK_CONTAINER (vbox2));

  prefs_enum_combo_box_add (object, "thumbnail-size", 0, 0,
                            _("Size of _thumbnails:"),
                            GTK_TABLE (table), 0, size_group);

  prefs_memsize_entry_add (object, "thumbnail-filesize-limit",
                           _("Maximum _filesize for thumbnailing:"),
                           GTK_TABLE (table), 1, size_group);

  /*  File Saving  */
  vbox2 = prefs_frame_new (_("Saving Images"), GTK_CONTAINER (vbox), FALSE);

  prefs_check_button_add (object, "confirm-on-close",
                          _("Confirm closing of unsa_ved images"),
                          GTK_BOX (vbox2));

  g_object_unref (size_group);
  size_group = NULL;

  /*  Document History  */
  vbox2 = prefs_frame_new (_("Document History"), GTK_CONTAINER (vbox), FALSE);

  prefs_check_button_add (object, "save-document-history",
                          _("Save document _history on exit"),
                          GTK_BOX (vbox2));


  /***************/
  /*  Interface  */
  /***************/
  vbox = prefs_notebook_append_page (gimp,
                                     GTK_NOTEBOOK (notebook),
                                     _("User Interface"),
                                     "interface",
                                     GTK_TREE_STORE (tree),
                                     _("Interface"),
                                      GIMP_HELP_PREFS_INTERFACE,
                                     NULL,
                                     &top_iter,
                                     page_index++);

  /*  Previews  */
  vbox2 = prefs_frame_new (_("Previews"), GTK_CONTAINER (vbox), FALSE);

  prefs_check_button_add (object, "layer-previews",
                          _("_Enable layer & channel previews"),
                          GTK_BOX (vbox2));

  table = prefs_table_new (3, GTK_CONTAINER (vbox2));

  prefs_enum_combo_box_add (object, "layer-preview-size", 0, 0,
                            _("_Default layer & channel preview size:"),
                            GTK_TABLE (table), 0, size_group);
  prefs_enum_combo_box_add (object, "navigation-preview-size", 0, 0,
                            _("Na_vigation preview size:"),
                            GTK_TABLE (table), 1, size_group);

  /* Keyboard Shortcuts */
  vbox2 = prefs_frame_new (_("Keyboard Shortcuts"),
                           GTK_CONTAINER (vbox), FALSE);

  prefs_check_button_add (object, "menu-mnemonics",
                          _("Show menu _mnemonics (access keys)"),
                          GTK_BOX (vbox2));
  prefs_check_button_add (object, "can-change-accels",
                          _("_Use dynamic keyboard shortcuts"),
                          GTK_BOX (vbox2));

  button = prefs_button_add (GTK_STOCK_PREFERENCES,
                             _("Configure _Keyboard Shortcuts..."),
                             GTK_BOX (vbox2));
  g_signal_connect (button, "clicked",
                    G_CALLBACK (prefs_keyboard_shortcuts_dialog),
                    gimp);

  prefs_check_button_add (object, "save-accels",
                          _("_Save keyboard shortcuts on exit"),
                          GTK_BOX (vbox2));

  button = prefs_button_add (GTK_STOCK_SAVE,
                             _("Save Keyboard Shortcuts _Now"),
                             GTK_BOX (vbox2));
  g_signal_connect (button, "clicked",
                    G_CALLBACK (prefs_menus_save_callback),
                    gimp);

  button2 = prefs_button_add (GIMP_STOCK_RESET,
                              _("_Reset Keyboard Shortcuts to Default Values"),
                              GTK_BOX (vbox2));
  g_signal_connect (button2, "clicked",
                    G_CALLBACK (prefs_menus_clear_callback),
                    gimp);

  g_object_set_data (G_OBJECT (button), "clear-button", button2);

  button = prefs_button_add (GTK_STOCK_CLEAR,
                             _("Remove _All Keyboard Shortcuts"),
                             GTK_BOX (vbox2));
  g_signal_connect (button, "clicked",
                    G_CALLBACK (prefs_menus_remove_callback),
                    gimp);


  /***********/
  /*  Theme  */
  /***********/
  vbox = prefs_notebook_append_page (gimp,
                                     GTK_NOTEBOOK (notebook),
                                     _("Theme"),
                                     "theme",
                                     GTK_TREE_STORE (tree),
                                     NULL,
                                     GIMP_HELP_PREFS_THEME,
                                     NULL,
                                     &top_iter,
                                     page_index++);

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
        GtkTreeIter iter;

        gtk_list_store_append (list_store, &iter);
        gtk_list_store_set (list_store, &iter,
                            0, themes[i],
                            1, themes_get_theme_dir (gimp, themes[i]),
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

    if (themes)
      g_strfreev (themes);

    g_signal_connect (sel, "changed",
                      G_CALLBACK (prefs_theme_select_callback),
                      gimp);
  }

  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  button = prefs_button_add (GTK_STOCK_REFRESH,
                             _("Reload C_urrent Theme"),
                             GTK_BOX (hbox));
  g_signal_connect (button, "clicked",
                    G_CALLBACK (prefs_theme_reload_callback),
                    gimp);


  /*****************/
  /*  Help System  */
  /*****************/
  vbox = prefs_notebook_append_page (gimp,
                                     GTK_NOTEBOOK (notebook),
                                     _("Help System"),
                                     "help-system",
                                     GTK_TREE_STORE (tree),
                                     NULL,
                                     GIMP_HELP_PREFS_HELP,
                                     NULL,
                                     &top_iter,
                                     page_index++);

  size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  /*  General  */
  vbox2 = prefs_frame_new (_("General"), GTK_CONTAINER (vbox), FALSE);

  prefs_check_button_add (object, "show-tooltips",
                          _("Show _tooltips"),
                          GTK_BOX (vbox2));
  prefs_check_button_add (object, "show-help-button",
                          _("Show help _buttons"),
                          GTK_BOX (vbox2));
  prefs_check_button_add (object, "show-tips",
                          _("Show tips on _startup"),
                          GTK_BOX (vbox2));

  /*  Help Browser  */
  vbox2 = prefs_frame_new (_("Help Browser"), GTK_CONTAINER (vbox), FALSE);
  table = prefs_table_new (1, GTK_CONTAINER (vbox2));

  prefs_enum_combo_box_add (object, "help-browser", 0, 0,
                            _("H_elp browser to use:"),
                            GTK_TABLE (table), 0, size_group);

  /*  Web Browser  (unused on win32)  */
#ifndef G_OS_WIN32
  vbox2 = prefs_frame_new (_("Web Browser"), GTK_CONTAINER (vbox), FALSE);
  table = prefs_table_new (1, GTK_CONTAINER (vbox2));

  prefs_widget_add_aligned (gimp_prop_entry_new (object, "web-browser", 0),
                            _("_Web browser to use:"),
                            GTK_TABLE (table), 0, FALSE, size_group);
#endif

  g_object_unref (size_group);
  size_group = NULL;


  /******************/
  /*  Tool Options  */
  /******************/
  vbox = prefs_notebook_append_page (gimp,
                                     GTK_NOTEBOOK (notebook),
                                     _("Tool Options"),
                                     "tool-options",
                                     GTK_TREE_STORE (tree),
                                     NULL,
                                     GIMP_HELP_PREFS_TOOL_OPTIONS,
                                     NULL,
                                     &top_iter,
                                     page_index++);

  size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  /*  General  */
  vbox2 = prefs_frame_new (_("General"), GTK_CONTAINER (vbox), FALSE);

  prefs_check_button_add (object, "save-tool-options",
                          _("_Save tool options on exit"),
                          GTK_BOX (vbox2));

  button = prefs_button_add (GTK_STOCK_SAVE,
                             _("Save Tool Options _Now"),
                             GTK_BOX (vbox2));
  g_signal_connect (button, "clicked",
                    G_CALLBACK (prefs_tool_options_save_callback),
                    gimp);

  button2 = prefs_button_add (GIMP_STOCK_RESET,
                              _("_Reset Saved Tool Options to "
                                "Default Values"),
                              GTK_BOX (vbox2));
  g_signal_connect (button2, "clicked",
                    G_CALLBACK (prefs_tool_options_clear_callback),
                    gimp);

  g_object_set_data (G_OBJECT (button), "clear-button", button2);

  /*  Snapping Distance  */
  vbox2 = prefs_frame_new (_("Guide & Grid Snapping"),
                           GTK_CONTAINER (vbox), FALSE);
  table = prefs_table_new (1, GTK_CONTAINER (vbox2));

  prefs_spin_button_add (object, "snap-distance", 1.0, 5.0, 0,
                         _("_Snap distance:"),
                         GTK_TABLE (table), 0, size_group);

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
                                    _("_Brush"),    GIMP_STOCK_BRUSH,
                                    GTK_BOX (vbox2), size_group);
  prefs_check_button_add_with_icon (object, "global-pattern",
                                    _("_Pattern"),  GIMP_STOCK_PATTERN,
                                    GTK_BOX (vbox2), size_group);
  prefs_check_button_add_with_icon (object, "global-gradient",
                                    _("_Gradient"), GIMP_STOCK_GRADIENT,
                                    GTK_BOX (vbox2), size_group);

  vbox2 = prefs_frame_new (_("Move Tool"),
                           GTK_CONTAINER (vbox), FALSE);

  prefs_check_button_add_with_icon (object, "move-tool-changes-active",
                                    _("Set layer or path as active"),
                                    GIMP_STOCK_TOOL_MOVE,
                                    GTK_BOX (vbox2), size_group);

  g_object_unref (size_group);
  size_group = NULL;


  /*************/
  /*  Toolbox  */
  /*************/
  vbox = prefs_notebook_append_page (gimp,
                                     GTK_NOTEBOOK (notebook),
                                     _("Toolbox"),
                                     "toolbox",
                                     GTK_TREE_STORE (tree),
                                     NULL,
                                     GIMP_HELP_PREFS_TOOLBOX,
                                     NULL,
                                     &top_iter,
                                     page_index++);

  size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  /*  Appearance  */
  vbox2 = prefs_frame_new (_("Appearance"),
                           GTK_CONTAINER (vbox), FALSE);

  prefs_check_button_add_with_icon (object, "toolbox-color-area",
                                    _("Show _foreground & background color"),
                                    GIMP_STOCK_DEFAULT_COLORS,
                                    GTK_BOX (vbox2), size_group);
  prefs_check_button_add_with_icon (object, "toolbox-foo-area",
                                    _("Show active _brush, pattern & gradient"),
                                    GIMP_STOCK_BRUSH,
                                    GTK_BOX (vbox2), size_group);
  prefs_check_button_add_with_icon (object, "toolbox-image-area",
                                    _("Show active _image"),
                                    GIMP_STOCK_IMAGE,
                                    GTK_BOX (vbox2), size_group);

  g_object_unref (size_group);
  size_group = NULL;


  /***********************/
  /*  Default New Image  */
  /***********************/
  vbox = prefs_notebook_append_page (gimp,
                                     GTK_NOTEBOOK (notebook),
                                     _("Default New Image"),
                                     "new-image",
                                     GTK_TREE_STORE (tree),
                                     _("Default Image"),
                                     GIMP_HELP_PREFS_NEW_IMAGE,
                                     NULL,
                                     &top_iter,
                                     page_index++);

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



  /******************/
  /*  Default Grid  */
  /******************/
  vbox = prefs_notebook_append_page (gimp,
                                     GTK_NOTEBOOK (notebook),
                                     _("Default Image Grid"),
                                     "default-grid",
                                     GTK_TREE_STORE (tree),
                                     _("Default Grid"),
                                     GIMP_HELP_PREFS_DEFAULT_GRID,
                                     NULL,
                                     &top_iter,
                                     page_index++);

  /*  Grid  */
  editor = gimp_grid_editor_new (core_config->default_grid,
                                 gimp_get_user_context (gimp),
                                 core_config->default_image->xresolution,
                                 core_config->default_image->yresolution);

  gtk_container_add (GTK_CONTAINER (vbox), editor);
  gtk_widget_show (editor);


  /*******************/
  /*  Image Windows  */
  /*******************/
  vbox = prefs_notebook_append_page (gimp,
                                     GTK_NOTEBOOK (notebook),
                                     _("Image Windows"),
                                     "image-windows",
                                     GTK_TREE_STORE (tree),
                                     NULL,
                                     GIMP_HELP_PREFS_IMAGE_WINDOW,
                                     NULL,
                                     &top_iter,
                                     page_index++);

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
                               _("Fit to window"),
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
  prefs_enum_combo_box_add (object, "cursor-format", 0, 0,
                            _("Pointer re_ndering:"),
                            GTK_TABLE (table), 1, size_group);

  g_object_unref (size_group);
  size_group = NULL;


  /********************************/
  /*  Image Windows / Appearance  */
  /********************************/
  vbox = prefs_notebook_append_page (gimp,
                                     GTK_NOTEBOOK (notebook),
                                     _("Image Window Appearance"),
                                     "image-windows",
                                     GTK_TREE_STORE (tree),
                                     _("Appearance"),
                                     GIMP_HELP_PREFS_IMAGE_WINDOW_APPEARANCE,
                                     &top_iter,
                                     &child_iter,
                                     page_index++);

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
  vbox = prefs_notebook_append_page (gimp,
                                     GTK_NOTEBOOK (notebook),
                                     _("Image Title & Statusbar Format"),
                                     "image-title",
                                     GTK_TREE_STORE (tree),
                                     _("Title & Status"),
                                     GIMP_HELP_PREFS_IMAGE_WINDOW_TITLE,
                                     &top_iter,
                                     &child_iter,
                                     page_index++);

  {
    const gchar *format_strings[] =
    {
      NULL,
      NULL,
      "%f-%p.%i (%t) %z%%",
      "%f-%p.%i (%t) %d:%s",
      "%f-%p.%i (%t) %wx%h"
    };

    const gchar *format_names[] =
    {
      N_("Current format"),
      N_("Default format"),
      N_("Show zoom percentage"),
      N_("Show zoom ratio"),
      N_("Show image size")
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

    g_assert (G_N_ELEMENTS (format_strings) == G_N_ELEMENTS (format_names));

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


  /*************/
  /*  Display  */
  /*************/
  vbox = prefs_notebook_append_page (gimp,
                                     GTK_NOTEBOOK (notebook),
                                     _("Display"),
                                     "display",
                                     GTK_TREE_STORE (tree),
                                     NULL,
                                     GIMP_HELP_PREFS_DISPLAY,
                                     NULL,
                                     &top_iter,
                                     page_index++);

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

  hbox = gtk_hbox_new (FALSE, 0);

  gtk_box_pack_start (GTK_BOX (hbox), entry, FALSE, FALSE, 24);
  gtk_widget_show (entry);
  gtk_widget_set_sensitive (entry, ! display_config->monitor_res_from_gdk);

  group = NULL;

  {
    gdouble  xres, yres;
    gchar   *str;

    gimp_get_screen_resolution (NULL, &xres, &yres);

    str = g_strdup_printf (_("_Detect automatically (currently %d × %d ppi)"),
                           ROUND (xres), ROUND (yres));

    button = gtk_radio_button_new_with_mnemonic (group, str);

    g_free (str);
  }

  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_object_set_data (G_OBJECT (button), "monitor_resolution_sizeentry", entry);
  g_object_set_data (G_OBJECT (button), "set_sensitive", label);
  g_object_set_data (G_OBJECT (button), "inverse_sensitive", entry);

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

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  calibrate_button = gtk_button_new_with_mnemonic (_("C_alibrate..."));
  gtk_misc_set_padding (GTK_MISC (GTK_BIN (calibrate_button)->child), 4, 0);
  gtk_box_pack_start (GTK_BOX (hbox), calibrate_button, FALSE, FALSE, 0);
  gtk_widget_show (calibrate_button);
  gtk_widget_set_sensitive (calibrate_button,
                            ! display_config->monitor_res_from_gdk);

  g_object_set_data (G_OBJECT (entry), "inverse_sensitive", calibrate_button);

  g_signal_connect (calibrate_button, "clicked",
                    G_CALLBACK (prefs_resolution_calibrate_callback),
                    entry);

  g_object_unref (size_group);
  size_group = NULL;


  /**********************/
  /*  Color Management  */
  /**********************/
  vbox = prefs_notebook_append_page (gimp,
                                     GTK_NOTEBOOK (notebook),
                                     _("Color Management"),
                                     "color-management",
                                     GTK_TREE_STORE (tree),
                                     NULL,
                                     GIMP_HELP_PREFS_COLOR_MANAGEMENT,
                                     NULL,
                                     &top_iter,
                                     page_index++);

  table = prefs_table_new (9, GTK_CONTAINER (vbox));

  {
    static const struct
    {
      const gchar *label;
      const gchar *fs_label;
      const gchar *property_name;
    }
    profiles[] =
    {
      { N_("_RGB profile:"),
        N_("Select RGB Color Profile"),     "rgb-profile"     },
      { N_("_CMYK profile:"),
        N_("Select CMYK Color Profile"),    "cmyk-profile"    },
      { N_("_Monitor profile:"),
        N_("Select Monitor Color Profile"), "display-profile" },
      { N_("_Print simulation profile:"),
        N_("Select Printer Color Profile"), "printer-profile" }
    };

    GObject      *color_config;
    GtkListStore *store;
    gchar        *filename;
    gint          row = 0;

    g_object_get (object, "color-management", &color_config, NULL);

    prefs_enum_combo_box_add (color_config, "mode", 0, 0,
                              _("_Mode of operation:"),
                              GTK_TABLE (table), row++, NULL);
    gtk_table_set_row_spacing (GTK_TABLE (table), row - 1, 12);

    filename = gimp_personal_rc_file ("profilerc");
    store = gimp_color_profile_store_new (filename);
    g_free (filename);

    gimp_color_profile_store_add (GIMP_COLOR_PROFILE_STORE (store), NULL, NULL);

    for (i = 0; i < G_N_ELEMENTS (profiles); i++)
      {
        button = prefs_profile_combo_box_new (gimp,
                                              color_config,
                                              store,
                                              gettext (profiles[i].fs_label),
                                              profiles[i].property_name);

        gimp_table_attach_aligned (GTK_TABLE (table), 0, row++,
                                   gettext (profiles[i].label), 0.0, 0.5,
                                   button, 1, FALSE);


        if (i == 2) /* display profile */
          {
            button =
              gimp_prop_check_button_new (color_config,
                                          "display-profile-from-gdk",
                                          _("_Try to obtain the monitor "
                                            "profile from the windowing "
                                            "system"));

            gtk_table_attach_defaults (GTK_TABLE (table),
                                       button, 0, 2, row, row + 1);
            gtk_widget_show (button);
            row++;

            prefs_enum_combo_box_add (color_config,
                                      "display-rendering-intent", 0, 0,
                                      _("_Display rendering intent:"),
                                      GTK_TABLE (table), row++, NULL);
            gtk_table_set_row_spacing (GTK_TABLE (table), row - 1, 12);
          }

        if (i == 3) /* printer profile */
          {
            prefs_enum_combo_box_add (color_config,
                                      "simulation-rendering-intent", 0, 0,
                                      _("_Softproof rendering intent:"),
                                      GTK_TABLE (table), row++, NULL);
          }
      }

    gtk_table_set_row_spacing (GTK_TABLE (table), row - 1, 12);

    g_object_unref (store);
    g_object_unref (color_config);

    button = prefs_enum_combo_box_add (object, "color-profile-policy", 0, 0,
                                       _("File Open behaviour:"),
                                       GTK_TABLE (table), row++, NULL);
  }


  /*******************/
  /*  Input Devices  */
  /*******************/
  vbox = prefs_notebook_append_page (gimp,
                                     GTK_NOTEBOOK (notebook),
                                     _("Input Devices"),
                                     "input-devices",
                                     GTK_TREE_STORE (tree),
                                     NULL,
                                     GIMP_HELP_PREFS_INPUT_DEVICES,
                                     NULL,
                                     &top_iter,
                                     page_index++);

  /*  Extended Input Devices  */
  vbox2 = prefs_frame_new (_("Extended Input Devices"),
                           GTK_CONTAINER (vbox), FALSE);

  button = prefs_button_add (GTK_STOCK_PREFERENCES,
                             _("Configure E_xtended Input Devices..."),
                             GTK_BOX (vbox2));
  g_signal_connect (button, "clicked",
                    G_CALLBACK (prefs_input_devices_dialog),
                    gimp);

  prefs_check_button_add (object, "save-device-status",
                          _("_Save input device settings on exit"),
                          GTK_BOX (vbox2));

  button = prefs_button_add (GTK_STOCK_SAVE,
                             _("Save Input Device Settings _Now"),
                             GTK_BOX (vbox2));
  g_signal_connect (button, "clicked",
                    G_CALLBACK (prefs_devices_save_callback),
                    gimp);

  button2 = prefs_button_add (GIMP_STOCK_RESET,
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
  vbox = prefs_notebook_append_page (gimp,
                                     GTK_NOTEBOOK (notebook),
                                     _("Additional Input Controllers"),
                                     "controllers",
                                     GTK_TREE_STORE (tree),
                                     _("Input Controllers"),
                                     GIMP_HELP_PREFS_INPUT_CONTROLLERS,
                                     &top_iter,
                                     &child_iter,
                                     page_index++);

  vbox2 = gimp_controller_list_new (gimp);
  gtk_box_pack_start (GTK_BOX (vbox), vbox2, TRUE, TRUE, 0);
  gtk_widget_show (vbox2);


  /***********************/
  /*  Window Management  */
  /***********************/
  vbox = prefs_notebook_append_page (gimp,
                                     GTK_NOTEBOOK (notebook),
                                     _("Window Management"),
                                     "window-management",
                                     GTK_TREE_STORE (tree),
                                     NULL,
                                     GIMP_HELP_PREFS_WINDOW_MANAGEMENT,
                                     NULL,
                                     &top_iter,
                                     page_index++);

  vbox2 = prefs_frame_new (_("Window Manager Hints"),
                           GTK_CONTAINER (vbox), FALSE);

  table = prefs_table_new (2, GTK_CONTAINER (vbox2));

  prefs_enum_combo_box_add (object, "toolbox-window-hint", 0, 0,
                            _("Hint for the _toolbox:"),
                            GTK_TABLE (table), 0, size_group);

  prefs_enum_combo_box_add (object, "dock-window-hint", 0, 0,
                            _("Hint for other _docks:"),
                            GTK_TABLE (table), 1, size_group);

#ifdef GIMP_UNSTABLE
  prefs_check_button_add (object, "transient-docks",
                          _("Toolbox and other docks are transient "
                            "to the active image window"),
                          GTK_BOX (vbox2));
#endif

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

  button = prefs_button_add (GTK_STOCK_SAVE,
                             _("Save Window Positions _Now"),
                             GTK_BOX (vbox2));
  g_signal_connect (button, "clicked",
                    G_CALLBACK (prefs_session_save_callback),
                    gimp);

  button2 = prefs_button_add (GIMP_STOCK_RESET,
                              _("_Reset Saved Window Positions to "
                                "Default Values"),
                              GTK_BOX (vbox2));
  g_signal_connect (button2, "clicked",
                    G_CALLBACK (prefs_session_clear_callback),
                    gimp);

  g_object_set_data (G_OBJECT (button), "clear-button", button2);


  /*************/
  /*  Folders  */
  /*************/
  vbox = prefs_notebook_append_page (gimp,
                                     GTK_NOTEBOOK (notebook),
                                     _("Folders"),
                                     "folders",
                                     GTK_TREE_STORE (tree),
                                     NULL,
                                     GIMP_HELP_PREFS_FOLDERS,
                                     NULL,
                                     &top_iter,
                                     page_index++);

  {
    static const struct
    {
      const gchar *property_name;
      const gchar *label;
      const gchar *fs_label;
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
        button = gimp_prop_file_chooser_button_new (object,
                                                    dirs[i].property_name,
                                                    gettext (dirs[i].fs_label),
                                                    GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
        gimp_table_attach_aligned (GTK_TABLE (table), 0, i,
                                   gettext (dirs[i].label), 0.0, 0.5,
                                   button, 1, FALSE);
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
      { N_("Plug-Ins"), N_("Plug-In Folders"), "folders-plug-ins",
        GIMP_HELP_PREFS_FOLDERS_PLUG_INS,
        N_("Select Plug-In Folders"),
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
        "theme-path", NULL }
    };

    for (i = 0; i < G_N_ELEMENTS (paths); i++)
      {
        GtkWidget *editor;

        vbox = prefs_notebook_append_page (gimp,
                                           GTK_NOTEBOOK (notebook),
                                           gettext (paths[i].label),
                                           paths[i].icon,
                                           GTK_TREE_STORE (tree),
                                           gettext (paths[i].tree_label),
                                           paths[i].help_data,
                                           &top_iter,
                                           &child_iter,
                                           page_index++);

        editor = gimp_prop_path_editor_new (object,
                                            paths[i].path_property_name,
                                            paths[i].writable_property_name,
                                            gettext (paths[i].fs_label));
        gtk_container_add (GTK_CONTAINER (vbox), editor);
        gtk_widget_show (editor);
      }
  }

  gtk_tree_view_expand_all (GTK_TREE_VIEW (tv));

  /*  collapse the Folders subtree */
  path = gtk_tree_model_get_path (GTK_TREE_MODEL (tree), &top_iter);
  gtk_tree_view_collapse_row (GTK_TREE_VIEW (tv), path);
  gtk_tree_path_free (path);

  gtk_widget_show (tv);
  gtk_widget_show (notebook);

  return dialog;
}
