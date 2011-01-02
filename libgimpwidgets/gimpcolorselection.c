/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpcolorselection.c
 * Copyright (C) 2003 Michael Natterer <mitch@gimp.org>
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

#include "libgimpcolor/gimpcolor.h"
#include "libgimpconfig/gimpconfig.h"

#include "gimpwidgetstypes.h"

#include "gimpcolorarea.h"
#include "gimpcolornotebook.h"
#include "gimpcolorscales.h"
#include "gimpcolorselect.h"
#include "gimpcolorselection.h"
#include "gimphelpui.h"
#include "gimpicons.h"
#include "gimpwidgets.h"
#include "gimpwidgets-private.h"

#include "gimpwidgetsmarshal.h"

#include "libgimp/libgimp-intl.h"


/**
 * SECTION: gimpcolorselection
 * @title: GimpColorSelection
 * @short_description: Widget for doing a color selection.
 *
 * Widget for doing a color selection.
 **/


#define COLOR_AREA_SIZE  20


typedef enum
{
  UPDATE_NOTEBOOK  = 1 << 0,
  UPDATE_SCALES    = 1 << 1,
  UPDATE_ENTRY     = 1 << 2,
  UPDATE_COLOR     = 1 << 3
} UpdateType;

#define UPDATE_ALL (UPDATE_NOTEBOOK | \
                    UPDATE_SCALES   | \
                    UPDATE_ENTRY    | \
                    UPDATE_COLOR)

enum
{
  COLOR_CHANGED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_CONFIG
};


static void   gimp_color_selection_set_property      (GObject            *object,
                                                      guint               property_id,
                                                      const GValue       *value,
                                                      GParamSpec         *pspec);

static void   gimp_color_selection_switch_page       (GtkWidget          *widget,
                                                      gpointer            page,
                                                      guint               page_num,
                                                      GimpColorSelection *selection);
static void   gimp_color_selection_notebook_changed  (GimpColorSelector  *selector,
                                                      const GimpRGB      *rgb,
                                                      const GimpHSV      *hsv,
                                                      GimpColorSelection *selection);
static void   gimp_color_selection_scales_changed    (GimpColorSelector  *selector,
                                                      const GimpRGB      *rgb,
                                                      const GimpHSV      *hsv,
                                                      GimpColorSelection *selection);
static void   gimp_color_selection_color_picked      (GtkWidget          *widget,
                                                      const GimpRGB      *rgb,
                                                      GimpColorSelection *selection);
static void   gimp_color_selection_entry_changed     (GimpColorHexEntry  *entry,
                                                      GimpColorSelection *selection);
static void   gimp_color_selection_channel_changed   (GimpColorSelector  *selector,
                                                      GimpColorSelectorChannel channel,
                                                      GimpColorSelection *selection);
static void   gimp_color_selection_new_color_changed (GtkWidget          *widget,
                                                      GimpColorSelection *selection);

static void   gimp_color_selection_update            (GimpColorSelection *selection,
                                                      UpdateType          update);


G_DEFINE_TYPE (GimpColorSelection, gimp_color_selection, GTK_TYPE_BOX)

#define parent_class gimp_color_selection_parent_class

static guint selection_signals[LAST_SIGNAL] = { 0 };


