/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * color_notebook module (C) 1998 Austin Donnelly <austin@greenend.org.uk>
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "gui-types.h"

#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpviewabledialog.h"

#include "color-history.h"
#include "color-notebook.h"

#include "gimp-intl.h"


#define COLOR_AREA_SIZE  20


typedef enum
{
  UPDATE_NOTEBOOK   = 1 << 0,
  UPDATE_CHANNEL    = 1 << 1,
  UPDATE_SCALES     = 1 << 2,
  UPDATE_ORIG_COLOR = 1 << 3,
  UPDATE_NEW_COLOR  = 1 << 4,
  UPDATE_CALLER     = 1 << 5
} ColorNotebookUpdateType;


struct _ColorNotebook
{
  GtkWidget                *shell;

  GtkWidget                *notebook;
  GtkWidget                *scales;

  GtkWidget                *new_color;
  GtkWidget                *orig_color;
  GtkWidget                *history[COLOR_HISTORY_SIZE];

  GimpHSV                   hsv;
  GimpRGB                   rgb;

  GimpRGB                   orig_rgb;

  GimpColorSelectorChannel  active_channel;

  ColorNotebookCallback     callback;
  gpointer                  client_data;

  gboolean                  wants_updates;
  gboolean                  show_alpha;
};


static ColorNotebook *
              color_notebook_new_internal      (GimpViewable          *viewable,
                                                const gchar           *title,
                                                const gchar           *wmclass_name,
                                                const gchar           *stock_id,
                                                const gchar           *desc,
                                                GimpDialogFactory     *dialog_factory,
                                                const gchar           *dialog_identifier,
                                                const GimpRGB         *color,
                                                ColorNotebookCallback  callback,
                                                gpointer               client_data,
                                                gboolean               wants_updates,
                                                gboolean               show_alpha);

static void   color_notebook_help_func         (const gchar           *help_id,
                                                gpointer               help_data);

static void   color_notebook_response          (GtkWidget             *widget,
                                                gint                   response_id,
                                                ColorNotebook         *cnp);

static void   color_notebook_update            (ColorNotebook         *cnp,
                                                ColorNotebookUpdateType update);

static void   color_notebook_reset_clicked     (ColorNotebook         *cnp);
static void   color_notebook_new_color_changed (GtkWidget             *widget,
                                                ColorNotebook         *cnp);

static void   color_notebook_notebook_changed  (GimpColorSelector     *selector,
                                                const GimpRGB         *rgb,
                                                const GimpHSV         *hsv,
                                                ColorNotebook         *cnp);
static void   color_notebook_switch_page       (GtkWidget             *widget,
                                                GtkNotebookPage       *page,
                                                guint                  page_num,
                                                ColorNotebook         *cnp);

static void   color_notebook_channel_changed   (GimpColorSelector     *selector,
                                                GimpColorSelectorChannel channel,
                                                ColorNotebook         *cnp);
static void   color_notebook_scales_changed    (GimpColorSelector     *selector,
                                                const GimpRGB         *rgb,
                                                const GimpHSV         *hsv,
                                                ColorNotebook         *cnp);

static void   color_history_color_clicked      (GtkWidget             *widget,
                                                ColorNotebook         *cnp);
static void   color_history_color_changed      (GtkWidget             *widget,
                                                gpointer               data);
static void   color_history_add_clicked        (GtkWidget             *widget,
                                                ColorNotebook         *cnp);


static GList *color_notebooks = NULL;


/*  public functions  */

ColorNotebook *
color_notebook_new (const gchar           *title,
                    GimpDialogFactory     *dialog_factory,
                    const gchar           *dialog_identifier,
                    const GimpRGB         *color,
                    ColorNotebookCallback  callback,
                    gpointer               client_data,
                    gboolean               wants_updates,
                    gboolean               show_alpha)
{
  return color_notebook_new_internal (NULL,
                                      title,
                                      "color_selection",
                                      NULL,
                                      NULL,
                                      dialog_factory,
                                      dialog_identifier,
                                      color,
                                      callback, client_data,
                                      wants_updates, show_alpha);
}

