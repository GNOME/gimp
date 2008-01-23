/* GIMP - The GNU Image Manipulation Program
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

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "dialogs-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"

#include "pdb/gimppdb.h"

#include "about.h"

#include "about-dialog.h"
#include "authors.h"

#include "gimp-intl.h"


#define PDB_URL_LOAD   "plug-in-web-browser"


typedef struct
{
  GtkWidget    *dialog;

  GtkWidget    *anim_area;
  PangoLayout  *layout;

  gint          n_authors;
  gint          shuffle[G_N_ELEMENTS (authors) - 1];  /* NULL terminated */

  guint         timer;

  gint          index;
  gint          animstep;
  gint          textrange[2];
  gint          state;
  gboolean      visible;
} GimpAboutDialog;


static void        about_dialog_map           (GtkWidget       *widget,
                                               GimpAboutDialog *dialog);
static void        about_dialog_unmap         (GtkWidget       *widget,
                                               GimpAboutDialog *dialog);
static void        about_dialog_load_url      (GtkAboutDialog  *dialog,
                                               const gchar     *url,
                                               gpointer         data);
static GdkPixbuf * about_dialog_load_logo     (void);
static void        about_dialog_add_animation (GtkWidget       *vbox,
                                               GimpAboutDialog *dialog);
static gboolean    about_dialog_anim_expose   (GtkWidget       *widget,
                                               GdkEventExpose  *event,
                                               GimpAboutDialog *dialog);
static void        about_dialog_reshuffle     (GimpAboutDialog *dialog);
static gboolean    about_dialog_timer         (gpointer         data);

static void        about_dialog_add_message   (GtkWidget       *vbox);


GtkWidget *
about_dialog_create (GimpContext *context)
{
  static GimpAboutDialog *dialog = NULL;

  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  if (! dialog)
    {
      GtkWidget *widget;
      GtkWidget *container;
      GdkPixbuf *pixbuf;
      GList     *children;

      if (gimp_pdb_lookup_procedure (context->gimp->pdb, PDB_URL_LOAD))
        gtk_about_dialog_set_url_hook (about_dialog_load_url,
                                       g_object_ref (context),
                                       (GDestroyNotify) g_object_unref);

      dialog = g_new0 (GimpAboutDialog, 1);

      dialog->n_authors = G_N_ELEMENTS (authors) - 1;

      pixbuf = about_dialog_load_logo ();

      widget = g_object_new (GTK_TYPE_ABOUT_DIALOG,
                             "role",               "about-dialog",
                             "window-position",    GTK_WIN_POS_CENTER,
                             "title",              _("About GIMP"),
                             (gtk_check_version (2, 12, 0) ?
                              "name" :
                              "program-name"),     GIMP_ACRONYM,
                             "version",            GIMP_VERSION,
                             "copyright",          GIMP_COPYRIGHT,
                             "comments",           GIMP_NAME,
                             "license",            GIMP_LICENSE,
                             "wrap-license",       TRUE,
                             "logo",               pixbuf,
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

      if (pixbuf)
        g_object_unref (pixbuf);

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

      /*  kids, don't try this at home!  */
      container = GTK_DIALOG (widget)->vbox;
      children = gtk_container_get_children (GTK_CONTAINER (container));

      if (GTK_IS_VBOX (children->data))
        {
          about_dialog_add_animation (children->data, dialog);
          about_dialog_add_message (children->data);
        }
      else
        g_warning ("%s: ooops, no vbox in this container?", G_STRLOC);

      g_list_free (children);
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
  GValueArray *return_vals;

  return_vals = gimp_pdb_execute_procedure_by_name (context->gimp->pdb,
                                                    context, NULL,
                                                    PDB_URL_LOAD,
                                                    G_TYPE_STRING, url,
                                                    G_TYPE_NONE);
  g_value_array_free (return_vals);
}

static GdkPixbuf *
about_dialog_load_logo (void)
{
  GdkPixbuf *pixbuf;
  gchar     *filename;

  filename = g_build_filename (gimp_data_directory (), "images",
                               "gimp-logo.png",
                               NULL);

  pixbuf = gdk_pixbuf_new_from_file (filename, NULL);
  g_free (filename);

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

  g_signal_connect (dialog->anim_area, "expose-event",
                    G_CALLBACK (about_dialog_anim_expose),
                    dialog);
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
about_dialog_anim_expose (GtkWidget       *widget,
                          GdkEventExpose  *event,
                          GimpAboutDialog *dialog)
{
  GdkGC *text_gc;
  gint   x, y;
  gint   width, height;

  if (! dialog->visible)
    return FALSE;

  text_gc = widget->style->text_gc[GTK_STATE_NORMAL];

  pango_layout_get_pixel_size (dialog->layout, &width, &height);

  x = (widget->allocation.width - width) / 2;
  y = (widget->allocation.height - height) / 2;

  if (dialog->textrange[1] > 0)
    {
      GdkRegion *covered_region = NULL;
      GdkRegion *rect_region;

      covered_region = gdk_pango_layout_get_clip_region (dialog->layout,
                                                         x, y,
                                                         dialog->textrange, 1);

      rect_region = gdk_region_rectangle (&event->area);

      gdk_region_intersect (covered_region, rect_region);
      gdk_region_destroy (rect_region);

      gdk_gc_set_clip_region (text_gc, covered_region);
      gdk_region_destroy (covered_region);
    }

  gdk_draw_layout (widget->window, text_gc, x, y, dialog->layout);

  gdk_gc_set_clip_region (text_gc, NULL);

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

static void inline
mix_colors (const GdkColor *start,
            const GdkColor *end,
            GdkColor       *target,
            gdouble         pos)
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
  GdkColor        mix;

  mix_colors (dialog->anim_area->style->bg + GTK_STATE_NORMAL,
              dialog->anim_area->style->fg + GTK_STATE_NORMAL, &mix, time);

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

          mix_colors (dialog->anim_area->style->fg + GTK_STATE_NORMAL,
                      dialog->anim_area->style->bg + GTK_STATE_NORMAL,
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

static void
about_dialog_add_message (GtkWidget *vbox)
{
#ifdef GIMP_UNSTABLE
  GtkWidget *label;

  label = gtk_label_new (_("This is an unstable development release."));
  gimp_label_set_attributes (GTK_LABEL (label),
                             PANGO_ATTR_STYLE, PANGO_STYLE_ITALIC,
                             -1);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_box_reorder_child (GTK_BOX (vbox), label, 2);
  gtk_widget_show (label);
#endif
}

