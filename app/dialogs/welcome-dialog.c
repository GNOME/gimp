/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * welcome-dialog.c
 * Copyright (C) 2022 Jehan
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
#ifdef GDK_WINDOWING_WAYLAND
#include <gdk/gdkwayland.h>
#endif

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpthumb/gimpthumb.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "dialogs-types.h"

#include "gimp-version.h"

#include "config/gimprc.h"

#include "core/gimp.h"
#include "core/gimp-utils.h"
#include "core/gimpcontainer.h"
#include "core/gimpimagefile.h"

#include "file/file-open.h"

#include "widgets/gimpdialogfactory.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpprefsbox.h"
#include "widgets/gimprow.h"
#include "widgets/gimpuimanager.h"
#include "widgets/gimpwidgets-utils.h"

#include "menus/menus.h"

#include "gui/icon-themes.h"
#include "gui/themes.h"

#include "file-open-dialog.h"
#include "preferences-dialog-utils.h"
#include "welcome-dialog.h"
#include "welcome-dialog-data.h"

#include "gimp-intl.h"


static GtkWidget * welcome_dialog_new                (Gimp          *gimp,
                                                      GimpConfig    *config,
                                                      gboolean       show_welcome_page);
static void   welcome_dialog_response                (GtkWidget     *widget,
                                                      gint           response_id,
                                                      GtkWidget     *dialog);
static void   welcome_dialog_release_item_activated  (GtkListBox    *listbox,
                                                      GtkListBoxRow *row,
                                                      gpointer       user_data);
static void   welcome_add_link                       (GtkGrid        *grid,
                                                      gint            column,
                                                      gint           *row,
                                                      const gchar    *emoji,
                                                      const gchar    *title,
                                                      const gchar    *link);
static void   welcome_size_allocate                  (GtkWidget      *welcome_dialog,
                                                      GtkAllocation  *allocation,
                                                      gpointer        user_data);
static void   welcome_dialog_create_welcome_page     (Gimp           *gimp,
                                                      GtkWidget      *welcome_dialog,
                                                      GtkWidget      *main_vbox);
static void   welcome_dialog_create_personalize_page (Gimp           *gimp,
                                                      GimpConfig     *config,
                                                      GtkWidget      *welcome_dialog,
                                                      GtkWidget      *main_vbox);
static void   welcome_dialog_create_contribute_page  (Gimp           *gimp,
                                                      GtkWidget      *welcome_dialog,
                                                      GtkWidget      *main_vbox);
static void   welcome_dialog_create_creation_page    (Gimp           *gimp,
                                                      GimpConfig     *config,
                                                      GtkWidget      *welcome_dialog,
                                                      GtkWidget      *main_vbox);
static void   welcome_dialog_create_release_page     (Gimp           *gimp,
                                                      GtkWidget      *welcome_dialog,
                                                      GtkWidget      *main_vbox);

static void   welcome_dialog_new_image_dialog        (GtkWidget      *button,
                                                      GtkWidget      *welcome_dialog);
static void   welcome_dialog_open_image_dialog       (GtkWidget      *button,
                                                      GtkWidget      *welcome_dialog);
static void   welcome_dialog_new_dialog_response     (GtkWidget      *dialog,
                                                      gint            response_id,
                                                      GtkWidget      *welcome_dialog);
static void   welcome_dialog_open_dialog_close       (GtkWidget      *dialog,
                                                      GtkWidget      *welcome_dialog);
static void   welcome_open_activated_callback        (GtkListBox     *listbox,
                                                      GtkListBoxRow  *row,
                                                      GtkWidget      *welcome_dialog);
static void   welcome_open_images_callback           (GtkWidget      *button,
                                                      GtkListBox     *listbox);
static void   welcome_dialog_new_image_accelerator   (GtkAccelGroup  *accel_group,
                                                      GObject        *accelerator_widget,
                                                      guint           keyval,
                                                      GdkModifierType mods,
                                                      gpointer        user_data);
static void   welcome_dialog_open_image_dialog_accelerator
                                                     (GtkAccelGroup  *accel_group,
                                                      GObject        *accelerator_widget,
                                                      guint           keyval,
                                                      GdkModifierType mods,
                                                      gpointer        user_data);
static void   welcome_dialog_open_image_accelerator  (GtkAccelGroup  *accel_group,
                                                      GObject        *accelerator_widget,
                                                      guint           keyval,
                                                      GdkModifierType mods,
                                                      gpointer        user_data);

static gboolean welcome_scrollable_resize            (gpointer        data);


static GtkWidget *welcome_dialog;

GtkWidget *
welcome_dialog_create (Gimp     *gimp,
                       gboolean  show_welcome_page)
{
  GimpConfig *config;
  GimpConfig *config_copy;
  GimpConfig *config_orig;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (GIMP_IS_CONFIG (gimp->edit_config), NULL);

  if (welcome_dialog)
    return welcome_dialog;

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

  g_set_weak_pointer (&welcome_dialog,
                      welcome_dialog_new (gimp, config_copy, show_welcome_page));

  g_object_set_data (G_OBJECT (welcome_dialog), "gimp", gimp);

  g_object_set_data_full (G_OBJECT (welcome_dialog), "config-copy", config_copy,
                          (GDestroyNotify) g_object_unref);
  g_object_set_data_full (G_OBJECT (welcome_dialog), "config-orig", config_orig,
                          (GDestroyNotify) g_object_unref);

  gtk_style_context_add_class (gtk_widget_get_style_context (welcome_dialog),
                               "gimp-welcome-dialog");

  return welcome_dialog;
}

