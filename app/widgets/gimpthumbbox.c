/* LIGMA - The GNU Image Manipulation Program
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

#include "libligmabase/ligmabase.h"
#include "libligmathumb/ligmathumb.h"
#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "config/ligmacoreconfig.h"

#include "core/ligma.h"
#include "core/ligmacontext.h"
#include "core/ligmaimagefile.h"
#include "core/ligmaprogress.h"
#include "core/ligmasubprogress.h"

#include "plug-in/ligmapluginmanager-file.h"

#include "ligmafiledialog.h" /* eek */
#include "ligmathumbbox.h"
#include "ligmaview.h"
#include "ligmaviewrenderer-frame.h"
#include "ligmawidgets-utils.h"

#include "ligma-intl.h"


/*  local function prototypes  */

static void     ligma_thumb_box_progress_iface_init (LigmaProgressInterface *iface);

static void     ligma_thumb_box_dispose            (GObject           *object);
static void     ligma_thumb_box_finalize           (GObject           *object);

static LigmaProgress *
                ligma_thumb_box_progress_start     (LigmaProgress      *progress,
                                                   gboolean           cancellable,
                                                   const gchar       *message);
static void     ligma_thumb_box_progress_end       (LigmaProgress      *progress);
static gboolean ligma_thumb_box_progress_is_active (LigmaProgress      *progress);
static void     ligma_thumb_box_progress_set_value (LigmaProgress      *progress,
                                                   gdouble            percentage);
static gdouble  ligma_thumb_box_progress_get_value (LigmaProgress      *progress);
static void     ligma_thumb_box_progress_pulse     (LigmaProgress      *progress);

static gboolean ligma_thumb_box_progress_message   (LigmaProgress      *progress,
                                                   Ligma              *ligma,
                                                   LigmaMessageSeverity  severity,
                                                   const gchar       *domain,
                                                   const gchar       *message);

static gboolean ligma_thumb_box_ebox_button_press  (GtkWidget         *widget,
                                                   GdkEventButton    *bevent,
                                                   LigmaThumbBox      *box);
static void ligma_thumb_box_thumbnail_clicked      (GtkWidget         *widget,
                                                   GdkModifierType    state,
                                                   LigmaThumbBox      *box);
static void ligma_thumb_box_imagefile_info_changed (LigmaImagefile     *imagefile,
                                                   LigmaThumbBox      *box);
static void ligma_thumb_box_thumb_state_notify     (LigmaThumbnail     *thumb,
                                                   GParamSpec        *pspec,
                                                   LigmaThumbBox      *box);
static void ligma_thumb_box_create_thumbnails      (LigmaThumbBox      *box,
                                                   gboolean           force);
static void ligma_thumb_box_create_thumbnail       (LigmaThumbBox      *box,
                                                   GFile             *file,
                                                   LigmaThumbnailSize  size,
                                                   gboolean           force,
                                                   LigmaProgress      *progress);
static gboolean ligma_thumb_box_auto_thumbnail     (LigmaThumbBox      *box);


G_DEFINE_TYPE_WITH_CODE (LigmaThumbBox, ligma_thumb_box, GTK_TYPE_FRAME,
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_PROGRESS,
                                                ligma_thumb_box_progress_iface_init))

#define parent_class ligma_thumb_box_parent_class


static void
ligma_thumb_box_class_init (LigmaThumbBoxClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose  = ligma_thumb_box_dispose;
  object_class->finalize = ligma_thumb_box_finalize;

  gtk_widget_class_set_css_name (widget_class, "treeview");
}

static void
ligma_thumb_box_init (LigmaThumbBox *box)
{
  gtk_frame_set_shadow_type (GTK_FRAME (box), GTK_SHADOW_IN);

  box->idle_id = 0;
}

static void
ligma_thumb_box_progress_iface_init (LigmaProgressInterface *iface)
{
  iface->start     = ligma_thumb_box_progress_start;
  iface->end       = ligma_thumb_box_progress_end;
  iface->is_active = ligma_thumb_box_progress_is_active;
  iface->set_value = ligma_thumb_box_progress_set_value;
  iface->get_value = ligma_thumb_box_progress_get_value;
  iface->pulse     = ligma_thumb_box_progress_pulse;
  iface->message   = ligma_thumb_box_progress_message;
}

static void
ligma_thumb_box_dispose (GObject *object)
{
  LigmaThumbBox *box = LIGMA_THUMB_BOX (object);

  if (box->idle_id)
    {
      g_source_remove (box->idle_id);
      box->idle_id = 0;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);

  box->progress = NULL;
}