static void
gimp_color_selection_class_init (GimpColorSelectionClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = gimp_color_selection_set_property;

  klass->color_changed       = NULL;

  g_object_class_install_property (object_class, PROP_CONFIG,
                                   g_param_spec_object ("config",
                                                        NULL, NULL,
                                                        GIMP_TYPE_COLOR_CONFIG,
                                                        G_PARAM_WRITABLE));

  selection_signals[COLOR_CHANGED] =
    g_signal_new ("color-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpColorSelectionClass, color_changed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

}

static void
gimp_color_selection_init (GimpColorSelection *selection)
{
  GtkWidget    *main_hbox;
  GtkWidget    *hbox;
  GtkWidget    *vbox;
  GtkWidget    *frame;
  GtkWidget    *label;
  GtkWidget    *entry;
  GtkWidget    *button;
  GtkSizeGroup *new_group;
  GtkSizeGroup *old_group;

  selection->show_alpha = TRUE;

  gtk_orientable_set_orientation (GTK_ORIENTABLE (selection),
                                  GTK_ORIENTATION_VERTICAL);

  gimp_rgba_set (&selection->rgb, 0.0, 0.0, 0.0, 1.0);
  gimp_rgb_to_hsv (&selection->rgb, &selection->hsv);

  selection->channel = GIMP_COLOR_SELECTOR_HUE;

  main_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (selection), main_hbox, TRUE, TRUE, 0);
  gtk_widget_show (main_hbox);

  /*  The left vbox with the notebook  */
  selection->left_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_box_pack_start (GTK_BOX (main_hbox), selection->left_vbox,
                      TRUE, TRUE, 0);
  gtk_widget_show (selection->left_vbox);

  if (_gimp_ensure_modules_func)
    {
      g_type_class_ref (GIMP_TYPE_COLOR_SELECT);
      _gimp_ensure_modules_func ();
    }

  selection->notebook = gimp_color_selector_new (GIMP_TYPE_COLOR_NOTEBOOK,
                                                 &selection->rgb,
                                                 &selection->hsv,
                                                 selection->channel);

  if (_gimp_ensure_modules_func)
    g_type_class_unref (g_type_class_peek (GIMP_TYPE_COLOR_SELECT));

  gimp_color_selector_set_toggles_visible
    (GIMP_COLOR_SELECTOR (selection->notebook), FALSE);
  gtk_box_pack_start (GTK_BOX (selection->left_vbox), selection->notebook,
                      TRUE, TRUE, 0);
  gtk_widget_show (selection->notebook);

  g_signal_connect (selection->notebook, "color-changed",
                    G_CALLBACK (gimp_color_selection_notebook_changed),
                    selection);
  g_signal_connect (gimp_color_notebook_get_notebook (GIMP_COLOR_NOTEBOOK (selection->notebook)),
                    "switch-page",
                    G_CALLBACK (gimp_color_selection_switch_page),
                    selection);

  /*  The hbox for the color_areas  */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_end (GTK_BOX (selection->left_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  /*  The labels  */
  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  label = gtk_label_new (_("Current:"));
  gtk_label_set_xalign (GTK_LABEL (label), 1.0);
  gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, TRUE, 0);
  gtk_widget_show (label);

  new_group = gtk_size_group_new (GTK_SIZE_GROUP_VERTICAL);
  gtk_size_group_add_widget (new_group, label);
  g_object_unref (new_group);

  label = gtk_label_new (_("Old:"));
  gtk_label_set_xalign (GTK_LABEL (label), 1.0);
  gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, TRUE, 0);
  gtk_widget_show (label);

  old_group = gtk_size_group_new (GTK_SIZE_GROUP_VERTICAL);
  gtk_size_group_add_widget (old_group, label);
  g_object_unref (old_group);

  /*  The color areas  */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  selection->new_color = gimp_color_area_new (&selection->rgb,
                                              selection->show_alpha ?
                                              GIMP_COLOR_AREA_SMALL_CHECKS :
                                              GIMP_COLOR_AREA_FLAT,
                                              GDK_BUTTON1_MASK |
                                              GDK_BUTTON2_MASK);
  gtk_size_group_add_widget (new_group, selection->new_color);
  gtk_box_pack_start (GTK_BOX (vbox), selection->new_color, FALSE, FALSE, 0);
  gtk_widget_show (selection->new_color);

  g_signal_connect (selection->new_color, "color-changed",
                    G_CALLBACK (gimp_color_selection_new_color_changed),
                    selection);

  selection->old_color = gimp_color_area_new (&selection->rgb,
                                              selection->show_alpha ?
                                              GIMP_COLOR_AREA_SMALL_CHECKS :
                                              GIMP_COLOR_AREA_FLAT,
                                              GDK_BUTTON1_MASK |
                                              GDK_BUTTON2_MASK);
  gtk_drag_dest_unset (selection->old_color);
  gtk_size_group_add_widget (old_group, selection->old_color);
  gtk_box_pack_start (GTK_BOX (vbox), selection->old_color, FALSE, FALSE, 0);
  gtk_widget_show (selection->old_color);

  /*  The right vbox with color scales  */
  selection->right_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_box_pack_start (GTK_BOX (main_hbox), selection->right_vbox,
                      TRUE, TRUE, 0);
  gtk_widget_show (selection->right_vbox);

  selection->scales = gimp_color_selector_new (GIMP_TYPE_COLOR_SCALES,
                                               &selection->rgb,
                                               &selection->hsv,
                                               selection->channel);
  gimp_color_selector_set_toggles_visible
    (GIMP_COLOR_SELECTOR (selection->scales), TRUE);
  gimp_color_selector_set_show_alpha (GIMP_COLOR_SELECTOR (selection->scales),
                                      selection->show_alpha);
  gtk_box_pack_start (GTK_BOX (selection->right_vbox), selection->scales,
                      TRUE, TRUE, 0);
  gtk_widget_show (selection->scales);

  g_signal_connect (selection->scales, "channel-changed",
                    G_CALLBACK (gimp_color_selection_channel_changed),
                    selection);
  g_signal_connect (selection->scales, "color-changed",
                    G_CALLBACK (gimp_color_selection_scales_changed),
                    selection);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (selection->right_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  /*  The color picker  */
  button = gimp_pick_button_new ();
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "color-picked",
                    G_CALLBACK (gimp_color_selection_color_picked),
                    selection);

  /* The hex triplet entry */
  entry = gimp_color_hex_entry_new ();
  gimp_help_set_help_data (entry,
                           _("Hexadecimal color notation as used in HTML and "
                             "CSS.  This entry also accepts CSS color names."),
                           NULL);
  gtk_box_pack_end (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
  gtk_widget_show (entry);

  label = gtk_label_new_with_mnemonic (_("HTML _notation:"));
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), entry);
  gtk_box_pack_end (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  g_object_set_data (G_OBJECT (selection), "color-hex-entry", entry);

  g_signal_connect (entry, "color-changed",
                    G_CALLBACK (gimp_color_selection_entry_changed),
                    selection);
}

