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

#include "apptypes.h"

#include "color_area.h"
#include "color_notebook.h"
#include "colormaps.h"
#include "gimpdnd.h"
#include "gimpui.h"

#include "libgimp/gimphelpui.h"
#include "libgimp/gimpcolorselector.h"

#include "libgimp/gimpintl.h"


#define COLOR_AREA_WIDTH   74
#define COLOR_AREA_HEIGHT  20


enum
{
  RED,
  GREEN,
  BLUE,
  ALPHA
};

typedef enum
{
  UPDATE_NOTEBOOK   = 1 << 0,
  UPDATE_NEW_COLOR  = 1 << 1,
  UPDATE_ORIG_COLOR = 1 << 2,
  UPDATE_CALLER     = 1 << 3
} ColorNotebookUpdateType;


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
static void       color_notebook_update_caller   (ColorNotebook     *cnp);
static void       color_notebook_update_colors   (ColorNotebook     *cnp,
						  ColorNotebookUpdateType which);

static gboolean   color_notebook_color_events    (GtkWidget         *widget,
						  GdkEvent          *event,
						  gpointer           data);

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
  /*GtkWidget             *right_vbox;*/
  GtkWidget             *colors_frame;
  GtkWidget             *colors_hbox;
  GtkWidget             *label;
  ColorSelectorInfo     *info;
  ColorSelectorInstance *csel;

  g_return_val_if_fail (selector_info != NULL, NULL);

  cnp = g_new0 (ColorNotebook, 1);

  cnp->gc            = NULL;

  cnp->callback      = callback;
  cnp->client_data   = client_data;
  cnp->wants_updates = wants_updates;
  cnp->selectors     = NULL;
  cnp->cur_page      = NULL;

  cnp->values[RED]   = cnp->orig_values[RED]   = red   & 0xff;
  cnp->values[GREEN] = cnp->orig_values[GREEN] = green & 0xff;
  cnp->values[BLUE]  = cnp->orig_values[BLUE]  = blue  & 0xff;
  cnp->values[ALPHA] = cnp->orig_values[ALPHA] = alpha & 0xff;

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
  gtk_container_set_border_width (GTK_CONTAINER (main_hbox), 1);
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
	  csel->frame = info->methods.new (red, green, blue, alpha,
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
  /*
  right_vbox = gtk_vbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (main_hbox), right_vbox, TRUE, TRUE, 0);
  gtk_widget_show (right_vbox);
  */

  /*  The old/new color area frame and hbox  */
  colors_frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (colors_frame), GTK_SHADOW_IN);

  /* gtk_box_pack_start (GTK_BOX (right_vbox), colors_frame, FALSE, FALSE, 0); */

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (cnp->shell)->action_area),
		      colors_frame,
		      FALSE, FALSE, 0);

  gtk_widget_show (colors_frame);

  colors_hbox = gtk_hbox_new (TRUE, 2);
  gtk_container_add (GTK_CONTAINER (colors_frame), colors_hbox);
  gtk_widget_show (colors_hbox);

  /*  The new color area  */
  cnp->new_color = gtk_drawing_area_new ();
  gtk_drawing_area_size (GTK_DRAWING_AREA (cnp->new_color),
                         COLOR_AREA_WIDTH, COLOR_AREA_HEIGHT);
  gtk_widget_set_events (cnp->new_color, GDK_EXPOSURE_MASK);
  gtk_signal_connect (GTK_OBJECT (cnp->new_color), "event",
                      GTK_SIGNAL_FUNC (color_notebook_color_events),
                      cnp);

  gtk_object_set_user_data (GTK_OBJECT (cnp->new_color), cnp);
  gtk_box_pack_start (GTK_BOX (colors_hbox), cnp->new_color, TRUE, TRUE, 0);
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
  gtk_box_pack_start (GTK_BOX (colors_hbox), cnp->orig_color, TRUE, TRUE, 0);
  gtk_widget_show (cnp->orig_color);

  /*  dnd stuff  */
  gtk_drag_source_set (cnp->orig_color,
                       GDK_BUTTON1_MASK | GDK_BUTTON2_MASK,
                       color_notebook_target_table, n_color_notebook_targets,
                       GDK_ACTION_COPY | GDK_ACTION_MOVE);
  gimp_dnd_color_source_set (cnp->orig_color, color_notebook_drag_old_color,
			     cnp);

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
  gtk_widget_show (cnp->shell);
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

  cnp->orig_values[RED]   = red;
  cnp->orig_values[GREEN] = green;
  cnp->orig_values[BLUE]  = blue;
  cnp->orig_values[ALPHA] = alpha;

  cnp->values[RED]   = red;
  cnp->values[GREEN] = green;
  cnp->values[BLUE]  = blue;
  cnp->values[ALPHA] = alpha;

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

  cnp->values[RED]   = red;
  cnp->values[GREEN] = green;
  cnp->values[BLUE]  = blue;
  cnp->values[ALPHA] = alpha;

  color_notebook_update (cnp, UPDATE_NEW_COLOR | UPDATE_CALLER);
}

static void
color_notebook_ok_callback (GtkWidget *widget,
			    gpointer   data)
{
  ColorNotebook *cnp;

  cnp = (ColorNotebook *) data;

  if (cnp->callback)
    {
      (* cnp->callback) (cnp->values[RED],
			 cnp->values[GREEN],
			 cnp->values[BLUE],
			 cnp->values[ALPHA],
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
      (* cnp->callback) (cnp->orig_values[RED],
			 cnp->orig_values[GREEN],
			 cnp->orig_values[BLUE],
			 cnp->orig_values[ALPHA],
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

  color_notebook_update (cnp, UPDATE_NOTEBOOK);
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
  csel->info->methods.setcolor (csel->selector_data,
				cnp->values[RED],
				cnp->values[GREEN],
				cnp->values[BLUE],
				cnp->values[ALPHA]);
}

static void
color_notebook_update_caller (ColorNotebook *cnp)
{
  if (cnp && cnp->callback)
    {
      (* cnp->callback) (cnp->values[RED],
			 cnp->values[GREEN],
			 cnp->values[BLUE],
			 cnp->values[ALPHA],
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
      red    = cnp->values[RED];
      green  = cnp->values[GREEN];
      blue   = cnp->values[BLUE];
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
color_notebook_drag_new_color (GtkWidget *widget,
			       guchar    *r,
			       guchar    *g,
			       guchar    *b,
			       guchar    *a,
			       gpointer   data)
{
  ColorNotebook *cnp;

  cnp = (ColorNotebook *) data;

  *r = (guchar) cnp->values[RED];
  *g = (guchar) cnp->values[GREEN];
  *b = (guchar) cnp->values[BLUE];
  *a = (guchar) cnp->values[ALPHA];
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

  cnp->values[RED]   = (gint) r;
  cnp->values[GREEN] = (gint) g;
  cnp->values[BLUE]  = (gint) b;
  cnp->values[ALPHA] = (gint) a;

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