ColorNotebook *
color_notebook_viewable_new (GimpViewable          *viewable,
                             const gchar           *title,
                             const gchar           *stock_id,
                             const gchar           *desc,
                             GimpDialogFactory     *dialog_factory,
                             const gchar           *dialog_identifier,
                             const GimpRGB         *color,
                             ColorNotebookCallback  callback,
                             gpointer               client_data,
                             gboolean               wants_updates,
                             gboolean               show_alpha)
{
  return color_notebook_new_internal (viewable,
                                      title,
                                      "color_selection",
                                      stock_id,
                                      desc,
                                      dialog_factory,
                                      dialog_identifier,
                                      color,
                                      callback, client_data,
                                      wants_updates, show_alpha);
}

void
color_notebook_free (ColorNotebook *cnp)
{
  g_return_if_fail (cnp != NULL);

  color_notebooks = g_list_remove (color_notebooks, cnp);

  /*  may be already destroyed by dialog factory  */
  if (cnp->shell)
    {
      g_object_remove_weak_pointer (G_OBJECT (cnp->shell),
                                    (gpointer *) &cnp->shell);
      gtk_widget_destroy (cnp->shell);
    }

  g_free (cnp);
}

void
color_notebook_set_viewable (ColorNotebook *cnb,
                             GimpViewable  *viewable)
{
  g_return_if_fail (cnb != NULL);

  if (GIMP_IS_VIEWABLE_DIALOG (cnb->shell))
    gimp_viewable_dialog_set_viewable (GIMP_VIEWABLE_DIALOG (cnb->shell),
                                       viewable);
}

void
color_notebook_set_title (ColorNotebook *cnb,
                          const gchar   *title)
{
  g_return_if_fail (cnb != NULL);
  g_return_if_fail (title != NULL);

  gtk_window_set_title (GTK_WINDOW (cnb->shell), title);
}

void
color_notebook_set_color (ColorNotebook *cnp,
			  const GimpRGB *color)
{
  g_return_if_fail (cnp != NULL);
  g_return_if_fail (color != NULL);

  cnp->rgb      = *color;
  cnp->orig_rgb = *color;

  gimp_rgb_to_hsv (&cnp->rgb, &cnp->hsv);

  color_notebook_update (cnp,
			 UPDATE_NOTEBOOK   |
                         UPDATE_SCALES     |
			 UPDATE_ORIG_COLOR |
			 UPDATE_NEW_COLOR);
}

void
color_notebook_get_color (ColorNotebook *cnp,
			  GimpRGB       *color)
{
  g_return_if_fail (cnp != NULL);
  g_return_if_fail (color != NULL);

  *color = cnp->rgb;
}

void
color_notebook_show (ColorNotebook *cnp)
{
  g_return_if_fail (cnp != NULL);

  gtk_window_present (GTK_WINDOW (cnp->shell));
}

void
color_notebook_hide (ColorNotebook *cnp)
{
  g_return_if_fail (cnp != NULL);

  gtk_widget_hide (cnp->shell);
}


/*  private functions  */

