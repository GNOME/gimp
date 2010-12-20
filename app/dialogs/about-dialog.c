/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "dialogs-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"

#include "pdb/gimppdb.h"

#include "about.h"
#include "git-version.h"

#include "about-dialog.h"
#include "authors.h"

#include "gimp-intl.h"


/* The first authors are the creators and maintainers, don't shuffle
 * them
 */
#define START_INDEX (G_N_ELEMENTS (creators)    - 1 /*NULL*/ + \
                     G_N_ELEMENTS (maintainers) - 1 /*NULL*/)


typedef struct
{
  GtkWidget   *dialog;

  GtkWidget   *anim_area;
  PangoLayout *layout;

  gint         n_authors;
  gint         shuffle[G_N_ELEMENTS (authors) - 1];  /* NULL terminated */

  guint        timer;

  gint         index;
  gint         animstep;
  gint         textrange[2];
  gint         state;
  gboolean     visible;
} GimpAboutDialog;


static void        about_dialog_map           (GtkWidget       *widget,
                                               GimpAboutDialog *dialog);
static void        about_dialog_unmap         (GtkWidget       *widget,
                                               GimpAboutDialog *dialog);
static GdkPixbuf * about_dialog_load_logo     (void);
static void        about_dialog_add_animation (GtkWidget       *vbox,
                                               GimpAboutDialog *dialog);
static gboolean    about_dialog_anim_draw     (GtkWidget       *widget,
                                               cairo_t         *cr,
                                               GimpAboutDialog *dialog);
static void        about_dialog_reshuffle     (GimpAboutDialog *dialog);
static gboolean    about_dialog_timer         (gpointer         data);

#ifdef GIMP_UNSTABLE
static void        about_dialog_add_unstable_message
                                              (GtkWidget       *vbox);
#endif /* GIMP_UNSTABLE */