static GtkWidget *
welcome_dialog_new (Gimp       *gimp,
                    GimpConfig *config,
                    gboolean    show_welcome_page)
{
  GtkWidget      *dialog;
  GList          *windows;
  GtkWidget      *switcher;
  GtkWidget      *stack;
  GtkWidget      *tree_view;
  GtkTreeIter     top_iter;

  GtkWidget      *prefs_box;
  GtkWidget      *main_vbox;

  gchar          *title;

  GtkAccelGroup  *accel_group;
  guint           accel_key;
  GdkModifierType accel_mods;
  gchar         **accels;

  /* Translators: the %s string will be the version, e.g. "3.0". */
  title = g_strdup_printf (_("Welcome to GIMP %s"), GIMP_VERSION);
  windows = gimp_get_image_windows (gimp);
  dialog = gimp_dialog_new (title,
                            "gimp-welcome-dialog",
                            windows ?  windows->data : NULL,
                            0, gimp_standard_help_func,
                            GIMP_HELP_WELCOME_DIALOG,
                            _("_Close"), GTK_RESPONSE_CLOSE,
                            NULL);
  g_list_free (windows);
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER_ON_PARENT);
  g_free (title);

  gtk_widget_set_margin_start (GTK_WIDGET (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), 0);
  gtk_widget_set_margin_end (GTK_WIDGET (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), 0);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (welcome_dialog_response),
                    dialog);

  /*****************/
  /* Page Switcher */
  /*****************/
  switcher  = gtk_stack_switcher_new ();
  prefs_box = gimp_prefs_box_new ();
  stack     = gimp_prefs_box_get_stack (GIMP_PREFS_BOX (prefs_box));

  gimp_prefs_box_set_header_visible (GIMP_PREFS_BOX (prefs_box), FALSE);
  gtk_stack_switcher_set_stack (GTK_STACK_SWITCHER (switcher),
                                GTK_STACK (stack));
  gtk_container_set_border_width (GTK_CONTAINER (switcher), 2);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      switcher, FALSE, FALSE, 0);
  gtk_widget_set_halign (switcher, GTK_ALIGN_CENTER);
  gtk_widget_set_visible (switcher, TRUE);

  gtk_container_set_border_width (GTK_CONTAINER (prefs_box), 0);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      prefs_box, TRUE, TRUE, 0);
  gtk_widget_set_visible (prefs_box, TRUE);

  tree_view = gimp_prefs_box_get_tree_view (GIMP_PREFS_BOX (prefs_box));
  /* Hide the side panel selection since we're using GtkStackSwitcher */
  gtk_widget_set_visible (gtk_widget_get_parent (tree_view), FALSE);

  g_object_set_data (G_OBJECT (dialog), "prefs-box", prefs_box);

  main_vbox = gimp_prefs_box_add_page (GIMP_PREFS_BOX (prefs_box),
                                       "gimp-wilber",
                                       _("Welcome"),
                                       _("Welcome"),
                                       "gimp-welcome",
                                       NULL,
                                       &top_iter);
  gtk_widget_set_margin_top (main_vbox, 0);
  gtk_widget_set_margin_bottom (main_vbox, 0);
  gtk_widget_set_margin_start (main_vbox, 0);
  gtk_widget_set_margin_end (main_vbox, 0);

  welcome_dialog_create_welcome_page (gimp, dialog, main_vbox);
  gtk_widget_set_visible (main_vbox, TRUE);

  main_vbox = gimp_prefs_box_add_page (GIMP_PREFS_BOX (prefs_box),
                                       "gimp-wilber",
                                       _("Personalize"),
                                       _("Personalize"),
                                       "gimp-welcome-personalize",
                                       NULL,
                                       &top_iter);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);

  welcome_dialog_create_personalize_page (gimp, config, dialog, main_vbox);
  gtk_widget_set_visible (main_vbox, TRUE);

  main_vbox = gimp_prefs_box_add_page (GIMP_PREFS_BOX (prefs_box),
                                       "gimp-wilber",
                                       _("Contribute"),
                                       _("Contribute"),
                                       "gimp-welcome-contribute",
                                       NULL,
                                       &top_iter);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);

  welcome_dialog_create_contribute_page (gimp, dialog, main_vbox);
  gtk_widget_set_visible (main_vbox, TRUE);

  main_vbox = gimp_prefs_box_add_page (GIMP_PREFS_BOX (prefs_box),
                                       "gimp-wilber",
                                       _("Create"),
                                       _("Create"),
                                       "gimp-welcome-create",
                                       NULL,
                                       &top_iter);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);

  welcome_dialog_create_creation_page (gimp, config, dialog, main_vbox);
  gtk_widget_set_visible (main_vbox, TRUE);

  /* If dialog is set to always show on load, switch to the Create page */
  if (! show_welcome_page)
    gtk_stack_set_visible_child_name (GTK_STACK (stack), "gimp-welcome-create");

  if (gimp_welcome_dialog_n_items > 0)
    {
      main_vbox = gimp_prefs_box_add_page (GIMP_PREFS_BOX (prefs_box),
                                           "gimp-wilber",
                                           _("Release Notes"),
                                           _("Release Notes"),
                                           "gimp-welcome-release_notes",
                                           NULL,
                                           &top_iter);
      gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);

      welcome_dialog_create_release_page (gimp, dialog, main_vbox);
      gtk_widget_set_visible (main_vbox, TRUE);
    }

  /*************/
  /* Shortcuts */
  /*************/
  /* XXX: GtkAccelGroup will be deprecated in GTK4
   * See: https://docs.gtk.org/gtk4/migrating-3to4.html#use-the-new-apis-for-keyboard-shortcuts
   * This GtkAccelGroup must be converted to a GtkShortcutController
   */
  accel_group = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (dialog), accel_group);

  accels = gtk_application_get_accels_for_action (GTK_APPLICATION (gimp->app),
                                                  "app.image-new");
  if (accels && accels[0])
    {
      gtk_accelerator_parse (accels[0], &accel_key, &accel_mods);
      gtk_accel_group_connect (accel_group,
                              accel_key, accel_mods, 0,
                              g_cclosure_new (G_CALLBACK (welcome_dialog_new_image_accelerator),
                                              dialog, NULL));
      g_strfreev (accels);
    }

  accels = gtk_application_get_accels_for_action (GTK_APPLICATION (gimp->app),
                                                  "app.file-open");
  if (accels && accels[0])
    {
      gtk_accelerator_parse (accels[0], &accel_key, &accel_mods);
      gtk_accel_group_connect (accel_group,
                              accel_key, accel_mods, 0,
                              g_cclosure_new (G_CALLBACK (welcome_dialog_open_image_dialog_accelerator),
                                              dialog, NULL));
      g_strfreev (accels);
    }

  for (guint i = 0; i < 10; i++)
    {
      gchar accel_str[24];

      g_snprintf (accel_str, sizeof (accel_str), "app.file-open-recent-%02u", i + 1);
      accels = gtk_application_get_accels_for_action (GTK_APPLICATION (gimp->app),
                                                      accel_str);
      if (accels && accels[0])
        {
          gtk_accelerator_parse (accels[0], &accel_key, &accel_mods);
          gtk_accel_group_connect (accel_group,
                                  accel_key, accel_mods, 0,
                                  g_cclosure_new (G_CALLBACK (welcome_dialog_open_image_accelerator),
                                                  GUINT_TO_POINTER (i), NULL));
          g_strfreev (accels);
        }
    }

  return dialog;
}

static void
welcome_dialog_response (GtkWidget *widget,
                         gint       response_id,
                         GtkWidget *dialog)
{
  Gimp    *gimp = g_object_get_data (G_OBJECT (dialog), "gimp");
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

          /* The first 3 bytes are the bullet unicode character
           * for doing a list (U+2022).
           */
          g_string_append_printf (string, "\xe2\x80\xa2 %s\n", g_param_spec_get_nick (param_spec));
        }

      prefs_message (dialog, GTK_MESSAGE_INFO, FALSE, string->str);

      g_string_free (string, TRUE);
    }

  g_list_free (restart_diff);

  gtk_widget_destroy (dialog);
}

