/*
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This is a plug-in for the GIMP.
 *
 * Generates images containing vector type drawings.
 *
 * Copyright (C) 1997 Andy Thomas  alt@picnic.demon.co.uk
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
#include <string.h>

#ifdef __GNUC__
#warning GTK_DISABLE_DEPRECATED
#endif
#undef GTK_DISABLE_DEPRECATED

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "gfig.h"
#include "gfig-grid.h"
#include "gfig-preview.h"

#include "libgimp/stdplugins-intl.h"

#define PREVIEW_MASK  (GDK_EXPOSURE_MASK       | \
		       GDK_POINTER_MOTION_MASK | \
                       GDK_BUTTON_PRESS_MASK   | \
		       GDK_BUTTON_RELEASE_MASK | \
		       GDK_BUTTON_MOTION_MASK  | \
		       GDK_KEY_PRESS_MASK      | \
		       GDK_KEY_RELEASE_MASK)

static gint x_pos_val;
static gint y_pos_val;
static gint pos_tag = -1;
GtkWidget *status_label_dname;
GtkWidget *status_label_fname;
static GtkWidget *pos_label;       /* XY pos marker */
static guchar  preview_row[PREVIEW_SIZE * 4];
static guchar *pv_cache;
static gint    img_bpp;

static gboolean  	gfig_preview_expose     (GtkWidget *widget,
						 GdkEvent  *event);
static void      	gfig_preview_realize    (GtkWidget *widget);
static gboolean  	gfig_preview_events     (GtkWidget *widget,
						 GdkEvent  *event);
static void		cache_preview 		(GimpDrawable *drawable);

static gint      	gfig_invscale_x         (gint x);
static gint      	gfig_invscale_y         (gint y);
static GtkWidget*	gfig_pos_labels 	(void);
static GtkWidget* 	make_pos_info 		(void);
static GtkWidget*	make_status 		(void);

static void		gfig_pos_update		(gint x,
						 gint y);

GtkWidget *
make_preview (void)
{
  GtkWidget *xframe;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *table;
  GtkWidget *ruler;

  gfig_preview = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_widget_set_events (GTK_WIDGET (gfig_preview), PREVIEW_MASK);

  g_signal_connect (gfig_preview , "realize",
                    G_CALLBACK (gfig_preview_realize),
                    NULL);

  g_signal_connect (gfig_preview , "event",
                    G_CALLBACK (gfig_preview_events),
                    NULL);

  g_signal_connect_after (gfig_preview , "expose_event",
			  G_CALLBACK (gfig_preview_expose),
			  NULL);

  gtk_preview_size (GTK_PREVIEW (gfig_preview), preview_width,
		    preview_height);

  xframe = gtk_frame_new (NULL);

  gtk_frame_set_shadow_type (GTK_FRAME (xframe), GTK_SHADOW_IN);

  table = gtk_table_new (3, 3, FALSE);
  gtk_table_attach (GTK_TABLE (table), gfig_preview, 1, 2, 1, 2,
		    GTK_FILL , GTK_FILL , 0, 0);
  gtk_container_add (GTK_CONTAINER (xframe), table);

  ruler = gtk_hruler_new ();
  gtk_ruler_set_range (GTK_RULER (ruler), 0, preview_width, 0, PREVIEW_SIZE);
  g_signal_connect_swapped (gfig_preview, "motion_notify_event",
                            G_CALLBACK (GTK_WIDGET_CLASS (G_OBJECT_GET_CLASS (ruler))->motion_notify_event),
                            ruler);
  gtk_table_attach (GTK_TABLE (table), ruler, 1, 2, 0, 1,
		    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (ruler);

  ruler = gtk_vruler_new ();
  gtk_ruler_set_range (GTK_RULER (ruler), 0, preview_height, 0, PREVIEW_SIZE);
  g_signal_connect_swapped (gfig_preview, "motion_notify_event",
                            G_CALLBACK (GTK_WIDGET_CLASS (G_OBJECT_GET_CLASS (ruler))->motion_notify_event),
			    ruler);
  gtk_table_attach (GTK_TABLE (table), ruler, 0, 1, 1, 2,
		    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (ruler);

  gtk_widget_show (xframe);
  gtk_widget_show (table);

  vbox = gtk_vbox_new (FALSE, 0);
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), xframe, FALSE, FALSE, 0);

  gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  xframe = make_pos_info ();
  gtk_box_pack_start (GTK_BOX (vbox), xframe, TRUE, TRUE, 0);

  xframe = make_status ();
  gtk_box_pack_start (GTK_BOX (vbox), xframe, TRUE, TRUE, 0);

  gtk_widget_show (vbox);
  gtk_widget_show (hbox);

  return vbox;
}

