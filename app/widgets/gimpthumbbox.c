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

#include <gtk/gtk.h>

#include "libgimpthumb/gimpthumb.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "config/gimpcoreconfig.h"

#include "core/gimp.h"
#include "core/gimpimagefile.h"

#include "file/file-utils.h"

#include "gimppreview.h"
#include "gimpthumbbox.h"
#include "gimpwidgets-utils.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   gimp_thumb_box_class_init (GimpThumbBoxClass *klass);
static void   gimp_thumb_box_init       (GimpThumbBox      *box);

static void   gimp_thumb_box_finalize   (GObject           *object);

static void   gimp_thumb_box_style_set  (GtkWidget         *widget,
                                         GtkStyle          *prev_style);

static gboolean  gimp_thumb_box_ebox_button_press (GtkWidget         *widget,
                                                   GdkEventButton    *bevent,
                                                   GimpThumbBox      *box);
static void gimp_thumb_box_thumbnail_clicked      (GtkWidget         *widget,
                                                   GdkModifierType    state,
                                                   GimpThumbBox      *box);
static void gimp_thumb_box_imagefile_info_changed (GimpImagefile     *imagefile,
                                                   GimpThumbBox      *box);
static void gimp_thumb_box_create_thumbnails      (GimpThumbBox      *box,
                                                   gboolean           always_create);
static void gimp_thumb_box_create_thumbnail       (GimpThumbBox      *box,
                                                   const gchar       *uri,
                                                   GimpThumbnailSize  size,
                                                   gboolean           always_create);


/*  private variables  */

static GtkFrameClass *parent_class = NULL;


GType
gimp_thumb_box_get_type (void)
{
  static GType box_type = 0;

  if (! box_type)
    {
      static const GTypeInfo box_info =
      {
        sizeof (GimpThumbBoxClass),
        (GBaseInitFunc)     NULL,
        (GBaseFinalizeFunc) NULL,
	(GClassInitFunc)    gimp_thumb_box_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpThumbBox),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) gimp_thumb_box_init,
      };

      box_type = g_type_register_static (GTK_TYPE_FRAME,
                                         "GimpThumbBox",
                                         &box_info, 0);
    }

  return box_type;
}

static void
gimp_thumb_box_class_init (GimpThumbBoxClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize  = gimp_thumb_box_finalize;

  widget_class->style_set = gimp_thumb_box_style_set;
}

static void
gimp_thumb_box_init (GimpThumbBox *box)
{
  gtk_frame_set_shadow_type (GTK_FRAME (box), GTK_SHADOW_IN);
}

