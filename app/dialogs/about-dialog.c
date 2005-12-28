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

#include "libgimpbase/gimpbase.h"

#include "dialogs-types.h"

#include "core/gimpcontext.h"

#include "pdb/procedural_db.h"

#include "about.h"

#include "about-dialog.h"
#include "authors.h"

#include "gimp-intl.h"


#define PDB_URL_LOAD   "plug-in-web-browser"


typedef struct
{
  GtkWidget    *dialog;

  GtkWidget    *logo_area;
  GdkPixmap    *logo_pixmap;
  gint          logo_width;
  gint          logo_height;

  GdkRectangle  text_area;
  gdouble       text_size;
  gdouble       min_text_size;
  PangoLayout  *layout;
  PangoFontDescription *font_desc;

  gint          n_authors;
  gint          shuffle[G_N_ELEMENTS (authors) - 1];  /* NULL terminated */

  guint         timer;

  gint          index;
  gint          animstep;
  gint          textrange[2];
  gint          state;
  gboolean      visible;
  gboolean      pp;
} GimpAboutDialog;


static void      about_dialog_map         (GtkWidget       *widget,
                                           GimpAboutDialog *dialog);
static void      about_dialog_unmap       (GtkWidget       *widget,
                                           GimpAboutDialog *dialog);
static void      about_dialog_load_url    (GtkAboutDialog  *dialog,
                                           const gchar     *url,
                                           gpointer         data);
static void      about_dialog_add_logo    (GtkWidget       *vbox,
                                           GimpAboutDialog *dialog);
static gboolean  about_dialog_logo_expose (GtkWidget       *widget,
                                           GdkEventExpose  *event,
                                           GimpAboutDialog *dialog);
static gboolean  about_dialog_load_logo   (GtkWidget       *widget,
                                           GimpAboutDialog *dialog);
static void      about_dialog_reshuffle   (GimpAboutDialog *dialog);
static gboolean  about_dialog_timer       (gpointer         data);


GtkWidget *
about_dialog_create (GimpContext *context)
{
  static GimpAboutDialog *dialog = NULL;

  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  if (! dialog)
    {
      GtkWidget       *widget;
      GtkWidget       *container;
      GList           *children;
      GdkModifierType  mask;

      if (procedural_db_lookup (context->gimp, PDB_URL_LOAD))
        gtk_about_dialog_set_url_hook (about_dialog_load_url,
                                       g_object_ref (context),
                                       (GDestroyNotify) g_object_unref);

      dialog = g_new0 (GimpAboutDialog, 1);

      dialog->n_authors = G_N_ELEMENTS (authors) - 1;

      widget = g_object_new (GTK_TYPE_ABOUT_DIALOG,
                             "role",               "about-dialog",
                             "window-position",    GTK_WIN_POS_CENTER,
                             "name",               GIMP_ACRONYM,
                             "version",            GIMP_VERSION,
                             "copyright",          GIMP_COPYRIGHT,
                             "comments",           GIMP_NAME,
                             "license",            GIMP_LICENSE,
                             "wrap-license",       TRUE,
                             "website",            "http://www.gimp.org/",
                             "website-label",      _("Visit the GIMP website"),
                             "authors",            authors,
                             "artists",            artists,
                             "documenters",        documenters,
                             /* Translators: insert your names here,
                              * separated by newline
                              */
                             "translator-credits", _("translator-credits"),
                             NULL);

      dialog->dialog = widget;

      g_object_add_weak_pointer (G_OBJECT (widget), (gpointer) &dialog);

      g_signal_connect (widget, "response",
                        G_CALLBACK (gtk_widget_destroy),
                        NULL);

      g_signal_connect (widget, "map",
                        G_CALLBACK (about_dialog_map),
                        dialog);
      g_signal_connect (widget, "unmap",
                        G_CALLBACK (about_dialog_unmap),
                        dialog);

      if (! about_dialog_load_logo (widget, dialog))
        {
          gtk_widget_destroy (widget);
          return NULL;
        }

      /*  kids, don't try this at home!  */
      container = GTK_DIALOG (widget)->vbox;
      children = gtk_container_get_children (GTK_CONTAINER (container));

      if (GTK_IS_VBOX (children->data))
        about_dialog_add_logo (children->data, dialog);
      else
        g_warning ("%s: ooops, no vbox in this container?", G_STRLOC);

      g_list_free (children);

      /* weird magic to determine the way the logo should be shown */
      gdk_window_get_pointer (NULL, NULL, NULL, &mask);

      mask &= ~GDK_BUTTON3_MASK;
      dialog->pp = (mask &= (GDK_SHIFT_MASK | GDK_CONTROL_MASK) &
                    (GDK_CONTROL_MASK | GDK_MOD1_MASK) &
                    (GDK_MOD1_MASK | ~GDK_SHIFT_MASK));

      dialog->text_area.height = dialog->pp ? 50 : 32;
      dialog->text_area.y = (dialog->logo_height - dialog->text_area.height);
    }

  gtk_window_present (GTK_WINDOW (dialog->dialog));

  return dialog->dialog;
}

