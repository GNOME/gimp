/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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
#include <time.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "about_dialog.h"
#include "appenv.h"
#include "authors.h"
#include "gimpdnd.h"
#include "gimphelp.h"

#include "libgimp/gimpfeatures.h"

#include "libgimp/gimpenv.h"
#include "libgimp/gimpintl.h"
#include "libgimp/gimpmath.h"

#include "pixmaps/wilber2.xpm"


#define ANIMATION_STEPS 16
#define ANIMATION_SIZE   2

static gboolean  about_dialog_load_logo   (GtkWidget      *window);
static void      about_dialog_destroy     (GtkObject      *object,
					   gpointer        data);
static void      about_dialog_unmap       (GtkWidget      *widget,
					   GdkEvent       *event,
					   gpointer        data);
static gint      about_dialog_logo_expose (GtkWidget      *widget,
					   GdkEventExpose *event,
					   gpointer        data);
static gint      about_dialog_button      (GtkWidget      *widget,
					   GdkEventButton *event,
					   gpointer        data);
static gint      about_dialog_key         (GtkWidget      *widget,
					   GdkEventKey    *event,
					   gpointer        data);
static void      about_dialog_tool_drop   (GtkWidget      *widget,
					   ToolType        tool,
					   gpointer        data);
static gint      about_dialog_timer       (gpointer        data);


static GtkWidget *about_dialog     = NULL;
static GtkWidget *logo_area        = NULL;
static GtkWidget *scroll_area      = NULL;
static GdkPixmap *logo_pixmap      = NULL;
static GdkPixmap *scroll_pixmap    = NULL;
static guchar    *dissolve_map     = NULL;
static gint       dissolve_width;
static gint       dissolve_height;
static gint       logo_width       = 0;
static gint       logo_height      = 0;
static gboolean   do_animation     = FALSE;
static gboolean   do_scrolling     = FALSE;
static gint       scroll_state     = 0;
static gint       frame            = 0;
static gint       offset           = 0;
static gint       timer            = 0;
static gint       hadja_state      = 0;
static gchar    **scroll_text      = authors;
static gint       nscroll_texts    = sizeof (authors) / sizeof (authors[0]);
static gint       scroll_text_widths[sizeof (authors) / sizeof (authors[0])];
static gint       cur_scroll_text  = 0;
static gint       cur_scroll_index = 0;
static gint       shuffle_array[sizeof (authors) / sizeof (authors[0])];

/*  dnd stuff  */
static GtkTargetEntry tool_target_table[] =
{
  GIMP_TARGET_TOOL
};
static guint n_tool_targets = (sizeof (tool_target_table) /
                               sizeof (tool_target_table[0]));

static gchar *drop_text[] = 
{ 
  "We are The GIMP." ,
  "Prepare to be manipulated.",
  "Resistance is futile."
};

static gchar *hadja_text[] =
{
  "Hadjaha!",
  "Nej!",
#ifndef GDK_USE_UTF8_MBS
  "Tvärtom!"
#else
  "TvÃ¤rtom!"
#endif
};

