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

#include "core/core-types.h"
#include "tools/tools-types.h"

#include "core/gimp.h"
#include "core/gimpbuffer.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpdrawable.h"
#include "core/gimpedit.h"
#include "core/gimpimage.h"
#include "core/gimpimage-mask.h"

#include "tools/tool_manager.h"

#include "widgets/gimpdialogfactory.h"

#include "dialogs.h"
#include "edit-commands.h"

#include "gdisplay.h"
#include "undo.h"

#include "libgimp/gimpintl.h"


#define return_if_no_display(gdisp) \
        gdisp = gdisplay_active (); \
        if (!gdisp) return


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
  GDisplay *gdisp;
  return_if_no_display (gdisp);

  undo_pop (gdisp->gimage);
}

void
edit_redo_cmd_callback (GtkWidget *widget,
			gpointer   data)
{
  GDisplay *gdisp;
  return_if_no_display (gdisp);

  undo_redo (gdisp->gimage);
}

void
edit_cut_cmd_callback (GtkWidget *widget,
		       gpointer   data)
{
  GDisplay *gdisp;
  return_if_no_display (gdisp);

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
  GDisplay *gdisp;
  return_if_no_display (gdisp);

  gimp_edit_copy (gdisp->gimage,
		  gimp_image_active_drawable (gdisp->gimage));
}

void
edit_paste_cmd_callback (GtkWidget *widget,
			 gpointer   data)
{
  GDisplay *gdisp;
  return_if_no_display (gdisp);

  if (gdisp->gimage->gimp->global_buffer)
    {
      /*  stop any active tool  */
      tool_manager_control_active (gdisp->gimage->gimp, HALT, gdisp);

      if (gimp_edit_paste (gdisp->gimage,
			   gimp_image_active_drawable (gdisp->gimage), 
			   gdisp->gimage->gimp->global_buffer,
			   FALSE))
	{
	  gdisplays_update_title (gdisp->gimage);
	  gdisplays_flush ();
	}
    }
}

void
edit_paste_into_cmd_callback (GtkWidget *widget,
			      gpointer   data)
{
  GDisplay *gdisp;
  return_if_no_display (gdisp);

  if (gdisp->gimage->gimp->global_buffer)
    {
      /*  stop any active tool  */
      tool_manager_control_active (gdisp->gimage->gimp, HALT, gdisp);

      if (gimp_edit_paste (gdisp->gimage,
			   gimp_image_active_drawable (gdisp->gimage), 
			   gdisp->gimage->gimp->global_buffer,
			   TRUE))
	{
	  gdisplays_update_title (gdisp->gimage);
	  gdisplays_flush ();
	}
    }
}

void
edit_paste_as_new_cmd_callback (GtkWidget *widget,
				gpointer   data)
{
  GDisplay *gdisp;
  return_if_no_display (gdisp);

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
  GDisplay  *gdisp;
  GtkWidget *qbox;

  return_if_no_display (gdisp);

  /*  stop any active tool  */
  tool_manager_control_active (gdisp->gimage->gimp, HALT, gdisp);

  qbox = gimp_query_string_box (_("Cut Named"),
				gimp_standard_help_func,
				"dialogs/cut_named.html",
				_("Enter a name for this buffer"),
				NULL,
				GTK_OBJECT (gdisp->gimage), "destroy",
				cut_named_buffer_callback, gdisp->gimage);
  gtk_widget_show (qbox);
}

void
edit_named_copy_cmd_callback (GtkWidget *widget,
			      gpointer   data)
{
  GDisplay  *gdisp;
  GtkWidget *qbox;

  return_if_no_display (gdisp);

  qbox = gimp_query_string_box (_("Copy Named"),
				gimp_standard_help_func,
				"dialogs/copy_named.html",
				_("Enter a name for this buffer"),
				NULL,
				GTK_OBJECT (gdisp->gimage), "destroy",
				copy_named_buffer_callback, gdisp->gimage);
  gtk_widget_show (qbox);
}

void
edit_named_paste_cmd_callback (GtkWidget *widget,
			       gpointer   data)
{
  GDisplay *gdisp;
  return_if_no_display (gdisp);

  gimp_dialog_factory_dialog_raise (global_dock_factory, "gimp:buffer-list");
}

void
edit_clear_cmd_callback (GtkWidget *widget,
			 gpointer   data)
{
  GDisplay *gdisp;
  return_if_no_display (gdisp);

  gimp_edit_clear (gdisp->gimage, gimp_image_active_drawable (gdisp->gimage));
  gdisplays_flush ();
}

void
edit_fill_cmd_callback (GtkWidget *widget,
			gpointer   data,
			guint      action)
{
  GimpFillType  fill_type;
  GDisplay     *gdisp;

  return_if_no_display (gdisp);

  fill_type = (GimpFillType) action;

  gimp_edit_fill (gdisp->gimage,
		  gimp_image_active_drawable (gdisp->gimage),
		  fill_type);

  gdisplays_flush ();
}

void
edit_stroke_cmd_callback (GtkWidget *widget,
			  gpointer   data)
{
  GDisplay *gdisp;
  return_if_no_display (gdisp);

  gimage_mask_stroke (gdisp->gimage,
		      gimp_image_active_drawable (gdisp->gimage));

  gdisplays_flush ();
}


/*  private functions  */

static void
cut_named_buffer_callback (GtkWidget *widget,
			   gchar     *name,
			   gpointer   data)
{
  TileManager *new_tiles;
  GimpImage   *gimage;

  gimage = (GimpImage *) data;
  
  new_tiles = gimp_edit_cut (gimage,
			     gimp_image_active_drawable (gimage));

  if (new_tiles)
    {
      GimpBuffer *buffer;

      buffer = gimp_buffer_new (new_tiles, name);
      gimp_container_add (gimage->gimp->named_buffers, GIMP_OBJECT (buffer));
    }

  gdisplays_flush ();
}

static void
copy_named_buffer_callback (GtkWidget *widget,
			    gchar     *name,
			    gpointer   data)
{
  TileManager *new_tiles;
  GimpImage   *gimage;

  gimage = (GimpImage *) data;

  new_tiles = gimp_edit_copy (gimage,
			      gimp_image_active_drawable (gimage));

  if (new_tiles)
    {
      GimpBuffer *buffer;

      buffer = gimp_buffer_new (new_tiles, name);
      gimp_container_add (gimage->gimp->named_buffers, GIMP_OBJECT (buffer));
    }
}
