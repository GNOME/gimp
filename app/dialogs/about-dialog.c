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
#import <AppKit/AppKit.h>
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

#include "core/gimp-utils.h"

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

#ifdef PLATFORM_OSX
static NSWindow *previous_key_window = nil;
#endif

static void        about_dialog_response      (GtkDialog       *dialog,
                                               gint             response_id,
                                               gpointer         user_data);
#if defined(G_OS_WIN32) || defined(PLATFORM_OSX)
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

static gchar     * about_dialog_debug_text      (void);
static void        about_dialog_add_debug_info  (GtkBox        *vbox);
static void        about_dialog_copy_debug_info (GtkButton     *button,
                                                 gpointer       data);

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

static gchar *
gimp_native_date_time_format (GDateTime   *datetime,
                              const gchar *format)
{
#define FORMAT_DATE 1
#define FORMAT_TIME 2

#if defined(PLATFORM_OSX) || defined(G_OS_WIN32)
  gint   mode     = (g_strcmp0 (format, "%x") == 0) ? FORMAT_DATE : FORMAT_TIME;
  gchar *result   = NULL;
#endif

#if defined(PLATFORM_OSX)
  NSAutoreleasePool *pool         = [[NSAutoreleasePool alloc] init];
  NSDateFormatter   *formatter    = [[NSDateFormatter alloc] init];
  NSDate            *current_date = [NSDate date];
  NSString          *formatted    = NULL;

  formatter.locale = [NSLocale currentLocale];

  if (mode == FORMAT_DATE)
    {
      formatter.dateStyle = NSDateFormatterShortStyle;
      formatter.timeStyle = NSDateFormatterNoStyle;
    }
  else
    {
      formatter.dateStyle = NSDateFormatterNoStyle;
      formatter.timeStyle = NSDateFormatterMediumStyle;
    }
  formatted = [formatter stringFromDate:current_date];

  if (formatted)
    result = g_strdup ([formatted UTF8String]);

  [formatter release];
  [pool drain];

  if (result)
    return result;

#elif defined(G_OS_WIN32)
  SYSTEMTIME st;
  int        date_len, time_len;
  wchar_t   *date_buf = NULL;
  wchar_t   *time_buf = NULL;

  GetLocalTime (&st);

  if (mode == FORMAT_DATE)
    {
      date_len = GetDateFormatEx (LOCALE_NAME_USER_DEFAULT, 0, &st,
                                  NULL, NULL, 0, NULL);
      if (date_len > 0)
        {
          date_buf = g_malloc (date_len * sizeof (wchar_t));
          if (GetDateFormatEx(LOCALE_NAME_USER_DEFAULT, 0, &st, NULL, date_buf, date_len, NULL))
            result = g_utf16_to_utf8 ((gunichar2*) date_buf, -1, NULL, NULL, NULL);
          g_free (date_buf);
        }
    }
  else
    {
      time_len = GetTimeFormatEx (LOCALE_NAME_USER_DEFAULT, 0, &st,
                                  NULL, NULL, 0);
      if (time_len > 0)
        {
          time_buf = g_malloc (time_len * sizeof (wchar_t));
          if (GetTimeFormatEx(LOCALE_NAME_USER_DEFAULT, 0, &st, NULL, time_buf, time_len))
            result = g_utf16_to_utf8 ((gunichar2*) time_buf, -1, NULL, NULL, NULL);
          g_free (time_buf);
        }
    }

  if (result)
    return result;
#endif

  /* Fallback for Linux/Unix and for Windows/macOS API failures */
  return g_date_time_format (datetime, format);

#undef FORMAT_DATE
#undef FORMAT_TIME
}

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

#ifdef PLATFORM_OSX
      /* since `widget` is not created with gimp_dialog_new, there is no auto focus */
      previous_key_window = [NSApp keyWindow];
#endif

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
#if defined(G_OS_WIN32) || defined(PLATFORM_OSX)
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
          about_dialog_add_debug_info (GTK_BOX (children->data));
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