static void
welcome_dialog_create_welcome_page (Gimp      *gimp,
                                    GtkWidget *welcome_dialog,
                                    GtkWidget *main_vbox)
{
  GtkWidget  *grid;
  GtkWidget  *image;
  GtkWidget  *widget;

  gchar      *markup;
  gchar      *tmp;
  gint        row;

  /****************/
  /* Welcome page */
  /****************/

  image = gtk_image_new_from_icon_name ("gimp-wilber",
                                        GTK_ICON_SIZE_DIALOG);
  gtk_widget_set_valign (image, GTK_ALIGN_CENTER);
  gtk_box_pack_start (GTK_BOX (main_vbox), image, TRUE, TRUE, 0);
  gtk_widget_set_visible (image, TRUE);

  g_object_set_data (G_OBJECT (welcome_dialog), "welcome-vbox", main_vbox);
  g_signal_connect (welcome_dialog,
                    "size-allocate",
                    G_CALLBACK (welcome_size_allocate),
                    image);

  /* Welcome title. */
  grid = gtk_grid_new ();
  gtk_grid_set_column_homogeneous (GTK_GRID (grid), TRUE);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 0);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 4);
  gtk_box_pack_start (GTK_BOX (main_vbox), grid, TRUE, TRUE, 0);
  gtk_widget_set_margin_start (GTK_WIDGET (grid), 12);
  gtk_widget_set_margin_end (GTK_WIDGET (grid), 12);
  gtk_widget_set_visible (grid, TRUE);

  /* Translators: the %s string will be the version, e.g. "3.0". */
  tmp = g_strdup_printf (_("You installed GIMP %s!"), GIMP_VERSION);
  widget = gtk_label_new (NULL);
  /* XXX For GTK4, we may just replace with gtk_widget_add_css_class() AFAICS. */
  gtk_style_context_add_class (gtk_widget_get_style_context (widget), "title-3");
  gtk_label_set_text (GTK_LABEL (widget), tmp);
  g_free (tmp);
  gtk_label_set_justify (GTK_LABEL (widget), GTK_JUSTIFY_CENTER);
  gtk_label_set_line_wrap (GTK_LABEL (widget), FALSE);
  gtk_widget_set_margin_bottom (widget, 10);
  gtk_grid_attach (GTK_GRID (grid), widget, 0, 0, 2, 1);
  gtk_widget_set_visible (widget, TRUE);

  /* Welcome message: left */

  markup = _("GIMP is Free Software for image authoring and manipulation.\n"
             "Want to know more?");

  widget = gtk_label_new (NULL);
  gtk_label_set_max_width_chars (GTK_LABEL (widget), 30);
  /*gtk_widget_set_size_request (widget, max_width / 2, -1);*/
  gtk_label_set_line_wrap (GTK_LABEL (widget), TRUE);
  gtk_widget_set_vexpand (widget, FALSE);
  gtk_widget_set_hexpand (widget, FALSE);

  /* Making sure the labels are well top aligned to avoid some ugly
   * misalignment if left and right labels have different sizes,
   * but also left-aligned so that the messages are slightly to the left
   * of the emoji/link list below.
   * Following design decisions by Aryeom.
   */
  gtk_label_set_xalign (GTK_LABEL (widget), 0.0);
  gtk_label_set_yalign (GTK_LABEL (widget), 0.0);
  gtk_widget_set_margin_bottom (widget, 10);
  gtk_label_set_markup (GTK_LABEL (widget), markup);

  gtk_grid_attach (GTK_GRID (grid), widget, 0, 1, 1, 1);

  gtk_widget_set_visible (widget, TRUE);

  row = 2;
  welcome_add_link (GTK_GRID (grid), 0, &row,
                    /* "globe with meridians" emoticone in UTF-8. */
                    "\xf0\x9f\x8c\x90",
                    _("GIMP website"), "https://www.gimp.org/");
  welcome_add_link (GTK_GRID (grid), 0, &row,
                    /* "open book" emoticone in UTF-8. */
                    "\xf0\x9f\x93\x96",
                    _("Documentation"),
                    "https://docs.gimp.org/");
  welcome_add_link (GTK_GRID (grid), 0, &row,
                    /* "graduation cap" emoticone in UTF-8. */
                    "\xf0\x9f\x8e\x93",
                    _("Community Tutorials"),
                    "https://www.gimp.org/tutorials/");

  /* XXX: should we add API docs for plug-in developers once it's
   * properly set up? */

  /* Welcome message: right */

  markup = _("GIMP is Community Software under the GNU general public license v3.\n"
             "Want to contribute?");

  widget = gtk_label_new (NULL);
  gtk_label_set_line_wrap (GTK_LABEL (widget), TRUE);
  gtk_label_set_max_width_chars (GTK_LABEL (widget), 30);
  /*gtk_widget_set_size_request (widget, max_width / 2, -1);*/

  /* Again the alignments are important. */
  gtk_label_set_xalign (GTK_LABEL (widget), 0.0);
  gtk_widget_set_vexpand (widget, FALSE);
  gtk_widget_set_hexpand (widget, FALSE);
  gtk_label_set_xalign (GTK_LABEL (widget), 0.0);
  gtk_label_set_yalign (GTK_LABEL (widget), 0.0);
  gtk_widget_set_margin_bottom (widget, 10);
  gtk_label_set_markup (GTK_LABEL (widget), markup);

  gtk_grid_attach (GTK_GRID (grid), widget, 1, 1, 1, 1);

  gtk_widget_set_visible (widget, TRUE);

  row = 2;
  welcome_add_link (GTK_GRID (grid), 1, &row,
                    /* "keyboard" emoticone in UTF-8. */
                    "\xe2\x8c\xa8",
                    _("Contributing"),
                    "https://www.gimp.org/develop/");
  welcome_add_link (GTK_GRID (grid), 1, &row,
                    /* "love letter" emoticone in UTF-8. */
                    "\xf0\x9f\x92\x8c",
                    _("Donating"),
                    "https://www.gimp.org/donating/");
}

