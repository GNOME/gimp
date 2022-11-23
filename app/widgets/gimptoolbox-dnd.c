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

#include "widgets-types.h"

#include "core/ligma.h"
#include "core/ligmabuffer.h"
#include "core/ligmacontext.h"
#include "core/ligmaimage.h"
#include "core/ligmaimage-new.h"
#include "core/ligmaimage-undo.h"
#include "core/ligmalayer.h"
#include "core/ligmalayermask.h"
#include "core/ligmatoolinfo.h"

#include "file/file-open.h"

#include "ligmadnd.h"
#include "ligmatoolbox.h"
#include "ligmatoolbox-dnd.h"
#include "ligmawidgets-utils.h"

#include "ligma-intl.h"


/*  local function prototypes  */

static void   ligma_toolbox_drop_uri_list  (GtkWidget       *widget,
                                           gint             x,
                                           gint             y,
                                           GList           *uri_list,
                                           gpointer         data);
static void   ligma_toolbox_drop_drawable  (GtkWidget       *widget,
                                           gint             x,
                                           gint             y,
                                           LigmaViewable    *viewable,
                                           gpointer         data);
static void   ligma_toolbox_drop_tool      (GtkWidget       *widget,
                                           gint             x,
                                           gint             y,
                                           LigmaViewable    *viewable,
                                           gpointer         data);
static void   ligma_toolbox_drop_buffer    (GtkWidget       *widget,
                                           gint             x,
                                           gint             y,
                                           LigmaViewable    *viewable,
                                           gpointer         data);
static void   ligma_toolbox_drop_component (GtkWidget       *widget,
                                           gint             x,
                                           gint             y,
                                           LigmaImage       *image,
                                           LigmaChannelType  component,
                                           gpointer         data);
static void   ligma_toolbox_drop_pixbuf    (GtkWidget       *widget,
                                           gint             x,
                                           gint             y,
                                           GdkPixbuf       *pixbuf,
                                           gpointer         data);


/*  public functions  */

void
ligma_toolbox_dnd_init (LigmaToolbox *toolbox,
                       GtkWidget   *vbox)
{
  LigmaContext *context = NULL;

  g_return_if_fail (LIGMA_IS_TOOLBOX (toolbox));
  g_return_if_fail (GTK_IS_BOX (vbox));

  context = ligma_toolbox_get_context (toolbox);

  /* Before calling any dnd helper functions, setup the drag
   * destination manually since we want to handle all drag events
   * manually, otherwise we would not be able to give the drag handler
   * a chance to handle drag events
   */
  gtk_drag_dest_set (vbox,
                     0, NULL, 0,
                     GDK_ACTION_COPY | GDK_ACTION_MOVE);

  ligma_dnd_viewable_dest_add  (vbox,
                               LIGMA_TYPE_LAYER,
                               ligma_toolbox_drop_drawable,
                               context);
  ligma_dnd_viewable_dest_add  (vbox,
                               LIGMA_TYPE_LAYER_MASK,
                               ligma_toolbox_drop_drawable,
                               context);
  ligma_dnd_viewable_dest_add  (vbox,
                               LIGMA_TYPE_CHANNEL,
                               ligma_toolbox_drop_drawable,
                               context);
  ligma_dnd_viewable_dest_add  (vbox,
                               LIGMA_TYPE_TOOL_INFO,
                               ligma_toolbox_drop_tool,
                               context);
  ligma_dnd_viewable_dest_add  (vbox,
                               LIGMA_TYPE_BUFFER,
                               ligma_toolbox_drop_buffer,
                               context);
  ligma_dnd_component_dest_add (vbox,
                               ligma_toolbox_drop_component,
                               context);
  ligma_dnd_uri_list_dest_add  (vbox,
                               ligma_toolbox_drop_uri_list,
                               context);
  ligma_dnd_pixbuf_dest_add    (vbox,
                               ligma_toolbox_drop_pixbuf,
                               context);
}


/*  private functions  */

