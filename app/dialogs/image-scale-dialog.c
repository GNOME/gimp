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
#include "libligmawidgets/ligmawidgets.h"

#include "dialogs-types.h"

#include "config/ligmaguiconfig.h"

#include "core/ligma.h"
#include "core/ligmacontext.h"
#include "core/ligmaimage.h"
#include "core/ligmaimage-scale.h"

#include "widgets/ligmahelp-ids.h"
#include "widgets/ligmamessagebox.h"
#include "widgets/ligmamessagedialog.h"

#include "scale-dialog.h"

#include "image-scale-dialog.h"

#include "ligma-intl.h"


typedef struct
{
  GtkWidget             *dialog;

  LigmaImage             *image;

  gint                   width;
  gint                   height;
  LigmaUnit               unit;
  LigmaInterpolationType  interpolation;
  gdouble                xresolution;
  gdouble                yresolution;
  LigmaUnit               resolution_unit;

  LigmaScaleCallback      callback;
  gpointer               user_data;
} ImageScaleDialog;


/*  local function prototypes  */

static void        image_scale_dialog_free      (ImageScaleDialog      *private);
static void        image_scale_callback         (GtkWidget             *widget,
                                                 LigmaViewable          *viewable,
                                                 gint                   width,
                                                 gint                   height,
                                                 LigmaUnit               unit,
                                                 LigmaInterpolationType  interpolation,
                                                 gdouble                xresolution,
                                                 gdouble                yresolution,
                                                 LigmaUnit               resolution_unit,
                                                 gpointer               data);

static GtkWidget * image_scale_confirm_dialog   (ImageScaleDialog      *private);
static void        image_scale_confirm_large    (ImageScaleDialog      *private,
                                                 gint64                 new_memsize,
                                                 gint64                 max_memsize);
static void        image_scale_confirm_small    (ImageScaleDialog      *private);
static void        image_scale_confirm_response (GtkWidget             *widget,
                                                 gint                   response_id,
                                                 ImageScaleDialog      *private);


/*  public functions  */

GtkWidget *
image_scale_dialog_new (LigmaImage             *image,
                        LigmaContext           *context,
                        GtkWidget             *parent,
                        LigmaUnit               unit,
                        LigmaInterpolationType  interpolation,
                        LigmaScaleCallback      callback,
                        gpointer               user_data)
{
  ImageScaleDialog *private;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (callback != NULL, NULL);

  private = g_slice_new0 (ImageScaleDialog);

  private->image     = image;
  private->callback  = callback;
  private->user_data = user_data;

  private->dialog = scale_dialog_new (LIGMA_VIEWABLE (image), context,
                                      C_("dialog-title", "Scale Image"),
                                      "ligma-image-scale",
                                      parent,
                                      ligma_standard_help_func,
                                      LIGMA_HELP_IMAGE_SCALE,
                                      unit,
                                      interpolation,
                                      image_scale_callback,
                                      private);

  g_object_weak_ref (G_OBJECT (private->dialog),
                     (GWeakNotify) image_scale_dialog_free, private);

  return private->dialog;
}


/*  private functions  */

static void
image_scale_dialog_free (ImageScaleDialog *private)
{
  g_slice_free (ImageScaleDialog, private);
}

