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

#include <glib.h>
#include <stdlib.h>
#include <string.h>

#include "appenv.h" 
#include "channel.h"
#include "gdisplayF.h"
#include "gimpcontext.h"
#include "gimage_mask.h"
#include "gimpimage.h"
#include "global_edit.h"
#include "qmask.h"
#include "undo.h"

/*  Global variables  */

/*  Prototypes */
/*  Static variables  */
void
qmask_buttons_update (GDisplay *gdisp)
{
g_assert(gdisp);
g_assert(gdisp->gimage);

if (gdisp->gimage->qmask_state != GTK_TOGGLE_BUTTON(gdisp->qmaskon)->active)
  {
   /* Disable toggle from doing anything */
   gtk_signal_handler_block_by_func(GTK_OBJECT(gdisp->qmaskoff), 
                                   (GtkSignalFunc) qmask_deactivate,
                                   gdisp);
   gtk_signal_handler_block_by_func(GTK_OBJECT(gdisp->qmaskon), 
                                   (GtkSignalFunc) qmask_activate,
                                   gdisp);
   
   /* Change the state of the buttons */
   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gdisp->qmaskon), 
                                gdisp->gimage->qmask_state);

   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gdisp->qmaskoff),
                                !gdisp->gimage->qmask_state);
   
   /* Enable toggle again */
   gtk_signal_handler_unblock_by_func(GTK_OBJECT(gdisp->qmaskoff), 
                                   (GtkSignalFunc) qmask_deactivate,
                                   gdisp);
   gtk_signal_handler_unblock_by_func(GTK_OBJECT(gdisp->qmaskon), 
                                   (GtkSignalFunc) qmask_activate,
                                   gdisp);
/* Flush event queue */
/*  while (g_main_iteration(FALSE)); */
  }
}

void
qmask_deactivate(GtkWidget *w,
	    GDisplay *gdisp)
{
GimpImage *gimg;
GimpChannel *gmask;

if (gdisp)
  {
  gimg = gdisp->gimage;
  if (!gimg) return;

  if (!gdisp->gimage->qmask_state) {
	return; /* if already set do nothing */
	}

  undo_push_group_start (gimg, QMASK_UNDO);
  if ( (gmask = gimp_image_get_channel_by_name (gimg, "Qmask")) )
  	{
	gimage_mask_load (gimg, gmask);
	gimage_remove_channel(gimg, gmask);
	undo_push_qmask(gimg,1);
  	gdisp->gimage->qmask_state = 0;
        gdisplays_flush (); 
	}
  
  undo_push_group_end (gimg);
  }
}

void
qmask_activate(GtkWidget *w,
           GDisplay *gdisp)
{
GimpImage *gimg;
GimpChannel *gmask;

unsigned char color[3] = {255,0,0};
double opacity = 50;

if (gdisp)
  {
  gimg = gdisp->gimage;
  if (!gimg) return;

  if (gdisp->gimage->qmask_state) {
	return; /* If already set, do nothing */
	}

  if ( (gmask = gimp_image_get_channel_by_name (gimg, "Qmask")) )
    return; /* do nothing if Qmask already exists */
  undo_push_group_start (gimg, QMASK_UNDO);
  if (gimage_mask_is_empty(gimg))
    { /* if no selection */
    gmask = channel_new(gimg, 
                        gimg->width, 
                        gimg->height,
                        "Qmask", 
    	                (int)(255*opacity)/100,
                        color);
    gimp_image_add_channel (gimg, gmask, 0);
    edit_clear(gimg,GIMP_DRAWABLE(gmask)); 
    undo_push_qmask(gimg,0);
    gdisp->gimage->qmask_state = 1;
    gdisplays_flush();  
    }
  else 
    { /* if selection */
    gmask = channel_copy (gimage_get_mask (gimg));
    gimp_image_add_channel (gimg, gmask, 0);
    channel_set_color(gmask, color);
    channel_set_name(gmask, "Qmask");
    channel_set_opacity(gmask, opacity);
    gimage_mask_none (gimg);           /* Clear the selection */
    undo_push_qmask(gimg,0);
    gdisp->gimage->qmask_state = 1;
    gdisplays_flush();
    }
  undo_push_group_end(gimg);
  }
}
