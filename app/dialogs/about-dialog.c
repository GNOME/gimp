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

#include "libgimpmath/gimpmath.h"
#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "gui-types.h"

#include "core/gimptoolinfo.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpdnd.h"

#include "about-dialog.h"
#include "authors.h"

#include "gimp-intl.h"

static gchar *founders[] =
{
  N_("Version %s brought to you by"),
  "Spencer Kimball & Peter Mattis"
};

static gchar *translators[] =
{
  N_("Translation by"),
  /* Translators: insert your names here, separated by newline */
  /* we'd prefer just the names, please no email adresses.     */
  N_("translator-credits"),
};

static gchar *contri_intro[] =
{
  N_("Contributions by")
};

static gchar **translator_names = NULL;

typedef struct
{
  GtkWidget    *about_dialog;
  GtkWidget    *logo_area;
  GdkPixmap    *logo_pixmap;
  GdkRectangle  pixmaparea;

  GdkBitmap    *shape_bitmap;
  GdkGC        *trans_gc;
  GdkGC        *opaque_gc;

  GdkRectangle  textarea;
  gdouble       text_size;
  gdouble       min_text_size;
  PangoLayout  *layout;

  gint          timer;

  gint          index;
  gint          animstep;
  gboolean      visible;
  gint          textrange[2];
  gint          state;

} GimpAboutInfo;

PangoColor gradient[] =
{
  { 50372, 50372, 50115 },
  { 65535, 65535, 65535 },
  { 10000, 10000, 10000 },
};

PangoColor foreground = { 10000, 10000, 10000 };
PangoColor background = { 50372, 50372, 50115 };

/* backup values */

PangoColor grad1ent[] =
{
  { 0xff * 257, 0xba * 257, 0x00 * 257 },
  { 37522, 51914, 57568 },
};

PangoColor foregr0und = { 37522, 51914, 57568 };
PangoColor backgr0und = { 0, 0, 0 };

static GimpAboutInfo about_info = { 0 };
static gboolean pp = FALSE;

static gboolean  about_dialog_load_logo   (GtkWidget      *window);
static void      about_dialog_destroy     (GtkObject      *object,
                                           gpointer        data);
