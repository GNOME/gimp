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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "appenv.h"
#include "actionarea.h"
#include "buildmenu.h"
#include "colormaps.h"
#include "color_area.h"
#include "color_select.h"
#include "datafiles.h"
#include "errors.h"
#include "general.h"
#include "gimprc.h"
#include "interface.h"
#include "paint_funcs_area.h"
#include "palette.h"
#include "pixelrow.h"
#include "tag.h"


#define ENTRY_WIDTH  14
#define ENTRY_HEIGHT 10
#define SPACING 1
#define COLUMNS 16
#define ROWS 16

#define PREVIEW_WIDTH ((ENTRY_WIDTH * COLUMNS) + (SPACING * (COLUMNS + 1)))
#define PREVIEW_HEIGHT ((ENTRY_HEIGHT * ROWS) + (SPACING * (ROWS + 1)))

#define PALETTE_EVENT_MASK GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK | GDK_ENTER_NOTIFY_MASK

typedef struct _Palette _Palette, *PaletteP;

struct _Palette {
  GtkWidget *shell;
  GtkWidget *vbox;
  GtkWidget *frame;
  GtkWidget *menu;
  GtkWidget *option_menu;
  GtkWidget *color_area;
  GtkWidget *color_name;
  GtkWidget *palette_ops;
  GdkGC *gc;
  GtkAdjustment *sbar_data;
  PaletteEntriesP entries;
  PaletteEntryP color;
  ColorSelectP color_select;
  int color_select_active;
  int scroll_offset;
  int updating;
};

static void palette_create_palette_menu (PaletteP, PaletteEntriesP);
static PaletteEntryP palette_add_entry (PaletteEntriesP, char *, gfloat, gfloat, gfloat);
static void palette_delete_entry (PaletteP);
static void palette_calc_scrollbar (PaletteP);

static void palette_entries_load (char *);
static GSList * palette_entries_insert_list (GSList *, PaletteEntriesP);
static void palette_entries_delete (char *);
static void palette_entries_save (PaletteEntriesP, char *);
static void palette_entries_free (PaletteEntriesP);
static void palette_entry_free (PaletteEntryP);
static void palette_entries_set_callback (GtkWidget *, gpointer);

static void palette_change_color (PixelRow *, int);
static gint palette_color_area_expose (GtkWidget *, GdkEventExpose *, PaletteP);
static gint palette_color_area_events (GtkWidget *, GdkEvent *, PaletteP);
static void palette_scroll_update (GtkAdjustment *, gpointer);
static void palette_new_callback (GtkWidget *, gpointer);
static void palette_delete_callback (GtkWidget *, gpointer);
static void palette_refresh_callback (GtkWidget *, gpointer);
static void palette_edit_callback (GtkWidget *, gpointer);
static void palette_close_callback (GtkWidget *, gpointer);
static gint palette_dialog_delete_callback (GtkWidget *, GdkEvent *, gpointer);
static void palette_new_entries_callback (GtkWidget *, gpointer);
static void palette_add_entries_callback (GtkWidget *, gpointer, gpointer);
/* static void palette_merge_entries_callback (GtkWidget *, gpointer); */
static void palette_delete_entries_callback (GtkWidget *, gpointer);
static void palette_select_callback (PixelRow *, ColorSelectState, void *);

static void palette_draw_entries (PaletteP);
static void palette_draw_current_entry (PaletteP);
static void palette_update_current_entry (PaletteP);

GSList *palette_entries_list = NULL;

static PaletteP         palette = NULL;
static PaletteEntriesP  default_palette_entries = NULL;
static int              num_palette_entries = 0;

static PixelRow foreground;
static gfloat foreground_data[3];

static PixelRow background;
static gfloat background_data[3];

/*  Color select dialog  */
/* static ColorSelectP color_select = NULL;
static int color_select_active = 0; */

static ActionAreaItem action_items[] =
{
  { "New", palette_new_callback, NULL, NULL },
  { "Edit", palette_edit_callback, NULL, NULL },
  { "Delete", palette_delete_callback, NULL, NULL },
  { "Close", palette_close_callback, NULL, NULL },
};

static MenuItem palette_ops[] =
{
  { "New Palette", 0, 0, palette_new_entries_callback, NULL, NULL, NULL },
  { "Delete Palette", 0, 0, palette_delete_entries_callback, NULL, NULL, NULL },
  { "Refresh Palettes", 0, 0, palette_refresh_callback, NULL, NULL, NULL },
  { "Close", 0, 0, palette_close_callback, NULL, NULL, NULL },
  { NULL, 0, 0, NULL, NULL, NULL, NULL },
};



/* ================ */
int
color16_is_black  (
                   PixelRow * color
                   )
{
  COLOR16_NEW (init, tag_new (PRECISION_FLOAT, FORMAT_GRAY, ALPHA_NO));
  COLOR16_INIT (init);
  copy_row (color, &init);
  return (*(gfloat*)init_data == 0) ? 1 : 0;
}

int
color16_is_white  (
                   PixelRow * color
                   )
{
  COLOR16_NEW (init, tag_new (PRECISION_FLOAT, FORMAT_GRAY, ALPHA_NO));
  COLOR16_INIT (init);
  copy_row (color, &init);
  return (*(gfloat*)init_data == 1) ? 1 : 0;
}

int
color16_is_transparent  (
                         PixelRow * color
                         )
{
  COLOR16_NEW (init, tag_new (PRECISION_FLOAT, FORMAT_GRAY, ALPHA_YES));
  COLOR16_INIT (init);
  copy_row (color, &init);
  return (((gfloat*)init_data)[1] == 0) ? 1 : 0;
}







void
palettes_init (int no_data)
{
  palette_init_palettes (no_data);
}

void
palettes_free ()
{
  palette_free_palettes ();
}