void
about_dialog_create (void)
{
  GtkWidget *vbox;
  GtkWidget *aboutframe;
  GtkWidget *label;
  GtkWidget *alignment;
  GtkStyle  *style;
  GdkFont   *font;
  gint       max_width;
  gint       i;
  gchar     *label_text;

  if (!about_dialog)
    {
      about_dialog = gtk_window_new (GTK_WINDOW_DIALOG);
      gtk_window_set_wmclass (GTK_WINDOW (about_dialog), "about_dialog", "Gimp");
      gtk_window_set_title (GTK_WINDOW (about_dialog), _("About the GIMP"));
      gtk_window_set_policy (GTK_WINDOW (about_dialog), FALSE, FALSE, FALSE);
      gtk_window_set_position (GTK_WINDOW (about_dialog), GTK_WIN_POS_CENTER);

      gimp_help_connect_help_accel (about_dialog, gimp_standard_help_func,
				    "dialogs/about.html");

      gtk_signal_connect (GTK_OBJECT (about_dialog), "destroy",
			  GTK_SIGNAL_FUNC (about_dialog_destroy),
			  NULL);
      gtk_signal_connect (GTK_OBJECT (about_dialog), "unmap_event",
			  GTK_SIGNAL_FUNC (about_dialog_unmap),
			  NULL);
      gtk_signal_connect (GTK_OBJECT (about_dialog), "button_press_event",
			  GTK_SIGNAL_FUNC (about_dialog_button),
			  NULL);
      gtk_signal_connect (GTK_OBJECT (about_dialog), "key_press_event",
			  GTK_SIGNAL_FUNC (about_dialog_key),
			  NULL);
      
      /*  dnd stuff  */
      gtk_drag_dest_set (about_dialog,
			 GTK_DEST_DEFAULT_MOTION |
			 GTK_DEST_DEFAULT_DROP,
			 tool_target_table, n_tool_targets,
			 GDK_ACTION_COPY); 
      gimp_dnd_tool_dest_set (about_dialog, about_dialog_tool_drop, NULL);

      gtk_widget_set_events (about_dialog, GDK_BUTTON_PRESS_MASK);

      if (!about_dialog_load_logo (about_dialog))
	{
	  gtk_widget_destroy (about_dialog);
	  about_dialog = NULL;
	  return;
	}

      vbox = gtk_vbox_new (FALSE, 1);
      gtk_container_set_border_width (GTK_CONTAINER (vbox), 1);
      gtk_container_add (GTK_CONTAINER (about_dialog), vbox);
      gtk_widget_show (vbox);

      aboutframe = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (aboutframe), GTK_SHADOW_IN);
      gtk_container_set_border_width (GTK_CONTAINER (aboutframe), 0);
      gtk_box_pack_start (GTK_BOX (vbox), aboutframe, TRUE, TRUE, 0);
      gtk_widget_show (aboutframe);

      logo_area = gtk_drawing_area_new ();
      gtk_signal_connect (GTK_OBJECT (logo_area), "expose_event",
			  GTK_SIGNAL_FUNC (about_dialog_logo_expose),
			  NULL);
      gtk_drawing_area_size (GTK_DRAWING_AREA (logo_area),
			     logo_width, logo_height);
      gtk_widget_set_events (logo_area, GDK_EXPOSURE_MASK);
      gtk_container_add (GTK_CONTAINER (aboutframe), logo_area);
      gtk_widget_show (logo_area);

      gtk_widget_realize (logo_area);
      gdk_window_set_background (logo_area->window, &logo_area->style->black);

      /* this is a font, provide only one single font definition */
      font = gdk_font_load (_("-*-helvetica-medium-r-normal--*-140-*-*-*-*-*-*"));
      if (font)
	{
	  style = gtk_style_new ();
	  gdk_font_unref (style->font);
	  style->font = font;
	  gdk_font_ref (style->font);
	  gtk_widget_push_style (style);
	  gtk_style_unref (style);
	}
      label_text = g_strdup_printf (_("Version %s brought to you by"),
				    GIMP_VERSION);
      label = gtk_label_new (label_text);
      g_free (label_text);
      label_text = NULL;
      gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, TRUE, 0);
      gtk_widget_show (label);

      label = gtk_label_new ("Spencer Kimball & Peter Mattis");
      gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, TRUE, 0);
      gtk_widget_show (label);

      gtk_widget_pop_style ();

      alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
      gtk_box_pack_start (GTK_BOX (vbox), alignment, FALSE, TRUE, 0);
      gtk_widget_show (alignment);

      aboutframe = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (aboutframe), GTK_SHADOW_IN);
      gtk_container_set_border_width (GTK_CONTAINER (aboutframe), 0);
      gtk_container_add (GTK_CONTAINER (alignment), aboutframe);
      gtk_widget_show (aboutframe);

      max_width = 0;
      for (i = 0; i < nscroll_texts; i++)
	{
	  scroll_text_widths[i] = gdk_string_width (aboutframe->style->font,
						    scroll_text[i]);
	  max_width = MAX (max_width, scroll_text_widths[i]);
	}
      for (i = 0; i < (sizeof (drop_text) / sizeof (drop_text[0])); i++)
	{
	  max_width = MAX (max_width, 
			   gdk_string_width (aboutframe->style->font, drop_text[i]));
	}
      for (i = 0; i < (sizeof (hadja_text) / sizeof (hadja_text[0])); i++)
	{
	  max_width = MAX (max_width,
			   gdk_string_width (aboutframe->style->font, hadja_text[i]));
	}
      scroll_area = gtk_drawing_area_new ();
      gtk_drawing_area_size (GTK_DRAWING_AREA (scroll_area),
			     max_width + 10,
			     aboutframe->style->font->ascent +
			     aboutframe->style->font->descent);
      gtk_widget_set_events (scroll_area, GDK_BUTTON_PRESS_MASK);
      gtk_container_add (GTK_CONTAINER (aboutframe), scroll_area);
      gtk_widget_show (scroll_area);

      label = gtk_label_new (_("Please visit http://www.gimp.org/ for more info"));
      gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, TRUE, 0);
      gtk_widget_show (label);

      gtk_widget_realize (scroll_area);
      gdk_window_set_background (scroll_area->window,
				 &scroll_area->style->white);
    }

  if (!GTK_WIDGET_VISIBLE (about_dialog))
    {
      gtk_widget_show (about_dialog);

      do_animation = TRUE;
      do_scrolling = FALSE;
      scroll_state = 0;
      frame = 0;
      offset = 0;
      cur_scroll_text = 0;

      if (!double_speed && hadja_state != 7)
	{
	  for (i = 0; i < nscroll_texts; i++) 
	    {
	      shuffle_array[i] = i;
	    }

	  for (i = 0; i < nscroll_texts; i++) 
	    {
	      gint j;

	      j = rand() % nscroll_texts;
	      if (i != j) 
		{
		  gint t;

		  t = shuffle_array[j];
		  shuffle_array[j] = shuffle_array[i];
		  shuffle_array[i] = t;
		}
	    }
	  cur_scroll_text = rand() % nscroll_texts;
	}
    }
  else 
    {
      gdk_window_raise (about_dialog->window);
    }
}

