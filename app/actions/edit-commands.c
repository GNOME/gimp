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

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "gui-types.h"

#include "core/gimp.h"
#include "core/gimpbuffer.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpdrawable.h"
#include "core/gimpedit.h"
#include "core/gimpimage.h"
#include "core/gimpimage-mask.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplay-foreach.h"

#include "tools/tool_manager.h"

#include "widgets/gimpdialogfactory.h"

#include "dialogs.h"
#include "edit-commands.h"

#include "undo.h"

#include "libgimp/gimpintl.h"


#define return_if_no_display(gdisp,data) \
  gdisp = gimp_context_get_display (gimp_get_user_context (GIMP (data))); \
  if (! gdisp) \
    return

#define return_if_no_image(gimage,data) \
  gimage = gimp_context_get_image (gimp_get_user_context (GIMP (data))); \
  if (! gimage) \
    return


/*  local function prototypes  */

static void   cut_named_buffer_callback  (GtkWidget *widget,
					  gchar     *name,
					  gpointer   data);
static void   copy_named_buffer_callback (GtkWidget *widget,
					  gchar     *name,
					  gpointer   data);


/*  public functions  */

void
edit_undo_cmd_callback (GtkWidget *widget,
			gpointer   data)
{
  GimpImage *gimage;
  return_if_no_image (gimage, data);

  if (undo_pop (gimage))
    {
      gdisplays_flush ();
    }
}

void
edit_redo_cmd_callback (GtkWidget *widget,
			gpointer   data)
{
  GimpImage *gimage;
  return_if_no_image (gimage, data);

  if (undo_redo (gimage))
    {
      gdisplays_flush ();
    }
}

void
edit_cut_cmd_callback (GtkWidget *widget,
		       gpointer   data)
{
  GimpDisplay *gdisp;
  return_if_no_display (gdisp, data);

  /*  stop any active tool  */
  tool_manager_control_active (gdisp->gimage->gimp, HALT, gdisp);

  if (gimp_edit_cut (gdisp->gimage,
		     gimp_image_active_drawable (gdisp->gimage)))
    {
      gdisplays_flush ();
    }
}

void
edit_copy_cmd_callback (GtkWidget *widget,
			gpointer   data)
{
  GimpImage *gimage;
  return_if_no_image (gimage, data);

  gimp_edit_copy (gimage, gimp_image_active_drawable (gimage));
}

void
edit_paste_cmd_callback (GtkWidget *widget,
			 gpointer   data)
{
  GimpDisplay *gdisp;
  return_if_no_display (gdisp, data);

  if (gdisp->gimage->gimp->global_buffer)
    {
      /*  stop any active tool  */
      tool_manager_control_active (gdisp->gimage->gimp, HALT, gdisp);

      if (gimp_edit_paste (gdisp->gimage,
			   gimp_image_active_drawable (gdisp->gimage), 
			   gdisp->gimage->gimp->global_buffer,
			   FALSE))
	{
	  gdisplays_flush ();
	}
    }
}

void
edit_paste_into_cmd_callback (GtkWidget *widget,
			      gpointer   data)
{
  GimpDisplay *gdisp;
  return_if_no_display (gdisp, data);

  if (gdisp->gimage->gimp->global_buffer)
    {
      /*  stop any active tool  */
      tool_manager_control_active (gdisp->gimage->gimp, HALT, gdisp);

      if (gimp_edit_paste (gdisp->gimage,
			   gimp_image_active_drawable (gdisp->gimage), 
			   gdisp->gimage->gimp->global_buffer,
			   TRUE))
	{
	  gdisplays_flush ();
	}
    }
}

void
edit_paste_as_new_cmd_callback (GtkWidget *widget,
				gpointer   data)
{
  GimpDisplay *gdisp;
  return_if_no_display (gdisp, data);

  if (gdisp->gimage->gimp->global_buffer)
    {
      /*  stop any active tool  */
      tool_manager_control_active (gdisp->gimage->gimp, HALT, gdisp);

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

  /*  stop any active tool  */
  tool_manager_control_active (gdisp->gimage->gimp, HALT, gdisp);

  qbox = gimp_query_string_box (_("Cut Named"),
				gimp_standard_help_func,
				"dialogs/cut_named.html",
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
  GimpImage *gimage;
  GtkWidget *qbox;
  return_if_no_image (gimage, data);

  qbox = gimp_query_string_box (_("Copy Named"),
				gimp_standard_help_func,
				"dialogs/copy_named.html",
				_("Enter a name for this buffer"),
				NULL,
				G_OBJECT (gimage), "disconnect",
				copy_named_buffer_callback, gimage);
  gtk_widget_show (qbox);
}

void
edit_named_paste_cmd_callback (GtkWidget *widget,
			       gpointer   data)
{
  gimp_dialog_factory_dialog_raise (global_dock_factory, "gimp-buffer-list", -1);
}

void
edit_clear_cmd_callback (GtkWidget *widget,
			 gpointer   data)
{
  GimpImage *gimage;
  return_if_no_image (gimage, data);

  gimp_edit_clear (gimage, gimp_image_active_drawable (gimage));
  gdisplays_flush ();
}

void
edit_fill_cmd_callback (GtkWidget *widget,
			gpointer   data,
			guint      action)
{
  GimpImage    *gimage;
  GimpFillType  fill_type;
  return_if_no_image (gimage, data);

  fill_type = (GimpFillType) action;

  gimp_edit_fill (gimage,
		  gimp_image_active_drawable (gimage),
		  fill_type);

  gdisplays_flush ();
}

void
edit_stroke_cmd_callback (GtkWidget *widget,
			  gpointer   data)
{
  GimpImage *gimage;
  return_if_no_image (gimage, data);

  gimp_image_mask_stroke (gimage,
                          gimp_image_active_drawable (gimage),
                          gimp_get_current_context (gimage->gimp));

  gdisplays_flush ();
}


/*  private functions  */

static void
cut_named_buffer_callback (GtkWidget *widget,
			   gchar     *name,
			   gpointer   data)
{
  GimpBuffer *cut_buffer;
  GimpImage  *gimage;

  gimage = (GimpImage *) data;

  cut_buffer = gimp_edit_cut (gimage,
                              gimp_image_active_drawable (gimage));

  if (cut_buffer)
    {
      GimpBuffer *new_buffer;

      if (! (name && strlen (name)))
        name = _("(Unnamed Buffer)");

      new_buffer = gimp_buffer_new (cut_buffer->tiles, name, TRUE);

      gimp_container_add (gimage->gimp->named_buffers,
                          GIMP_OBJECT (new_buffer));
    }

  gdisplays_flush ();
}

static void
copy_named_buffer_callback (GtkWidget *widget,
			    gchar     *name,
			    gpointer   data)
{
  GimpBuffer *copy_buffer;
  GimpImage  *gimage;

  gimage = (GimpImage *) data;

  copy_buffer = gimp_edit_copy (gimage,
                                gimp_image_active_drawable (gimage));

  if (copy_buffer)
    {
      GimpBuffer *new_buffer;

      if (! (name && strlen (name)))
        name = _("(Unnamed Buffer)");

      new_buffer = gimp_buffer_new (copy_buffer->tiles, name, TRUE);

      gimp_container_add (gimage->gimp->named_buffers,
                          GIMP_OBJECT (new_buffer));
    }
}
