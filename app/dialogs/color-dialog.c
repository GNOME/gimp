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

#define __COLOR_NOTEBOOK_C__ 1

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gmodule.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "apptypes.h"

#include "color_area.h"
#include "color_notebook.h"
#include "colormaps.h"
#include "gimpdnd.h"
#include "gimpui.h"

#include "libgimp/gimphelpui.h"
#include "libgimp/gimpcolorselector.h"
#include "libgimp/gimpcolorspace.h"

#include "libgimp/gimpintl.h"


#define COLOR_AREA_WIDTH   74
#define COLOR_AREA_HEIGHT  20


typedef enum
{
  UPDATE_NOTEBOOK   = 1 << 0,
  UPDATE_CHANNEL    = 1 << 1,
  UPDATE_NEW_COLOR  = 1 << 2,
  UPDATE_ORIG_COLOR = 1 << 3,
  UPDATE_CALLER     = 1 << 4
} ColorNotebookUpdateType;


struct _ColorNotebook
{
  GtkWidget                *shell;
  GtkWidget                *notebook;

  GtkWidget                *new_color;
  GtkWidget                *orig_color;
  GtkWidget                *toggles[7];
  GtkObject                *slider_data[7];
  GtkWidget                *hex_entry;

  GdkGC                    *gc;

  gint                      values[7];
  gint                      orig_values[4];

  GimpColorSelectorChannelType  active_channel;

  ColorNotebookCallback     callback;
  gpointer                  client_data;

  gboolean                  wants_updates;
  gboolean                  show_alpha;

  ColorSelectorInstance    *selectors;
  ColorSelectorInstance    *cur_page;
};


/* information we keep on each registered colour selector */
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

struct _ColorSelectorInstance
{
  ColorNotebook         *color_notebook;
  ColorSelectorInfo     *info;
  GtkWidget             *frame;   /* main widget */
  gpointer               selector_data;

  ColorSelectorInstance *next;
};


static void       color_notebook_ok_callback     (GtkWidget         *widget,
						  gpointer           data);
static void       color_notebook_cancel_callback (GtkWidget         *widget,
						  gpointer           data);
static void       color_notebook_update_callback (gpointer           data,
						  gint               hue,
						  gint               saturation,
						  gint               value,
						  gint               red,
						  gint               green,
						  gint               blue,
						  gint               alpha);
static void       color_notebook_page_switch     (GtkWidget         *widget,
						  GtkNotebookPage   *page,
						  guint              page_num,
						  gpointer           data);
static void       color_notebook_help_func       (const gchar       *help_data);

static void       color_notebook_selector_death  (ColorSelectorInfo *info);

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

static gboolean   color_notebook_color_events    (GtkWidget         *widget,
						  GdkEvent          *event,
						  gpointer           data);

static void       color_notebook_toggle_update   (GtkWidget         *widget,
						  gpointer           data);
static void       color_notebook_scale_update    (GtkAdjustment     *adjustment,
						  gpointer           data);
static gint       color_notebook_hex_entry_events (GtkWidget      *widget,
						   GdkEvent       *event,
						   gpointer        data);

static void       color_notebook_drag_new_color  (GtkWidget         *widget,
						  guchar            *r,
						  guchar            *g,
						  guchar            *b,
						  guchar            *a,
						  gpointer           data);
static void       color_notebook_drop_new_color  (GtkWidget         *widget,
						  guchar             r,
						  guchar             g,
						  guchar             b,
						  guchar             a,
						  gpointer           data);
static void       color_notebook_drag_old_color  (GtkWidget         *widget,
						  guchar            *r,
						  guchar            *g,
						  guchar            *b,
						  guchar            *a,
						  gpointer           data);


/* master list of all registered colour selectors */
static ColorSelectorInfo *selector_info = NULL;

/*  dnd stuff  */
static GtkTargetEntry color_notebook_target_table[] =
{
  GIMP_TARGET_COLOR
};
static guint n_color_notebook_targets = (sizeof (color_notebook_target_table) /
					 sizeof (color_notebook_target_table[0]));