void
palette_create ()
{
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *sbar;
  GtkWidget *frame;
  GtkWidget *options_box;
  GtkWidget *arrow_hbox;
  GtkWidget *label;
  GtkWidget *arrow;
  GtkWidget *menu_bar;
  GtkWidget *menu_bar_item;
  int i;

  if (!palette)
    {
      palette = g_malloc (sizeof (_Palette));

      palette->entries = default_palette_entries;
      palette->color = NULL;
      palette->color_select = NULL;
      palette->color_select_active = 0;
      palette->scroll_offset = 0;
      palette->gc = NULL;
      palette->updating = FALSE;

      /*  The shell and main vbox  */
      palette->shell = gtk_dialog_new ();
      gtk_window_set_wmclass (GTK_WINDOW (palette->shell), "color_palette", "Gimp");
      gtk_window_set_policy (GTK_WINDOW (palette->shell), FALSE, FALSE, FALSE);
      gtk_window_set_title (GTK_WINDOW (palette->shell), "Color Palette");
      vbox = gtk_vbox_new (FALSE, 1);
      gtk_container_border_width (GTK_CONTAINER (vbox), 1);
      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (palette->shell)->vbox), vbox, TRUE, TRUE, 0);

      /* handle the wm close event */
      gtk_signal_connect (GTK_OBJECT (palette->shell), "delete_event",
			  GTK_SIGNAL_FUNC (palette_dialog_delete_callback),
			  palette);

      /*  The palette options box  */
      options_box = gtk_hbox_new (FALSE, 1);
      gtk_box_pack_start (GTK_BOX (vbox), options_box, FALSE, FALSE, 0);

      /*  The popup menu -- palette_ops  */
      for (i = 0; palette_ops[i].label; i++)
	  palette_ops[i].user_data = palette;

      palette->palette_ops = build_menu (palette_ops, NULL);

      /*  The palette commands pulldown menu  */
      menu_bar = gtk_menu_bar_new ();
      gtk_box_pack_start (GTK_BOX (options_box), menu_bar, FALSE, FALSE, 0);
      menu_bar_item = gtk_menu_item_new ();
      gtk_container_add (GTK_CONTAINER (menu_bar), menu_bar_item);
      gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_bar_item), palette->palette_ops);
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

      /*  The option menu  */
      palette->option_menu = gtk_option_menu_new ();
      gtk_box_pack_start (GTK_BOX (options_box), palette->option_menu, TRUE, TRUE, 0);
      gtk_widget_show (palette->option_menu);
      gtk_widget_show (options_box);

      /*  The active color name  */
      palette->color_name = gtk_entry_new ();
      gtk_entry_set_text (GTK_ENTRY (palette->color_name), "Active Color Name");
      gtk_box_pack_start (GTK_BOX (vbox), palette->color_name, FALSE, FALSE, 0);

      gtk_widget_show (palette->color_name);

      /*  The horizontal box containing preview & scrollbar  */
      hbox = gtk_hbox_new (FALSE, 1);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
      frame = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
      gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);
      palette->sbar_data = GTK_ADJUSTMENT (gtk_adjustment_new (0, 0, PREVIEW_HEIGHT, 1, 1, PREVIEW_HEIGHT));
      gtk_signal_connect (GTK_OBJECT (palette->sbar_data), "value_changed",
			  (GtkSignalFunc) palette_scroll_update,
			  palette);
      sbar = gtk_vscrollbar_new (palette->sbar_data);
      gtk_box_pack_start (GTK_BOX (hbox), sbar, FALSE, FALSE, 0);

      /*  Create the color area window and the underlying image  */
      palette->color_area = gtk_preview_new (GTK_PREVIEW_COLOR);
      gtk_preview_size (GTK_PREVIEW (palette->color_area), PREVIEW_WIDTH, PREVIEW_HEIGHT);
      gtk_widget_set_events (palette->color_area, PALETTE_EVENT_MASK);
      gtk_signal_connect_after (GTK_OBJECT (palette->color_area), "expose_event",
				(GtkSignalFunc) palette_color_area_expose,
				palette);
      gtk_signal_connect (GTK_OBJECT (palette->color_area), "event",
			  (GtkSignalFunc) palette_color_area_events,
			  palette);
      gtk_container_add (GTK_CONTAINER (frame), palette->color_area);

      gtk_widget_show (palette->color_area);
      gtk_widget_show (sbar);
      gtk_widget_show (frame);
      gtk_widget_show (hbox);

      if(no_data)
	 palettes_init(FALSE);

      /*  The action area  */
      action_items[0].user_data = palette;
      action_items[1].user_data = palette;
      action_items[2].user_data = palette;
      action_items[3].user_data = palette;
      build_action_area (GTK_DIALOG (palette->shell), action_items, 4, 0);

      gtk_widget_show (vbox);
      gtk_widget_show (palette->shell);

      palette_create_palette_menu (palette, default_palette_entries);
      palette_calc_scrollbar (palette);
    }
  else
    {
      if (!GTK_WIDGET_VISIBLE (palette->shell))
	{
	  gtk_widget_show (palette->shell);
	}
      else
	{
	  gdk_window_raise(palette->shell->window);
	}
    }
}

void
palette_free ()
{
  if (palette)
    {
      gdk_gc_destroy (palette->gc);

      if (palette->color_select)
	color_select_free (palette->color_select);

      g_free (palette);

      palette = NULL;
    }
}

void 
palette_get_foreground  (
                         PixelRow * color
                         )
{
  copy_row (&foreground, color);
}

void 
palette_get_background  (
                         PixelRow * color
                         )
{
  copy_row (&background, color);
}

void 
palette_get_black  (
                    PixelRow * color
                    )
{
  COLOR16_NEWDATA (init, gfloat,
                   PRECISION_FLOAT, FORMAT_RGB, ALPHA_NO) = {0.0, 0.0, 0.0};
  COLOR16_INIT (init);
  copy_row (&init, color);
}

void 
palette_get_white  (
                    PixelRow * color
                    )
{
  COLOR16_NEWDATA (init, gfloat,
                   PRECISION_FLOAT, FORMAT_RGB, ALPHA_NO) = {1.0, 1.0, 1.0};
  COLOR16_INIT (init);
  copy_row (&init, color);
}

