/* The GIMP -- an image manipulation program
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
#include "libgimpwidgets/gimpwidgets.h"

#include "gui-types.h"

#include "config/gimpconfig.h"
#include "config/gimpconfig-params.h"
#include "config/gimpconfig-utils.h"
#include "config/gimprc.h"

#include "core/gimp.h"
#include "core/gimptemplate.h"

#include "widgets/gimpcolorpanel.h"
#include "widgets/gimpcontainermenuimpl.h"
#include "widgets/gimpdeviceinfo.h"
#include "widgets/gimpdevices.h"
#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpgrideditor.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimppropwidgets.h"
#include "widgets/gimptemplateeditor.h"
#include "widgets/gimpwidgets-utils.h"

#include "menus.h"
#include "resolution-calibrate-dialog.h"
#include "session.h"
#include "themes.h"

#include "gimp-intl.h"


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

static void   prefs_resolution_source_callback    (GtkWidget  *widget,
                                                   GObject    *config);
static void   prefs_resolution_calibrate_callback (GtkWidget  *widget,
                                                   GtkWidget  *sizeentry);
static void   prefs_input_devices_dialog          (GtkWidget  *widget,
                                                   gpointer    user_data);
static void   prefs_input_dialog_able_callback    (GtkWidget  *widget,
                                                   GdkDevice  *device,
                                                   gpointer    data);


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
      if (param_spec->flags & GIMP_PARAM_CONFIRM)
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

  if (response_id == GTK_RESPONSE_OK)
    {
      GimpConfig *config_copy;
      GList      *restart_diff;
      GList      *confirm_diff;
      GList      *list;

      config_copy = g_object_get_data (G_OBJECT (dialog), "config-copy");

      /*  destroy config_orig  */
      g_object_set_data (G_OBJECT (dialog), "config-orig", NULL);

      gtk_widget_set_sensitive (GTK_WIDGET (dialog), FALSE);

      confirm_diff = gimp_config_diff (GIMP_CONFIG (gimp->edit_config),
                                       config_copy,
                                       GIMP_PARAM_CONFIRM);

      g_object_freeze_notify (G_OBJECT (gimp->edit_config));

      for (list = confirm_diff; list; list = g_list_next (list))
        {
          GParamSpec *param_spec;
          GValue      value = { 0, };

          param_spec = (GParamSpec *) list->data;

          g_value_init (&value, param_spec->value_type);

          g_object_get_property (G_OBJECT (config_copy),
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
      restart_diff = gimp_config_diff (GIMP_CONFIG (gimp->edit_config),
                                       GIMP_CONFIG (gimp->config),
                                       GIMP_PARAM_RESTART);

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

          g_message (string->str);

          g_string_free (string, TRUE);
        }

      g_list_free (restart_diff);
    }
  else  /* cancel */
    {
      GimpConfig *config_orig;
      GList      *diff;
      GList      *list;

      config_orig = g_object_get_data (G_OBJECT (dialog), "config-orig");

      /*  destroy config_copy  */
      g_object_set_data (G_OBJECT (dialog), "config-copy", NULL);

      gtk_widget_set_sensitive (GTK_WIDGET (dialog), FALSE);

      diff = gimp_config_diff (GIMP_CONFIG (gimp->edit_config), config_orig,
                               GIMP_PARAM_SERIALIZE);

      g_object_freeze_notify (G_OBJECT (gimp->edit_config));

      for (list = diff; list; list = g_list_next (list))
        {
          GParamSpec *param_spec;
          GValue      value = { 0, };

          param_spec = (GParamSpec *) list->data;

          g_value_init (&value, param_spec->value_type);

          g_object_get_property (G_OBJECT (config_orig),
                                 param_spec->name,
                                 &value);
          g_object_set_property (G_OBJECT (gimp->edit_config),
                                 param_spec->name,
                                 &value);

          g_value_unset (&value);
        }

      g_object_thaw_notify (G_OBJECT (gimp->edit_config));

      g_list_free (diff);
    }

  /*  enable autosaving again  */
  gimp_rc_set_autosave (GIMP_RC (gimp->edit_config), TRUE);

  gtk_widget_destroy (dialog);
}