ColorNotebook *
color_notebook_new (gint                   red,
		    gint                   green,
		    gint                   blue,
		    gint                   alpha,
		    ColorNotebookCallback  callback,
		    gpointer               client_data,
		    gboolean               wants_updates,
		    gboolean               show_alpha)
{
  ColorNotebook         *cnp;
  GtkWidget             *main_hbox;
  GtkWidget             *right_vbox;
  GtkWidget             *colors_frame;
  GtkWidget             *hbox;
  GtkWidget             *table;
  GtkWidget             *label;
  GSList                *group;
  gchar                  buffer[16];
  ColorSelectorInfo     *info;
  ColorSelectorInstance *csel;
  gint                   i;

  static gchar *toggle_titles[] = 
  { 
    N_("H"),
    N_("S"),
    N_("V"),
    N_("R"),
    N_("G"),
    N_("B"),
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
  static gdouble  slider_max_vals[] = { 360, 100, 100, 255, 255, 255, 255 };
  static gdouble  slider_incs[]     = {  30,  10,  10,  16,  16,  16,  16 };

  g_return_val_if_fail (selector_info != NULL, NULL);

  cnp = g_new0 (ColorNotebook, 1);

  cnp->gc            = NULL;

  cnp->callback      = callback;
  cnp->client_data   = client_data;
  cnp->wants_updates = wants_updates;
  cnp->show_alpha    = show_alpha;
  cnp->selectors     = NULL;
  cnp->cur_page      = NULL;

  cnp->values[GIMP_COLOR_SELECTOR_RED]   = cnp->orig_values[0] = red   & 0xff;
  cnp->values[GIMP_COLOR_SELECTOR_GREEN] = cnp->orig_values[1] = green & 0xff;
  cnp->values[GIMP_COLOR_SELECTOR_BLUE]  = cnp->orig_values[2] = blue  & 0xff;
  cnp->values[GIMP_COLOR_SELECTOR_ALPHA] = cnp->orig_values[3] = alpha & 0xff;

  color_notebook_update_hsv_values (cnp); 

  cnp->shell =
    gimp_dialog_new (_("Color Selection"), "color_selection",
		     color_notebook_help_func, (const gchar *) cnp,
		     GTK_WIN_POS_NONE,
		     FALSE, FALSE, FALSE,

		     wants_updates ? _("Close") : _("OK"),
		     color_notebook_ok_callback,
		     cnp, NULL, NULL, TRUE, wants_updates,
		     wants_updates ? _("Revert to Old Color") : _("Cancel"),
		     color_notebook_cancel_callback,
		     cnp, NULL, NULL, FALSE, !wants_updates,

		     NULL);

  main_hbox = gtk_hbox_new (FALSE, 6);
  gtk_container_set_border_width (GTK_CONTAINER (main_hbox), 2);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (cnp->shell)->vbox), main_hbox);
  gtk_widget_show (main_hbox);

  /* do we actually need a notebook? */
  if (selector_info->next)
    {
      cnp->notebook = gtk_notebook_new ();
      gtk_box_pack_start (GTK_BOX (main_hbox), cnp->notebook,
			  FALSE, FALSE, 0);
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
	    info->methods.new (cnp->values[GIMP_COLOR_SELECTOR_HUE],
			       cnp->values[GIMP_COLOR_SELECTOR_SATURATION],
			       cnp->values[GIMP_COLOR_SELECTOR_VALUE],
			       cnp->values[GIMP_COLOR_SELECTOR_RED],
			       cnp->values[GIMP_COLOR_SELECTOR_GREEN],
			       cnp->values[GIMP_COLOR_SELECTOR_BLUE],
			       cnp->values[GIMP_COLOR_SELECTOR_ALPHA],
			       show_alpha,
			       color_notebook_update_callback, csel,
			       &csel->selector_data);
	  gtk_object_set_data (GTK_OBJECT (csel->frame), "gimp_color_notebook",
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
	      gtk_box_pack_start (GTK_BOX (main_hbox), csel->frame,
				  FALSE, FALSE, 0);
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

  /*  The right vertical box with old/new color area and color space sliders  */
  right_vbox = gtk_vbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (main_hbox), right_vbox, TRUE, TRUE, 0);
  gtk_widget_show (right_vbox);

  /*  The old/new color area frame and hbox  */
  colors_frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (colors_frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (right_vbox), colors_frame, FALSE, FALSE, 0);
  gtk_widget_show (colors_frame);

  hbox = gtk_hbox_new (TRUE, 2);
  gtk_container_add (GTK_CONTAINER (colors_frame), hbox);
  gtk_widget_show (hbox);

  /*  The new color area  */
  cnp->new_color = gtk_drawing_area_new ();
  gtk_drawing_area_size (GTK_DRAWING_AREA (cnp->new_color),
                         COLOR_AREA_WIDTH, COLOR_AREA_HEIGHT);
  gtk_widget_set_events (cnp->new_color, GDK_EXPOSURE_MASK);
  gtk_signal_connect (GTK_OBJECT (cnp->new_color), "event",
                      GTK_SIGNAL_FUNC (color_notebook_color_events),
                      cnp);

  gtk_object_set_user_data (GTK_OBJECT (cnp->new_color), cnp);
  gtk_box_pack_start (GTK_BOX (hbox), cnp->new_color, TRUE, TRUE, 0);
  gtk_widget_show (cnp->new_color);

  /*  dnd stuff  */
  gtk_drag_source_set (cnp->new_color,
                       GDK_BUTTON1_MASK | GDK_BUTTON2_MASK,
                       color_notebook_target_table, n_color_notebook_targets,
                       GDK_ACTION_COPY | GDK_ACTION_MOVE);
  gimp_dnd_color_source_set (cnp->new_color, color_notebook_drag_new_color, cnp);

  gtk_drag_dest_set (cnp->new_color,
                     GTK_DEST_DEFAULT_HIGHLIGHT |
                     GTK_DEST_DEFAULT_MOTION |
                     GTK_DEST_DEFAULT_DROP,
                     color_notebook_target_table, n_color_notebook_targets,
                     GDK_ACTION_COPY);
  gimp_dnd_color_dest_set (cnp->new_color, color_notebook_drop_new_color, cnp);

  /*  The old color area  */
  cnp->orig_color = gtk_drawing_area_new ();
  gtk_drawing_area_size (GTK_DRAWING_AREA (cnp->orig_color),
                         COLOR_AREA_WIDTH, COLOR_AREA_HEIGHT);
  gtk_widget_set_events (cnp->orig_color, GDK_EXPOSURE_MASK);
  gtk_signal_connect (GTK_OBJECT (cnp->orig_color), "event",
                      GTK_SIGNAL_FUNC (color_notebook_color_events),
                      cnp);
  gtk_object_set_user_data (GTK_OBJECT (cnp->orig_color), cnp);
  gtk_box_pack_start (GTK_BOX (hbox), cnp->orig_color, TRUE, TRUE, 0);
  gtk_widget_show (cnp->orig_color);

  /*  dnd stuff  */
  gtk_drag_source_set (cnp->orig_color,
                       GDK_BUTTON1_MASK | GDK_BUTTON2_MASK,
                       color_notebook_target_table, n_color_notebook_targets,
                       GDK_ACTION_COPY | GDK_ACTION_MOVE);
  gimp_dnd_color_source_set (cnp->orig_color, color_notebook_drag_old_color,
			     cnp);

  /*  The color space sliders, toggle buttons and entries  */
  table = gtk_table_new (show_alpha ? 7 : 6, 3, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (right_vbox), table, TRUE, TRUE, 0);
  gtk_widget_show (table);

  group = NULL;
  for (i = 0; i < (show_alpha ? 7 : 6); i++)
    {
      if (i == 6)
	{
	  cnp->toggles[i] = NULL;

	  label = gtk_label_new (toggle_titles[i]);
	  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
	  gtk_table_attach (GTK_TABLE (table), label,
			    0, 1, i, i + 1, GTK_FILL, GTK_EXPAND, 0, 0);
	}
      else
	{
	  cnp->toggles[i] =
	    gtk_radio_button_new_with_label (group, gettext (toggle_titles[i]));

	  gimp_help_set_help_data (cnp->toggles[i],
				   gettext (slider_tips[i]), NULL);
	  group = gtk_radio_button_group (GTK_RADIO_BUTTON (cnp->toggles[i]));
	  gtk_table_attach (GTK_TABLE (table), cnp->toggles[i],
			    0, 1, i, i + 1, GTK_FILL, GTK_EXPAND, 0, 0);
	  gtk_signal_connect (GTK_OBJECT (cnp->toggles[i]), "toggled",
			      GTK_SIGNAL_FUNC (color_notebook_toggle_update),
			      cnp);
	  gtk_widget_show (cnp->toggles[i]);
	}

      cnp->slider_data[i] = gimp_scale_entry_new (GTK_TABLE (table), 0, i,
                                                  NULL, 
                                                  80, 55,
                                                  cnp->values[i], 
                                                  0.0, slider_max_vals[i],
                                                  1.0, slider_incs[i],
                                                  0, TRUE, 0.0, 0.0,
                                                  gettext (slider_tips[i]),
                                                  NULL);
      gtk_signal_connect (GTK_OBJECT (cnp->slider_data[i]), "value_changed",
                          GTK_SIGNAL_FUNC (color_notebook_scale_update),
                          cnp);
    }

  /* The hex triplet entry */
  hbox = gtk_hbox_new (FALSE, 3);
  gtk_box_pack_start (GTK_BOX (right_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  cnp->hex_entry = gtk_entry_new ();
  g_snprintf (buffer, sizeof (buffer), "#%.2x%.2x%.2x", red, green, blue);
  gtk_entry_set_text (GTK_ENTRY (cnp->hex_entry), buffer);
  gtk_widget_set_usize (GTK_WIDGET (cnp->hex_entry), 75, 0);
  gtk_box_pack_end (GTK_BOX (hbox), cnp->hex_entry, FALSE, FALSE, 2);
  gtk_signal_connect (GTK_OBJECT (cnp->hex_entry), "focus_out_event",
                      GTK_SIGNAL_FUNC (color_notebook_hex_entry_events),
                      cnp);
  gtk_signal_connect (GTK_OBJECT (cnp->hex_entry), "key_press_event",
                      GTK_SIGNAL_FUNC (color_notebook_hex_entry_events),
                      cnp);
  gtk_widget_show (cnp->hex_entry);

  label = gtk_label_new (_("Hex Triplet:"));
  gtk_box_pack_end (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  gtk_widget_show (cnp->shell);

  /* this must come after showing the widget, otherwise we get a
   * switch_page signal for a non-visible color selector, which is bad
   * news.
   */
  if (cnp->notebook)
    {
      gtk_signal_connect (GTK_OBJECT (cnp->notebook), "switch_page",
			  GTK_SIGNAL_FUNC (color_notebook_page_switch),
			  cnp);
    }

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
			  gint           red,
			  gint           green,
			  gint           blue,
			  gint           alpha)
{
  g_return_if_fail (cnp != NULL);

  cnp->values[GIMP_COLOR_SELECTOR_RED]   = cnp->orig_values[0] = red;
  cnp->values[GIMP_COLOR_SELECTOR_GREEN] = cnp->orig_values[1] = green;
  cnp->values[GIMP_COLOR_SELECTOR_BLUE]  = cnp->orig_values[2] = blue;
  cnp->values[GIMP_COLOR_SELECTOR_ALPHA] = cnp->orig_values[3] = alpha;

  color_notebook_update (cnp,
			 UPDATE_NOTEBOOK   |
			 UPDATE_ORIG_COLOR |
			 UPDATE_NEW_COLOR);
}

/*
 * Called by a color selector on user selection of a color
 */
static void
color_notebook_update_callback (gpointer data,
				gint     hue,
				gint     saturation,
				gint     value,
				gint     red,
				gint     green,
				gint     blue,
				gint     alpha)
{
  ColorSelectorInstance *csel;
  ColorNotebook         *cnp;

  g_return_if_fail (data != NULL);

  csel = (ColorSelectorInstance *) data;
  cnp = csel->color_notebook;

  cnp->values[GIMP_COLOR_SELECTOR_HUE]        = hue;
  cnp->values[GIMP_COLOR_SELECTOR_SATURATION] = saturation;
  cnp->values[GIMP_COLOR_SELECTOR_VALUE]      = value;
  cnp->values[GIMP_COLOR_SELECTOR_RED]        = red;
  cnp->values[GIMP_COLOR_SELECTOR_GREEN]      = green;
  cnp->values[GIMP_COLOR_SELECTOR_BLUE]       = blue;
  cnp->values[GIMP_COLOR_SELECTOR_ALPHA]      = alpha;

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

  if (cnp->callback)
    {
      (* cnp->callback) (cnp->values[GIMP_COLOR_SELECTOR_RED],
			 cnp->values[GIMP_COLOR_SELECTOR_GREEN],
			 cnp->values[GIMP_COLOR_SELECTOR_BLUE],
			 cnp->values[GIMP_COLOR_SELECTOR_ALPHA],
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
      (* cnp->callback) (cnp->orig_values[0],
			 cnp->orig_values[1],
			 cnp->orig_values[2],
			 cnp->orig_values[3],
			 COLOR_NOTEBOOK_CANCEL,
			 cnp->client_data);
    }
}

static void
color_notebook_page_switch (GtkWidget       *widget,
			    GtkNotebookPage *page,
			    guint            page_num,
			    gpointer         data)
{
  ColorNotebook         *cnp;
  ColorSelectorInstance *csel;

  cnp = (ColorNotebook *) data;

  csel = gtk_object_get_data (GTK_OBJECT (page->child), "gimp_color_notebook");

  g_return_if_fail (cnp != NULL && csel != NULL);

  cnp->cur_page = csel;

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

  g_warning ("unknown color selector id %p", id);
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

  g_warning ("color selector %p not found, can't happen!", info);
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
  csel->info->methods.set_color (csel->selector_data,
				 cnp->values[GIMP_COLOR_SELECTOR_HUE],
				 cnp->values[GIMP_COLOR_SELECTOR_SATURATION],
				 cnp->values[GIMP_COLOR_SELECTOR_VALUE],
				 cnp->values[GIMP_COLOR_SELECTOR_RED],
				 cnp->values[GIMP_COLOR_SELECTOR_GREEN],
				 cnp->values[GIMP_COLOR_SELECTOR_BLUE],
				 cnp->values[GIMP_COLOR_SELECTOR_ALPHA]);
}

static void
color_notebook_update_channel (ColorNotebook *cnp)
{
  ColorSelectorInstance *csel;

  g_return_if_fail (cnp != NULL);

  csel = cnp->cur_page;
  csel->info->methods.set_channel (csel->selector_data,
				   cnp->active_channel);
}

static void
color_notebook_update_caller (ColorNotebook *cnp)
{
  if (cnp && cnp->callback)
    {
      (* cnp->callback) (cnp->values[GIMP_COLOR_SELECTOR_RED],
			 cnp->values[GIMP_COLOR_SELECTOR_GREEN],
			 cnp->values[GIMP_COLOR_SELECTOR_BLUE],
			 cnp->values[GIMP_COLOR_SELECTOR_ALPHA],
			 COLOR_NOTEBOOK_UPDATE,
			 cnp->client_data);
    }
}

static void
color_notebook_update_colors (ColorNotebook           *cnp,
			      ColorNotebookUpdateType  which)
{
  GdkWindow *window;
  GdkColor   color;
  gint       red, green, blue;
  gint       width, height;

  if (!cnp)
    return;

  if (which == UPDATE_ORIG_COLOR)
    {
      window = cnp->orig_color->window;
      red    = cnp->orig_values[0];
      green  = cnp->orig_values[1];
      blue   = cnp->orig_values[2];
    }
  else if (which == UPDATE_NEW_COLOR)
    {
      window = cnp->new_color->window;
      red    = cnp->values[GIMP_COLOR_SELECTOR_RED];
      green  = cnp->values[GIMP_COLOR_SELECTOR_GREEN];
      blue   = cnp->values[GIMP_COLOR_SELECTOR_BLUE];
    }
  else
    {
      return;
    }

  /* if we haven't yet been realized, there's no need to redraw
   * anything.
   */
  if (!window)
    return;

  color.pixel = get_color (red, green, blue);

  gdk_window_get_size (window, &width, &height);

  if (cnp->gc)
    {
      color_area_draw_rect (window, cnp->gc,
                            0, 0, width, height,
                            red, green, blue);
    }
}

static void
color_notebook_update_rgb_values (ColorNotebook *cnp)
{
  gdouble h, s, v;

  if (!cnp)
    return;

  h = cnp->values[GIMP_COLOR_SELECTOR_HUE]        / 360.0;
  s = cnp->values[GIMP_COLOR_SELECTOR_SATURATION] / 100.0;
  v = cnp->values[GIMP_COLOR_SELECTOR_VALUE]      / 100.0;

  gimp_hsv_to_rgb_double (&h, &s, &v);

  cnp->values[GIMP_COLOR_SELECTOR_RED]   = h * 255;
  cnp->values[GIMP_COLOR_SELECTOR_GREEN] = s * 255;
  cnp->values[GIMP_COLOR_SELECTOR_BLUE]  = v * 255;
}

static void
color_notebook_update_hsv_values (ColorNotebook *cnp)
{
  gdouble r, g, b;

  if (!cnp)
    return;

  r = cnp->values[GIMP_COLOR_SELECTOR_RED]   / 255.0;
  g = cnp->values[GIMP_COLOR_SELECTOR_GREEN] / 255.0;
  b = cnp->values[GIMP_COLOR_SELECTOR_BLUE]  / 255.0;

  gimp_rgb_to_hsv_double (&r, &g, &b);

  cnp->values[GIMP_COLOR_SELECTOR_HUE]        = r * 360.0;
  cnp->values[GIMP_COLOR_SELECTOR_SATURATION] = g * 100.0;
  cnp->values[GIMP_COLOR_SELECTOR_VALUE]      = b * 100.0;
}

static void
color_notebook_update_scales (ColorNotebook *cnp,
			      gint           skip)
{
  gchar buffer[16];
  gint  i;

  if (!cnp)
    return;

  for (i = 0; i < (cnp->show_alpha ? 7 : 6); i++)
    if (i != skip)
      {
	gtk_signal_handler_block_by_data (GTK_OBJECT (cnp->slider_data[i]), cnp);
	gtk_adjustment_set_value (GTK_ADJUSTMENT (cnp->slider_data[i]),
				  cnp->values[i]);
	gtk_signal_handler_unblock_by_data (GTK_OBJECT (cnp->slider_data[i]), cnp);
      }

  g_snprintf (buffer, sizeof (buffer), "#%.2x%.2x%.2x",
              cnp->values[GIMP_COLOR_SELECTOR_RED],
              cnp->values[GIMP_COLOR_SELECTOR_GREEN],
              cnp->values[GIMP_COLOR_SELECTOR_BLUE]);

  gtk_entry_set_text (GTK_ENTRY (cnp->hex_entry), buffer);
}


static gboolean
color_notebook_color_events (GtkWidget *widget,
			     GdkEvent  *event,
			     gpointer   data)
{
  ColorNotebook *cnp;

  cnp = (ColorNotebook *) data;

  if (!cnp)
    return FALSE;

  switch (event->type)
    {
    case GDK_EXPOSE:
      if (!cnp->gc)
        cnp->gc = gdk_gc_new (widget->window);

      if (widget == cnp->new_color)
        color_notebook_update (cnp, UPDATE_NEW_COLOR);
      else if (widget == cnp->orig_color)
        color_notebook_update (cnp, UPDATE_ORIG_COLOR);
      break;

    default:
      break;
    }

  return FALSE;
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

  cnp->values[i] = (gint) (GTK_ADJUSTMENT (adjustment)->value);

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

      g_snprintf (buffer, sizeof (buffer), "#%.2x%.2x%.2x",
                  cnp->values[GIMP_COLOR_SELECTOR_RED],
                  cnp->values[GIMP_COLOR_SELECTOR_GREEN],
                  cnp->values[GIMP_COLOR_SELECTOR_BLUE]);

      if ((strlen (hex_color) == 7) &&
          (g_strcasecmp (buffer, hex_color) != 0))
        {
          if ((sscanf (hex_color, "#%x", &hex_rgb) == 1) &&
              (hex_rgb < (1 << 24)))
	    {
	      cnp->values[GIMP_COLOR_SELECTOR_RED] = (hex_rgb & 0xff0000) >> 16;
	      cnp->values[GIMP_COLOR_SELECTOR_GREEN] = (hex_rgb & 0x00ff00) >> 8;
	      cnp->values[GIMP_COLOR_SELECTOR_BLUE] = (hex_rgb & 0x0000ff);

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

static void
color_notebook_drag_new_color (GtkWidget *widget,
			       guchar    *r,
			       guchar    *g,
			       guchar    *b,
			       guchar    *a,
			       gpointer   data)
{
  ColorNotebook *cnp;

  cnp = (ColorNotebook *) data;

  *r = (guchar) cnp->values[GIMP_COLOR_SELECTOR_RED];
  *g = (guchar) cnp->values[GIMP_COLOR_SELECTOR_GREEN];
  *b = (guchar) cnp->values[GIMP_COLOR_SELECTOR_BLUE];
  *a = (guchar) cnp->values[GIMP_COLOR_SELECTOR_ALPHA];
}

static void
color_notebook_drop_new_color (GtkWidget *widget,
			       guchar     r,
			       guchar     g,
			       guchar     b,
			       guchar     a,
			       gpointer   data)
{
  ColorNotebook *cnp;

  cnp = (ColorNotebook *) data;

  cnp->values[GIMP_COLOR_SELECTOR_RED]   = (gint) r;
  cnp->values[GIMP_COLOR_SELECTOR_GREEN] = (gint) g;
  cnp->values[GIMP_COLOR_SELECTOR_BLUE]  = (gint) b;
  cnp->values[GIMP_COLOR_SELECTOR_ALPHA] = (gint) a;

  color_notebook_update_hsv_values (cnp);
  color_notebook_update_scales (cnp, -1);

  color_notebook_update (cnp,
			 UPDATE_NOTEBOOK  |
			 UPDATE_NEW_COLOR |
			 UPDATE_CALLER);
}

static void
color_notebook_drag_old_color (GtkWidget *widget,
			       guchar    *r,
			       guchar    *g,
			       guchar    *b,
			       guchar    *a,
			       gpointer   data)
{
  ColorNotebook *cnp;

  cnp = (ColorNotebook *) data;

  *r = (guchar) cnp->orig_values[0];
  *g = (guchar) cnp->orig_values[1];
  *b = (guchar) cnp->orig_values[2];
  *a = (guchar) cnp->orig_values[3];
}
