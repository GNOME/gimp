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
#include "appenv.h"
#include "actionarea.h"
#include "buildmenu.h"
#include "dialog_handler.h"
#include "gimage.h"
#include "gimpcontext.h"
#include "gimprc.h"
#include "gimpset.h"
#include "interface.h"
#include "image_render.h"
#include "lc_dialog.h"
#include "lc_dialogP.h"
#include "session.h"

#include "libgimp/gimpintl.h"

/*  local function prototypes  */
static void  lc_dialog_update              (GimpImage *);
static void  lc_dialog_image_menu_callback (GtkWidget *, gpointer);
static void  lc_dialog_auto_callback       (GtkWidget *, gpointer);
static gint  lc_dialog_close_callback      (GtkWidget *, gpointer);
static void  lc_dialog_add_cb              (GimpSet *, GimpImage *, gpointer);
static void  lc_dialog_remove_cb           (GimpSet *, GimpImage *, gpointer);
static void  lc_dialog_destroy_cb          (GimpImage *, gpointer);
static void  lc_dialog_change_image        (GimpContext *, GimpImage *,
					    gpointer);

/*  FIXME: move these to a better place  */
static GtkWidget * lc_dialog_create_image_menu    (GimpImage **, int *,
						   MenuItemCallback);
static void        lc_dialog_create_image_menu_cb (gpointer, gpointer);

/*  the main dialog structure  */
LCDialog * lc_dialog = NULL;

/*********************************/
/*  Public L&C dialog functions  */
/*********************************/

void
lc_dialog_create (GimpImage* gimage)
{
  GtkWidget *util_box;
  GtkWidget *auto_button;
  GtkWidget *button;
  GtkWidget *label;
  GtkWidget *notebook;
  GtkWidget *separator;
  int default_index;

  if (lc_dialog)
    {
      if (! GTK_WIDGET_VISIBLE (lc_dialog->shell))
	{
	  gtk_widget_show (lc_dialog->shell);
	}
      else 
	{
	  gdk_window_raise (lc_dialog->shell->window);
	}

      lc_dialog_update (gimage);
      lc_dialog_update_image_list ();

      return;
    }

  lc_dialog = g_malloc (sizeof (LCDialog));
  lc_dialog->shell = gtk_dialog_new ();
  lc_dialog->gimage = NULL;
  lc_dialog->auto_follow_active = TRUE;

  /*  Register the dialog  */
  dialog_register (lc_dialog->shell);

  gtk_window_set_title (GTK_WINDOW (lc_dialog->shell), _("Layers & Channels"));
  gtk_window_set_wmclass (GTK_WINDOW (lc_dialog->shell),
			  "layers_and_channels", "Gimp");
  session_set_window_geometry (lc_dialog->shell, &lc_dialog_session_info, TRUE);

  /*  Handle the WM delete event  */
  gtk_signal_connect (GTK_OBJECT (lc_dialog->shell), "delete_event", 
		      GTK_SIGNAL_FUNC (lc_dialog_close_callback), NULL);

  /*  The toplevel vbox  */
  lc_dialog->subshell = gtk_vbox_new (FALSE, 1);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (lc_dialog->shell)->vbox),
		     lc_dialog->subshell);

  /*  The hbox to hold the image option menu box  */
  util_box = gtk_hbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (util_box), 2);
  gtk_box_pack_start (GTK_BOX (lc_dialog->subshell), util_box, FALSE, FALSE, 0);

  /*  The GIMP image option menu  */
  label = gtk_label_new (_("Image:"));
  gtk_box_pack_start (GTK_BOX (util_box), label, FALSE, FALSE, 2);
  gtk_widget_show (label);

  lc_dialog->image_option_menu = gtk_option_menu_new ();
  lc_dialog->image_menu = 
    lc_dialog_create_image_menu (&gimage, &default_index,
				 lc_dialog_image_menu_callback);
  gtk_box_pack_start (GTK_BOX (util_box), lc_dialog->image_option_menu,
		      TRUE, TRUE, 0);
  gtk_widget_show (lc_dialog->image_option_menu);

  gtk_option_menu_set_menu (GTK_OPTION_MENU (lc_dialog->image_option_menu),
			    lc_dialog->image_menu);
  if (default_index != -1)
    gtk_option_menu_set_history
      (GTK_OPTION_MENU (lc_dialog->image_option_menu), default_index);

  /*  The Auto-button  */
  auto_button = gtk_toggle_button_new_with_label (_("Auto"));
  gtk_box_pack_start (GTK_BOX (util_box), auto_button, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (auto_button), "clicked",
		      (GtkSignalFunc) lc_dialog_auto_callback,
		      auto_button);
  gtk_widget_show (auto_button);
  /*  State will be set, when the sub-dialogs exists (see below)  */

  gtk_widget_show (util_box);

  separator = gtk_hseparator_new ();
  gtk_box_pack_start (GTK_BOX (lc_dialog->subshell), separator,
		      FALSE, FALSE, 0);
  gtk_widget_show (separator);

  /*  The notebook widget  */
  notebook = gtk_notebook_new ();
  gtk_container_set_border_width (GTK_CONTAINER (notebook), 2);
  gtk_box_pack_start (GTK_BOX (lc_dialog->subshell), notebook, TRUE, TRUE, 0);

  label = gtk_label_new (_("Layers"));
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
			    layers_dialog_create (), label);
  gtk_widget_show (label);

  label = gtk_label_new (_("Channels"));
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
			    channels_dialog_create (), label);
  gtk_widget_show (label);

  label = gtk_label_new (_("Paths"));
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
			    paths_dialog_create (), label);
  gtk_widget_show (label);

  /*  Now all notebook pages exist, we can set the Auto-togglebutton  */
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (auto_button),
				lc_dialog->auto_follow_active);

  gtk_widget_show (notebook);

  /*  The action area  */
  gtk_container_set_border_width
    (GTK_CONTAINER (GTK_DIALOG (lc_dialog->shell)->action_area), 1);

  /*  The close button  */
  button = gtk_button_new_with_label (_("Close"));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (lc_dialog->shell)->action_area),
		      button, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) lc_dialog_close_callback,
		      lc_dialog->shell);
  gtk_widget_show (button);

  /*  Make sure the channels page is realized  */
  gtk_notebook_set_page (GTK_NOTEBOOK (notebook), 1);
  gtk_notebook_set_page (GTK_NOTEBOOK (notebook), 0);

  gtk_signal_connect (GTK_OBJECT (image_context), "add",
		      GTK_SIGNAL_FUNC (lc_dialog_add_cb), NULL);
  gtk_signal_connect (GTK_OBJECT (image_context), "remove",
		      GTK_SIGNAL_FUNC (lc_dialog_remove_cb), NULL);
  gtk_signal_connect (GTK_OBJECT (gimp_context_get_user ()), "image_changed",
		      GTK_SIGNAL_FUNC (lc_dialog_change_image), NULL);

  lc_dialog_update (gimage);
  lc_dialog_update_image_list ();

  gtk_widget_show (lc_dialog->subshell);
  gtk_widget_show (lc_dialog->shell);

  gdisplays_flush ();
}

