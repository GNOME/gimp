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


typedef struct _GimpColorSelectionPrivate GimpColorSelectionPrivate;

struct _GimpColorSelectionPrivate
{
  gboolean                  show_alpha;

  GimpHSV                   hsv;
  GimpRGB                   rgb;
  GimpColorSelectorChannel  channel;

  GtkWidget                *left_vbox;
  GtkWidget                *right_vbox;

  GtkWidget                *notebook;
  GtkWidget                *scales;

  GtkWidget                *new_color;
  GtkWidget                *old_color;
};

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), \
                                                       GIMP_TYPE_COLOR_SELECTION, \
                                                       GimpColorSelectionPrivate))


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

  g_type_class_add_private (object_class, sizeof (GimpColorSelectionPrivate));
}

static void
gimp_color_selection_init (GimpColorSelection *selection)
{
  GimpColorSelectionPrivate *priv = GET_PRIVATE (selection);
  GtkWidget                 *main_hbox;
  GtkWidget                 *hbox;
  GtkWidget                 *vbox;
  GtkWidget                 *frame;
  GtkWidget                 *label;
  GtkWidget                 *entry;
  GtkWidget                 *button;
  GtkSizeGroup              *new_group;
  GtkSizeGroup              *old_group;

  priv->show_alpha = TRUE;

  gtk_orientable_set_orientation (GTK_ORIENTABLE (selection),
                                  GTK_ORIENTATION_VERTICAL);

  gimp_rgba_set (&priv->rgb, 0.0, 0.0, 0.0, 1.0);
  gimp_rgb_to_hsv (&priv->rgb, &priv->hsv);

  priv->channel = GIMP_COLOR_SELECTOR_HUE;

  main_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (selection), main_hbox, TRUE, TRUE, 0);
  gtk_widget_show (main_hbox);

  /*  The left vbox with the notebook  */
  priv->left_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_box_pack_start (GTK_BOX (main_hbox), priv->left_vbox,
                      TRUE, TRUE, 0);
  gtk_widget_show (priv->left_vbox);

  if (_gimp_ensure_modules_func)
    {
      g_type_class_ref (GIMP_TYPE_COLOR_SELECT);
      _gimp_ensure_modules_func ();
    }

  priv->notebook = gimp_color_selector_new (GIMP_TYPE_COLOR_NOTEBOOK,
                                            &priv->rgb,
                                            &priv->hsv,
                                            priv->channel);

  if (_gimp_ensure_modules_func)
    g_type_class_unref (g_type_class_peek (GIMP_TYPE_COLOR_SELECT));

  gimp_color_selector_set_toggles_visible
    (GIMP_COLOR_SELECTOR (priv->notebook), FALSE);
  gtk_box_pack_start (GTK_BOX (priv->left_vbox), priv->notebook,
                      TRUE, TRUE, 0);
  gtk_widget_show (priv->notebook);

  g_signal_connect (priv->notebook, "color-changed",
                    G_CALLBACK (gimp_color_selection_notebook_changed),
                    selection);
  g_signal_connect (gimp_color_notebook_get_notebook (GIMP_COLOR_NOTEBOOK (priv->notebook)),
                    "switch-page",
                    G_CALLBACK (gimp_color_selection_switch_page),
                    selection);

  /*  The hbox for the color_areas  */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_end (GTK_BOX (priv->left_vbox), hbox, FALSE, FALSE, 0);
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

  priv->new_color = gimp_color_area_new (&priv->rgb,
                                         priv->show_alpha ?
                                         GIMP_COLOR_AREA_SMALL_CHECKS :
                                         GIMP_COLOR_AREA_FLAT,
                                         GDK_BUTTON1_MASK |
                                         GDK_BUTTON2_MASK);
  gtk_size_group_add_widget (new_group, priv->new_color);
  gtk_box_pack_start (GTK_BOX (vbox), priv->new_color, FALSE, FALSE, 0);
  gtk_widget_show (priv->new_color);

  g_signal_connect (priv->new_color, "color-changed",
                    G_CALLBACK (gimp_color_selection_new_color_changed),
                    selection);

  priv->old_color = gimp_color_area_new (&priv->rgb,
                                         priv->show_alpha ?
                                         GIMP_COLOR_AREA_SMALL_CHECKS :
                                         GIMP_COLOR_AREA_FLAT,
                                         GDK_BUTTON1_MASK |
                                         GDK_BUTTON2_MASK);
  gtk_drag_dest_unset (priv->old_color);
  gtk_size_group_add_widget (old_group, priv->old_color);
  gtk_box_pack_start (GTK_BOX (vbox), priv->old_color, FALSE, FALSE, 0);
  gtk_widget_show (priv->old_color);

  /*  The right vbox with color scales  */
  priv->right_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_box_pack_start (GTK_BOX (main_hbox), priv->right_vbox,
                      TRUE, TRUE, 0);
  gtk_widget_show (priv->right_vbox);

  priv->scales = gimp_color_selector_new (GIMP_TYPE_COLOR_SCALES,
                                          &priv->rgb,
                                          &priv->hsv,
                                          priv->channel);
  gimp_color_selector_set_toggles_visible
    (GIMP_COLOR_SELECTOR (priv->scales), TRUE);
  gimp_color_selector_set_show_alpha (GIMP_COLOR_SELECTOR (priv->scales),
                                      priv->show_alpha);
  gtk_box_pack_start (GTK_BOX (priv->right_vbox), priv->scales,
                      TRUE, TRUE, 0);
  gtk_widget_show (priv->scales);

  g_signal_connect (priv->scales, "channel-changed",
                    G_CALLBACK (gimp_color_selection_channel_changed),
                    selection);
  g_signal_connect (priv->scales, "color-changed",
                    G_CALLBACK (gimp_color_selection_scales_changed),
                    selection);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (priv->right_vbox), hbox, FALSE, FALSE, 0);
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
  GimpColorSelectionPrivate *priv;

  g_return_if_fail (GIMP_IS_COLOR_SELECTION (selection));

  priv = GET_PRIVATE (selection);

  if (show_alpha != priv->show_alpha)
    {
      priv->show_alpha = show_alpha ? TRUE : FALSE;

      gimp_color_selector_set_show_alpha
        (GIMP_COLOR_SELECTOR (priv->notebook), priv->show_alpha);
      gimp_color_selector_set_show_alpha
        (GIMP_COLOR_SELECTOR (priv->scales), priv->show_alpha);

      gimp_color_area_set_type (GIMP_COLOR_AREA (priv->new_color),
                                priv->show_alpha ?
                                GIMP_COLOR_AREA_SMALL_CHECKS :
                                GIMP_COLOR_AREA_FLAT);
      gimp_color_area_set_type (GIMP_COLOR_AREA (priv->old_color),
                                priv->show_alpha ?
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

  return GET_PRIVATE (selection)->show_alpha;
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
  GimpColorSelectionPrivate *priv;

  g_return_if_fail (GIMP_IS_COLOR_SELECTION (selection));
  g_return_if_fail (color != NULL);

  priv = GET_PRIVATE (selection);

  priv->rgb = *color;
  gimp_rgb_to_hsv (&priv->rgb, &priv->hsv);

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

  *color = GET_PRIVATE (selection)->rgb;
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
  GimpColorSelectionPrivate *priv;

  g_return_if_fail (GIMP_IS_COLOR_SELECTION (selection));
  g_return_if_fail (color != NULL);

  priv = GET_PRIVATE (selection);

  gimp_color_area_set_color (GIMP_COLOR_AREA (priv->old_color), color);
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
  GimpColorSelectionPrivate *priv;

  g_return_if_fail (GIMP_IS_COLOR_SELECTION (selection));
  g_return_if_fail (color != NULL);

  priv = GET_PRIVATE (selection);

  gimp_color_area_get_color (GIMP_COLOR_AREA (priv->old_color), color);
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
  GimpColorSelectionPrivate *priv;
  GimpRGB                    color;

  g_return_if_fail (GIMP_IS_COLOR_SELECTION (selection));

  priv = GET_PRIVATE (selection);

  gimp_color_area_get_color (GIMP_COLOR_AREA (priv->old_color), &color);
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
  GimpColorSelectionPrivate *priv;

  g_return_if_fail (GIMP_IS_COLOR_SELECTION (selection));
  g_return_if_fail (config == NULL || GIMP_IS_COLOR_CONFIG (config));

  priv = GET_PRIVATE (selection);

  gimp_color_selector_set_config (GIMP_COLOR_SELECTOR (priv->notebook),
                                  config);
  gimp_color_selector_set_config (GIMP_COLOR_SELECTOR (priv->scales),
                                  config);
  gimp_color_area_set_color_config (GIMP_COLOR_AREA (priv->old_color),
                                    config);
  gimp_color_area_set_color_config (GIMP_COLOR_AREA (priv->new_color),
                                    config);
}

/**
 * gimp_color_selection_get_notebook:
 * @selection: A #GimpColorSelection widget.
 *
 * Returns: The selection's #GimpColorNotebook.
 *
 * Since: GIMP 3.0
 */
GtkWidget *
gimp_color_selection_get_notebook (GimpColorSelection *selection)
{
  g_return_val_if_fail (GIMP_IS_COLOR_SELECTION (selection), NULL);

  return GET_PRIVATE (selection)->notebook;
}

/**
 * gimp_color_selection_get_right_vbox:
 * @selection: A #GimpColorSelection widget.
 *
 * Returns: The selection's right #GtkBox which contains the color scales.
 *
 * Since: GIMP 3.0
 */
GtkWidget *
gimp_color_selection_get_right_vbox (GimpColorSelection *selection)
{
  g_return_val_if_fail (GIMP_IS_COLOR_SELECTION (selection), NULL);

  return GET_PRIVATE (selection)->right_vbox;
}


/*  private functions  */

static void
gimp_color_selection_switch_page (GtkWidget          *widget,
                                  gpointer            page,
                                  guint               page_num,
                                  GimpColorSelection *selection)
{
  GimpColorSelectionPrivate *priv     = GET_PRIVATE (selection);
  GimpColorNotebook         *notebook = GIMP_COLOR_NOTEBOOK (priv->notebook);
  GimpColorSelector         *current;
  gboolean                   sensitive;

  current = gimp_color_notebook_get_current_selector (notebook);

  sensitive = (GIMP_COLOR_SELECTOR_GET_CLASS (current)->set_channel != NULL);

  gimp_color_selector_set_toggles_sensitive
    (GIMP_COLOR_SELECTOR (priv->scales), sensitive);
}

static void
gimp_color_selection_notebook_changed (GimpColorSelector  *selector,
                                       const GimpRGB      *rgb,
                                       const GimpHSV      *hsv,
                                       GimpColorSelection *selection)
{
  GimpColorSelectionPrivate *priv = GET_PRIVATE (selection);

  priv->hsv = *hsv;
  priv->rgb = *rgb;

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
  GimpColorSelectionPrivate *priv = GET_PRIVATE (selection);

  priv->rgb = *rgb;
  priv->hsv = *hsv;

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
  GimpColorSelectionPrivate *priv = GET_PRIVATE (selection);

  gimp_color_hex_entry_get_color (entry, &priv->rgb);

  gimp_rgb_to_hsv (&priv->rgb, &priv->hsv);

  gimp_color_selection_update (selection,
                               UPDATE_NOTEBOOK | UPDATE_SCALES | UPDATE_COLOR);
  gimp_color_selection_color_changed (selection);
}

static void
gimp_color_selection_channel_changed (GimpColorSelector        *selector,
                                      GimpColorSelectorChannel  channel,
                                      GimpColorSelection       *selection)
{
  GimpColorSelectionPrivate *priv = GET_PRIVATE (selection);

  priv->channel = channel;

  gimp_color_selector_set_channel (GIMP_COLOR_SELECTOR (priv->notebook),
                                   priv->channel);
}

static void
gimp_color_selection_new_color_changed (GtkWidget          *widget,
                                        GimpColorSelection *selection)
{
  GimpColorSelectionPrivate *priv = GET_PRIVATE (selection);

  gimp_color_area_get_color (GIMP_COLOR_AREA (widget), &priv->rgb);
  gimp_rgb_to_hsv (&priv->rgb, &priv->hsv);

  gimp_color_selection_update (selection,
                               UPDATE_NOTEBOOK | UPDATE_SCALES | UPDATE_ENTRY);
  gimp_color_selection_color_changed (selection);
}

static void
gimp_color_selection_update (GimpColorSelection *selection,
                             UpdateType          update)
{
  GimpColorSelectionPrivate *priv = GET_PRIVATE (selection);

  if (update & UPDATE_NOTEBOOK)
    {
      g_signal_handlers_block_by_func (priv->notebook,
                                       gimp_color_selection_notebook_changed,
                                       selection);

      gimp_color_selector_set_color (GIMP_COLOR_SELECTOR (priv->notebook),
                                     &priv->rgb,
                                     &priv->hsv);

      g_signal_handlers_unblock_by_func (priv->notebook,
                                         gimp_color_selection_notebook_changed,
                                         selection);
    }

  if (update & UPDATE_SCALES)
    {
      g_signal_handlers_block_by_func (priv->scales,
                                       gimp_color_selection_scales_changed,
                                       selection);

      gimp_color_selector_set_color (GIMP_COLOR_SELECTOR (priv->scales),
                                     &priv->rgb,
                                     &priv->hsv);

      g_signal_handlers_unblock_by_func (priv->scales,
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

      gimp_color_hex_entry_set_color (entry, &priv->rgb);

      g_signal_handlers_unblock_by_func (entry,
                                         gimp_color_selection_entry_changed,
                                         selection);
    }

  if (update & UPDATE_COLOR)
    {
      g_signal_handlers_block_by_func (priv->new_color,
                                       gimp_color_selection_new_color_changed,
                                       selection);

      gimp_color_area_set_color (GIMP_COLOR_AREA (priv->new_color),
                                 &priv->rgb);

      g_signal_handlers_unblock_by_func (priv->new_color,
                                         gimp_color_selection_new_color_changed,
                                         selection);
    }
}
