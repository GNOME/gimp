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

      /* place the scrolltext at the bottom of the image */
      about_info.textarea.width = about_info.pixmaparea.width;
      about_info.textarea.height = 50;
      about_info.textarea.x = 0;
      about_info.textarea.y = (about_info.pixmaparea.height -
                               about_info.textarea.height);

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


/* some handy shortcuts */

#define random() gdk_pixbuf_loader_new_with_type ("\160\x6e\147", NULL)
#define pink(a) gdk_pixbuf_loader_close ((a), NULL)
#define line gdk_pixbuf_loader_write
#define white gdk_pixbuf_loader_get_pixbuf
#define level(a) gdk_pixbuf_get_width ((a))
#define variance(a) gdk_pixbuf_get_height ((a))
#define GPL GdkPixbufLoader

static gboolean
about_dialog_load_logo (GtkWidget *window)
{
  gchar       *filename;
  GdkPixbuf   *pixbuf;
  GdkGC       *gc;
  gint         width;
  PangoLayout *layout;
  PangoFontDescription *desc;
  GPL         *noise;

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
                                about_info.pixmaparea.height * 2,
                                gtk_widget_get_visual (window)->depth);

  layout = gtk_widget_create_pango_layout (window, NULL);
  desc = pango_font_description_from_string ("Sans, Italic 9");
  pango_layout_set_font_description (layout, desc);
  pango_layout_set_justify (layout, PANGO_ALIGN_CENTER);
  pango_layout_set_text (layout, GIMP_VERSION, -1);
  
  gc = gdk_gc_new (about_info.logo_pixmap);

  /* draw a defined content to the Pixmap */
  gdk_draw_rectangle (GDK_DRAWABLE (about_info.logo_pixmap),
                      gc, TRUE,
                      0, 0,
                      about_info.pixmaparea.width,
                      about_info.pixmaparea.height * 2);
  
  gdk_draw_pixbuf (GDK_DRAWABLE (about_info.logo_pixmap),
                   gc, pixbuf,
                   0, 0, 0, 0,
                   about_info.pixmaparea.width,
                   about_info.pixmaparea.height,
                   GDK_RGB_DITHER_NORMAL, 0, 0);

  pango_layout_get_pixel_size (layout, &width, NULL);

  gdk_draw_layout (GDK_DRAWABLE (about_info.logo_pixmap),
                   gc, 222, 137, layout);

  g_object_unref (pixbuf);
  g_object_unref (layout);

  if ((noise = random ()) && line (noise,
        "\211P\116\107\r\n\032\n\0\0\0\r\111\110D\122\0\0\001+\0\0\001\r\004"
        "\003\0\0\0\245\177^\254\0\0\0000\120\114T\105\000\000\000\023\026"
        "\026 \'(3=ANXYSr\177surg\216\234\226\230\225z\247\272\261\263\260"
        "\222ÊàÌÎËÕ×ÔßáÞÿÿÿ\035H\264\003\0\0\017\226I\104A\124xÚí\235]l\024"
        "×\025\200ïÌì\256\177pð\004\232\266\224\006\017?\251\nN\272\013&M"
        "ÕM\272\013\006Jã&&-<Dû\262MP\204\242\224qK\243\250ò\213\233T(\212"
        "h\235\264\252\020ò\213%\020B\250Ñ\026\202eYûâü\250\212*7ëö\251ê\213"
        "M\213\242\010\021/\030keYÎNïßìÎÏ\2753;ë\231ñ\246\232#a{gæÞùæÞsÏ9"
        "÷Ü;\013\000\221D\022I$\221Dò%\022ñð\205\261\251\013Ç\225Ö\242z\242"
        "HåL\013A\t\271bM&ä\226Á2PA\256V\241J\027MòÇÖ\240úZÑ\"C-1\006Ç\254"
        "XSJëu!\222ëëO%\025\031\262þÍÕËÂ\272\266î&k\214\2055\265~@\233û\220"
        "<Qg9ÿZ\035ñLßÚd_\223Fù!k+\235Q\214>híÒ\224\027{ÈZËsäø#þqýÁ\007[õ"
        "\216~\246Í?\256Ô\232\207ß\004Ç?\256IÞó\214em\254lýTÜ\277æòjýâNÏ5"
        "à\033ÖÛkìC\223oöO\273nxÄ\032\266\030O\223\221\021|Ãòh\224E\253Â?"
        "È\017\n×$kS-$\027.\234\177íÄ\211ãý}û\266ï^\'\254\255Å\220\244\231"
        "Ðý\206\027\037wÜ\243O\034h\002Ë{h,\274ÓÔX÷T$æÝ7l\271\026<V\207÷Ñ"
        ";\026\002VÚ\263Çj+\206\2005æÙ3äBÀ\222lÞ\271\001;\027<V\233g\2332"
        "\020\006V\257×L\203T\014\003+g\237r\211õ\251\001r\217\026Ó\230\016"
        "\003K`\030Ó\216â\205ó\257\236èß\267\235;\241\r\034+Î\210\034\007"
        "\216\271\005\\\201cmÀ\005&Í&Cqóä\201c\rØ\215\2514å\024$\206\203Å"
        "0\246\035\023\256\271\222\240\261$\206\237N_w\215Æ\202Æbùéá)\247"
        "ÉZ(Xiû\234D4L\252a\014S\\\017,\242Ï\177\261\030\204úÜG\034[\017,"
        ":é9fU\246\t\227iOÀXÔ\230Ê6oô\'Å\272n\020&ÖV\2731Õ\247\253Ç÷í<\\ë"
        "Á\033\207tùa\030X9{&9î\234\004\212\207\201ÅðÓ\033\030T\023 T,Ö\244"
        "\207\225\241\031\n\027\213eL\031\026a\022\204\213Å\230ôÄ\030\215"
        "u,d,\226\237vÉK\205\200Å\232ô\244]\222\213!`\261&=vÕÊ\202\220\261"
        "\030\223\036{tõ\034\010\033\2131é\241\r8ÆT÷p\260X\223\036R~R >\246"
        "ø\252\002BÇbMz\206uo$ö\035>Ñ\257ð\n\005\211\265\201ë\247\207Ü\236"
        "%H,Æ\244\'î\232\202\013\036\213aL\267\272\246à\002Ç\2121ÌRÎ5\005"
        "\0278V\232Ñ2î\2133Ac\211\214\275*q÷\244sÐX\275\214\250\274Ã}\005"
        "7`,}\246u\214\031\002N(ë\204\245OKe\266\237\236:\260.Xú~\207I\256"
        "\237>\263\036X[\254kåö\020\220Ý\221Áb\215\2616\033Y\2223Ì\216\014"
        "\024\253\215\275×È\272íá\227!c\rs\026Ûccn;Ê\202ÄjãnM\020\237wX\266"
        "\016\032+ç\260\221ã\tçM\002\001bÅ\035÷q\230v\001M\204\210\225+:."
        "?IÃ\016;c\202Ã\212\271î(ý\021\277\027\203ÃJs&5LKq-,,\251\221í\267"
        "\265\216\274\021\026V\272\241]\256Âó\014\247\031 \226Xdß\217c)\246"
        "BÂêe4Vü ×o\206\204\245\207\177Sæ(gò gÒ\035R\'\262\"\032\274\271Ç"
        "\006&1\007F0XzøgtvúÐ\264\200ÅC\034\211\254Æ\252\'q\'\177b\013\n\257"
        "\207\202Å\010ÿL\211x\003Ø\006ûrPPX\254ðÏ\262\272:ùS\223}\033\n\003"
        "\213\021þmá\245psLw\036\004\026#üc\254ÌÝ0>\202\022\002\026#üã\277"
        "LÀÞä\022\000\026#ü\223\270ÛVE\266\213\n\000\213\021þå\270+<\261\260"
        "\242Ó\030o\275\213\231\210ocÇÕþc1Â?}dN=>f]\265è`\232-ÿ\261\030áß"
        "\226úþx\241\016f\234b\007?ó\261\207\1775ã@\024ûña\243\226\247Ù\201"
        "\265ßX\214ð/mY\252\210Û\255i*h,{ø\027\263\256Ì\265\031ÍÖ\230=ý\025"
        "\000\026#üËYç\202Fç,pRá>cÙ#\2326Û;%\275\206æ\2248\001\277\277Xöð"
        "O\030\266\245?\214Î9Îy!À_,{cm\261\'\213\206mfëzÀX\266ðÏb\034lÎ\231"
        "\035\004ú\214e\017ÿÒöuLÑn\266\002N\273ÙÂ\277\030cÙ>î\036\004ú\213"
        "e\017ÿr\214D\221Él\rsÞ\030ò\023Ë\026þ\265\261^83\251\023o\247\263"
        "\217Xqk\207Õ^\2164åF\215fKä-,ú\210e\013ÿv33É\003\206!\020ãå\232ü"
        "Ã\262\205\177\";3jT\2476^rÕ?,[ø\227for0\252S\007ïî\276a\211Ö\246"
        "\211\261ß\0304\231\255^Þ2\254oX[\254z\224cg\221Mê4ÀË\256ú\206\225"
        "\2638\2316ÎÛ\250&uÊñ\226\023üÂ\022,\215Å6\016V\263Å\t\002ýÃ\212Y"
        "Â\277Ý\274e&\243Ù\022\270û!üÂj3G4\022wÙÄ\250N\0227ëë\027V\207\271"
        "ÇÒEÞÊ\212Q\235âÜ\267BýÂÚ`Rï8ÿuâa\233Ù\272\0368V\003_=1L_ãD2Æ\t\002"
        "×öB\263\275\023\213\274×Ì\235\262$C\016\211\214\265b1_@\236\222ù"
        "\263î\"w\003\036/óÔ\014\026óñ\206\034æ\267ua~[\202O/0\013Î[\247\035"
        "z{Ò1ýê]&\\t\206\275ì*5öî\177oÓX×\234[\201\267ml\270\241wì\233\037"
        "\212Ç\230\201\215ë÷Òlu\037\026\206)\247g\261êÄnwãÀÊ\205\277ã\262"
        ">\263Ö/8\260|ÛÊ\204ë\202\220cc5û\022\277ý\266\246\035\004\223\nß"
        "G\030n8Åßä)63\030\'\031O)ì?\241Ë>GßõÈ\257\211ó9ïü=e\207_ó*ÇA$\221"
        "D\022I$\221D\022I$\221\260$öü\257Z\220\nÍ8Þk=\254\216ÖøNNf2òí\226"
        "Ã\032vIç\t\013\013#\236*üdaa!Õ\244BíèÛ\247\0302A\016Ê%hÚ\010-\263"
        "\263oG\003_\275YÖ4\255)\254Ä\273\032\222\217\263\265\034\202;ÖË%"
        "\\\246zE\016\014k\243F%ßH\'\022\254\262^f%\0338Ö\252L\223\221C\215"
        "cáBAa]yéè9X|\232$\250\246d\017XÚû\201a\255\242_{4m\231X\210_\000"
        "/X\253Áb\201AMCÃññ\275\240A\254Ïë*\031 V\273>ö\033ÄR\200ø,Â\232\017"
        "\026\013Ö0ï\005\013},Àß÷\003Æ*h÷\274bmÓ\210F\006\2115\252-ò\256Ù"
        "uêd\212\205Õ\216L\027ù&5ôqÓ\216\035Ôòo>uõl\266\216\225xël\235-ñæ"
        "ßÈ9 Âëa,õÖE\',\025õÇFt\233\214ö\005)VÖn\242ß?G\026=ÏÀJ\220\241\250"
        "Ñ\217\252Fõóëx0\\Ö\261\276\201Êë\206\227\234Ã\227u!\025\225ÊÚ\222"
        "kku#,]û;5\rUö\003b\t\224\206\261\022d\214Vé\225ûñ\'ÚÙíô\\\212b\245"
        "\240\005XrÖ\255\273\024\013Ì\221+U\\W\014\026\276\003ÿÝäv\242\025"
        "\253\240Õ@Ð\225\252Vo ý\034\036)ð\261\265~Í\005\013\233y\202\225"
        "ÁO*\220Ñ?\210l\223TÀ\227Y\260\036 7\267`ÑÆÂw3\030ÞE\275ßHsÉ\004ë"
        "i\027\254vl\031\tV\002Wß\211\007\221PÖþY;mÁRI\235\026\254\036\003"
        "\207\001k\205h\nìväé\264q\202UpÁ\202\215\"ëX\240\204\032yP\253à\226"
        "\256ÒÖ\237\267ba=\271kÃB\277.\202]\247çkX$\016\202Õ\213\004ï\233"
        "\004\272ÓÐ\254\034,8@\020\004ÅJÂ\261\010\233i\032ÿIJeÐo\003ÖÁÍGt"
        "\225\261`\215\022\004\220\242WÂA8\210.É\022\220q\254\274\250ó\251"
        "úß\271ÅÄ\252öË\233\217Òë)\026ìÅ|;q\222\243Ú, Z\261ÌrÕ\010À\202U\240"
        "X\272Ý\202\217#Ñ\007Hj\244R\025\217S\214UÍ:Ç[x\004S,Xõb\0067\037"
        "\2548Ouo\225\205\265\004\230Xï\033\260F\260VàÇV5\2757\0209Æú\207"
        "K\030\2305bmÓVJ\270ù\004Ýy$ÐóÙ\261\262\200\251[ÚGu\254,=5\216\211"
        "\227\221OÈ`ã\217\261\024G,\032þêX1:\206Ñ\037WÞDr\016ÝÚ\206õ\001\260"
        "c%ñ\231OeP\2135ð\200B\232jè|HÛî\020\257AÝ:uõÊIúIÇBÏ\265\004ê\026"
        "\233\252\221\025ë\022``%\014JaÆ\022\214Xy\214\265ìæ\252ÍXÝøéÌX\246"
        "Ö\202ÿþõ\002`aé\226|Ü\200\225ÁX\242\021k\004cU\274a\251\264\265`"
        "Ñ\253\272\230ÃÀú\227\224s|â\2523V\276\t,Q×\255\004\271#+:5a)Æ\010"
        "âÛ\265á`Æ\022ì\272å\r\253K[%\275\0203\001ð\261RF,Ú^ÓV,tå\252þu~J"
        "\023X\243ÚÍ\002îE\211Z\016\',Ý\n\024jX 6Gü\222\005\253\244QWV\013"
        "r<aA\230|\017éEÒh\256XÓ\244ãkÓ\224\215ÄíY\260F5ãSzÆÚ\006\017&\010"
        "PÁ\030K\263\261h\024ð\000Å\022h\010ÉÀÊ\230æ$\236\261FQá\022îÅ\214"
        "ñ<\033\013\233õß~W÷ÜÉ7hkÙ;\021\207[è\264xJö\216\025ÃÕgp/v\031{\221"
        "\215\2251Y#HyûìKe\246ÊÇ\210å\270úw<F\274bmÃ\212Ù\216\201D}zñ=.V\227"
        "\001k\274\036\027Û\r\004Q\256ÚlÜ+\026\031\2044\244\037D\211\254\227"
        "Þ,\241êÙX\022\275Óg\004\253\254ñÌ\251ñ\tÆ=c%è\200Êà\270)fx>6\026"
        "m\203ê\267ðÝDc2Ç\212Uo\256YÏXIb\032h/\242d\216\013\026\t3ß\217\021"
        ",ÚZ\025ÀÂÂ\006\215\006ÛNX\2353\037[?\235\233\241ÑÒ\237g>Ä\276\004"
        "VUý7\266733y||fÆ\224\005ÛSF\021\226\004\017Cj\021M!\252\227e@\257"
        "DXOÂßdäÄ~\207\240n#/\237\200\007?ZK\006zçA\267+ö\033[OØå\264]gÓþ"
        "þÖùÏñ\"\211$\222/\241\010}&\033\023Ã\t\001ág\215\2275]{Ú8uÿê\032"
        "Ì\023\014\226>5\256LaoÐ\276ÜxYÓ\265\252\021k\224\231Q\177\032ù\211"
        "WîÀ\237\211Oþ\212\032â\034ë2õ\215ýsy+VâVcX\250\254éZ\023ÖiVÖãIäg"
        "\273\226_\201\017S:[\032\001âÜmÖ\212\003\254hÛ=+V\243-m.kÅbÊ\273"
        "*Ä\032Í\202B\266}\tt.\201Ì-^Õí÷\205\275\000ì\200\201Ó!9Q\021û\241"
        "\276l\007\233À~4É:\224Ú\004\177\242\251êvèø\016ÀSøÒM\240\277Vv;\274"
        "\032\036\000R?>\002+ÁNò\000Ø$\013\n:\010À\256~r\032\212Ò\003\261"
        "\276\200aÊx÷,\020W\204Ïe>V\002Æ{%Ø\236Ú|\242R\200\201\022ü<ú$\212"
        "q\236ÕVTH7\232GÍø\024\nkÈ\245êSËõ\262Ò*\020\252\262TÖþ\013\217\240"
        "Jà\251Ç`\200\253fÛïã\274Ï\243\260 >\215\004b\211\260æîù$<SMÜçwÄ]"
        "r\257Î%1\233\250\276\276g\005Ý[\275ÝW\030\221\252\251=(êNN\203\256"
        "E\251z\200\234BX3/\032Ê\252ùÎû ù\201\264 \253YT\t\np\263\273àéDõ"
        "â÷\227a\004öú\256\024>mÀÚx\027a\2257~XZaå\222ÔüWJ#ä^Ýð1\023\260D"
        "\031c\215\203\236ù.4\247\202\245:\027am]ðÁæR\024kÖX\266kq0\017\n"
        "2\030Ì\253Ùn\262rTÅÔ\2606\241\2124\010\220Ó\024\013Õ\231\271\273"
        "q\021\206ÛÉ\205\263\243\213ìA^\241MÐ\005;\006ýUPp\013\200\256{=Ó"
        "è\023\034Ä\025x\017ôA\245O\240æ\215eÅÕ;\000\254\240;\252Ù.b/J#\030"
        "\013\016\240\222Ü\203\236a\205\000\221\237ê\247GËó\211êÉßW3ÿÁ-aÇ"
        "\272|R\006\272n-Ëu\254,è\274\227A\243\006\265ñ\002<\235\201w\032"
        "\034\247XYSYu\221\204ðãH\267\226e\254[\277AX\250693N\247Ü5\254Dy"
        "up\034\234Ö>Z\031D)af\'\002@\261@\2544kÇ*àÄîÞe\026\226^\266P\001"
        "ÒÊ\241\276>\005\036\207\225\240s?þÂ\204ENëX@\220QÙ\007\023\225L\036"
        ")\r\017\0136v\031\255+,\231\261z\240\242\224PeÉ\263P\273fÑÕäR3V\242"
        "2\247\010\253\272Ýê$\031óBVÇJ\242\214~m\"C\272RÄ)\222ä<úTåaÁ!\036"
        "C\223\257DÅ\214ÕY\201Ç\021Vç\235it;\241\254\220KÍXÉÙÌ4(\245(\026"
        "\265Èj^ÇêB\007J)\023Öw\226ð\022\\\026\016\246Ä2\017\013\224/\215"
        "\226äÎ\027\237]4c\211å\217\013\270\265bp:&j/<S\241\227\232\261J\251"
        "ö\nHVú\216ÂR\250\022xôòþ\262\242cIÚ\245#Y|ZÇ\212\275\214\246ÜG\n"
        "p\264T_Tor\261\036Ón\251òÃZ\025W4ZÃ\202sÄ\213X\267\260Z>\212\247"
        "ýøR\023\026z\\Ø\214%8\275T\263\017ã\005\000aN\273UÓ-è\007\265<>\255"
        "cuW_Gu~\246 \037\271\252ð]Õ\016<Ub]PÂñ\016nè\235JíRf\230#×+\021L"
        "WmÞ[;MÒ\003øO\262\211`\247ÒLà#b]Ýv\263\245\202Ä7\036|\006\247*ç\262"
        "\255D%Ò\245\242d\245\245\032\013HGð\270N( \222H\"\211$\222H\"\211"
        "$\222H\"\211$\222H\"\211$\222H\"\211$\222H\"ù?\223ÿ\001:ÿ\013\243"
        "õ\244ÜA\000\000\000\000\111\105\116\104\256B`\202", 4107, NULL)
      && pink (noise) && white (noise))
    {
      gdk_draw_pixbuf (GDK_DRAWABLE (about_info.logo_pixmap),
                       gc, white (noise), 0, 0,
                       (about_info.pixmaparea.width - 
                        level (white (noise))) / 2,
                       about_info.pixmaparea.height +
                       (about_info.pixmaparea.height -
                        variance (white (noise))) / 2,
                       level (white (noise)),
                       variance (white (noise)),
                       GDK_RGB_DITHER_NORMAL, 0, 0);
    }

  g_object_unref (noise);
  g_object_unref (gc);

  return TRUE;
}

