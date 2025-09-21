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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#ifdef PLATFORM_OSX
#include <Foundation/Foundation.h>
#endif /* PLATFORM_OSX */
#ifdef G_OS_WIN32
#include <windows.h>
#include <datetimeapi.h>
#endif /* G_OS_WIN32 */

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "dialogs-types.h"

#include "config/gimpcoreconfig.h"
#include "config/gimpguiconfig.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpwidgets-utils.h"

#include "about.h"
#include "git-version.h"

#include "about-dialog.h"
#include "authors.h"
#include "gimp-update.h"
#include "gimp-version.h"

#include "gimp-intl.h"


/* The first authors are the creators and maintainers, don't shuffle
 * them
 */
#define START_INDEX (G_N_ELEMENTS (creators)    - 1 /*NULL*/ + \
                     G_N_ELEMENTS (maintainers) - 1 /*NULL*/)


typedef struct
{
  GtkWidget      *dialog;

  Gimp           *gimp;

  GtkWidget      *update_frame;
  GimpCoreConfig *config;

  GtkWidget      *anim_area;
  PangoLayout    *layout;
  gboolean        use_animation;

  gint            n_authors;
  gint            shuffle[G_N_ELEMENTS (authors) - 1];  /* NULL terminated */

  guint           timer;

  gint            index;
  gint            animstep;
  gint            state;
  gboolean        visible;
} GimpAboutDialog;

static void        about_dialog_response      (GtkDialog       *dialog,
                                               gint             response_id,
                                               gpointer         user_data);
#ifdef G_OS_WIN32
static void        about_dialog_realize       (GtkWidget       *widget,
                                               GimpAboutDialog *dialog);
#endif
static void        about_dialog_map           (GtkWidget       *widget,
                                               GimpAboutDialog *dialog);
static void        about_dialog_unmap         (GtkWidget       *widget,
                                               GimpAboutDialog *dialog);
static GdkPixbuf * about_dialog_load_logo     (void);
static void        about_dialog_add_animation (GtkWidget       *vbox,
                                               GimpAboutDialog *dialog);
static void        about_dialog_add_update    (GimpAboutDialog *dialog,
                                               GimpCoreConfig  *config);
static gboolean    about_dialog_anim_draw     (GtkWidget       *widget,
                                               cairo_t         *cr,
                                               GimpAboutDialog *dialog);
static void        about_dialog_reshuffle     (GimpAboutDialog *dialog);
static gboolean    about_dialog_timer         (gpointer         data);

#ifndef GIMP_RELEASE
static void        about_dialog_add_unstable_message
                                              (GtkWidget       *vbox);
#endif /* ! GIMP_RELEASE */

static void        about_dialog_last_release_changed
                                              (GimpCoreConfig   *config,
                                               const GParamSpec *pspec,
                                               GimpAboutDialog  *dialog);
static void        about_dialog_download_clicked
                                              (GtkButton   *button,
                                               const gchar *link);