static void
welcome_dialog_create_personalize_page (Gimp       *gimp,
                                        GimpConfig *config,
                                        GtkWidget  *welcome_dialog,
                                        GtkWidget  *main_vbox)
{
  GtkSizeGroup *size_group = NULL;
  GtkWidget    *scale;
  GtkListStore *store;

  GtkWidget    *vbox;
  GtkWidget    *hbox;
  GtkWidget    *widget;
  GtkWidget    *button;
  GtkWidget    *grid;

  GObject      *object;

  gchar       **themes;
  gint          n_themes;

  object = G_OBJECT (config);

  size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  /* Themes */
  vbox = prefs_frame_new (_("Theme"), GTK_CONTAINER (main_vbox), FALSE);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 0);
  gtk_widget_set_halign (GTK_WIDGET (hbox), GTK_ALIGN_START);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_set_visible (hbox, TRUE);

  grid = prefs_grid_new (GTK_CONTAINER (hbox));
  button = prefs_enum_combo_box_add (object, "theme-color-scheme",
                                     0, 0,
                                     _("Color scheme"), GTK_GRID (grid),
                                     0, size_group);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 0);
  gtk_widget_set_halign (GTK_WIDGET (hbox), GTK_ALIGN_START);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_set_visible (hbox, TRUE);


  /* Icon Theme */
  store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);
  themes = icon_themes_list_themes (gimp, &n_themes);
  for (gint i = 0; i < n_themes; i++)
    gtk_list_store_insert_with_values (store, NULL,
                                       -1,
                                       0, themes[i],
                                       1, themes[i],
                                       -1);
  g_strfreev (themes);

  widget = gimp_prop_string_combo_box_new (object, "icon-theme",
                                           GTK_TREE_MODEL (store), 0, 1);
  gtk_widget_set_visible (widget, TRUE);

  grid = prefs_grid_new (GTK_CONTAINER (hbox));
  prefs_widget_add_aligned (widget, _("Icon theme"), GTK_GRID (grid), 0, FALSE,
                            size_group);
  g_object_unref (store);

  /* Reset size group for next set of widgets */
  g_clear_object (&size_group);
  size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  prefs_switch_add (object, "prefer-symbolic-icons",
                    _("Use symbolic icons if available"),
                    GTK_BOX (hbox), NULL, &button);
  gtk_widget_set_valign (button, GTK_ALIGN_CENTER);

  vbox = prefs_frame_new (_("Icon Scaling"), GTK_CONTAINER (main_vbox), FALSE);

  prefs_switch_add (object, "override-theme-icon-size",
                    _("_Override icon sizes set by the theme"),
                    GTK_BOX (vbox), NULL, &button);

  scale = gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL,
                                    0.0, 3.0, 1.0);
  /* 'draw_value' updates round_digits. So set it first. */
  gtk_scale_set_draw_value (GTK_SCALE (scale), FALSE);
  gtk_range_set_round_digits (GTK_RANGE (scale), 0.0);
  gtk_scale_add_mark (GTK_SCALE (scale), 0.0, GTK_POS_BOTTOM,
                      _("Small"));
  gtk_scale_add_mark (GTK_SCALE (scale), 1.0, GTK_POS_BOTTOM,
                      _("Medium"));
  gtk_scale_add_mark (GTK_SCALE (scale), 2.0, GTK_POS_BOTTOM,
                      _("Large"));
  gtk_scale_add_mark (GTK_SCALE (scale), 3.0, GTK_POS_BOTTOM,
                      _("Huge"));
  gtk_range_set_value (GTK_RANGE (scale),
                       (gdouble) GIMP_GUI_CONFIG (object)->custom_icon_size);
  g_signal_connect (G_OBJECT (scale), "value-changed",
                    G_CALLBACK (prefs_icon_size_value_changed),
                    GIMP_GUI_CONFIG (object));
  g_signal_connect (G_OBJECT (object), "notify::custom-icon-size",
                    G_CALLBACK (prefs_gui_config_notify_icon_size),
                    scale);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);
  gtk_widget_set_visible (scale, TRUE);

  g_object_bind_property (button, "active",
                          scale,  "sensitive",
                          G_BINDING_SYNC_CREATE);

  vbox = prefs_frame_new (_("Font Scaling"), GTK_CONTAINER (main_vbox), FALSE);
  gimp_help_set_help_data (vbox,
                           _("Font scaling will not work with themes using absolute sizes."),
                           NULL);
  scale = gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL,
                                    50, 200, 10);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_BOTTOM);
  gtk_scale_add_mark (GTK_SCALE (scale), 50.0, GTK_POS_BOTTOM,
                      _("50%"));
  gtk_scale_add_mark (GTK_SCALE (scale), 100.0, GTK_POS_BOTTOM,
                      _("100%"));
  gtk_scale_add_mark (GTK_SCALE (scale), 200.0, GTK_POS_BOTTOM,
                      _("200%"));
  gtk_range_set_value (GTK_RANGE (scale),
                       (gdouble) GIMP_GUI_CONFIG (object)->font_relative_size * 100.0);
  g_signal_connect (G_OBJECT (scale), "value-changed",
                    G_CALLBACK (prefs_font_size_value_changed),
                    GIMP_GUI_CONFIG (object));
  g_signal_connect (G_OBJECT (object), "notify::font-relative-size",
                    G_CALLBACK (prefs_gui_config_notify_font_size),
                    scale);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);
  gtk_widget_set_visible (scale, TRUE);

#ifdef HAVE_ISO_CODES
  vbox = prefs_frame_new (_("GUI Language (requires restart)"),
                          GTK_CONTAINER (main_vbox), FALSE);
  prefs_language_combo_box_add (object, "language", GTK_BOX (vbox));
#endif

  vbox = prefs_frame_new (_("Additional Customizations"), GTK_CONTAINER (main_vbox), FALSE);

#ifndef GDK_WINDOWING_QUARTZ
  prefs_switch_add (object, "custom-title-bar",
                    _("Merge menu and title bar (requires restart)"),
                    GTK_BOX (vbox), size_group, NULL);
#endif

#ifdef CHECK_UPDATE
  if (gimp_version_check_update ())
    {
      prefs_switch_add (object, "check-updates",
                        _("Enable check for updates (requires internet)"),
                        GTK_BOX (vbox), size_group, NULL);
    }
#endif

  prefs_switch_add (object, "toolbox-groups",
                    _("Use tool _groups"),
                    GTK_BOX (vbox), size_group, NULL);

  g_clear_object (&size_group);
}