void 
palette_get_transparent  (
                          PixelRow * color
                          )
{
  COLOR16_NEWDATA (init, gfloat,
                   PRECISION_FLOAT, FORMAT_RGB, ALPHA_YES) = {0.0, 0.0, 0.0, 0.0};
  COLOR16_INIT (init);
  copy_row (&init, color);
}





void
palette_set_foreground (
                        PixelRow * col
                        )
{
  copy_row (col, &foreground);

  if (no_interface == FALSE)
    {
      store_color (&foreground_pixel, &foreground);
      color_area_update ();
    }
}

void
palette_set_background (PixelRow * col)
{
  copy_row (col, &background);

  if (no_interface == FALSE)
    {
      store_color (&background_pixel, &background);
      color_area_update ();
    }
}

void
palette_set_default_colors (void)
{
  COLOR16_NEW (col, tag_new (PRECISION_FLOAT, FORMAT_RGB, ALPHA_NO));

  COLOR16_INIT (col);
  
  palette_get_black (&col);
  palette_set_foreground (&col);

  palette_get_white (&col);
  palette_set_background (&col);
}


void 
palette_swap_colors (void)
{
  COLOR16_NEW (fg, tag_new (PRECISION_FLOAT, FORMAT_RGB, ALPHA_NO));
  COLOR16_NEW (bg, tag_new (PRECISION_FLOAT, FORMAT_RGB, ALPHA_NO));

  COLOR16_INIT (fg);
  COLOR16_INIT (bg);
  
  palette_get_foreground (&fg);
  palette_get_background (&bg);
  
  palette_set_foreground (&bg);
  palette_set_background (&fg);
}

void
palette_init_palettes (int no_data)
{
  if(!no_data)
    datafiles_read_directories (palette_path, palette_entries_load, 0);

  pixelrow_init (&foreground,
                 tag_new (PRECISION_FLOAT, FORMAT_RGB, ALPHA_NO),
                 (guchar *) foreground_data,
                 1);
  
  pixelrow_init (&background,
                 tag_new (PRECISION_FLOAT, FORMAT_RGB, ALPHA_NO),
                 (guchar *) background_data,
                 1);
}


void
palette_free_palettes (void)
{
  GSList *list;
  PaletteEntriesP entries;

  list = palette_entries_list;

  while (list)
    {
      entries = (PaletteEntriesP) list->data;

      /*  If the palette has been changed, save it, if possible  */
      if (entries->changed)
	/*  save the palette  */
	palette_entries_save (entries, entries->filename);

      palette_entries_free (entries);
      list = g_slist_next (list);
    }
  g_slist_free (palette_entries_list);

  if (palette)
    {
      palette->entries = NULL;
      palette->color = NULL;
      palette->color_select = NULL;
      palette->color_select_active = 0;
      palette->scroll_offset = 0;
    }
  num_palette_entries = 0;
  palette_entries_list = NULL;
}


/*****************************************/
/*         Local functions               */
/*****************************************/

static void
palette_create_palette_menu (PaletteP        palette,
			     PaletteEntriesP default_entries)
{
  GtkWidget *menu_item;
  GSList *list;
  PaletteEntriesP p_entries = NULL;
  PaletteEntriesP found_entries = NULL;
  int i = 0;
  int default_index = -1;

  palette->menu = gtk_menu_new ();

  list = palette_entries_list;
  while (list)
    {
      p_entries = (PaletteEntriesP) list->data;
      list = g_slist_next (list);

      /*  to make sure we get something!  */
      if (p_entries == NULL)
	{
	  found_entries = p_entries;
	  default_index = i;
	}

      if (p_entries == default_entries)
	{
	  found_entries = default_entries;
	  default_index = i;
	}

      menu_item = gtk_menu_item_new_with_label (p_entries->name);
      gtk_signal_connect (GTK_OBJECT (menu_item), "activate",
			  (GtkSignalFunc) palette_entries_set_callback,
			  (gpointer) p_entries);
      gtk_container_add (GTK_CONTAINER (palette->menu), menu_item);
      gtk_widget_show (menu_item);

      i++;
    }

  if (i == 0)
    {
      menu_item = gtk_menu_item_new_with_label ("none");
      gtk_container_add (GTK_CONTAINER (palette->menu), menu_item);
      gtk_widget_show (menu_item);

      /*  Set the action area and option menus to insensitive  */
      gtk_widget_set_sensitive (palette->option_menu, FALSE);
      gtk_widget_set_sensitive (GTK_DIALOG (palette->shell)->action_area, FALSE);

      /*  Clear the color area  */
      gdk_window_clear (palette->color_area->window);
    }
  else
    {
      /*  Make sure the action area and option menus are sensitive  */
      gtk_widget_set_sensitive (palette->option_menu, TRUE);
      gtk_widget_set_sensitive (GTK_DIALOG (palette->shell)->action_area, TRUE);

      /*  Clear the color area  */
      gdk_window_clear (palette->color_area->window);
    }

  gtk_option_menu_set_menu (GTK_OPTION_MENU (palette->option_menu), palette->menu);

  /*  Set the current item of the option menu to reflect
   *  the default palette.  Need to refresh here too
   */
  if (default_index != -1)
    {
      gtk_option_menu_set_history (GTK_OPTION_MENU (palette->option_menu), default_index);
      palette_entries_set_callback (NULL, found_entries);
    }
}

