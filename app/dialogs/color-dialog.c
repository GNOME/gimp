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

#include <gmodule.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"
#include "libgimp/gimpcolorselector.h"

#include "gui-types.h"

#include "color-area.h"
#include "color-notebook.h"

#include "libgimp/gimpintl.h"


#define COLOR_AREA_SIZE    20
#define COLOR_HISTORY_SIZE 16

typedef enum
{
  UPDATE_NOTEBOOK   = 1 << 0,
  UPDATE_CHANNEL    = 1 << 1,
  UPDATE_NEW_COLOR  = 1 << 2,
  UPDATE_ORIG_COLOR = 1 << 3,
  UPDATE_CALLER     = 1 << 4
} ColorNotebookUpdateType;


/* "class" information we keep on each registered color selector */

typedef struct _ColorSelectorInfo ColorSelectorInfo;

struct _ColorSelectorInfo
{
  gchar                       *name;    /* label used in notebook tab */
  gchar                       *help_page;
  GimpColorSelectorMethods     methods;
  gint                         refs;    /* number of instances around */
  gboolean                     active;
  GimpColorSelectorFinishedCB  death_callback;
  gpointer                     death_data;

  ColorSelectorInfo           *next;
};


/* "instance" information we keep on each notebook tab */

typedef struct _ColorSelectorInstance ColorSelectorInstance;

struct _ColorSelectorInstance
{
  ColorNotebook         *color_notebook;
  ColorSelectorInfo     *info;
  GtkWidget             *frame;   /* main widget */
  gpointer               selector_data;

  ColorSelectorInstance *next;
};


struct _ColorNotebook
{
  GtkWidget                *shell;
  GtkWidget                *notebook;

  GtkWidget                *new_color;
  GtkWidget                *orig_color;
  GtkWidget                *toggles[7];
  GtkObject                *slider_data[7];
  GtkWidget                *hex_entry;

  GtkWidget                *history[COLOR_HISTORY_SIZE];

  GimpHSV                   hsv;
  GimpRGB                   rgb;

  GimpRGB                   orig_rgb;

  GimpColorSelectorChannelType  active_channel;

  ColorNotebookCallback     callback;
  gpointer                  client_data;

  gboolean                  wants_updates;
  gboolean                  show_alpha;

  ColorSelectorInstance    *selectors;
  ColorSelectorInstance    *cur_page;
};


static void       color_notebook_ok_callback     (GtkWidget         *widget,
						  gpointer           data);
static void       color_notebook_cancel_callback (GtkWidget         *widget,
						  gpointer           data);
static void       color_notebook_reset_callback  (GtkWidget         *widget,
						  gpointer           data);
static void       color_notebook_update_callback (gpointer           data,
						  const GimpHSV     *hsv,
						  const GimpRGB     *rgb);
static void       color_notebook_page_switch     (GtkWidget         *widget,
						  GtkNotebookPage   *page,
						  guint              page_num,
						  gpointer           data);
static void       color_notebook_help_func       (const gchar       *help_data);

static void       color_notebook_selector_death  (ColorSelectorInfo *info);

static void       color_notebook_set_white       (ColorNotebook     *cnp);
static void       color_notebook_set_black       (ColorNotebook     *cnp);
static void       color_notebook_color_changed   (GtkWidget         *widget,
						  gpointer           data);
static void       color_notebook_update          (ColorNotebook     *cnp,
						  ColorNotebookUpdateType update);
static void       color_notebook_update_notebook (ColorNotebook     *cnp);
static void       color_notebook_update_channel  (ColorNotebook     *cnp);
static void       color_notebook_update_caller   (ColorNotebook     *cnp);
static void       color_notebook_update_colors   (ColorNotebook     *cnp,
						  ColorNotebookUpdateType which);
static void       color_notebook_update_rgb_values (ColorNotebook   *cnp);
static void       color_notebook_update_hsv_values (ColorNotebook   *cnp);
static void       color_notebook_update_scales     (ColorNotebook   *cnp,
						    gint             skip);

static void       color_notebook_toggle_update    (GtkWidget        *widget,
						   gpointer          data);
static void       color_notebook_scale_update     (GtkAdjustment    *adjustment,
						   gpointer          data);
static gint       color_notebook_hex_entry_events (GtkWidget        *widget,
						   GdkEvent         *event,
						   gpointer          data);

static void       color_history_init              (void);
static void       color_history_color_clicked     (GtkWidget        *widget,
						   gpointer          data);
static void       color_history_color_changed     (GtkWidget        *widget,
						   gpointer          data);
static void       color_history_add_clicked       (GtkWidget        *widget,
						   gpointer          data);


/* master list of all registered colour selectors */
static ColorSelectorInfo *selector_info = NULL;

static GList    *color_notebooks           = NULL;

static GimpRGB   color_history[COLOR_HISTORY_SIZE];
static gboolean  color_history_initialized = FALSE;


