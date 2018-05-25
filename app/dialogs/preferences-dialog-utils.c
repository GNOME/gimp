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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "dialogs-types.h"

#include "widgets/gimpcolorpanel.h"
#include "widgets/gimppropwidgets.h"
#include "widgets/gimpwidgets-constructors.h"

#include "preferences-dialog-utils.h"


GtkWidget *
prefs_frame_new (const gchar  *label,
                 GtkContainer *parent,
                 gboolean      expand)
{
  GtkWidget *frame;
  GtkWidget *vbox;

  frame = gimp_frame_new (label);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  if (GTK_IS_BOX (parent))
    gtk_box_pack_start (GTK_BOX (parent), frame, expand, expand, 0);
  else
    gtk_container_add (parent, frame);

  gtk_widget_show (frame);

  return vbox;
}

GtkWidget *
prefs_grid_new (GtkContainer *parent)
{
  GtkWidget *grid;

  grid = gtk_grid_new ();

  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);

  if (GTK_IS_BOX (parent))
    gtk_box_pack_start (GTK_BOX (parent), grid, FALSE, FALSE, 0);
  else
    gtk_container_add (parent, grid);

  gtk_widget_show (grid);

  return grid;
}

GtkWidget *
prefs_hint_box_new (const gchar  *icon_name,
                    const gchar  *text)
{
  GtkWidget *hbox;
  GtkWidget *image;
  GtkWidget *label;

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);

  image = gtk_image_new_from_icon_name (icon_name, GTK_ICON_SIZE_BUTTON);
  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
  gtk_widget_show (image);

  label = gtk_label_new (text);
  gimp_label_set_attributes (GTK_LABEL (label),
                             PANGO_ATTR_STYLE, PANGO_STYLE_ITALIC,
                             -1);
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);

  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
  gtk_widget_show (label);

  gtk_widget_show (hbox);

  return hbox;
}

GtkWidget *
prefs_button_add (const gchar *icon_name,
                  const gchar *label,
                  GtkBox      *box)
{
  GtkWidget *button;

  button = gimp_icon_button_new (icon_name, label);
  gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  return button;
}

GtkWidget *
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

GtkWidget *
prefs_check_button_add_with_icon (GObject      *config,
                                  const gchar  *property_name,
                                  const gchar  *label,
                                  const gchar  *icon_name,
                                  GtkBox       *vbox,
                                  GtkSizeGroup *group)
{
  GtkWidget *button;
  GtkWidget *hbox;
  GtkWidget *image;

  button = gimp_prop_check_button_new (config, property_name, label);
  if (!button)
    return NULL;

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (vbox, hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  image = gtk_image_new_from_icon_name (icon_name, GTK_ICON_SIZE_BUTTON);
  g_object_set (image,
                "margin-start",  2,
                "margin-end",    2,
                "margin-top",    2,
                "margin-bottom", 2,
                NULL);
  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
  gtk_widget_show (image);

  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  if (group)
    gtk_size_group_add_widget (group, image);

  return button;
}

GtkWidget *
prefs_widget_add_aligned (GtkWidget    *widget,
                          const gchar  *text,
                          GtkGrid      *grid,
                          gint          grid_top,
                          gboolean      left_align,
                          GtkSizeGroup *group)
{
  GtkWidget *label = gimp_grid_attach_aligned (grid, 0, grid_top,
                                               text, 0.0, 0.5,
                                               widget, 1);
  if (group)
    gtk_size_group_add_widget (group, label);

  if (left_align == TRUE)
    gtk_widget_set_halign (widget, GTK_ALIGN_START);

  return label;
}

GtkWidget *
prefs_color_button_add (GObject      *config,
                        const gchar  *property_name,
                        const gchar  *label,
                        const gchar  *title,
                        GtkGrid      *grid,
                        gint          grid_top,
                        GtkSizeGroup *group,
                        GimpContext  *context)
{
  GtkWidget  *button;
  GParamSpec *pspec;
  gboolean    has_alpha;

  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (config),
                                        property_name);

  has_alpha = gimp_param_spec_rgb_has_alpha (pspec);

  button = gimp_prop_color_button_new (config, property_name,
                                       title,
                                       PREFS_COLOR_BUTTON_WIDTH,
                                       PREFS_COLOR_BUTTON_HEIGHT,
                                       has_alpha ?
                                       GIMP_COLOR_AREA_SMALL_CHECKS :
                                       GIMP_COLOR_AREA_FLAT);

  if (button)
    {
      if (context)
        gimp_color_panel_set_context (GIMP_COLOR_PANEL (button), context);

      prefs_widget_add_aligned (button, label, grid, grid_top, TRUE, group);
    }

  return button;
}