static void
ligma_thumb_box_finalize (GObject *object)
{
  LigmaThumbBox *box = LIGMA_THUMB_BOX (object);

  ligma_thumb_box_take_files (box, NULL);

  g_clear_object (&box->imagefile);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static LigmaProgress *
ligma_thumb_box_progress_start (LigmaProgress *progress,
                               gboolean      cancellable,
                               const gchar  *message)
{
  LigmaThumbBox *box = LIGMA_THUMB_BOX (progress);

  if (! box->progress)
    return NULL;

  if (! box->progress_active)
    {
      GtkProgressBar *bar = GTK_PROGRESS_BAR (box->progress);
      GtkWidget      *toplevel;

      gtk_progress_bar_set_fraction (bar, 0.0);

      box->progress_active = TRUE;

      toplevel = gtk_widget_get_toplevel (GTK_WIDGET (box));

      if (LIGMA_IS_FILE_DIALOG (toplevel))
        gtk_dialog_set_response_sensitive (GTK_DIALOG (toplevel),
                                           GTK_RESPONSE_CANCEL, cancellable);

      return progress;
    }

  return NULL;
}

static void
ligma_thumb_box_progress_end (LigmaProgress *progress)
{
  if (ligma_thumb_box_progress_is_active (progress))
    {
      LigmaThumbBox   *box = LIGMA_THUMB_BOX (progress);
      GtkProgressBar *bar = GTK_PROGRESS_BAR (box->progress);

      gtk_progress_bar_set_fraction (bar, 0.0);

      box->progress_active = FALSE;
    }
}

static gboolean
ligma_thumb_box_progress_is_active (LigmaProgress *progress)
{
  LigmaThumbBox *box = LIGMA_THUMB_BOX (progress);

  return (box->progress && box->progress_active);
}

static void
ligma_thumb_box_progress_set_value (LigmaProgress *progress,
                                   gdouble       percentage)
{
  if (ligma_thumb_box_progress_is_active (progress))
    {
      LigmaThumbBox   *box = LIGMA_THUMB_BOX (progress);
      GtkProgressBar *bar = GTK_PROGRESS_BAR (box->progress);

      gtk_progress_bar_set_fraction (bar, percentage);
    }
}

static gdouble
ligma_thumb_box_progress_get_value (LigmaProgress *progress)
{
  if (ligma_thumb_box_progress_is_active (progress))
    {
      LigmaThumbBox   *box = LIGMA_THUMB_BOX (progress);
      GtkProgressBar *bar = GTK_PROGRESS_BAR (box->progress);

      return gtk_progress_bar_get_fraction (bar);
    }

  return 0.0;
}

static void
ligma_thumb_box_progress_pulse (LigmaProgress *progress)
{
  if (ligma_thumb_box_progress_is_active (progress))
    {
      LigmaThumbBox   *box = LIGMA_THUMB_BOX (progress);
      GtkProgressBar *bar = GTK_PROGRESS_BAR (box->progress);

      gtk_progress_bar_pulse (bar);
    }
}

static gboolean
ligma_thumb_box_progress_message (LigmaProgress        *progress,
                                 Ligma                *ligma,
                                 LigmaMessageSeverity  severity,
                                 const gchar         *domain,
                                 const gchar         *message)
{
  /*  LigmaThumbBox never shows any messages  */

  return TRUE;
}


/*  stupid LigmaHeader class just so we get a "header" CSS node  */

#define LIGMA_TYPE_HEADER (ligma_header_get_type ())

typedef struct _GtkBox      LigmaHeader;
typedef struct _GtkBoxClass LigmaHeaderClass;

static GType ligma_header_get_type (void) G_GNUC_CONST;

G_DEFINE_TYPE (LigmaHeader, ligma_header, GTK_TYPE_BOX)

static void
ligma_header_class_init (LigmaHeaderClass *klass)
{
  gtk_widget_class_set_css_name (GTK_WIDGET_CLASS (klass), "header");
}

static void
ligma_header_init (LigmaHeader *header)
{
  gtk_orientable_set_orientation (GTK_ORIENTABLE (header),
                                  GTK_ORIENTATION_VERTICAL);
}


/*  public functions  */

GtkWidget *
ligma_thumb_box_new (LigmaContext *context)
{
  LigmaThumbBox   *box;
  GtkWidget      *vbox;
  GtkWidget      *vbox2;
  GtkWidget      *ebox;
  GtkWidget      *button;
  GtkWidget      *label;
  gchar          *str;
  gint            h, v;

  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);

  box = g_object_new (LIGMA_TYPE_THUMB_BOX, NULL);

  gtk_style_context_add_class (gtk_widget_get_style_context (GTK_WIDGET (box)),
                               GTK_STYLE_CLASS_VIEW);

  box->context = context;

  ebox = gtk_event_box_new ();
  gtk_container_add (GTK_CONTAINER (box), ebox);
  gtk_widget_show (ebox);

  g_signal_connect (ebox, "button-press-event",
                    G_CALLBACK (ligma_thumb_box_ebox_button_press),
                    box);

  str = g_strdup_printf (_("Click to update preview\n"
                           "%s-Click to force update even "
                           "if preview is up-to-date"),
                         ligma_get_mod_string (ligma_get_toggle_behavior_mask ()));

  ligma_help_set_help_data (ebox, str, NULL);

  g_free (str);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add (GTK_CONTAINER (ebox), vbox);
  gtk_widget_show (vbox);

  vbox2 = g_object_new (LIGMA_TYPE_HEADER, NULL);
  gtk_box_pack_start (GTK_BOX (vbox), vbox2, FALSE, FALSE, 0);
  gtk_widget_show (vbox2);

  button = gtk_button_new ();
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
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
  gtk_container_set_border_width (GTK_CONTAINER (vbox2), 4);
  gtk_box_pack_start (GTK_BOX (vbox), vbox2, TRUE, TRUE, 0);
  gtk_widget_show (vbox2);

  box->imagefile = ligma_imagefile_new (context->ligma, NULL);

  g_signal_connect (box->imagefile, "info-changed",
                    G_CALLBACK (ligma_thumb_box_imagefile_info_changed),
                    box);

  g_signal_connect (ligma_imagefile_get_thumbnail (box->imagefile),
                    "notify::thumb-state",
                    G_CALLBACK (ligma_thumb_box_thumb_state_notify),
                    box);

  ligma_view_renderer_get_frame_size (&h, &v);

  box->preview = ligma_view_new (context,
                                LIGMA_VIEWABLE (box->imagefile),
                                /* add padding for the shadow frame */
                                context->ligma->config->thumbnail_size +
                                MAX (h, v),
                                0, FALSE);

  gtk_style_context_add_class (gtk_widget_get_style_context (box->preview),
                               GTK_STYLE_CLASS_VIEW);
  gtk_box_pack_start (GTK_BOX (vbox2), box->preview, FALSE, FALSE, 0);
  gtk_widget_show (box->preview);

  gtk_label_set_mnemonic_widget (GTK_LABEL (label), box->preview);

  g_signal_connect (box->preview, "clicked",
                    G_CALLBACK (ligma_thumb_box_thumbnail_clicked),
                    box);

  box->filename = gtk_label_new (_("No selection"));
  gtk_label_set_max_width_chars (GTK_LABEL (box->filename), 1);
  gtk_label_set_ellipsize (GTK_LABEL (box->filename), PANGO_ELLIPSIZE_MIDDLE);
  gtk_label_set_justify (GTK_LABEL (box->filename), GTK_JUSTIFY_CENTER);
  ligma_label_set_attributes (GTK_LABEL (box->filename),
                             PANGO_ATTR_STYLE, PANGO_STYLE_OBLIQUE,
                             -1);
  gtk_box_pack_start (GTK_BOX (vbox2), box->filename, FALSE, FALSE, 0);
  gtk_widget_show (box->filename);

  box->info = gtk_label_new (" \n \n \n ");
  gtk_label_set_justify (GTK_LABEL (box->info), GTK_JUSTIFY_CENTER);
  gtk_label_set_line_wrap (GTK_LABEL (box->info), TRUE);
  ligma_label_set_attributes (GTK_LABEL (box->info),
                             PANGO_ATTR_SCALE, PANGO_SCALE_SMALL,
                             -1);
  gtk_box_pack_start (GTK_BOX (vbox2), box->info, FALSE, FALSE, 0);
  gtk_widget_show (box->info);

  box->progress = gtk_progress_bar_new ();
  gtk_widget_set_halign (box->progress, GTK_ALIGN_FILL);
  gtk_progress_bar_set_show_text (GTK_PROGRESS_BAR (box->progress), TRUE);
  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (box->progress), "Fog");
  gtk_box_pack_end (GTK_BOX (vbox2), box->progress, FALSE, FALSE, 0);
  /* don't gtk_widget_show (box->progress); */

  gtk_widget_set_size_request (GTK_WIDGET (box),
                               MAX ((gint) LIGMA_THUMB_SIZE_NORMAL,
                                    (gint) context->ligma->config->thumbnail_size) +
                               2 * MAX (h, v),
                               -1);

  return GTK_WIDGET (box);
}