static ColorNotebook *
color_notebook_new_internal (GimpViewable          *viewable,
                             const gchar           *title,
                             const gchar           *role,
                             const gchar           *stock_id,
                             const gchar           *desc,
                             GimpDialogFactory     *dialog_factory,
                             const gchar           *dialog_identifier,
                             const GimpRGB         *color,
                             ColorNotebookCallback  callback,
                             gpointer               client_data,
                             gboolean               wants_updates,
                             gboolean               show_alpha)
{
  ColorNotebook *cnp;
  GtkWidget     *main_vbox;
  GtkWidget     *main_hbox;
  GtkWidget     *left_vbox;
  GtkWidget     *right_vbox;
  GtkWidget     *color_frame;
  GtkWidget     *table;
  GtkWidget     *button;
  GtkWidget     *image;
  GtkWidget     *arrow;
  gint           i;

  g_return_val_if_fail (dialog_factory == NULL ||
                        GIMP_IS_DIALOG_FACTORY (dialog_factory), NULL);
  g_return_val_if_fail (dialog_factory == NULL || dialog_identifier != NULL,
                        NULL);
  g_return_val_if_fail (color != NULL, NULL);

  cnp = g_new0 (ColorNotebook, 1);

  cnp->callback      = callback;
  cnp->client_data   = client_data;
  cnp->wants_updates = wants_updates;
  cnp->show_alpha    = show_alpha;

  cnp->rgb           = *color;
  cnp->orig_rgb      = *color;

  gimp_rgb_to_hsv (&cnp->rgb, &cnp->hsv);

  if (desc)
    {
      cnp->shell = gimp_viewable_dialog_new (viewable, title, role,
                                             stock_id, desc,
                                             color_notebook_help_func, NULL,
                                             NULL);

      gtk_window_set_resizable (GTK_WINDOW (cnp->shell), FALSE);
    }
  else
    {
      cnp->shell = gimp_dialog_new (title, role,
                                    NULL, 0,
                                    color_notebook_help_func, NULL,
                                    NULL);
   }

  g_object_set_data (G_OBJECT (cnp->shell), "color-notebook", cnp);

  gtk_dialog_add_buttons (GTK_DIALOG (cnp->shell),

                          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                          GTK_STOCK_OK,     GTK_RESPONSE_OK,

                          NULL);

  g_signal_connect (cnp->shell, "response",
                    G_CALLBACK (color_notebook_response),
                    cnp);

  g_object_add_weak_pointer (G_OBJECT (cnp->shell), (gpointer *) &cnp->shell);

  if (dialog_factory)
    gimp_dialog_factory_add_foreign (dialog_factory, dialog_identifier,
                                     cnp->shell);

  main_vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 6);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (cnp->shell)->vbox), main_vbox);
  gtk_widget_show (main_vbox);

  main_hbox = gtk_hbox_new (FALSE, 4);
  gtk_container_add (GTK_CONTAINER (main_vbox), main_hbox);
  gtk_widget_show (main_hbox);

  /*  The left vbox with the notebook  */
  left_vbox = gtk_vbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (main_hbox), left_vbox, FALSE, FALSE, 0);
  gtk_widget_show (left_vbox);

  /*  The right vbox with color areas and color space sliders  */
  right_vbox = gtk_vbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (main_hbox), right_vbox, TRUE, TRUE, 0);
  gtk_widget_show (right_vbox);

  cnp->notebook = gimp_color_selector_new (GIMP_TYPE_COLOR_NOTEBOOK,
                                           &cnp->rgb,
                                           &cnp->hsv,
                                           cnp->active_channel);
  gimp_color_selector_set_toggles_visible (GIMP_COLOR_SELECTOR (cnp->notebook),
                                           FALSE);
  gtk_box_pack_start (GTK_BOX (left_vbox), cnp->notebook, TRUE, TRUE, 0);
  gtk_widget_show (cnp->notebook);

  g_signal_connect (cnp->notebook, "color_changed",
                    G_CALLBACK (color_notebook_notebook_changed),
                    cnp);
  g_signal_connect (GIMP_COLOR_NOTEBOOK (cnp->notebook)->notebook,
                    "switch_page",
                    G_CALLBACK (color_notebook_switch_page),
                    cnp);

  /*  The table for the color_areas  */
  table = gtk_table_new (2, 3, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 1);
  gtk_table_set_col_spacings (GTK_TABLE (table), 1);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 3);
  gtk_box_pack_start (GTK_BOX (left_vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /*  The new color area  */
  color_frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (color_frame), GTK_SHADOW_IN);
  gtk_widget_set_size_request (color_frame, -1, COLOR_AREA_SIZE);

  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("Current:"), 1.0, 0.5,
			     color_frame, 1, FALSE);

  cnp->new_color =
    gimp_color_area_new (&cnp->rgb, 
			 show_alpha ? 
			 GIMP_COLOR_AREA_SMALL_CHECKS : GIMP_COLOR_AREA_FLAT,
			 GDK_BUTTON1_MASK | GDK_BUTTON2_MASK);
  gtk_container_add (GTK_CONTAINER (color_frame), cnp->new_color);
  gtk_widget_show (cnp->new_color);

  g_signal_connect (cnp->new_color, "color_changed",
		    G_CALLBACK (color_notebook_new_color_changed),
		    cnp);

  /*  The old color area  */
  color_frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (color_frame), GTK_SHADOW_IN);
  gtk_widget_set_size_request (color_frame, -1, COLOR_AREA_SIZE);

  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
			     _("Old:"), 1.0, 0.5,
			     color_frame, 1, FALSE);

  cnp->orig_color =
    gimp_color_area_new (&cnp->orig_rgb, 
			 show_alpha ? 
			 GIMP_COLOR_AREA_SMALL_CHECKS : GIMP_COLOR_AREA_FLAT,
			 GDK_BUTTON1_MASK | GDK_BUTTON2_MASK);
  gtk_drag_dest_unset (cnp->orig_color);
  gtk_container_add (GTK_CONTAINER (color_frame), cnp->orig_color);
  gtk_widget_show (cnp->orig_color);

  button = gtk_button_new ();
  gtk_table_attach (GTK_TABLE (table), button, 2, 3, 0, 2,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (button);

  image = gtk_image_new_from_stock (GIMP_STOCK_RESET, GTK_ICON_SIZE_MENU);
  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_show (image);

  gimp_help_set_help_data (button, _("Revert to old color"), NULL);

  g_signal_connect_swapped (button, "clicked",
                            G_CALLBACK (color_notebook_reset_clicked),
                            cnp);

  /*  The color space sliders, toggle buttons and entries  */
  cnp->scales = gimp_color_selector_new (GIMP_TYPE_COLOR_SCALES,
                                         &cnp->rgb,
                                         &cnp->hsv,
                                         cnp->active_channel);
  gimp_color_selector_set_toggles_visible (GIMP_COLOR_SELECTOR (cnp->scales),
                                           TRUE);
  gimp_color_selector_set_show_alpha (GIMP_COLOR_SELECTOR (cnp->scales),
                                      cnp->show_alpha);
  gtk_box_pack_start (GTK_BOX (right_vbox), cnp->scales, TRUE, TRUE, 0);
  gtk_widget_show (cnp->scales);

  g_signal_connect (cnp->scales, "channel_changed",
                    G_CALLBACK (color_notebook_channel_changed),
                    cnp);
  g_signal_connect (cnp->scales, "color_changed",
                    G_CALLBACK (color_notebook_scales_changed),
                    cnp);

  /* The color history */
  table = gtk_table_new (2, 9, TRUE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 1);
  gtk_table_set_col_spacings (GTK_TABLE (table), 1);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 3);
  gtk_box_pack_end (GTK_BOX (right_vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  button = gtk_button_new ();
  gtk_widget_set_size_request (button, COLOR_AREA_SIZE, COLOR_AREA_SIZE);
  gtk_table_attach_defaults (GTK_TABLE (table), button, 0, 1, 0, 1);
  gimp_help_set_help_data (button,
			   _("Add the current color to the color history"),
			   NULL);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
		    G_CALLBACK (color_history_add_clicked),
		    cnp);

  arrow = gtk_arrow_new (GTK_ARROW_RIGHT, GTK_SHADOW_OUT);
  gtk_container_add (GTK_CONTAINER (button), arrow);
  gtk_widget_show (arrow);

  for (i = 0; i < COLOR_HISTORY_SIZE; i++)
    {
      GimpRGB history_color;

      color_history_get (i, &history_color);

      button = gtk_button_new ();
      gtk_widget_set_size_request (button, COLOR_AREA_SIZE, COLOR_AREA_SIZE);
      gtk_table_attach_defaults (GTK_TABLE (table), button,
				 (i > 7 ? i - 8 : i) + 1,
				 (i > 7 ? i - 8 : i) + 2,
				 i > 7 ? 1 : 0,
				 i > 7 ? 2 : 1);

      cnp->history[i] = gimp_color_area_new (&history_color,
					     GIMP_COLOR_AREA_SMALL_CHECKS,
					     GDK_BUTTON2_MASK);
      gtk_container_add (GTK_CONTAINER (button), cnp->history[i]);
      gtk_widget_show (cnp->history[i]);
      gtk_widget_show (button);

      g_signal_connect (button, "clicked",
			G_CALLBACK (color_history_color_clicked),
			cnp);

      g_signal_connect (cnp->history[i], "color_changed",
			G_CALLBACK (color_history_color_changed),
			GINT_TO_POINTER (i));
    }

  gtk_widget_show (cnp->shell);

  color_notebooks = g_list_prepend (color_notebooks, cnp);

  return cnp;
}