static void
prefs_template_select_callback (GimpContainerMenu *menu,
                                GimpTemplate      *template,
                                gpointer           insert_data,
                                GimpTemplate      *edit_template)
{
  if (template)
    gimp_config_sync (GIMP_CONFIG (template), GIMP_CONFIG (edit_template), 0);
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
      GimpSizeEntry *sizeentry;

      sizeentry = g_object_get_data (G_OBJECT (widget),
                                     "monitor_resolution_sizeentry");

      if (sizeentry)
	{
	  xres = gimp_size_entry_get_refval (sizeentry, 0);
	  yres = gimp_size_entry_get_refval (sizeentry, 1);
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
				     GtkWidget *sizeentry)
{
  GtkWidget *dialog;
  GtkWidget *notebook;
  GtkWidget *image;

  dialog = gtk_widget_get_toplevel (sizeentry);

  notebook = g_object_get_data (G_OBJECT (dialog),   "notebook");
  image    = g_object_get_data (G_OBJECT (notebook), "image");

  resolution_calibrate_dialog (sizeentry,
                               gtk_image_get_pixbuf (GTK_IMAGE (image)),
                               NULL, NULL, NULL);
}

static void
prefs_input_devices_dialog (GtkWidget *widget,
                            gpointer   user_data)
{
  static GtkWidget *input_dialog = NULL;

  Gimp *gimp = GIMP (user_data);

  if (input_dialog)
    {
      gtk_window_present (GTK_WINDOW (input_dialog));
      return;
    }

  input_dialog = gtk_input_dialog_new ();

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
  gtk_notebook_append_page (notebook, event_box, NULL);
  gtk_widget_show (event_box);

  gimp_help_set_help_data (event_box, NULL, help_id);

  vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
  gtk_container_add (GTK_CONTAINER (event_box), vbox);
  gtk_widget_show (vbox);

  if (notebook_icon)
    {
      gchar *filename;

      filename = themes_get_theme_file (gimp, "images", "preferences",
                                        notebook_icon, NULL);

      if (g_file_test (filename, G_FILE_TEST_IS_REGULAR))
        pixbuf = gdk_pixbuf_new_from_file (filename, NULL);
      else
        pixbuf = NULL;

      g_free (filename);

      if (pixbuf)
        small_pixbuf = gdk_pixbuf_scale_simple (pixbuf,
                                                18, 18,
                                                GDK_INTERP_BILINEAR);
    }

  gtk_tree_store_append (tree, iter, parent);
  gtk_tree_store_set (tree, iter,
                      0, small_pixbuf,
                      1, tree_label,
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
  GValue        val = { 0, };

  if (! gtk_tree_selection_get_selected (sel, &model, &iter))
    return;

  label = g_object_get_data (G_OBJECT (notebook), "label");
  image = g_object_get_data (G_OBJECT (notebook), "image");

  gtk_tree_model_get_value (model, &iter, 3, &val);

  gtk_label_set_text (GTK_LABEL (label),
                      g_value_get_string (&val));

  g_value_unset (&val);

  gtk_tree_model_get_value (model, &iter, 4, &val);

  gtk_image_set_from_pixbuf (GTK_IMAGE (image),
                             g_value_get_object (&val));

  g_value_unset (&val);

  gtk_tree_model_get_value (model, &iter, 2, &val);

  gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook),
				 g_value_get_int (&val));

  g_value_unset (&val);
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
  const gboolean   hig_compliant = TRUE;
  GtkWidget       *frame;
  GtkWidget       *vbox;

  if (hig_compliant)
    {
      frame = gimp_frame_new (label);

      vbox = gtk_vbox_new (FALSE, 6);
      gtk_container_add (GTK_CONTAINER (frame), vbox);
      gtk_widget_show (vbox);
    }
  else
    {
      frame = gtk_frame_new (label);

      vbox = gtk_vbox_new (FALSE, 2);
      gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
      gtk_container_add (GTK_CONTAINER (frame), vbox);
      gtk_widget_show (vbox);
    }

  if (GTK_IS_BOX (parent))
    gtk_box_pack_start (GTK_BOX (parent), frame, expand, expand, 0);
  else
    gtk_container_add (parent, frame);

  gtk_widget_show (frame);

  return vbox;
}

static GtkWidget *
prefs_table_new (gint          rows,
                 GtkContainer *parent,
                 gboolean      left_align)
{
  GtkWidget *table;

  if (left_align)
    {
      GtkWidget *hbox;

      hbox = gtk_hbox_new (FALSE, 0);

      if (GTK_IS_BOX (parent))
        gtk_box_pack_start (GTK_BOX (parent), hbox, FALSE, FALSE, 0);
      else
        gtk_container_add (parent, hbox);

      gtk_widget_show (hbox);

      parent = GTK_CONTAINER (hbox);
    }

  table = gtk_table_new (rows, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 2);

  if (rows > 1)
    gtk_table_set_row_spacings (GTK_TABLE (table), 2);

  gtk_table_set_col_spacings (GTK_TABLE (table), 4);

  if (GTK_IS_BOX (parent))
    gtk_box_pack_start (GTK_BOX (parent), table, FALSE, FALSE, 0);
  else
    gtk_container_add (parent, table);

  gtk_widget_show (table);

  return table;
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
prefs_check_button_add_with_icon (GObject     *config,
                                  const gchar *property_name,
                                  const gchar *label,
                                  const gchar *stock_id,
                                  GtkBox      *vbox)
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

  return button;
}

static GtkWidget *
prefs_color_button_add (GObject     *config,
                        const gchar *property_name,
                        const gchar *label,
                        const gchar *title,
                        GtkTable    *table,
                        gint         table_row)
{
  GtkWidget *button;

  button = gimp_prop_color_button_new (config, property_name, title,
                                       20, 20, GIMP_COLOR_AREA_FLAT);

  if (button)
    gimp_table_attach_aligned (table, 0, table_row,
                               label, 1.0, 0.5,
                               button, 1, TRUE);

  return button;
}

static GtkWidget *
prefs_enum_combo_box_add (GObject     *config,
                          const gchar *property_name,
                          gint         minimum,
                          gint         maximum,
                          const gchar *label,
                          GtkTable    *table,
                          gint         table_row)
{
  GtkWidget *combo_box;

  combo_box = gimp_prop_enum_combo_box_new (config, property_name,
                                            minimum, maximum);

  if (combo_box)
    gimp_table_attach_aligned (table, 0, table_row,
                               label, 1.0, 0.5,
                               combo_box, 1, TRUE);

  return combo_box;
}

static GtkWidget *
prefs_boolean_combo_box_add (GObject     *config,
                             const gchar *property_name,
                             const gchar *true_text,
                             const gchar *false_text,
                             const gchar *label,
                             GtkTable    *table,
                             gint         table_row)
{
  GtkWidget *menu;

  menu = gimp_prop_boolean_combo_box_new (config, property_name,
                                          true_text, false_text);

  if (menu)
    gimp_table_attach_aligned (table, 0, table_row,
                               label, 1.0, 0.5,
                               menu, 1, TRUE);

  return menu;
}

static GtkWidget *
prefs_spin_button_add (GObject     *config,
                       const gchar *property_name,
                       gdouble      step_increment,
                       gdouble      page_increment,
                       gint         digits,
                       const gchar *label,
                       GtkTable    *table,
                       gint         table_row)
{
  GtkWidget *spinbutton;

  spinbutton = gimp_prop_spin_button_new (config, property_name,
                                          step_increment, page_increment,
                                          digits);

  if (spinbutton)
    gimp_table_attach_aligned (table, 0, table_row,
                               label, 1.0, 0.5,
                               spinbutton, 1, TRUE);

  return spinbutton;
}