void
ligma_thumb_box_take_file (LigmaThumbBox *box,
                          GFile        *file)
{
  g_return_if_fail (LIGMA_IS_THUMB_BOX (box));
  g_return_if_fail (file == NULL || G_IS_FILE (file));

  if (box->idle_id)
    {
      g_source_remove (box->idle_id);
      box->idle_id = 0;
    }

  ligma_imagefile_set_file (box->imagefile, file);

  if (file)
    {
      gchar *basename = g_path_get_basename (ligma_file_get_utf8_name (file));
      gtk_label_set_text (GTK_LABEL (box->filename), basename);
      g_free (basename);
    }
  else
    {
      gtk_label_set_text (GTK_LABEL (box->filename), _("No selection"));
    }

  gtk_widget_set_sensitive (GTK_WIDGET (box), file != NULL);
  ligma_imagefile_update (box->imagefile);
}

void
ligma_thumb_box_take_files (LigmaThumbBox *box,
                           GSList       *files)
{
  g_return_if_fail (LIGMA_IS_THUMB_BOX (box));

  if (box->files)
    {
      g_slist_free_full (box->files, (GDestroyNotify) g_object_unref);
      box->files = NULL;
    }

  box->files = files;
}


/*  private functions  */

static gboolean
ligma_thumb_box_ebox_button_press (GtkWidget      *widget,
                                  GdkEventButton *bevent,
                                  LigmaThumbBox   *box)
{
  ligma_thumb_box_thumbnail_clicked (widget, bevent->state, box);

  return TRUE;
}