static void
palette_entries_load (char *filename)
{
  PaletteEntriesP entries;
  char            str[512];
  char           *tok;
  FILE           *fp;
  gfloat          r, g, b;

  r = g = b = 0;

  entries = (PaletteEntriesP) g_malloc (sizeof (_PaletteEntries));

  entries->filename = g_strdup (filename);
  entries->name = g_strdup (prune_filename (filename));
  entries->colors = NULL;
  entries->n_colors = 0;

  /*  Open the requested file  */

  if (!(fp = fopen (filename, "r")))
    {
      palette_entries_free (entries);
      return;
    }

  fread (str, 13, 1, fp);
  str[13] = '\0';
  if (strcmp (str, "GIMP Palette\n"))
    {
      fclose (fp);
      return;
    }

  while (!feof (fp))
    {
      if (!fgets (str, 512, fp))
	continue;

      if (str[0] != '#')
	{
	  tok = strtok (str, " \t");
	  if (tok)
	    r = atof (tok);

	  tok = strtok (NULL, " \t");
	  if (tok)
	    g = atof (tok);

	  tok = strtok (NULL, " \t");
	  if (tok)
	    b = atof (tok);

	  tok = strtok (NULL, "\n");

	  palette_add_entry (entries, tok, r, g, b);
	} /* if */
    } /* while */

  /*  Clean up  */

  fclose (fp);
  entries->changed = 0;

  palette_entries_list = palette_entries_insert_list(palette_entries_list, entries);

  /* Check if the current palette is the default one */
  if (strcmp(default_palette, prune_filename(filename)) == 0)
    default_palette_entries = entries;
}

static void
palette_entries_delete (char *filename)
{
  if (filename)
    unlink (filename);
}

static GSList *
palette_entries_insert_list (GSList *        list,
			     PaletteEntriesP entries)
{
  /*  add it to the list  */
  num_palette_entries++;
  return g_slist_append (list, (void *) entries);
}

static void
palette_entries_save (PaletteEntriesP  palette,
		      char            *filename)
{
  FILE * fp;
  GSList * list;
  PaletteEntryP entry;

  if (! filename)
    return;

  /*  Open the requested file  */
  if (! (fp = fopen (filename, "w")))
    {
      g_message ("can't save palette \"%s\"\n", filename);
      return;
    }

  fprintf (fp, "GIMP Palette\n");
  fprintf (fp, "# %s -- GIMP Palette file\n", palette->name);

  list = palette->colors;
  while (list)
    {
      entry = (PaletteEntryP) list->data;
      fprintf (fp, "%f %f %f\t%s\n", entry->color[0], entry->color[1],
	       entry->color[2], entry->name);
      list = g_slist_next (list);
    }

  /*  Clean up  */
  fclose (fp);
}

static void
palette_entries_free (PaletteEntriesP entries)
{
  PaletteEntryP entry;
  GSList * list;

  list = entries->colors;
  while (list)
    {
      entry = (PaletteEntryP) list->data;
      palette_entry_free (entry);
      list = list->next;
    }

  g_free (entries->name);
  if (entries->filename)
    g_free (entries->filename);
  g_free (entries);
}

static void
palette_entry_free (PaletteEntryP entry)
{
  if (entry->name)
    g_free (entry->name);

  g_free (entry);
}

static void
palette_entries_set_callback (GtkWidget *w,
			      gpointer   client_data)
{
  PaletteEntriesP pal;

  if (palette)
    {
      pal = (PaletteEntriesP) client_data;

      palette->entries = pal;
      palette->color = NULL;
      if (palette->color_select_active)
	{
	  palette->color_select_active = 0;
	  color_select_hide (palette->color_select);
	}
      palette->color_select = NULL;
      palette->scroll_offset = 0;

      palette_calc_scrollbar (palette);
      palette_draw_entries (palette);
      palette_draw_current_entry (palette);
    }
}

static void
palette_change_color (PixelRow * col,
		      int state)
{
  if (palette && palette->entries)
    {
      PixelRow r;
      gfloat d[3];

      pixelrow_init (&r, tag_new (PRECISION_FLOAT, FORMAT_RGB, ALPHA_NO), (guchar *) d, 1);
      copy_row (col, &r);
      
      switch (state)
	{
	case COLOR_NEW:
	  palette->color = palette_add_entry (palette->entries, "Untitled", d[0], d[1], d[2]);

	  palette_calc_scrollbar (palette);
	  palette_draw_entries (palette);
	  palette_draw_current_entry (palette);
	  break;

	case COLOR_UPDATE_NEW:
	  palette->color->color[0] = d[0];
	  palette->color->color[1] = d[1];
	  palette->color->color[2] = d[2];
	  palette_draw_entries (palette);
	  palette_draw_current_entry (palette);
	  break;

	default:
	  break;
	}
    }

  if (active_color == FOREGROUND)
    palette_set_foreground (col);
  else if (active_color == BACKGROUND)
    palette_set_background (col);
}

void
palette_set_active_color (PixelRow * col,
			  int state)
{
  palette_change_color (col, state);
}

static gint
palette_color_area_expose (GtkWidget      *widget,
			   GdkEventExpose *event,
			   PaletteP        palette)
{
  if (!palette->gc)
    palette->gc = gdk_gc_new (widget->window);

  palette_draw_current_entry (palette);

  return FALSE;
}

static gint
palette_color_area_events (GtkWidget *widget,
			   GdkEvent  *event,
			   PaletteP   palette)
{
  GdkEventButton *bevent;
  GSList *tmp_link;
  int width, height;
  int entry_width;
  int entry_height;
  int row, col;
  int pos;

  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *) event;
      width = palette->color_area->requisition.width;
      height = palette->color_area->requisition.height;
      entry_width = ((width - (SPACING * (COLUMNS + 1))) / COLUMNS) + SPACING;
      entry_height = ((height - (SPACING * (ROWS + 1))) / ROWS) + SPACING;
      
      col = (bevent->x - 1) / entry_width;
      row = (palette->scroll_offset + bevent->y - 1) / entry_height;
      pos = row * COLUMNS + col;
      
      if (bevent->button == 1 && palette->entries)
	{
	  
	  tmp_link = g_slist_nth (palette->entries->colors, pos);
	  if (tmp_link)
	    {
	      palette_draw_current_entry (palette);
	      palette->color = tmp_link->data;

	      /*  Update either foreground or background colors  */
              {
                PixelRow col;
                pixelrow_init (&col, tag_new (PRECISION_FLOAT, FORMAT_RGB, ALPHA_NO),
                               (guchar *) palette->color->color, 1);

                if (active_color == FOREGROUND)
                  palette_set_foreground (&col);
                else if (active_color == BACKGROUND)
                  palette_set_background (&col);
              }
	      palette_update_current_entry (palette);
	    }
	}
      break;
      
    default:
      break;
    }
  
  return FALSE;
}