void
lc_dialog_free ()
{
  if (lc_dialog == NULL)
    return;

  session_get_window_info (lc_dialog->shell, &lc_dialog_session_info);

  layers_dialog_free ();
  channels_dialog_free ();

  gtk_widget_destroy (lc_dialog->shell);

  g_free (lc_dialog);
  lc_dialog = NULL;
}

void
lc_dialog_rebuild (int new_preview_size)
{
  GimpImage* gimage;
  int flag;

  gimage = NULL;
  
  flag = 0;
  if (lc_dialog)
    {
      flag = 1;
      gimage = lc_dialog->gimage;
      lc_dialog_free ();
    }

  preview_size = new_preview_size;
  render_setup (transparency_type, transparency_size);

  if (flag)
    lc_dialog_create (gimage);
}

void
lc_dialog_flush ()
{
  if (! lc_dialog || lc_dialog->gimage == NULL)
    return;

  layers_dialog_flush ();
  channels_dialog_flush ();
  paths_dialog_flush ();
}

void
lc_dialog_update_image_list ()
{
  GimpImage* default_gimage;
  int default_index;

  if (lc_dialog == NULL)
    return;

  default_gimage = lc_dialog->gimage;
  lc_dialog->image_menu =
    lc_dialog_create_image_menu (&default_gimage, &default_index,
				 lc_dialog_image_menu_callback);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (lc_dialog->image_option_menu),
			    lc_dialog->image_menu);

  if (default_index != -1)
    {
      gtk_option_menu_set_history
	(GTK_OPTION_MENU (lc_dialog->image_option_menu), default_index);

      lc_dialog_update (default_gimage);
      gdisplays_flush ();

      if (! GTK_WIDGET_IS_SENSITIVE (lc_dialog->subshell) )
	gtk_widget_set_sensitive (lc_dialog->subshell, TRUE);
    }
  else
    {
      layers_dialog_clear ();
      channels_dialog_clear ();

      lc_dialog->gimage = NULL;

      if (GTK_WIDGET_IS_SENSITIVE (lc_dialog->subshell))
	gtk_widget_set_sensitive (lc_dialog->subshell, FALSE);
    }
}

