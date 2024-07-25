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

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpbuffer.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimpimage-new.h"
#include "core/gimpimage-undo.h"
#include "core/gimplayer.h"
#include "core/gimplayermask.h"
#include "core/gimptoolinfo.h"

#include "file/file-open.h"

#include "gimpdnd.h"
#include "gimptoolbox.h"
#include "gimptoolbox-dnd.h"
#include "gimpwidgets-utils.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   gimp_toolbox_drop_uri_list  (GtkWidget       *widget,
                                           gint             x,
                                           gint             y,
                                           GList           *uri_list,
                                           gpointer         data);
static void   gimp_toolbox_drop_drawable  (GtkWidget       *widget,
                                           gint             x,
                                           gint             y,
                                           GimpViewable    *viewable,
                                           gpointer         data);
static void   gimp_toolbox_drop_tool      (GtkWidget       *widget,
                                           gint             x,
                                           gint             y,
                                           GimpViewable    *viewable,
                                           gpointer         data);
static void   gimp_toolbox_drop_buffer    (GtkWidget       *widget,
                                           gint             x,
                                           gint             y,
                                           GimpViewable    *viewable,
                                           gpointer         data);
static void   gimp_toolbox_drop_component (GtkWidget       *widget,
                                           gint             x,
                                           gint             y,
                                           GimpImage       *image,
                                           GimpChannelType  component,
                                           gpointer         data);
static void   gimp_toolbox_drop_pixbuf    (GtkWidget       *widget,
                                           gint             x,
                                           gint             y,
                                           GdkPixbuf       *pixbuf,
                                           gpointer         data);


/*  public functions  */

void
gimp_toolbox_dnd_init (GimpToolbox *toolbox,
                       GtkWidget   *vbox)
{
  GimpContext *context = NULL;

  g_return_if_fail (GIMP_IS_TOOLBOX (toolbox));
  g_return_if_fail (GTK_IS_BOX (vbox));

  context = gimp_toolbox_get_context (toolbox);

  /* Before calling any dnd helper functions, setup the drag
   * destination manually since we want to handle all drag events
   * manually, otherwise we would not be able to give the drag handler
   * a chance to handle drag events
   */
  gtk_drag_dest_set (vbox,
                     0, NULL, 0,
                     GDK_ACTION_COPY | GDK_ACTION_MOVE);

  gimp_dnd_viewable_dest_add  (vbox,
                               GIMP_TYPE_LAYER,
                               gimp_toolbox_drop_drawable,
                               context);
  gimp_dnd_viewable_dest_add  (vbox,
                               GIMP_TYPE_LAYER_MASK,
                               gimp_toolbox_drop_drawable,
                               context);
  gimp_dnd_viewable_dest_add  (vbox,
                               GIMP_TYPE_CHANNEL,
                               gimp_toolbox_drop_drawable,
                               context);
  gimp_dnd_viewable_dest_add  (vbox,
                               GIMP_TYPE_TOOL_INFO,
                               gimp_toolbox_drop_tool,
                               context);
  gimp_dnd_viewable_dest_add  (vbox,
                               GIMP_TYPE_BUFFER,
                               gimp_toolbox_drop_buffer,
                               context);
  gimp_dnd_component_dest_add (vbox,
                               gimp_toolbox_drop_component,
                               context);
  gimp_dnd_uri_list_dest_add  (vbox,
                               gimp_toolbox_drop_uri_list,
                               context);
  gimp_dnd_pixbuf_dest_add    (vbox,
                               gimp_toolbox_drop_pixbuf,
                               context);
}


/*  private functions  */

static void
gimp_toolbox_drop_uri_list (GtkWidget *widget,
                            gint       x,
                            gint       y,
                            GList     *uri_list,
                            gpointer   data)
{
  GimpContext *context = GIMP_CONTEXT (data);
  GList       *list;

  if (context->gimp->busy)
    return;

  for (list = uri_list; list; list = g_list_next (list))
    {
      GFile             *file = g_file_new_for_uri (list->data);
      GimpPDBStatusType  status;
      GError            *error = NULL;

      file_open_with_display (context->gimp, context, NULL,
                              file, FALSE,
                              G_OBJECT (gimp_widget_get_monitor (widget)),
                              &status, &error);

      if (status != GIMP_PDB_CANCEL && status != GIMP_PDB_SUCCESS)
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
          gimp_message (context->gimp, G_OBJECT (widget), GIMP_MESSAGE_ERROR,
                        _("Opening '%s' failed:\n\n%s"),
                        gimp_file_get_utf8_name (file), error->message);
          g_clear_error (&error);
        }

      g_object_unref (file);
    }
}

static void
gimp_toolbox_drop_drawable (GtkWidget    *widget,
                            gint          x,
                            gint          y,
                            GimpViewable *viewable,
                            gpointer      data)
{
  GimpContext *context = GIMP_CONTEXT (data);
  GimpImage   *new_image;

  if (context->gimp->busy)
    return;

  new_image = gimp_image_new_from_drawable (context->gimp,
                                            GIMP_DRAWABLE (viewable));
  gimp_create_display (context->gimp, new_image, gimp_unit_pixel (), 1.0,
                       G_OBJECT (gimp_widget_get_monitor (widget)));
  g_object_unref (new_image);
}

static void
gimp_toolbox_drop_tool (GtkWidget    *widget,
                        gint          x,
                        gint          y,
                        GimpViewable *viewable,
                        gpointer      data)
{
  GimpContext *context = GIMP_CONTEXT (data);

  if (context->gimp->busy)
    return;

  gimp_context_set_tool (context, GIMP_TOOL_INFO (viewable));
}

static void
gimp_toolbox_drop_buffer (GtkWidget    *widget,
                          gint          x,
                          gint          y,
                          GimpViewable *viewable,
                          gpointer      data)
{
  GimpContext *context = GIMP_CONTEXT (data);
  GimpImage   *image;

  if (context->gimp->busy)
    return;

  image = gimp_image_new_from_buffer (context->gimp,
                                      GIMP_BUFFER (viewable));
  gimp_create_display (image->gimp, image, gimp_unit_pixel (), 1.0,
                       G_OBJECT (gimp_widget_get_monitor (widget)));
  g_object_unref (image);
}

static void
gimp_toolbox_drop_component (GtkWidget       *widget,
                             gint             x,
                             gint             y,
                             GimpImage       *image,
                             GimpChannelType  component,
                             gpointer         data)
{
  GimpContext *context = GIMP_CONTEXT (data);
  GimpImage   *new_image;

  if (context->gimp->busy)
    return;

  new_image = gimp_image_new_from_component (context->gimp,
                                             image, component);
  gimp_create_display (new_image->gimp, new_image, gimp_unit_pixel (), 1.0,
                       G_OBJECT (gimp_widget_get_monitor (widget)));
  g_object_unref (new_image);
}

static void
gimp_toolbox_drop_pixbuf (GtkWidget *widget,
                          gint       x,
                          gint       y,
                          GdkPixbuf *pixbuf,
                          gpointer   data)
{
  GimpContext   *context = GIMP_CONTEXT (data);
  GimpImage     *new_image;

  if (context->gimp->busy)
    return;

  new_image = gimp_image_new_from_pixbuf (context->gimp, pixbuf,
                                          _("Dropped Buffer"));
  gimp_create_display (new_image->gimp, new_image, gimp_unit_pixel (), 1.0,
                       G_OBJECT (gimp_widget_get_monitor (widget)));
  g_object_unref (new_image);
}