#if defined(G_OS_WIN32) || defined(PLATFORM_OSX)
static void
about_dialog_realize (GtkWidget *widget,
                      GimpAboutDialog *dialog)
{
#ifdef G_OS_WIN32
  gimp_window_set_title_bar_theme (dialog->gimp, widget);
#endif

#ifdef PLATFORM_OSX
  /* since `widget` is not created with gimp_dialog_new, drop minimize manually */
  GdkWindow *window = gtk_widget_get_window (widget);

  if (window)
    {
      gdk_window_set_functions (window,
                                GDK_FUNC_RESIZE |
                                GDK_FUNC_MOVE   |
                                GDK_FUNC_CLOSE  |
                                GDK_FUNC_MAXIMIZE);
    }
#endif
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

#ifdef PLATFORM_OSX
  /* restore the focus manually due to the reason stated on about_dialog_create */
  if (previous_key_window && [previous_key_window canBecomeKeyWindow])
    {
      [previous_key_window makeKeyAndOrderFront:nil];
      previous_key_window = nil;
    }
#endif
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
  gtk_widget_set_visible (dialog->anim_area, TRUE);

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
  gtk_widget_set_visible (button, TRUE);

  box2 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_container_add (GTK_CONTAINER (button), box2);
  gtk_widget_set_visible (box2, TRUE);

  button_image = gtk_image_new_from_icon_name (NULL, GTK_ICON_SIZE_DIALOG);
  gtk_box_pack_start (GTK_BOX (box2), button_image, FALSE, FALSE, 0);
  gtk_widget_set_visible (button_image, TRUE);

  button_label = gtk_label_new (NULL);
  gtk_box_pack_start (GTK_BOX (box2), button_label, FALSE, FALSE, 0);
  gtk_container_child_set (GTK_CONTAINER (box2), button_label, "expand", TRUE, NULL);
  gtk_widget_set_visible (button_label, TRUE);

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
      gtk_widget_set_visible (label, TRUE);
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
      date = gimp_native_date_time_format (datetime, "%x");
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
          gtk_widget_set_visible (label, TRUE);
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
  gtk_widget_set_visible (box2, TRUE);

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
      gtk_widget_set_visible (button, TRUE);
    }

  if (config->check_update_timestamp > 0)
    {
      gchar             *subtext;
      gchar             *time;

      datetime = g_date_time_new_from_unix_local (config->check_update_timestamp);

      date = gimp_native_date_time_format (datetime, "%x");
      time = gimp_native_date_time_format (datetime, "%X");
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
      gtk_widget_set_visible (label, TRUE);
      g_free (text);
      g_free (subtext);
    }

  gtk_widget_set_visible (box, TRUE);
  gtk_widget_set_visible (frame, TRUE);

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
  gtk_widget_set_visible (label, TRUE);
}

#endif /* ! GIMP_RELEASE */

/* the version block shown in the about dialog and dropped on the clipboard by
 * the Copy button - a few lines a bug report actually needs, not the full
 * dump. caller frees.
 */