static gboolean
about_dialog_load_logo (GtkWidget *window)
{
  GtkWidget *preview;
  GdkGC     *gc;
  gchar      buf[1024];
  gchar     *filename;
  guchar    *pixelrow;
  FILE      *fp;
  gint       count;
  gint       i, j, k;

  if (logo_pixmap)
    return TRUE;

  filename = g_strconcat (gimp_data_directory (),
			  G_DIR_SEPARATOR_S,
			  "gimp_logo.ppm",
			  NULL);
  fp = fopen (filename, "rb");
  g_free (filename);

  if (!fp)
    return FALSE;

  fgets (buf, 1024, fp);

  if (strncmp (buf, "P6", 2) != 0)
    {
      fclose (fp);
      return FALSE;
    }

  fgets (buf, 1024, fp);
  fgets (buf, 1024, fp);
  sscanf (buf, "%d %d", &logo_width, &logo_height);

  fgets (buf, 1024, fp);
  if (strncmp (buf, "255", 3) != 0)
    {
      fclose (fp);
      return FALSE;
    }

  preview = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_size (GTK_PREVIEW (preview), logo_width, logo_height);
  pixelrow = g_new (guchar, logo_width * 3);

  for (i = 0; i < logo_height; i++)
    {
      count = fread (pixelrow, sizeof (guchar), logo_width * 3, fp);
      if (count != (logo_width * 3))
	{
	  gtk_widget_destroy (preview);
	  g_free (pixelrow);
	  fclose (fp);
	  return FALSE;
	}

      gtk_preview_draw_row (GTK_PREVIEW (preview), pixelrow, 0, i, logo_width);
    }

  gtk_widget_realize (window);
  logo_pixmap = gdk_pixmap_new (window->window, logo_width, logo_height, 
				gtk_preview_get_visual ()->depth);
  gc = gdk_gc_new (logo_pixmap);
  gtk_preview_put (GTK_PREVIEW (preview),
		   logo_pixmap, gc,
		   0, 0, 0, 0, logo_width, logo_height);
  gdk_gc_destroy (gc);

  gtk_widget_unref (preview);
  g_free (pixelrow);

  fclose (fp);

  dissolve_width =
    (logo_width / ANIMATION_SIZE) + (logo_width % ANIMATION_SIZE == 0 ? 0 : 1);
  dissolve_height =
    (logo_height / ANIMATION_SIZE) + (logo_height % ANIMATION_SIZE == 0 ? 0 : 1);

  dissolve_map = g_new (guchar, dissolve_width * dissolve_height);

  srand (time (NULL));

  for (i = 0, k = 0; i < dissolve_height; i++)
    for (j = 0; j < dissolve_width; j++, k++)
      dissolve_map[k] = rand () % ANIMATION_STEPS;

  return TRUE;
}