static GtkWidget *
prefs_memsize_entry_add (GObject     *config,
                         const gchar *property_name,
                         const gchar *label,
                         GtkTable    *table,
                         gint         table_row)
{
  GtkWidget *entry;

  entry = gimp_prop_memsize_entry_new (config, property_name);

  if (entry)
    gimp_table_attach_aligned (table, 0, table_row,
                               label, 1.0, 0.5,
                               entry, 1, TRUE);

  return entry;
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
  GtkWidget *button;

  vbox = prefs_frame_new (label, parent, FALSE);

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  checks_vbox = gtk_vbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (hbox), checks_vbox, TRUE, TRUE, 0);
  gtk_widget_show (checks_vbox);

  prefs_check_button_add (object, "show-menubar",
                          _("Show _Menubar"),
                          GTK_BOX (checks_vbox));
  prefs_check_button_add (object, "show-rulers",
                          _("Show _Rulers"),
                          GTK_BOX (checks_vbox));
  prefs_check_button_add (object, "show-scrollbars",
                          _("Show Scroll_bars"),
                          GTK_BOX (checks_vbox));
  prefs_check_button_add (object, "show-statusbar",
                          _("Show S_tatusbar"),
                          GTK_BOX (checks_vbox));

  checks_vbox = gtk_vbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (hbox), checks_vbox, TRUE, TRUE, 0);
  gtk_widget_show (checks_vbox);

  prefs_check_button_add (object, "show-selection",
                          _("Show S_election"),
                          GTK_BOX (checks_vbox));
  prefs_check_button_add (object, "show-layer-boundary",
                          _("Show _Layer Boundary"),
                          GTK_BOX (checks_vbox));
  prefs_check_button_add (object, "show-guides",
                          _("Show _Guides"),
                          GTK_BOX (checks_vbox));
  prefs_check_button_add (object, "show-grid",
                          _("Show Gri_d"),
                          GTK_BOX (checks_vbox));

  table = prefs_table_new (2, GTK_CONTAINER (vbox), FALSE);

  prefs_enum_combo_box_add (object, "padding-mode", 0, 0,
                            _("Canvas Padding Mode:"), GTK_TABLE (table), 0);

  button = prefs_color_button_add (object, "padding-color",
                                   _("Custom Padding Color:"),
                                   _("Select Custom Canvas Padding Color"),
                                   GTK_TABLE (table), 1);
  gimp_color_panel_set_context (GIMP_COLOR_PANEL (button),
                                gimp_get_user_context (gimp));
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
  GtkTreeIter        top_iter;
  GtkTreeIter        child_iter;
  GtkTreeIter        grandchild_iter;
  gint               page_index;

  GtkWidget         *ebox;
  GtkWidget         *frame;
  GtkWidget         *notebook;
  GtkWidget         *vbox;
  GtkWidget         *vbox2;
  GtkWidget         *hbox;
  GtkWidget         *abox;
  GtkWidget         *button;
  GtkWidget         *fileselection;
  GtkWidget         *patheditor;
  GtkWidget         *table;
  GtkWidget         *label;
  GtkWidget         *image;
  GtkWidget         *sizeentry;
  GtkWidget         *separator;
  GtkWidget         *calibrate_button;
  PangoAttrList     *attrs;
  PangoAttribute    *attr;
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

                            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                            GTK_STOCK_OK,     GTK_RESPONSE_OK,

                            NULL);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (prefs_response),
                    dialog);

  /* The main hbox */
  hbox = gtk_hbox_new (FALSE, 8);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 8);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), hbox);
  gtk_widget_show (hbox);

  /* The categories tree */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
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

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  ebox = gtk_event_box_new ();
  gtk_widget_set_state (ebox, GTK_STATE_PRELIGHT);
  gtk_box_pack_start (GTK_BOX (vbox), ebox, FALSE, TRUE, 0);
  gtk_widget_show (ebox);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
  gtk_container_add (GTK_CONTAINER (ebox), frame);
  gtk_widget_show (frame);

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 4);
  gtk_container_add (GTK_CONTAINER (frame), hbox);
  gtk_widget_show (hbox);

  label = gtk_label_new (NULL);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  attrs = pango_attr_list_new ();

  attr = pango_attr_scale_new (PANGO_SCALE_LARGE);
  attr->start_index = 0;
  attr->end_index   = -1;
  pango_attr_list_insert (attrs, attr);

  attr = pango_attr_weight_new (PANGO_WEIGHT_BOLD);
  attr->start_index = 0;
  attr->end_index   = -1;
  pango_attr_list_insert (attrs, attr);

  gtk_label_set_attributes (GTK_LABEL (label), attrs);
  pango_attr_list_unref (attrs);

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

  page_index = 0;


  /***************/
  /*  New Image  */
  /***************/
  vbox = prefs_notebook_append_page (gimp,
                                     GTK_NOTEBOOK (notebook),
				     _("New Image"),
                                     "new-image.png",
				     GTK_TREE_STORE (tree),
				     _("New Image"),
				     GIMP_HELP_PREFS_NEW_IMAGE,
				     NULL,
				     &top_iter,
				     page_index++);

  /* select this page in the tree */
  gtk_tree_selection_select_iter (sel, &top_iter);

  table = prefs_table_new (1, GTK_CONTAINER (vbox), TRUE);

  {
    GtkWidget *optionmenu;
    GtkWidget *menu;

    optionmenu = gtk_option_menu_new ();
    gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                               _("From _Template:"),  1.0, 0.5,
                               optionmenu, 1, FALSE);

    menu = gimp_container_menu_new (gimp->templates, NULL, 16, 0);
    gtk_option_menu_set_menu (GTK_OPTION_MENU (optionmenu), menu);
    gtk_widget_show (menu);

    gimp_container_menu_select_item (GIMP_CONTAINER_MENU (menu), NULL);

    g_signal_connect (menu, "select_item",
                      G_CALLBACK (prefs_template_select_callback),
                      core_config->default_image);
  }

  editor = gimp_template_editor_new (core_config->default_image, gimp, FALSE);
  gtk_box_pack_start (GTK_BOX (vbox), editor, TRUE, TRUE, 0);
  gtk_widget_show (editor);

  table = prefs_table_new (1, GTK_CONTAINER (vbox), TRUE);
  prefs_memsize_entry_add (object, "max-new-image-size",
                           _("Maximum New Image Size:"),
                           GTK_TABLE (table), 1);


  /******************/
  /*  Default Grid  */
  /******************/
  vbox = prefs_notebook_append_page (gimp,
                                     GTK_NOTEBOOK (notebook),
				     _("Default Image Grid"),
                                     "default-grid.png",
				     GTK_TREE_STORE (tree),
				     _("Default Grid"),
				     GIMP_HELP_PREFS_DEFAULT_GRID,
				     NULL,
				     &top_iter,
				     page_index++);

  /*  Grid  */
  editor = gimp_grid_editor_new (core_config->default_grid,
                                 core_config->default_image->xresolution,
                                 core_config->default_image->yresolution);

  gtk_container_add (GTK_CONTAINER (vbox), editor);
  gtk_widget_show (editor);


  /***************/
  /*  Interface  */
  /***************/
  vbox = prefs_notebook_append_page (gimp,
                                     GTK_NOTEBOOK (notebook),
				     _("User Interface"),
                                     "interface.png",
				     GTK_TREE_STORE (tree),
				     _("Interface"),
				      GIMP_HELP_PREFS_INTERFACE,
				     NULL,
				     &top_iter,
				     page_index++);

  /*  Previews  */
  vbox2 = prefs_frame_new (_("Previews"), GTK_CONTAINER (vbox), FALSE);

  prefs_check_button_add (object, "layer-previews",
                          _("_Enable Layer & Channel Previews"),
                          GTK_BOX (vbox2));

  table = prefs_table_new (3, GTK_CONTAINER (vbox2), FALSE);

  prefs_enum_combo_box_add (object, "layer-preview-size", 0, 0,
                            _("Default _Layer & Channel Preview Size:"),
                            GTK_TABLE (table), 0);
  prefs_enum_combo_box_add (object, "navigation-preview-size", 0, 0,
                            _("_Navigation Preview Size:"),
                            GTK_TABLE (table), 1);
  prefs_enum_combo_box_add (object, "undo-preview-size", 0, 0,
                            _("_Undo History Preview Size:"),
                            GTK_TABLE (table), 2);

  /* Dialog Bahavior */
  vbox2 = prefs_frame_new (_("Dialog Behavior"), GTK_CONTAINER (vbox), FALSE);

  prefs_check_button_add (object, "info-window-per-display",
                          _("_Info Window Per Display"),
                          GTK_BOX (vbox2));

  /* Menus */
  vbox2 = prefs_frame_new (_("Menus"), GTK_CONTAINER (vbox), FALSE);

  prefs_check_button_add (object, "tearoff-menus",
                          _("Enable _Tearoff Menus"),
                          GTK_BOX (vbox2));

  table = prefs_table_new (1, GTK_CONTAINER (vbox2), FALSE);

  prefs_spin_button_add (object, "last-opened-size", 1.0, 5.0, 0,
                         _("Open _Recent Menu Size:"),
                         GTK_TABLE (table), 3);

  /* Keyboard Shortcuts */
  vbox2 = prefs_frame_new (_("Keyboard Shortcuts"),
                           GTK_CONTAINER (vbox), FALSE);

  prefs_check_button_add (object, "can-change-accels",
                          _("Use Dynamic _Keyboard Shortcuts"),
                          GTK_BOX (vbox2));

  /* Themes */
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
            gtk_tree_selection_select_iter (sel, &iter);
          }
      }

    if (themes)
      g_strfreev (themes);

    g_signal_connect (sel, "changed",
                      G_CALLBACK (prefs_theme_select_callback),
                      gimp);
  }

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  button = gtk_button_new_with_mnemonic (_("Reload C_urrent Theme"));
  gtk_misc_set_padding (GTK_MISC (GTK_BIN (button)->child), 4, 0);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (prefs_theme_reload_callback),
                    gimp);


  /*****************************/
  /*  Interface / Help System  */
  /*****************************/
  vbox = prefs_notebook_append_page (gimp,
                                     GTK_NOTEBOOK (notebook),
                                     _("Help System"),
                                     "help-system.png",
				     GTK_TREE_STORE (tree),
				     _("Help System"),
				     GIMP_HELP_PREFS_HELP,
				     &top_iter,
				     &child_iter,
				     page_index++);

  /*  General  */
  vbox2 = prefs_frame_new (_("General"), GTK_CONTAINER (vbox), FALSE);

  prefs_check_button_add (object, "show-tool-tips",
                          _("Show Tool _Tips"),
                          GTK_BOX (vbox2));
  prefs_check_button_add (object, "use-help",
                          _("Context Sensitive _Help with \"F1\""),
                          GTK_BOX (vbox2));
  prefs_check_button_add (object, "show-tips",
                          _("Show Tips on _Startup"),
                          GTK_BOX (vbox2));

  /*  Help Browser  */
  vbox2 = prefs_frame_new (_("Help Browser"), GTK_CONTAINER (vbox), FALSE);
  table = prefs_table_new (1, GTK_CONTAINER (vbox2), FALSE);

  prefs_enum_combo_box_add (object, "help-browser", 0, 0,
                            _("Help _Browser to Use:"),
                            GTK_TABLE (table), 0);

  /*  Web Browser  (unused on win32)  */