static void
welcome_dialog_create_creation_page (Gimp       *gimp,
                                     GimpConfig *config,
                                     GtkWidget  *welcome_dialog,
                                     GtkWidget  *main_vbox)
{
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *button;
  GtkWidget *listbox;
  GtkWidget *toggle;
  gint       num_images;
  gint       list_count;

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_set_homogeneous (GTK_BOX (hbox), TRUE);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_set_visible (hbox, TRUE);

  vbox = prefs_frame_new (_("Create a New Image"), GTK_CONTAINER (hbox),
                          FALSE);

  button = gtk_button_new_with_mnemonic (_("C_reate"));
  /* Balancing the indent from the frame */
  gtk_widget_set_margin_end (button, 12);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_set_visible (button, TRUE);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (welcome_dialog_new_image_dialog),
                    welcome_dialog);

  vbox = prefs_frame_new (_("Open an Existing Image"), GTK_CONTAINER (hbox),
                          FALSE);

  button = gtk_button_new_with_mnemonic (_("_Open"));
  gtk_widget_set_margin_end (button, 12);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_set_visible (button, TRUE);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (welcome_dialog_open_image_dialog),
                    welcome_dialog);

  /* Recent Files */
  vbox = prefs_frame_new (_("Recent Images"), GTK_CONTAINER (main_vbox),
                          FALSE);

  listbox = gtk_list_box_new ();
  gtk_list_box_set_selection_mode (GTK_LIST_BOX (listbox),
                                   GTK_SELECTION_MULTIPLE);
  gtk_list_box_set_activate_on_single_click (GTK_LIST_BOX (listbox),
                                             FALSE);
  gtk_container_add (GTK_CONTAINER (vbox), listbox);
  gtk_widget_set_visible (listbox, TRUE);

  num_images = gimp_container_get_n_children (gimp->documents);
  list_count = (num_images <= 8) ? num_images : 8;

  for (gint i = 0; i < list_count; i++)
    {
      GimpImagefile *imagefile = NULL;
      GtkWidget     *row;
      GFile         *file;
      const gchar   *name;
      gchar         *basename;
      gchar         *action_name;

      imagefile = (GimpImagefile *)
        gimp_container_get_child_by_index (gimp->documents, i);

      file = gimp_imagefile_get_file (imagefile);

      name     = gimp_file_get_utf8_name (file);
      basename = g_path_get_basename (name);

      /* If the file is not found, remove it and try to
       * load another one if possible */
      if (! g_file_test (name, G_FILE_TEST_IS_REGULAR))
        {
          g_free (basename);

          if (list_count < num_images)
            list_count++;

          continue;
        }

      row = gimp_row_new (gimp_get_user_context (gimp),
                          GIMP_VIEWABLE (imagefile),
                          32, 0);

      action_name = g_strdup_printf ("file-open-recent-%02u", i + 1);
      g_object_set_data_full (G_OBJECT (row), "action_name", action_name,
                              g_free);

      gtk_widget_set_visible (row, TRUE);
      gtk_list_box_insert (GTK_LIST_BOX (listbox), row, -1);
    }

  g_signal_connect (listbox, "row-activated",
                    G_CALLBACK (welcome_open_activated_callback),
                    welcome_dialog);

  button = gtk_button_new_with_mnemonic (_("O_pen Selected Images"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_set_visible (button, TRUE);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (welcome_open_images_callback),
                    listbox);

  /* "Always show welcome dialog" checkbox */
  toggle = prefs_check_button_add (G_OBJECT (config), "show-welcome-dialog",
                                   _("Show on Start "
                                     "(You can show the Welcome dialog again from the \"Help\" menu)"),
                                   GTK_BOX (main_vbox));
  gtk_container_child_set (GTK_CONTAINER (main_vbox), toggle,
                           "fill",      TRUE,
                           "pack-type", GTK_PACK_END,
                           NULL);
}

static void
welcome_dialog_create_contribute_page (Gimp       *gimp,
                                       GtkWidget  *welcome_dialog,
                                       GtkWidget  *main_vbox)
{
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *button;
  GtkWidget *label;

  gchar     *markup;
  gchar     *tmp;

  gtk_box_set_spacing (GTK_BOX (main_vbox), 2);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_set_visible (hbox, TRUE);

  tmp = g_strdup_printf (_("Ways to contribute"));
  markup = g_strdup_printf ("<big>%s</big>", tmp);
  g_free (tmp);
  label = gtk_label_new (NULL);
  gtk_label_set_markup (GTK_LABEL (label), markup);
  g_free (markup);
  gtk_box_set_center_widget (GTK_BOX (hbox), label);
  gtk_widget_set_visible (label, TRUE);

  vbox = prefs_frame_new (_("Report Bugs"), GTK_CONTAINER (main_vbox), FALSE);

  tmp = g_strdup_printf (_("As any application, GIMP is not bug-free, so "
                           "reporting bugs that you encounter is very "
                           "important to the development."));
  label = gtk_label_new (tmp);
  g_free (tmp);
  gtk_label_set_max_width_chars (GTK_LABEL (label), 30);
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_set_visible (label, TRUE);
  button = gtk_link_button_new_with_label ("https://www.gimp.org/bugs/", _("Report Bugs"));
  gtk_box_pack_start (GTK_BOX (main_vbox), button, FALSE, FALSE, 0);
  gtk_widget_set_visible (button, TRUE);

  vbox = prefs_frame_new (_("Write Code"), GTK_CONTAINER (main_vbox), FALSE);

  tmp = g_strdup_printf (_("Our Developer Website is where you want to start "
                           "learning about being a code contributor."));
  label = gtk_label_new (tmp);
  g_free (tmp);
  gtk_label_set_max_width_chars (GTK_LABEL (label), 30);
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_set_visible (label, TRUE);
  button = gtk_link_button_new_with_label ("https://developer.gimp.org/", _("Write Code"));
  gtk_box_pack_start (GTK_BOX (main_vbox), button, FALSE, FALSE, 0);
  gtk_widget_set_visible (button, TRUE);

  vbox = prefs_frame_new (_("Translate"), GTK_CONTAINER (main_vbox), FALSE);

  tmp = g_strdup_printf (_("Contact the respective translation team for your "
                           "language"));
  label = gtk_label_new (tmp);
  g_free (tmp);
  gtk_label_set_max_width_chars (GTK_LABEL (label), 30);
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_set_visible (label, TRUE);
  button = gtk_link_button_new_with_label ("https://l10n.gnome.org/teams/", _("Translate"));
  gtk_box_pack_start (GTK_BOX (main_vbox), button, FALSE, FALSE, 0);
  gtk_widget_set_visible (button, TRUE);

  vbox = prefs_frame_new (_("Donate"), GTK_CONTAINER (main_vbox), FALSE);

  tmp = g_strdup_printf (_("Donating money is important: it makes GIMP "
                           "sustainable."));
  label = gtk_label_new (tmp);
  g_free (tmp);
  gtk_label_set_max_width_chars (GTK_LABEL (label), 30);
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_set_visible (label, TRUE);
  button = gtk_link_button_new_with_label ("https://liberapay.com/GIMP/donate", _("Donate via Liberapay"));
  gtk_box_pack_start (GTK_BOX (main_vbox), button, FALSE, FALSE, 0);
  gtk_widget_set_visible (button, TRUE);
  button = gtk_link_button_new_with_label ("https://www.gimp.org/donating/", _("Other donation options"));
  gtk_box_pack_start (GTK_BOX (main_vbox), button, FALSE, FALSE, 0);
  gtk_widget_set_visible (button, TRUE);
}

