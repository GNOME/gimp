/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpcolorselection.c
 * Copyright (C) 2003 Michael Natterer <mitch@gimp.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"

#include "gimpwidgetstypes.h"

#include "gimpcolorarea.h"
#include "gimpcolornotebook.h"
#include "gimpcolorscales.h"
#include "gimpcolorselect.h"
#include "gimpcolorselection.h"
#include "gimphelpui.h"
#include "gimpstock.h"
#include "gimpwidgets.h"
#include "gimpwidgets-private.h"

#include "gimpwidgetsmarshal.h"

#include "libgimp/libgimp-intl.h"


#define COLOR_AREA_SIZE  20


typedef enum
{
  UPDATE_NOTEBOOK  = 1 << 0,
  UPDATE_SCALES    = 1 << 1,
  UPDATE_NEW_COLOR = 1 << 2
} UpdateType;

enum
{
  COLOR_CHANGED,
  LAST_SIGNAL
};


static void   gimp_color_selection_class_init (GimpColorSelectionClass *klass);
static void   gimp_color_selection_init       (GimpColorSelection      *selection);

static void   gimp_color_selection_switch_page       (GtkWidget          *widget,
                                                      GtkNotebookPage    *page,
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
static void   gimp_color_selection_channel_changed   (GimpColorSelector  *selector,
                                                      GimpColorSelectorChannel channel,
                                                      GimpColorSelection *selection);
static void   gimp_color_selection_new_color_changed (GtkWidget          *widget,
                                                      GimpColorSelection *selection);

static void   gimp_color_selection_update            (GimpColorSelection *selection,
                                                      UpdateType          update);


static GtkVBoxClass *parent_class = NULL;

static guint  selection_signals[LAST_SIGNAL] = { 0 };


GType
gimp_color_selection_get_type (void)
{
  static GType selection_type = 0;

  if (! selection_type)
    {
      static const GTypeInfo selection_info =
      {
        sizeof (GimpColorSelectionClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_color_selection_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpColorSelection),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_color_selection_init,
      };

      selection_type = g_type_register_static (GTK_TYPE_VBOX,
                                              "GimpColorSelection",
                                              &selection_info, 0);
    }

  return selection_type;
}

static void
gimp_color_selection_class_init (GimpColorSelectionClass *klass)
{
  parent_class = g_type_class_peek_parent (klass);

  selection_signals[COLOR_CHANGED] =
    g_signal_new ("color_changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpColorSelectionClass, color_changed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  klass->color_changed = NULL;
}

static void
gimp_color_selection_init (GimpColorSelection *selection)
{
  GtkWidget *main_hbox;
  GtkWidget *frame;
  GtkWidget *table;

  selection->show_alpha = TRUE;

  gimp_rgba_set (&selection->rgb, 0.0, 0.0, 0.0, 1.0);
  gimp_rgb_to_hsv (&selection->rgb, &selection->hsv);

  selection->channel = GIMP_COLOR_SELECTOR_HUE;

  main_hbox = gtk_hbox_new (FALSE, 6);
  gtk_container_add (GTK_CONTAINER (selection), main_hbox);
  gtk_widget_show (main_hbox);

  /*  The left vbox with the notebook  */
  selection->left_vbox = gtk_vbox_new (FALSE, 6);
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

  g_signal_connect (selection->notebook, "color_changed",
                    G_CALLBACK (gimp_color_selection_notebook_changed),
                    selection);
  g_signal_connect (GIMP_COLOR_NOTEBOOK (selection->notebook)->notebook,
                    "switch_page",
                    G_CALLBACK (gimp_color_selection_switch_page),
                    selection);

  /*  The table for the color_areas  */
  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_end (GTK_BOX (selection->left_vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /*  The new color area  */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_widget_set_size_request (frame, -1, COLOR_AREA_SIZE);

  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("Current:"), 1.0, 0.5,
			     frame, 1, FALSE);

  selection->new_color = gimp_color_area_new (&selection->rgb,
                                              selection->show_alpha ?
                                              GIMP_COLOR_AREA_SMALL_CHECKS :
                                              GIMP_COLOR_AREA_FLAT,
                                              GDK_BUTTON1_MASK |
                                              GDK_BUTTON2_MASK);
  gtk_container_add (GTK_CONTAINER (frame), selection->new_color);
  gtk_widget_show (selection->new_color);

  g_signal_connect (selection->new_color, "color_changed",
		    G_CALLBACK (gimp_color_selection_new_color_changed),
		    selection);

  /*  The old color area  */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_widget_set_size_request (frame, -1, COLOR_AREA_SIZE);

  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
			     _("Old:"), 1.0, 0.5,
			     frame, 1, FALSE);

  selection->old_color = gimp_color_area_new (&selection->rgb,
                                              selection->show_alpha ?
                                              GIMP_COLOR_AREA_SMALL_CHECKS :
                                              GIMP_COLOR_AREA_FLAT,
                                              GDK_BUTTON1_MASK |
                                              GDK_BUTTON2_MASK);
  gtk_drag_dest_unset (selection->old_color);
  gtk_container_add (GTK_CONTAINER (frame), selection->old_color);
  gtk_widget_show (selection->old_color);

  /*  The right vbox with color scales  */
  selection->right_vbox = gtk_vbox_new (FALSE, 6);
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

  g_signal_connect (selection->scales, "channel_changed",
                    G_CALLBACK (gimp_color_selection_channel_changed),
                    selection);
  g_signal_connect (selection->scales, "color_changed",
                    G_CALLBACK (gimp_color_selection_scales_changed),
                    selection);
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

  gimp_color_selection_update (selection,
                               UPDATE_NOTEBOOK   |
                               UPDATE_SCALES     |
                               UPDATE_NEW_COLOR);

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
 * Emits the "color_changed" signal.
 **/
void
gimp_color_selection_color_changed (GimpColorSelection *selection)
{
  g_return_if_fail (GIMP_IS_COLOR_SELECTION (selection));

  g_signal_emit (selection, selection_signals[COLOR_CHANGED], 0);
}


/*  private functions  */

static void
gimp_color_selection_switch_page (GtkWidget          *widget,
                                  GtkNotebookPage    *page,
                                  guint               page_num,
                                  GimpColorSelection *selection)
{
  GimpColorNotebook *notebook = GIMP_COLOR_NOTEBOOK (selection->notebook);
  gboolean           sensitive;

  sensitive =
    (GIMP_COLOR_SELECTOR_GET_CLASS (notebook->cur_page)->set_channel != NULL);

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

  gimp_color_selection_update (selection, UPDATE_SCALES | UPDATE_NEW_COLOR);
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

  gimp_color_selection_update (selection, UPDATE_NOTEBOOK | UPDATE_NEW_COLOR);
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

  gimp_color_selection_update (selection, UPDATE_NOTEBOOK | UPDATE_SCALES);
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

  if (update & UPDATE_NEW_COLOR)
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