static void
ligma_thumb_box_thumbnail_clicked (GtkWidget       *widget,
                                  GdkModifierType  state,
                                  LigmaThumbBox    *box)
{
  ligma_thumb_box_create_thumbnails (box,
                                    (state & ligma_get_toggle_behavior_mask ()) ?
                                    TRUE : FALSE);
}

static void
ligma_thumb_box_imagefile_info_changed (LigmaImagefile *imagefile,
                                       LigmaThumbBox  *box)
{
  gtk_label_set_text (GTK_LABEL (box->info),
                      ligma_imagefile_get_desc_string (imagefile));
}

static void
ligma_thumb_box_thumb_state_notify (LigmaThumbnail *thumb,
                                   GParamSpec    *pspec,
                                   LigmaThumbBox  *box)
{
  if (box->idle_id)
    return;

  if (thumb->image_state == LIGMA_THUMB_STATE_REMOTE)
    return;

  switch (thumb->thumb_state)
    {
    case LIGMA_THUMB_STATE_NOT_FOUND:
    case LIGMA_THUMB_STATE_OLD:
      box->idle_id =
        g_idle_add_full (G_PRIORITY_LOW,
                         (GSourceFunc) ligma_thumb_box_auto_thumbnail,
                         box, NULL);
      break;

    default:
      break;
    }
}

