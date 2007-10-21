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

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "dialogs-types.h"

#include "core/gimp-edit.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimpimage-undo.h"
#include "core/gimpdrawable.h"
#include "core/gimpdrawableundo.h"
#include "core/gimpundostack.h"

#include "widgets/gimppropwidgets.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpviewabledialog.h"

#include "fade-dialog.h"

#include "gimp-intl.h"


typedef struct
{
  GimpImage            *image;
  GimpDrawable         *drawable;
  GimpContext          *context;

  gboolean              applied;
  GimpLayerModeEffects  orig_paint_mode;
  gdouble               orig_opacity;
} FadeDialog;


static void   fade_dialog_response        (GtkWidget  *dialog,
                                           gint        response_id,
                                           FadeDialog *private);

static void   fade_dialog_context_changed (FadeDialog *private);
static void   fade_dialog_free            (FadeDialog *private);


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
  GtkWidget        *table;
  GtkWidget        *menu;
  GtkWidget        *label;
  gchar            *title;
  gint              table_row = 0;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GTK_IS_WIDGET (parent), NULL);

  undo = GIMP_DRAWABLE_UNDO (gimp_image_undo_get_fadeable (image));

  if (! (undo && undo->src2_tiles))
    return NULL;

  item      = GIMP_ITEM_UNDO (undo)->item;
  drawable  = GIMP_DRAWABLE (item);

  private = g_slice_new0 (FadeDialog);

  private->image           = image;
  private->drawable        = drawable;
  private->context         = gimp_context_new (image->gimp,
                                               "fade-dialog", NULL);
  private->applied         = FALSE;
  private->orig_paint_mode = undo->paint_mode;
  private->orig_opacity    = undo->opacity;

  g_object_set (private->context,
                /* Use Normal mode instead of Replace.
                 * This is not quite correct but the dialog is somewhat
                 * difficult to use otherwise.
                 */
                "paint-mode", (undo->paint_mode == GIMP_REPLACE_MODE ?
                               GIMP_NORMAL_MODE : undo->paint_mode),
                "opacity",    undo->opacity,
                NULL);

  title = g_strdup_printf (_("Fade %s"),
                           gimp_object_get_name (GIMP_OBJECT (undo)));


  dialog = gimp_viewable_dialog_new (GIMP_VIEWABLE (drawable),
                                     private->context,
                                     title, "gimp-edit-fade",
                                     GTK_STOCK_UNDO, title,
                                     parent,
                                     gimp_standard_help_func,
                                     GIMP_HELP_EDIT_FADE,

                                     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                     _("_Fade"),       GTK_RESPONSE_OK,

                                     NULL);

  g_free (title);

  gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  g_object_weak_ref (G_OBJECT (dialog),
                     (GWeakNotify) fade_dialog_free, private);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (fade_dialog_response),
                    private);

  main_vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), main_vbox);
  gtk_widget_show (main_vbox);

  table = gtk_table_new (3, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (main_vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /*  the paint mode menu  */
  menu = gimp_prop_paint_mode_menu_new (G_OBJECT (private->context),
                                        "paint-mode", TRUE, TRUE);
  label = gimp_table_attach_aligned (GTK_TABLE (table), 0, table_row++,
                                     _("_Mode:"), 0.0, 0.5,
                                     menu, 2, FALSE);

  /*  the opacity scale  */
  gimp_prop_opacity_entry_new (G_OBJECT (private->context), "opacity",
                               GTK_TABLE (table), 0, table_row++,
                               _("_Opacity:"));

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
  if (gimp_edit_fade (private->image, private->context))
    {
      private->applied = TRUE;
      gimp_image_flush (private->image);
    }
}

static void
fade_dialog_free (FadeDialog *private)
{
  g_slice_free (FadeDialog, private);
}
