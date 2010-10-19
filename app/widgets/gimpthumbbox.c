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
#include "libgimpthumb/gimpthumb.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "config/gimpcoreconfig.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpimagefile.h"
#include "core/gimpprogress.h"
#include "core/gimpsubprogress.h"

#include "plug-in/gimppluginmanager-file.h"

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
                                                   gboolean           cancellable,
                                                   const gchar       *message);
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
                                                   GFile             *file,
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

  object_class->dispose     = gimp_thumb_box_dispose;
  object_class->finalize    = gimp_thumb_box_finalize;

  widget_class->style_set   = gimp_thumb_box_style_set;
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

  box->progress = NULL;
}

static void
gimp_thumb_box_finalize (GObject *object)
{
  GimpThumbBox *box = GIMP_THUMB_BOX (object);

  gimp_thumb_box_take_files (box, NULL);

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
  GimpThumbBox *box   = GIMP_THUMB_BOX (widget);
  GtkStyle     *style = gtk_widget_get_style (widget);
  GtkWidget    *ebox;

  GTK_WIDGET_CLASS (parent_class)->style_set (widget, prev_style);

  gtk_widget_modify_bg (box->preview, GTK_STATE_NORMAL,
                        &style->base[GTK_STATE_NORMAL]);
  gtk_widget_modify_bg (box->preview, GTK_STATE_INSENSITIVE,
                        &style->base[GTK_STATE_NORMAL]);

  ebox = gtk_bin_get_child (GTK_BIN (widget));

  gtk_widget_modify_bg (ebox, GTK_STATE_NORMAL,
                        &style->base[GTK_STATE_NORMAL]);
  gtk_widget_modify_bg (ebox, GTK_STATE_INSENSITIVE,
                        &style->base[GTK_STATE_NORMAL]);
}

static GimpProgress *
gimp_thumb_box_progress_start (GimpProgress *progress,
                               gboolean      cancellable,
                               const gchar  *message)
{
  GimpThumbBox *box = GIMP_THUMB_BOX (progress);

  if (! box->progress)
    return NULL;

  if (! box->progress_active)
    {
      GtkProgressBar *bar = GTK_PROGRESS_BAR (box->progress);
      GtkWidget      *toplevel;

      gtk_progress_bar_set_fraction (bar, 0.0);

      box->progress_active = TRUE;

      toplevel = gtk_widget_get_toplevel (GTK_WIDGET (box));

      if (GIMP_IS_FILE_DIALOG (toplevel))
        gtk_dialog_set_response_sensitive (GTK_DIALOG (toplevel),
                                           GTK_RESPONSE_CANCEL, cancellable);

      return progress;
    }

  return NULL;
}

static void
gimp_thumb_box_progress_end (GimpProgress *progress)
{
  if (gimp_thumb_box_progress_is_active (progress))
    {
      GimpThumbBox   *box = GIMP_THUMB_BOX (progress);
      GtkProgressBar *bar = GTK_PROGRESS_BAR (box->progress);

      gtk_progress_bar_set_fraction (bar, 0.0);

      box->progress_active = FALSE;
    }
}

static gboolean
gimp_thumb_box_progress_is_active (GimpProgress *progress)
{
  GimpThumbBox *box = GIMP_THUMB_BOX (progress);

  return (box->progress && box->progress_active);
}

static void
gimp_thumb_box_progress_set_value (GimpProgress *progress,
                                   gdouble       percentage)
{
  if (gimp_thumb_box_progress_is_active (progress))
    {
      GimpThumbBox   *box = GIMP_THUMB_BOX (progress);
      GtkProgressBar *bar = GTK_PROGRESS_BAR (box->progress);

      gtk_progress_bar_set_fraction (bar, percentage);
    }
}

static gdouble
gimp_thumb_box_progress_get_value (GimpProgress *progress)
{
  if (gimp_thumb_box_progress_is_active (progress))
    {
      GimpThumbBox   *box = GIMP_THUMB_BOX (progress);
      GtkProgressBar *bar = GTK_PROGRESS_BAR (box->progress);

      return gtk_progress_bar_get_fraction (bar);
    }

  return 0.0;
}