static void
welcome_dialog_create_release_page (Gimp      *gimp,
                                    GtkWidget *welcome_dialog,
                                    GtkWidget *main_vbox)
{
  GtkWidget  *scrolled_window;
  GtkWidget  *hbox;
  GtkWidget  *image;
  GtkWidget  *listbox;
  GtkWidget  *widget;

  gchar      *release_link;
  gchar      *markup;
  gchar      *tmp;

  /*****************/
  /* Release Notes */
  /*****************/
  if (gimp_welcome_dialog_n_items > 0)
    {
      gint n_demos = 0;

      /* Release note title. */
      hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
      gtk_container_set_border_width (GTK_CONTAINER (hbox), 6);
      gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);
      gtk_widget_set_visible (hbox, TRUE);

      /* Translators: the %s string will be the version, e.g. "3.0". */
      tmp = g_strdup_printf (_("GIMP %s Release Notes"), GIMP_VERSION);
      markup = g_strdup_printf ("<b><big>%s</big></b>", tmp);
      g_free (tmp);
      widget = gtk_label_new (NULL);
      gtk_label_set_markup (GTK_LABEL (widget), markup);
      g_free (markup);
      gtk_label_set_selectable (GTK_LABEL (widget), FALSE);
      gtk_label_set_justify (GTK_LABEL (widget), GTK_JUSTIFY_CENTER);
      gtk_label_set_line_wrap (GTK_LABEL (widget), FALSE);
      gtk_box_pack_start (GTK_BOX (hbox), widget, TRUE, TRUE, 0);
      gtk_widget_set_visible (widget, TRUE);

      image = gtk_image_new_from_icon_name ("gimp-user-manual",
                                            GTK_ICON_SIZE_DIALOG);
      gtk_widget_set_valign (image, GTK_ALIGN_START);
      gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
      gtk_widget_set_visible (image, TRUE);

      /* Release note introduction. */

      if (gimp_welcome_dialog_intro_n_paragraphs)
        {
          GString *introduction = NULL;

          for (gint i = 0; i < gimp_welcome_dialog_intro_n_paragraphs; i++)
            {
              if (i == 0)
                introduction = g_string_new (_(gimp_welcome_dialog_intro[i]));
              else
                g_string_append_printf (introduction, "\n%s",
                                        _(gimp_welcome_dialog_intro[i]));
            }
          widget = gtk_label_new (NULL);
          gtk_label_set_markup (GTK_LABEL (widget), introduction->str);
          gtk_label_set_max_width_chars (GTK_LABEL (widget), 70);
          gtk_label_set_selectable (GTK_LABEL (widget), FALSE);
          gtk_label_set_justify (GTK_LABEL (widget), GTK_JUSTIFY_LEFT);
          gtk_label_set_line_wrap (GTK_LABEL (widget), TRUE);
          gtk_box_pack_start (GTK_BOX (main_vbox), widget, FALSE, FALSE, 0);
          gtk_widget_set_visible (widget, TRUE);

          g_string_free (introduction, TRUE);
        }

      /* Release note's change items. */

      scrolled_window = gtk_scrolled_window_new (NULL, NULL);
      gtk_box_pack_start (GTK_BOX (main_vbox), scrolled_window, TRUE, TRUE, 0);
      gtk_widget_set_visible (scrolled_window, TRUE);

      listbox = gtk_list_box_new ();

      for (gint i = 0; i < gimp_welcome_dialog_n_items; i++)
        {
          GtkWidget *row;
          gchar     *markup;
          gchar     *text;

          text = g_markup_escape_text (_((gchar *) gimp_welcome_dialog_items[i]), -1);

          /* Add a bold dot for pretty listing. */
          if (i < gimp_welcome_dialog_n_items &&
              gimp_welcome_dialog_demos[i] != NULL)
            {
              markup = g_strdup_printf ("<span weight='ultrabold'>\xe2\x96\xb6</span>  %s", text);
              n_demos++;
            }
          else
            {
              markup = g_strdup_printf ("<span weight='ultrabold'>\xe2\x80\xa2</span>  %s", text);
            }

          row = gtk_list_box_row_new ();
          widget = gtk_label_new (NULL);
          gtk_label_set_markup (GTK_LABEL (widget), markup);
          gtk_label_set_line_wrap (GTK_LABEL (widget), TRUE);
          gtk_label_set_line_wrap_mode (GTK_LABEL (widget), PANGO_WRAP_WORD);
          gtk_label_set_justify (GTK_LABEL (widget), GTK_JUSTIFY_LEFT);
          gtk_widget_set_halign (widget, GTK_ALIGN_START);
          gtk_label_set_xalign (GTK_LABEL (widget), 0.0);
          gtk_container_add (GTK_CONTAINER (row), widget);

          gtk_list_box_insert (GTK_LIST_BOX (listbox), row, -1);
          gtk_widget_show_all (row);

          g_free (markup);
          g_free (text);
        }
      gtk_container_add (GTK_CONTAINER (scrolled_window), listbox);
      gtk_list_box_set_selection_mode (GTK_LIST_BOX (listbox),
                                       GTK_SELECTION_NONE);

      g_signal_connect (listbox, "row-activated",
                        G_CALLBACK (welcome_dialog_release_item_activated),
                        gimp);
      gtk_widget_set_visible (listbox, TRUE);

      if (n_demos > 0)
        {
          /* A small explicative string to help discoverability of the demo
           * ability.
           */
          hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
          gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);
          gtk_widget_set_visible (hbox, TRUE);

          image = gtk_image_new_from_icon_name ("dialog-information",
                                                GTK_ICON_SIZE_MENU);
          gtk_widget_set_valign (image, GTK_ALIGN_CENTER);
          gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
          gtk_widget_set_visible (image, TRUE);

          widget = gtk_label_new (NULL);
          tmp = g_strdup_printf (_("Click on release items with a %s bullet point to get a tour."),
                                 "<span weight='ultrabold'>\xe2\x96\xb6</span>");
          markup = g_strdup_printf ("<i>%s</i>", tmp);
          g_free (tmp);
          gtk_label_set_markup (GTK_LABEL (widget), markup);
          g_free (markup);
          gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
          gtk_widget_set_visible (widget, TRUE);

          /* TODO: if a demo changed settings, should we add a "reset"
           * button to get back to previous state?
           */
        }

      /* Link to full release notes on web site at the bottom. */
      hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
      gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);
      gtk_widget_set_visible (hbox, TRUE);

      if (GIMP_MINOR_VERSION % 2 == 0)
        {
          if (GIMP_MICRO_VERSION == 0)
#ifdef GIMP_RC_VERSION
            release_link = g_strdup_printf ("https://www.gimp.org/release/%d.%d.0-RC%d/",
                                            GIMP_MAJOR_VERSION, GIMP_MINOR_VERSION,
                                            GIMP_RC_VERSION);
#else
            release_link = g_strdup_printf ("https://www.gimp.org/release-notes/gimp-%d.%d.html",
                                            GIMP_MAJOR_VERSION, GIMP_MINOR_VERSION);
#endif
          else
            release_link = g_strdup_printf ("https://www.gimp.org/release/%d.%d.%d/",
                                            GIMP_MAJOR_VERSION, GIMP_MINOR_VERSION,
                                            GIMP_MICRO_VERSION);
        }
      else
        {
          release_link = g_strdup ("https://www.gimp.org/");
        }

      widget = gtk_link_button_new_with_label (release_link, _("Learn more"));
      gtk_widget_set_visible (widget, TRUE);
      gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
      g_free (release_link);
    }
}