static gchar *
about_dialog_debug_text (void)
{
  GString     *text;
  GdkDisplay  *display;
  gchar       *os_name;
  gchar       *package;
  const gchar *display_sys;
  const gchar *type_name;
  gchar       *mem_size;
  gchar       *swap_dir;
  guint64      phys_mem;
  guint64      avail_mem;
  gint         babl_major, babl_minor, babl_micro;
  gint         gegl_major, gegl_minor, gegl_micro;

  os_name = g_get_os_info (G_OS_INFO_KEY_PRETTY_NAME);
  if (! os_name)
    {
      gchar *name    = g_get_os_info (G_OS_INFO_KEY_NAME);
      gchar *version = g_get_os_info (G_OS_INFO_KEY_VERSION_ID);

      if (name && version)
        os_name = g_strdup_printf ("%s %s", name, version);
      else if (name)
        os_name = g_strdup (name);

      g_free (name);
      g_free (version);
    }
  babl_get_version (&babl_major, &babl_minor, &babl_micro);
  gegl_get_version (&gegl_major, &gegl_minor, &gegl_micro);

  /* map the GdkDisplay class to a friendly windowing-system name without
   * dragging in the per-backend headers (gdkx.h/gdkwayland.h)
   */
  display   = gdk_display_get_default ();
  type_name = display ? G_OBJECT_TYPE_NAME (display) : "";
  if (strstr (type_name, "Wayland"))
    display_sys = "Wayland";
  else if (strstr (type_name, "X11"))
    display_sys = "X11";
  else if (strstr (type_name, "Win32"))
    display_sys = "Windows (GDI)";
  else if (strstr (type_name, "Quartz"))
    display_sys = "Quartz";
  else if (strstr (type_name, "Broadway"))
    display_sys = "Broadway";
  else
    display_sys = type_name;

  /* GIMP_BUILD_ID is "unknown" on a plain source build. So put
   * "Built from source" instead of "unknown" into the report.
   */
  if (g_strcmp0 (GIMP_BUILD_ID, "unknown") == 0)
    package = g_strdup (_("built from source"));
  else if (gimp_version_get_revision () > 0)
    package = g_strdup_printf ("%s revision %d",
                               GIMP_BUILD_ID, gimp_version_get_revision ());
  else
    package = g_strdup (GIMP_BUILD_ID);

  text = g_string_new (NULL);

  g_string_append_printf (text, "%s %s (%s)\n",
                          _("Version:"), GIMP_VERSION, package);
  g_string_append_printf (text, "%s %s (%s)\n",
                          _("OS:"), os_name ? os_name : "unknown",
                          display_sys);

  /* total physical RAM - gimp_get_physical_memory_size returns 0 where the
   * platform can't tell us
   */
  phys_mem  = gimp_get_physical_memory_size ();
  avail_mem = gimp_get_available_memory_size ();
  if (phys_mem > 0)
    {
      mem_size = g_format_size (phys_mem);

      if (avail_mem > 0)
        {
          gchar *free_size = g_format_size (avail_mem);

          g_string_append_printf (text, _("Memory: %s (%s free)\n"),
                                  mem_size, free_size);
          g_free (free_size);
        }
      else
        {
          g_string_append_printf (text, "%s %s\n", _("Memory:"), mem_size);
        }

      g_free (mem_size);
    }
  else
    {
      g_string_append_printf (text, "%s %s\n", _("Memory:"), "unknown");
    }

  /* how much room is left on the partition GEGL swaps to - the dir itself
   * isn't much use in a report, so only the free space goes in. same source
   * the dashboard reads
   */
  g_object_get (gegl_config (), "swap", &swap_dir, NULL);
  if (swap_dir)
    {
      GFile     *file = g_file_new_for_path (swap_dir);
      GFileInfo *info = g_file_query_filesystem_info (
                          file, G_FILE_ATTRIBUTE_FILESYSTEM_FREE, NULL, NULL);

      if (info)
        {
          guint64 free_space =
            g_file_info_get_attribute_uint64 (info,
                                              G_FILE_ATTRIBUTE_FILESYSTEM_FREE);
          gchar *free_str = g_format_size (free_space);

          g_string_append_printf (text, "%s %s free on partition\n",
                                  _("Swap:"), free_str);
          g_free (free_str);
          g_object_unref (info);
        }

      g_object_unref (file);
      g_free (swap_dir);
    }

  g_string_append_printf (text,
                          "%s GEGL %d.%d.%d, babl %d.%d.%d, GTK %d.%d.%d",
                          _("Library Versions:"),
                          gegl_major, gegl_minor, gegl_micro,
                          babl_major, babl_minor, babl_micro,
                          gtk_get_major_version (),
                          gtk_get_minor_version (),
                          gtk_get_micro_version ());

  g_free (os_name);
  g_free (package);

  return g_string_free (text, FALSE);
}

/* lay the "key: value" lines out in a two-column grid so all the values line
 * up. the key's colon always comes before any colon in the value (e.g. a
 * windows "C:\" path), so a split on the first colon is safe.
 */
static GtkWidget *
about_dialog_info_grid (gchar **lines)
{
  GtkWidget *grid;
  gint       i;
  gint       row = 0;

  grid = gtk_grid_new ();
  gtk_widget_set_halign (grid, GTK_ALIGN_CENTER);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 12);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 2);

  for (i = 0; lines[i]; i++)
    {
      GtkWidget   *key;
      GtkWidget   *value;
      gchar       *sep = strchr (lines[i], ':');
      gchar       *k;
      const gchar *v;

      if (sep)
        {
          k = g_strndup (lines[i], sep - lines[i] + 1);
          v = sep + 1;
          while (*v == ' ')
            v++;
        }
      else
        {
          k = g_strdup (lines[i]);
          v = "";
        }

      key = gtk_label_new (k);
      gtk_label_set_xalign (GTK_LABEL (key), 0.0);

      value = gtk_label_new (v);
      gtk_label_set_xalign (GTK_LABEL (value), 0.0);
      gtk_label_set_selectable (GTK_LABEL (value), TRUE);
      gtk_label_set_ellipsize (GTK_LABEL (value), PANGO_ELLIPSIZE_MIDDLE);
      gtk_label_set_max_width_chars (GTK_LABEL (value), 50);

      gtk_grid_attach (GTK_GRID (grid), key,   0, row, 1, 1);
      gtk_grid_attach (GTK_GRID (grid), value, 1, row, 1, 1);

      g_free (k);
      row++;
    }

  return grid;
}