GtkWidget *
about_dialog_create (GimpContext *context)
{
  static GimpAboutDialog dialog;

  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  if (! dialog.dialog)
    {
      GtkWidget *widget;
      GtkWidget *container;
      GdkPixbuf *pixbuf;
      GList     *children;
      gchar     *copyright;

      dialog.n_authors = G_N_ELEMENTS (authors) - 1;

      pixbuf = about_dialog_load_logo ();

      copyright = g_strdup_printf (GIMP_COPYRIGHT, GIMP_GIT_LAST_COMMIT_YEAR);

      widget = g_object_new (GTK_TYPE_ABOUT_DIALOG,
                             "role",               "gimp-about",
                             "window-position",    GTK_WIN_POS_CENTER,
                             "title",              _("About GIMP"),
                             "program-name",       GIMP_ACRONYM,
                             "version",            GIMP_VERSION,
                             "copyright",          copyright,
                             "comments",           GIMP_NAME,
                             "license",            GIMP_LICENSE,
                             "wrap-license",       TRUE,
                             "logo",               pixbuf,
                             "website",            "https://www.gimp.org/",
                             "website-label",      _("Visit the GIMP website"),
                             "authors",            authors,
                             "artists",            artists,
                             "documenters",        documenters,
                             /* Translators: insert your names here,
                                separated by newline */
                             "translator-credits", _("translator-credits"),
                             NULL);

      if (pixbuf)
        g_object_unref (pixbuf);

      g_free (copyright);

      dialog.dialog = widget;

      g_object_add_weak_pointer (G_OBJECT (widget), (gpointer) &dialog.dialog);

      g_signal_connect (widget, "response",
                        G_CALLBACK (gtk_widget_destroy),
                        NULL);

      g_signal_connect (widget, "map",
                        G_CALLBACK (about_dialog_map),
                        &dialog);
      g_signal_connect (widget, "unmap",
                        G_CALLBACK (about_dialog_unmap),
                        &dialog);

      /*  kids, don't try this at home!  */
      container = gtk_dialog_get_content_area (GTK_DIALOG (widget));
      children = gtk_container_get_children (GTK_CONTAINER (container));

      if (GTK_IS_BOX (children->data))
        {
          about_dialog_add_animation (children->data, &dialog);
#ifdef GIMP_UNSTABLE
          about_dialog_add_unstable_message (children->data);
#endif /* GIMP_UNSTABLE */
        }
      else
        g_warning ("%s: ooops, no box in this container?", G_STRLOC);

      g_list_free (children);
    }

  gtk_window_present (GTK_WINDOW (dialog.dialog));

  return dialog.dialog;
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

static GdkPixbuf *
about_dialog_load_logo (void)
{
  GdkPixbuf    *pixbuf = NULL;
  GFile        *file;
  GInputStream *input;

  file = gimp_data_directory_file ("images",
#ifdef GIMP_UNSTABLE
                                   "gimp-devel-logo.png",
#else
                                   "gimp-logo.png",
#endif
                                   NULL);

  input = G_INPUT_STREAM (g_file_read (file, NULL, NULL));
  g_object_unref (file);

  if (input)
    {
      pixbuf = gdk_pixbuf_new_from_stream (input, NULL, NULL);
      g_object_unref (input);
    }

  return pixbuf;
}

static void
about_dialog_add_animation (GtkWidget       *vbox,
                            GimpAboutDialog *dialog)
{
  gint  height;

  dialog->anim_area = gtk_drawing_area_new ();
  gtk_box_pack_start (GTK_BOX (vbox), dialog->anim_area, FALSE, FALSE, 0);
  gtk_box_reorder_child (GTK_BOX (vbox), dialog->anim_area, 4);
  gtk_widget_show (dialog->anim_area);

  dialog->layout = gtk_widget_create_pango_layout (dialog->anim_area, NULL);
  g_object_weak_ref (G_OBJECT (dialog->anim_area),
                     (GWeakNotify) g_object_unref, dialog->layout);

  pango_layout_get_pixel_size (dialog->layout, NULL, &height);

  gtk_widget_set_size_request (dialog->anim_area, -1, 2 * height);

  g_signal_connect (dialog->anim_area, "draw",
                    G_CALLBACK (about_dialog_anim_draw),
                    dialog);
}

static void
about_dialog_reshuffle (GimpAboutDialog *dialog)
{
  GRand *gr = g_rand_new ();
  gint   i;

  for (i = 0; i < dialog->n_authors; i++)
    dialog->shuffle[i] = i;

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

  g_rand_free (gr);
}

static gboolean
about_dialog_anim_draw (GtkWidget       *widget,
                        cairo_t         *cr,
                        GimpAboutDialog *dialog)
{
  GtkStyleContext *style = gtk_widget_get_style_context (widget);
  GtkAllocation    allocation;
  GdkRGBA          color;
  gint             x, y;
  gint             width, height;

  if (! dialog->visible)
    return FALSE;

  gtk_style_context_save (style);
  gtk_style_context_add_class (style, GTK_STYLE_CLASS_ENTRY);
  gtk_style_context_get_color (style, 0, &color);
  gdk_cairo_set_source_rgba (cr, &color);
  gtk_style_context_restore (style);

  gtk_widget_get_allocation (widget, &allocation);
  pango_layout_get_pixel_size (dialog->layout, &width, &height);

  x = (allocation.width  - width)  / 2;
  y = (allocation.height - height) / 2;

  if (dialog->textrange[1] > 0)
    {
      cairo_region_t *covered_region;

      covered_region = gdk_pango_layout_get_clip_region (dialog->layout,
                                                         x, y,
                                                         dialog->textrange, 1);

      gdk_cairo_region (cr, covered_region);
      cairo_clip (cr);

      cairo_region_destroy (covered_region);
    }

  cairo_move_to (cr, x, y);

  pango_cairo_show_layout (cr, dialog->layout);

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

static inline void
mix_colors (const GdkRGBA *start,
            const GdkRGBA *end,
            GdkRGBA       *target,
            gdouble        pos)
{
  target->red   = start->red   * (1.0 - pos) + end->red   * pos;
  target->green = start->green * (1.0 - pos) + end->green * pos;
  target->blue  = start->blue  * (1.0 - pos) + end->blue  * pos;
  target->alpha = start->alpha * (1.0 - pos) + end->alpha * pos;
}

static void
decorate_text (GimpAboutDialog *dialog,
               gint             anim_type,
               gdouble          time)
{
  GtkStyleContext *style = gtk_widget_get_style_context (dialog->anim_area);
  const gchar     *text;
  const gchar     *ptr;
  gint             letter_count = 0;
  gint             text_length  = 0;
  gint             text_bytelen = 0;
  gint             cluster_start, cluster_end;
  gunichar         unichr;
  PangoAttrList   *attrlist = NULL;
  PangoAttribute  *attr;
  PangoRectangle   irect = {0, 0, 0, 0};
  PangoRectangle   lrect = {0, 0, 0, 0};
  GdkRGBA          fg;
  GdkRGBA          bg;
  GdkRGBA          mix;

  gtk_style_context_get_color (style, 0, &fg);
  gtk_style_context_get_background_color (style, 0, &bg);

  mix_colors (&bg, &fg, &mix, time);

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
      attr = pango_attr_foreground_new (mix.red   * 0xffff,
                                        mix.green * 0xffff,
                                        mix.blue  * 0xffff);
      attr->start_index = 0;
      attr->end_index = text_bytelen;
      pango_attr_list_insert (attrlist, attr);
      break;

    case 1: /* Fade in, spread */
      attr = pango_attr_foreground_new (mix.red   * 0xffff,
                                        mix.green * 0xffff,
                                        mix.blue  * 0xffff);
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
      attr = pango_attr_foreground_new (mix.red   * 0xffff,
                                        mix.green * 0xffff,
                                        mix.blue  * 0xffff);
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

          mix_colors (&fg, &bg, &mix, pos);

          ptr = g_utf8_next_char (ptr);

          cluster_end = ptr - text;

          attr = pango_attr_foreground_new (mix.red   * 0xffff,
                                            mix.green * 0xffff,
                                            mix.blue  * 0xffff);
          attr->start_index = cluster_start;
          attr->end_index = cluster_end;
          pango_attr_list_change (attrlist, attr);

          if (pos < 1.0)
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
      gchar *text = NULL;

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

      g_free (text);
    }

  if (dialog->animstep < 16)
    {
      decorate_text (dialog, 2, ((gfloat) dialog->animstep) / 15.0);
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

  gtk_widget_queue_draw (dialog->anim_area);

  if (timeout > 0)
    {
      dialog->timer = g_timeout_add (timeout, about_dialog_timer, dialog);
      return FALSE;
    }

  /* else keep the current timeout */
  return TRUE;
}

#ifdef GIMP_UNSTABLE

static void
about_dialog_add_unstable_message (GtkWidget *vbox)
{
  GtkWidget *label;
  gchar     *text;

  text = g_strdup_printf (_("This is an unstable development release\n"
                            "commit %s"), GIMP_GIT_VERSION_ABBREV);
  label = gtk_label_new (text);
  g_free (text);

  gtk_label_set_selectable (GTK_LABEL (label), TRUE);
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_CENTER);
  gimp_label_set_attributes (GTK_LABEL (label),
                             PANGO_ATTR_STYLE, PANGO_STYLE_ITALIC,
                             -1);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_box_reorder_child (GTK_BOX (vbox), label, 2);
  gtk_widget_show (label);
}

#endif /* GIMP_UNSTABLE */