static void
color_notebook_help_func (const gchar *help_id,
                          gpointer     help_data)
{
  ColorNotebook     *cnp;
  GimpColorNotebook *notebook;

  cnp = g_object_get_data (G_OBJECT (help_data), "color-notebook");

  notebook = GIMP_COLOR_NOTEBOOK (cnp->notebook);

  help_id = GIMP_COLOR_SELECTOR_GET_CLASS (notebook->cur_page)->help_page;

  gimp_standard_help_func (help_id, NULL);
}

static void
color_notebook_response (GtkWidget     *widget,
                         gint           response_id,
                         ColorNotebook *cnp)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      color_history_add_clicked (NULL, cnp);

      if (cnp->callback)
        cnp->callback (cnp,
                       &cnp->rgb,
                       COLOR_NOTEBOOK_OK,
                       cnp->client_data);
    }
  else
    {
      if (cnp->callback)
        cnp->callback (cnp,
                       &cnp->orig_rgb,
                       COLOR_NOTEBOOK_CANCEL,
                       cnp->client_data);
    }
}

static void
color_notebook_update (ColorNotebook           *cnp,
		       ColorNotebookUpdateType  update)
{
  if (!cnp)
    return;

  if (update & UPDATE_NOTEBOOK)
    {
      g_signal_handlers_block_by_func (cnp->notebook,
                                       color_notebook_notebook_changed,
                                       cnp);

      gimp_color_selector_set_color (GIMP_COLOR_SELECTOR (cnp->notebook),
                                     &cnp->rgb,
                                     &cnp->hsv);

      g_signal_handlers_unblock_by_func (cnp->notebook,
                                         color_notebook_notebook_changed,
                                         cnp);
    }

  if (update & UPDATE_CHANNEL)
    gimp_color_selector_set_channel (GIMP_COLOR_SELECTOR (cnp->notebook),
                                     cnp->active_channel);

  if (update & UPDATE_SCALES)
    {
      g_signal_handlers_block_by_func (cnp->scales,
                                       color_notebook_scales_changed,
                                       cnp);

      gimp_color_selector_set_color (GIMP_COLOR_SELECTOR (cnp->scales),
                                     &cnp->rgb,
                                     &cnp->hsv);

      g_signal_handlers_unblock_by_func (cnp->scales,
                                         color_notebook_scales_changed,
                                         cnp);
    }

  if (update & UPDATE_ORIG_COLOR)
    gimp_color_area_set_color (GIMP_COLOR_AREA (cnp->orig_color), 
                               &cnp->orig_rgb);

  if (update & UPDATE_NEW_COLOR)
    {
      g_signal_handlers_block_by_func (cnp->new_color,
				       color_notebook_new_color_changed,
				       cnp);

      gimp_color_area_set_color (GIMP_COLOR_AREA (cnp->new_color), 
				 &cnp->rgb);

      g_signal_handlers_unblock_by_func (cnp->new_color,
					 color_notebook_new_color_changed,
					 cnp);
    }

  if (update & UPDATE_CALLER)
    {
      if (cnp->wants_updates && cnp->callback)
        cnp->callback (cnp,
                       &cnp->rgb,
                       COLOR_NOTEBOOK_UPDATE,
                       cnp->client_data);
    }
}