static void
gimp_thumb_box_progress_pulse (GimpProgress *progress)
{
  if (gimp_thumb_box_progress_is_active (progress))
    {
      GimpThumbBox   *box = GIMP_THUMB_BOX (progress);
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
                           "%s-Click to force update even "
                           "if preview is up-to-date"),
                         gimp_get_mod_string (gimp_get_toggle_behavior_mask ()));

  gimp_help_set_help_data (ebox, str, NULL);

  g_free (str);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add (GTK_CONTAINER (ebox), vbox);
  gtk_widget_show (vbox);

  button = gtk_button_new ();
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  label = gtk_label_new_with_mnemonic (_("Pr_eview"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
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

  vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_set_border_width (GTK_CONTAINER (vbox2), 2);
  gtk_box_pack_start (GTK_BOX (vbox), vbox2, TRUE, TRUE, 0);
  gtk_widget_show (vbox2);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_set_homogeneous (GTK_BOX (hbox), TRUE);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  box->imagefile = gimp_imagefile_new (context->gimp, NULL);

  g_signal_connect (box->imagefile, "info-changed",
                    G_CALLBACK (gimp_thumb_box_imagefile_info_changed),
                    box);

  g_signal_connect (gimp_imagefile_get_thumbnail (box->imagefile),
                    "notify::thumb-state",
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
  gtk_label_set_justify (GTK_LABEL (box->info), GTK_JUSTIFY_CENTER);
  gtk_label_set_line_wrap (GTK_LABEL (box->info), TRUE);
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
  gtk_widget_get_preferred_size (box->info,     &info_requisition,     NULL);
  gtk_widget_get_preferred_size (box->progress, &progress_requisition, NULL);

  gtk_widget_set_size_request (box->info,
                               -1, info_requisition.height);
  gtk_widget_set_size_request (box->filename,
                               progress_requisition.width, -1);

  gtk_widget_set_size_request (box->progress,
                               -1, progress_requisition.height);
  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (box->progress), "");

  return GTK_WIDGET (box);
}

void
gimp_thumb_box_take_file (GimpThumbBox *box,
                          GFile        *file)
{
  g_return_if_fail (GIMP_IS_THUMB_BOX (box));
  g_return_if_fail (file == NULL || G_IS_FILE (file));

  if (box->idle_id)
    {
      g_source_remove (box->idle_id);
      box->idle_id = 0;
    }

  gimp_imagefile_set_file (box->imagefile, file);

  if (file)
    {
      gchar *basename = g_path_get_basename (gimp_file_get_utf8_name (file));
      gtk_label_set_text (GTK_LABEL (box->filename), basename);
      g_free (basename);
    }
  else
    {
      gtk_label_set_text (GTK_LABEL (box->filename), _("No selection"));
    }

  gtk_widget_set_sensitive (GTK_WIDGET (box), file != NULL);
  gimp_imagefile_update (box->imagefile);
}

void
gimp_thumb_box_take_files (GimpThumbBox *box,
                           GSList       *files)
{
  g_return_if_fail (GIMP_IS_THUMB_BOX (box));

  if (box->files)
    {
      g_slist_free_full (box->files, (GDestroyNotify) g_object_unref);
      box->files = NULL;
    }

  box->files = files;
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
                                    (state & gimp_get_toggle_behavior_mask ()) ?
                                    TRUE : FALSE);
}

static void
this_is_ugly (GtkWidget     *widget,
              GtkAllocation *allocation,
              gpointer       data)
{
  gtk_widget_queue_resize (widget);

  g_signal_handlers_disconnect_by_func (widget,
                                        this_is_ugly,
                                        data);
}