static void
palette_scroll_update (GtkAdjustment *adjustment,
		       gpointer       data)
{
  PaletteP palette;

  palette = (PaletteP) data;

  if (palette)
    {
      palette->scroll_offset = adjustment->value;
      palette_draw_entries (palette);
      palette_draw_current_entry (palette);
    }
}

static void
palette_new_callback (GtkWidget *w,
		      gpointer   client_data)
{
  PaletteP palette;

  palette = client_data;
  if (palette && palette->entries)
    {
      if (active_color == FOREGROUND)
	palette->color =
	  palette_add_entry (palette->entries, "Untitled",
			     foreground_data[0], foreground_data[1], foreground_data[2]);
      else if (active_color == BACKGROUND)
	palette->color =
	  palette_add_entry (palette->entries, "Untitled",
			     background_data[0], background_data[1], background_data[2]);

      palette_calc_scrollbar (palette);
      palette_draw_entries (palette);
      palette_draw_current_entry (palette);
    }
}

static void
palette_delete_callback (GtkWidget *w,
			 gpointer   client_data)
{
  PaletteP palette;

  palette = client_data;
  if (palette)
    palette_delete_entry (palette);
}


static void
palette_refresh_callback (GtkWidget *w,
			  gpointer client_data)
{
  PaletteP palette;
  
  palette = client_data;
  if(palette)
    {
      palette_free_palettes (); 
      palette_init_palettes(FALSE);
      palette_create_palette_menu (palette, default_palette_entries);
      palette_calc_scrollbar (palette);
      palette_draw_entries (palette);
      palette_draw_current_entry (palette);
    }
  else
    {
      palette_free_palettes (); 
      palette_init_palettes(FALSE);
    }
  
}



static void
palette_edit_callback (GtkWidget *w,
		       gpointer   client_data)
{
  PaletteP palette;

  palette = client_data;
  if (palette && palette->entries && palette->color)
    {
      PixelRow col;

      pixelrow_init (&col, tag_new (PRECISION_FLOAT, FORMAT_RGB, ALPHA_NO),
                     (guchar *) palette->color->color, 1);

      if (!palette->color_select)
	{
	  palette->color_select = color_select_new (&col,
						    palette_select_callback, NULL,
						    FALSE);
	  palette->color_select_active = 1;
	}
      else
	{
	  if (!palette->color_select_active)
	    {
	      color_select_show (palette->color_select);
	      palette->color_select_active = 1;
	    }

	  color_select_set_color (palette->color_select, &col, 1);
	}
    }
}

static gint
palette_dialog_delete_callback (GtkWidget *w,
			 GdkEvent *e,
			 gpointer client_data) 
{
  palette_close_callback (w, client_data);

  return TRUE;
}


static void
palette_close_callback (GtkWidget *w,
			gpointer   client_data)
{
  PaletteP palette;

  palette = client_data;
  if (palette)
    {
      if (palette->color_select_active)
	{
	  palette->color_select_active = 0;
	  color_select_hide (palette->color_select);
	}

      if (GTK_WIDGET_VISIBLE (palette->shell))
	gtk_widget_hide (palette->shell);
    }
}

static void
palette_new_entries_callback (GtkWidget *w,
			      gpointer   client_data)
{
  query_string_box ("New Palette", "Enter a name for new palette", NULL,
		    palette_add_entries_callback, NULL);
}

static void
palette_add_entries_callback (GtkWidget *w,
			      gpointer   client_data,
			      gpointer   call_data)
{
  char *home;
  char *palette_name;
  char *local_path;
  char *first_token;
  char *token;
  char *path;
  PaletteEntriesP entries;

  palette_name = (char *) call_data;
  if (palette && palette_name)
    {
      entries = g_malloc (sizeof (_PaletteEntries));
      if (palette_path)
	{
	  /*  Get the first path specified in the palette path list  */
	  home = getenv("HOME");
	  local_path = g_strdup (palette_path);
	  first_token = local_path;
	  token = xstrsep(&first_token, ":");

	  if (token)
	    {
	      if (*token == '~')
		{
		  path = g_malloc(strlen(home) + strlen(token) + 1);
		  sprintf(path, "%s%s", home, token + 1);
		}
	      else
		{
		  path = g_malloc(strlen(token) + 1);
		  strcpy(path, token);
		}

	      entries->filename = g_malloc (strlen (path) + strlen (palette_name) + 2);
	      sprintf (entries->filename, "%s/%s", path, palette_name);

	      g_free (path);
	    }

	  g_free (local_path);
	}
      else
	entries->filename = NULL;

      entries->name = palette_name;  /*  don't need to copy because this memory is ours  */
      entries->colors = NULL;
      entries->n_colors = 0;
      entries->changed = 1;

      palette_entries_list = palette_entries_insert_list (palette_entries_list, entries);

      gtk_option_menu_remove_menu (GTK_OPTION_MENU (palette->option_menu));
      palette_create_palette_menu (palette, entries);
    }
}

/* static void
palette_merge_entries_callback (GtkWidget *w,
				gpointer   client_data)
{
} */

static void
palette_delete_entries_callback (GtkWidget *w,
				 gpointer   client_data)
{
  PaletteP palette;
  PaletteEntriesP entries;

  palette = client_data;
  if (palette && palette->entries)
    {
      /*  If a color selection dialog is up, hide it  */
      if (palette->color_select_active)
	{
	  palette->color_select_active = 0;
	  color_select_hide (palette->color_select);
	}

      gtk_option_menu_remove_menu (GTK_OPTION_MENU (palette->option_menu));

      entries = palette->entries;
      if (entries && entries->filename)
	palette_entries_delete (entries->filename);

      if (default_palette_entries == entries)
	default_palette_entries = NULL;
      palette->entries = NULL;

      palette_free_palettes ();   /*  free palettes, don't save any modified versions  */
      palette_init_palettes (FALSE);   /*  load in brand new palettes  */

      palette_create_palette_menu (palette, default_palette_entries);
    }
}

