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
#include "buildmenu.h"
#include "dialog_handler.h"
#include "gimage.h"
#include "gimpcontext.h"
#include "gimprc.h"
#include "gimpset.h"
#include "gimpui.h"
#include "image_render.h"
#include "lc_dialog.h"
#include "lc_dialogP.h"
#include "layers_dialogP.h"
#include "session.h"

#include "libgimp/gimpintl.h"
#include "libgimp/gimplimits.h"

#define MENU_THUMBNAIL_WIDTH 24
#define MENU_THUMBNAIL_HEIGHT 24

/*  local function prototypes  */
static void  lc_dialog_update              (GimpImage *);
static void  lc_dialog_image_menu_callback (GtkWidget *, gpointer);
static void  lc_dialog_auto_callback       (GtkWidget *, gpointer);
static gint  lc_dialog_close_callback      (GtkWidget *, gpointer);
static void  lc_dialog_add_callback        (GimpSet *, GimpImage *, gpointer);
static void  lc_dialog_remove_callback     (GimpSet *, GimpImage *, gpointer);
static void  lc_dialog_change_image        (GimpContext *, GimpImage *,
					    gpointer);
static void  lc_dialog_help_func           (gchar *);

static void  lc_dialog_image_menu_preview_update_callback (GtkWidget *,
							   gpointer);
static void  lc_dialog_fill_preview_with_thumb (GtkWidget *, GimpImage *,
						gint, gint);


/*  FIXME: move these to a better place  */
static GtkWidget * lc_dialog_create_image_menu          (GimpImage **, gint *,
							 MenuItemCallback);
static void        lc_dialog_create_image_menu_callback (gpointer, gpointer);

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
  GtkWidget *separator;
  gint default_index;

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

      if (gimage)
	lc_dialog_update (gimage);

      lc_dialog_update_image_list ();

      return;
    }

  lc_dialog = g_new (LCDialog, 1);
  lc_dialog->shell =
    gimp_dialog_new (_("Layers & Channels"), "layers_and_channels",
		     lc_dialog_help_func,
		     "dialogs/layers_and_channels.html",
		     GTK_WIN_POS_NONE,
		     FALSE, TRUE, FALSE,
		     NULL);
  lc_dialog->gimage = NULL;
  lc_dialog->auto_follow_active = TRUE;

  /*  Register the dialog  */
  dialog_register (lc_dialog->shell);
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
  gimp_help_set_help_data (lc_dialog->image_option_menu, NULL, "#image_menu");
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
  gimp_help_set_help_data (auto_button, NULL, "#auto_button");
  gtk_widget_show (auto_button);
  /*  State will be set when the sub-dialogs exists (see below)  */

  gtk_widget_show (util_box);

  separator = gtk_hseparator_new ();
  gtk_box_pack_start (GTK_BOX (lc_dialog->subshell), separator,
		      FALSE, FALSE, 0);
  gtk_widget_show (separator);

  /*  The notebook widget  */
  lc_dialog->notebook = gtk_notebook_new ();
  gtk_container_set_border_width (GTK_CONTAINER (lc_dialog->notebook), 2);
  gtk_box_pack_start (GTK_BOX (lc_dialog->subshell), lc_dialog->notebook,
		      TRUE, TRUE, 0);

  label = gtk_label_new (_("Layers"));
  gtk_notebook_append_page (GTK_NOTEBOOK (lc_dialog->notebook),
			    layers_dialog_create (), label);
  gtk_widget_show (label);

  label = gtk_label_new (_("Channels"));
  gtk_notebook_append_page (GTK_NOTEBOOK (lc_dialog->notebook),
			    channels_dialog_create (), label);
  gtk_widget_show (label);

  label = gtk_label_new (_("Paths"));
  gtk_notebook_append_page (GTK_NOTEBOOK (lc_dialog->notebook),
			    paths_dialog_create (), label);
  gtk_widget_show (label);

  /*  Now all notebook pages exist, we can set the Auto-togglebutton  */
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (auto_button),
				lc_dialog->auto_follow_active);

  gtk_widget_show (lc_dialog->notebook);

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
  gtk_notebook_set_page (GTK_NOTEBOOK (lc_dialog->notebook), 1);
  gtk_notebook_set_page (GTK_NOTEBOOK (lc_dialog->notebook), 0);

  gtk_signal_connect (GTK_OBJECT (image_context), "add",
		      GTK_SIGNAL_FUNC (lc_dialog_add_callback), NULL);
  gtk_signal_connect (GTK_OBJECT (image_context), "remove",
		      GTK_SIGNAL_FUNC (lc_dialog_remove_callback), NULL);
  gtk_signal_connect (GTK_OBJECT (gimp_context_get_user ()), "image_changed",
		      GTK_SIGNAL_FUNC (lc_dialog_change_image), NULL);

  lc_dialog_update (gimage);
  lc_dialog_update_image_list ();

  gtk_widget_show (lc_dialog->subshell);
  gtk_widget_show (lc_dialog->shell);

  gdisplays_flush ();
}