static void
gimp_thumb_box_imagefile_info_changed (GimpImagefile *imagefile,
                                       GimpThumbBox  *box)
{
  gtk_label_set_text (GTK_LABEL (box->info),
                      gimp_imagefile_get_desc_string (imagefile));

  g_signal_connect_after (box->info, "size-allocate",
                          G_CALLBACK (this_is_ugly),
                          "this too");
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
  Gimp           *gimp     = box->context->gimp;
  GimpProgress   *progress = GIMP_PROGRESS (box);
  GimpFileDialog *dialog   = NULL;
  GtkWidget      *toplevel;
  GSList         *list;
  gint            n_files;
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

  if (box->files)
    {
      gtk_widget_hide (box->info);
      gtk_widget_show (box->progress);
    }

  n_files = g_slist_length (box->files);

  if (n_files > 1)
    {
      gchar *str;

      gimp_progress_start (GIMP_PROGRESS (box), TRUE, "%s", "");

      progress = gimp_sub_progress_new (GIMP_PROGRESS (box));

      gimp_sub_progress_set_step (GIMP_SUB_PROGRESS (progress), 0, n_files);

      for (list = box->files->next, i = 1;
           list;
           list = g_slist_next (list), i++)
        {
          str = g_strdup_printf (_("Thumbnail %d of %d"), i, n_files);
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

          gimp_sub_progress_set_step (GIMP_SUB_PROGRESS (progress), i, n_files);
        }

      str = g_strdup_printf (_("Thumbnail %d of %d"), n_files, n_files);
      gtk_progress_bar_set_text (GTK_PROGRESS_BAR (box->progress), str);
      g_free (str);

      gimp_progress_set_value (progress, 0.0);

      while (gtk_events_pending ())
        gtk_main_iteration ();
    }

  if (box->files)
    {
      gimp_thumb_box_create_thumbnail (box,
                                       box->files->data,
                                       gimp->config->thumbnail_size,
                                       force,
                                       progress);

      gimp_progress_set_value (progress, 1.0);
    }

 canceled:

  if (n_files > 1)
    {
      g_object_unref (progress);

      gimp_progress_end (GIMP_PROGRESS (box));
      gtk_progress_bar_set_text (GTK_PROGRESS_BAR (box->progress), "");
    }

  if (box->files)
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
                                 GFile             *file,
                                 GimpThumbnailSize  size,
                                 gboolean           force,
                                 GimpProgress      *progress)
{
  GimpThumbnail *thumb = gimp_imagefile_get_thumbnail (box->imagefile);
  gchar         *basename;

  basename = g_path_get_basename (gimp_file_get_utf8_name (file));
  gtk_label_set_text (GTK_LABEL (box->filename), basename);
  g_free (basename);

  gimp_imagefile_set_file (box->imagefile, file);

  if (force ||
      (gimp_thumbnail_peek_thumb (thumb, (GimpThumbSize) size) < GIMP_THUMB_STATE_FAILED &&
       ! gimp_thumbnail_has_failed (thumb)))
    {
      GError *error = NULL;

      if (! gimp_imagefile_create_thumbnail (box->imagefile, box->context,
                                             progress,
                                             size, ! force, &error))
        {
          gimp_message_literal (box->context->gimp,
                                G_OBJECT (progress), GIMP_MESSAGE_ERROR,
                                error->message);
          g_clear_error (&error);
        }
    }
}

static gboolean
gimp_thumb_box_auto_thumbnail (GimpThumbBox *box)
{
  Gimp          *gimp  = box->context->gimp;
  GimpThumbnail *thumb = gimp_imagefile_get_thumbnail (box->imagefile);
  GFile         *file  = gimp_imagefile_get_file (box->imagefile);

  box->idle_id = 0;

  if (thumb->image_state == GIMP_THUMB_STATE_NOT_FOUND)
    return FALSE;

  switch (thumb->thumb_state)
    {
    case GIMP_THUMB_STATE_NOT_FOUND:
    case GIMP_THUMB_STATE_OLD:
      if (thumb->image_filesize < gimp->config->thumbnail_filesize_limit &&
          ! gimp_thumbnail_has_failed (thumb)                            &&
          gimp_plug_in_manager_file_procedure_find_by_extension (gimp->plug_in_manager,
                                                                 GIMP_FILE_PROCEDURE_GROUP_OPEN,
                                                                 file))
        {
          if (thumb->image_filesize > 0)
            {
              gchar *size;
              gchar *text;

              size = g_format_size (thumb->image_filesize);
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