static void
ligma_thumb_box_create_thumbnails (LigmaThumbBox *box,
                                  gboolean      force)
{
  Ligma           *ligma     = box->context->ligma;
  LigmaProgress   *progress = LIGMA_PROGRESS (box);
  LigmaFileDialog *dialog   = NULL;
  GtkWidget      *toplevel;
  GSList         *list;
  gint            n_files;
  gint            i;

  if (ligma->config->thumbnail_size == LIGMA_THUMBNAIL_SIZE_NONE)
    return;

  toplevel = gtk_widget_get_toplevel (GTK_WIDGET (box));

  if (LIGMA_IS_FILE_DIALOG (toplevel))
    dialog = LIGMA_FILE_DIALOG (toplevel);

  ligma_set_busy (ligma);

  if (dialog)
    ligma_file_dialog_set_sensitive (dialog, FALSE);
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

      ligma_progress_start (LIGMA_PROGRESS (box), TRUE, "%s", "");

      progress = ligma_sub_progress_new (LIGMA_PROGRESS (box));

      ligma_sub_progress_set_step (LIGMA_SUB_PROGRESS (progress), 0, n_files);

      for (list = box->files->next, i = 1;
           list;
           list = g_slist_next (list), i++)
        {
          str = g_strdup_printf (_("Thumbnail %d of %d"), i, n_files);
          gtk_progress_bar_set_text (GTK_PROGRESS_BAR (box->progress), str);
          g_free (str);

          ligma_progress_set_value (progress, 0.0);

          while (g_main_context_pending (NULL))
            g_main_context_iteration (NULL, FALSE);

          ligma_thumb_box_create_thumbnail (box,
                                           list->data,
                                           ligma->config->thumbnail_size,
                                           force,
                                           progress);

          if (dialog && dialog->canceled)
            goto canceled;

          ligma_sub_progress_set_step (LIGMA_SUB_PROGRESS (progress), i, n_files);
        }

      str = g_strdup_printf (_("Thumbnail %d of %d"), n_files, n_files);
      gtk_progress_bar_set_text (GTK_PROGRESS_BAR (box->progress), str);
      g_free (str);

      ligma_progress_set_value (progress, 0.0);

      while (g_main_context_pending (NULL))
        g_main_context_iteration (NULL, FALSE);
    }

  if (box->files)
    {
      ligma_thumb_box_create_thumbnail (box,
                                       box->files->data,
                                       ligma->config->thumbnail_size,
                                       force,
                                       progress);

      ligma_progress_set_value (progress, 1.0);
    }

 canceled:

  if (n_files > 1)
    {
      g_object_unref (progress);

      ligma_progress_end (LIGMA_PROGRESS (box));
      gtk_progress_bar_set_text (GTK_PROGRESS_BAR (box->progress), "");
    }

  if (box->files)
    {
      gtk_widget_hide (box->progress);
      gtk_widget_show (box->info);
    }

  if (dialog)
    ligma_file_dialog_set_sensitive (dialog, TRUE);
  else
    gtk_widget_set_sensitive (toplevel, TRUE);

  ligma_unset_busy (ligma);
}

static void
ligma_thumb_box_create_thumbnail (LigmaThumbBox      *box,
                                 GFile             *file,
                                 LigmaThumbnailSize  size,
                                 gboolean           force,
                                 LigmaProgress      *progress)
{
  LigmaThumbnail *thumb = ligma_imagefile_get_thumbnail (box->imagefile);
  gchar         *basename;

  basename = g_path_get_basename (ligma_file_get_utf8_name (file));
  gtk_label_set_text (GTK_LABEL (box->filename), basename);
  g_free (basename);

  ligma_imagefile_set_file (box->imagefile, file);

  if (force ||
      (ligma_thumbnail_peek_thumb (thumb, (LigmaThumbSize) size) < LIGMA_THUMB_STATE_FAILED &&
       ! ligma_thumbnail_has_failed (thumb)))
    {
      GError *error = NULL;

      if (! ligma_imagefile_create_thumbnail (box->imagefile, box->context,
                                             progress,
                                             size, ! force, &error))
        {
          ligma_message_literal (box->context->ligma,
                                G_OBJECT (progress), LIGMA_MESSAGE_ERROR,
                                error->message);
          g_clear_error (&error);
        }
    }
}

static gboolean
ligma_thumb_box_auto_thumbnail (LigmaThumbBox *box)
{
  Ligma          *ligma  = box->context->ligma;
  LigmaThumbnail *thumb = ligma_imagefile_get_thumbnail (box->imagefile);
  GFile         *file  = ligma_imagefile_get_file (box->imagefile);

  box->idle_id = 0;

  if (thumb->image_state == LIGMA_THUMB_STATE_NOT_FOUND)
    return FALSE;

  switch (thumb->thumb_state)
    {
    case LIGMA_THUMB_STATE_NOT_FOUND:
    case LIGMA_THUMB_STATE_OLD:
      if (thumb->image_filesize < ligma->config->thumbnail_filesize_limit &&
          ! ligma_thumbnail_has_failed (thumb)                            &&
          ligma_plug_in_manager_file_procedure_find_by_extension (ligma->plug_in_manager,
                                                                 LIGMA_FILE_PROCEDURE_GROUP_OPEN,
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

          ligma_imagefile_create_thumbnail_weak (box->imagefile, box->context,
                                                LIGMA_PROGRESS (box),
                                                ligma->config->thumbnail_size,
                                                TRUE);
        }
      break;

    default:
      break;
    }

  return FALSE;
}
