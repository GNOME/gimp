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

#include <string.h>
#include <time.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "dialogs-types.h"

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
  /* we'd prefer just the names, please no email addresses.    */
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
  { 139 * 257, 137 * 257, 124 * 257 },
  { 65535, 65535, 65535 },
  { 5000, 5000, 5000 },
};

PangoColor foreground = { 5000, 5000, 5000 };
PangoColor background = { 139 * 257, 137 * 257, 124 * 257 };

/* backup values */

PangoColor grad1ent[] =
{
  { 0xff * 257, 0xba * 257, 0x00 * 257 },
  { 37522, 51914, 57568 },
};

PangoColor foregr0und = { 37522, 51914, 57568 };
PangoColor backgr0und = { 0, 0, 0 };

static GimpAboutInfo about_info = { 0, };
static gboolean pp = FALSE;

static gboolean  about_dialog_load_logo   (GtkWidget         *window);
static void      about_dialog_destroy     (GtkObject         *object,
                                           gpointer           data);
static void      about_dialog_unmap       (GtkWidget         *widget,
                                           gpointer           data);
static void      about_dialog_center      (GtkWindow         *window);
static gboolean  about_dialog_logo_expose (GtkWidget         *widget,
                                           GdkEventExpose    *event,
                                           gpointer           data);
static gint      about_dialog_button      (GtkWidget         *widget,
                                           GdkEventButton    *event,
                                           gpointer           data);
static gint      about_dialog_key         (GtkWidget         *widget,
                                           GdkEventKey       *event,
                                           gpointer           data);
static void      reshuffle_array          (void);
static gboolean  about_dialog_timer       (gpointer           data);


static PangoFontDescription  *font_desc     = NULL;
static const gchar          **scroll_text   = authors;
static gint                   nscroll_texts = G_N_ELEMENTS (authors);
static gint                   shuffle_array[G_N_ELEMENTS (authors)];