static void
palette_select_callback (PixelRow * color,
			 ColorSelectState state,
			 void *client_data)
{
  if (palette && palette->entries)
    {
      switch (state) {
      case COLOR_SELECT_UPDATE:
	break;
      case COLOR_SELECT_OK:
	if (palette->color)
	  {
            /* update the palette */
            PixelRow col;
            pixelrow_init (&col, tag_new (PRECISION_FLOAT, FORMAT_RGB, ALPHA_NO),
                           (guchar *) palette->color->color, 1);
            copy_row (color, &col);

            /*  Update either foreground or background colors  */
            if (active_color == FOREGROUND)
              palette_set_foreground (&col);
            else if (active_color == BACKGROUND)
              palette_set_background (&col);
	  
            palette_calc_scrollbar (palette);
            palette_draw_entries (palette);
            palette_draw_current_entry (palette);
	  }
	/* Fallthrough */
      case COLOR_SELECT_CANCEL:
        color_select_hide (palette->color_select);
	palette->color_select_active = 0;
      }
    }
}

static int
palette_draw_color_row (gfloat        **colors,
			int             ncolors,
			int             y,
			unsigned char  *buffer,
			GtkWidget      *preview)
{
  unsigned char *p;
  unsigned char bcolor;
  int width, height;
  int entry_width;
  int entry_height;
  int vsize;
  int vspacing;
  int i, j;

  bcolor = 0;

  width = preview->requisition.width;
  height = preview->requisition.height;
  entry_width = (width - (SPACING * (COLUMNS + 1))) / COLUMNS;
  entry_height = (height - (SPACING * (ROWS + 1))) / ROWS;

  if ((y >= 0) && ((y + SPACING) < height))
    vspacing = SPACING;
  else if (y < 0)
    vspacing = SPACING + y;
  else
    vspacing = height - y;

  if (vspacing > 0)
    {
      if (y < 0)
	y += SPACING - vspacing;

      for (i = SPACING - vspacing; i < SPACING; i++, y++)
	{
	  p = buffer;
	  for (j = 0; j < width; j++)
	    {
	      *p++ = bcolor;
	      *p++ = bcolor;
	      *p++ = bcolor;
	    }

	  gtk_preview_draw_row (GTK_PREVIEW (preview), buffer, 0, y, width);
	}

      if (y > SPACING)
	y += SPACING - vspacing;
    }
  else
    y += SPACING;

  vsize = (y >= 0) ? (entry_height) : (entry_height + y);

  if ((y >= 0) && ((y + entry_height) < height))
    vsize = entry_height;
  else if (y < 0)
    vsize = entry_height + y;
  else
    vsize = height - y;

  if (vsize > 0)
    {
      p = buffer;
      for (i = 0; i < ncolors; i++)
	{
	  for (j = 0; j < SPACING; j++)
	    {
	      *p++ = bcolor;
	      *p++ = bcolor;
	      *p++ = bcolor;
	    }

	  for (j = 0; j < entry_width; j++)
	    {
	      *p++ = colors[i][0] * 255;
	      *p++ = colors[i][1] * 255;
	      *p++ = colors[i][2] * 255;
	    }
	}

      for (i = 0; i < (COLUMNS - ncolors); i++)
	{
	  for (j = 0; j < (SPACING + entry_width); j++)
	    {
	      *p++ = 0;
	      *p++ = 0;
	      *p++ = 0;
	    }
	}

      for (j = 0; j < SPACING; j++)
	{
	  *p++ = bcolor;
	  *p++ = bcolor;
	  *p++ = bcolor;
	}

      if (y < 0)
	y += entry_height - vsize;
      for (i = 0; i < vsize; i++, y++)
	gtk_preview_draw_row (GTK_PREVIEW (preview), buffer, 0, y, width);
      if (y > entry_height)
	y += entry_height - vsize;
    }
  else
    y += entry_height;

  return y;
}

static void
palette_draw_entries (PaletteP palette)
{
  PaletteEntryP entry;
  unsigned char *buffer;
  gfloat *colors[COLUMNS];
  GSList *tmp_link;
  int width, height;
  int entry_width;
  int entry_height;
  int row_vsize;
  int index, y;

  if (palette && palette->entries && !palette->updating)
    {
      width = palette->color_area->requisition.width;
      height = palette->color_area->requisition.height;
      entry_width = (width - (SPACING * (COLUMNS + 1))) / COLUMNS;
      entry_height = (height - (SPACING * (ROWS + 1))) / ROWS;

      buffer = g_malloc (width * 3);

      y = -palette->scroll_offset;
      row_vsize = SPACING + entry_height;
      tmp_link = palette->entries->colors;
      index = 0;

      while ((tmp_link) && (y < -row_vsize))
	{
	  tmp_link = tmp_link->next;

	  if (++index == COLUMNS)
	    {
	      index = 0;
	      y += row_vsize;
	    }
	}

      index = 0;
      while (tmp_link)
	{
	  entry = tmp_link->data;
	  tmp_link = tmp_link->next;

	  colors[index] = entry->color;
	  index++;

	  if (index == COLUMNS)
	    {
	      index = 0;
	      y = palette_draw_color_row (colors, COLUMNS, y, buffer, palette->color_area);
	      if (y >= height)
		break;
	    }
	}

      while (y < height)
	{
	  y = palette_draw_color_row (colors, index, y, buffer, palette->color_area);
	  index = 0;
	}

      gtk_widget_draw (palette->color_area, NULL);

      g_free (buffer);
    }
}

