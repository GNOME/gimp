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

#include "libgimpwidgets/gimpwidgets.h"

#include "gui-types.h"

#include "core/gimp.h"
#include "core/gimp-edit.h"
#include "core/gimpbuffer.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimpimage-undo.h"
#include "core/gimptoolinfo.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-transform.h"

#include "widgets/gimpdock.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpdialogfactory.h"

#include "dialogs.h"
#include "edit-commands.h"
#include "stroke-dialog.h"

#include "gimp-intl.h"


#define return_if_no_display(gdisp,data) \
  if (GIMP_IS_DISPLAY (data)) \
    gdisp = data; \
  else if (GIMP_IS_GIMP (data)) \
    gdisp = gimp_context_get_display (gimp_get_user_context (GIMP (data))); \
  else if (GIMP_IS_DOCK (data)) \
    gdisp = gimp_context_get_display (((GimpDock *) data)->context); \
  else \
    gdisp = NULL; \
  if (! gdisp) \
    return

#define return_if_no_image(gimage,data) \
  if (GIMP_IS_DISPLAY (data)) \
    gimage = ((GimpDisplay *) data)->gimage; \
  else if (GIMP_IS_GIMP (data)) \
    gimage = gimp_context_get_image (gimp_get_user_context (GIMP (data))); \
  else if (GIMP_IS_DOCK (data)) \
    gimage = gimp_context_get_image (((GimpDock *) data)->context); \
  else \
    gimage = NULL; \
  if (! gimage) \
    return

#define return_if_no_drawable(gimage,drawable,data) \
  return_if_no_image (gimage, data); \
  drawable = gimp_image_active_drawable (gimage); \
  if (! drawable) \
    return;


/*  local function prototypes  */

static void   cut_named_buffer_callback  (GtkWidget   *widget,
                                          const gchar *name,
                                          gpointer     data);
static void   copy_named_buffer_callback (GtkWidget   *widget,
                                          const gchar *name,
                                          gpointer     data);


/*  public functions  */

void
edit_undo_cmd_callback (GtkWidget *widget,
                        gpointer   data)
{
  GimpImage *gimage;
  return_if_no_image (gimage, data);

  if (gimp_image_undo (gimage))
    gimp_image_flush (gimage);
}

void
edit_redo_cmd_callback (GtkWidget *widget,
                        gpointer   data)
{
  GimpImage *gimage;
  return_if_no_image (gimage, data);

  if (gimp_image_redo (gimage))
    gimp_image_flush (gimage);
}

void
edit_cut_cmd_callback (GtkWidget *widget,
                       gpointer   data)
{
  GimpImage    *gimage;
  GimpDrawable *drawable;
  return_if_no_drawable (gimage, drawable, data);

  if (gimp_edit_cut (gimage, drawable, gimp_get_user_context (gimage->gimp)))
    gimp_image_flush (gimage);
}

void
edit_copy_cmd_callback (GtkWidget *widget,
                        gpointer   data)
{
  GimpImage    *gimage;
  GimpDrawable *drawable;
  return_if_no_drawable (gimage, drawable, data);

  if (gimp_edit_copy (gimage, drawable, gimp_get_user_context (gimage->gimp)))
    gimp_image_flush (gimage);
}

static void
edit_paste (GimpDisplay *gdisp,
            gboolean     paste_into)
{
  if (gdisp->gimage->gimp->global_buffer)
    {
      GimpDisplayShell *shell;
      gint              x, y, width, height;

      shell = GIMP_DISPLAY_SHELL (gdisp->shell);

      gimp_display_shell_untransform_viewport (shell, &x, &y, &width, &height);

      if (gimp_edit_paste (gdisp->gimage,
                           gimp_image_active_drawable (gdisp->gimage),
                           gdisp->gimage->gimp->global_buffer,
                           paste_into, x, y, width, height))
	{
          gimp_image_flush (gdisp->gimage);
	}
    }
}

void
edit_paste_cmd_callback (GtkWidget *widget,
                         gpointer   data)
{
  GimpDisplay *gdisp;
  return_if_no_display (gdisp, data);

  edit_paste (gdisp, FALSE);
}

void
edit_paste_into_cmd_callback (GtkWidget *widget,
                              gpointer   data)
{
  GimpDisplay *gdisp;
  return_if_no_display (gdisp, data);

  edit_paste (gdisp, TRUE);
}

void
edit_paste_as_new_cmd_callback (GtkWidget *widget,
                                gpointer   data)
{
  GimpDisplay *gdisp;
  return_if_no_display (gdisp, data);

  if (gdisp->gimage->gimp->global_buffer)
    {
      gimp_edit_paste_as_new (gdisp->gimage->gimp,
                              gdisp->gimage,
                              gdisp->gimage->gimp->global_buffer);
    }
}

void
edit_named_cut_cmd_callback (GtkWidget *widget,
                             gpointer   data)
{
  GimpDisplay *gdisp;
  GtkWidget   *qbox;
  return_if_no_display (gdisp, data);

  qbox = gimp_query_string_box (_("Cut Named"),
                                gdisp->shell,
                                gimp_standard_help_func,
                                GIMP_HELP_BUFFER_CUT,
                                _("Enter a name for this buffer"),
                                NULL,
                                G_OBJECT (gdisp->gimage), "disconnect",
                                cut_named_buffer_callback, gdisp->gimage);
  gtk_widget_show (qbox);
}