GtkWidget *
about_dialog_create (void)
{
  if (! about_info.about_dialog)
    {
      GtkWidget   *widget;
      GdkGCValues  shape_gcv;

      about_info.visible = FALSE;
      about_info.state = 0;
      about_info.animstep = -1;

      widget = g_object_new (GTK_TYPE_WINDOW,
                             "type",            GTK_WINDOW_POPUP,
                             "title",           _("About The GIMP"),
                             "window_position", GTK_WIN_POS_CENTER,
                             "resizable",       FALSE,
                             NULL);

      about_info.about_dialog = widget;

      gtk_window_set_role (GTK_WINDOW (widget), "about-dialog");

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

      about_dialog_center (GTK_WINDOW (widget));

      /* place the scrolltext at the bottom of the image */
      about_info.textarea.width = about_info.pixmaparea.width;
      about_info.textarea.height = 32; /* gets changed in _expose () as well */
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
      about_info.state = 0;
      about_info.index = 0;

      reshuffle_array ();
      pango_layout_set_text (about_info.layout, "", -1);
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

static void
about_dialog_center (GtkWindow *window)
{
  GdkDisplay   *display = gtk_widget_get_display (GTK_WIDGET (window));
  GdkScreen    *screen;
  GdkRectangle  rect;
  gint          monitor;
  gint          x, y;

  gdk_display_get_pointer (display, &screen, &x, &y, NULL);

  monitor = gdk_screen_get_monitor_at_point (screen, x, y);
  gdk_screen_get_monitor_geometry (screen, monitor, &rect);

  gtk_window_set_screen (window, screen);
  gtk_window_move (window,
                   rect.x + (rect.width  - about_info.pixmaparea.width)  / 2,
                   rect.y + (rect.height - about_info.pixmaparea.height) / 2);
}

static gboolean
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

      about_info.textarea.height = pp ? 50 : 32;
      about_info.textarea.y = (about_info.pixmaparea.height -
                               about_info.textarea.height);
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

  mix_colors ((pp ? &backgr0und : &background),
              (pp ? &foregr0und : &foreground),
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

          mix_colors ((pp ? &foregr0und : &foreground),
                      (pp ? &backgr0und : &background),
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
  PangoLayout *layout;
  PangoFontDescription *desc;
  GPL         *noise;

  if (about_info.logo_pixmap)
    return TRUE;

  filename = g_build_filename (gimp_data_directory (), "images",
                               "gimp-logo.png", NULL);

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

  gdk_draw_layout (GDK_DRAWABLE (about_info.logo_pixmap),
                   gc, 8, 39, layout);

  g_object_unref (pixbuf);
  g_object_unref (layout);

  if ((noise = random ()) && line (noise,
        "\211\120\116\107\r\n\032\n\0\0\0\r\111\110D\122\0\0\001+\0\0\001"
        "\r\004\003\0\0\0\245\177^\254\0\0\0000\120\114TE\0\0\0\023\026\026"
        " \'(2=ANXYSr\177surg\216\234\226\230\225z\247\272\261\263\257\222Ê"
        "àÌÎËÕ×ÔßáÞÿÿÿ;\014æ\016\000\000\017\210I\104A\124xÚí\235[l\033Ç\025"
        "@gw)êaÆbì\246\255ëÆ\242\037)j+\251hII\2122)iËvÝ\270\211\234Öþ\010"
        "øÃ&F`\004\251\251Ö\r\202B?jR\030A\220\226I\213Â0ô\243Â\201a\030\t"
        "X;\026\004\201?Ê\003E\020\250\241Ú\257\242?\222Q#\010\002W\264e\201"
        "\020\004\205Ûy-\271\217;\273\\\212\273b\212\275\200%qç\261ggîÜ\271"
        "sg\226F(\220@\002\t$\220@\276B\"\037\2760>}áD\254\265\250\036-p9"
        "ÛBPR\272P\225ÉhË`é\2500W\253P%\n\006ùSkP}\243`\222\221\226\030\203"
        "ãf\254éXëu!\221k\033O\245\024\000Ùøæê\205\260\256n\270É\032\207\260"
        "\2467\016hë\000\221Gk,ç_\256!\236\035X\237ô7h\224ï3\267ÒÙ\230~\016"
        "Z\27744\213Ýg\256åivý\201æqý\261\t\266ê--\245\275y\\ñu\017\277IÁ"
        "ü\270.yÏ5Ö\270øÁÚ\232×\\n\255_\233Ýs\035k\032Ö\233ëìCÃÜÜ\276a\275"
        "8j2\236\006##5\rË\245Q\226Í\n\177\257Ø)\\\227\254O\265\210\\\270"
        "pþå\223\'O\014\rôïÜ\273AXÛ\013>I#\256û{næ\270\023.çÄc\r`\271w\215"
        "\245\267\032\032ë\256\212\204ÜÏ\rÛ\256z\217Õé~ô\216û\200\225pmëÚ"
        "\013>`\215\273\236\031Ò>`ñ5EÊ\225\235ó\036\253Ý\265M9æ\007V\257Û"
        "H\203Rð\003+m]rÉ\265\245\001\231\036M\2461á\007\226\004\030ÓÎÂ\205"
        "ó/\235\034êß)\\Ðz\216Õ\006x\216Ç\216;9\\\236cm\242\005\246\214&#"
        "æ4\223{\216uÌjL\225i;\'Ñ\037,À\230vN:ÆJ\274ÆR\200y:qÍÑ\033ó\032\013"
        "\232\247G\247í\026k\276`\261yú\272É\265×\271SÛ\n\033\201Åôù\257&"
        "\203P[ûÈã\033\201Å\027=ÇÍÊ4é\260ìñ\030\213\033Ó\250e6úsÌ\274oà\'"
        "Öv\2531Õ\226\253\'úw\037\256öàõC\232üÈ\017\254\2645\222Üf\037\004"
        "jó\003\013\230\2477\001T\223ÈW,hÑ\003EhFüÅ\202\214)`\021\246\220"
        "\277XÀ\242\'\0044Öq\237\261\240yÚJu\035ù\213\005-z\022\016ÁE\037"
        "\260\240E\217U\265RÈg,`Ñcõ\256\236F~c\001\213\036Þ\200ã\240\272û"
        "\203\005-zXù)\211Í1\205\227bÈw,hÑ3\252ÍFòÀá\223C1Q!/\2616\tçé\021"
        "\247gñ\022\013Xô\2649\206à\274Ç\002\214évÇ\020\234çX\2201M;\206à"
        "<ÇJ\000-ã\2749ã5\226\014\234Uis\016:{\215Õ\013xå\235Î;\270\036ci"
        "+\255ã\240\0138\031Û ,mY\032\205çéé\003\033\202\245\235w\230\022"
        "ÎÓg7\002k\233y\257Üê\002Â\035é-Ö8tØÈ\024\234\001;ÒS\254vø\254\221"
        "ùØÃ\257|Æ\032\025l\266\207Æ\235N\224y\211Õ.ÜÔ\226\237\261Ù\266ö\032"
        "+ms\220ãQûC\002\036b\265Ù\236\0000\234\002\232ô\021+]\260Ý~RFmNÆ"
        "x\207\025r<Qúcq/z\207\225\020,j@KqÕ/,\245\236ã\267Õ\216\274î\027"
        "V\242\256S\256Ò3À\244é!\226\\\200ï\'\260\024Ó>aõ\002\215ÕvP8oú\204"
        "\245\271\177ÓF/gê\240`ÑíS\'B\036\r=Üc\001SÀ\201á\r\226æþé\';mh\232"
        "ÀÚ|\034\211PcÕ\202\270S?\2658\205×|Á\002Ü?C ^\007\266É\272\035ä\025"
        "\026äþ\231vW\247~f\260o#~`\001îß6Q\0107\rNç^`\001î\037\2603w]ÿ\010"
        "1\037\260\000÷Oü2\001|ÈÅ\003,ÀýS\204ÇVex\212ò\000\013pÿÒÂ\035\236"
        "\220_ÞiH\264ß\005\006âÛa\277\272ùX\200û\247\215Ìé\207ÇÍ\273\026\235"
        "\240Ùj>\026àþm\253\235\217\227j`ú%\266÷+\037\253ûW5\016L\261\037"
        "\036Õky\002v\254\233\215\005\270\177\tÓVE\233Õ\232Æ\275Æ\262\272"
        "\177!óÎ\\\273Þl\215[Ã_\036`\001î_Ú\274\026ÔOÎ\222 \024Þd,\253GÓn"
        "y\247\244W×\234\212Àáo.\226Õý\223F-á\017ýä\014;\201ÍÆ\2626Ö6k\260"
        "hÔb\266\256y\214eqÿLÆÁ29ÃN`\223\261\254î_Â\272\217)[Í\226Ça7\213"
        "û\027\002\266íÛ\234\235ÀæbYÝ\2774\020(2\230\255QÁ\033CÍÄ\262\270"
        "\177íÐ\013g\006u\022\235tn\"\226Åý\253\276\034i\210\215êÍ\226,ÚX"
        "l\"\226ÅýÛ\013F\222\217é\206@H\024kj\036\226Åý\223áÈ\250^\235ÚEÁ"
        "ÕæaYÜ\277\004|ÈA\257N\235\242\2737\rK67M\010~×Ì`\266zEÛ\260MÃÚfÖ"
        "\2434\034E6\250SB\024]m\032VÚ4É\264\013ÞF5\250SZ\264\235Ð,,ÉÔX\260"
        "q0\233-\201\023Ø<\254\220ÉýÛ+ÚfÒ\233-Ix\036\242YXíF\217F\021n\233"
        "èÍ\226\"\214ú6\013\253ÓØc\211\202hgE\257N\"\'\260yX\233\014êÝ&~\021"
        "uÔb\266\256y\216UÇWO\214ò×8\211\214\013\234Àõ\275ÐlíÄ\202è5s\273"
        "(É\210M c\275Xà\013ÈÓQñ\252\273 <\200\'\212<5\202\005Ö3b\263\276"
        "\255\tøm\tMz\201Y\262?:mÓÛS\266áW÷2é\2403ð\266\253Rß[ã\275\rc]\265"
        "o\005Ñ\261\261Ñ\272Þ\261o|(\036\007\035\033Çï\245Ùî<,tKN×bÖ\211\275"
        "ÎÆ\001\212\205\277å\260?\263Þ/80}ÛÊ\244ã\206\220mc5ú\022\277õ\266"
        "\206\023\004S1ñ\034\241\273á\264ø\220\247ÜÈ`\234\002\236R\032<\251"
        "I\277íÜõÀoØäsÞþ{Ê\016\277ìVN\240@\002\t$\220@\002\t$\220@\002\201"
        "$ôÌ\257[\220\212\2548Þk=\254ÎÖøNN0\030ùfËa\215:\204ó\244ÅÅ1W\025"
        "~\272\270\270\030oP\241v\rôÇt\221 \033å\222Tu\214\227Ù=\260\253\216"
        "\257Þ,\251\252Ú\020Vø]\225ÈÇ\251j\014Á\031ë\205\"-S\271\034õ\014"
        "k\263Ê%SO\'2\254\222Vf5å9ÖZ\224\007#GêÇ\242\205\274Â\272üüÑ7pñ\031"
        "\026\240\232\216\272ÀRß÷\014k\215üÚ\247\252+ÌBü\022\271ÁZó\026\013"
        "\r\253*\031\216\017ïGubý\267\246\222\036buhc\277N\254\030\222\237"
        "\"X\013Þbá\032\026Ü`\221\217yüû\256ÇXyõ\216[\254\035*ÓH/\261rê\222"
        "(Ï\236Ó\247â\020V\0071]ì\233ÔÈÇ-\273vqË\277õô\225s\251\032Vøõs5\266"
        "ðk\237\2604$ãüØ\227zý\242\035V\226ôÇfr\233\244ú%+VRo\220ß\277 \026"
        "=\003`\205ÙPTùÇ\254Êõó\233t0\\Ò\260\276EÊk\206\227\245Ñl\021\242"
        "\242JI]vl\255n\202\245i\177\227\252\222Ê~È,A\254n\2540\033\243\025"
        "\236s\220~â\235ÝÁÓâ\034+\216-À\262\275nÝæXh\236åÌÒ\272B\270ð-üï\206"
        "\260\023ÍXy\265\nBrfÕZ\003iit\244àÇV\207T\007,jæ\031V\222>\251ÄF"
        "ÿ0\261MJ\236f3aÝÃnnÂâ\215Eï\2463\274KZ\277\261æ\2122\254\'\034\260"
        ":\250edXaZ}\027\035DRIýg5Ù\204\225eu\232\260zt\034:\254U\246)\270"
        "ÛÉL\247N0\254\274\003\026n\224\250\206\205\212\244\221\207Õ2mé\n"
        "oý\0053\026Õ\223Û\026,òë\"Úsf\241\212Åü \\\275Ìð\276Í\240\273tÍ*"
        "ÀÂ\003\204@p\254><\026q3ÍÐ?Y\251$ù\255Ã:\270õ\210\2462&\254\034C"
        "@q\236\023\017Âa\222%Å@&\250ò\222Îçê\177ë&\210U\031\212n=Êós,Ü\213"
        "\231\0166IæÔ9Ä\264b\005\232\252\t\200\t+Ï\2614\273\205\037Gá\017"
        "Ð\247\262J\263t\234R\254JÊÞß\242#\230cá\252\227\222\264ùpÅ\031\256"
        "{k\020Ö2\002\261Þ×a\215Q\255\240\217\235U\265Þ ä\024ë\037\016n`J"
        "\217\265C]-Òæ\223\264É#L\236Ï\212\225B\240n\251\037Õ\260R<i\202\022"
        "\257\2209!I\215?Å\212Ùbq÷WÃ\nñ1Lþ\270ü\032\2217È\255-X\037 +V\037"
        "Mù,\212\252\276\006\035PDSu\235\217i;lü5\254[\247\257\\>Å?iXä\271"
        "\226QÍbs52c\275\215\000\254\260N)\214X\222\036+C\261V\234\246j#V"
        "7}:#\226\241\265ð\277\177=\213 ,Í\222Oè\260\222\024KÖc\215Q\254\262"
        ";\254,o-\\ô\212&F7\260ö%å\2029qÍ\036+Ó\000\226\254éV\230Ý\021òN\r"
        "X1\275\007ñÝêp0bIVÝr\207\025Q×X/\204\014\000b\254\270\036\213\267"
        "×\214\031\213ä\\Ó\276Î/Ö\000VN\275\221\247\275\250pËa\207\245Y\201"
        "|\025\013\205æÙ\274dÂ*\252|*\253:9\256\2600L\246\207õ\"k4G\254\031"
        "ÖñÕeÊf6í\231\260r\252þ)]cíÀ\027Ã\014(\257÷\245a,î\005ÜÃ\261$îB\002"
        "XIÃ\232Ä5V\216\024.Ò^LêÓa,jÖ\177÷\2106s÷\275Ê[ËÚ\211ÔÝ\"Éòé\250{"
        "\254\020\255>I{1\242ïE\030+i\260F\230ò\213sÏ\227@\225\0171Ëqåït\214"
        "\270ÅÚA\025\263\203\002ÉÚòâûB\254\210\016k\242æ\027[\r\004S\256ê"
        "jÜ-\026\033\204Ü\245\037&\201\254ç_+\222êa,\205ßés\206UREæTÿ\004"
        "\023\256\261Â|@%\251ß\024Ò=\037\214ÅÛ\240ò\035z7Y\037Ì1cÕ\232kÎ5"
        "V\0373\r\274\027I0Ç\001\213\271\231ï\207\030\026o\2552\202\260\250"
        "AãÎ\266\035V×ìÇæOoÌroé\235Ù\017é\\\202\253\252ü\233Ú\233ÙÙ\014\275"
        ">;k\210\202í+\021\017KÁ\2271\265L\226\020\225KQÄs\022\254Çðo6rB\277"
        "\'P_\220Y>\214/~\264\236\010ôî\203N9\006õ\255\'í\261;\256\263ep\250"
        "uþs\274@\002\tä+(Ò\200ÁÆ\204h@@úyýe\ryÏè\227î__\207yÂÎÒgú\235):\033"
        "t\254Ô_Ö\2207\253ÇÊ\201\021õ\'È<ñâ-ü3üéßðÇ+Wþ\002Uýêà|Æ\214\025\276"
        "Y\037\026)kÈkÀ:\003E=\036#óldåEü0ÅsÅ1\224[\\\274\001U\235B;î\230"
        "\261êmicY3\026(ïf1V.\205ò\251\216eÔ\265\214Þ\021VÝqWÚ\217Ð.ì8\035"
        "\212\206Ëò\020Ö\227\235h\013\032$\213\254Cñ-ø\'Y\252îÄ\023ß\001\234"
        "D\263nACÕ\262;qn|\001)Cô\n\256\204N\222\007Ð\226\250\024#\027\021"
        "Ú3Ä\222\261Äz0Ö\227ØM\231è\236Cò*úÜ\006+\214ý\275\"\222çÕ\205p9\217"
        "\035%ü9÷\030ñq\236RW\263\230.\227!Íø8qkXÖìã+\265\262Ê\032\222*Q\245"
        "\244þ\007_!\225à\244\207\260\203\233MuÜ\245q\237\007qA\232L\004c"
        "a\032Ô\275Ð\207S*\250,î\210Ûì^]Ër*\\yeß*\271wö\213\201ü\230R\211"
        "ï#^wß\014\212,)\225\003,\211`Í>\247+\233ÍtÝE}\037(\213Ñl\212TB\034"
        "ÜÔ\036\234\034\256\\üÁ\nöÀ^Ù\023\247É:\254Í\267\tVI^\273\014ö{6ó"
        "\265â\030\273W7~Ì0.Q\242X\023\250g!BÖTØeéZÂ\265Eðjh>Î\261æôe#KÃ\031"
        "\224\217\242áL6ÕÍv\216*\224\032×&U\210\006!\226Ì\261H\235ÉÛ\233\227"
        "\260\273\255|R\254ÄàA^æM\020Á\035CþÊÇh\013\240È\235\236\031ò\t[\263"
        "2\276\007ù\220åO\220ÍèËÊk\267\020Z%wÌ\246\"Ì^\024Ç(\026î\242b\264"
        "\207<Ã*\003b?\263\237\035--\204+\247þ@By\2719\010ëÒ\251(Òtk%ZÃJ\241"
        "\256;I2jÈÃ,âä$\276Óð\004ÇJ\031Êf\227\230\013?Atk%Juë\267\004\213"
        "Ô\026MNð%w\025+\\Z\033\236@gÔ\217È\202&r\027ìD\2048\026\n\025ç\254"
        "Xy\032ØÝ\277\002aieóe\244\254\036\032\030\210áë\270\022\222ö\223"
        "/\rX,YÃBR\224\224\275\227\232\243\216e!\026N.\221}\205e#V\017V\224"
        "\"\251\254ï\034Ö\2569\222\233e5b\205Ëó1iM\263[]ì6ù\224\206ÕG\"úÕ"
        "\205\014ëJ\231\206HH\n\036Ì\",<ÄCdñ\025.\033\261\272Êø:Áê\2725Cn"
        "\'\225b,\253\021\253o.9\203\212q\216Å-r6\243aEÈ\205bÜ\200õ\275e\272"
        "\005\227ÒØ@,Tz;W\214v=÷Ô\222\021K.}\234\247\255\025ÂË1Y}öÉ2ÏjÄ*Æ"
        ";Ê\250\257<p\024\227\"\225à\253\227\006K1\rKQß>\222\242É\032Vè\005"
        "\262ä>\222ÇvþÔ#\245\224\020ë!õf6z\277Z\241\025å\252Xx\215x\221ê\026"
        "\"1Á\007é\262\237f5`\205WÈ\220W\212xy\231MÝO7\000\244yõfU\267ð<\250"
        "fh\262\206Õ]y\205Ôùy\214Dv>\264\231\252vÑ\245\022dA\212Ôß\241Ã~w"
        "\254\232\025ts\242\265J$C\256\255û\253É,<@ÿ\244\207\010\266ö7äøÈ"
        "TWwÜh)\'ñÕ{\237\244\241ÊùT+QÉ|\253\250\257ÜR\215\205\224#t\\\207"
        "cÁê\"\220@\002\t$\220@\002\t$\220@\002\t$\220@\002\t$\220@\002\t"
        "$\220@þßä\177tÐ\007ÇuUy$\0\0\0\0IE\116\104\256B`\202", 4093, NULL)
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