/* Given a row then srink it down a bit */
static void
do_gfig_preview (guchar *dest_row,
		 guchar *src_row,
		 gint    width,
		 gint    dh,
		 gint    height,
		 gint    bpp)
{
  memcpy (dest_row, src_row, width * bpp);
}

void
refill_cache (GimpDrawable *drawable)
{
  static GdkCursor *preview_cursor1 = NULL;
  static GdkCursor *preview_cursor2 = NULL;

  if (!preview_cursor1)
    {
      GdkDisplay *display = gtk_widget_get_display (GTK_WIDGET (gfig_preview));

      preview_cursor1 = gdk_cursor_new_for_display (display, GDK_WATCH);
      preview_cursor2 = gdk_cursor_new_for_display (display, GDK_TOP_LEFT_ARROW);
    }

  gdk_window_set_cursor
    (gtk_widget_get_toplevel (GTK_WIDGET (gfig_preview))->window,
     preview_cursor1);

  gdk_window_set_cursor (gfig_preview->window, preview_cursor1);

  gdk_flush ();

  cache_preview (drawable);

  gdk_window_set_cursor
    (gtk_widget_get_toplevel (GTK_WIDGET (gfig_preview))->window,
     preview_cursor2);

  toggle_obj_type (NULL, GINT_TO_POINTER (selvals.otype));
}

/* Cache the preview image - updates are a lot faster. */
/* The preview_cache will contain the small image */

static void
cache_preview (GimpDrawable *drawable)
{
  GimpPixelRgn  src_rgn;
  gint          y, x;
  guchar       *src_rows;
  guchar       *p;
  gboolean      isgrey, has_alpha;
  gint		bpp, img_bpp;
  gint		sel_x1, sel_y1, sel_x2, sel_y2;
  gint    	sel_width, sel_height;

  gimp_drawable_mask_bounds (drawable->drawable_id,
			     &sel_x1, &sel_y1, &sel_x2, &sel_y2);

  sel_width  = sel_x2 - sel_x1;
  sel_height = sel_y2 - sel_y1;

  gimp_pixel_rgn_init (&src_rgn, drawable,
		       sel_x1, sel_y1, sel_width, sel_height, FALSE, FALSE);

  src_rows = g_new (guchar , sel_width * 4);
  p = pv_cache = g_new (guchar , preview_width * preview_height * 4);

  bpp = gimp_drawable_bpp (drawable->drawable_id);

  has_alpha = gimp_drawable_has_alpha (drawable->drawable_id);

  if (bpp < 3)
    {
      img_bpp = 3 + has_alpha;
    }
  else
    {
      img_bpp = bpp;
    }

  isgrey = gimp_drawable_is_gray (drawable->drawable_id);

  for (y = 0; y < preview_height; y++)
    {
      gimp_pixel_rgn_get_row (&src_rgn,
			      src_rows,
			      sel_x1,
			      sel_y1 + (y*sel_height)/preview_height,
			      sel_width);

      for (x = 0; x < (preview_width); x ++)
        {
          /* Get the pixels of each col */
          gint i;

          for (i = 0 ; i < 3; i++)
            p[x*img_bpp+i] =
              src_rows[((x*sel_width)/preview_width) * src_rgn.bpp +
		       ((isgrey)?0:i)];
          if (has_alpha)
            p[x*img_bpp+3] =
              src_rows[((x*sel_width)/preview_width) * src_rgn.bpp +
		       ((isgrey)?1:3)];
        }
      p += (preview_width*img_bpp);
    }

  g_free (src_rows);
}

void
dialog_update_preview (GimpDrawable *drawable)
{
  gint y;
  gint check, check_0, check_1;

  if (!selvals.showimage)
    {
      memset (preview_row, -1, preview_width*4);
      for (y = 0; y < preview_height; y++)
	{
	  gtk_preview_draw_row (GTK_PREVIEW (gfig_preview), preview_row,
				0, y, preview_width);
	}
      return;
    }

  if (!pv_cache)
    {
      refill_cache (drawable);
    }

  for (y = 0; y < preview_height; y++)
    {
      if ((y / GIMP_CHECK_SIZE) & 1)
	{
	  check_0 = GIMP_CHECK_DARK * 255;
	  check_1 = GIMP_CHECK_LIGHT * 255;
	}
      else
	{
	  check_0 = GIMP_CHECK_LIGHT * 255;
	  check_1 = GIMP_CHECK_DARK * 255;
	}

      do_gfig_preview (preview_row,
		       pv_cache + y * preview_width * img_bpp,
		       preview_width,
		       y,
		       preview_height,
		       img_bpp);

      if (img_bpp > 3)
	{
	  int i, j;
	  for (i = 0, j = 0 ; i < sizeof (preview_row); i += 4, j += 3)
	    {
	      gint alphaval;
	      if (((i/4) / GIMP_CHECK_SIZE) & 1)
		check = check_0;
	      else
		check = check_1;

	      alphaval = preview_row[i + 3];

	      preview_row[j] =
		check + (((preview_row[i] - check)*alphaval)/255);
	      preview_row[j + 1] =
		check + (((preview_row[i + 1] - check)*alphaval)/255);
	      preview_row[j + 2] =
		check + (((preview_row[i + 2] - check)*alphaval)/255);
	    }
	}

      gtk_preview_draw_row (GTK_PREVIEW (gfig_preview), preview_row,
			    0, y, preview_width);
    }
}