static void
gimp_thumb_box_finalize (GObject *object)
{
  GimpThumbBox *box = GIMP_THUMB_BOX (object);

  gimp_thumb_box_set_uris (box, NULL);

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


/*  public functions  */

GtkWidget *
gimp_thumb_box_new (Gimp *gimp)
{
  GimpThumbBox   *box;
  GtkWidget      *vbox;
  GtkWidget      *vbox2;
  GtkWidget      *ebox;
  GtkWidget      *hbox;
  GtkWidget      *button;
  GtkWidget      *label;
  gchar          *str;
  GtkRequisition  info_requisition;
  GtkRequisition  progress_requisition;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  box = g_object_new (GIMP_TYPE_THUMB_BOX, NULL);

  ebox = gtk_event_box_new ();
  gtk_container_add (GTK_CONTAINER (box), ebox);
  gtk_widget_show (ebox);

  g_signal_connect (ebox, "button_press_event",
                    G_CALLBACK (gimp_thumb_box_ebox_button_press),
                    box);

  str = g_strdup_printf (_("Click to update preview\n"
                           "%s  Click to force update even "
                           "if preview is up-to-date"),
                         gimp_get_mod_name_control ());

  gimp_help_set_help_data (ebox, str, NULL);

  g_free (str);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (ebox), vbox);
  gtk_widget_show (vbox);

  button = gtk_button_new ();
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  label = gtk_label_new_with_mnemonic (_("_Preview"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_container_add (GTK_CONTAINER (button), label);
  gtk_widget_show (label);

  g_signal_connect (button, "button_press_event",
                    G_CALLBACK (gtk_true),
                    NULL);
  g_signal_connect (button, "button_release_event",
                    G_CALLBACK (gtk_true),
                    NULL);
  g_signal_connect (button, "enter_notify_event",
                    G_CALLBACK (gtk_true),
                    NULL);
  g_signal_connect (button, "leave_notify_event",
                    G_CALLBACK (gtk_true),
                    NULL);

  vbox2 = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (vbox2), 2);
  gtk_container_add (GTK_CONTAINER (vbox), vbox2);
  gtk_widget_show (vbox2);

  hbox = gtk_hbox_new (TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  box->imagefile = gimp_imagefile_new (gimp, NULL);

  g_signal_connect (box->imagefile, "info_changed",
                    G_CALLBACK (gimp_thumb_box_imagefile_info_changed),
                    box);

  box->preview = gimp_preview_new (GIMP_VIEWABLE (box->imagefile),
                                   gimp->config->thumbnail_size, 0, FALSE);
  gtk_box_pack_start (GTK_BOX (hbox), box->preview, TRUE, FALSE, 10);
  gtk_widget_show (box->preview);

  gtk_label_set_mnemonic_widget (GTK_LABEL (label), box->preview);

  g_signal_connect (box->preview, "clicked",
                    G_CALLBACK (gimp_thumb_box_thumbnail_clicked),
                    box);

  box->filename = gtk_label_new (_("No Selection"));
  gtk_box_pack_start (GTK_BOX (vbox2), box->filename, FALSE, FALSE, 0);
  gtk_widget_show (box->filename);

  box->info = gtk_label_new (" \n \n ");
  gtk_misc_set_alignment (GTK_MISC (box->info), 0.5, 0.0);
  gtk_label_set_justify (GTK_LABEL (box->info), GTK_JUSTIFY_CENTER);
  gtk_box_pack_start (GTK_BOX (vbox2), box->info, FALSE, FALSE, 0);
  gtk_widget_show (box->info);

  box->progress = gtk_progress_bar_new ();
  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (box->progress), "foo");
  gtk_box_pack_end (GTK_BOX (vbox2), box->progress, FALSE, FALSE, 0);
  /* don't gtk_widget_show (box->progress); */

  /* eek */
  gtk_widget_size_request (box->info,     &info_requisition);
  gtk_widget_size_request (box->progress, &progress_requisition);

  gtk_widget_set_size_request (box->info,
                               progress_requisition.width,
                               info_requisition.height);
  gtk_widget_set_size_request (box->filename,
                               progress_requisition.width, -1);

  return GTK_WIDGET (box);
}

void
gimp_thumb_box_set_uri (GimpThumbBox *box,
                        const gchar  *uri)
{
  g_return_if_fail (GIMP_IS_THUMB_BOX (box));

  gimp_object_set_name (GIMP_OBJECT (box->imagefile), uri);

  if (uri)
    {
      gchar *basename;

      basename = file_utils_uri_to_utf8_basename (uri);
      gtk_label_set_text (GTK_LABEL (box->filename), basename);
      g_free (basename);
    }
  else
    {
      gtk_label_set_text (GTK_LABEL (box->filename), _("No Selection"));
    }

  gtk_widget_set_sensitive (GTK_WIDGET (box), uri != NULL);
  gimp_imagefile_update (box->imagefile);
}

void
gimp_thumb_box_set_uris (GimpThumbBox *box,
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
gimp_thumb_box_create_thumbnails (GimpThumbBox *box,
                                  gboolean      always_create)
{
  Gimp *gimp = box->imagefile->gimp;

  if (gimp->config->thumbnail_size != GIMP_THUMBNAIL_SIZE_NONE &&
      gimp->config->layer_previews)
    {
      GSList *list;
      gint    n_uris;
      gint    i;

      gimp_set_busy (gimp);
      gtk_widget_set_sensitive (gtk_widget_get_toplevel (GTK_WIDGET (box)),
                                FALSE);

      n_uris = g_slist_length (box->uris);

      if (n_uris > 1)
        {
          gchar *str;

          gtk_progress_bar_set_text (GTK_PROGRESS_BAR (box->progress), NULL);
          gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (box->progress), 0.0);

          gtk_widget_hide (box->info);
          gtk_widget_show (box->progress);

          for (list = box->uris->next, i = 0;
               list;
               list = g_slist_next (list), i++)
            {
              str = g_strdup_printf (_("Thumbnail %d of %d"), i, n_uris);
              gtk_progress_bar_set_text (GTK_PROGRESS_BAR (box->progress), str);
              g_free (str);

              while (g_main_context_pending (NULL))
                g_main_context_iteration (NULL, FALSE);

              gimp_thumb_box_create_thumbnail (box,
                                               list->data,
                                               gimp->config->thumbnail_size,
                                               always_create);

              gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (box->progress),
                                             (gdouble) i /
                                             (gdouble) n_uris);

              while (g_main_context_pending (NULL))
                g_main_context_iteration (NULL, FALSE);
            }

          str = g_strdup_printf (_("Thumbnail %d of %d"), n_uris, n_uris);
          gtk_progress_bar_set_text (GTK_PROGRESS_BAR (box->progress), str);
          g_free (str);

          while (g_main_context_pending (NULL))
            g_main_context_iteration (NULL, FALSE);
        }

      if (box->uris)
        {
          gimp_thumb_box_create_thumbnail (box,
                                           box->uris->data,
                                           gimp->config->thumbnail_size,
                                           always_create);

          if (n_uris > 1)
            {
              gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (box->progress),
                                             1.0);

              while (g_main_context_pending (NULL))
                g_main_context_iteration (NULL, FALSE);
            }
        }

      if (n_uris > 1)
        {
          gtk_widget_hide (box->progress);
          gtk_widget_show (box->info);
        }

      gtk_widget_set_sensitive (gtk_widget_get_toplevel (GTK_WIDGET (box)),
                                                         TRUE);
      gimp_unset_busy (gimp);
    }
}

static void
gimp_thumb_box_create_thumbnail (GimpThumbBox      *box,
                                 const gchar       *uri,
                                 GimpThumbnailSize  size,
                                 gboolean           always_create)
{
  gchar *filename = g_filename_from_uri (uri, NULL, NULL);

  if (filename && g_file_test (filename, G_FILE_TEST_IS_REGULAR))
    {
      gchar *basename = file_utils_uri_to_utf8_basename (uri);

      gtk_label_set_text (GTK_LABEL (box->filename), basename);
      g_free (basename);

      gimp_object_set_name (GIMP_OBJECT (box->imagefile), uri);

      if (always_create ||
          gimp_thumbnail_peek_thumb (box->imagefile->thumbnail, size)
          < GIMP_THUMB_STATE_FAILED)
        {
          gimp_imagefile_create_thumbnail (box->imagefile, size);
        }
    }

  g_free (filename);
}