ColorNotebook *
color_notebook_new (const gchar           *title,
		    const GimpRGB         *color,
		    ColorNotebookCallback  callback,
		    gpointer               client_data,
		    gboolean               wants_updates,
		    gboolean               show_alpha)
{
  ColorNotebook         *cnp;
  GtkWidget             *main_vbox;
  GtkWidget             *main_hbox;
  GtkWidget             *left_vbox;
  GtkWidget             *right_vbox;
  GtkWidget             *color_frame;
  GtkWidget             *hbox;
  GtkWidget             *table;
  GtkWidget             *label;
  GtkWidget             *button;
  GtkWidget             *image;
  GtkWidget             *arrow;
  GtkWidget             *color_area;
  GimpRGB                bw;
  GSList                *group;
  guchar                 r, g, b;
  gchar                  buffer[8];
  ColorSelectorInfo     *info;
  ColorSelectorInstance *csel;
  gint                   i;

  static gchar *toggle_titles[] = 
  { 
    /* Hue */
    N_("H"),
    /* Saturation */
    N_("S"),
    /* Value */
    N_("V"),
    /* Red */
    N_("R"),
    /* Green */
    N_("G"),
    /* Blue */
    N_("B"),
    /* Alpha */
    N_("A")
  };
  static gchar *slider_tips[7] = 
  {
    N_("Hue"),
    N_("Saturation"),
    N_("Value"),
    N_("Red"),
    N_("Green"),
    N_("Blue"),
    N_("Alpha")
  };
  static gdouble  slider_initial_vals[] = {   0,   0,   0,   0,   0,   0,   0 };
  static gdouble  slider_max_vals[]     = { 360, 100, 100, 255, 255, 255, 100 };
  static gdouble  slider_incs[]         = {  30,  10,  10,  16,  16,  16,  10 };

  g_return_val_if_fail (selector_info != NULL, NULL);
  g_return_val_if_fail (color != NULL, NULL);

  if (! color_history_initialized)
    color_history_init ();

  cnp = g_new0 (ColorNotebook, 1);

  cnp->callback      = callback;
  cnp->client_data   = client_data;
  cnp->wants_updates = wants_updates;
  cnp->show_alpha    = show_alpha;
  cnp->selectors     = NULL;
  cnp->cur_page      = NULL;

  cnp->rgb           = *color;
  cnp->orig_rgb      = *color;

  color_notebook_update_hsv_values (cnp); 

  if (wants_updates)
    {
      cnp->shell =
	gimp_dialog_new (title, "color_selection",
			 color_notebook_help_func, (const gchar *) cnp,
			 GTK_WIN_POS_NONE,
			 FALSE, TRUE, TRUE,

			 "_delete_event_", color_notebook_ok_callback,
			 cnp, NULL, NULL, TRUE, TRUE,

			 NULL);

      gtk_dialog_set_has_separator (GTK_DIALOG (cnp->shell), FALSE);
      gtk_widget_hide (GTK_DIALOG (cnp->shell)->action_area);
    }
  else
    {
      cnp->shell =
	gimp_dialog_new (title, "color_selection",
			 color_notebook_help_func, (const gchar *) cnp,
			 GTK_WIN_POS_NONE,
			 FALSE, TRUE, TRUE,

			 GTK_STOCK_CANCEL, color_notebook_cancel_callback,
			 cnp, NULL, NULL, FALSE, TRUE,

			 GTK_STOCK_OK, color_notebook_ok_callback,
			 cnp, NULL, NULL, TRUE, FALSE,

			 NULL);
    }

  main_vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 2);
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

  /* do we actually need a notebook? */
  if (selector_info->next)
    {
      cnp->notebook = gtk_notebook_new ();
      gtk_box_pack_start (GTK_BOX (left_vbox), cnp->notebook,
			  TRUE, TRUE, 0);
      gtk_widget_show (cnp->notebook);
    }
  else /* only one selector */
    {
      cnp->notebook = NULL;
    }

  /* create each registered color selector */
  info = selector_info;
  while (info)
    {
      if (info->active)
	{
	  csel = g_new (ColorSelectorInstance, 1);
	  csel->color_notebook = cnp;
	  csel->info = info;
	  info->refs++;
	  csel->frame =
	    info->methods.new (&cnp->hsv,
			       &cnp->rgb,
			       show_alpha,
			       color_notebook_update_callback, csel,
			       &csel->selector_data);
	  g_object_set_data (G_OBJECT (csel->frame), "gimp_color_notebook",
			     csel);

	  if (cnp->notebook)
	    {
	      label = gtk_label_new (info->name);
	      gtk_widget_show (label);
	      /* hide the frame, so it doesn't get selected by mistake */
	      gtk_widget_hide (csel->frame);
	      gtk_notebook_append_page (GTK_NOTEBOOK (cnp->notebook),
					csel->frame,
					label);
	    }
	  else
	    {
	      gtk_box_pack_start (GTK_BOX (left_vbox), csel->frame,
				  TRUE, TRUE, 0);
	    }

	  gtk_widget_show (csel->frame);

	  if (!cnp->cur_page)
	    cnp->cur_page = csel;

	  /* link into list of all selectors hanging off the new notebook */
	  csel->next = cnp->selectors;
	  cnp->selectors = csel;
	}

      info = info->next;
    }

  /*  The table for the color_areas  */
  table = gtk_table_new (2, 4, FALSE);
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

  g_signal_connect (G_OBJECT (cnp->new_color), "color_changed",
		    G_CALLBACK (color_notebook_color_changed),
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
  gtk_table_attach (GTK_TABLE (table), button, 2, 4, 1, 2,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (button);

  image = gtk_image_new_from_stock (GTK_STOCK_UNDO, GTK_ICON_SIZE_MENU);
  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_show (image);

  gimp_help_set_help_data (button,
			   _("Revert to old color"),
			   NULL);

  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (color_notebook_reset_callback),
		    cnp);

  /*  The white color button  */
  button = gtk_button_new ();
  gtk_table_attach (GTK_TABLE (table), button, 2, 3, 0, 1,
		    GTK_SHRINK, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (button);

  gimp_rgba_set (&bw, 1.0, 1.0, 1.0, 1.0);
  color_area = gimp_color_area_new (&bw, 
				    GIMP_COLOR_AREA_FLAT,
				    GDK_BUTTON2_MASK);
  gtk_drag_dest_unset (color_area);
  gtk_widget_set_size_request (button, 16, COLOR_AREA_SIZE);
  gtk_container_add (GTK_CONTAINER (button), color_area);
  gtk_widget_show (color_area);

  g_signal_connect_swapped (G_OBJECT (button), "clicked", 
			    G_CALLBACK (color_notebook_set_white),
			    cnp);

  /*  The black color button  */
  button = gtk_button_new ();
  gtk_table_attach (GTK_TABLE (table), button, 3, 4, 0, 1,
		    GTK_SHRINK, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (button);

  gimp_rgba_set (&bw, 0.0, 0.0, 0.0, 1.0);
  color_area = gimp_color_area_new (&bw, 
				    GIMP_COLOR_AREA_FLAT,
				    GDK_BUTTON2_MASK);
  gtk_drag_dest_unset (color_area);
  gtk_widget_set_size_request (button, 16, COLOR_AREA_SIZE);
  gtk_container_add (GTK_CONTAINER (button), color_area);
  gtk_widget_show (color_area);

  g_signal_connect_swapped (G_OBJECT (button), "clicked", 
			    G_CALLBACK (color_notebook_set_black),
			    cnp);

  /*  The color space sliders, toggle buttons and entries  */
  table = gtk_table_new (7, 4, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 1);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 0);
  gtk_box_pack_start (GTK_BOX (right_vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  group = NULL;
  for (i = 0; i < (show_alpha ? 7 : 6); i++)
    {
      if (i == 6)
	{
	  cnp->toggles[i] = NULL;
	}
      else
	{
	  cnp->toggles[i] = gtk_radio_button_new (group);

	  gimp_help_set_help_data (cnp->toggles[i],
				   gettext (slider_tips[i]), NULL);
	  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (cnp->toggles[i]));
	  gtk_table_attach (GTK_TABLE (table), cnp->toggles[i],
			    0, 1, i, i + 1,
			    GTK_SHRINK, GTK_EXPAND, 0, 0);
	  gtk_widget_show (cnp->toggles[i]);

	  g_signal_connect (G_OBJECT (cnp->toggles[i]), "toggled",
			    G_CALLBACK (color_notebook_toggle_update),
			    cnp);
	}

      cnp->slider_data[i] = gimp_scale_entry_new (GTK_TABLE (table), 1, i,
                                                  gettext (toggle_titles[i]), 
                                                  80, 55,
                                                  slider_initial_vals[i],
                                                  0.0, slider_max_vals[i],
                                                  1.0, slider_incs[i],
                                                  0, TRUE, 0.0, 0.0,
                                                  gettext (slider_tips[i]),
                                                  NULL);

      g_signal_connect (G_OBJECT (cnp->slider_data[i]), "value_changed",
			G_CALLBACK (color_notebook_scale_update),
			cnp);
    }

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

  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (color_history_add_clicked),
		    cnp);

  arrow = gtk_arrow_new (GTK_ARROW_RIGHT, GTK_SHADOW_OUT);
  gtk_container_add (GTK_CONTAINER (button), arrow);
  gtk_widget_show (arrow);

  for (i = 0; i < COLOR_HISTORY_SIZE; i++)
    {
      button = gtk_button_new ();
      gtk_widget_set_size_request (button, COLOR_AREA_SIZE, COLOR_AREA_SIZE);
      gtk_table_attach_defaults (GTK_TABLE (table), button,
				 (i > 7 ? i - 8 : i) + 1,
				 (i > 7 ? i - 8 : i) + 2,
				 i > 7 ? 1 : 0,
				 i > 7 ? 2 : 1);

      cnp->history[i] = gimp_color_area_new (&color_history[i], 
					     GIMP_COLOR_AREA_SMALL_CHECKS,
					     GDK_BUTTON2_MASK);
      gtk_container_add (GTK_CONTAINER (button), cnp->history[i]);
      gtk_widget_show (cnp->history[i]);
      gtk_widget_show (button);

      g_signal_connect (G_OBJECT (button), "clicked",
			G_CALLBACK (color_history_color_clicked),
			cnp);

      g_signal_connect (G_OBJECT (cnp->history[i]), "color_changed",
			G_CALLBACK (color_history_color_changed),
			&color_history[i]);
    }

  /* The hex triplet entry */
  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_end (GTK_BOX (right_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  cnp->hex_entry = gtk_entry_new ();
  gtk_entry_set_max_length (GTK_ENTRY (cnp->hex_entry), 7);
  gimp_rgb_get_uchar (&cnp->rgb, &r, &g, &b);
  g_snprintf (buffer, sizeof (buffer), "#%.2x%.2x%.2x", r, g, b);
  gtk_entry_set_text (GTK_ENTRY (cnp->hex_entry), buffer);
  gtk_widget_set_size_request (GTK_WIDGET (cnp->hex_entry), 60, -1);
  gtk_box_pack_end (GTK_BOX (hbox), cnp->hex_entry, TRUE, TRUE, 0);
  gtk_widget_show (cnp->hex_entry);

  g_signal_connect (G_OBJECT (cnp->hex_entry), "focus_out_event",
		    G_CALLBACK (color_notebook_hex_entry_events),
		    cnp);
  g_signal_connect (G_OBJECT (cnp->hex_entry), "key_press_event",
		    G_CALLBACK (color_notebook_hex_entry_events),
		    cnp);

  label = gtk_label_new (_("Hex Triplet:"));
  gtk_box_pack_end (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  color_notebook_update_scales (cnp, -1);

  gtk_widget_show (cnp->shell);

  /* this must come after showing the widget, otherwise we get a
   * switch_page signal for a non-visible color selector, which is bad
   * news.
   */
  if (cnp->notebook)
    {
      g_signal_connect (G_OBJECT (cnp->notebook), "switch_page",
			G_CALLBACK (color_notebook_page_switch),
			cnp);
    }

  color_notebooks = g_list_prepend (color_notebooks, cnp);

  return cnp;
}

void
color_notebook_show (ColorNotebook *cnp)
{
  g_return_if_fail (cnp != NULL);

  if (! GTK_WIDGET_VISIBLE (cnp->shell))
    gtk_widget_show (cnp->shell);
  else
    gdk_window_raise (cnp->shell->window);
}

void
color_notebook_hide (ColorNotebook *cnp)
{
  g_return_if_fail (cnp != NULL);
  gtk_widget_hide (cnp->shell);
}

void
color_notebook_free (ColorNotebook *cnp)
{
  ColorSelectorInstance *csel, *next;

  g_return_if_fail (cnp != NULL);

  color_notebooks = g_list_remove (color_notebooks, cnp);

  gtk_widget_destroy (cnp->shell);

  /* call the free functions for all the color selectors */
  csel = cnp->selectors;
  while (csel)
    {
      next = csel->next;

      csel->info->methods.free (csel->selector_data);

      csel->info->refs--;
      if (csel->info->refs == 0 && !csel->info->active)
	color_notebook_selector_death (csel->info);

      g_free (csel);
      csel = next;
    }

  g_free (cnp);
}

void
color_notebook_set_color (ColorNotebook *cnp,
			  const GimpRGB *color)
{
  g_return_if_fail (cnp != NULL);
  g_return_if_fail (color != NULL);

  cnp->rgb      = *color;
  cnp->orig_rgb = *color;

  color_notebook_update_hsv_values (cnp);
  color_notebook_update_scales (cnp, -1);

  color_notebook_update (cnp,
			 UPDATE_NOTEBOOK   |
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

static void
color_notebook_set_white (ColorNotebook *cnp)
{
  gimp_rgb_set (&cnp->rgb, 1.0, 1.0, 1.0);

  color_notebook_update_hsv_values (cnp);
  color_notebook_update_scales (cnp, -1);

  color_notebook_update (cnp,
			 UPDATE_NOTEBOOK   |
			 UPDATE_NEW_COLOR  |
			 UPDATE_CALLER);
}

static void
color_notebook_set_black (ColorNotebook *cnp)
{
  gimp_rgb_set (&cnp->rgb, 0.0, 0.0, 0.0);

  color_notebook_update_hsv_values (cnp);
  color_notebook_update_scales (cnp, -1);

  color_notebook_update (cnp,
			 UPDATE_NOTEBOOK   |
			 UPDATE_NEW_COLOR  |
			 UPDATE_CALLER);
}

static void
color_notebook_color_changed (GtkWidget *widget,
			      gpointer   data)
{
  ColorNotebook *cnp;

  cnp = (ColorNotebook *) data;

  gimp_color_area_get_color (GIMP_COLOR_AREA (widget), &cnp->rgb);

  color_notebook_update_hsv_values (cnp);
  color_notebook_update_scales (cnp, -1);

  color_notebook_update (cnp,
			 UPDATE_NOTEBOOK   |
			 UPDATE_NEW_COLOR  |
			 UPDATE_CALLER);
}

/*
 * Called by a color selector on user selection of a color
 */
static void
color_notebook_update_callback (gpointer       data,
				const GimpHSV *hsv,
				const GimpRGB *rgb)
{
  ColorSelectorInstance *csel;
  ColorNotebook         *cnp;

  g_return_if_fail (data != NULL);
  g_return_if_fail (hsv != NULL);
  g_return_if_fail (rgb != NULL);

  csel = (ColorSelectorInstance *) data;
  cnp = csel->color_notebook;

  cnp->hsv = *hsv;
  cnp->rgb = *rgb;

  color_notebook_update_scales (cnp, -1);

  color_notebook_update (cnp,
			 UPDATE_NEW_COLOR |
			 UPDATE_CALLER);
}

static void
color_notebook_ok_callback (GtkWidget *widget,
			    gpointer   data)
{
  ColorNotebook *cnp;

  cnp = (ColorNotebook *) data;

  color_history_add_clicked (NULL, cnp);

  if (cnp->callback)
    {
      (* cnp->callback) (cnp,
			 &cnp->rgb,
			 COLOR_NOTEBOOK_OK,
			 cnp->client_data);
    }
}

static void
color_notebook_cancel_callback (GtkWidget *widget,
				gpointer   data)
{
  ColorNotebook *cnp;

  cnp = (ColorNotebook *) data;

  if (cnp->callback)
    {
      (* cnp->callback) (cnp,
			 &cnp->orig_rgb,
			 COLOR_NOTEBOOK_CANCEL,
			 cnp->client_data);
    }
}

static void
color_notebook_reset_callback (GtkWidget *widget,
			       gpointer   data)
{
  ColorNotebook *cnp;

  cnp = (ColorNotebook *) data;

  cnp->rgb = cnp->orig_rgb;

  color_notebook_update_hsv_values (cnp);
  color_notebook_update_scales (cnp, -1);

  color_notebook_update (cnp,
			 UPDATE_NOTEBOOK  |
			 UPDATE_NEW_COLOR |
			 UPDATE_CALLER);
}

static void
color_notebook_page_switch (GtkWidget       *widget,
			    GtkNotebookPage *page,
			    guint            page_num,
			    gpointer         data)
{
  GtkWidget             *page_widget;
  ColorNotebook         *cnp;
  ColorSelectorInstance *csel;
  gint                   i;

  cnp = (ColorNotebook *) data;

  page_widget = gtk_notebook_get_nth_page (GTK_NOTEBOOK (widget), page_num);

  csel = g_object_get_data (G_OBJECT (page_widget), "gimp_color_notebook");

  g_return_if_fail (cnp != NULL && csel != NULL);

  cnp->cur_page = csel;

  for (i = 0; i < 6; i++)
    {
      gtk_widget_set_sensitive (cnp->toggles[i], 
				csel->info->methods.set_channel != NULL);
    }

  color_notebook_update (cnp, UPDATE_CHANNEL | UPDATE_NOTEBOOK);
}

static void
color_notebook_help_func (const gchar *data)
{
  ColorNotebook *cnp;
  gchar         *help_path;

  cnp = (ColorNotebook *) data;

  help_path = g_strconcat ("dialogs/color_selectors/",
			   cnp->cur_page->info->help_page,
			   NULL);
  gimp_standard_help_func (help_path);
  g_free (help_path);
}

/**************************/
/* Registration functions */

G_MODULE_EXPORT
GimpColorSelectorID
gimp_color_selector_register (const gchar              *name,
			      const gchar              *help_page,
			      GimpColorSelectorMethods *methods)
{
  ColorSelectorInfo *info;

  /* check the name is unique */
  info = selector_info;
  while (info)
    {
      if (!strcmp (info->name, name))
	return NULL;
      info = info->next;
    }

  info = g_new (ColorSelectorInfo, 1);

  info->name      = g_strdup (name);
  info->help_page = g_strdup (help_page);
  info->methods   = *methods;
  info->refs      = 0;
  info->active    = TRUE;

  info->next      = selector_info;

  selector_info = info;
  
  return info;
}


G_MODULE_EXPORT
gboolean
gimp_color_selector_unregister (GimpColorSelectorID          id,
				GimpColorSelectorFinishedCB  callback,
				gpointer                     data)
{
  ColorSelectorInfo *info;

  info = selector_info;
  while (info)
    {
      if (info == id)
	{
	  info->active         = FALSE;
	  info->death_callback = callback;
	  info->death_data     = data;
	  if (info->refs == 0)
	    color_notebook_selector_death (info);
	  return TRUE;
	}
      info = info->next;
    }

  g_warning ("%s: unknown color selector ID %p", G_STRLOC, id);
  return FALSE;
}

static void
color_notebook_selector_death (ColorSelectorInfo *info)
{
  ColorSelectorInfo *here, *prev;

  here = selector_info;
  prev = NULL;
  while (here)
    {
      if (here == info)
	{	  
	  if (prev)
	    prev->next = info->next;
	  else
	    selector_info = info->next;

	  if (info->death_callback)
	    (*info->death_callback) (info->death_data);

	  g_free (info->name);
	  g_free (info);

	  return;
	}
      prev = here;
      here = here->next;
    }

  g_warning ("%s: color selector %p not found.", G_STRLOC, info);
}

static void
color_notebook_update (ColorNotebook           *cnp,
		       ColorNotebookUpdateType  update)
{
  if (!cnp)
    return;

  if (update & UPDATE_NOTEBOOK)
    color_notebook_update_notebook (cnp);

  if (update & UPDATE_CHANNEL)
    color_notebook_update_channel (cnp);

  if (update & UPDATE_NEW_COLOR)
    color_notebook_update_colors (cnp, UPDATE_NEW_COLOR);

  if (update & UPDATE_ORIG_COLOR)
    color_notebook_update_colors (cnp, UPDATE_ORIG_COLOR);

  if (update & UPDATE_CALLER && cnp->wants_updates)
    color_notebook_update_caller (cnp);
}

static void
color_notebook_update_notebook (ColorNotebook *cnp)
{
  ColorSelectorInstance *csel;

  g_return_if_fail (cnp != NULL);

  csel = cnp->cur_page;

  if (csel->info->methods.set_color)   
    csel->info->methods.set_color (csel->selector_data,
				   &cnp->hsv,
				   &cnp->rgb);
}

static void
color_notebook_update_channel (ColorNotebook *cnp)
{
  ColorSelectorInstance *csel;

  g_return_if_fail (cnp != NULL);

  csel = cnp->cur_page;

  if (csel->info->methods.set_channel)   
    csel->info->methods.set_channel (csel->selector_data,
				     cnp->active_channel);
}

static void
color_notebook_update_caller (ColorNotebook *cnp)
{
  if (cnp && cnp->callback)
    {
      (* cnp->callback) (cnp,
			 &cnp->rgb,
			 COLOR_NOTEBOOK_UPDATE,
			 cnp->client_data);
    }
}

static void
color_notebook_update_colors (ColorNotebook           *cnp,
			      ColorNotebookUpdateType  which)
{
  if (!cnp)
    return;

  switch (which)
    {
    case UPDATE_ORIG_COLOR:
      gimp_color_area_set_color (GIMP_COLOR_AREA (cnp->orig_color), 
				 &cnp->orig_rgb);
      break;
      
    case UPDATE_NEW_COLOR:
      g_signal_handlers_block_by_func (G_OBJECT (cnp->new_color),
				       color_notebook_color_changed,
				       cnp);

      gimp_color_area_set_color (GIMP_COLOR_AREA (cnp->new_color), 
				 &cnp->rgb);

      g_signal_handlers_unblock_by_func (G_OBJECT (cnp->new_color),
					 color_notebook_color_changed,
					 cnp);
      break;
      
    default:
      return;
    }
}

static void
color_notebook_update_rgb_values (ColorNotebook *cnp)
{
  if (! cnp)
    return;

  gimp_hsv_to_rgb (&cnp->hsv, &cnp->rgb);
}

static void
color_notebook_update_hsv_values (ColorNotebook *cnp)
{
  if (! cnp)
    return;

  gimp_rgb_to_hsv (&cnp->rgb, &cnp->hsv);
}

static void
color_notebook_update_scales (ColorNotebook *cnp,
			      gint           skip)
{
  gint  values[7];
  gchar buffer[8];
  gint  i;

  if (! cnp)
    return;

  values[GIMP_COLOR_SELECTOR_HUE]        = (gint) (cnp->hsv.h * 360.999);
  values[GIMP_COLOR_SELECTOR_SATURATION] = (gint) (cnp->hsv.s * 100.999);
  values[GIMP_COLOR_SELECTOR_VALUE]      = (gint) (cnp->hsv.v * 100.999);
  values[GIMP_COLOR_SELECTOR_RED]        = (gint) (cnp->rgb.r * 255.999);
  values[GIMP_COLOR_SELECTOR_GREEN]      = (gint) (cnp->rgb.g * 255.999);
  values[GIMP_COLOR_SELECTOR_BLUE]       = (gint) (cnp->rgb.b * 255.999);
  values[GIMP_COLOR_SELECTOR_ALPHA]      = (gint) (cnp->rgb.a * 100.999);

  for (i = 0; i < (cnp->show_alpha ? 7 : 6); i++)
    if (i != skip)
      {
	g_signal_handlers_block_by_func (G_OBJECT (cnp->slider_data[i]),
					 color_notebook_scale_update,
					 cnp);

	gtk_adjustment_set_value (GTK_ADJUSTMENT (cnp->slider_data[i]),
				  values[i]);

	g_signal_handlers_unblock_by_func (G_OBJECT (cnp->slider_data[i]),
					   color_notebook_scale_update,
					   cnp);
      }

#if 0
  g_print ("RGB: %d %d %d HSV: %d %d %d ALPHA: %d\n", 
	   values[GIMP_COLOR_SELECTOR_RED],
	   values[GIMP_COLOR_SELECTOR_GREEN],
	   values[GIMP_COLOR_SELECTOR_BLUE],
	   values[GIMP_COLOR_SELECTOR_HUE],
	   values[GIMP_COLOR_SELECTOR_SATURATION],
	   values[GIMP_COLOR_SELECTOR_VALUE],
	   values[GIMP_COLOR_SELECTOR_ALPHA]);
#endif

  g_snprintf (buffer, sizeof (buffer), "#%.2x%.2x%.2x",
              values[GIMP_COLOR_SELECTOR_RED],
              values[GIMP_COLOR_SELECTOR_GREEN],
              values[GIMP_COLOR_SELECTOR_BLUE]);

  gtk_entry_set_text (GTK_ENTRY (cnp->hex_entry), buffer);
}

static void
color_notebook_toggle_update (GtkWidget *widget,
			      gpointer   data)
{
  ColorNotebook *cnp;
  gint           i;

  if (! GTK_TOGGLE_BUTTON (widget)->active)
    return;

  cnp = (ColorNotebook *) data;

  if (!cnp)
    return;

  for (i = 0; i < 6; i++)
    if (widget == cnp->toggles[i])
      cnp->active_channel = (GimpColorSelectorChannelType) i;

  color_notebook_update (cnp, UPDATE_CHANNEL);
}

static void
color_notebook_scale_update (GtkAdjustment *adjustment,
			     gpointer       data)
{
  ColorNotebook *cnp;
  gint           i;

  cnp = (ColorNotebook *) data;

  if (!cnp)
    return;

  for (i = 0; i < (cnp->show_alpha ? 7 : 6); i++)
    if (cnp->slider_data[i] == GTK_OBJECT (adjustment))
      break;

  switch (i)
    {
    case GIMP_COLOR_SELECTOR_HUE:
      cnp->hsv.h = GTK_ADJUSTMENT (adjustment)->value / 360.0;
      break;

    case GIMP_COLOR_SELECTOR_SATURATION:
      cnp->hsv.s = GTK_ADJUSTMENT (adjustment)->value / 100.0;
      break;

    case GIMP_COLOR_SELECTOR_VALUE:
      cnp->hsv.v = GTK_ADJUSTMENT (adjustment)->value / 100.0;
      break;

    case GIMP_COLOR_SELECTOR_RED:
      cnp->rgb.r = GTK_ADJUSTMENT (adjustment)->value / 255.0;
      break;

    case GIMP_COLOR_SELECTOR_GREEN:
      cnp->rgb.g = GTK_ADJUSTMENT (adjustment)->value / 255.0;
      break;

    case GIMP_COLOR_SELECTOR_BLUE:
      cnp->rgb.b = GTK_ADJUSTMENT (adjustment)->value / 255.0;
      break;

    case GIMP_COLOR_SELECTOR_ALPHA:
      cnp->hsv.a = cnp->rgb.a = GTK_ADJUSTMENT (adjustment)->value / 100.0;
      break;
    }

  if ((i >= GIMP_COLOR_SELECTOR_HUE) && (i <= GIMP_COLOR_SELECTOR_VALUE))
    {
      color_notebook_update_rgb_values (cnp);
    }
  else if ((i >= GIMP_COLOR_SELECTOR_RED) && (i <= GIMP_COLOR_SELECTOR_BLUE))
    {
      color_notebook_update_hsv_values (cnp);
    }

  color_notebook_update_scales (cnp, i);

  color_notebook_update (cnp,
			 UPDATE_NOTEBOOK  |
			 UPDATE_NEW_COLOR |
			 UPDATE_CALLER);
}

static gint
color_notebook_hex_entry_events (GtkWidget *widget,
				 GdkEvent  *event,
				 gpointer   data)
{
  ColorNotebook *cnp;
  gchar          buffer[8];
  gchar         *hex_color;
  guint          hex_rgb;
  guchar         r, g, b;

  cnp = (ColorNotebook *) data;

  if (!cnp)
    return FALSE;
  
  switch (event->type)
    {
    case GDK_KEY_PRESS:
      if (((GdkEventKey *) event)->keyval != GDK_Return)
        break;
      /*  else fall through  */

    case GDK_FOCUS_CHANGE:
      hex_color = g_strdup (gtk_entry_get_text (GTK_ENTRY (cnp->hex_entry)));

      gimp_rgb_get_uchar (&cnp->rgb, &r, &g, &b);
      g_snprintf (buffer, sizeof (buffer), "#%.2x%.2x%.2x", r, g, b);

      if ((strlen (hex_color) == 7) &&
          (g_ascii_strcasecmp (buffer, hex_color) != 0))
        {
          if ((sscanf (hex_color, "#%x", &hex_rgb) == 1) &&
              (hex_rgb < (1 << 24)))
	    {
	      gimp_rgb_set_uchar (&cnp->rgb, 
				  (hex_rgb & 0xff0000) >> 16,
				  (hex_rgb & 0x00ff00) >> 8,
				  (hex_rgb & 0x0000ff));

	      color_notebook_update_hsv_values (cnp);
	      color_notebook_update_scales (cnp, -1);

	      color_notebook_update (cnp,
				     UPDATE_NOTEBOOK  |
				     UPDATE_NEW_COLOR |
				     UPDATE_CALLER);
	    }
        }

      g_free (hex_color);

      break;

    default:
      /*  do nothing  */
      break;
    }

  return FALSE;
}

void
color_history_add_color_from_rc (GimpRGB *color)
{
  static gint index = 0;

  if (! color_history_initialized)
    color_history_init ();

  if (color && index < COLOR_HISTORY_SIZE)
    {
      color_history[index++] = *color;
    }
}

void
color_history_write (FILE *fp)
{
  gint i;

  fprintf (fp, "(color-history");

  for (i = 0; i < COLOR_HISTORY_SIZE; i++)
    {
      gchar buf[4][G_ASCII_DTOSTR_BUF_SIZE];

      g_ascii_formatd (buf[0],  
                       G_ASCII_DTOSTR_BUF_SIZE, "%f", color_history[i].r);
      g_ascii_formatd (buf[1],  
                       G_ASCII_DTOSTR_BUF_SIZE, "%f", color_history[i].g);
      g_ascii_formatd (buf[2],  
                       G_ASCII_DTOSTR_BUF_SIZE, "%f", color_history[i].b);
      g_ascii_formatd (buf[3],  
                       G_ASCII_DTOSTR_BUF_SIZE, "%f", color_history[i].a);

      fprintf (fp, "\n    (color-rgba %s %s %s %s)", 
               buf[0], buf[1], buf[2], buf[3]);
    }

  fprintf (fp, ")\n\n");
}

static void
color_history_init (void)
{
  gint i;

  for (i = 0; i < COLOR_HISTORY_SIZE; i++)
    gimp_rgba_set (&color_history[i], 1.0, 1.0, 1.0, 1.0);

  color_history_initialized = TRUE;
}

static void
color_history_color_clicked (GtkWidget *widget,
			     gpointer   data)
{
  ColorNotebook *cnp;
  GimpColorArea *color_area;

  cnp = (ColorNotebook *) data;

  color_area = GIMP_COLOR_AREA (GTK_BIN (widget)->child);

  gimp_color_area_get_color (color_area, &cnp->rgb);

  color_notebook_update_hsv_values (cnp);
  color_notebook_update_scales (cnp, -1);

  color_notebook_update (cnp,
			 UPDATE_NOTEBOOK   |
			 UPDATE_ORIG_COLOR |
			 UPDATE_NEW_COLOR  |
			 UPDATE_CALLER);
}

static void
color_history_color_changed (GtkWidget *widget,
			     gpointer   data)
{
  GimpRGB *color;
  GimpRGB  changed_color;
  gint     i;

  color = (GimpRGB *) data;

  gimp_color_area_get_color (GIMP_COLOR_AREA (widget), &changed_color);

  for (i = 0; i < COLOR_HISTORY_SIZE; i++)
    {
      if (color == &color_history[i])
	{
	  GList         *list;
	  ColorNotebook *notebook;

	  color_history[i] = changed_color;

	  for (list = color_notebooks; list; list = g_list_next (list))
	    {
	      notebook = (ColorNotebook *) list->data;

	      if (notebook->history[i] == widget)
		continue;

	      g_signal_handlers_block_by_func (G_OBJECT (notebook->history[i]),
					       color_history_color_changed,
					       data);

	      gimp_color_area_set_color
		(GIMP_COLOR_AREA (notebook->history[i]), &changed_color);

	      g_signal_handlers_unblock_by_func (G_OBJECT (notebook->history[i]),
						 color_history_color_changed,
						 data);
	    }
	}
    }
}

static void
color_history_add_clicked (GtkWidget *widget,
			   gpointer   data)
{
  ColorNotebook *cnp;
  gint           shift_begin = -1;
  gint           i, j;

  cnp = (ColorNotebook *) data;

  /*  is the added color already there?  */
  for (i = 0; i < COLOR_HISTORY_SIZE; i++)
    {
      if (gimp_rgba_distance (&color_history[i], &cnp->rgb) < 0.0001)
	{
	  shift_begin = i;

	  goto doit;
	}
    }

  /*  if not, are there two equal colors?  */
  if (shift_begin == -1)
    {
      for (i = 0; i < COLOR_HISTORY_SIZE; i++)
	{
	  for (j = i + 1; j < COLOR_HISTORY_SIZE; j++)
	    {
	      if (gimp_rgba_distance (&color_history[i],
				      &color_history[j]) < 0.0001)
		{
		  shift_begin = i;

		  goto doit;
		}
	      
	    }
	}
    }

  /*  if not, shift them all  */
  if (shift_begin == -1)
    {
      shift_begin = COLOR_HISTORY_SIZE - 1;
    }

 doit:

  for (i = shift_begin; i > 0; i--)
    {
      gimp_color_area_set_color (GIMP_COLOR_AREA (cnp->history[i]),
				 &color_history[i - 1]);
    }

  gimp_color_area_set_color (GIMP_COLOR_AREA (cnp->history[0]),
			     &cnp->rgb);
}