void
edit_named_copy_cmd_callback (GtkWidget *widget,
                              gpointer   data)
{
  GimpDisplay *gdisp;
  GtkWidget   *qbox;
  return_if_no_display (gdisp, data);

  qbox = gimp_query_string_box (_("Copy Named"),
                                gdisp->shell,
                                gimp_standard_help_func,
                                GIMP_HELP_BUFFER_COPY,
                                _("Enter a name for this buffer"),
                                NULL,
                                G_OBJECT (gdisp->gimage), "disconnect",
                                copy_named_buffer_callback, gdisp->gimage);
  gtk_widget_show (qbox);
}

void
edit_named_paste_cmd_callback (GtkWidget *widget,
                               gpointer   data)
{
  gimp_dialog_factory_dialog_raise (global_dock_factory,
                                    gtk_widget_get_screen (widget),
                                    "gimp-buffer-list|gimp-buffer-grid", -1);
}

void
edit_clear_cmd_callback (GtkWidget *widget,
                         gpointer   data)
{
  GimpImage    *gimage;
  GimpDrawable *drawable;
  return_if_no_drawable (gimage, drawable, data);

  gimp_edit_clear (gimage, drawable, gimp_get_user_context (gimage->gimp));
  gimp_image_flush (gimage);
}

void
edit_fill_cmd_callback (GtkWidget *widget,
                        gpointer   data,
                        guint      action)
{
  GimpImage    *gimage;
  GimpDrawable *drawable;
  GimpFillType  fill_type;
  return_if_no_drawable (gimage, drawable, data);

  fill_type = (GimpFillType) action;

  gimp_edit_fill (gimage, drawable, gimp_get_user_context (gimage->gimp),
                  fill_type);
  gimp_image_flush (gimage);
}

void
edit_stroke_cmd_callback (GtkWidget *widget,
                          gpointer   data)
{
  GimpImage    *gimage;
  GimpDrawable *drawable;
  return_if_no_drawable (gimage, drawable, data);

  edit_stroke_selection (GIMP_ITEM (gimp_image_get_mask (gimage)), widget);
}

void
edit_stroke_selection (GimpItem  *item,
                       GtkWidget *parent)
{
  GimpImage    *gimage;
  GimpDrawable *active_drawable;
  GtkWidget    *dialog;

  g_return_if_fail (GIMP_IS_ITEM (item));

  gimage = gimp_item_get_image (item);

  active_drawable = gimp_image_active_drawable (gimage);

  if (! active_drawable)
    {
      g_message (_("There is no active layer or channel to stroke to."));
      return;
    }

  dialog = stroke_dialog_new (item, GIMP_STOCK_SELECTION_STROKE,
                              GIMP_HELP_SELECTION_STROKE,
                              parent);
  gtk_widget_show (dialog);
}


/*  private functions  */

static void
cut_named_buffer_callback (GtkWidget   *widget,
                           const gchar *name,
                           gpointer     data)
{
  GimpImage        *gimage = GIMP_IMAGE (data);
  const GimpBuffer *cut_buffer;
  GimpDrawable     *active_drawable;

  active_drawable = gimp_image_active_drawable (gimage);

  if (! active_drawable)
    {
      g_message (_("There is no active layer or channel to cut from."));
      return;
    }

  cut_buffer = gimp_edit_cut (gimage, active_drawable,
                              gimp_get_user_context (gimage->gimp));

  if (cut_buffer)
    {
      GimpBuffer *new_buffer;

      if (! (name && strlen (name)))
        name = _("(Unnamed Buffer)");

      new_buffer = gimp_buffer_new (cut_buffer->tiles, name, TRUE);

      gimp_container_add (gimage->gimp->named_buffers,
                          GIMP_OBJECT (new_buffer));
      g_object_unref (new_buffer);

      gimp_image_flush (gimage);
    }
}

static void
copy_named_buffer_callback (GtkWidget   *widget,
			    const gchar *name,
			    gpointer     data)
{
  GimpImage        *gimage = GIMP_IMAGE (data);
  const GimpBuffer *copy_buffer;
  GimpDrawable     *active_drawable;

  active_drawable = gimp_image_active_drawable (gimage);

  if (! active_drawable)
    {
      g_message (_("There is no active layer or channel to copy from."));
      return;
    }

  copy_buffer = gimp_edit_copy (gimage, active_drawable,
                                gimp_get_user_context (gimage->gimp));

  if (copy_buffer)
    {
      GimpBuffer *new_buffer;

      if (! (name && strlen (name)))
        name = _("(Unnamed Buffer)");

      new_buffer = gimp_buffer_new (copy_buffer->tiles, name, TRUE);

      gimp_container_add (gimage->gimp->named_buffers,
                          GIMP_OBJECT (new_buffer));
      g_object_unref (new_buffer);

      gimp_image_flush (gimage);
    }
}