#ifndef G_OS_WIN32
  vbox2 = prefs_frame_new (_("Web Browser"), GTK_CONTAINER (vbox), FALSE);
  table = prefs_table_new (1, GTK_CONTAINER (vbox2), FALSE);

  fileselection = gimp_prop_file_entry_new (object, "web-browser",
                                            _("Select Web Browser"),
                                            FALSE, FALSE);

  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                             _("Web Browser to Use:"), 1.0, 0.5,
                             fileselection, 1, TRUE);
#endif

  /******************************/
  /*  Interface / Tool Options  */
  /******************************/
  vbox = prefs_notebook_append_page (gimp,
                                     GTK_NOTEBOOK (notebook),
                                     _("Tool Options"),
                                     "tool-options.png",
				     GTK_TREE_STORE (tree),
				     _("Tool Options"),
				     GIMP_HELP_PREFS_TOOL_OPTIONS,
				     &top_iter,
				     &child_iter,
				     page_index++);

  /*  Snapping Distance  */
  vbox2 = prefs_frame_new (_("Guide and Grid Snapping"),
                           GTK_CONTAINER (vbox), FALSE);
  table = prefs_table_new (1, GTK_CONTAINER (vbox2), FALSE);

  prefs_spin_button_add (object, "snap-distance", 1.0, 5.0, 0,
                         _("_Snap Distance:"),
                         GTK_TABLE (table), 0);

  /*  Contiguous Regions  */
  vbox2 = prefs_frame_new (_("Finding Contiguous Regions"),
                           GTK_CONTAINER (vbox), FALSE);
  table = prefs_table_new (1, GTK_CONTAINER (vbox2), FALSE);

  prefs_spin_button_add (object, "default-threshold", 1.0, 5.0, 0,
                         _("Default _Threshold:"),
                         GTK_TABLE (table), 0);

  /*  Scaling  */
  vbox2 = prefs_frame_new (_("Scaling"), GTK_CONTAINER (vbox), FALSE);
  table = prefs_table_new (1, GTK_CONTAINER (vbox2), TRUE);

  prefs_enum_combo_box_add (object, "interpolation-type", 0, 0,
                            _("Default _Interpolation:"),
                            GTK_TABLE (table), 0);

  /*  Global Brush, Pattern, ...  */
  vbox2 = prefs_frame_new (_("Paint Options Shared Between Tools"),
                           GTK_CONTAINER (vbox), FALSE);

  prefs_check_button_add_with_icon (object, "global-brush",
                                    _("_Brush"),    GIMP_STOCK_BRUSH,
                                    GTK_BOX (vbox2));
  prefs_check_button_add_with_icon (object, "global-pattern",
                                    _("_Pattern"),  GIMP_STOCK_PATTERN,
                                    GTK_BOX (vbox2));
  prefs_check_button_add_with_icon (object, "global-gradient",
                                    _("_Gradient"), GIMP_STOCK_GRADIENT,
                                    GTK_BOX (vbox2));


  /*******************************/
  /*  Interface / Input Devices  */
  /*******************************/
  vbox = prefs_notebook_append_page (gimp,
                                     GTK_NOTEBOOK (notebook),
				     _("Input Devices"),
                                     "input-devices.png",
				     GTK_TREE_STORE (tree),
				     _("Input Devices"),
				     GIMP_HELP_PREFS_INPUT_DEVICES,
				     &top_iter,
				     &child_iter,
				     page_index++);

  /*  Input Device Settings  */
  vbox2 = prefs_frame_new (_("Extended Input Devices"),
                           GTK_CONTAINER (vbox), FALSE);

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 4);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  button = gtk_button_new_with_label (_("Configure Extended Input Devices"));
  gtk_misc_set_padding (GTK_MISC (GTK_BIN (button)->child), 2, 0);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (prefs_input_devices_dialog),
                    gimp);


  /*******************************/
  /*  Interface / Image Windows  */
  /*******************************/
  vbox = prefs_notebook_append_page (gimp,
                                     GTK_NOTEBOOK (notebook),
				     _("Image Windows"),
                                     "image-windows.png",
				     GTK_TREE_STORE (tree),
				     _("Image Windows"),
				     GIMP_HELP_PREFS_IMAGE_WINDOW,
				     &top_iter,
				     &child_iter,
				     page_index++);

  /*  General  */
  vbox2 = prefs_frame_new (_("General"), GTK_CONTAINER (vbox), FALSE);

  prefs_check_button_add (object, "default-dot-for-dot",
                          _("Use \"_Dot for Dot\" by default"),
                          GTK_BOX (vbox2));

  table = prefs_table_new (1, GTK_CONTAINER (vbox2), FALSE);

  prefs_spin_button_add (object, "marching-ants-speed", 10.0, 100.0, 0,
                         _("Marching _Ants Speed:"),
                         GTK_TABLE (table), 0);

  /*  Zoom & Resize Behavior  */
  vbox2 = prefs_frame_new (_("Zoom & Resize Behavior"),
                           GTK_CONTAINER (vbox), FALSE);

  prefs_check_button_add (object, "resize-windows-on-zoom",
                          _("Resize Window on _Zoom"),
                          GTK_BOX (vbox2));
  prefs_check_button_add (object, "resize-windows-on-resize",
                          _("Resize Window on Image _Size Change"),
                          GTK_BOX (vbox2));

  table = prefs_table_new (1, GTK_CONTAINER (vbox2), FALSE);

  prefs_boolean_combo_box_add (object, "initial-zoom-to-fit",
                               _("Fit to Window"),
                               "1:1",
                               _("Inital Zoom Ratio:"),
                               GTK_TABLE (table), 0);

  /*  Pointer Movement Feedback  */
  vbox2 = prefs_frame_new (_("Pointer Movement Feedback"),
                           GTK_CONTAINER (vbox), FALSE);

  prefs_check_button_add (object, "show-brush-outline",
                          _("Show _Brush Outline"),
                          GTK_BOX (vbox2));
  prefs_check_button_add (object, "perfect-mouse",
                          _("Perfect-but-Slow _Pointer Tracking"),
                          GTK_BOX (vbox2));
  prefs_check_button_add (object, "cursor-updating",
                          _("Enable Cursor _Updating"),
                          GTK_BOX (vbox2));

  table = prefs_table_new (1, GTK_CONTAINER (vbox2), FALSE);

  prefs_enum_combo_box_add (object, "cursor-mode", 0, 0,
                            _("Cursor M_ode:"),
                            GTK_TABLE (table), 0);


  /********************************************/
  /*  Interface / Image Windows / Appearance  */
  /********************************************/
  vbox = prefs_notebook_append_page (gimp,
                                     GTK_NOTEBOOK (notebook),
				     _("Image Window Appearance"),
                                     "image-windows.png",
				     GTK_TREE_STORE (tree),
				     _("Appearance"),
				     GIMP_HELP_PREFS_IMAGE_WINDOW_APPEARANCE,
				     &child_iter,
				     &grandchild_iter,
				     page_index++);

  prefs_display_options_frame_add (gimp,
                                   G_OBJECT (display_config->default_view),
                                   _("Default Appearance in Normal Mode"),
                                   GTK_CONTAINER (vbox));

  prefs_display_options_frame_add (gimp,
                                   G_OBJECT (display_config->default_fullscreen_view),
                                   _("Default Appearance in Fullscreen Mode"),
                                   GTK_CONTAINER (vbox));


  /****************************************************************/
  /*  Interface / Image Windows / Image Title & Statusbar Format  */
  /****************************************************************/
  vbox = prefs_notebook_append_page (gimp,
                                     GTK_NOTEBOOK (notebook),
				     _("Image Title & Statusbar Format"),
                                     "image-title.png",
				     GTK_TREE_STORE (tree),
				     _("Title & Status"),
				     GIMP_HELP_PREFS_IMAGE_WINDOW_TITLE,
				     &child_iter,
				     &grandchild_iter,
				     page_index++);

  {
    const gchar *format_strings[] =
    {
      NULL,
      "%f-%p.%i (%t)",
      "%f-%p.%i (%t) %z%%",
      "%f-%p.%i (%t) %d:%s",
      "%f-%p.%i (%t) %s:%d",
      "%f-%p.%i (%t) %m"
    };

    const gchar *format_names[] =
    {
      N_("Custom"),
      N_("Standard"),
      N_("Show zoom percentage"),
      N_("Show zoom ratio"),
      N_("Show reversed zoom ratio"),
      N_("Show memory usage")
    };

    struct
    {
      gchar       *current_setting;
      const gchar *title;
      const gchar *property_name;
    }
    formats[] =
    {
      { NULL, N_("Image Title Format"),     "image-title-format"  },
      { NULL, N_("Image Statusbar Format"), "image-status-format" }
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
        GtkWidget        *entry;
        GtkTreeSelection *sel;
        gint              i;

        format_strings[0] = formats[format].current_setting;

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
                                0, format_names[i],
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


  /*************************/
  /*  Interface / Display  */
  /*************************/
  vbox = prefs_notebook_append_page (gimp,
                                     GTK_NOTEBOOK (notebook),
				     _("Display"),
                                     "display.png",
				     GTK_TREE_STORE (tree),
				     _("Display"),
				     GIMP_HELP_PREFS_DISPLAY,
				     &top_iter,
				     &child_iter,
				     page_index++);

  /*  Transparency  */
  vbox2 = prefs_frame_new (_("Transparency"), GTK_CONTAINER (vbox), FALSE);
  table = prefs_table_new (2, GTK_CONTAINER (vbox2), TRUE);

  prefs_enum_combo_box_add (object, "transparency-type", 0, 0,
                            _("Transparency _Type:"),
                            GTK_TABLE (table), 0);
  prefs_enum_combo_box_add (object, "transparency-size", 0, 0,
                            _("Check _Size:"),
                            GTK_TABLE (table), 1);

  /*  8-Bit Displays  */
  vbox2 = prefs_frame_new (_("8-Bit Displays"), GTK_CONTAINER (vbox), FALSE);

  /*  disabling this entry is not multi-head safe  */
#if 0
  if (gdk_rgb_get_visual ()->depth != 8)
    gtk_widget_set_sensitive (GTK_WIDGET (vbox2->parent), FALSE);
#endif

  table = prefs_table_new (1, GTK_CONTAINER (vbox2), FALSE);

  prefs_spin_button_add (object, "min-colors", 1.0, 8.0, 0,
                         _("Minimum Number of Colors:"),
                         GTK_TABLE (table), 0);
  prefs_check_button_add (object, "install-colormap",
                          _("Install Colormap"),
                          GTK_BOX (vbox2));


  /*************************/
  /*  Interface / Monitor  */
  /*************************/
  vbox = prefs_notebook_append_page (gimp,
                                     GTK_NOTEBOOK (notebook),
				     _("Monitor Resolution"),
                                     "monitor.png",
				     GTK_TREE_STORE (tree),
				     _("Monitor"),
				     GIMP_HELP_PREFS_MONITOR,
				     &top_iter,
				     &child_iter,
				     page_index++);

  vbox2 = prefs_frame_new (_("Get Monitor Resolution"),
                           GTK_CONTAINER (vbox), FALSE);

  {
    gdouble  xres, yres;
    gchar   *str;

    gimp_get_screen_resolution (NULL, &xres, &yres);

    str = g_strdup_printf (_("(Currently %d x %d dpi)"),
			   ROUND (xres), ROUND (yres));
    label = gtk_label_new (str);
    g_free (str);
  }

  abox = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);

  {
    gchar *pixels_per_unit = g_strconcat (_("Pixels"), "/%s", NULL);

    sizeentry = gimp_prop_coordinates_new (object,
                                           "monitor-xresolution",
                                           "monitor-yresolution",
                                           NULL,
                                           pixels_per_unit,
                                           GIMP_SIZE_ENTRY_UPDATE_RESOLUTION,
                                           0.0, 0.0,
                                           TRUE);

    g_free (pixels_per_unit);
  }

  gtk_table_set_col_spacings (GTK_TABLE (sizeentry), 2);
  gtk_table_set_row_spacings (GTK_TABLE (sizeentry), 2);

  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (sizeentry),
				_("Horizontal"), 0, 1, 0.0);
  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (sizeentry),
				_("Vertical"), 0, 2, 0.0);
  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (sizeentry),
				_("dpi"), 1, 4, 0.0);

  gtk_container_add (GTK_CONTAINER (abox), sizeentry);
  gtk_widget_show (sizeentry);
  gtk_widget_set_sensitive (sizeentry, ! display_config->monitor_res_from_gdk);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 4);

  calibrate_button = gtk_button_new_with_mnemonic (_("C_alibrate"));
  gtk_misc_set_padding (GTK_MISC (GTK_BIN (calibrate_button)->child), 4, 0);
  gtk_box_pack_start (GTK_BOX (hbox), calibrate_button, FALSE, FALSE, 0);
  gtk_widget_show (calibrate_button);
  gtk_widget_set_sensitive (calibrate_button,
                            ! display_config->monitor_res_from_gdk);

  g_signal_connect (calibrate_button, "clicked",
		    G_CALLBACK (prefs_resolution_calibrate_callback),
		    sizeentry);

  group = NULL;

  button = gtk_radio_button_new_with_mnemonic (group,
                                               _("From _Windowing System"));
  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_object_set_data (G_OBJECT (button), "monitor_resolution_sizeentry",
                     sizeentry);
  g_object_set_data (G_OBJECT (button), "set_sensitive",
                     label);
  g_object_set_data (G_OBJECT (button), "inverse_sensitive",
                     sizeentry);
  g_object_set_data (G_OBJECT (sizeentry), "inverse_sensitive",
                     calibrate_button);

  g_signal_connect (button, "toggled",
		    G_CALLBACK (prefs_resolution_source_callback),
		    config);

  gtk_box_pack_start (GTK_BOX (vbox2), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  separator = gtk_hseparator_new ();
  gtk_box_pack_start (GTK_BOX (vbox2), separator, FALSE, FALSE, 0);
  gtk_widget_show (separator);

  button = gtk_radio_button_new_with_mnemonic (group, _("_Manually"));
  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  gtk_box_pack_start (GTK_BOX (vbox2), abox, FALSE, FALSE, 0);
  gtk_widget_show (abox);

  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  if (! display_config->monitor_res_from_gdk)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);


  /***********************/
  /*  Window Management  */
  /***********************/
  vbox = prefs_notebook_append_page (gimp,
                                     GTK_NOTEBOOK (notebook),
				     _("Window Management"),
                                     "window-management.png",
				     GTK_TREE_STORE (tree),
				     _("Window Management"),
				     GIMP_HELP_PREFS_WINDOW_MANAGEMENT,
				     NULL,
				     &top_iter,
				     page_index++);

  vbox2 = prefs_frame_new (_("Window Type Hints"),
                           GTK_CONTAINER (vbox), FALSE);

  table = prefs_table_new (2, GTK_CONTAINER (vbox2), FALSE);

  prefs_enum_combo_box_add (object, "toolbox-window-hint", 0, 0,
                            _("Window Type Hint for the _Toolbox:"),
                            GTK_TABLE (table), 0);

  prefs_enum_combo_box_add (object, "dock-window-hint", 0, 0,
                            _("Window Type Hint for the _Docks:"),
                            GTK_TABLE (table), 1);

  vbox2 = prefs_frame_new (_("Focus"),
                           GTK_CONTAINER (vbox), FALSE);

  prefs_check_button_add (object, "activate-on-focus",
                          _("Activate the _Focused Image"),
                          GTK_BOX (vbox2));


  /*****************/
  /*  Environment  */
  /*****************/
  vbox = prefs_notebook_append_page (gimp,
                                     GTK_NOTEBOOK (notebook),
				     _("Environment"),
                                     "environment.png",
				     GTK_TREE_STORE (tree),
				     _("Environment"),
				     GIMP_HELP_PREFS_ENVIRONMENT,
				     NULL,
				     &top_iter,
				     page_index++);

  vbox2 = prefs_frame_new (_("Resource Consumption"),
                           GTK_CONTAINER (vbox), FALSE);