/* once the pointer leaves the Copy button, put its tooltip back to the
 * "Copy ..." wording so a fresh hover doesn't still read "Copied ..."
 */
static gboolean
about_dialog_copy_button_left (GtkWidget *button,
                               GdkEvent  *event,
                               gpointer   data)
{
  gtk_widget_set_tooltip_text (button, _("Copy Version Information"));

  return FALSE;
}

/* the version/website/copyright labels ride the "main" page of a GtkStack
 * that sits in the dialog's vbox. GtkAboutDialog hands us no accessor, so
 * pick the stack out of the vbox's children.
 */
static void
about_dialog_add_debug_info (GtkBox *dialog_vbox)
{
  GtkWidget  *stack = NULL;
  GtkWidget  *page;
  GtkWidget  *vbox;
  GtkWidget  *header;
  GtkWidget  *heading;
  GtkWidget  *button;
  GList      *children;
  GList      *iter;
  gchar      *text;
  gchar     **lines;
  gint        pos = -1;
  gint        i;

  children = gtk_container_get_children (GTK_CONTAINER (dialog_vbox));
  for (iter = children; iter; iter = iter->next)
    if (GTK_IS_STACK (iter->data))
      {
        stack = iter->data;
        break;
      }
  g_list_free (children);

  if (! stack)
    return;

  page = gtk_stack_get_child_by_name (GTK_STACK (stack), "main");
  if (! GTK_IS_BOX (page))
    return;

  text  = about_dialog_debug_text ();
  lines = g_strsplit (text, "\n", -1);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_widget_set_halign (vbox, GTK_ALIGN_CENTER);
  gtk_widget_set_margin_top (vbox, 4);
  gtk_widget_set_margin_bottom (vbox, 4);

  /* header row - "Version Information:" with the Copy button sat right next
   * to it. a word on the button, not an icon, so it still shows where the
   * icon theme isn't installed
   */
  header  = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_widget_set_halign (header, GTK_ALIGN_CENTER);
  heading = gtk_label_new (_("Version Information"));

  button = gtk_button_new_with_mnemonic (_("_Copy"));
  gtk_widget_set_tooltip_text (button, _("Copy Version Information"));
  /* stash the copy blob on the button - it outlives the grid's own copy */
  g_object_set_data_full (G_OBJECT (button), "debug-text", text, g_free);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (about_dialog_copy_debug_info), NULL);
  g_signal_connect (button, "leave-notify-event",
                    G_CALLBACK (about_dialog_copy_button_left), NULL);

  gtk_box_pack_start (GTK_BOX (header), heading, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (header), button, FALSE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX (vbox), header, FALSE, FALSE, 0);

  /* every line shows in the dialog and rides the clipboard copy alike */
  gtk_box_pack_start (GTK_BOX (vbox),
                      about_dialog_info_grid (lines), FALSE, FALSE, 0);

  g_strfreev (lines);

  /* drop it right before the copyright line, so it sits between the website
   * link and the copyright
   */
  children = gtk_container_get_children (GTK_CONTAINER (page));
  for (iter = children, i = 0; iter; iter = iter->next, i++)
    if (g_strcmp0 (gtk_buildable_get_name (GTK_BUILDABLE (iter->data)),
                   "copyright_label") == 0)
      {
        pos = i;
        break;
      }
  g_list_free (children);

  gtk_box_pack_start (GTK_BOX (page), vbox, FALSE, FALSE, 0);
  if (pos >= 0)
    gtk_box_reorder_child (GTK_BOX (page), vbox, pos);
  gtk_widget_show_all (vbox);
}

static void
about_dialog_copy_debug_info (GtkButton *button,
                              gpointer   data)
{
  GtkClipboard *clipboard;
  const gchar  *text;

  text = g_object_get_data (G_OBJECT (button), "debug-text");

  clipboard = gtk_widget_get_clipboard (GTK_WIDGET (button),
                                        GDK_SELECTION_CLIPBOARD);
  gtk_clipboard_set_text (clipboard, text, -1);

  /* flip the tooltip to "Copied ..." right away - trigger_tooltip_query
   * repaints the popup that's already up under the pointer. it goes back to
   * "Copy ..." once the pointer leaves (see the leave-notify handler)
   */
  gtk_widget_set_tooltip_text (GTK_WIDGET (button),
                               _("Copied Version Information"));
  gtk_widget_trigger_tooltip_query (GTK_WIDGET (button));
}

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