static void
color_notebook_reset_clicked (ColorNotebook *cnp)
{
  cnp->rgb = cnp->orig_rgb;
  gimp_rgb_to_hsv (&cnp->rgb, &cnp->hsv);

  color_notebook_update (cnp,
			 UPDATE_NOTEBOOK  |
                         UPDATE_SCALES    |
			 UPDATE_NEW_COLOR |
			 UPDATE_CALLER);
}

static void
color_notebook_new_color_changed (GtkWidget     *widget,
                                  ColorNotebook *cnp)
{
  gimp_color_area_get_color (GIMP_COLOR_AREA (widget), &cnp->rgb);
  gimp_rgb_to_hsv (&cnp->rgb, &cnp->hsv);

  color_notebook_update (cnp,
			 UPDATE_NOTEBOOK  |
                         UPDATE_SCALES    |
			 UPDATE_NEW_COLOR |
			 UPDATE_CALLER);
}


/*  color notebook callbacks  */

static void
color_notebook_notebook_changed (GimpColorSelector *selector,
                                 const GimpRGB     *rgb,
                                 const GimpHSV     *hsv,
                                 ColorNotebook     *cnp)
{
  cnp->hsv = *hsv;
  cnp->rgb = *rgb;

  color_notebook_update (cnp,
                         UPDATE_SCALES    |
			 UPDATE_NEW_COLOR |
			 UPDATE_CALLER);
}

