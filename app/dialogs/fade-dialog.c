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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "dialogs-types.h"

#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimpimage-fade.h"
#include "core/gimpimage-undo.h"
#include "core/gimpdrawable.h"
#include "core/gimpdrawableundo.h"
#include "core/gimpundostack.h"

#include "widgets/gimplayermodebox.h"
#include "widgets/gimppropwidgets.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpviewabledialog.h"

#include "fade-dialog.h"

#include "gimp-intl.h"


typedef struct
{
  GimpImage     *image;
  GimpDrawable  *drawable;
  GimpContext   *context;

  gboolean       applied;
  GimpLayerMode  orig_paint_mode;
  gdouble        orig_opacity;
} FadeDialog;


/*  local function prototypes  */

static void   fade_dialog_free            (FadeDialog *private);
static void   fade_dialog_response        (GtkWidget  *dialog,
                                           gint        response_id,
                                           FadeDialog *private);
static void   fade_dialog_context_changed (FadeDialog *private);


/*  public functions  */

GtkWidget *
fade_dialog_new (GimpImage *image,
                 GtkWidget *parent)
{
  FadeDialog       *private;
  GimpDrawableUndo *undo;
  GimpDrawable     *drawable;
  GimpItem         *item;

  GtkWidget        *dialog;
  GtkWidget        *main_vbox;
  GtkWidget        *menu;
  GtkWidget        *scale;
  gchar            *title;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GTK_IS_WIDGET (parent), NULL);

  undo = GIMP_DRAWABLE_UNDO (gimp_image_undo_get_fadeable (image));

  if (! (undo && undo->applied_buffer))
    return NULL;

  item     = GIMP_ITEM_UNDO (undo)->item;
  drawable = GIMP_DRAWABLE (item);

  private = g_slice_new0 (FadeDialog);

  private->image           = image;
  private->drawable        = drawable;
  private->context         = gimp_context_new (image->gimp,
                                               "fade-dialog", NULL);
  private->applied         = FALSE;
  private->orig_paint_mode = undo->paint_mode;
  private->orig_opacity    = undo->opacity;

  g_object_set (private->context,
                "paint-mode", undo->paint_mode,
                "opacity",    undo->opacity,
                NULL);

  title = g_strdup_printf (_("Fade %s"), gimp_object_get_name (undo));

  dialog = gimp_viewable_dialog_new (GIMP_VIEWABLE (drawable),
                                     private->context,
                                     title, "gimp-edit-fade",
                                     "edit-undo", title,
                                     parent,
                                     gimp_standard_help_func,
                                     GIMP_HELP_EDIT_FADE,

                                     _("_Cancel"), GTK_RESPONSE_CANCEL,
                                     _("_Fade"),   GTK_RESPONSE_OK,

                                     NULL);

  g_free (title);

  gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);

  g_object_weak_ref (G_OBJECT (dialog),
                     (GWeakNotify) fade_dialog_free, private);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (fade_dialog_response),
                    private);

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      main_vbox, TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);

  /*  the paint mode menu  */
  menu = gimp_prop_layer_mode_box_new (G_OBJECT (private->context),
                                       "paint-mode",
                                       GIMP_LAYER_MODE_CONTEXT_FADE);
  gimp_layer_mode_box_set_label (GIMP_LAYER_MODE_BOX (menu), _("Mode"));
  gtk_box_pack_start (GTK_BOX (main_vbox), menu, FALSE, FALSE, 0);
  gtk_widget_show (menu);

  /*  the opacity scale  */
  scale = gimp_prop_spin_scale_new (G_OBJECT (private->context),
                                    "opacity",
                                    _("Opacity"),
                                    0.01, 0.1, 2);
  gimp_prop_widget_set_factor (scale, 100, 1.0, 10.0, 1);
  gtk_box_pack_start (GTK_BOX (main_vbox), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  g_signal_connect_swapped (private->context, "paint-mode-changed",
                            G_CALLBACK (fade_dialog_context_changed),
                            private);
  g_signal_connect_swapped (private->context, "opacity-changed",
                            G_CALLBACK (fade_dialog_context_changed),
                            private);

  return dialog;
}


/*  private functions  */

static void
fade_dialog_free (FadeDialog *private)
{
  g_slice_free (FadeDialog, private);
}

static void
fade_dialog_response (GtkWidget  *dialog,
                      gint        response_id,
                      FadeDialog *private)
{
  g_signal_handlers_disconnect_by_func (private->context,
                                        fade_dialog_context_changed,
                                        private);

  if (response_id != GTK_RESPONSE_OK && private->applied)
    {
      g_object_set (private->context,
                    "paint-mode", private->orig_paint_mode,
                    "opacity",    private->orig_opacity,
                    NULL);

      fade_dialog_context_changed (private);
    }

  g_object_unref (private->context);
  gtk_widget_destroy (dialog);
}

static void
fade_dialog_context_changed (FadeDialog *private)
{
  if (gimp_image_fade (private->image, private->context))
    {
      private->applied = TRUE;
      gimp_image_flush (private->image);
    }
}