static void
image_scale_callback (GtkWidget             *widget,
                      LigmaViewable          *viewable,
                      gint                   width,
                      gint                   height,
                      LigmaUnit               unit,
                      LigmaInterpolationType  interpolation,
                      gdouble                xresolution,
                      gdouble                yresolution,
                      LigmaUnit               resolution_unit,
                      gpointer               data)
{
  ImageScaleDialog        *private = data;
  LigmaImage               *image   = LIGMA_IMAGE (viewable);
  LigmaImageScaleCheckType  scale_check;
  gint64                   max_memsize;
  gint64                   new_memsize;

  private->width           = width;
  private->height          = height;
  private->unit            = unit;
  private->interpolation   = interpolation;
  private->xresolution     = xresolution;
  private->yresolution     = yresolution;
  private->resolution_unit = resolution_unit;

  gtk_widget_set_sensitive (widget, FALSE);

  max_memsize = LIGMA_GUI_CONFIG (image->ligma->config)->max_new_image_size;

  scale_check = ligma_image_scale_check (image,
                                        width, height, max_memsize,
                                        &new_memsize);
  switch (scale_check)
    {
    case LIGMA_IMAGE_SCALE_TOO_BIG:
      image_scale_confirm_large (private, new_memsize, max_memsize);
      break;

    case LIGMA_IMAGE_SCALE_TOO_SMALL:
      image_scale_confirm_small (private);
      break;

    case LIGMA_IMAGE_SCALE_OK:
      private->callback (private->dialog,
                         LIGMA_VIEWABLE (private->image),
                         private->width,
                         private->height,
                         private->unit,
                         private->interpolation,
                         private->xresolution,
                         private->yresolution,
                         private->resolution_unit,
                         private->user_data);
      break;
    }
}

static GtkWidget *
image_scale_confirm_dialog (ImageScaleDialog *private)
{
  GtkWidget *widget;

  widget = ligma_message_dialog_new (_("Confirm Scaling"),
                                    LIGMA_ICON_DIALOG_WARNING,
                                    private->dialog,
                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                    ligma_standard_help_func,
                                    LIGMA_HELP_IMAGE_SCALE_WARNING,

                                    _("_Cancel"), GTK_RESPONSE_CANCEL,
                                    _("_Scale"),  GTK_RESPONSE_OK,

                                    NULL);

  ligma_dialog_set_alternative_button_order (GTK_DIALOG (widget),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  g_signal_connect (widget, "response",
                    G_CALLBACK (image_scale_confirm_response),
                    private);

  return widget;
}

static void
image_scale_confirm_large (ImageScaleDialog *private,
                           gint64            new_memsize,
                           gint64            max_memsize)
{
  GtkWidget *widget = image_scale_confirm_dialog (private);
  gchar     *size;

  size = g_format_size (new_memsize);
  ligma_message_box_set_primary_text (LIGMA_MESSAGE_DIALOG (widget)->box,
                                     _("You are trying to create an image "
                                       "with a size of %s."), size);
  g_free (size);

  size = g_format_size (max_memsize);
  ligma_message_box_set_text (LIGMA_MESSAGE_DIALOG (widget)->box,
                             _("Scaling the image to the chosen size will "
                               "make it use more memory than what is "
                               "configured as \"Maximum Image Size\" in the "
                               "Preferences dialog (currently %s)."), size);
  g_free (size);

  gtk_widget_show (widget);
}

static void
image_scale_confirm_small (ImageScaleDialog *private)
{
  GtkWidget *widget = image_scale_confirm_dialog (private);

  ligma_message_box_set_primary_text (LIGMA_MESSAGE_DIALOG (widget)->box,
                                     _("Scaling the image to the chosen size "
                                       "will shrink some layers completely "
                                       "away."));
  ligma_message_box_set_text (LIGMA_MESSAGE_DIALOG (widget)->box,
                             _("Is this what you want to do?"));

  gtk_widget_show (widget);
}

static void
image_scale_confirm_response (GtkWidget        *widget,
                              gint              response_id,
                              ImageScaleDialog *private)
{
  gtk_widget_destroy (widget);

  if (response_id == GTK_RESPONSE_OK)
    {
      private->callback (private->dialog,
                         LIGMA_VIEWABLE (private->image),
                         private->width,
                         private->height,
                         private->unit,
                         private->interpolation,
                         private->xresolution,
                         private->yresolution,
                         private->resolution_unit,
                         private->user_data);
    }
  else
    {
      gtk_widget_set_sensitive (private->dialog, TRUE);
    }
}