static void
ligma_toolbox_drop_uri_list (GtkWidget *widget,
                            gint       x,
                            gint       y,
                            GList     *uri_list,
                            gpointer   data)
{
  LigmaContext *context = LIGMA_CONTEXT (data);
  GList       *list;

  if (context->ligma->busy)
    return;

  for (list = uri_list; list; list = g_list_next (list))
    {
      GFile             *file = g_file_new_for_uri (list->data);
      LigmaPDBStatusType  status;
      GError            *error = NULL;

      file_open_with_display (context->ligma, context, NULL,
                              file, FALSE,
                              G_OBJECT (ligma_widget_get_monitor (widget)),
                              &status, &error);

      if (status != LIGMA_PDB_CANCEL && status != LIGMA_PDB_SUCCESS)
        {
          /* file_open_image() took care of always having a filled error when
           * the status is neither CANCEL nor SUCCESS (and to transform a
           * wrongful success without a returned image into an EXECUTION_ERROR).
           *
           * In some case, we may also have a SUCCESS without an image, when the
           * file procedure is a `generic_file_proc` (e.g. for a .gex extension
           * file). So we should not rely on having a returned image or not.
           * Once again, sanitizing the returned status is handled by
           * file_open_image().
           */
          ligma_message (context->ligma, G_OBJECT (widget), LIGMA_MESSAGE_ERROR,
                        _("Opening '%s' failed:\n\n%s"),
                        ligma_file_get_utf8_name (file), error->message);
          g_clear_error (&error);
        }

      g_object_unref (file);
    }
}

static void
ligma_toolbox_drop_drawable (GtkWidget    *widget,
                            gint          x,
                            gint          y,
                            LigmaViewable *viewable,
                            gpointer      data)
{
  LigmaContext *context = LIGMA_CONTEXT (data);
  LigmaImage   *new_image;

  if (context->ligma->busy)
    return;

  new_image = ligma_image_new_from_drawable (context->ligma,
                                            LIGMA_DRAWABLE (viewable));
  ligma_create_display (context->ligma, new_image, LIGMA_UNIT_PIXEL, 1.0,
                       G_OBJECT (ligma_widget_get_monitor (widget)));
  g_object_unref (new_image);
}

static void
ligma_toolbox_drop_tool (GtkWidget    *widget,
                        gint          x,
                        gint          y,
                        LigmaViewable *viewable,
                        gpointer      data)
{
  LigmaContext *context = LIGMA_CONTEXT (data);

  if (context->ligma->busy)
    return;

  ligma_context_set_tool (context, LIGMA_TOOL_INFO (viewable));
}

static void
ligma_toolbox_drop_buffer (GtkWidget    *widget,
                          gint          x,
                          gint          y,
                          LigmaViewable *viewable,
                          gpointer      data)
{
  LigmaContext *context = LIGMA_CONTEXT (data);
  LigmaImage   *image;

  if (context->ligma->busy)
    return;

  image = ligma_image_new_from_buffer (context->ligma,
                                      LIGMA_BUFFER (viewable));
  ligma_create_display (image->ligma, image, LIGMA_UNIT_PIXEL, 1.0,
                       G_OBJECT (ligma_widget_get_monitor (widget)));
  g_object_unref (image);
}

static void
ligma_toolbox_drop_component (GtkWidget       *widget,
                             gint             x,
                             gint             y,
                             LigmaImage       *image,
                             LigmaChannelType  component,
                             gpointer         data)
{
  LigmaContext *context = LIGMA_CONTEXT (data);
  LigmaImage   *new_image;

  if (context->ligma->busy)
    return;

  new_image = ligma_image_new_from_component (context->ligma,
                                             image, component);
  ligma_create_display (new_image->ligma, new_image, LIGMA_UNIT_PIXEL, 1.0,
                       G_OBJECT (ligma_widget_get_monitor (widget)));
  g_object_unref (new_image);
}

static void
ligma_toolbox_drop_pixbuf (GtkWidget *widget,
                          gint       x,
                          gint       y,
                          GdkPixbuf *pixbuf,
                          gpointer   data)
{
  LigmaContext   *context = LIGMA_CONTEXT (data);
  LigmaImage     *new_image;

  if (context->ligma->busy)
    return;

  new_image = ligma_image_new_from_pixbuf (context->ligma, pixbuf,
                                          _("Dropped Buffer"));
  ligma_create_display (new_image->ligma, new_image, LIGMA_UNIT_PIXEL, 1.0,
                       G_OBJECT (ligma_widget_get_monitor (widget)));
  g_object_unref (new_image);
}