void
lc_dialog_free (void)
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
lc_dialog_rebuild (gint new_preview_size)
{
  GimpImage* gimage;
  gboolean flag;

  gimage = NULL;
  flag   = FALSE;

  if (lc_dialog)
    {
      flag = TRUE;
      gimage = lc_dialog->gimage;
      /*  Unregister the dialog  */
      dialog_unregister (lc_dialog->shell);
      lc_dialog_free ();
    }

  preview_size = new_preview_size;
  render_setup (transparency_type, transparency_size);

  if (flag)
    lc_dialog_create (gimage);
}

void
lc_dialog_flush (void)
{
  if (! lc_dialog || lc_dialog->gimage == NULL)
    return;

  layers_dialog_flush ();
  channels_dialog_flush ();
  paths_dialog_flush ();
}

static gint 
image_menu_preview_update_do (GimpImage *gimage)
{
  if (lc_dialog)
    {
      gtk_container_foreach (GTK_CONTAINER (lc_dialog->image_menu),
			     lc_dialog_image_menu_preview_update_callback,
			     (gpointer) gimage);
    }
  return FALSE;
}

void
lc_dialog_menu_preview_dirty (GtkObject *obj,
			      gpointer   data)
{
  if (!preview_size)
    return;

  /* Update preview at a less busy time */
  gtk_idle_add ((GtkFunction) image_menu_preview_update_do, obj); 
}

void 
lc_dialog_preview_update (GimpImage *gimage)
{
  if (!preview_size)
    return;

  layers_dialog_invalidate_previews (gimage);
  gtk_idle_add ((GtkFunction) image_menu_preview_update_do, gimage);
}

static void
lc_dialog_image_menu_preview_update_callback (GtkWidget *widget,
					      gpointer   data)
{
  GtkWidget *menu_preview;
  GimpImage *gimage;
  GimpImage *gimage_to_update;

  gimage_to_update = (GimpImage *) data; 

  /* This is called via an idle  function, so it is possible
   * that the client_data no longer points to a GimpImage.. So don't
   * pass it to the GIMP_IMAGE() cast function. We never deference
   * it here anyways.
   */

  menu_preview = (GtkWidget *) gtk_object_get_data (GTK_OBJECT (widget),
						    "menu_preview");
  gimage = (GimpImage *) gtk_object_get_data (GTK_OBJECT (widget),
					      "menu_preview_gimage");

  if (menu_preview && gimage && gimage_to_update == gimage)
    {
      /* Must update the preview? */
      lc_dialog_fill_preview_with_thumb (menu_preview,
					 gimage,
					 MENU_THUMBNAIL_WIDTH,
					 MENU_THUMBNAIL_HEIGHT);
      gtk_widget_draw (GTK_WIDGET (menu_preview), NULL);
    }
}

void
lc_dialog_update_image_list (void)
{
  GimpImage* default_gimage;
  gint default_index;

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

      if (! GTK_WIDGET_IS_SENSITIVE (lc_dialog->subshell))
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
}

typedef struct
{
  GImage           **def;
  gint              *default_index;
  MenuItemCallback   callback;
  GtkWidget         *menu;
  gint               num_items;
  GImage            *id;
} IMCBData;

