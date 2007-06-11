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
#include "libgimpthumb/gimpthumb.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "config/gimpcoreconfig.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpimagefile.h"
#include "core/gimpprogress.h"
#include "core/gimpsubprogress.h"

#include "plug-in/gimppluginmanager.h"

#include "file/file-procedure.h"
#include "file/file-utils.h"

#include "gimpfiledialog.h" /* eek */
#include "gimpthumbbox.h"
#include "gimpview.h"
#include "gimpviewrenderer-frame.h"
#include "gimpwidgets-utils.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void     gimp_thumb_box_progress_iface_init (GimpProgressInterface *iface);

static void     gimp_thumb_box_dispose            (GObject           *object);
static void     gimp_thumb_box_finalize           (GObject           *object);

static void     gimp_thumb_box_style_set          (GtkWidget         *widget,
                                                   GtkStyle          *prev_style);

static GimpProgress *
                gimp_thumb_box_progress_start     (GimpProgress      *progress,
                                                   const gchar       *message,
                                                   gboolean           cancelable);
static void     gimp_thumb_box_progress_end       (GimpProgress      *progress);
static gboolean gimp_thumb_box_progress_is_active (GimpProgress      *progress);
static void     gimp_thumb_box_progress_set_value (GimpProgress      *progress,
                                                   gdouble            percentage);
static gdouble  gimp_thumb_box_progress_get_value (GimpProgress      *progress);
static void     gimp_thumb_box_progress_pulse     (GimpProgress      *progress);

static gboolean gimp_thumb_box_progress_message   (GimpProgress      *progress,
                                                   Gimp              *gimp,
                                                   GimpMessageSeverity  severity,
                                                   const gchar       *domain,
                                                   const gchar       *message);

static gboolean gimp_thumb_box_ebox_button_press  (GtkWidget         *widget,
                                                   GdkEventButton    *bevent,
                                                   GimpThumbBox      *box);
static void gimp_thumb_box_thumbnail_clicked      (GtkWidget         *widget,
                                                   GdkModifierType    state,
                                                   GimpThumbBox      *box);
static void gimp_thumb_box_imagefile_info_changed (GimpImagefile     *imagefile,
                                                   GimpThumbBox      *box);
static void gimp_thumb_box_thumb_state_notify     (GimpThumbnail     *thumb,
                                                   GParamSpec        *pspec,
                                                   GimpThumbBox      *box);
static void gimp_thumb_box_create_thumbnails      (GimpThumbBox      *box,
                                                   gboolean           force);
static void gimp_thumb_box_create_thumbnail       (GimpThumbBox      *box,
                                                   const gchar       *uri,
                                                   GimpThumbnailSize  size,
                                                   gboolean           force,
                                                   GimpProgress      *progress);
static gboolean gimp_thumb_box_auto_thumbnail     (GimpThumbBox      *box);


G_DEFINE_TYPE_WITH_CODE (GimpThumbBox, gimp_thumb_box, GTK_TYPE_FRAME,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_PROGRESS,
                                                gimp_thumb_box_progress_iface_init))

#define parent_class gimp_thumb_box_parent_class


static void
gimp_thumb_box_class_init (GimpThumbBoxClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose   = gimp_thumb_box_dispose;
  object_class->finalize  = gimp_thumb_box_finalize;

  widget_class->style_set = gimp_thumb_box_style_set;
}

static void
gimp_thumb_box_init (GimpThumbBox *box)
{
  gtk_frame_set_shadow_type (GTK_FRAME (box), GTK_SHADOW_IN);

  box->idle_id = 0;
}

static void
gimp_thumb_box_progress_iface_init (GimpProgressInterface *iface)
{
  iface->start     = gimp_thumb_box_progress_start;
  iface->end       = gimp_thumb_box_progress_end;
  iface->is_active = gimp_thumb_box_progress_is_active;
  iface->set_value = gimp_thumb_box_progress_set_value;
  iface->get_value = gimp_thumb_box_progress_get_value;
  iface->pulse     = gimp_thumb_box_progress_pulse;
  iface->message   = gimp_thumb_box_progress_message;
}