static void
about_dialog_destroy (GtkObject *object,
		      gpointer   data)
{
  about_dialog = NULL;
  about_dialog_unmap (NULL, NULL, NULL);
}

static void
about_dialog_unmap (GtkWidget *widget,
		    GdkEvent  *event,
		    gpointer   data)
{
  if (timer)
    {
      gtk_timeout_remove (timer);
      timer = 0;
    }
}

static gint
about_dialog_logo_expose (GtkWidget      *widget,
			  GdkEventExpose *event,
			  gpointer        data)
{
  if (do_animation)
    {
      if (!timer)
	{
	  about_dialog_timer (widget);
	  timer = gtk_timeout_add (75, about_dialog_timer, NULL);
	}
    }
  else
    {
      /* If we draw beyond the boundaries of the pixmap, then X
	 will generate an expose area for those areas, starting
	 an infinite cycle. We now set allow_grow = FALSE, so
	 the drawing area can never be bigger than the preview.
         Otherwise, it would be necessary to intersect event->area
         with the pixmap boundary rectangle. */

      gdk_draw_pixmap (widget->window,
		       widget->style->black_gc,
		       logo_pixmap, 
		       event->area.x, event->area.y,
		       event->area.x, event->area.y,
		       event->area.width, event->area.height);
    }

  return FALSE;
}

static gint
about_dialog_button (GtkWidget      *widget,
		     GdkEventButton *event,
		     gpointer        data)
{
  if (timer)
    gtk_timeout_remove (timer);
  timer = 0;
  frame = 0;

  gtk_widget_hide (about_dialog);

  return FALSE;
}

static gint
about_dialog_key (GtkWidget      *widget,
		  GdkEventKey    *event,
		  gpointer        data)
{
  gint i;
  
  if (hadja_state == 7)
    return FALSE;
    
  switch (event->keyval)
    {
    case GDK_h:
    case GDK_H:
      if (hadja_state == 0 || hadja_state == 5)
	hadja_state++;
      else
	hadja_state = 1;
      break;
    case GDK_a:
    case GDK_A:
      if (hadja_state == 1 || hadja_state == 4 || hadja_state == 6)
	hadja_state++;
      else
	hadja_state = 0;
      break;
    case GDK_d:
    case GDK_D:
      if (hadja_state == 2)
	hadja_state++;
      else
	hadja_state = 0;
      break;
    case GDK_j:
    case GDK_J:
      if (hadja_state == 3)
	hadja_state++;
      else
	hadja_state = 0;
      break;
    default:
      hadja_state = 0;
    }

  if (hadja_state == 7)
    {
      scroll_text = hadja_text;
      nscroll_texts = sizeof (hadja_text) / sizeof (hadja_text[0]);
      
      for (i = 0; i < nscroll_texts; i++)
	{
	  shuffle_array[i] = i;
	  scroll_text_widths[i] = gdk_string_width (scroll_area->style->font,
						    scroll_text[i]);
	}
      
      scroll_state = 0;
      cur_scroll_index = 0;
      cur_scroll_text = 0;
      offset = 0;
    }
  
  return FALSE;
}