/* Actions */
static void
welcome_dialog_new_image_dialog (GtkWidget *button,
                                 GtkWidget *welcome_dialog)
{
  GimpUIManager *manager;
  Gimp          *gimp;
  GtkWidget     *dialog;

  gimp    = g_object_get_data (G_OBJECT (welcome_dialog), "gimp");
  manager = menus_get_image_manager_singleton (gimp);

  /* XXX: to avoid code duplication and divergence, we just call the "image-new"
   * action, then we check that the new dialog singleton exists in order to
   * handle its responses.
   */
  if (gimp_ui_manager_activate_action (manager, "image", "image-new") &&
      (dialog = gimp_dialog_factory_find_widget (gimp_dialog_factory_get_singleton (),
                                                 "gimp-image-new-dialog")))
    {
      gtk_widget_set_visible (welcome_dialog, FALSE);
      g_signal_connect (dialog, "response",
                        G_CALLBACK (welcome_dialog_new_dialog_response),
                        welcome_dialog);
      g_signal_connect_swapped (dialog, "destroy",
                                G_CALLBACK (gtk_widget_destroy),
                                welcome_dialog);
    }
}

static void
welcome_dialog_open_image_dialog (GtkWidget *button,
                                  GtkWidget *welcome_dialog)
{
  Gimp          *gimp    = g_object_get_data (G_OBJECT (welcome_dialog), "gimp");
  GtkWidget     *dialog  = file_open_dialog_new (gimp);
  GimpUIManager *manager = menus_get_image_manager_singleton (gimp);

  if (gimp_ui_manager_activate_action (manager, "file", "file-open") &&
      (dialog = gimp_dialog_factory_find_widget (gimp_dialog_factory_get_singleton (),
                                                 "gimp-file-open-dialog")))
    {
      gtk_widget_set_visible (welcome_dialog, FALSE);

      gtk_window_present (GTK_WINDOW (dialog));

      g_signal_connect (dialog, "destroy",
                        G_CALLBACK (welcome_dialog_open_dialog_close),
                        welcome_dialog);
    }
}

static void
welcome_dialog_new_dialog_response (GtkWidget *dialog,
                                    gint       response_id,
                                    GtkWidget *welcome_dialog)
{
  switch (response_id)
    {
    case GTK_RESPONSE_OK:
      /* Don't destroy the welcome dialog directly, because it's possible to get
       * the OK response without the new image dialog closing (in case it
       * triggers a confirm dialog), followed by a GTK_RESPONSE_CANCEL.
       *
       * Let the "destroy" handlers take care of destroying the welcome dialog.
       */
      break;

    case GTK_RESPONSE_CANCEL:
    case GTK_RESPONSE_DELETE_EVENT:
      g_signal_handlers_disconnect_by_func (dialog,
                                            G_CALLBACK (gtk_widget_destroy),
                                            welcome_dialog);
      gtk_widget_set_visible (welcome_dialog, TRUE);
      break;

    default:
      break;
    }
}

static void
welcome_dialog_open_dialog_close (GtkWidget *dialog,
                                  GtkWidget *welcome_dialog)
{
  GSList *files = NULL;

  files = gtk_file_chooser_get_files (GTK_FILE_CHOOSER (dialog));

  if (files && welcome_dialog)
    {
      gtk_widget_destroy (welcome_dialog);
      g_slist_free_full (files, (GDestroyNotify) g_object_unref);
      return;
    }

  if (welcome_dialog)
    gtk_widget_set_visible (welcome_dialog, TRUE);
}

static void
welcome_open_activated_callback (GtkListBox    *listbox,
                                 GtkListBoxRow *row,
                                 GtkWidget     *welcome_dialog)
{
  welcome_open_images_callback (NULL, listbox);
}

static void
welcome_open_images_callback (GtkWidget  *button,
                              GtkListBox *listbox)
{
  GList         *rows   = NULL;
  Gimp          *gimp   = NULL;
  gboolean       opened = FALSE;
  gchar         *action_name;
  GimpUIManager *manager;

  if (! welcome_dialog)
    return;

  gimp = g_object_get_data (G_OBJECT (welcome_dialog), "gimp");
  manager = menus_get_image_manager_singleton (gimp);

  rows = gtk_list_box_get_selected_rows (listbox);
  if (rows)
    {
      gtk_widget_set_sensitive (welcome_dialog, FALSE);

      for (GList *iter = rows; iter; iter = iter->next)
        {
          action_name = (gchar *) g_object_get_data (G_OBJECT (iter->data),
                                                     "action_name");

          if (gimp_ui_manager_activate_action (manager, "file", action_name))
            opened = TRUE;
        }

      g_list_free (rows);
    }

  /* If no images were successfully opened, leave the dialogue up */
  gtk_widget_set_sensitive (welcome_dialog, TRUE);
  if (opened)
    gtk_widget_destroy (welcome_dialog);
}

static void
welcome_dialog_release_item_activated (GtkListBox    *listbox,
                                       GtkListBoxRow *row,
                                       gpointer       user_data)
{
  Gimp         *gimp          = user_data;
  GList        *blink_script  = NULL;
  const gchar  *script_string;
  gchar       **script_steps;
  gint          row_index;
  gint          i;

  row_index = gtk_list_box_row_get_index (row);

  g_return_if_fail (row_index < gimp_welcome_dialog_n_items);

  script_string = gimp_welcome_dialog_demos[row_index];

  if (script_string == NULL)
    /* Not an error. Some release items have no demos. */
    return;

  script_steps = g_strsplit (script_string, ",", 0);

  for (i = 0; script_steps[i]; i++)
    {
      gchar **ids;
      gchar  *dockable_id    = NULL;
      gchar  *widget_id      = NULL;
      gchar **settings       = NULL;
      gchar  *settings_value = NULL;

      ids = g_strsplit (script_steps[i], ":", 2);
      /* Even if the string doesn't contain a second part, it is
       * NULL-terminated, hence the widget_id will simply be NULL, which
       * is fine when you just want to blink a dialog.
       */
      dockable_id = ids[0];
      widget_id   = ids[1];

      if (widget_id != NULL)
        {
          settings = g_strsplit (widget_id, "=", 2);

          widget_id = settings[0];
          settings_value = settings[1];
        }

      /* Allowing white spaces so that the demo in XML metadata can be
       * spaced-out or even on multiple lines for clarity.
       */
      dockable_id = g_strstrip (dockable_id);
      if (widget_id != NULL)
        widget_id = g_strstrip (widget_id);

      /* All our dockable IDs start with "gimp-". This allows to write
       * shorter names in the demo script.
       */
      if (! g_str_has_prefix (dockable_id, "gimp-"))
        {
          gchar *tmp = g_strdup_printf ("gimp-%s", dockable_id);

          g_free (ids[0]);
          dockable_id = ids[0] = tmp;
        }

      /* Blink widget. */
      if (g_strcmp0 (dockable_id, "gimp-toolbox") == 0)
        {
          /* All tool button IDs start with "tools-". This allows to
           * write shorter tool names in the demo script.
           */
          if (widget_id != NULL && ! g_str_has_prefix (widget_id, "tools-"))
            {
              gchar *tmp = g_strdup_printf ("tools-%s", widget_id);

              g_free (settings[0]);
              widget_id = settings[0] = tmp;
            }

          gimp_blink_toolbox (gimp, widget_id, &blink_script);
        }
      else
        {
          gimp_blink_dockable (gimp, dockable_id,
                               widget_id, settings_value,
                               &blink_script);
        }

      g_strfreev (ids);
      if (settings)
        g_strfreev (settings);
    }
  if (blink_script != NULL)
    {
      GList *windows = gimp_get_image_windows (gimp);

      /* Losing focus on the welcome dialog on purpose for the main GUI
       * to be more readable.
       */
      if (windows)
        gtk_window_present (windows->data);

      gimp_blink_play_script (blink_script);

      g_list_free (windows);
    }

  g_strfreev (script_steps);
}