static void
about_dialog_map (GtkWidget       *widget,
                  GimpAboutDialog *dialog)
{
  if (dialog->layout && dialog->timer == 0)
    {
      dialog->state    = 0;
      dialog->index    = 0;
      dialog->animstep = 0;
      dialog->visible  = FALSE;

      about_dialog_reshuffle (dialog);

      dialog->timer = g_timeout_add (800, about_dialog_timer, dialog);
    }
}

static void
about_dialog_unmap (GtkWidget       *widget,
                    GimpAboutDialog *dialog)
{
  if (dialog->timer)
    {
      g_source_remove (dialog->timer);
      dialog->timer = 0;
    }
}

static void
about_dialog_load_url (GtkAboutDialog *dialog,
                       const gchar    *url,
                       gpointer        data)
{
  GimpContext *context = GIMP_CONTEXT (data);
  Argument    *return_vals;
  gint         nreturn_vals;

  return_vals = procedural_db_run_proc (context->gimp, context, NULL,
                                        PDB_URL_LOAD,
                                        &nreturn_vals,
                                        GIMP_PDB_STRING, url,
                                        GIMP_PDB_END);
  procedural_db_destroy_args (return_vals, nreturn_vals);
}


static void
about_dialog_add_logo (GtkWidget       *vbox,
                       GimpAboutDialog *dialog)
{
  GtkWidget *align;
  GList     *children;

  children = gtk_container_get_children (GTK_CONTAINER (vbox));

  for (children = gtk_container_get_children (GTK_CONTAINER (vbox));
       children;
       children = g_list_next (children))
    {
      if (GTK_IS_IMAGE (children->data))
        {
          gtk_widget_hide (children->data);
          break;
        }
    }

  g_list_free (children);

  align = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (vbox), align, FALSE, FALSE, 0);
  gtk_box_reorder_child (GTK_BOX (vbox), align, 0);
  gtk_widget_show (align);

  dialog->logo_area = gtk_drawing_area_new ();
  gtk_widget_set_size_request (dialog->logo_area,
                               dialog->logo_width, dialog->logo_height);
  gtk_widget_set_events (dialog->logo_area, GDK_EXPOSURE_MASK);
  gtk_container_add (GTK_CONTAINER (align), dialog->logo_area);
  gtk_widget_show (dialog->logo_area);

  g_signal_connect (dialog->logo_area, "expose-event",
                    G_CALLBACK (about_dialog_logo_expose),
                    dialog);

  /* place the scrolltext at the bottom of the image */
  dialog->text_area.width = dialog->logo_width;
  dialog->text_area.height = 32; /* gets changed in map() as well */
  dialog->text_area.x = 0;
  dialog->text_area.y = (dialog->logo_height - dialog->text_area.height);

  dialog->layout = gtk_widget_create_pango_layout (dialog->logo_area, NULL);
  g_object_weak_ref (G_OBJECT (dialog->logo_area),
                     (GWeakNotify) g_object_unref, dialog->layout);

  dialog->font_desc = pango_font_description_from_string ("Sans, 11");

  pango_layout_set_font_description (dialog->layout, dialog->font_desc);
  pango_layout_set_justify (dialog->layout, PANGO_ALIGN_CENTER);
}