static void
palette_draw_current_entry (PaletteP palette)
{
  PaletteEntryP entry;
  int width, height;
  int entry_width;
  int entry_height;
  int row, col;
  int x, y;

  if (palette && palette->entries && !palette->updating && palette->color)
    {
      gdk_gc_set_function (palette->gc, GDK_INVERT);

      entry = palette->color;

      row = entry->position / COLUMNS;
      col = entry->position % COLUMNS;

      entry_width = (palette->color_area->requisition.width -
		     (SPACING * (COLUMNS + 1))) / COLUMNS;
      entry_height = (palette->color_area->requisition.height -
		      (SPACING * (ROWS + 1))) / ROWS;

      x = col * (entry_width + SPACING);
      y = row * (entry_height + SPACING);
      y -= palette->scroll_offset;

      width = entry_width + SPACING;
      height = entry_height + SPACING;

      gdk_draw_rectangle (palette->color_area->window, palette->gc,
			  0, x, y, width, height);

      gdk_gc_set_function (palette->gc, GDK_COPY);
    }
}

static void
palette_update_current_entry (PaletteP palette)
{
  if (palette && palette->entries)
    {
      /*  Draw the current entry  */
      palette_draw_current_entry (palette);

      /*  Update the active color name  */
      gtk_entry_set_text (GTK_ENTRY (palette->color_name), palette->color->name);
    }
}

static PaletteEntryP
palette_add_entry (PaletteEntriesP  entries,
		   char            *name,
		   gfloat           r,
		   gfloat           g,
		   gfloat           b)
{
  PaletteEntryP entry;

  if (entries)
    {
      entry = g_malloc (sizeof (_PaletteEntry));

      entry->color[0] = r;
      entry->color[1] = g;
      entry->color[2] = b;
      if (name)
	entry->name = g_strdup (name);
      else
	entry->name = g_strdup ("Untitled");
      entry->position = entries->n_colors;

      entries->colors = g_slist_append (entries->colors, entry);
      entries->n_colors += 1;

      entries->changed = 1;

      return entry;
    }

  return NULL;
}

static void
palette_delete_entry (PaletteP palette)
{
  PaletteEntryP entry;
  GSList *tmp_link;
  int pos;

  if (palette && palette->entries && palette->color)
    {
      entry = palette->color;
      palette->entries->colors = g_slist_remove (palette->entries->colors, entry);
      palette->entries->n_colors--;
      palette->entries->changed = 1;

      pos = entry->position;
      palette_entry_free (entry);

      tmp_link = g_slist_nth (palette->entries->colors, pos);

      if (tmp_link)
	{
	  palette->color = tmp_link->data;

	  while (tmp_link)
	    {
	      entry = tmp_link->data;
	      tmp_link = tmp_link->next;
	      entry->position = pos++;
	    }
	}
      else
	{
	  tmp_link = g_slist_nth (palette->entries->colors, pos - 1);
	  if (tmp_link)
	    palette->color = tmp_link->data;
	}

      if (palette->entries->n_colors == 0)
	palette->color = palette_add_entry (palette->entries, "Black", 0, 0, 0);

      palette_calc_scrollbar (palette);
      palette_draw_entries (palette);
      palette_draw_current_entry (palette);
    }
}

static void
palette_calc_scrollbar (PaletteP palette)
{
  int n_entries;
  int cur_entry_row;
  int nrows;
  int row_vsize;
  int vsize;
  int page_size;
  int new_offset;

  if (palette && palette->entries)
    {
      n_entries = palette->entries->n_colors;
      nrows = n_entries / COLUMNS;
      if (n_entries % COLUMNS)
	nrows += 1;
      row_vsize = SPACING + ((palette->color_area->requisition.height -
			      (SPACING * (ROWS + 1))) / ROWS);
      vsize = row_vsize * nrows;
      page_size = row_vsize * ROWS;

      if (palette->color)
	cur_entry_row = palette->color->position / COLUMNS;
      else
	cur_entry_row = 0;

      new_offset = cur_entry_row * row_vsize;

      /* scroll only if necessary */
      if (new_offset < palette->scroll_offset)
	{
	  palette->scroll_offset = new_offset;
	}
      else if (new_offset > palette->scroll_offset)
	{
	  /* only scroll the minimum amount to bring the current color into view */
	  if ((palette->scroll_offset + page_size - row_vsize) < new_offset)
	    palette->scroll_offset = new_offset - (page_size - row_vsize);
	}

      /* sanity check to make sure the scrollbar offset is valid */
      if (vsize > page_size)
	if (palette->scroll_offset > (vsize - page_size))
	  palette->scroll_offset = vsize - page_size;

      palette->sbar_data->value = palette->scroll_offset;
      palette->sbar_data->upper = vsize;
      palette->sbar_data->page_size = (page_size < vsize) ? page_size : vsize;
      palette->sbar_data->page_increment = page_size;
      palette->sbar_data->step_increment = row_vsize;

      gtk_signal_emit_by_name (GTK_OBJECT (palette->sbar_data), "changed");
    }
}


/*  Procedural database entries  */

/****************************/
/*  PALETTE_GET_FOREGROUND  */


static Argument *
palette_refresh_invoker (Argument *args)
{
  /* FIXME: I've hardcoded success to be 1, because brushes_init() is a 
   *        void function right now.  It'd be nice if it returned a value at 
   *        some future date, so we could tell if things blew up when reparsing
   *        the list (for whatever reason). 
   *                       - Seth "Yes, this is a kludge" Burgess
   *                         <sjburges@ou.edu>
   *   -and shaemlessly stolen by Adrian Likins for use here...
   */
  
  int success = TRUE ;
  palette_free_palettes();
  palette_init_palettes(FALSE);

  return procedural_db_return_args (&palette_refresh_proc, success);
}


ProcRecord palette_refresh_proc =
{
  "gimp_palette_refresh",
  "Refreshes current palettes",
  "This procedure incorporates all palettes currently in the users palette path. ",
  "Adrian Likins <adrain@gimp.org>",
  "Adrian Likins",
  "1998",
  PDB_INTERNAL,

  /* input aarguments */
  0,
  NULL,

  /* Output arguments */
  0,
  NULL,
  
  /* Exec mehtos  */
  { { palette_refresh_invoker } },
};