GtkWidget *
prefs_entry_add (GObject      *config,
                 const gchar  *property_name,
                 const gchar  *label,
                 GtkGrid      *grid,
                 gint          grid_top,
                 GtkSizeGroup *group)
{
  GtkWidget *entry = gimp_prop_entry_new (config, property_name, -1);

  if (entry)
    prefs_widget_add_aligned (entry, label, grid, grid_top, FALSE, group);

  return entry;
}

GtkWidget *
prefs_spin_button_add (GObject      *config,
                       const gchar  *property_name,
                       gdouble       step_increment,
                       gdouble       page_increment,
                       gint          digits,
                       const gchar  *label,
                       GtkGrid      *grid,
                       gint          grid_top,
                       GtkSizeGroup *group)
{
  GtkWidget *button = gimp_prop_spin_button_new (config, property_name,
                                                 step_increment,
                                                 page_increment,
                                                 digits);

  if (button)
    prefs_widget_add_aligned (button, label, grid, grid_top, TRUE, group);

  return button;
}

GtkWidget *
prefs_memsize_entry_add (GObject      *config,
                         const gchar  *property_name,
                         const gchar  *label,
                         GtkGrid      *grid,
                         gint          grid_top,
                         GtkSizeGroup *group)
{
  GtkWidget *entry = gimp_prop_memsize_entry_new (config, property_name);

  if (entry)
    prefs_widget_add_aligned (entry, label, grid, grid_top, TRUE, group);

  return entry;
}

GtkWidget *
prefs_file_chooser_button_add (GObject      *config,
                               const gchar  *property_name,
                               const gchar  *label,
                               const gchar  *dialog_title,
                               GtkGrid      *grid,
                               gint          grid_top,
                               GtkSizeGroup *group)
{
  GtkWidget *button;

  button = gimp_prop_file_chooser_button_new (config, property_name,
                                              dialog_title,
                                              GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);

  if (button)
    prefs_widget_add_aligned (button, label, grid, grid_top, FALSE, group);

  return button;
}

GtkWidget *
prefs_enum_combo_box_add (GObject      *config,
                          const gchar  *property_name,
                          gint          minimum,
                          gint          maximum,
                          const gchar  *label,
                          GtkGrid      *grid,
                          gint          grid_top,
                          GtkSizeGroup *group)
{
  GtkWidget *combo = gimp_prop_enum_combo_box_new (config, property_name,
                                                   minimum, maximum);

  if (combo)
    prefs_widget_add_aligned (combo, label, grid, grid_top, FALSE, group);

  return combo;
}

GtkWidget *
prefs_boolean_combo_box_add (GObject      *config,
                             const gchar  *property_name,
                             const gchar  *true_text,
                             const gchar  *false_text,
                             const gchar  *label,
                             GtkGrid      *grid,
                             gint          grid_top,
                             GtkSizeGroup *group)
{
  GtkWidget *combo = gimp_prop_boolean_combo_box_new (config, property_name,
                                                      true_text, false_text);

  if (combo)
    prefs_widget_add_aligned (combo, label, grid, grid_top, FALSE, group);

  return combo;
}

#ifdef HAVE_ISO_CODES
GtkWidget *
prefs_language_combo_box_add (GObject      *config,
                              const gchar  *property_name,
                              GtkBox       *vbox)
{
  GtkWidget *combo = gimp_prop_language_combo_box_new (config, property_name);

  if (combo)
    {
      gtk_box_pack_start (vbox, combo, FALSE, FALSE, 0);
      gtk_widget_show (combo);
    }

  return combo;
}
#endif

GtkWidget *
prefs_profile_combo_box_add (GObject      *config,
                             const gchar  *property_name,
                             GtkListStore *profile_store,
                             const gchar  *dialog_title,
                             const gchar  *label,
                             GtkGrid      *grid,
                             gint          grid_top,
                             GtkSizeGroup *group,
                             GObject      *profile_path_config,
                             const gchar  *profile_path_property_name)
{
  GtkWidget *combo = gimp_prop_profile_combo_box_new (config,
                                                      property_name,
                                                      profile_store,
                                                      dialog_title,
                                                      profile_path_config,
                                                      profile_path_property_name);

  if (combo)
    prefs_widget_add_aligned (combo, label, grid, grid_top, FALSE, group);

  return combo;
}