static void
gfig_preview_realize (GtkWidget *widget)
{
  GdkDisplay *display = gtk_widget_get_display (widget);

  gdk_window_set_cursor (gfig_preview->window,
			 gdk_cursor_new_for_display (display, GDK_CROSSHAIR));
}

static gboolean
gfig_preview_expose (GtkWidget *widget,
		     GdkEvent  *event)
{
  draw_grid ();
  draw_objects (pic_obj->obj_list, TRUE);

  return FALSE;
}

static gint
gfig_preview_events (GtkWidget *widget,
		     GdkEvent  *event)
{
  GdkEventButton *bevent;
  GdkEventMotion *mevent;
  GdkPoint        point;
  static gint     tmp_show_single = 0;

  switch (event->type)
    {
    case GDK_EXPOSE:
      break;

    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *) event;
      point.x = bevent->x;
      point.y = bevent->y;

      g_assert (need_to_scale == 0); /* If not out of step some how */

      /* Start drawing of object */
      if (selvals.otype >= MOVE_OBJ)
	{
	  if (!selvals.scaletoimage)
	    {
	      point.x = gfig_invscale_x (point.x);
	      point.y = gfig_invscale_y (point.y);
	    }
	  object_operation_start (&point, bevent->state & GDK_SHIFT_MASK);

	  /* If constraining save start pnt */
	  if (selvals.opts.snap2grid)
	    {
	      /* Save point to constained point ... if button 3 down */
	      if (bevent->button == 3)
		{
		  find_grid_pos (&point, &point, FALSE);
		}
	    }
	}
      else
	{
	  if (selvals.opts.snap2grid)
	    {
	      if (bevent->button == 3)
		{
		  find_grid_pos (&point, &point, FALSE);
		}
	      else
		{
		  find_grid_pos (&point, &point, FALSE);
		}
	    }
	  object_start (&point, bevent->state & GDK_SHIFT_MASK);
	}

      break;

    case GDK_BUTTON_RELEASE:
      bevent = (GdkEventButton *) event;
      point.x = bevent->x;
      point.y = bevent->y;

      if (selvals.opts.snap2grid)
	find_grid_pos (&point, &point, bevent->button == 3);

      /* Still got shift down ?*/
      if (selvals.otype >= MOVE_OBJ)
	{
	  if (!selvals.scaletoimage)
	    {
	      point.x = gfig_invscale_x (point.x);
	      point.y = gfig_invscale_y (point.y);
	    }
	  object_operation_end (&point, bevent->state & GDK_SHIFT_MASK);
	}
      else
	{
	  if (obj_creating)
	    {
	      object_end (&point, bevent->state & GDK_SHIFT_MASK);
	    }
	  else
	    break;
	}

      /* make small preview reflect changes ?*/
      list_button_update (current_obj);
      break;

    case GDK_MOTION_NOTIFY:
      mevent = (GdkEventMotion *) event;
      point.x = mevent->x;
      point.y = mevent->y;

      if (selvals.opts.snap2grid)
	find_grid_pos (&point, &point, mevent->state & GDK_BUTTON3_MASK);

      if (selvals.otype >= MOVE_OBJ)
	{
	  /* Moving objects around */
	  if (!selvals.scaletoimage)
	    {
	      point.x = gfig_invscale_x (point.x);
	      point.y = gfig_invscale_y (point.y);
	    }
	  object_operation (&point, mevent->state & GDK_SHIFT_MASK);
	  gfig_pos_update (point.x, point.y);
	  return FALSE;
	}

      if (obj_creating)
	{
	  object_update (&point);
	}
      gfig_pos_update (point.x, point.y);
      break;

    case GDK_KEY_PRESS:
      if ((tmp_show_single = obj_show_single) != -1)
	{
	  obj_show_single = -1;
	  draw_grid_clear ();
	}
      break;

    case GDK_KEY_RELEASE:
      if (tmp_show_single != -1)
	{
	  obj_show_single = tmp_show_single;
	  draw_grid_clear ();
	}
      break;

    default:
      break;
    }

  return FALSE;
}