static void
gimp_thumb_box_dispose (GObject *object)
{
  GimpThumbBox *box = GIMP_THUMB_BOX (object);

  if (box->idle_id)
    {
      g_source_remove (box->idle_id);
      box->idle_id = 0;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_thumb_box_finalize (GObject *object)
{
  GimpThumbBox *box = GIMP_THUMB_BOX (object);

  gimp_thumb_box_take_uris (box, NULL);

  if (box->imagefile)
    {
      g_object_unref (box->imagefile);
      box->imagefile = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_thumb_box_style_set (GtkWidget *widget,
                          GtkStyle  *prev_style)
{
  GimpThumbBox *box = GIMP_THUMB_BOX (widget);
  GtkWidget    *ebox;

  GTK_WIDGET_CLASS (parent_class)->style_set (widget, prev_style);

  gtk_widget_modify_bg (box->preview, GTK_STATE_NORMAL,
                        &widget->style->base[GTK_STATE_NORMAL]);
  gtk_widget_modify_bg (box->preview, GTK_STATE_INSENSITIVE,
                        &widget->style->base[GTK_STATE_NORMAL]);

  ebox = gtk_bin_get_child (GTK_BIN (widget));

  gtk_widget_modify_bg (ebox, GTK_STATE_NORMAL,
                        &widget->style->base[GTK_STATE_NORMAL]);
  gtk_widget_modify_bg (ebox, GTK_STATE_INSENSITIVE,
                        &widget->style->base[GTK_STATE_NORMAL]);
}

static GimpProgress *
gimp_thumb_box_progress_start (GimpProgress *progress,
                               const gchar  *message,
                               gboolean      cancelable)
{
  GimpThumbBox *box = GIMP_THUMB_BOX (progress);

  if (! box->progress_active)
    {
      GtkProgressBar *bar = GTK_PROGRESS_BAR (box->progress);
      GtkWidget      *toplevel;

      gtk_progress_bar_set_fraction (bar, 0.0);

      box->progress_active = TRUE;

      toplevel = gtk_widget_get_toplevel (GTK_WIDGET (box));

      if (GIMP_IS_FILE_DIALOG (toplevel))
        gtk_dialog_set_response_sensitive (GTK_DIALOG (toplevel),
                                           GTK_RESPONSE_CANCEL, cancelable);

      return progress;
    }

  return NULL;
}

static void
gimp_thumb_box_progress_end (GimpProgress *progress)
{
  GimpThumbBox *box = GIMP_THUMB_BOX (progress);

  if (box->progress_active)
    {
      GtkProgressBar *bar = GTK_PROGRESS_BAR (box->progress);

      gtk_progress_bar_set_fraction (bar, 0.0);

      box->progress_active = FALSE;
    }
}

static gboolean
gimp_thumb_box_progress_is_active (GimpProgress *progress)
{
  GimpThumbBox *box = GIMP_THUMB_BOX (progress);

  return box->progress_active;
}

static void
gimp_thumb_box_progress_set_value (GimpProgress *progress,
                                   gdouble       percentage)
{
  GimpThumbBox *box = GIMP_THUMB_BOX (progress);

  if (box->progress_active)
    {
      GtkProgressBar *bar = GTK_PROGRESS_BAR (box->progress);

      gtk_progress_bar_set_fraction (bar, percentage);
    }
}

static gdouble
gimp_thumb_box_progress_get_value (GimpProgress *progress)
{
  GimpThumbBox *box = GIMP_THUMB_BOX (progress);

  if (box->progress_active)
    {
      GtkProgressBar *bar = GTK_PROGRESS_BAR (box->progress);

      return gtk_progress_bar_get_fraction (bar);
    }

  return 0.0;
}

static void
gimp_thumb_box_progress_pulse (GimpProgress *progress)
{
  GimpThumbBox *box = GIMP_THUMB_BOX (progress);

  if (box->progress_active)
    {
      GtkProgressBar *bar = GTK_PROGRESS_BAR (box->progress);

      gtk_progress_bar_pulse (bar);
    }
}

static gboolean
gimp_thumb_box_progress_message (GimpProgress        *progress,
                                 Gimp                *gimp,
                                 GimpMessageSeverity  severity,
                                 const gchar         *domain,
                                 const gchar         *message)
{
  /*  GimpThumbBox never shows any messages  */

  return TRUE;
}


/*  public functions  */

GtkWidget *
gimp_thumb_box_new (GimpContext *context)
{
  GimpThumbBox   *box;
  GtkWidget      *vbox;
  GtkWidget      *vbox2;
  GtkWidget      *ebox;
  GtkWidget      *hbox;
  GtkWidget      *button;
  GtkWidget      *label;
  gchar          *str;
  gint            h, v;
  GtkRequisition  info_requisition;
  GtkRequisition  progress_requisition;

  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  box = g_object_new (GIMP_TYPE_THUMB_BOX, NULL);

  box->context = context;

  ebox = gtk_event_box_new ();
  gtk_container_add (GTK_CONTAINER (box), ebox);
  gtk_widget_show (ebox);

  g_signal_connect (ebox, "button-press-event",
                    G_CALLBACK (gimp_thumb_box_ebox_button_press),
                    box);

  str = g_strdup_printf (_("Click to update preview\n"
                           "%s%sClick to force update even "
                           "if preview is up-to-date"),
                         gimp_get_mod_string (GDK_CONTROL_MASK),
                         gimp_get_mod_separator ());

  gimp_help_set_help_data (ebox, str, NULL);

  g_free (str);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (ebox), vbox);
  gtk_widget_show (vbox);

  button = gtk_button_new ();
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  label = gtk_label_new_with_mnemonic (_("Pr_eview"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_container_add (GTK_CONTAINER (button), label);
  gtk_widget_show (label);

  g_signal_connect (button, "button-press-event",
                    G_CALLBACK (gtk_true),
                    NULL);
  g_signal_connect (button, "button-release-event",
                    G_CALLBACK (gtk_true),
                    NULL);
  g_signal_connect (button, "enter-notify-event",
                    G_CALLBACK (gtk_true),
                    NULL);
  g_signal_connect (button, "leave-notify-event",
                    G_CALLBACK (gtk_true),
                    NULL);

  vbox2 = gtk_vbox_new (FALSE, 6);
  gtk_container_set_border_width (GTK_CONTAINER (vbox2), 2);
  gtk_container_add (GTK_CONTAINER (vbox), vbox2);
  gtk_widget_show (vbox2);

  hbox = gtk_hbox_new (TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  box->imagefile = gimp_imagefile_new (context->gimp, NULL);

  g_signal_connect (box->imagefile, "info-changed",
                    G_CALLBACK (gimp_thumb_box_imagefile_info_changed),
                    box);

  g_signal_connect (box->imagefile->thumbnail, "notify::thumb-state",
                    G_CALLBACK (gimp_thumb_box_thumb_state_notify),
                    box);

  gimp_view_renderer_get_frame_size (&h, &v);

  box->preview = gimp_view_new (context,
                                GIMP_VIEWABLE (box->imagefile),
                                /* add padding for the shadow frame */
                                context->gimp->config->thumbnail_size +
                                MAX (h, v),
                                0, FALSE);

  gtk_box_pack_start (GTK_BOX (hbox), box->preview, TRUE, FALSE, 2);
  gtk_widget_show (box->preview);

  gtk_label_set_mnemonic_widget (GTK_LABEL (label), box->preview);

  g_signal_connect (box->preview, "clicked",
                    G_CALLBACK (gimp_thumb_box_thumbnail_clicked),
                    box);

  box->filename = gtk_label_new (_("No selection"));
  gtk_label_set_ellipsize (GTK_LABEL (box->filename), PANGO_ELLIPSIZE_MIDDLE);
  gtk_label_set_justify (GTK_LABEL (box->filename), GTK_JUSTIFY_CENTER);
  gimp_label_set_attributes (GTK_LABEL (box->filename),
                             PANGO_ATTR_STYLE, PANGO_STYLE_OBLIQUE,
                             -1);
  gtk_box_pack_start (GTK_BOX (vbox2), box->filename, FALSE, FALSE, 0);
  gtk_widget_show (box->filename);

  box->info = gtk_label_new (" \n \n \n ");
  gtk_misc_set_alignment (GTK_MISC (box->info), 0.5, 0.0);
  gtk_label_set_justify (GTK_LABEL (box->info), GTK_JUSTIFY_CENTER);
  gimp_label_set_attributes (GTK_LABEL (box->info),
                             PANGO_ATTR_SCALE, PANGO_SCALE_SMALL,
                             -1);
  gtk_box_pack_start (GTK_BOX (vbox2), box->info, FALSE, FALSE, 0);
  gtk_widget_show (box->info);

  box->progress = gtk_progress_bar_new ();
  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (box->progress), "Fog");
  gtk_box_pack_end (GTK_BOX (vbox2), box->progress, FALSE, FALSE, 0);
  gtk_widget_set_no_show_all (box->progress, TRUE);
  /* don't gtk_widget_show (box->progress); */

  /* eek */
  gtk_widget_size_request (box->info,     &info_requisition);
  gtk_widget_size_request (box->progress, &progress_requisition);

  gtk_widget_set_size_request (box->info,
                               progress_requisition.width,
                               info_requisition.height);
  gtk_widget_set_size_request (box->filename,
                               progress_requisition.width, -1);

  gtk_widget_set_size_request (box->progress, -1,
                               progress_requisition.height);
  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (box->progress), "");

  return GTK_WIDGET (box);
}

void
gimp_thumb_box_take_uri (GimpThumbBox *box,
                         gchar        *uri)
{
  g_return_if_fail (GIMP_IS_THUMB_BOX (box));

  if (box->idle_id)
    {
      g_source_remove (box->idle_id);
      box->idle_id = 0;
    }

  gimp_object_take_name (GIMP_OBJECT (box->imagefile), uri);

  if (uri)
    {
      gchar *basename = file_utils_uri_display_basename (uri);

      gtk_label_set_text (GTK_LABEL (box->filename), basename);
      g_free (basename);
    }
  else
    {
      gtk_label_set_text (GTK_LABEL (box->filename), _("No selection"));
    }

  gtk_widget_set_sensitive (GTK_WIDGET (box), uri != NULL);
  gimp_imagefile_update (box->imagefile);
}

void
gimp_thumb_box_take_uris (GimpThumbBox *box,
                          GSList       *uris)
{
  g_return_if_fail (GIMP_IS_THUMB_BOX (box));

  if (box->uris)
    {
      g_slist_foreach (box->uris, (GFunc) g_free, NULL);
      g_slist_free (box->uris);
      box->uris = NULL;
    }

  box->uris = uris;
}


/*  private functions  */

static gboolean
gimp_thumb_box_ebox_button_press (GtkWidget      *widget,
                                  GdkEventButton *bevent,
                                  GimpThumbBox   *box)
{
  gimp_thumb_box_thumbnail_clicked (widget, bevent->state, box);

  return TRUE;
}

static void
gimp_thumb_box_thumbnail_clicked (GtkWidget       *widget,
                                  GdkModifierType  state,
                                  GimpThumbBox    *box)
{
  gimp_thumb_box_create_thumbnails (box,
                                    (state & GDK_CONTROL_MASK) ? TRUE : FALSE);
}

static void
gimp_thumb_box_imagefile_info_changed (GimpImagefile *imagefile,
                                       GimpThumbBox  *box)
{
  gtk_label_set_text (GTK_LABEL (box->info),
                      gimp_imagefile_get_desc_string (imagefile));
}

static void
gimp_thumb_box_thumb_state_notify (GimpThumbnail *thumb,
                                   GParamSpec    *pspec,
                                   GimpThumbBox  *box)
{
  if (box->idle_id)
    return;

  if (thumb->image_state == GIMP_THUMB_STATE_REMOTE)
    return;

  switch (thumb->thumb_state)
    {
    case GIMP_THUMB_STATE_NOT_FOUND:
    case GIMP_THUMB_STATE_OLD:
      box->idle_id =
        g_idle_add_full (G_PRIORITY_LOW,
                         (GSourceFunc) gimp_thumb_box_auto_thumbnail,
                         box, NULL);
      break;

    default:
      break;
    }
}

static void
gimp_thumb_box_create_thumbnails (GimpThumbBox *box,
                                  gboolean      force)
{
  Gimp           *gimp     = box->imagefile->gimp;
  GimpProgress   *progress = GIMP_PROGRESS (box);
  GimpFileDialog *dialog   = NULL;
  GtkWidget      *toplevel;
  GSList         *list;
  gint            n_uris;
  gint            i;

  if (gimp->config->thumbnail_size == GIMP_THUMBNAIL_SIZE_NONE)
    return;

  toplevel = gtk_widget_get_toplevel (GTK_WIDGET (box));

  if (GIMP_IS_FILE_DIALOG (toplevel))
    dialog = GIMP_FILE_DIALOG (toplevel);

  gimp_set_busy (gimp);

  if (dialog)
    gimp_file_dialog_set_sensitive (dialog, FALSE);
  else
    gtk_widget_set_sensitive (toplevel, FALSE);

  if (box->uris)
    {
      gtk_widget_hide (box->info);
      gtk_widget_show (box->progress);
    }

  n_uris = g_slist_length (box->uris);

  if (n_uris > 1)
    {
      gchar *str;

      gimp_progress_start (GIMP_PROGRESS (box), "", TRUE);

      progress = gimp_sub_progress_new (GIMP_PROGRESS (box));

      gimp_sub_progress_set_step (GIMP_SUB_PROGRESS (progress), 0, n_uris);

      for (list = box->uris->next, i = 1;
           list;
           list = g_slist_next (list), i++)
        {
          str = g_strdup_printf (_("Thumbnail %d of %d"), i, n_uris);
          gtk_progress_bar_set_text (GTK_PROGRESS_BAR (box->progress), str);
          g_free (str);

          gimp_progress_set_value (progress, 0.0);

          while (gtk_events_pending ())
            gtk_main_iteration ();

          gimp_thumb_box_create_thumbnail (box,
                                           list->data,
                                           gimp->config->thumbnail_size,
                                           force,
                                           progress);

          if (dialog && dialog->canceled)
            goto canceled;

          gimp_sub_progress_set_step (GIMP_SUB_PROGRESS (progress), i, n_uris);
        }

      str = g_strdup_printf (_("Thumbnail %d of %d"), n_uris, n_uris);
      gtk_progress_bar_set_text (GTK_PROGRESS_BAR (box->progress), str);
      g_free (str);

      gimp_progress_set_value (progress, 0.0);

      while (gtk_events_pending ())
        gtk_main_iteration ();
    }

  if (box->uris)
    {
      gimp_thumb_box_create_thumbnail (box,
                                       box->uris->data,
                                       gimp->config->thumbnail_size,
                                       force,
                                       progress);

      gimp_progress_set_value (progress, 1.0);
    }

 canceled:

  if (n_uris > 1)
    {
      g_object_unref (progress);

      gimp_progress_end (GIMP_PROGRESS (box));
      gtk_progress_bar_set_text (GTK_PROGRESS_BAR (box->progress), "");
    }

  if (box->uris)
    {
      gtk_widget_hide (box->progress);
      gtk_widget_show (box->info);
    }

  if (dialog)
    gimp_file_dialog_set_sensitive (dialog, TRUE);
  else
    gtk_widget_set_sensitive (toplevel, TRUE);

  gimp_unset_busy (gimp);
}

static void
gimp_thumb_box_create_thumbnail (GimpThumbBox      *box,
                                 const gchar       *uri,
                                 GimpThumbnailSize  size,
                                 gboolean           force,
                                 GimpProgress      *progress)
{
  gchar         *filename = file_utils_filename_from_uri (uri);
  GimpThumbnail *thumb;
  gchar         *basename;

  if (filename)
    {
      gboolean regular = g_file_test (filename, G_FILE_TEST_IS_REGULAR);

      g_free (filename);

      if (! regular)
        return;
    }

  thumb = box->imagefile->thumbnail;

  basename = file_utils_uri_display_basename (uri);
  gtk_label_set_text (GTK_LABEL (box->filename), basename);
  g_free (basename);

  gimp_object_set_name (GIMP_OBJECT (box->imagefile), uri);

  if (force ||
      (gimp_thumbnail_peek_thumb (thumb, size) < GIMP_THUMB_STATE_FAILED &&
       ! gimp_thumbnail_has_failed (thumb)))
    {
      gimp_imagefile_create_thumbnail (box->imagefile, box->context,
                                       progress,
                                       size,
                                       !force);
    }
}

static gboolean
gimp_thumb_box_auto_thumbnail (GimpThumbBox *box)
{
  Gimp          *gimp  = box->imagefile->gimp;
  GimpThumbnail *thumb = box->imagefile->thumbnail;
  const gchar   *uri   = gimp_object_get_name (GIMP_OBJECT (box->imagefile));

  box->idle_id = 0;

  if (thumb->image_state == GIMP_THUMB_STATE_NOT_FOUND)
    return FALSE;

  switch (thumb->thumb_state)
    {
    case GIMP_THUMB_STATE_NOT_FOUND:
    case GIMP_THUMB_STATE_OLD:
      if (thumb->image_filesize < gimp->config->thumbnail_filesize_limit &&
          ! gimp_thumbnail_has_failed (thumb)                            &&
          file_procedure_find_by_extension (gimp->plug_in_manager->load_procs,
                                            uri))
        {
          if (thumb->image_filesize > 0)
            {
              gchar *size;
              gchar *text;

              size = gimp_memsize_to_string (thumb->image_filesize);
              text = g_strdup_printf ("%s\n%s",
                                      size, _("Creating preview..."));

              gtk_label_set_text (GTK_LABEL (box->info), text);

              g_free (text);
              g_free (size);
            }
          else
            {
              gtk_label_set_text (GTK_LABEL (box->info),
                                  _("Creating preview..."));
            }

          gimp_imagefile_create_thumbnail_weak (box->imagefile, box->context,
                                                GIMP_PROGRESS (box),
                                                gimp->config->thumbnail_size,
                                                TRUE);
        }
      break;

    default:
      break;
    }

  return FALSE;
}