static void
welcome_add_link (GtkGrid     *grid,
                  gint         column,
                  gint        *row,
                  const gchar *emoji,
                  const gchar *title,
                  const gchar *link)
{
  GtkWidget *hbox;
  GtkWidget *button;
  GtkWidget *icon;

  /* TODO: Aryeom doesn't like the spacing here. There is too much
   * spacing between the link lines and between emojis and links. But we
   * didn't manage to find how to close a bit these 2 spacings in GTK.
   * :-/
   */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_grid_attach (grid, hbox, column, *row, 1, 1);
  /* These margin are by design to emphasize a bit the link list by
   * moving them a tiny bit to the right instead of being exactly
   * aligned with the top text.
   */
  gtk_widget_set_margin_start (hbox, 10);
  gtk_widget_set_visible (hbox, TRUE);

  ++(*row);

  icon = gtk_label_new (emoji);
  gtk_box_pack_start (GTK_BOX (hbox), icon, FALSE, FALSE, 0);
  gtk_widget_set_visible (icon, TRUE);

  button = gtk_link_button_new_with_label (link, title);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_set_visible (button, TRUE);
}

static void
welcome_size_allocate (GtkWidget     *welcome_dialog,
                       GtkAllocation *allocation,
                       gpointer       user_data)
{
  GtkWidget     *image = GTK_WIDGET (user_data);
  GError        *error = NULL;
  GFile         *splash_file;
  GdkPixbuf     *pixbuf;
  GdkMonitor    *monitor;
  GdkWindow     *gdk_window;
  GdkRectangle   workarea;
  gint           image_width;

  gdk_window = gtk_widget_get_window (welcome_dialog);
  if (gdk_window)
    monitor = gdk_display_get_monitor_at_window (gdk_display_get_default (), gdk_window);
  else
    monitor = gimp_get_monitor_at_pointer ();
  gdk_monitor_get_workarea (monitor, &workarea);

  if (gtk_image_get_storage_type (GTK_IMAGE (image)) == GTK_IMAGE_PIXBUF)
    {
      if (allocation->height > workarea.height - 10 &&
          ! g_object_get_data (G_OBJECT (welcome_dialog), "resized-once"))
        {
          g_object_set_data (G_OBJECT (welcome_dialog), "resized-once", GINT_TO_POINTER (TRUE));
          g_idle_add_full (G_PRIORITY_DEFAULT_IDLE,
                           welcome_scrollable_resize, NULL,
                           NULL);
        }
    return;
    }

  image_width = MAX (allocation->width - 2, workarea.width / 4);
  image_width = MIN (image_width, workarea.width / 3);
  /* Splash screens are fullHD. We should not load it bigger.
   * See: https://gitlab.gnome.org/GNOME/gimp-data/-/blob/main/images/README.md#requirements
   */
  image_width = MIN (image_width, 1920);

  splash_file = gimp_data_directory_file ("images", "gimp-splash.png", NULL);
  pixbuf = gdk_pixbuf_new_from_file_at_scale (g_file_peek_path (splash_file),
                                              image_width, -1,
                                              TRUE, &error);
  if (pixbuf)
    {
      gtk_image_set_from_pixbuf (GTK_IMAGE (image), pixbuf);
      g_object_unref (pixbuf);
    }
  else if (error)
    {
      g_printerr ("%s: %s\n", G_STRFUNC, error->message);
    }
  g_object_unref (splash_file);
  g_clear_error (&error);

  gtk_widget_set_visible (image, TRUE);

  gtk_window_set_resizable (GTK_WINDOW (welcome_dialog), FALSE);
}

static gboolean
welcome_scrollable_resize (gpointer data)
{
  if (welcome_dialog)
    {
      GtkWidget *prefs_box = g_object_get_data (G_OBJECT (welcome_dialog), "prefs-box");
      GtkWidget *main_vbox = g_object_get_data (G_OBJECT (welcome_dialog), "welcome-vbox");

      /* Make the first page scrollable to prevent height issues on
       * smaller screens */
      gimp_prefs_box_set_page_scrollable (GIMP_PREFS_BOX (prefs_box), main_vbox, TRUE);

      gtk_widget_queue_resize (GTK_WIDGET (welcome_dialog));
    }

  return G_SOURCE_REMOVE;
}

static void
welcome_dialog_new_image_accelerator (GtkAccelGroup  *accel_group,
                                      GObject        *accelerator_widget,
                                      guint           keyval,
                                      GdkModifierType mods,
                                      gpointer        user_data)
{
  GtkWidget *dialog = GTK_WIDGET (user_data);

  welcome_dialog_new_image_dialog (NULL, dialog);
}

static void
welcome_dialog_open_image_dialog_accelerator (GtkAccelGroup  *accel_group,
                                              GObject        *accelerator_widget,
                                              guint           keyval,
                                              GdkModifierType mods,
                                              gpointer        user_data)
{
  GtkWidget *dialog = GTK_WIDGET (user_data);

  welcome_dialog_open_image_dialog (NULL, dialog);
}

static void
welcome_dialog_open_image_accelerator (GtkAccelGroup  *accel_group,
                                       GObject        *accelerator_widget,
                                       guint           keyval,
                                       GdkModifierType mods,
                                       gpointer        user_data)
{
  Gimp          *gimp    = g_object_get_data (G_OBJECT (welcome_dialog), "gimp");
  GimpUIManager *manager = menus_get_image_manager_singleton (gimp);
  guint          index   = GPOINTER_TO_UINT (user_data);
  gchar          action_name[20];

  g_snprintf (action_name, sizeof (action_name), "file-open-recent-%02u", index + 1);

  if (gimp_ui_manager_activate_action (manager, "file", action_name))
    gtk_widget_destroy (welcome_dialog);
}