GtkWidget *
about_dialog_create (Gimp           *gimp,
                     GimpCoreConfig *config)
{
  static GimpAboutDialog dialog;

  if (! dialog.dialog)
    {
      GtkWidget *widget;
      GtkWidget *container;
      GdkPixbuf *pixbuf;
      GList     *children;
      gchar     *copyright;
      gchar     *version;

      dialog.gimp      = gimp;
      dialog.n_authors = G_N_ELEMENTS (authors) - 1;
      dialog.config    = config;

      /* For some people, animated contents may be distracting, or even
       * disturbing. "Vestibular motion disorders" are an example of
       * such discomfort. This is why most platforms have a "reduce
       * animations" option in accessibility settings.
       * When it's set, we just won't display the fancy animated authors
       * list. This is redundant anyway as the full list is available in
       * the Credits tab.
       */
      dialog.use_animation = gimp_widget_animation_enabled ();

      pixbuf = about_dialog_load_logo ();

      copyright = g_strdup_printf (GIMP_COPYRIGHT, GIMP_GIT_LAST_COMMIT_YEAR);
      if (gimp_version_get_revision () > 0)
        /* Translators: the %s is GIMP version, the %d is the
         * installer/package revision.
         * For instance: "2.10.18 (revision 2)"
         */
        version = g_strdup_printf (_("%s (revision %d)"), GIMP_VERSION,
                                   gimp_version_get_revision ());
      else
        version = g_strdup (GIMP_VERSION);

      widget = g_object_new (GTK_TYPE_ABOUT_DIALOG,
                             "role",               "gimp-about",
                             "window-position",    GTK_WIN_POS_CENTER,
                             "title",              _("About GIMP"),
                             "program-name",       GIMP_ACRONYM,
                             "version",            version,
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
      g_free (version);

      g_set_weak_pointer (&dialog.dialog, widget);

      g_signal_connect (widget, "response",
                        G_CALLBACK (about_dialog_response),
                        NULL);
#ifdef G_OS_WIN32
      g_signal_connect (widget, "realize",
                        G_CALLBACK (about_dialog_realize),
                        &dialog);
#endif
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
          if (dialog.use_animation)
            about_dialog_add_animation (children->data, &dialog);
#ifndef GIMP_RELEASE
          about_dialog_add_unstable_message (children->data);
#endif /* ! GIMP_RELEASE */
#ifdef CHECK_UPDATE
          if (gimp_version_check_update ())
            about_dialog_add_update (&dialog, config);
#endif
        }
      else
        g_warning ("%s: ooops, no box in this container?", G_STRLOC);

      g_list_free (children);
    }

  if (GIMP_GUI_CONFIG (config)->show_help_button)
    {
      gimp_help_connect (dialog.dialog, NULL, gimp_standard_help_func,
                         GIMP_HELP_ABOUT_DIALOG, NULL, NULL);

      gtk_dialog_add_buttons (GTK_DIALOG (dialog.dialog),
                              _("_Help"), GTK_RESPONSE_HELP,
                              NULL);
    }

  gtk_style_context_add_class (gtk_widget_get_style_context (dialog.dialog),
                               "gimp-about-dialog");

  return dialog.dialog;
}

static void
about_dialog_response (GtkDialog *dialog,
                       gint       response_id,
                       gpointer   user_data)
{
  if (response_id == GTK_RESPONSE_HELP)
    gimp_standard_help_func (GIMP_HELP_ABOUT_DIALOG, NULL);
  else
    gtk_widget_destroy (GTK_WIDGET (dialog));
}

#ifdef G_OS_WIN32
static void
about_dialog_realize (GtkWidget *widget,
                      GimpAboutDialog *dialog)
{
  gimp_window_set_title_bar_theme (dialog->gimp, widget);
}
#endif