static void
color_notebook_switch_page (GtkWidget       *widget,
                            GtkNotebookPage *page,
                            guint            page_num,
                            ColorNotebook   *cnp)
{
  GimpColorNotebook *notebook;
  gboolean           set_channel;

  notebook = GIMP_COLOR_NOTEBOOK (cnp->notebook);

  set_channel =
    (GIMP_COLOR_SELECTOR_GET_CLASS (notebook->cur_page)->set_channel != NULL);

  gimp_color_selector_set_toggles_sensitive (GIMP_COLOR_SELECTOR (cnp->scales),
                                             set_channel);
}


/*  color scales callbacks  */

static void
color_notebook_channel_changed (GimpColorSelector        *selector,
                                GimpColorSelectorChannel  channel,
                                ColorNotebook            *cnp)
{
  cnp->active_channel = channel;

  color_notebook_update (cnp, UPDATE_CHANNEL);
}

static void
color_notebook_scales_changed (GimpColorSelector *selector,
                               const GimpRGB     *rgb,
                               const GimpHSV     *hsv,
                               ColorNotebook     *cnp)
{
  cnp->rgb = *rgb;
  cnp->hsv = *hsv;

  color_notebook_update (cnp,
			 UPDATE_NOTEBOOK  |
			 UPDATE_NEW_COLOR |
			 UPDATE_CALLER);
}


/*  color history callbacks  */

static void
color_history_color_clicked (GtkWidget     *widget,
			     ColorNotebook *cnp)
{
  GimpColorArea *color_area;

  color_area = GIMP_COLOR_AREA (GTK_BIN (widget)->child);

  gimp_color_area_get_color (color_area, &cnp->rgb);
  gimp_rgb_to_hsv (&cnp->rgb, &cnp->hsv);

  color_notebook_update (cnp,
			 UPDATE_NOTEBOOK   |
                         UPDATE_SCALES     |
			 UPDATE_ORIG_COLOR |
			 UPDATE_NEW_COLOR  |
			 UPDATE_CALLER);
}

static void
color_history_color_changed (GtkWidget *widget,
			     gpointer   data)
{
  GimpRGB        changed_color;
  gint           color_index;
  GList         *list;
  ColorNotebook *notebook;

  color_index = GPOINTER_TO_INT (data);

  gimp_color_area_get_color (GIMP_COLOR_AREA (widget), &changed_color);

  color_history_set (color_index, &changed_color);

  for (list = color_notebooks; list; list = g_list_next (list))
    {
      notebook = (ColorNotebook *) list->data;

      if (notebook->history[color_index] == widget)
        continue;

      g_signal_handlers_block_by_func (notebook->history[color_index],
                                       color_history_color_changed,
                                       data);

      gimp_color_area_set_color
        (GIMP_COLOR_AREA (notebook->history[color_index]), &changed_color);

      g_signal_handlers_unblock_by_func (notebook->history[color_index],
                                         color_history_color_changed,
                                         data);
    }
}

static void
color_history_add_clicked (GtkWidget     *widget,
			   ColorNotebook *cnp)
{
  gint shift_begin;
  gint i;

  shift_begin = color_history_add (&cnp->rgb);

  for (i = shift_begin; i >= 0; i--)
    {
      GimpRGB color;

      color_history_get (i, &color);

      gimp_color_area_set_color (GIMP_COLOR_AREA (cnp->history[i]), &color);
    }
}