static void
about_dialog_reshuffle (GimpAboutDialog *dialog)
{
  GRand *gr = g_rand_new ();
  gint   i;

  for (i = 0; i < dialog->n_authors; i++)
    dialog->shuffle[i] = i;

  /* here we rely on the authors array having Peter and Spencer first */
#define START_INDEX 2

  for (i = START_INDEX; i < dialog->n_authors; i++)
    {
      gint j = g_rand_int_range (gr, START_INDEX, dialog->n_authors);

      if (i != j)
        {
          gint t;

          t = dialog->shuffle[j];
          dialog->shuffle[j] = dialog->shuffle[i];
          dialog->shuffle[i] = t;
        }
    }

#undef START_INDEX

  g_rand_free (gr);
}

static gboolean
about_dialog_logo_expose (GtkWidget       *widget,
                          GdkEventExpose  *event,
                          GimpAboutDialog *dialog)
{
  gdk_gc_set_clip_rectangle (widget->style->black_gc, &event->area);

  gdk_draw_drawable (widget->window,
                     widget->style->black_gc,
                     dialog->logo_pixmap,
                     event->area.x, event->area.y +
                     (dialog->pp ? dialog->logo_height : 0),
                     event->area.x, event->area.y,
                     event->area.width, event->area.height);

  gdk_gc_set_clip_rectangle (widget->style->black_gc, NULL);

  if (dialog->visible)
    {
      GdkGC *text_gc;
      gint   layout_x, layout_y;
      gint   width, height;

      text_gc = dialog->logo_area->style->text_gc[GTK_STATE_NORMAL];

      pango_layout_get_pixel_size (dialog->layout, &width, &height);

      layout_x = dialog->text_area.x + (dialog->text_area.width - width) / 2;
      layout_y = dialog->text_area.y + (dialog->text_area.height - height) / 2;

      if (dialog->textrange[1] > 0)
        {
          GdkRegion *covered_region = NULL;
          GdkRegion *rect_region;

          covered_region =
            gdk_pango_layout_get_clip_region (dialog->layout,
                                              layout_x, layout_y,
                                              dialog->textrange, 1);

          rect_region = gdk_region_rectangle (&event->area);

          gdk_region_intersect (covered_region, rect_region);
          gdk_region_destroy (rect_region);

          gdk_gc_set_clip_region (text_gc, covered_region);
          gdk_region_destroy (covered_region);
        }

      gdk_draw_layout (widget->window,
                       widget->style->text_gc[GTK_STATE_NORMAL],
                       layout_x, layout_y,
                       dialog->layout);

      gdk_gc_set_clip_region (text_gc, NULL);
    }

  return FALSE;
}

