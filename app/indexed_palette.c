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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gdk/gdkkeysyms.h"
#include "appenv.h"
#include "actionarea.h"
#include "buildmenu.h"
#include "colormaps.h"
#include "color_select.h"
#include "color_area.h"
#include "errors.h"
#include "gdisplay.h"
#include "gimage.h"
#include "gimprc.h"
#include "general.h"
#include "image_render.h"
#include "interface.h"
#include "indexed_palette.h"
#include "palette.h"
#include "undo.h"

#define EVENT_MASK     GDK_BUTTON_PRESS_MASK | GDK_ENTER_NOTIFY_MASK

#define CELL_WIDTH     20
#define CELL_HEIGHT    20
#define P_AREA_WIDTH   (CELL_WIDTH * 16)
#define P_AREA_HEIGHT  (CELL_HEIGHT * 16)

/*  Add these features:
 *
 *  load/save colormaps
 *  requantize
 *  add color--by clicking in the checked region
 *  all changes need to flush colormap lookup cache
 */

typedef struct _IndexedPalette IndexedPalette;

struct _IndexedPalette {
  GtkWidget *shell;
  GtkWidget *vbox;
  GtkWidget *palette;
  GtkWidget *image_menu;
  GtkWidget *image_option_menu;

  /*  state information  */
  int gimage_id;
  int col_index;
};

/*  indexed palette routines  */
static void indexed_palette_draw (void);
static void indexed_palette_clear (void);
static void indexed_palette_update (int);

/*  indexed palette menu callbacks  */
static void indexed_palette_close_callback (GtkWidget *, gpointer);
static void indexed_palette_select_callback (int, int, int, ColorSelectState, void *);
static gint indexed_palette_delete_callback (GtkWidget *, GdkEvent *, gpointer);

/*  event callback  */
static gint indexed_palette_area_events (GtkWidget *, GdkEvent *);

/*  create image menu  */
static void image_menu_callback (GtkWidget *, gpointer);
static GtkWidget * create_image_menu (int *, int *, MenuItemCallback);

/*  Only one indexed palette  */
static IndexedPalette *indexedP = NULL;

/*  Color select dialog  */
static ColorSelectP color_select = NULL;
static int color_select_active = 0;

/*  the action area structure  */
static ActionAreaItem action_items[] =
{
  { "Close", indexed_palette_close_callback, NULL, NULL },
};

static MenuItem indexed_color_ops[] =
{
  { "Close", 'W', GDK_CONTROL_MASK,
    indexed_palette_close_callback, NULL, NULL, NULL },
  { NULL, 0, 0, NULL, NULL, NULL, NULL },
};


/**************************************/
/*  Public indexed palette functions  */
/**************************************/