static void
lc_dialog_fill_preview_with_thumb (GtkWidget *widget,
				   GimpImage *gimage,
				   gint       width,
				   gint       height)
{
  guchar    *drawable_data;
  TempBuf   *buf;
  gint       bpp;
  gint       dwidth;
  gint       dheight;
  gint       x,y;
  guchar    *src;
  gdouble    r, g, b, a;
  gdouble    c0, c1;
  guchar    *p0, *p1, *even, *odd;
  
  bpp = 0; /* Only returned */

  dwidth  = gimage->width;
  dheight = gimage->height;
  
  /* Get right aspect ratio */  
  if (dwidth > dheight)
    {
      height = (width * dheight) / dwidth;
      width = (dwidth * height) / dheight;
    }
  else
    {
      width = (height * dwidth) / dheight;
      height = (dheight * width) / dwidth;
    }

  buf = gimp_image_construct_composite_preview (gimage, width, height);
  drawable_data = temp_buf_data (buf);
  bpp = buf->bytes;

  gtk_preview_size (GTK_PREVIEW (widget), width, height);

  /*  Draw the thumbnail with checks  */
  src = drawable_data;

  even = g_malloc (width * 3);
  odd  = g_malloc (width * 3);

  for (y = 0; y < height; y++)
    {
      p0 = even;
      p1 = odd;
      
      for (x = 0; x < width; x++)
	{
	  if (bpp == 4)
	    {
	      r =  ((gdouble)src[x*4+0])/255.0;
	      g = ((gdouble)src[x*4+1])/255.0;
	      b = ((gdouble)src[x*4+2])/255.0;
	      a = ((gdouble)src[x*4+3])/255.0;
	    }
	  else
	    {
	      r = ((gdouble)src[x*bpp+0])/255.0;
	      g = b = r;
	      a = ((gdouble)src[x*bpp+1])/255.0;
	    }

	  if ((x / GIMP_CHECK_SIZE_SM) & 1)
	    {
	      c0 = GIMP_CHECK_LIGHT;
	      c1 = GIMP_CHECK_DARK;
	    }
	  else
	    {
	      c0 = GIMP_CHECK_DARK;
	      c1 = GIMP_CHECK_LIGHT;
	    }

	  *p0++ = (c0 + (r - c0) * a) * 255.0;
	  *p0++ = (c0 + (g - c0) * a) * 255.0;
	  *p0++ = (c0 + (b - c0) * a) * 255.0;

	  *p1++ = (c1 + (r - c1) * a) * 255.0;
	  *p1++ = (c1 + (g - c1) * a) * 255.0;
	  *p1++ = (c1 + (b - c1) * a) * 255.0;
	}
      
      if ((y / GIMP_CHECK_SIZE_SM) & 1)
	{
	  gtk_preview_draw_row (GTK_PREVIEW (widget),
				(guchar *) odd, 0, y, width);
	}
      else
	{
	  gtk_preview_draw_row (GTK_PREVIEW (widget),
				(guchar *) even, 0, y, width);
	}
      src += width * bpp;
    }

  g_free (even);
  g_free (odd);
  temp_buf_free (buf);
}