static void      about_dialog_unmap       (GtkWidget      *widget,
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
static void      reshuffle_array          (void);
static gboolean  about_dialog_timer       (gpointer        data);


static gboolean     double_speed     = FALSE;

static PangoFontDescription *font_desc = NULL;
static gchar      **scroll_text      = authors;
static gint         nscroll_texts    = G_N_ELEMENTS (authors);
static gint         shuffle_array[G_N_ELEMENTS (authors)];


GtkWidget *
about_dialog_create (void)
{
  if (! about_info.about_dialog)
    {
      GtkWidget *widget;
      GdkGCValues shape_gcv;

      about_info.textarea.x = 0;
      about_info.textarea.y = 220;
      about_info.textarea.width = 299;
      about_info.textarea.height = 49;

      about_info.visible = FALSE;
      about_info.state = 0;
      about_info.animstep = -1;

      widget = gtk_window_new (GTK_WINDOW_TOPLEVEL);
      about_info.about_dialog = widget;

      gtk_window_set_type_hint (GTK_WINDOW (widget),
                                GDK_WINDOW_TYPE_HINT_SPLASHSCREEN);
      gtk_window_set_wmclass (GTK_WINDOW (widget), "about_dialog", "Gimp");
      gtk_window_set_title (GTK_WINDOW (widget), _("About The GIMP"));
      gtk_window_set_position (GTK_WINDOW (widget), GTK_WIN_POS_CENTER);

      /* The window must not be resizeable, since otherwise
       * the copying of nonexisting parts of the image pixmap
       * would result in an endless loop due to the X-Server
       * generating expose events on the pixmap. */

      gtk_window_set_resizable (GTK_WINDOW (widget), FALSE);

      g_signal_connect (widget, "destroy",
                        G_CALLBACK (about_dialog_destroy),
                        NULL);
      g_signal_connect (widget, "unmap",
                        G_CALLBACK (about_dialog_unmap),
                        NULL);
      g_signal_connect (widget, "button_press_event",
                        G_CALLBACK (about_dialog_button),
                        NULL);
      g_signal_connect (widget, "key_press_event",
                        G_CALLBACK (about_dialog_key),
                        NULL);
      
      gtk_widget_set_events (widget, GDK_BUTTON_PRESS_MASK);

      if (! about_dialog_load_logo (widget))
        {
	  gtk_widget_destroy (widget);
	  about_info.about_dialog = NULL;
	  return NULL;
	}

      widget = gtk_drawing_area_new ();
      about_info.logo_area = widget;

      gtk_widget_set_size_request (widget,
                                   about_info.pixmaparea.width,
                                   about_info.pixmaparea.height);
      gtk_widget_set_events (widget, GDK_EXPOSURE_MASK);
      gtk_container_add (GTK_CONTAINER (about_info.about_dialog),
                         widget);
      gtk_widget_show (widget);

      g_signal_connect (widget, "expose_event",
			G_CALLBACK (about_dialog_logo_expose),
			NULL);

      gtk_widget_realize (widget);
      gdk_window_set_background (widget->window,
                                 &(widget->style)->black);

      /* setup shape bitmap */

      about_info.shape_bitmap = gdk_pixmap_new (widget->window,
                                                about_info.pixmaparea.width,
                                                about_info.pixmaparea.height,
                                                1);
      about_info.trans_gc = gdk_gc_new (about_info.shape_bitmap);

      about_info.opaque_gc = gdk_gc_new (about_info.shape_bitmap);
      gdk_gc_get_values (about_info.opaque_gc, &shape_gcv);
      gdk_gc_set_foreground (about_info.opaque_gc, &shape_gcv.background);

      gdk_draw_rectangle (about_info.shape_bitmap,
                          about_info.trans_gc,
                          TRUE,
                          0, 0,
                          about_info.pixmaparea.width,
                          about_info.pixmaparea.height);

      gdk_draw_line (about_info.shape_bitmap,
                     about_info.opaque_gc,
                     0, 0,
                     about_info.pixmaparea.width, 0);

      gtk_widget_shape_combine_mask (about_info.about_dialog,
                                     about_info.shape_bitmap,
                                     0, 0);
      about_info.layout = gtk_widget_create_pango_layout (about_info.logo_area,
                                                          NULL);
      g_object_weak_ref (G_OBJECT (about_info.logo_area), 
                         (GWeakNotify) g_object_unref, about_info.layout);

      font_desc = pango_font_description_from_string ("Sans, 11");

      pango_layout_set_font_description (about_info.layout, font_desc);
      pango_layout_set_justify (about_info.layout, PANGO_ALIGN_CENTER);

    }

  if (! GTK_WIDGET_VISIBLE (about_info.about_dialog))
    {
      if (! double_speed)
	{
          about_info.state = 0;
          about_info.index = 0;

          reshuffle_array ();
          pango_layout_set_text (about_info.layout, "", -1);
	}
    }

  gtk_window_present (GTK_WINDOW (about_info.about_dialog));

  return about_info.about_dialog;
}

static gboolean
about_dialog_load_logo (GtkWidget *window)
{
  gchar       *filename;
  GdkPixbuf   *pixbuf;
  GdkGC       *gc;
  gint         width;
  PangoLayout *layout;
  PangoFontDescription *desc;

  if (about_info.logo_pixmap)
    return TRUE;

  filename = g_build_filename (gimp_data_directory (), "images",
                               "gimp_logo.png", NULL);

  pixbuf = gdk_pixbuf_new_from_file (filename, NULL);
  g_free (filename);

  if (! pixbuf)
    return FALSE;

  about_info.pixmaparea.x = 0;
  about_info.pixmaparea.y = 0;
  about_info.pixmaparea.width = gdk_pixbuf_get_width (pixbuf);
  about_info.pixmaparea.height = gdk_pixbuf_get_height (pixbuf);

  gtk_widget_realize (window);

  about_info.logo_pixmap = gdk_pixmap_new (window->window,
                                about_info.pixmaparea.width,
                                about_info.pixmaparea.height,
                                gtk_widget_get_visual (window)->depth);

  layout = gtk_widget_create_pango_layout (window, NULL);
  desc = pango_font_description_from_string ("Sans, Italic 9");
  pango_layout_set_font_description (layout, desc);
  pango_layout_set_justify (layout, PANGO_ALIGN_CENTER);
  pango_layout_set_text (layout, GIMP_VERSION, -1);
  
  gc = gdk_gc_new (about_info.logo_pixmap);

  gdk_draw_pixbuf (GDK_DRAWABLE (about_info.logo_pixmap),
                   gc, pixbuf,
                   0, 0, 0, 0,
                   about_info.pixmaparea.width,
                   about_info.pixmaparea.height,
                   GDK_RGB_DITHER_NORMAL, 0, 0);

  pango_layout_get_pixel_size (layout, &width, NULL);

  gdk_draw_layout (GDK_DRAWABLE (about_info.logo_pixmap),
                   gc, 222, 137, layout);

  about_info.pixmaparea.height /= 2;

  g_object_unref (gc);
  g_object_unref (pixbuf);
  g_object_unref (layout);

  return TRUE;
}

static void
about_dialog_destroy (GtkObject *object,
		      gpointer   data)
{
  about_info.about_dialog = NULL;
  about_dialog_unmap (NULL, NULL);
}

static void
about_dialog_unmap (GtkWidget *widget,
                    gpointer   data)
{
  if (about_info.timer)
    {
      g_source_remove (about_info.timer);
      about_info.timer = 0;
    }

  if (about_info.about_dialog)
    {
      gdk_draw_rectangle (about_info.shape_bitmap,
                          about_info.trans_gc,
                          TRUE,
                          0, 0,
                          about_info.pixmaparea.width,
                          about_info.pixmaparea.height);

      gdk_draw_line (about_info.shape_bitmap,
                     about_info.opaque_gc,
                     0, 0,
                     about_info.pixmaparea.width, 0);

      gtk_widget_shape_combine_mask (about_info.about_dialog,
                                     about_info.shape_bitmap,
                                     0, 0);
    }
}

static gint
about_dialog_logo_expose (GtkWidget      *widget,
			  GdkEventExpose *event,
			  gpointer        data)
{
  gint width, height;

  if (!about_info.timer)
    {
      GdkModifierType mask;

      /* No timeout, we were unmapped earlier */
      about_info.state = 0;
      about_info.index = 1;
      about_info.animstep = -1;
      about_info.visible = FALSE;
      reshuffle_array ();
      gdk_draw_rectangle (about_info.shape_bitmap,
                          about_info.trans_gc,
                          TRUE,
                          0, 0,
                          about_info.pixmaparea.width,
                          about_info.pixmaparea.height);

      gdk_draw_line (about_info.shape_bitmap,
                     about_info.opaque_gc,
                     0, 0,
                     about_info.pixmaparea.width, 0);

      gtk_widget_shape_combine_mask (about_info.about_dialog,
                                     about_info.shape_bitmap,
                                     0, 0);
      about_info.timer = g_timeout_add (30, about_dialog_timer, NULL);

      gdk_window_get_pointer (NULL, NULL, NULL, &mask);

      /* weird magic to determine the way the logo should be shown */
      mask &= ~GDK_BUTTON3_MASK;
      pp = (mask &= (GDK_SHIFT_MASK | GDK_CONTROL_MASK) & 
                    (GDK_CONTROL_MASK | GDK_MOD1_MASK) &
                    (GDK_MOD1_MASK | ~GDK_SHIFT_MASK),
      height = mask ? (about_info.pixmaparea.height > 0) &&
                      (about_info.pixmaparea.width > 0): 0);

    }

  /* only operate on the region covered by the pixmap */
  if (! gdk_rectangle_intersect (&(about_info.pixmaparea),
                                 &(event->area),
                                 &(event->area)))
    return FALSE;

  gdk_gc_set_clip_rectangle (widget->style->black_gc, &(event->area));

  gdk_draw_drawable (widget->window,
                     widget->style->white_gc,
                     about_info.logo_pixmap,
                     event->area.x, event->area.y +
                     (pp ? about_info.pixmaparea.height : 0),
                     event->area.x, event->area.y,
                     event->area.width, event->area.height);

  if (pp && about_info.state == 0 &&
      (about_info.index < about_info.pixmaparea.height / 12 ||
       about_info.index < g_random_int () %
                          (about_info.pixmaparea.height / 8 + 13)))
    {
      gdk_draw_rectangle (widget->window,
                          widget->style->black_gc,
                          TRUE,
                          0, 0, about_info.pixmaparea.width, 158);
                          
    }

  if (about_info.visible == TRUE)
    {
      gint layout_x, layout_y;

      pango_layout_get_pixel_size (about_info.layout, &width, &height);

      layout_x = about_info.textarea.x +
                        (about_info.textarea.width - width) / 2;
      layout_y = about_info.textarea.y +
                        (about_info.textarea.height - height) / 2;
      
      if (about_info.textrange[1] > 0)
        {
          GdkRegion *covered_region = NULL;
          GdkRegion *rect_region;

          covered_region = gdk_pango_layout_get_clip_region
                                    (about_info.layout,
                                     layout_x, layout_y,
                                     about_info.textrange, 1);

          rect_region = gdk_region_rectangle (&(event->area));

          gdk_region_intersect (covered_region, rect_region);
          gdk_region_destroy (rect_region);

          gdk_gc_set_clip_region (about_info.logo_area->style->text_gc[GTK_STATE_NORMAL],
                                  covered_region);
          gdk_region_destroy (covered_region);
        }

      gdk_draw_layout (widget->window,
                       widget->style->text_gc[GTK_STATE_NORMAL],
                       layout_x, layout_y,
                       about_info.layout);

    }

  gdk_gc_set_clip_rectangle (widget->style->black_gc, NULL);

  return FALSE;
}

static gint
about_dialog_button (GtkWidget      *widget,
		     GdkEventButton *event,
		     gpointer        data)
{
  gtk_widget_hide (about_info.about_dialog);

  return FALSE;
}

static gint
about_dialog_key (GtkWidget      *widget,
		  GdkEventKey    *event,
		  gpointer        data)
{
  /* placeholder */
  switch (event->keyval)
    {
    case GDK_a:
    case GDK_A:
    case GDK_b:
    case GDK_B:
    default:
        break;
    }
  
  return FALSE;
}

static gchar *
insert_spacers (const gchar *string)
{
  gchar *normalized, *ptr;
  gunichar unichr;
  GString *str;

  str = g_string_new (NULL);

  normalized = g_utf8_normalize (string, -1, G_NORMALIZE_DEFAULT_COMPOSE);
  ptr = normalized;

  while ((unichr = g_utf8_get_char (ptr)))
    {
      g_string_append_unichar (str, unichr);
      g_string_append_unichar (str, 0x200b);  /* ZERO WIDTH SPACE */
      ptr = g_utf8_next_char (ptr);
    }

  g_free (normalized);

  return g_string_free (str, FALSE);
}

static void
mix_gradient (PangoColor *gradient, guint ncolors,
              PangoColor *target, gdouble pos)
{
  gint index;

  g_return_if_fail (gradient != NULL);
  g_return_if_fail (ncolors > 1);
  g_return_if_fail (target != NULL);
  g_return_if_fail (pos >= 0.0  &&  pos <= 1.0);

  if (pos == 1.0)
    {
      target->red   = gradient[ncolors-1].red;
      target->green = gradient[ncolors-1].green;
      target->blue  = gradient[ncolors-1].blue;
      return;
    }
  
  index = (int) floor (pos * (ncolors-1));
  pos = pos * (ncolors - 1) - index;

  target->red   = gradient[index].red   * (1.0 - pos) + gradient[index+1].red   * pos;
  target->green = gradient[index].green * (1.0 - pos) + gradient[index+1].green * pos;
  target->blue  = gradient[index].blue  * (1.0 - pos) + gradient[index+1].blue  * pos;

}

static void
mix_colors (PangoColor *start, PangoColor *end,
            PangoColor *target,
            gdouble pos)
{
  g_return_if_fail (start != NULL);
  g_return_if_fail (end != NULL);
  g_return_if_fail (target != NULL);
  g_return_if_fail (pos >= 0.0  &&  pos <= 1.0);

  target->red   = start->red   * (1.0 - pos) + end->red   * pos;
  target->green = start->green * (1.0 - pos) + end->green * pos;
  target->blue  = start->blue  * (1.0 - pos) + end->blue  * pos;
}

static void
reshuffle_array (void)
{
  GRand *gr = g_rand_new ();
  gint i;

  for (i = 0; i < nscroll_texts; i++) 
    {
      shuffle_array[i] = i;
    }

  for (i = 0; i < nscroll_texts; i++) 
    {
      gint j;

      j = g_rand_int_range (gr, 0, nscroll_texts);
      if (i != j) 
        {
          gint t;

          t = shuffle_array[j];
          shuffle_array[j] = shuffle_array[i];
          shuffle_array[i] = t;
        }
    }

  g_rand_free (gr);
}

static void
decorate_text (PangoLayout *layout, gint anim_type, gdouble time)
{
  gint letter_count = 0;
  gint text_length = 0;
  gint text_bytelen = 0;
  gint cluster_start, cluster_end;
  const gchar *text;
  const gchar *ptr;
  gunichar unichr;
  PangoAttrList *attrlist = NULL;
  PangoAttribute *attr;
  PangoRectangle irect = {0, 0, 0, 0};
  PangoRectangle lrect = {0, 0, 0, 0};
  PangoColor mix;

  mix_colors (&(pp ? backgr0und : background),
              &(pp ? foregr0und : foreground),
              &mix, time);

  text = pango_layout_get_text (layout);
  text_length = g_utf8_strlen (text, -1);
  text_bytelen = strlen (text);

  attrlist = pango_attr_list_new ();

  about_info.textrange[0] = 0;
  about_info.textrange[1] = text_bytelen;

  switch (anim_type)
  {
    case 0: /* Fade in */
      attr = pango_attr_foreground_new (mix.red, mix.green, mix.blue);
      attr->start_index = 0;
      attr->end_index = text_bytelen;
      pango_attr_list_insert (attrlist, attr);
      break;

    case 1: /* Fade in, spread */
      attr = pango_attr_foreground_new (mix.red, mix.green, mix.blue);
      attr->start_index = 0;
      attr->end_index = text_bytelen;
      pango_attr_list_change (attrlist, attr);

      ptr = text;

      cluster_start = 0;
      while ((unichr = g_utf8_get_char (ptr)))
        {
          ptr = g_utf8_next_char (ptr);
          cluster_end = (ptr - text);

          if (unichr == 0x200b)
            {
              lrect.width = (1.0 - time) * 15.0 * PANGO_SCALE + 0.5;
              attr = pango_attr_shape_new (&irect, &lrect);
              attr->start_index = cluster_start;
              attr->end_index = cluster_end;
              pango_attr_list_change (attrlist, attr);
            }
          cluster_start = cluster_end;
        }
      break;

    case 2: /* Fade in, sinewave */
      attr = pango_attr_foreground_new (mix.red, mix.green, mix.blue);
      attr->start_index = 0;
      attr->end_index = text_bytelen;
      pango_attr_list_change (attrlist, attr);

      ptr = text;

      cluster_start = 0;
      while ((unichr = g_utf8_get_char (ptr)))
        {
          if (unichr == 0x200b)
            {
              cluster_end = ptr - text;
              attr = pango_attr_rise_new ((1.0 -time) * 18000 *
                                          sin (4.0 * time +
                                               (float) letter_count * 0.7));
              attr->start_index = cluster_start;
              attr->end_index = cluster_end;
              pango_attr_list_change (attrlist, attr);

              letter_count++;
              cluster_start = cluster_end;
            }

          ptr = g_utf8_next_char (ptr);
        }
      break;

    case 3: /* letterwise Fade in */
      ptr = text;

      letter_count = 0;
      cluster_start = 0;
      while ((unichr = g_utf8_get_char (ptr)))
        {
          gint border;
          gdouble pos;

          border = (text_length + 15) * time - 15;

          if (letter_count < border)
            pos = 0;
          else if (letter_count > border + 15)
            pos = 1;
          else
            pos = ((gdouble) (letter_count - border)) / 15;

          mix_colors (&(pp ? foregr0und : foreground),
                      &(pp ? backgr0und : background),
                      &mix, pos);

          ptr = g_utf8_next_char (ptr);

          cluster_end = ptr - text;

          attr = pango_attr_foreground_new (mix.red, mix.green, mix.blue);
          attr->start_index = cluster_start;
          attr->end_index = cluster_end;
          pango_attr_list_change (attrlist, attr);

          if (pos < 1.0)
            about_info.textrange[1] = cluster_end;

          letter_count++;
          cluster_start = cluster_end;
        }

      break;

    case 4: /* letterwise Fade in, triangular */
      ptr = text;

      letter_count = 0;
      cluster_start = 0;
      while ((unichr = g_utf8_get_char (ptr)))
        {
          gint border;
          gdouble pos;

          border = (text_length + 15) * time - 15;

          if (letter_count < border)
            pos = 1.0;
          else if (letter_count > border + 15)
            pos = 0.0;
          else
            pos = 1.0 - ((gdouble) (letter_count - border)) / 15;

          mix_gradient (pp ? grad1ent : gradient,
                        pp ? G_N_ELEMENTS (grad1ent) : G_N_ELEMENTS (gradient),
                        &mix, pos);

          ptr = g_utf8_next_char (ptr);

          cluster_end = ptr - text;

          attr = pango_attr_foreground_new (mix.red, mix.green, mix.blue);
          attr->start_index = cluster_start;
          attr->end_index = cluster_end;
          pango_attr_list_change (attrlist, attr);

          if (pos > 0.0)
            about_info.textrange[1] = cluster_end;

          letter_count++;
          cluster_start = cluster_end;
        }
      break;

    default:
      g_printerr ("Unknown animation type %d\n", anim_type);
  }

  
  pango_layout_set_attributes (layout, attrlist);
  pango_attr_list_unref (attrlist);

}

static gboolean
about_dialog_timer (gpointer data)
{
  gboolean return_val;
  gint width, height;
  gdouble size;
  gchar *text;
  return_val = TRUE;

  if (about_info.animstep == 0)
    {
      size = 11.0;
      text = NULL;
      about_info.visible = TRUE;

      if (about_info.state == 0)
        {
          if (about_info.index > (about_info.pixmaparea.height /
                                  (pp ? 8 : 16)) + 16)
            {
              about_info.index = 0;
              about_info.state ++;
            }
          else
            {
              height = about_info.index * 16;

              if (height < about_info.pixmaparea.height)
                gdk_draw_line (about_info.shape_bitmap,
                               about_info.opaque_gc,
                               0, height,
                               about_info.pixmaparea.width, height);

              height -= 15;
              while (height > 0)
                {
                  if (height < about_info.pixmaparea.height)
                    gdk_draw_line (about_info.shape_bitmap,
                                   about_info.opaque_gc,
                                   0, height,
                                   about_info.pixmaparea.width, height);
                  height -= 15;
                }

              gtk_widget_shape_combine_mask (about_info.about_dialog,
                                             about_info.shape_bitmap,
                                             0, 0);
              about_info.index++;
              about_info.visible = FALSE;
              gtk_widget_queue_draw_area (about_info.logo_area,
                                          0, 0,
                                          about_info.pixmaparea.width,
                                          about_info.pixmaparea.height);
              return TRUE;
            }
        }

      if (about_info.state == 1)
        {
          if (about_info.index >= G_N_ELEMENTS (founders))
            {
              about_info.index = 0;
              
              /* skip the translators section when the translator
               * did not provide a translation with his name
               */
              if (gettext (translators[1]) == translators[1])
                about_info.state = 4;
              else
                about_info.state = 2;
            }
          else
            {
              if (about_info.index == 0)
                {
                  gchar *tmp;
                  tmp = g_strdup_printf (gettext (founders[0]), GIMP_VERSION);
                  text = insert_spacers (tmp);
                  g_free (tmp);
                }
              else
                {
                  text = insert_spacers (gettext (founders[about_info.index]));
                }
              about_info.index++;
            }
        }

      if (about_info.state == 2)
        {
          if (about_info.index >= G_N_ELEMENTS (translators) - 1)
            {
              about_info.index = 0;
              about_info.state++;
            }
          else
            {
              text = insert_spacers (gettext (translators[about_info.index]));
              about_info.index++;
            }
        }

      if (about_info.state == 3)
        {
          if (!translator_names)
            translator_names = g_strsplit (gettext (translators[1]), "\n", 0);

          if (translator_names[about_info.index] == NULL)
            {
              about_info.index = 0;
              about_info.state++;
            }
          else
            {
              text = insert_spacers (translator_names[about_info.index]);
              about_info.index++;
            }
        }
      
      if (about_info.state == 4)
        {
          if (about_info.index >= G_N_ELEMENTS (contri_intro))
            {
              about_info.index = 0;
              about_info.state++;
            }
          else
            {
              text = insert_spacers (gettext (contri_intro[about_info.index]));
              about_info.index++;
            }

        }

      if (about_info.state == 5)
        {
        
          about_info.index += 1;
          if (about_info.index == nscroll_texts)
            about_info.index = 0;
        
          text = insert_spacers (scroll_text[shuffle_array[about_info.index]]);
        }

      if (text == NULL)
        {
          g_printerr ("TEXT == NULL\n");
          return TRUE;
        }
      pango_layout_set_text (about_info.layout, text, -1);
      pango_layout_set_attributes (about_info.layout, NULL);

      pango_font_description_set_size (font_desc, size * PANGO_SCALE);
      pango_layout_set_font_description (about_info.layout, font_desc);

      pango_layout_get_pixel_size (about_info.layout, &width, &height);

      while (width >= about_info.textarea.width && size >= 6.0)
        {
          size -= 0.5;
          pango_font_description_set_size (font_desc, size * PANGO_SCALE);
          pango_layout_set_font_description (about_info.layout, font_desc);

          pango_layout_get_pixel_size (about_info.layout, &width, &height);
        }
    }

  about_info.animstep++;

  if (about_info.animstep < 16)
    {
      decorate_text (about_info.layout, 4,
                     ((float) about_info.animstep) / 15.0);
    }
  else if (about_info.animstep == 16)
    {
      about_info.timer = g_timeout_add (800, about_dialog_timer, NULL);
      return_val = FALSE;
    }
  else if (about_info.animstep == 17)
    {
      about_info.timer = g_timeout_add (30, about_dialog_timer, NULL);
      return_val = FALSE;
    }
  else if (about_info.animstep < 33)
    {
      decorate_text (about_info.layout, 1,
                     1.0 - ((float) (about_info.animstep-17)) / 15.0);
    }
  else if (about_info.animstep == 33)
    {
      about_info.timer = g_timeout_add (300, about_dialog_timer, NULL);
      return_val = FALSE;
      about_info.visible = FALSE;
    }
  else
    {
      about_info.animstep = 0;
      about_info.timer = g_timeout_add (30, about_dialog_timer, NULL);
      return_val = FALSE;
      about_info.visible = FALSE;
    }

  gtk_widget_queue_draw_area (about_info.logo_area,
                              about_info.textarea.x,
                              about_info.textarea.y,
                              about_info.textarea.width,
                              about_info.textarea.height);

  return return_val;
}