static Argument *
palette_get_foreground_invoker (Argument *args)
{
  Argument *return_args;
  PixelRow r;
  guchar d[TAG_MAX_BYTES];
  unsigned char *col;

  pixelrow_init (&r, tag_new (PRECISION_U8, FORMAT_RGB, ALPHA_NO), d, 1);
  palette_get_foreground (&r);

  col = (unsigned char *) g_malloc (3);
  col[RED_PIX] = d[0];
  col[GREEN_PIX] = d[1];
  col[BLUE_PIX] = d[2];

  return_args = procedural_db_return_args (&palette_get_foreground_proc, TRUE);
  return_args[1].value.pdb_pointer = col;

  return return_args;
}

/*  The procedure definition  */
ProcArg palette_get_foreground_args[] =
{
  { PDB_COLOR,
    "foreground",
    "the foreground color"
  }
};

ProcRecord palette_get_foreground_proc =
{
  "gimp_palette_get_foreground",
  "Get the current GIMP foreground color",
  "This procedure retrieves the current GIMP foreground color.  The foreground color is used in a variety of tools such as paint tools, blending, and bucket fill.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  0,
  NULL,

  /*  Output arguments  */
  1,
  palette_get_foreground_args,

  /*  Exec method  */
  { { palette_get_foreground_invoker } },
};


/****************************/
/*  PALETTE_GET_BACKGROUND  */

static Argument *
palette_get_background_invoker (Argument *args)
{
  Argument *return_args;
  PixelRow r;
  guchar d[TAG_MAX_BYTES];
  unsigned char *col;

  pixelrow_init (&r, tag_new (PRECISION_U8, FORMAT_RGB, ALPHA_NO), d, 1);
  palette_get_background (&r);

  col = (unsigned char *) g_malloc (3);
  col[RED_PIX] = d[0];
  col[GREEN_PIX] = d[1];
  col[BLUE_PIX] = d[2];

  return_args = procedural_db_return_args (&palette_get_background_proc, TRUE);
  return_args[1].value.pdb_pointer = col;

  return return_args;
}

/*  The procedure definition  */
ProcArg palette_get_background_args[] =
{
  { PDB_COLOR,
    "background",
    "the background color"
  }
};

ProcRecord palette_get_background_proc =
{
  "gimp_palette_get_background",
  "Get the current GIMP background color",
  "This procedure retrieves the current GIMP background color.  The background color is used in a variety of tools such as blending, erasing (with non-apha images), and image filling.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  0,
  NULL,

  /*  Output arguments  */
  1,
  palette_get_background_args,

  /*  Exec method  */
  { { palette_get_background_invoker } },
};


/****************************/
/*  PALETTE_SET_FOREGROUND  */

static Argument *
palette_set_foreground_invoker (Argument *args)
{
  unsigned char *color;
  int success;

  success = TRUE;
  if (success)
    color = (unsigned char *) args[0].value.pdb_pointer;

  if (success)
    {
      PixelRow r;
      pixelrow_init (&r, tag_new (PRECISION_U8, FORMAT_RGB, ALPHA_NO), color, 1);
      palette_set_foreground (&r);
    }
  
  return procedural_db_return_args (&palette_set_foreground_proc, success);
}

/*  The procedure definition  */
ProcArg palette_set_foreground_args[] =
{
  { PDB_COLOR,
    "foreground",
    "the foreground color"
  }
};

ProcRecord palette_set_foreground_proc =
{
  "gimp_palette_set_foreground",
  "Set the current GIMP foreground color",
  "This procedure sets the current GIMP foreground color.  After this is set, operations which use foreground such as paint tools, blending, and bucket fill will use the new value.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  palette_set_foreground_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { palette_set_foreground_invoker } },
};


/****************************/
/*  PALETTE_SET_BACKGROUND  */

static Argument *
palette_set_background_invoker (Argument *args)
{
  unsigned char *color;
  int success;

  success = TRUE;
  if (success)
    color = (unsigned char *) args[0].value.pdb_pointer;

  if (success)
    {
      PixelRow r;
      pixelrow_init (&r, tag_new (PRECISION_U8, FORMAT_RGB, ALPHA_NO), color, 1);
      palette_set_background (&r);
    }

  return procedural_db_return_args (&palette_set_background_proc, success);
}

/*  The procedure definition  */
ProcArg palette_set_background_args[] =
{
  { PDB_COLOR,
    "background",
    "the background color"
  }
};

ProcRecord palette_set_background_proc =
{
  "gimp_palette_set_background",
  "Set the current GIMP background color",
  "This procedure sets the current GIMP background color.  After this is set, operations which use background such as blending, filling images, clearing, and erasing (in non-alpha images) will use the new value.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  1,
  palette_set_background_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { palette_set_background_invoker } },
};

/*******************************/
/*  PALETTE_SET_DEFAULT_COLORS */

static Argument *
palette_set_default_colors_invoker (Argument *args)
{
  int success;

  success = TRUE;
  if (success)
    palette_set_default_colors ();

  return procedural_db_return_args (&palette_set_background_proc, success);
}

/*  The procedure definition  */

ProcRecord palette_set_default_colors_proc =
{
  "gimp_palette_set_default_colors",
  "Set the current GIMP foreground and background colors to black and white",
  "This procedure sets the current GIMP foreground and background colors to their initial default values, black and white.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  0,
  NULL,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { palette_set_default_colors_invoker } },
};

/************************/
/*  PALETTE_SWAP_COLORS */

static Argument *
palette_swap_colors_invoker (Argument *args)
{
  int success;

  success = TRUE;
  if (success)
    palette_swap_colors ();

  return procedural_db_return_args (&palette_swap_colors_proc, success);
}

/*  The procedure definition  */

ProcRecord palette_swap_colors_proc =
{
  "gimp_palette_swap_colors",
  "Swap the current GIMP foreground and background colors",
  "This procedure swaps the current GIMP foreground and background colors, so that the new foreground color becomes the old background color and vice versa.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  0,
  NULL,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { palette_swap_colors_invoker } },
};