void
indexed_palette_create (int gimage_id)
{
  GtkWidget *vbox;
  GtkWidget *frame;
  GtkWidget *util_box;
  GtkWidget *label;
  GtkWidget *arrow_hbox;
  GtkWidget *arrow;
  GtkWidget *ops_menu;
  GtkWidget *menu_bar;
  GtkWidget *menu_bar_item;
  GtkWidget *hbox;
  GtkAcceleratorTable *table;
  int default_index;

  if (!indexedP)
    {
      indexedP = g_malloc (sizeof (IndexedPalette));
      indexedP->gimage_id = -1;

      table = gtk_accelerator_table_new ();

      /*  The shell and main vbox  */
      indexedP->shell = gtk_dialog_new ();
      gtk_window_set_wmclass (GTK_WINDOW (indexedP->shell), "indexed_color_palette", "Gimp");
      gtk_window_set_title (GTK_WINDOW (indexedP->shell), "Indexed Color Palette");
      gtk_window_add_accelerator_table (GTK_WINDOW (indexedP->shell), table);

      gtk_signal_connect (GTK_OBJECT (indexedP->shell), "delete_event",
			  GTK_SIGNAL_FUNC (indexed_palette_delete_callback),
			  indexedP);

      indexedP->vbox = vbox = gtk_vbox_new (FALSE, 1);
      gtk_container_border_width (GTK_CONTAINER (vbox), 1);
      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (indexedP->shell)->vbox), vbox, TRUE, TRUE, 0);

      /*  The hbox to hold the command menu and image option menu box  */
      util_box = gtk_hbox_new (FALSE, 1);
      gtk_box_pack_start (GTK_BOX (vbox), util_box, FALSE, FALSE, 0);

      /*  The GIMP image option menu  */
      label = gtk_label_new ("Image:");
      gtk_box_pack_start (GTK_BOX (util_box), label, FALSE, FALSE, 2);
      indexedP->image_option_menu = gtk_option_menu_new ();
      indexedP->image_menu = create_image_menu (&gimage_id, &default_index, image_menu_callback);
      gtk_box_pack_start (GTK_BOX (util_box), indexedP->image_option_menu, TRUE, TRUE, 2);

      gtk_widget_show (indexedP->image_option_menu);
      gtk_option_menu_set_menu (GTK_OPTION_MENU (indexedP->image_option_menu), indexedP->image_menu);
      if (default_index != -1)
	gtk_option_menu_set_history (GTK_OPTION_MENU (indexedP->image_option_menu), default_index);
      gtk_widget_show (label);

      /*  The indexed palette commands pulldown menu  */
      ops_menu = build_menu (indexed_color_ops, table);

      menu_bar = gtk_menu_bar_new ();
      gtk_box_pack_start (GTK_BOX (util_box), menu_bar, FALSE, FALSE, 2);
      menu_bar_item = gtk_menu_item_new ();
      gtk_container_add (GTK_CONTAINER (menu_bar), menu_bar_item);
      gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_bar_item), ops_menu);
      arrow_hbox = gtk_hbox_new (FALSE, 1);
      gtk_container_add (GTK_CONTAINER (menu_bar_item), arrow_hbox);
      label = gtk_label_new ("Ops");
      arrow = gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_OUT);
      gtk_box_pack_start (GTK_BOX (arrow_hbox), arrow, FALSE, FALSE, 0);
      gtk_box_pack_start (GTK_BOX (arrow_hbox), label, FALSE, FALSE, 4);
      gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
      gtk_misc_set_alignment (GTK_MISC (arrow), 0.5, 0.5);

      gtk_widget_show (arrow);
      gtk_widget_show (label);
      gtk_widget_show (arrow_hbox);
      gtk_widget_show (menu_bar_item);
      gtk_widget_show (menu_bar);
      gtk_widget_show (util_box);

      /*  The palette frame  */
      frame = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
      gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 2);
      indexedP->palette = gtk_preview_new (GTK_PREVIEW_COLOR);
      gtk_preview_size (GTK_PREVIEW (indexedP->palette), P_AREA_WIDTH, P_AREA_HEIGHT);
      gtk_widget_set_events (indexedP->palette, EVENT_MASK);
      gtk_signal_connect (GTK_OBJECT (indexedP->palette), "event",
			  (GtkSignalFunc) indexed_palette_area_events,
			  NULL);
      gtk_container_add (GTK_CONTAINER (frame), indexedP->palette);

      gtk_widget_show (indexedP->palette);
      gtk_widget_show (frame);

      /* some helpful hints */
      hbox = gtk_hbox_new(FALSE, 1);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 1);
      label = gtk_label_new (" Click to select color.  Right-click to edit color");
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 1);

      gtk_widget_show (hbox);
      gtk_widget_show (label);
      /*  The action area  */
      action_items[0].user_data = indexedP;
      build_action_area (GTK_DIALOG (indexedP->shell), action_items, 1, 0);

      gtk_widget_show (vbox);
      gtk_widget_show (indexedP->shell);

      indexed_palette_update (gimage_id);
    }
  else
    {
      if (!GTK_WIDGET_VISIBLE (indexedP->shell))
	gtk_widget_show (indexedP->shell);

      indexed_palette_update (gimage_id);
      indexed_palette_update_image_list ();
    }
}