static void
about_dialog_tool_drop (GtkWidget *widget,
			ToolType   tool,
			gpointer   data)
{
  GdkPixmap *pixmap = NULL;
  GdkBitmap *mask   = NULL;
  gint width  = 0;
  gint height = 0;
  gint i;
  
  if (do_animation)
    return;

  if (timer)
    gtk_timeout_remove (timer);

  timer = gtk_timeout_add (75, about_dialog_timer, NULL);

  frame = 0;
  do_animation = TRUE;
  do_scrolling = FALSE;

  gdk_draw_rectangle (logo_pixmap,
		      logo_area->style->white_gc,
		      TRUE,
		      0, 0,
		      logo_area->allocation.width,
		      logo_area->allocation.height);

  pixmap =
    gdk_pixmap_create_from_xpm_d (widget->window,
                                  &mask,
                                  NULL,
                                  wilber2_xpm);

  gdk_window_get_size (pixmap, &width, &height);

  if (logo_area->allocation.width  >= width &&
      logo_area->allocation.height >= height)
    {
      gint x, y;

      x = (logo_area->allocation.width  - width) / 2;
      y = (logo_area->allocation.height - height) / 2;

      gdk_gc_set_clip_mask (logo_area->style->black_gc, mask);
      gdk_gc_set_clip_origin (logo_area->style->black_gc, x, y);

      gdk_draw_pixmap (logo_pixmap,
                       logo_area->style->black_gc,
                       pixmap, 0, 0,
                       x, y,
                       width, height);

      gdk_gc_set_clip_mask (logo_area->style->black_gc, NULL);
      gdk_gc_set_clip_origin (logo_area->style->black_gc, 0, 0);
    }

  gdk_pixmap_unref (pixmap);
  gdk_bitmap_unref (mask);

  scroll_text = drop_text;
  nscroll_texts = sizeof (drop_text) / sizeof (drop_text[0]);

  for (i = 0; i < nscroll_texts; i++)
    {
      shuffle_array[i] = i;
      scroll_text_widths[i] = gdk_string_width (scroll_area->style->font,
						scroll_text[i]);
    }

  scroll_state = 0;
  cur_scroll_index = 0;
  cur_scroll_text = 0;
  offset = 0;

  double_speed = TRUE;
}

static gint
about_dialog_timer (gpointer data)
{
  gint i, j, k;
  gint return_val;

  return_val = TRUE;

  if (do_animation)
    {
      if (logo_area->allocation.width != 1)
	{
	  for (i = 0, k = 0; i < dissolve_height; i++)
	    for (j = 0; j < dissolve_width; j++, k++)
	      if (frame == dissolve_map[k])
		{
		  gdk_draw_pixmap (logo_area->window,
				   logo_area->style->black_gc,
				   logo_pixmap,
				   j * ANIMATION_SIZE, i * ANIMATION_SIZE,
				   j * ANIMATION_SIZE, i * ANIMATION_SIZE,
				   ANIMATION_SIZE, ANIMATION_SIZE);
		}

	  frame += 1;

	  if (frame == ANIMATION_STEPS)
	    {
	      do_animation = FALSE;
	      do_scrolling = TRUE;
	      frame = 0;

	      timer = gtk_timeout_add (75, about_dialog_timer, NULL);

	      return FALSE;
	    }
	}
    }

  if (do_scrolling)
    {
      if (!scroll_pixmap)
	scroll_pixmap = gdk_pixmap_new (scroll_area->window,
					scroll_area->allocation.width,
					scroll_area->allocation.height,
					-1);

      switch (scroll_state)
	{
	case 1:
	  scroll_state = 2;
	  timer = gtk_timeout_add (700, about_dialog_timer, NULL);
	  return_val = FALSE;
	  break;
	case 2:
	  scroll_state = 3;
	  timer = gtk_timeout_add (75, about_dialog_timer, NULL);
	  return_val = FALSE;
	  break;
	}

      if (offset > (scroll_text_widths[cur_scroll_text] +
		    scroll_area->allocation.width))
	{
	  scroll_state = 0;
	  cur_scroll_index += 1;
	  if (cur_scroll_index == nscroll_texts)
	    cur_scroll_index = 0;
	  
	  cur_scroll_text = shuffle_array[cur_scroll_index];

	  offset = 0;
	}

      gdk_draw_rectangle (scroll_pixmap,
			  scroll_area->style->white_gc,
			  TRUE, 0, 0,
			  scroll_area->allocation.width,
			  scroll_area->allocation.height);
      gdk_draw_string (scroll_pixmap,
		       scroll_area->style->font,
		       scroll_area->style->black_gc,
		       scroll_area->allocation.width - offset,
		       scroll_area->style->font->ascent,
		       scroll_text[cur_scroll_text]);
      gdk_draw_pixmap (scroll_area->window,
		       scroll_area->style->black_gc,
		       scroll_pixmap, 0, 0, 0, 0,
		       scroll_area->allocation.width,
		       scroll_area->allocation.height);

      offset += 15;
      if (scroll_state == 0)
	{
	  if (offset > ((scroll_area->allocation.width +
			 scroll_text_widths[cur_scroll_text]) / 2))
	    {
	      scroll_state = 1;
	      offset = (scroll_area->allocation.width +
			scroll_text_widths[cur_scroll_text]) / 2;
	    }
	}
    }

  return return_val;
}