static gchar *
insert_spacers (const gchar *string)
{
  GString  *str = g_string_new (NULL);
  gchar    *normalized;
  gchar    *ptr;
  gunichar  unichr;

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


/* color constants */

static const PangoColor foreground = { 5000,      5000,      5000      };
static const PangoColor background = { 139 * 257, 137 * 257, 124 * 257 };

static const PangoColor gradient[] =
{
  { 139 * 257,  137 * 257,  124 * 257  },
  { 65535,      65535,      65535      },
  { 5000,       5000,       5000       }
};

/* backup values */

static const PangoColor grad1ent[] =
{
  { 0xff * 257, 0xba * 257, 0x00 * 257 },
  { 37522,      51914,      57568      }
};

static const PangoColor foregr0und = { 37522, 51914, 57568 };
static const PangoColor backgr0und = { 0,     0,     0     };

static void
mix_gradient (const PangoColor *gradient,
              guint             ncolors,
              PangoColor       *target,
              gdouble           pos)
{
  gint index;

  if (pos == 1.0)
    {
      target->red   = gradient[ncolors - 1].red;
      target->green = gradient[ncolors - 1].green;
      target->blue  = gradient[ncolors - 1].blue;
      return;
    }

  index = (gint) floor (pos * (ncolors - 1));
  pos = pos * (ncolors - 1) - index;

  target->red   = (gradient[index].red *
                   (1.0 - pos) + gradient[index + 1].red   * pos);
  target->green = (gradient[index].green *
                   (1.0 - pos) + gradient[index + 1].green * pos);
  target->blue  = (gradient[index].blue *
                   (1.0 - pos) + gradient[index + 1].blue  * pos);
}

static void inline
mix_colors (const PangoColor *start,
            const PangoColor *end,
            PangoColor       *target,
            gdouble           pos)
{
  target->red   = start->red   * (1.0 - pos) + end->red   * pos;
  target->green = start->green * (1.0 - pos) + end->green * pos;
  target->blue  = start->blue  * (1.0 - pos) + end->blue  * pos;
}

static void
decorate_text (GimpAboutDialog *dialog,
               gint             anim_type,
               gdouble          time)
{
  const gchar    *text;
  const gchar    *ptr;
  gint            letter_count = 0;
  gint            text_length  = 0;
  gint            text_bytelen = 0;
  gint            cluster_start, cluster_end;
  gunichar        unichr;
  PangoAttrList  *attrlist = NULL;
  PangoAttribute *attr;
  PangoRectangle  irect = {0, 0, 0, 0};
  PangoRectangle  lrect = {0, 0, 0, 0};
  PangoColor      mix;

  mix_colors ((dialog->pp ? &backgr0und : &background),
              (dialog->pp ? &foregr0und : &foreground), &mix, time);

  text = pango_layout_get_text (dialog->layout);
  g_return_if_fail (text != NULL);

  text_length = g_utf8_strlen (text, -1);
  text_bytelen = strlen (text);

  attrlist = pango_attr_list_new ();

  dialog->textrange[0] = 0;
  dialog->textrange[1] = text_bytelen;

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

      letter_count  = 0;
      cluster_start = 0;

      while ((unichr = g_utf8_get_char (ptr)))
        {
          gint    border = (text_length + 15) * time - 15;
          gdouble pos;

          if (letter_count < border)
            pos = 0;
          else if (letter_count > border + 15)
            pos = 1;
          else
            pos = ((gdouble) (letter_count - border)) / 15;

          mix_colors ((dialog->pp ? &foregr0und : &foreground),
                      (dialog->pp ? &backgr0und : &background),
                      &mix, pos);

          ptr = g_utf8_next_char (ptr);

          cluster_end = ptr - text;

          attr = pango_attr_foreground_new (mix.red, mix.green, mix.blue);
          attr->start_index = cluster_start;
          attr->end_index = cluster_end;
          pango_attr_list_change (attrlist, attr);

          if (pos < 1.0)
            dialog->textrange[1] = cluster_end;

          letter_count++;
          cluster_start = cluster_end;
        }

      break;

    case 4: /* letterwise Fade in, triangular */
      ptr = text;

      letter_count  = 0;
      cluster_start = 0;

      while ((unichr = g_utf8_get_char (ptr)))
        {
          gint    border = (text_length + 15) * time - 15;
          gdouble pos;

          if (letter_count < border)
            pos = 1.0;
          else if (letter_count > border + 15)
            pos = 0.0;
          else
            pos = 1.0 - ((gdouble) (letter_count - border)) / 15;

          mix_gradient (dialog->pp ? grad1ent : gradient,
                        dialog->pp ?
                        G_N_ELEMENTS (grad1ent) : G_N_ELEMENTS (gradient),
                        &mix, pos);

          ptr = g_utf8_next_char (ptr);

          cluster_end = ptr - text;

          attr = pango_attr_foreground_new (mix.red, mix.green, mix.blue);
          attr->start_index = cluster_start;
          attr->end_index = cluster_end;
          pango_attr_list_change (attrlist, attr);

          if (pos > 0.0)
            dialog->textrange[1] = cluster_end;

          letter_count++;
          cluster_start = cluster_end;
        }
      break;

    default:
      g_printerr ("Unknown animation type %d\n", anim_type);
    }

  pango_layout_set_attributes (dialog->layout, attrlist);
  pango_attr_list_unref (attrlist);
}