void
indexed_palette_update_image_list ()
{
  int default_index;
  int default_id;

  if (! indexedP)
    return;

  gtk_option_menu_remove_menu (GTK_OPTION_MENU (indexedP->image_option_menu));
  gtk_widget_destroy (indexedP->image_menu);

  default_id = indexedP->gimage_id;
  indexedP->image_menu = create_image_menu (&default_id, &default_index, image_menu_callback);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (indexedP->image_option_menu), indexedP->image_menu);

  if (default_index != -1)
    {
      if (! GTK_WIDGET_IS_SENSITIVE (indexedP->vbox))
	gtk_widget_set_sensitive (indexedP->vbox, TRUE);
      gtk_option_menu_set_history (GTK_OPTION_MENU (indexedP->image_option_menu), default_index);

      indexed_palette_update (default_id);
    }
  else
    {
      if (GTK_WIDGET_IS_SENSITIVE (indexedP->vbox))
	{
	  gtk_widget_set_sensitive (indexedP->vbox, FALSE);
	  indexed_palette_clear ();
	}
    }
}

static void
indexed_palette_draw ()
{
  GImage *gimage;
  int i, j, k, l, b;
  int col;
  guchar row[P_AREA_WIDTH * 3];

  if (!indexedP)
    return;
  if ((gimage = gimage_get_ID (indexedP->gimage_id)) == NULL)
    return;

  col = 0;
  for (i = 0; i < 16; i++)
    {
      for (j = 0; j < 16 && col < gimage->num_cols; j++, col++)
	{
	  for (k = 0; k < CELL_WIDTH; k++)
	    for (b = 0; b < 3; b++)
	      row[(j * CELL_WIDTH + k) * 3 + b] = gimage->cmap[col * 3 + b];
	}

      for (k = 0; k < CELL_HEIGHT; k++)
	{
	  for (l = j * CELL_WIDTH; l < 16 * CELL_WIDTH; l++)
	    for (b = 0; b < 3; b++)
	      row[l * 3 + b] = ((((i * CELL_HEIGHT + k) & 0x4) ? (l) : (l + 0x4)) & 0x4) ?
		blend_light_check[0] : blend_dark_check[0];

	  gtk_preview_draw_row (GTK_PREVIEW (indexedP->palette), row, 0,
				i * CELL_HEIGHT + k, P_AREA_WIDTH);
	}
    }

  gtk_widget_draw (indexedP->palette, NULL);
}

static void
indexed_palette_clear ()
{
  int i, j;
  int offset;
  guchar row[P_AREA_WIDTH * 3];

  if (!indexedP)
    return;

  for (i = 0; i < P_AREA_HEIGHT; i += 4)
    {
      offset = (i & 0x4) ? 0x4 : 0x0;

      for (j = 0; j < P_AREA_WIDTH; j++)
	{
	  row[j * 3 + 0] = row[j * 3 + 1] = row[j * 3 + 2] =
	    ((j + offset) & 0x4) ? blend_light_check[0] : blend_dark_check[0];
	}

      for (j = 0; j < 4; j++)
	gtk_preview_draw_row (GTK_PREVIEW (indexedP->palette), row, 0, i + j, P_AREA_WIDTH);
    }

  gtk_widget_draw (indexedP->palette, NULL);
}

static void
indexed_palette_update (int gimage_id)
{
  GImage *gimage;

  if (!indexedP)
    return;
  if ((gimage = gimage_get_ID (gimage_id)) == NULL)
    return;

  if (gimage_base_type (gimage) == INDEXED)
    {
      indexedP->gimage_id = gimage_id;
      indexed_palette_draw ();
    }
}

static gint
indexed_palette_delete_callback (GtkWidget *w,
				 GdkEvent *e,
				 gpointer   client_data)
{
  indexed_palette_close_callback (w, client_data);

  return FALSE;
}

static void
indexed_palette_close_callback (GtkWidget *w,
				gpointer   client_data)
{
  if (!indexedP)
    return;

  if (GTK_WIDGET_VISIBLE (indexedP->shell))
    gtk_widget_hide (indexedP->shell);
}

static void
indexed_palette_select_callback (int   r,
				 int   g,
				 int   b,
				 ColorSelectState state,
				 void *client_data)
{
  GImage *gimage;

  if (!indexedP)
    return;

  if ((gimage = gimage_get_ID (indexedP->gimage_id)) == NULL)
    return;

  if (color_select )
    {
      switch (state) {
      case COLOR_SELECT_UPDATE:
	break;
      case COLOR_SELECT_OK:
	gimage->cmap[indexedP->col_index * 3 + 0] = r;
	gimage->cmap[indexedP->col_index * 3 + 1] = g;
	gimage->cmap[indexedP->col_index * 3 + 2] = b;
	
	gdisplays_update_full (gimage->ID);
	indexed_palette_draw ();
	/* Fallthrough */
      case COLOR_SELECT_CANCEL:
	color_select_hide (color_select);
	color_select_active = FALSE;
      }
    }
}