static void
lc_dialog_create_image_menu_callback (gpointer im,
				      gpointer d)
{
  GimpImage *gimage = GIMP_IMAGE (im);
  IMCBData  *data   = (IMCBData *) d;
  gchar     *image_name;
  gchar     *menu_item_label;
  GtkWidget *menu_item;
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *wcolor_box;
  GtkWidget *wlabel;

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
  menu_item = gtk_menu_item_new();
  gtk_signal_connect (GTK_OBJECT (menu_item), "activate",
		      (GtkSignalFunc) data->callback, gimage);
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_add  (GTK_CONTAINER (menu_item), hbox);
  gtk_widget_show (hbox);

  if (preview_size)
    {
      vbox = gtk_vbox_new (FALSE, 0);
      gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
      gtk_widget_show (vbox);

      wcolor_box = gtk_preview_new (GTK_PREVIEW_COLOR);
      gtk_preview_set_dither (GTK_PREVIEW (wcolor_box), GDK_RGB_DITHER_MAX);
      gtk_widget_set_usize (GTK_WIDGET (wcolor_box) , 
			    MENU_THUMBNAIL_WIDTH , 
			    MENU_THUMBNAIL_HEIGHT);

      lc_dialog_fill_preview_with_thumb (wcolor_box,
					 gimage,
					 MENU_THUMBNAIL_WIDTH,
					 MENU_THUMBNAIL_HEIGHT);

      gtk_container_add (GTK_CONTAINER (vbox), wcolor_box);
      gtk_widget_show (wcolor_box);

      if (gtk_object_get_data (GTK_OBJECT (gimage),
			       "menu_preview_dirty") == NULL)
	{
	  /* Only add this signal once */
	  gtk_object_set_data (GTK_OBJECT (gimage), "menu_preview_dirty",
			       (gpointer) 1);
	  gtk_signal_connect_after (GTK_OBJECT (gimage), "dirty",
				    GTK_SIGNAL_FUNC (lc_dialog_menu_preview_dirty),
				    NULL);
	}
      gtk_object_set_data (GTK_OBJECT (menu_item), "menu_preview",
			   wcolor_box);
      gtk_object_set_data (GTK_OBJECT (menu_item), "menu_preview_gimage",
			   gimage);
    }
  
  gtk_container_add (GTK_CONTAINER (data->menu), menu_item);

  wlabel = gtk_label_new (menu_item_label);
  gtk_misc_set_alignment (GTK_MISC (wlabel), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (hbox), wlabel, TRUE, TRUE, 4);
  gtk_widget_show (wlabel);

  gtk_widget_show (menu_item);

  g_free (menu_item_label);
  data->num_items++;  
}

static GtkWidget *
lc_dialog_create_image_menu (GimpImage        **def,
			     gint              *default_index,
			     MenuItemCallback   callback)
{
  IMCBData data;

  data.def           = def;
  data.default_index = default_index;
  data.callback      = callback;
  data.menu          = gtk_menu_new ();
  data.num_items     = 0;
  data.id            = NULL;

  *default_index = -1;

  gimage_foreach (lc_dialog_create_image_menu_callback, &data);

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
lc_dialog_image_menu_callback (GtkWidget *widget,
			       gpointer   data)
{
  if (! lc_dialog)
    return;

  lc_dialog_update (GIMP_IMAGE (data));
  gdisplays_flush ();
}

static void
lc_dialog_auto_callback (GtkWidget *widget,
			 gpointer   data)
{
  GimpContext *context;

  if (! lc_dialog)
    return;

  lc_dialog->auto_follow_active =
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));

  context = gimp_context_get_user ();

  if (lc_dialog->auto_follow_active)
    lc_dialog_change_image (context,
			    gimp_context_get_image (context),
			    NULL);
}

static gint
lc_dialog_close_callback (GtkWidget *widget,
			  gpointer   data)
{
  if (! lc_dialog)
    return TRUE;

  lc_dialog->gimage = NULL;
  gtk_widget_hide (lc_dialog->shell);

  return TRUE;
}

static void
lc_dialog_add_callback (GimpSet   *set,
			GimpImage *gimage,
			gpointer   data)
{
  if (! lc_dialog)
    return;

  lc_dialog_update_image_list ();
}

static void
lc_dialog_remove_callback (GimpSet   *set,
			   GimpImage *gimage,
			   gpointer   data)
{
  if (! lc_dialog)
    return;

  lc_dialog_update_image_list ();
}

static void
lc_dialog_change_image (GimpContext *context,
			GimpImage   *gimage,
		        gpointer     data)
{
  if (! lc_dialog || ! lc_dialog->auto_follow_active)
    return;

  if (gimage && gimp_set_have (image_context, gimage))
    {
      lc_dialog_update (gimage);
      lc_dialog_update_image_list ();
    }
}

static void
lc_dialog_help_func (gchar *help_data)
{
  gchar *help_page;
  gint page_num;

  static gchar* dialog_names[] = { "layers", "channels", "paths" };

  page_num =
    gtk_notebook_get_current_page (GTK_NOTEBOOK (lc_dialog->notebook));

  if (page_num > 2)
    return;

  help_page = g_strconcat (dialog_names[page_num], "/",
			   "dialogs/",
			   dialog_names[page_num], ".html",
			   NULL);
  gimp_help (help_page);
  g_free (help_page);
}