static void
about_dialog_map (GtkWidget       *widget,
                  GimpAboutDialog *dialog)
{
  gimp_update_refresh (dialog->config);

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
  gtk_box_reorder_child (GTK_BOX (vbox), dialog->anim_area, 5);
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
about_dialog_add_update (GimpAboutDialog *dialog,
                         GimpCoreConfig  *config)
{
  GtkWidget *container;
  GList     *children;
  GtkWidget *vbox;

  GtkWidget *frame;
  GtkWidget *box;
  GtkWidget *box2;
  GtkWidget *label;
  GtkWidget *button;
  GtkWidget *button_image;
  GtkWidget *button_label;
  GDateTime *datetime;
  gchar     *date;
  gchar     *text;

  if (dialog->update_frame)
    {
      gtk_widget_destroy (dialog->update_frame);
      dialog->update_frame = NULL;
    }

  /* Get the dialog vbox. */
  container = gtk_dialog_get_content_area (GTK_DIALOG (dialog->dialog));
  children = gtk_container_get_children (GTK_CONTAINER (container));
  g_return_if_fail (GTK_IS_BOX (children->data));
  vbox = children->data;
  g_list_free (children);

  /* The update frame. */
  frame = gtk_frame_new (NULL);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 2);

  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add (GTK_CONTAINER (frame), box);

  /* Button in the frame. */
  button = gtk_button_new ();
  gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  box2 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_container_add (GTK_CONTAINER (button), box2);
  gtk_widget_show (box2);

  button_image = gtk_image_new_from_icon_name (NULL, GTK_ICON_SIZE_DIALOG);
  gtk_box_pack_start (GTK_BOX (box2), button_image, FALSE, FALSE, 0);
  gtk_widget_show (button_image);

  button_label = gtk_label_new (NULL);
  gtk_box_pack_start (GTK_BOX (box2), button_label, FALSE, FALSE, 0);
  gtk_container_child_set (GTK_CONTAINER (box2), button_label, "expand", TRUE, NULL);
  gtk_widget_show (button_label);

  if (config->last_known_release != NULL)
    {
      /* There is a newer version. */
      const gchar *download_url = NULL;
      gchar       *comment      = NULL;

      /* We want the frame to stand out. */
      label = gtk_label_new (NULL);
      text = g_strdup_printf ("<tt><b><big>%s</big></b></tt>",
                              _("Update available!"));
      gtk_label_set_markup (GTK_LABEL (label), text);
      g_free (text);
      gtk_widget_show (label);
      gtk_frame_set_label_widget (GTK_FRAME (frame), label);
      gtk_frame_set_label_align (GTK_FRAME (frame), 0.5, 0.5);
      gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_OUT);
      gtk_box_reorder_child (GTK_BOX (vbox), frame, 3);

      /* Button is an update link. */
      gtk_image_set_from_icon_name (GTK_IMAGE (button_image),
                                    "software-update-available",
                                    GTK_ICON_SIZE_DIALOG);
#ifdef GIMP_UNSTABLE
      download_url = "https://www.gimp.org/downloads/devel/";
#else
      download_url = "https://www.gimp.org/downloads/";
#endif
      g_signal_connect (button, "clicked",
                        (GCallback) about_dialog_download_clicked,
                        (gpointer) download_url);

      /* The preferred localized date representation without the time. */
      datetime = g_date_time_new_from_unix_local (config->last_release_timestamp);
      date = g_date_time_format (datetime, "%x");
      g_date_time_unref (datetime);

      if (config->last_revision > 0)
        {
          /* This is actually a new revision of current version. */
          text = g_strdup_printf (_("Download GIMP %s revision %d (released on %s)\n"),
                                  config->last_known_release,
                                  config->last_revision,
                                  date);

          /* Finally an optional release comment. */
          if (config->last_release_comment)
            {
              /* Translators: <> tags are Pango markup. Please keep these
               * markups in your translation. */
              comment = g_strdup_printf (_("<u>Release comment</u>: <i>%s</i>"), config->last_release_comment);
            }
        }
      else
        {
          text = g_strdup_printf (_("Download GIMP %s (released on %s)\n"),
                                  config->last_known_release, date);
        }
      gtk_label_set_text (GTK_LABEL (button_label), text);
      g_free (text);
      g_free (date);

      if (comment)
        {
          label = gtk_label_new (NULL);
          gtk_label_set_max_width_chars (GTK_LABEL (label), 80);
          gtk_label_set_markup (GTK_LABEL (label), comment);
          gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
          g_free (comment);

          gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
          gtk_widget_show (label);
        }
    }
  else
    {
      /* Button is a "Check for updates" action. */
      gtk_image_set_from_icon_name (GTK_IMAGE (button_image),
                                    "view-refresh",
                                    GTK_ICON_SIZE_MENU);
      gtk_label_set_text (GTK_LABEL (button_label), _("Check for updates"));
      gtk_style_context_add_class (gtk_widget_get_style_context (button),
                                   "text-button");
      g_signal_connect_swapped (button, "clicked",
                                (GCallback) gimp_update_check, config);

    }

  gtk_box_reorder_child (GTK_BOX (vbox), frame, 4);

  /* Last check date box. */
  box2 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  if (config->last_known_release != NULL)
    gtk_widget_set_margin_top (box2, 20);
  gtk_container_add (GTK_CONTAINER (box), box2);
  gtk_widget_show (box2);

  /* Show a small "Check for updates" button only if the big one has
   * been replaced by a download button.
   */
  if (config->last_known_release != NULL)
    {
      button = gtk_button_new_from_icon_name ("view-refresh", GTK_ICON_SIZE_MENU);
      gtk_widget_set_tooltip_text (button, _("Check for updates"));
      gtk_box_pack_start (GTK_BOX (box2), button, FALSE, FALSE, 0);
      g_signal_connect_swapped (button, "clicked",
                                (GCallback) gimp_update_check, config);
      gtk_widget_show (button);
    }

  if (config->check_update_timestamp > 0)
    {
      gchar             *subtext;
      gchar             *time;
#if defined(PLATFORM_OSX)
      NSAutoreleasePool *pool         = [[NSAutoreleasePool alloc] init];
      NSDateFormatter   *formatter    = [[NSDateFormatter alloc] init];
      NSDate            *current_date = [NSDate date];
      NSString          *formatted_date;
      NSString          *formatted_time;
#elif defined(G_OS_WIN32)
      SYSTEMTIME         st;
      int                date_len, time_len;
      wchar_t           *date_buf = NULL;
      wchar_t           *time_buf = NULL;
#endif

      datetime = g_date_time_new_from_unix_local (config->check_update_timestamp);

#if defined(PLATFORM_OSX)
      formatter.locale = [NSLocale currentLocale];

      formatter.dateStyle = NSDateFormatterShortStyle;
      formatter.timeStyle = NSDateFormatterNoStyle;
      formatted_date      = [formatter stringFromDate:current_date];

      formatter.dateStyle = NSDateFormatterNoStyle;
      formatter.timeStyle = NSDateFormatterMediumStyle;
      formatted_time      = [formatter stringFromDate:current_date];

      if (formatted_date)
        date = g_strdup ([formatted_date UTF8String]);
      else
        date = g_date_time_format (datetime, "%x");

      if (formatted_time)
        time = g_strdup ([formatted_time UTF8String]);
      else
        time = g_date_time_format (datetime, "%X");

      [formatter release];
      [pool drain];
#elif defined(G_OS_WIN32)
      GetLocalTime (&st);

      date_len = GetDateFormatEx (LOCALE_NAME_USER_DEFAULT, 0, &st,
                                  NULL, NULL, 0, NULL);
      if (date_len > 0)
       {
          date_buf = g_malloc (date_len * sizeof (wchar_t));
          if (! GetDateFormatEx(LOCALE_NAME_USER_DEFAULT, 0, &st, NULL, date_buf, date_len, NULL))
            {
              g_free (date_buf);
              date_buf = NULL;
            }
        }

      time_len = GetTimeFormatEx (LOCALE_NAME_USER_DEFAULT, 0, &st,
                                  NULL, NULL, 0);
      if (time_len > 0)
        {
            time_buf = g_malloc (time_len * sizeof (wchar_t));
            if (! GetTimeFormatEx (LOCALE_NAME_USER_DEFAULT, 0, &st, NULL, time_buf, time_len))
              {
                g_free (time_buf);
                time_buf = NULL;
              }
        }

      if (date_buf)
        date = g_utf16_to_utf8 ((gunichar2*) date_buf, -1, NULL, NULL, NULL);
      else
        date = g_date_time_format (datetime, "%x");

      if (time_buf)
        time = g_utf16_to_utf8 ((gunichar2*) time_buf, -1, NULL, NULL, NULL);
      else
        time = g_date_time_format (datetime, "%X");

      g_free (date_buf);
      g_free (time_buf);
#else
      date = g_date_time_format (datetime, "%x");
      time = g_date_time_format (datetime, "%X");
#endif
      if (config->last_known_release != NULL)
        /* Translators: first string is the date in the locale's date
        * representation (e.g., 12/31/99), second is the time in the
        * locale's time representation (e.g., 23:13:48).
        */
        subtext = g_strdup_printf (_("Last checked on %s at %s"), date, time);
      else
        /* Translators: first string is the date in the locale's date
        * representation (e.g., 12/31/99), second is the time in the
        * locale's time representation (e.g., 23:13:48).
        */
        subtext = g_strdup_printf (_("Up to date as of %s at %s"), date, time);

      g_date_time_unref (datetime);
      g_free (date);
      g_free (time);

      text = g_strdup_printf ("<i>%s</i>", subtext);
      label = gtk_label_new (NULL);
      gtk_label_set_markup (GTK_LABEL (label), text);
      gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_CENTER);
      gtk_box_pack_start (GTK_BOX (box2), label, FALSE, FALSE, 0);
      gtk_container_child_set (GTK_CONTAINER (box2), label, "expand", TRUE, NULL);
      gtk_widget_show (label);
      g_free (text);
      g_free (subtext);
    }

  gtk_widget_show (box);
  gtk_widget_show (frame);

  g_set_weak_pointer (&dialog->update_frame, frame);

  /* Reconstruct the dialog when release info changes. */
  g_signal_connect (config, "notify::last-known-release",
                    (GCallback) about_dialog_last_release_changed,
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
  gdouble          alpha = 0.0;
  gint             x, y;
  gint             width, height;

  if (! dialog->visible)
    return FALSE;

  if (dialog->animstep < 16)
    {
      alpha = (gfloat) dialog->animstep / 15.0;
    }
  else if (dialog->animstep < 18)
    {
      alpha = 1.0;
    }
  else if (dialog->animstep < 33)
    {
      alpha = 1.0 - ((gfloat) (dialog->animstep - 17)) / 15.0;
    }

  gtk_style_context_get_color (style, gtk_style_context_get_state (style),
                               &color);
  gdk_cairo_set_source_rgba (cr, &color);

  gtk_widget_get_allocation (widget, &allocation);
  pango_layout_get_pixel_size (dialog->layout, &width, &height);

  x = (allocation.width  - width)  / 2;
  y = (allocation.height - height) / 2;

  cairo_move_to (cr, x, y);

  cairo_push_group (cr);

  pango_cairo_show_layout (cr, dialog->layout);

  cairo_pop_group_to_source (cr);
  cairo_paint_with_alpha (cr, alpha);

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

static void
decorate_text (GimpAboutDialog *dialog,
               gint             anim_type,
               gdouble          time)
{
  const gchar    *text;
  const gchar    *ptr;
  gint            letter_count = 0;
  gint            cluster_start, cluster_end;
  gunichar        unichr;
  PangoAttrList  *attrlist = NULL;
  PangoAttribute *attr;
  PangoRectangle  irect = {0, 0, 0, 0};
  PangoRectangle  lrect = {0, 0, 0, 0};

  text = pango_layout_get_text (dialog->layout);

  g_return_if_fail (text != NULL);

  attrlist = pango_attr_list_new ();

  switch (anim_type)
    {
    case 0: /* Fade in */
      break;

    case 1: /* Fade in, spread */
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

    default:
      g_printerr ("Unknown animation type %d\n", anim_type);
    }

  pango_layout_set_attributes (dialog->layout, attrlist);
  pango_attr_list_unref (attrlist);
}

static gboolean
about_dialog_timer (gpointer data)
{
  GimpAboutDialog *dialog        = data;
  gint             timeout       = 0;

  if (dialog->animstep == 0)
    {
      gchar *text = NULL;

      dialog->visible = TRUE;

      switch (dialog->state)
        {
        case 0:
          dialog->timer = g_timeout_add (30, about_dialog_timer, dialog);
          dialog->state += 1;
          return G_SOURCE_REMOVE;

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
      decorate_text (dialog, 1, 1.0 - ((gfloat) (dialog->animstep - 17)) / 15.0);
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
      return G_SOURCE_REMOVE;
    }

  /* else keep the current timeout */
  return G_SOURCE_CONTINUE;
}

#ifndef GIMP_RELEASE

static void
about_dialog_add_unstable_message (GtkWidget *vbox)
{
  GtkWidget *label;
  gchar     *text;

  text = g_strdup_printf (_("This is a development build\n"
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

#endif /* ! GIMP_RELEASE */

static void
about_dialog_last_release_changed (GimpCoreConfig   *config,
                                   const GParamSpec *pspec,
                                   GimpAboutDialog  *dialog)
{
  g_signal_handlers_disconnect_by_func (config,
                                        (GCallback) about_dialog_last_release_changed,
                                        dialog);
  if (! dialog->dialog)
    return;

  about_dialog_add_update (dialog, config);
}

static void
about_dialog_download_clicked (GtkButton   *button,
                               const gchar *link)
{
  GtkWidget *window;

  window = gtk_widget_get_ancestor (GTK_WIDGET (button), GTK_TYPE_WINDOW);

  if (window)
    gtk_show_uri_on_window (GTK_WINDOW (window), link, GDK_CURRENT_TIME, NULL);
}