#ifdef ENABLE_MP
  table = prefs_table_new (4, GTK_CONTAINER (vbox2), FALSE);
#else
  table = prefs_table_new (3, GTK_CONTAINER (vbox2), FALSE);
#endif /* ENABLE_MP */

  prefs_spin_button_add (object, "undo-levels", 1.0, 5.0, 0,
                         _("Minimal Number of Undo Levels:"),
                         GTK_TABLE (table), 0);
  prefs_memsize_entry_add (object, "undo-size",
                           _("Maximum Undo Memory:"),
                           GTK_TABLE (table), 1);
  prefs_memsize_entry_add (object, "tile-cache-size",
                           _("Tile Cache Size:"),
                           GTK_TABLE (table), 2);

#ifdef ENABLE_MP
  prefs_spin_button_add (object, "num-processors", 1.0, 4.0, 0,
                         _("Number of Processors to Use:"),
                         GTK_TABLE (table), 3);
#endif /* ENABLE_MP */

  /*  Resource Consumption  */
  prefs_check_button_add (object, "stingy-memory-use",
                          _("Conservative Memory Usage"),
                          GTK_BOX (vbox2));

  /*  File Saving  */
  vbox2 = prefs_frame_new (_("File Saving"), GTK_CONTAINER (vbox), FALSE);

  prefs_check_button_add (object, "confirm-on-close",
                          _("Confirm Closing of Unsaved Images"),
                          GTK_BOX (vbox2));

  table = prefs_table_new (2, GTK_CONTAINER (vbox2), TRUE);

  prefs_boolean_combo_box_add (object, "trust-dirty-flag",
                               _("Only when Modified"),
                               _("Always"),
                               _("\"File -> Save\" Saves the Image:"),
                               GTK_TABLE (table), 0);
  prefs_enum_combo_box_add (object, "thumbnail-size", 0, 0,
                            _("Size of Thumbnail Files:"),
                            GTK_TABLE (table), 1);


  /************************/
  /*  Session Management  */
  /************************/
  vbox = prefs_notebook_append_page (gimp,
                                     GTK_NOTEBOOK (notebook),
				     _("Session Management"),
                                     "session.png",
				     GTK_TREE_STORE (tree),
				     _("Session"),
				     GIMP_HELP_PREFS_SESSION,
				     NULL,
				     &top_iter,
				     page_index++);

  /* Window Positions */
  vbox2 = prefs_frame_new (_("Window Positions"), GTK_CONTAINER (vbox), FALSE);

  prefs_check_button_add (object, "save-session-info",
                          _("_Save Window Positions on Exit"),
                          GTK_BOX (vbox2));
  prefs_check_button_add (object, "restore-session",
                          _("R_estore Saved Window Positions on Start-up"),
                          GTK_BOX (vbox2));

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 4);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  button = gtk_button_new_with_label (_("Save Window Positions Now"));
  gtk_misc_set_padding (GTK_MISC (GTK_BIN (button)->child), 2, 0);
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  g_signal_connect_swapped (button, "clicked",
                            G_CALLBACK (session_save),
                            gimp);

  button = gtk_button_new_with_label (_("Clear Saved Window Positions Now"));
  gtk_misc_set_padding (GTK_MISC (GTK_BIN (button)->child), 2, 0);
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  g_signal_connect_swapped (button, "clicked",
                            G_CALLBACK (session_clear),
                            gimp);

  /* Keyboard Shortcuts */
  vbox2 = prefs_frame_new (_("Keyboard Shortcuts"), GTK_CONTAINER (vbox), FALSE);

  prefs_check_button_add (object, "save-accels",
                          _("Save Keyboard Shortcuts on Exit"),
                          GTK_BOX (vbox2));
  prefs_check_button_add (object, "restore-accels",
                          _("Restore Saved Keyboard Shortcuts on Start-up"),
                          GTK_BOX (vbox2));

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 4);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  button = gtk_button_new_with_label (_("Save Keyboard Shortcuts Now"));
  gtk_misc_set_padding (GTK_MISC (GTK_BIN (button)->child), 2, 0);
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  g_signal_connect_swapped (button, "clicked",
                            G_CALLBACK (menus_save),
                            gimp);

  button = gtk_button_new_with_label (_("Clear Saved Keyboard Shortcuts Now"));
  gtk_misc_set_padding (GTK_MISC (GTK_BIN (button)->child), 2, 0);
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  g_signal_connect_swapped (button, "clicked",
                            G_CALLBACK (menus_clear),
                            gimp);

  /* Input Devices */
  vbox2 = prefs_frame_new (_("Input Devices"), GTK_CONTAINER (vbox), FALSE);

  prefs_check_button_add (object, "save-device-status",
                          _("Save Input Device Settings on Exit"),
                          GTK_BOX (vbox2));

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 4);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  button = gtk_button_new_with_label (_("Save Input Device Settings Now"));
  gtk_misc_set_padding (GTK_MISC (GTK_BIN (button)->child), 2, 0);
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  g_signal_connect_swapped (button, "clicked",
                            G_CALLBACK (gimp_devices_save),
                            gimp);

  button = gtk_button_new_with_label (_("Clear Saved Input Device Settings Now"));
  gtk_misc_set_padding (GTK_MISC (GTK_BIN (button)->child), 2, 0);
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  g_signal_connect_swapped (button, "clicked",
                            G_CALLBACK (gimp_devices_clear),
                            gimp);


  /*************/
  /*  Folders  */
  /*************/
  vbox = prefs_notebook_append_page (gimp,
                                     GTK_NOTEBOOK (notebook),
				     _("Folders"),
                                     "folders.png",
				     GTK_TREE_STORE (tree),
				     _("Folders"),
				     GIMP_HELP_PREFS_FOLDERS,
				     NULL,
				     &top_iter,
				     page_index++);

  {
    static const struct
    {
      const gchar *label;
      const gchar *fs_label;
      const gchar *property_name;
    }
    dirs[] =
    {
      { N_("Temp Dir:"), N_("Select Temp Dir"), "temp-path" },
      { N_("Swap Dir:"), N_("Select Swap Dir"), "swap-path" },
    };

    table = prefs_table_new (G_N_ELEMENTS (dirs) + 1,
                             GTK_CONTAINER (vbox), FALSE);

    for (i = 0; i < G_N_ELEMENTS (dirs); i++)
      {
	fileselection = gimp_prop_file_entry_new (object, dirs[i].property_name,
                                                  gettext (dirs[i].fs_label),
                                                  TRUE, TRUE);
	gimp_table_attach_aligned (GTK_TABLE (table), 0, i,
				   gettext (dirs[i].label), 1.0, 0.5,
				   fileselection, 1, FALSE);
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
      { N_("Brushes"), N_("Brush Folders"), "folders-brushes.png",
	GIMP_HELP_PREFS_FOLDERS_BRUSHES,
	N_("Select Brush Folders"),
	"brush-path", "brush-path-writable" },
      { N_("Patterns"), N_("Pattern Folders"), "folders-patterns.png",
	GIMP_HELP_PREFS_FOLDERS_PATTERNS,
	N_("Select Pattern Folders"),
	"pattern-path", "pattern-path-writable" },
      { N_("Palettes"), N_("Palette Folders"), "folders-palettes.png",
	GIMP_HELP_PREFS_FOLDERS_PALETTES,
	N_("Select Palette Folders"),
        "palette-path", "palette-path-writable" },
      { N_("Gradients"), N_("Gradient Folders"), "folders-gradients.png",
	GIMP_HELP_PREFS_FOLDERS_GRADIENTS,
	N_("Select Gradient Folders"),
        "gradient-path", "gradient-path-writable" },
      { N_("Fonts"), N_("Font Folders"), "folders-fonts.png",
	GIMP_HELP_PREFS_FOLDERS_FONTS,
	N_("Select Font Folders"),
        "font-path", "font-path-writable" },
      { N_("Plug-Ins"), N_("Plug-In Folders"), "folders-plug-ins.png",
	GIMP_HELP_PREFS_FOLDERS_PLUG_INS,
	N_("Select Plug-In Folders"),
        "plug-in-path", NULL },
      { N_("Scripts"), N_("Script-Fu Folders"), "folders-scripts.png",
	GIMP_HELP_PREFS_FOLDERS_SCRIPTS,
	N_("Select Script-Fu Folders"),
        "script-fu-path", NULL },
      { N_("Modules"), N_("Module Folders"), "folders-modules.png",
	GIMP_HELP_PREFS_FOLDERS_MODULES,
	N_("Select Module Folders"),
        "module-path", NULL },
      { N_("Environment"), N_("Environment Folders"), "folders-environ.png",
	GIMP_HELP_PREFS_FOLDERS_ENVIRONMENT,
	N_("Select Environment Folders"),
        "environ-path", NULL },
      { N_("Themes"), N_("Theme Folders"), "folders-themes.png",
	GIMP_HELP_PREFS_FOLDERS_THEMES,
	N_("Select Theme Folders"),
        "theme-path", NULL }
    };

    for (i = 0; i < G_N_ELEMENTS (paths); i++)
      {
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

	patheditor = gimp_prop_path_editor_new (object,
                                                paths[i].path_property_name,
                                                paths[i].writable_property_name,
                                                gettext (paths[i].fs_label));
	gtk_container_add (GTK_CONTAINER (vbox), patheditor);
	gtk_widget_show (patheditor);
      }
  }

  gtk_widget_show (tv);
  gtk_widget_show (notebook);

  gtk_tree_view_expand_all (GTK_TREE_VIEW (tv));

  return dialog;
}