static void
gimp_color_selection_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  GimpColorSelection *selection = GIMP_COLOR_SELECTION (object);

  switch (property_id)
    {
    case PROP_CONFIG:
      gimp_color_selection_set_config (selection, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


/**
 * gimp_color_selection_new:
 *
 * Creates a new #GimpColorSelection widget.
 *
 * Return value: The new #GimpColorSelection widget.
 **/
GtkWidget *
gimp_color_selection_new (void)
{
  return g_object_new (GIMP_TYPE_COLOR_SELECTION, NULL);
}

/**
 * gimp_color_selection_set_show_alpha:
 * @selection:  A #GimpColorSelection widget.
 * @show_alpha: The new @show_alpha setting.
 *
 * Sets the @show_alpha property of the @selection widget.
 **/
void
gimp_color_selection_set_show_alpha (GimpColorSelection *selection,
                                     gboolean            show_alpha)
{
  g_return_if_fail (GIMP_IS_COLOR_SELECTION (selection));

  if (show_alpha != selection->show_alpha)
    {
      selection->show_alpha = show_alpha ? TRUE : FALSE;

      gimp_color_selector_set_show_alpha
        (GIMP_COLOR_SELECTOR (selection->notebook), selection->show_alpha);
      gimp_color_selector_set_show_alpha
        (GIMP_COLOR_SELECTOR (selection->scales), selection->show_alpha);

      gimp_color_area_set_type (GIMP_COLOR_AREA (selection->new_color),
                                selection->show_alpha ?
                                GIMP_COLOR_AREA_SMALL_CHECKS :
                                GIMP_COLOR_AREA_FLAT);
      gimp_color_area_set_type (GIMP_COLOR_AREA (selection->old_color),
                                selection->show_alpha ?
                                GIMP_COLOR_AREA_SMALL_CHECKS :
                                GIMP_COLOR_AREA_FLAT);
    }
}

/**
 * gimp_color_selection_get_show_alpha:
 * @selection: A #GimpColorSelection widget.
 *
 * Returns the @selection's @show_alpha property.
 *
 * Return value: #TRUE if the #GimpColorSelection has alpha controls.
 **/
gboolean
gimp_color_selection_get_show_alpha (GimpColorSelection *selection)
{
  g_return_val_if_fail (GIMP_IS_COLOR_SELECTION (selection), FALSE);

  return selection->show_alpha;
}

/**
 * gimp_color_selection_set_color:
 * @selection: A #GimpColorSelection widget.
 * @color:     The @color to set as current color.
 *
 * Sets the #GimpColorSelection's current color to the new @color.
 **/
void
gimp_color_selection_set_color (GimpColorSelection *selection,
                                const GimpRGB      *color)
{
  g_return_if_fail (GIMP_IS_COLOR_SELECTION (selection));
  g_return_if_fail (color != NULL);

  selection->rgb = *color;
  gimp_rgb_to_hsv (&selection->rgb, &selection->hsv);

  gimp_color_selection_update (selection, UPDATE_ALL);

  gimp_color_selection_color_changed (selection);
}

/**
 * gimp_color_selection_get_color:
 * @selection: A #GimpColorSelection widget.
 * @color:     Return location for the @selection's current @color.
 *
 * This function returns the #GimpColorSelection's current color.
 **/
void
gimp_color_selection_get_color (GimpColorSelection *selection,
                                GimpRGB            *color)
{
  g_return_if_fail (GIMP_IS_COLOR_SELECTION (selection));
  g_return_if_fail (color != NULL);

  *color = selection->rgb;
}

/**
 * gimp_color_selection_set_old_color:
 * @selection: A #GimpColorSelection widget.
 * @color:     The @color to set as old color.
 *
 * Sets the #GimpColorSelection's old color.
 **/
void
gimp_color_selection_set_old_color (GimpColorSelection *selection,
                                    const GimpRGB      *color)
{
  g_return_if_fail (GIMP_IS_COLOR_SELECTION (selection));
  g_return_if_fail (color != NULL);

  gimp_color_area_set_color (GIMP_COLOR_AREA (selection->old_color), color);
}

/**
 * gimp_color_selection_get_old_color:
 * @selection: A #GimpColorSelection widget.
 * @color:     Return location for the @selection's old @color.
 *
 * This function returns the #GimpColorSelection's old color.
 **/
void
gimp_color_selection_get_old_color (GimpColorSelection *selection,
                                    GimpRGB            *color)
{
  g_return_if_fail (GIMP_IS_COLOR_SELECTION (selection));
  g_return_if_fail (color != NULL);

  gimp_color_area_get_color (GIMP_COLOR_AREA (selection->old_color), color);
}

/**
 * gimp_color_selection_reset:
 * @selection: A #GimpColorSelection widget.
 *
 * Sets the #GimpColorSelection's current color to its old color.
 **/
void
gimp_color_selection_reset (GimpColorSelection *selection)
{
  GimpRGB color;

  g_return_if_fail (GIMP_IS_COLOR_SELECTION (selection));

  gimp_color_area_get_color (GIMP_COLOR_AREA (selection->old_color), &color);
  gimp_color_selection_set_color (selection, &color);
}

/**
 * gimp_color_selection_color_changed:
 * @selection: A #GimpColorSelection widget.
 *
 * Emits the "color-changed" signal.
 **/
void
gimp_color_selection_color_changed (GimpColorSelection *selection)
{
  g_return_if_fail (GIMP_IS_COLOR_SELECTION (selection));

  g_signal_emit (selection, selection_signals[COLOR_CHANGED], 0);
}

/**
 * gimp_color_selection_set_config:
 * @selection: A #GimpColorSelection widget.
 * @config:    A #GimpColorConfig object.
 *
 * Sets the color management configuration to use with this color selection.
 *
 * Since: 2.4
 */
void
gimp_color_selection_set_config (GimpColorSelection *selection,
                                 GimpColorConfig    *config)
{
  g_return_if_fail (GIMP_IS_COLOR_SELECTION (selection));
  g_return_if_fail (config == NULL || GIMP_IS_COLOR_CONFIG (config));

  gimp_color_selector_set_config (GIMP_COLOR_SELECTOR (selection->notebook),
                                  config);
  gimp_color_selector_set_config (GIMP_COLOR_SELECTOR (selection->scales),
                                  config);
  gimp_color_area_set_color_config (GIMP_COLOR_AREA (selection->old_color),
                                    config);
  gimp_color_area_set_color_config (GIMP_COLOR_AREA (selection->new_color),
                                    config);
}

/*  private functions  */

static void
gimp_color_selection_switch_page (GtkWidget          *widget,
                                  gpointer            page,
                                  guint               page_num,
                                  GimpColorSelection *selection)
{
  GimpColorNotebook *notebook = GIMP_COLOR_NOTEBOOK (selection->notebook);
  GimpColorSelector *current;
  gboolean           sensitive;

  current = gimp_color_notebook_get_current_selector (notebook);

  sensitive = (GIMP_COLOR_SELECTOR_GET_CLASS (current)->set_channel != NULL);

  gimp_color_selector_set_toggles_sensitive
    (GIMP_COLOR_SELECTOR (selection->scales), sensitive);
}

static void
gimp_color_selection_notebook_changed (GimpColorSelector  *selector,
                                       const GimpRGB      *rgb,
                                       const GimpHSV      *hsv,
                                       GimpColorSelection *selection)
{
  selection->hsv = *hsv;
  selection->rgb = *rgb;

  gimp_color_selection_update (selection,
                               UPDATE_SCALES | UPDATE_ENTRY | UPDATE_COLOR);
  gimp_color_selection_color_changed (selection);
}

static void
gimp_color_selection_scales_changed (GimpColorSelector  *selector,
                                     const GimpRGB      *rgb,
                                     const GimpHSV      *hsv,
                                     GimpColorSelection *selection)
{
  selection->rgb = *rgb;
  selection->hsv = *hsv;

  gimp_color_selection_update (selection,
                               UPDATE_ENTRY | UPDATE_NOTEBOOK | UPDATE_COLOR);
  gimp_color_selection_color_changed (selection);
}

static void
gimp_color_selection_color_picked (GtkWidget          *widget,
                                   const GimpRGB      *rgb,
                                   GimpColorSelection *selection)
{
  gimp_color_selection_set_color (selection, rgb);
}

static void
gimp_color_selection_entry_changed (GimpColorHexEntry  *entry,
                                    GimpColorSelection *selection)
{
  gimp_color_hex_entry_get_color (entry, &selection->rgb);

  gimp_rgb_to_hsv (&selection->rgb, &selection->hsv);

  gimp_color_selection_update (selection,
                               UPDATE_NOTEBOOK | UPDATE_SCALES | UPDATE_COLOR);
  gimp_color_selection_color_changed (selection);
}

static void
gimp_color_selection_channel_changed (GimpColorSelector        *selector,
                                      GimpColorSelectorChannel  channel,
                                      GimpColorSelection       *selection)
{
  selection->channel = channel;

  gimp_color_selector_set_channel (GIMP_COLOR_SELECTOR (selection->notebook),
                                   selection->channel);
}

static void
gimp_color_selection_new_color_changed (GtkWidget          *widget,
                                        GimpColorSelection *selection)
{
  gimp_color_area_get_color (GIMP_COLOR_AREA (widget), &selection->rgb);
  gimp_rgb_to_hsv (&selection->rgb, &selection->hsv);

  gimp_color_selection_update (selection,
                               UPDATE_NOTEBOOK | UPDATE_SCALES | UPDATE_ENTRY);
  gimp_color_selection_color_changed (selection);
}

static void
gimp_color_selection_update (GimpColorSelection *selection,
                             UpdateType          update)
{
  if (update & UPDATE_NOTEBOOK)
    {
      g_signal_handlers_block_by_func (selection->notebook,
                                       gimp_color_selection_notebook_changed,
                                       selection);

      gimp_color_selector_set_color (GIMP_COLOR_SELECTOR (selection->notebook),
                                     &selection->rgb,
                                     &selection->hsv);

      g_signal_handlers_unblock_by_func (selection->notebook,
                                         gimp_color_selection_notebook_changed,
                                         selection);
    }

  if (update & UPDATE_SCALES)
    {
      g_signal_handlers_block_by_func (selection->scales,
                                       gimp_color_selection_scales_changed,
                                       selection);

      gimp_color_selector_set_color (GIMP_COLOR_SELECTOR (selection->scales),
                                     &selection->rgb,
                                     &selection->hsv);

      g_signal_handlers_unblock_by_func (selection->scales,
                                         gimp_color_selection_scales_changed,
                                         selection);
    }

  if (update & UPDATE_ENTRY)
    {
      GimpColorHexEntry *entry;

      entry = g_object_get_data (G_OBJECT (selection), "color-hex-entry");

      g_signal_handlers_block_by_func (entry,
                                       gimp_color_selection_entry_changed,
                                       selection);

      gimp_color_hex_entry_set_color (entry, &selection->rgb);

      g_signal_handlers_unblock_by_func (entry,
                                         gimp_color_selection_entry_changed,
                                         selection);
    }

  if (update & UPDATE_COLOR)
    {
      g_signal_handlers_block_by_func (selection->new_color,
                                       gimp_color_selection_new_color_changed,
                                       selection);

      gimp_color_area_set_color (GIMP_COLOR_AREA (selection->new_color),
                                 &selection->rgb);

      g_signal_handlers_unblock_by_func (selection->new_color,
                                         gimp_color_selection_new_color_changed,
                                         selection);
    }
}