static gint
indexed_palette_area_events (GtkWidget *widget,
			     GdkEvent * event)
{
  GImage *gimage;
  GdkEventButton *bevent;
  guchar r, g, b;

  if (!indexedP)
    return FALSE;

  if ((gimage = gimage_get_ID (indexedP->gimage_id)) == NULL)
    return FALSE;

  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *) event;

      if (bevent->button == 1)
	{
	  indexedP->col_index = 16 * ((int)bevent->y / CELL_HEIGHT) + ((int)bevent->x / CELL_WIDTH);
	  r = gimage->cmap[indexedP->col_index * 3 + 0];
	  g = gimage->cmap[indexedP->col_index * 3 + 1];
	  b = gimage->cmap[indexedP->col_index * 3 + 2];
	  if (active_color == FOREGROUND) 
	    palette_set_foreground (r, g, b);
	  else if (active_color == BACKGROUND)
	    palette_set_background (r, g, b); 
	}
 
        if (bevent->button == 3)
	{
	  indexedP->col_index = 16 * ((int)bevent->y / CELL_HEIGHT) + ((int)bevent->x / CELL_WIDTH);
	  r = gimage->cmap[indexedP->col_index * 3 + 0];
	  g = gimage->cmap[indexedP->col_index * 3 + 1];
	  b = gimage->cmap[indexedP->col_index * 3 + 2];

	  if (! color_select)
	    {
	      color_select = color_select_new (r, g, b, indexed_palette_select_callback, NULL, FALSE);
	      color_select_active = 1;
	    }
	  else
	    {
	      if (! color_select_active)
		color_select_show (color_select);
	      color_select_set_color (color_select, r, g, b, 1);
	    }
	}
      break;

    default:
      break;
    }

  return FALSE;
}

static void
image_menu_callback (GtkWidget *w,
		     gpointer   client_data)
{
  if (!indexedP)
    return;
  if (gimage_get_ID ((long) client_data) != NULL)
    {
      indexed_palette_update ((long) client_data);
    }
}

static GtkWidget *
create_image_menu (int              *default_id,
		   int              *default_index,
		   MenuItemCallback  callback)
{
  extern GSList *image_list;

  GImage *gimage;
  GtkWidget *menu_item;
  GtkWidget *menu;
  char *menu_item_label;
  char *image_name;
  GSList *tmp;
  int num_items = 0;
  int id;

  id = -1;

  *default_index = -1;
  menu = gtk_menu_new ();

  tmp = image_list;
  while (tmp)
    {
      gimage = tmp->data;
      tmp = g_slist_next (tmp);

      if (gimage_base_type (gimage) == INDEXED)
	{
	  id = -1;
	  
	  /*  make sure the default index gets set to _something_, if possible  */
	  if (*default_index == -1)
	    {
	      id = gimage->ID;
	      *default_index = num_items;
	    }

	  if (gimage->ID == *default_id)
	    {
	      id = *default_id;
	      *default_index = num_items;
	    }

	  image_name = prune_filename (gimage_filename (gimage));
	  menu_item_label = (char *) g_malloc (strlen (image_name) + 15);
	  sprintf (menu_item_label, "%s-%d", image_name, gimage->ID);
	  menu_item = gtk_menu_item_new_with_label (menu_item_label);
	  gtk_signal_connect (GTK_OBJECT (menu_item), "activate",
			      (GtkSignalFunc) callback,
			      (gpointer) ((long) gimage->ID));
	  gtk_container_add (GTK_CONTAINER (menu), menu_item);
	  gtk_widget_show (menu_item);

	  g_free (menu_item_label);
	  num_items ++;
	}
    }

  if (!num_items)
    {
      menu_item = gtk_menu_item_new_with_label ("none");
      gtk_container_add (GTK_CONTAINER (menu), menu_item);
      gtk_widget_show (menu_item);
    }

  *default_id = id;

  return menu;
}