static GtkWidget*
make_pos_info (void)
{
  GtkWidget *xframe;
  GtkWidget *hbox;
  GtkWidget *label;

  xframe = gtk_frame_new (_("Object Details"));

  hbox = gtk_hbox_new (TRUE, 6);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 2);
  gtk_container_add (GTK_CONTAINER (xframe), hbox);

  /* Add labels */
  label = gfig_pos_labels ();
  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
  gfig_pos_enable (NULL, NULL);

#if 0
  label = gfig_obj_size_label ();
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
#endif /* 0 */

  gtk_widget_show (hbox);
  gtk_widget_show (xframe);

  return xframe;
}

static GtkWidget*
make_status (void)
{
  GtkWidget *xframe;
  GtkWidget *table;
  GtkWidget *label;

  xframe = gtk_frame_new (_("Collection Details"));

  table = gtk_table_new (6, 6, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (table), 1, 6);
  gtk_container_set_border_width (GTK_CONTAINER (table), 2);

  label = gtk_label_new (_("Draw Name:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 1, 2, 0, 1,
		    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  label = gtk_label_new (_("Filename:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 1, 2, 1, 2,
		    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  status_label_dname = gtk_label_new (_("(none)"));
  gtk_misc_set_alignment (GTK_MISC (status_label_dname), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), status_label_dname, 2, 4, 0, 1,
		    GTK_FILL | GTK_EXPAND, 0, 0, 0);
  gtk_widget_show (status_label_dname);

  status_label_fname = gtk_label_new (_("(none)"));
  gtk_misc_set_alignment (GTK_MISC (status_label_fname), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), status_label_fname, 2, 4, 1, 2,
		    GTK_FILL | GTK_EXPAND, 0, 0, 0);
  gtk_widget_show (status_label_fname);

  gtk_container_add (GTK_CONTAINER (xframe), table);

  gtk_widget_show (table);
  gtk_widget_show (xframe);

  return xframe;
}

void
gfig_update_stat_labels (void)
{
  gchar str[45];

  if (current_obj->draw_name)
    sprintf (str, "%.34s", current_obj->draw_name);
  else
    sprintf (str,_("<NONE>"));

  gtk_label_set_text (GTK_LABEL (status_label_dname), str);

  if (current_obj->filename)
    {
      gint slen;
      gchar *hm  = (gchar *) g_get_home_dir ();
      gchar *dfn = g_strdup (current_obj->filename);

      if (hm != NULL && !strncmp (dfn, hm, strlen (hm)-1))
        {
          strcpy (dfn, "~");
          strcat (dfn, &dfn[strlen (hm)]);
        }
      if ((slen = strlen (dfn)) > 40)
	{
	  strncpy (str, dfn, 19);
	  str[19] = '\0';
	  strcat (str, "...");
	  strncat (str, &dfn[slen - 21], 19);
	  str[40] ='\0';
	}
      else
	sprintf (str, "%.40s", dfn);
      g_free (dfn);
#ifdef __EMX__
      g_free (hm);
#endif
    }
  else
    sprintf (str,_("<NONE>"));

  gtk_label_set_text (GTK_LABEL (status_label_fname), str);

}

static gint
gfig_invscale_x (gint x)
{
  if (!selvals.scaletoimage)
    return (gint) (x * scale_x_factor);
  else
    return x;
}

static gint
gfig_invscale_y (gint y)
{
  if (!selvals.scaletoimage)
    return (gint) (y * scale_y_factor);
  else
    return y;
}

static GtkWidget *
gfig_pos_labels (void)
{
  GtkWidget *label;
  GtkWidget *hbox;
  gchar      buf[256];

  hbox = gtk_hbox_new (FALSE, 6);
  gtk_widget_show (hbox);

  /* Position labels */
  label = gtk_label_new (_("XY Position:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  pos_label = gtk_label_new ("");
  gtk_box_pack_start (GTK_BOX (hbox), pos_label, FALSE, FALSE, 0);
  gtk_widget_show (pos_label);

  g_snprintf (buf, sizeof (buf), "%d, %d", 0, 0);
  gtk_label_set_text (GTK_LABEL (pos_label), buf);

  return hbox;
}

void
gfig_pos_enable (GtkWidget *widget,
		 gpointer   data)
{
  gboolean enable = selvals.showpos;

  gtk_widget_set_sensitive (GTK_WIDGET (pos_label), enable);
}

static void
gfig_pos_update_labels (gpointer data)
{
  static gchar buf[256];

  pos_tag = -1;

  g_snprintf (buf, sizeof (buf), "%d, %d", x_pos_val, y_pos_val);
  gtk_label_set_text (GTK_LABEL (pos_label), buf);
}

static void
gfig_pos_update (gint x,
		 gint y)
{
  if ((x_pos_val !=x || y_pos_val != y) && pos_tag == -1 && selvals.showpos)
    {
      x_pos_val = x;
      y_pos_val = y;
      gfig_pos_update_labels (NULL);
    }
}