static gboolean
about_dialog_timer (gpointer data)
{
  GimpAboutDialog *dialog  = data;
  gint             timeout = 0;

  if (dialog->animstep == 0)
    {
      gchar   *text = NULL;
      gdouble  size = 11.0;
      gint     width;
      gint     height;

      dialog->visible = TRUE;

      switch (dialog->state)
        {
        case 0:
          dialog->timer = g_timeout_add (30, about_dialog_timer, dialog);
          dialog->state += 1;
          return FALSE;

        case 1:
          text = insert_spacers (_("GIMP is brought to you by"));
          dialog->state += 1;
          break;

        case 2:
          if (! (dialog->index < dialog->n_authors))
            dialog->index = 0;

          text = insert_spacers (authors[dialog->shuffle[dialog->index]]);
          dialog->index += 1;
          break;

        default:
          g_return_val_if_reached (TRUE);
          break;
        }

      g_return_val_if_fail (text != NULL, TRUE);

      pango_layout_set_text (dialog->layout, text, -1);
      pango_layout_set_attributes (dialog->layout, NULL);

      pango_font_description_set_size (dialog->font_desc, size * PANGO_SCALE);
      pango_layout_set_font_description (dialog->layout, dialog->font_desc);

      pango_layout_get_pixel_size (dialog->layout, &width, &height);

      while (width >= dialog->text_area.width && size >= 6.0)
        {
          size -= 0.5;
          pango_font_description_set_size (dialog->font_desc,
                                           size * PANGO_SCALE);
          pango_layout_set_font_description (dialog->layout, dialog->font_desc);
          pango_layout_get_pixel_size (dialog->layout, &width, &height);
        }
    }

  if (dialog->animstep < 16)
    {
      decorate_text (dialog, 4, ((gfloat) dialog->animstep) / 15.0);
    }
  else if (dialog->animstep == 16)
    {
      timeout = 800;
    }
  else if (dialog->animstep == 17)
    {
      timeout = 30;
    }
  else if (dialog->animstep < 33)
    {
      decorate_text (dialog, 1,
                     1.0 - ((gfloat) (dialog->animstep - 17)) / 15.0);
    }
  else if (dialog->animstep == 33)
    {
      dialog->visible = FALSE;
      timeout = 300;
    }
  else
    {
      dialog->visible  = FALSE;
      dialog->animstep = -1;
      timeout = 30;
    }

  dialog->animstep++;

  gtk_widget_queue_draw_area (dialog->logo_area,
                              dialog->text_area.x,
                              dialog->text_area.y,
                              dialog->text_area.width,
                              dialog->text_area.height);

  if (timeout > 0)
    {
      dialog->timer = g_timeout_add (timeout, about_dialog_timer, dialog);
      return FALSE;
    }

  /* else keep the current timeout */
  return TRUE;
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
about_dialog_load_logo (GtkWidget       *widget,
                         GimpAboutDialog *dialog)
{
  GdkPixbuf *pixbuf;
  GdkGC     *gc;
  GPL       *noise;
  gchar     *filename;

  g_return_val_if_fail (dialog->logo_pixmap == NULL, FALSE);

  filename = g_build_filename (gimp_data_directory (), "images",
                               "gimp-logo.png",
                               NULL);

  pixbuf = gdk_pixbuf_new_from_file (filename, NULL);
  g_free (filename);

  if (! pixbuf)
    return FALSE;

  dialog->logo_width = gdk_pixbuf_get_width (pixbuf);
  dialog->logo_height = gdk_pixbuf_get_height (pixbuf);

  gtk_widget_realize (widget);

  dialog->logo_pixmap = gdk_pixmap_new (widget->window,
                                        dialog->logo_width,
                                        dialog->logo_height * 2,
                                        -1);

  gc = gdk_gc_new (dialog->logo_pixmap);

  /* draw a defined content to the pixmap */
  gdk_draw_rectangle (GDK_DRAWABLE (dialog->logo_pixmap),
                      gc, TRUE,
                      0, 0,
                      dialog->logo_width, dialog->logo_height * 2);

  gdk_draw_pixbuf (GDK_DRAWABLE (dialog->logo_pixmap),
                   gc, pixbuf,
                   0, 0, 0, 0,
                   dialog->logo_width, dialog->logo_height,
                   GDK_RGB_DITHER_NORMAL, 0, 0);

  g_object_unref (pixbuf);

  if ((noise = random ()) && line (noise, (const guchar *)
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
      gdk_draw_pixbuf (GDK_DRAWABLE (dialog->logo_pixmap),
                       gc, white (noise), 0, 0,
                       (dialog->logo_width - level (white (noise))) / 2,
                       dialog->logo_height + (dialog->logo_height -
                                              variance (white (noise))) / 2,
                       level (white (noise)),
                       variance (white (noise)),
                       GDK_RGB_DITHER_NORMAL, 0, 0);
    }

  g_object_unref (noise);
  g_object_unref (gc);

  return TRUE;
}
