/* gap_resi_dialog.c
 * 1998.07.01 hof (Wolfgang Hofer)
 *
 * GAP ... Gimp Animation Plugins
 *
 * This Module contains the resize and scale Dialog for AnimFrames.
 * (It just is a shell to call Gimp's resize / scale Dialog
 *  as it is coded in the Gimp kernel $GIMP_HOME/app/resize.c )
 */
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

/* revision history
 * 0.96.00; 1998/07/01   hof: first release
 */
 
/* SYTEM (UNIX) includes */ 
#include <stdio.h>

/* GIMP includes */
#include "gtk/gtk.h"
#include "config.h"
#include "libgimp/stdplugins-intl.h"
#include "libgimp/gimp.h"
#include "libgimp/gimpui.h"
#include "resize.h"

/* GAP includes */
#include "gap_lib.h"
#include "gap_range_ops.h"
#include "gap_resi_dialog.h"

typedef struct
{
  GtkWidget * shell;
  Resize *    resize;
  int         gimage_id;
} ImageResize;

typedef struct {
  GtkWidget *dlg;
  gint run;
} t_res_int;

static void
res_cancel_callback (GtkWidget *widget,
		                gpointer   data)
{
  gtk_main_quit ();
}
static void
res_ok_callback (GtkWidget *widget,
		    gpointer   data)
{
  t_res_int *resi_ptr;
  
  resi_ptr = (t_res_int *)data;
  resi_ptr->run = TRUE;
  gtk_widget_destroy (GTK_WIDGET (resi_ptr->dlg));
}


gint
p_resi_dialog (gint32 image_id, t_gap_asiz asiz_mode, char *title_text,
               long *size_x, long *size_y, 
               long *offs_x, long *offs_y)
{
  gchar **l_argsv;
  gint    l_argsc;
  gint   l_width;
  gint   l_height;
  t_res_int  l_resint;
  Resize *l_rp;  
  
  GtkWidget *button;
  GtkWidget *vbox;
  ImageResize *image_resize;

  l_resint.run = FALSE;

  /* get info about the image (size is common to all frames) */
  l_width  = gimp_image_width(image_id);
  l_height = gimp_image_height(image_id);

  /* gtk init */
  l_argsc = 1;
  l_argsv = g_new (gchar *, 1);
  l_argsv[0] = g_strdup ("gap_res_dialog");
  
  gtk_init (&l_argsc, &l_argsv);

  /*  the ImageResize structure  */
  image_resize = (ImageResize *) g_malloc (sizeof (ImageResize));
  image_resize->gimage_id = image_id;

  if(asiz_mode == ASIZ_RESIZE)
  {
    image_resize->resize = resize_widget_new (ResizeWidget, l_width, l_height);
  }
  else
  {
    image_resize->resize = resize_widget_new (ScaleWidget, l_width, l_height);
  }

  /*  the dialog  */
  image_resize->shell = gtk_dialog_new ();
  l_resint.dlg = image_resize->shell;
  gtk_window_set_title (GTK_WINDOW (image_resize->shell), title_text);
  gtk_window_position (GTK_WINDOW (image_resize->shell), GTK_WIN_POS_MOUSE);
  gtk_signal_connect (GTK_OBJECT (image_resize->shell), "destroy",
		      (GtkSignalFunc) res_cancel_callback,
		      NULL);

  /*  the main vbox  */
  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_border_width (GTK_CONTAINER (vbox), 1);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (image_resize->shell)->vbox), vbox, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), image_resize->resize->resize_widget, FALSE, FALSE, 0);

  /*  Action area  */
  button = gtk_button_new_with_label ( _("OK"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc) res_ok_callback,
                       &l_resint);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (image_resize->shell)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  button = gtk_button_new_with_label ( _("Cancel"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc) res_cancel_callback,
                       NULL);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (image_resize->shell)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  gtk_widget_show (image_resize->resize->resize_widget);
  gtk_widget_show (vbox);
  gtk_widget_show (image_resize->shell);

  gtk_main ();
  gdk_flush ();


  l_rp = image_resize->resize;
  *size_x  = l_rp->width;
  *size_y  = l_rp->height;
  *offs_x  = l_rp->off_x;
  *offs_y  = l_rp->off_y;

  return (l_resint.run);
}