/**********************************/
/*  Private L&C dialog functions  */
/**********************************/

static void
lc_dialog_update (GimpImage* gimage)
{
  if (! lc_dialog || lc_dialog->gimage == gimage)
    return;

  lc_dialog->gimage = gimage;

  layers_dialog_update (gimage);
  channels_dialog_update (gimage);
  paths_dialog_update (gimage);

  if (gimage)
    {
      gtk_signal_connect (GTK_OBJECT (gimage), "destroy",
			  GTK_SIGNAL_FUNC (lc_dialog_destroy_cb), NULL);
    }
}

typedef struct
{
  GImage           **def;
  int               *default_index;
  MenuItemCallback   callback;
  GtkWidget         *menu;
  int                num_items;
  GImage            *id;
} IMCBData;

static void
lc_dialog_create_image_menu_cb (gpointer im,
				gpointer d)
{
  GimpImage *gimage = GIMP_IMAGE (im);
  IMCBData  *data   = (IMCBData *) d;
  char      *image_name;
  char      *menu_item_label;
  GtkWidget *menu_item;

  /*  make sure the default index gets set to _something_, if possible  */
  if (*data->default_index == -1)
    {
      data->id = gimage;
      *data->default_index = data->num_items;
    }

  if (gimage == *data->def)
    {
      data->id = *data->def;
      *data->default_index = data->num_items;
    }

  image_name = g_basename (gimage_filename (gimage));
  menu_item_label =
    g_strdup_printf ("%s-%d", image_name, pdb_image_to_id (gimage));
  menu_item = gtk_menu_item_new_with_label (menu_item_label);
  gtk_signal_connect (GTK_OBJECT (menu_item), "activate",
		      (GtkSignalFunc) data->callback, gimage);
  gtk_container_add (GTK_CONTAINER (data->menu), menu_item);
  gtk_widget_show (menu_item);

  g_free (menu_item_label);
  data->num_items ++;  
}

static GtkWidget *
lc_dialog_create_image_menu (GimpImage        **def,
			     int               *default_index,
			     MenuItemCallback   callback)
{
  IMCBData data;

  data.def = def;
  data.default_index = default_index;
  data.callback = callback;
  data.menu = gtk_menu_new ();
  data.num_items = 0;
  data.id = NULL;

  *default_index = -1;

  gimage_foreach (lc_dialog_create_image_menu_cb, &data);

  if (! data.num_items)
    {
      GtkWidget *menu_item;

      menu_item = gtk_menu_item_new_with_label (_("none"));
      gtk_container_add (GTK_CONTAINER (data.menu), menu_item);
      gtk_widget_show (menu_item);
    }

  *def = data.id;

  return data.menu;
}

static void
lc_dialog_image_menu_callback (GtkWidget *w,
			       gpointer   client_data)
{
  if (! lc_dialog)
    return;

  lc_dialog_update (GIMP_IMAGE (client_data));
  gdisplays_flush ();
}

static void
lc_dialog_auto_callback (GtkWidget *toggle_button,
			 gpointer   client_data)
{
  GimpContext *context;

  if (! lc_dialog)
    return;

  lc_dialog->auto_follow_active =
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (toggle_button));

  context = gimp_context_get_user ();

  if (lc_dialog->auto_follow_active)
    lc_dialog_change_image (context,
			    gimp_context_get_image (context),
			    NULL);
}

static gint
lc_dialog_close_callback (GtkWidget *w,
			  gpointer   client_data)
{
  if (! lc_dialog)
    return TRUE;

  lc_dialog->gimage = NULL;
  gtk_widget_hide (lc_dialog->shell);

  return TRUE;
}

static void
lc_dialog_add_cb (GimpSet   *set,
		  GimpImage *gimage,
		  gpointer   user_data)
{
  if (! lc_dialog)
    return;

  lc_dialog_update_image_list ();
}

static void
lc_dialog_remove_cb (GimpSet   *set,
		     GimpImage *gimage,
		     gpointer   user_data)
{
  if (! lc_dialog)
    return;

  lc_dialog_update_image_list ();
}

static void
lc_dialog_destroy_cb (GimpImage *gimage,
		      gpointer   user_data)
{
  if (! lc_dialog)
    return;

  lc_dialog_update_image_list ();
}

static void
lc_dialog_change_image (GimpContext *context,
			GimpImage   *gimage,
		        gpointer     user_data)
{
  if (! lc_dialog || ! lc_dialog->auto_follow_active)
    return;

  if (gimage && gimp_set_have (image_context, gimage))
    {
      lc_dialog_update (gimage);
    }

  lc_dialog_update_image_list ();
}
