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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_DIRENT_H
#include <sys/types.h>
#include <dirent.h>
#endif
#include <sys/stat.h>
#include <sys/types.h>

#include <gtk/gtk.h>

#include "apptypes.h"

#include "appenv.h"
#include "color_area.h"
#include "color_notebook.h"
#include "datafiles.h"
#include "dialog_handler.h"
#include "gimage.h"
#include "gimpcontext.h"
#include "gimpdnd.h"
#include "gimppalette.h"
#include "gimprc.h"
#include "gimpui.h"
#include "gradient.h"
#include "gradient_header.h"
#include "gradient_select.h"
#include "palette.h"
#include "paletteP.h"
#include "session.h"
#include "palette_select.h"
#include "pixel_region.h"
#include "procedural_db.h"
#include "temp_buf.h"

#include "libgimp/gimpenv.h"

#include "libgimp/gimpintl.h"

#include "pixmaps/zoom_in.xpm"
#include "pixmaps/zoom_out.xpm"


#define ENTRY_WIDTH  12
#define ENTRY_HEIGHT 10
#define SPACING       1
#define COLUMNS      16
#define ROWS         11

#define PREVIEW_WIDTH  ((ENTRY_WIDTH * COLUMNS) + (SPACING * (COLUMNS + 1)))
#define PREVIEW_HEIGHT ((ENTRY_HEIGHT * ROWS) + (SPACING * (ROWS + 1)))

#define PALETTE_EVENT_MASK GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK | \
                           GDK_ENTER_NOTIFY_MASK

/* New palette code... */

#define IMPORT_PREVIEW_WIDTH  80
#define IMPORT_PREVIEW_HEIGHT 80
#define MAX_IMAGE_COLORS      (10000*2)

typedef enum
{
  GRAD_IMPORT = 0,
  IMAGE_IMPORT = 1,
  INDEXED_IMPORT = 2
} ImportType;

typedef struct _ImportDialog ImportDialog;

struct _ImportDialog
{
  GtkWidget     *dialog;
  GtkWidget     *preview;
  GtkWidget     *entry;
  GtkWidget     *select_area;
  GtkWidget     *select;
  GtkWidget     *image_list;
  GtkWidget     *image_menu_item_image;
  GtkWidget     *image_menu_item_indexed;
  GtkWidget     *image_menu_item_gradient;
  GtkWidget     *optionmenu1_menu;
  GtkWidget     *type_option;
  GtkWidget     *threshold_scale;
  GtkWidget     *threshold_text;
  GtkAdjustment *threshold;
  GtkAdjustment *sample;
  ImportType     import_type;
  GimpImage     *gimage;
};

typedef struct _PaletteDialog PaletteDialog;

struct _PaletteDialog
{
  GtkWidget        *shell;

  GtkWidget        *color_area;
  GtkWidget        *scrolled_window;
  GtkWidget        *color_name;
  GtkWidget        *clist;

  GtkWidget        *popup_menu;
  GtkWidget        *delete_menu_item;
  GtkWidget        *edit_menu_item;

  ColorNotebook    *color_notebook;
  gboolean          color_notebook_active;

  GimpPalette      *palette;
  GimpPaletteEntry *color;
  GimpPaletteEntry *dnd_color;

  GdkGC            *gc;
  guint             entry_sig_id;
  gfloat            zoom_factor;  /* range from 0.1 to 4.0 */
  gint              col_width;
  gint              last_width;
  gint              columns;
  gboolean          freeze_update;
  gboolean          columns_valid;
};

/*  local function prototypes  */
static void   palette_entries_load           (const gchar    *filename);
static void   palette_save_palettes          (void);
static void   palette_entries_list_insert    (GimpPalette    *entries);

static void   palette_dialog_draw_entries    (PaletteDialog  *palette,
					      gint            row_start,
					      gint            column_highlight);
static void   palette_dialog_redraw          (PaletteDialog  *palette);
static void   palette_dialog_scroll_top_left (PaletteDialog  *palette);

static PaletteDialog * palette_dialog_new        (gboolean       editor);
static ImportDialog  * palette_import_dialog_new (PaletteDialog *palette);


GSList                *palettes_list           = NULL;

PaletteDialog         *top_level_edit_palette  = NULL;
PaletteDialog         *top_level_palette       = NULL;

static GimpPalette    *default_palette_entries = NULL;
static gint            num_palettes            = 0;
static ImportDialog   *import_dialog           = NULL;


/*  dnd stuff  */
static GtkTargetEntry color_palette_target_table[] =
{
  GIMP_TARGET_COLOR
};
static guint n_color_palette_targets = (sizeof (color_palette_target_table) /
					sizeof (color_palette_target_table[0]));

/*  public functions  ********************************************************/

void
palettes_init (gboolean no_data)
{
  if (!no_data)
    datafiles_read_directories (palette_path, palette_entries_load, 0);

  if (!default_palette_entries && palettes_list)
    default_palette_entries = palettes_list->data;
}

void
palettes_free (void)
{
  GimpPalette *entries;
  GSList      *list;

  for (list = palettes_list; list; list = g_slist_next (list))
    {
      entries = (GimpPalette *) list->data;

      if (entries->changed)
	gimp_palette_save (entries);

      gtk_object_unref (GTK_OBJECT (entries));
    }

  g_slist_free (palettes_list);

  num_palettes  = 0;
  palettes_list = NULL;
}

void 
palette_dialog_create (void)
{
  if (top_level_palette == NULL)
    {
      top_level_palette = palette_dialog_new (TRUE);
      session_set_window_geometry (top_level_palette->shell,
				   &palette_session_info, TRUE);
      dialog_register (top_level_palette->shell);

      gtk_widget_show (top_level_palette->shell);
    }
  else
    {
      if (! GTK_WIDGET_VISIBLE (top_level_palette->shell))
	{
	  gtk_widget_show (top_level_palette->shell);
	}
      else
	{
	  gdk_window_raise (top_level_palette->shell->window);
	}
    }
}

void
palette_dialog_free (void)
{
  if (top_level_edit_palette) 
    {
      if (import_dialog)
	{
	  gtk_widget_destroy (import_dialog->dialog);
	  g_free (import_dialog);
	  import_dialog = NULL;
	}

      gdk_gc_destroy (top_level_edit_palette->gc); 
      
      if (top_level_edit_palette->color_notebook) 
	color_notebook_free (top_level_edit_palette->color_notebook); 

      g_free (top_level_edit_palette); 
      top_level_edit_palette = NULL; 
    }

  if (top_level_palette)
    {
      gdk_gc_destroy (top_level_palette->gc); 
      session_get_window_info (top_level_palette->shell,
			       &palette_session_info);  

      if (top_level_palette->color_notebook) 
	color_notebook_free (top_level_palette->color_notebook); 

      g_free (top_level_palette);
      top_level_palette = NULL;
    }
}

static void
palette_entries_load (const gchar *filename)
{
  GimpPalette *palette;

  palette = gimp_palette_new_from_file (filename);

  if (palette)
    {
      palette_entries_list_insert (palette);

      /*  Check if the current palette is the default one  */
      if (default_palette && 
	  strcmp (default_palette, GIMP_OBJECT (palette)->name) == 0)
	{
	  default_palette_entries = palette;
	}
    }
}

static void
palette_save_palettes (void)
{
  GimpPalette *palette;
  GSList      *list;

  for (list = palettes_list; list; list = g_slist_next (list))
    {
      palette = (GimpPalette *) list->data;

      /*  If the palette has been changed, save it, if possible  */
      if (palette->changed)
	gimp_palette_save (palette);
    }
}

static void
palette_entries_list_insert (GimpPalette *palette)
{
  GimpPalette *palette2;
  GSList      *list;
  gint         pos = 0;

  for (list = palettes_list; list; list = g_slist_next (list))
    {
      palette2 = (GimpPalette *) list->data;

      /*  to make sure we get something!  */
      if (palette2 == NULL)
	palette2 = default_palette_entries;

      if (strcmp (GIMP_OBJECT (palette2)->name,
		  GIMP_OBJECT (palette)->name) > 0)
	break;

      pos++;
    }

  /*  add it to the list  */
  num_palettes++;

  palettes_list = g_slist_insert (palettes_list,
				  (gpointer) palette,
				  pos);
}

/*  general palette clist update functions  **********************************/

void
palette_clist_init (GtkWidget *clist, 
		    GtkWidget *shell,
		    GdkGC     *gc)
{
  GimpPalette *palette = NULL;
  GSList      *list;
  gint         pos;

  pos = 0;

  for (list = palettes_list; list; list = g_slist_next (list))
    {
      palette = (GimpPalette *) list->data;

      /*  to make sure we get something!  */
      if (! palette)
	palette = default_palette_entries;

      palette_clist_insert (clist, shell, gc, palette, pos);

      pos++;
    }
}

void
palette_clist_insert (GtkWidget   *clist, 
		      GtkWidget   *shell,
		      GdkGC       *gc,
		      GimpPalette *palette,
		      gint         pos)
{
  gchar *string[3];

  string[0] = NULL;
  string[1] = g_strdup_printf ("%d", palette->n_colors);
  string[2] = GIMP_OBJECT (palette)->name;

  gtk_clist_insert (GTK_CLIST (clist), pos, string);

  g_free (string[1]);

  if (palette->pixmap == NULL)
    {
      palette->pixmap = gdk_pixmap_new (shell->window,
					SM_PREVIEW_WIDTH, 
					SM_PREVIEW_HEIGHT, 
					gtk_widget_get_visual (shell)->depth);
      gimp_palette_update_preview (palette, gc);
    }

  gtk_clist_set_pixmap (GTK_CLIST (clist), pos, 0, palette->pixmap, NULL);
  gtk_clist_set_row_data (GTK_CLIST (clist), pos, (gpointer) palette);
}  

/*  palette dialog clist update functions  ***********************************/

static void
palette_dialog_clist_insert (PaletteDialog *palette_dialog,
			     GimpPalette   *palette)
{
  GimpPalette *chk_palette;
  GSList      *list;
  gint         pos;

  pos = 0;

  for (list = palettes_list; list; list = g_slist_next (list))
    {
      chk_palette = (GimpPalette *) list->data;
      
      /*  to make sure we get something!  */
      if (chk_palette == NULL)
	return;

      if (strcmp (GIMP_OBJECT (palette)->name,
		  GIMP_OBJECT (chk_palette)->name) == 0)
	break;

      pos++;
    }

  gtk_clist_freeze (GTK_CLIST (palette_dialog->clist));
  palette_clist_insert (palette_dialog->clist,
			palette_dialog->shell,
			palette_dialog->gc,
			palette,
			pos);
  gtk_clist_thaw (GTK_CLIST (palette_dialog->clist));
}

static void
palette_dialog_clist_set_text (PaletteDialog *palette_dialog,
			       GimpPalette   *palette)
{
  GimpPalette *chk_palette = NULL;
  GSList      *list;
  gchar       *num_buf;
  gint         pos;

  pos = 0;

  for (list = palettes_list; list; list = g_slist_next (list))
    {
      chk_palette = (GimpPalette *) list->data;

      if (palette == chk_palette)
	break;

      pos++;
    }

  if (chk_palette == NULL)
    return; /* This is actually and error */

  num_buf = g_strdup_printf ("%d", palette->n_colors);;

  gtk_clist_set_text (GTK_CLIST (palette_dialog->clist), pos, 1, num_buf);

  g_free (num_buf);
}

static void
palette_dialog_clist_refresh (PaletteDialog *palette_dialog)
{
  gtk_clist_freeze (GTK_CLIST (palette_dialog->clist));

  gtk_clist_clear (GTK_CLIST (palette_dialog->clist));
  palette_clist_init (palette_dialog->clist,
		      palette_dialog->shell,
		      palette_dialog->gc);

  gtk_clist_thaw (GTK_CLIST (palette_dialog->clist));

  palette_dialog->palette = (GimpPalette *) palettes_list->data;
}

static void 
palette_dialog_clist_scroll_to_current (PaletteDialog *palette_dialog)
{
  GimpPalette *palette;
  GSList      *list;
  gint         pos;

  if (! (palette_dialog && palette_dialog->palette))
    return;

  pos = 0;

  for (list = palettes_list; list; list = g_slist_next (list))
    {
      palette = (GimpPalette *) list->data;

      if (palette == palette_dialog->palette)
	break;

      pos++;
    }

  gtk_clist_unselect_all (GTK_CLIST (palette_dialog->clist));
  gtk_clist_select_row (GTK_CLIST (palette_dialog->clist), pos, -1);
  gtk_clist_moveto (GTK_CLIST (palette_dialog->clist), pos, 0, 0.0, 0.0); 
}

/*  update functions for all palette dialogs  ********************************/

static void
palette_insert_all (GimpPalette *palette)
{
  PaletteDialog *palette_dialog;

  if ((palette_dialog = top_level_palette))
    {
      palette_dialog_clist_insert (palette_dialog, palette);

      if (palette_dialog->palette == NULL)
	{
	  palette_dialog->palette = palette;
	  palette_dialog_redraw (palette_dialog);
	}
    }

  if ((palette_dialog = top_level_edit_palette))
    {
      palette_dialog_clist_insert (palette_dialog, palette);

      palette_dialog->palette = palette;
      palette_dialog_redraw (palette_dialog);

      palette_dialog_clist_scroll_to_current (palette_dialog);
    }

  /*  Update other selectors on screen  */
  palette_select_clist_insert_all (palette);
}

static void
palette_update_all (GimpPalette *palette)
{
  PaletteDialog *palette_dialog;
  GdkGC         *gc = NULL;

  if (top_level_palette)
    gc = top_level_palette->gc;
  else if (top_level_edit_palette)
    gc = top_level_edit_palette->gc;

  if (gc)
    gimp_palette_update_preview (palette, gc);

  if ((palette_dialog = top_level_palette))
    {
      if (palette_dialog->palette == palette)
	{
	  palette_dialog->columns_valid = FALSE;
	  palette_dialog_redraw (palette_dialog);
	}
      palette_dialog_clist_set_text (palette_dialog, palette);
    }

  if ((palette_dialog = top_level_edit_palette))
    {
      if (palette_dialog->palette == palette)
	{
	  palette_dialog->columns_valid = FALSE;
	  palette_dialog_redraw (palette_dialog);
	  palette_dialog_clist_scroll_to_current (palette_dialog);
	}
      palette_dialog_clist_set_text (palette_dialog, palette);
    }

  /*  Update other selectors on screen  */
  palette_select_set_text_all (palette);
}

static void
palette_draw_all (GimpPalette      *palette,
		  GimpPaletteEntry *entry)
{
  PaletteDialog *palette_dialog;
  GdkGC         *gc = NULL;

  if (top_level_palette)
    gc = top_level_palette->gc;
  else if (top_level_edit_palette)
    gc = top_level_edit_palette->gc;

  if (gc)
    gimp_palette_update_preview (palette, gc);

  if ((palette_dialog = top_level_palette))
    {
      if (palette_dialog->palette == palette)
	{
	  palette_dialog_draw_entries (palette_dialog,
				       entry->position / palette_dialog->columns,
				       entry->position % palette_dialog->columns);
	}
    }

  if ((palette_dialog = top_level_edit_palette))
    {
      if (palette_dialog->palette == palette)
	{
	  palette_dialog_draw_entries (palette_dialog,
				       entry->position / palette_dialog->columns,
				       entry->position % palette_dialog->columns);
	}
    }
}

static void
palette_refresh_all (void)
{
  PaletteDialog *palette_dialog;

  default_palette_entries = NULL;

  palettes_free ();
  palettes_init (FALSE);

  if ((palette_dialog = top_level_palette))
    {
      palette_dialog_clist_refresh (palette_dialog);
      palette_dialog->columns_valid = FALSE;
      palette_dialog_redraw (palette_dialog);
      palette_dialog_clist_scroll_to_current (palette_dialog);
    }

  if ((palette_dialog = top_level_edit_palette))
    {
      palette_dialog_clist_refresh (palette_dialog);
      palette_dialog->columns_valid = FALSE;
      palette_dialog_redraw (palette_dialog);
      palette_dialog_clist_scroll_to_current (palette_dialog);
    }

  /*  Update other selectors on screen  */
  palette_select_refresh_all ();
}

/*  called from color_picker.h  *********************************************/

void
palette_set_active_color (gint r,
			  gint g,
			  gint b,
			  gint state)
{
  GimpRGB color;

  gimp_rgba_set_uchar (&color,
		       (guchar) r,
		       (guchar) g,
		       (guchar) b,
		       255);

  if (top_level_edit_palette && top_level_edit_palette->palette) 
    {
      switch (state)
 	{
 	case COLOR_NEW:
	  top_level_edit_palette->color =
	    gimp_palette_add_entry (top_level_edit_palette->palette,
				    NULL,
				    &color);

	  palette_update_all (top_level_edit_palette->palette);
 	  break;

 	case COLOR_UPDATE_NEW:
 	  top_level_edit_palette->color->color = color;

	  palette_draw_all (top_level_edit_palette->palette,
			    top_level_edit_palette->color);
	  break;

 	default:
 	  break;
 	}
    }

  if (active_color == FOREGROUND)
    gimp_context_set_foreground (gimp_context_get_user (), &color);
  else if (active_color == BACKGROUND)
    gimp_context_set_background (gimp_context_get_user (), &color);
}

/*  called from palette_select.c  ********************************************/

void 
palette_select_palette_init (void)
{
  if (top_level_edit_palette == NULL)
    {
      top_level_edit_palette = palette_dialog_new (FALSE);
      dialog_register (top_level_edit_palette->shell);
    }
}

void 
palette_create_edit (GimpPalette *palette)
{
  if (top_level_edit_palette == NULL)
    {
      top_level_edit_palette = palette_dialog_new (FALSE);
      dialog_register (top_level_edit_palette->shell);

      gtk_widget_show (top_level_edit_palette->shell);
      palette_dialog_draw_entries (top_level_edit_palette, -1, -1);
    }
  else
    {
      if (! GTK_WIDGET_VISIBLE (top_level_edit_palette->shell))
	{
	  gtk_widget_show (top_level_edit_palette->shell);
	  palette_dialog_draw_entries (top_level_edit_palette, -1, -1);
	}
      else
	{
	  gdk_window_raise (top_level_edit_palette->shell->window);
	}
    }

  if (palette != NULL)
    {
      top_level_edit_palette->palette = palette;
      palette_dialog_clist_scroll_to_current (top_level_edit_palette);
    }
}

static void
palette_select_callback (ColorNotebook      *color_notebook,
			 const GimpRGB      *color,
			 ColorNotebookState  state,
			 gpointer            data)
{
  PaletteDialog *palette_dialog;
  
  palette_dialog = data;

  if (! (palette_dialog && palette_dialog->palette))
    return;

  switch (state)
    {
    case COLOR_NOTEBOOK_UPDATE:
      break;

    case COLOR_NOTEBOOK_OK:
      if (palette_dialog->color)
	{
	  palette_dialog->color->color = *color;

	  /*  Update either foreground or background colors  */
	  if (active_color == FOREGROUND)
	    gimp_context_set_foreground (gimp_context_get_user (), color);
	  else if (active_color == BACKGROUND)
	    gimp_context_set_background (gimp_context_get_user (), color);

	  palette_draw_all (palette_dialog->palette, palette_dialog->color);
	}

      /* Fallthrough */
    case COLOR_NOTEBOOK_CANCEL:
      if (palette_dialog->color_notebook_active)
	{
	  color_notebook_hide (palette_dialog->color_notebook);
	  palette_dialog->color_notebook_active = FALSE;
	}
    }
}

/*  the palette dialog popup menu & callbacks  *******************************/

static void
palette_dialog_new_entry_callback (GtkWidget *widget,
				   gpointer   data)
{
  PaletteDialog *palette_dialog;
  GimpRGB        color;

  palette_dialog = (PaletteDialog *) data;

  if (! (palette_dialog && palette_dialog->palette))
    return;

  if (active_color == FOREGROUND)
    gimp_context_get_foreground (gimp_context_get_user (), &color);
  else if (active_color == BACKGROUND)
    gimp_context_get_background (gimp_context_get_user (), &color);

  palette_dialog->color = gimp_palette_add_entry (palette_dialog->palette,
						  NULL,
						  &color);

  palette_update_all (palette_dialog->palette);
}

static void
palette_dialog_edit_entry_callback (GtkWidget *widget,
				    gpointer   data)
{
  PaletteDialog *palette_dialog;

  palette_dialog = (PaletteDialog *) data;

  if (! (palette_dialog && palette_dialog->palette && palette_dialog->color))
    return;

  if (! palette_dialog->color_notebook)
    {
      palette_dialog->color_notebook =
	color_notebook_new (_("Edit Palette Color"),
			    (const GimpRGB *) &palette_dialog->color->color,
			    palette_select_callback,
			    palette_dialog,
			    FALSE,
			    FALSE);
      palette_dialog->color_notebook_active = TRUE;
    }
  else
    {
      if (! palette_dialog->color_notebook_active)
	{
	  color_notebook_show (palette_dialog->color_notebook);
	  palette_dialog->color_notebook_active = TRUE;
	}

      color_notebook_set_color (palette_dialog->color_notebook,
				&palette_dialog->color->color);
    }
}

static void
palette_dialog_delete_entry_callback (GtkWidget *widget,
				      gpointer   data)
{
  PaletteDialog *palette_dialog;

  palette_dialog = (PaletteDialog *) data;

  if (! (palette_dialog && palette_dialog->palette && palette_dialog->color))
    return;

  gimp_palette_delete_entry (palette_dialog->palette,
			     palette_dialog->color);

  palette_update_all (palette_dialog->palette);
}

static void
palette_dialog_create_popup_menu (PaletteDialog *palette_dialog)
{
  GtkWidget *menu;
  GtkWidget *menu_item;

  palette_dialog->popup_menu = menu = gtk_menu_new ();

  menu_item = gtk_menu_item_new_with_label (_("New"));
  gtk_menu_append (GTK_MENU (menu), menu_item);
  gtk_signal_connect (GTK_OBJECT (menu_item), "activate", 
		      GTK_SIGNAL_FUNC (palette_dialog_new_entry_callback),
		      palette_dialog);
  gtk_widget_show (menu_item);

  menu_item = gtk_menu_item_new_with_label (_("Edit"));
  gtk_menu_append (GTK_MENU (menu), menu_item);

  gtk_signal_connect (GTK_OBJECT (menu_item), "activate", 
		      GTK_SIGNAL_FUNC (palette_dialog_edit_entry_callback),
		      palette_dialog);
  gtk_widget_show (menu_item);

  palette_dialog->edit_menu_item = menu_item;

  menu_item = gtk_menu_item_new_with_label (_("Delete"));
  gtk_signal_connect (GTK_OBJECT (menu_item), "activate", 
		      GTK_SIGNAL_FUNC (palette_dialog_delete_entry_callback),
		      palette_dialog);
  gtk_menu_append (GTK_MENU (menu), menu_item);
  gtk_widget_show (menu_item);

  palette_dialog->delete_menu_item = menu_item;
}

/*  the color area event callbacks  ******************************************/

static gint
palette_dialog_eventbox_button_press (GtkWidget      *widget,
				      GdkEventButton *bevent,
				      PaletteDialog  *palette_dialog)
{
  if (gtk_get_event_widget ((GdkEvent *) bevent) == palette_dialog->color_area)
    return FALSE;

  if (bevent->button == 3)
    {
      if (GTK_WIDGET_SENSITIVE (palette_dialog->edit_menu_item))
	{
	  gtk_widget_set_sensitive (palette_dialog->edit_menu_item, FALSE);
	  gtk_widget_set_sensitive (palette_dialog->delete_menu_item, FALSE);
	}

      gtk_menu_popup (GTK_MENU (palette_dialog->popup_menu), NULL, NULL, 
		      NULL, NULL, 3,
		      bevent->time);
    }

  return TRUE;
}

static gint
palette_dialog_color_area_events (GtkWidget     *widget,
				  GdkEvent      *event,
				  PaletteDialog *palette_dialog)
{
  GdkEventButton *bevent;
  GList          *list;
  gint            entry_width;
  gint            entry_height;
  gint            row, col;
  gint            pos;

  switch (event->type)
    {
    case GDK_EXPOSE:
      palette_dialog_redraw (palette_dialog);
      break;

    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *) event;
      entry_width  = palette_dialog->col_width + SPACING;
      entry_height = (ENTRY_HEIGHT * palette_dialog->zoom_factor) +  SPACING;
      col = (bevent->x - 1) / entry_width;
      row = (bevent->y - 1) / entry_height;
      pos = row * palette_dialog->columns + col;

      if (palette_dialog->palette)
	list = g_list_nth (palette_dialog->palette->colors, pos);
      else
	list = NULL;

      if (list)
	palette_dialog->dnd_color = list->data;
      else
	palette_dialog->dnd_color = NULL;

      if ((bevent->button == 1 || bevent->button == 3) &&
	  palette_dialog->palette)
	{
	  if (list)
	    {
	      if (palette_dialog->color)
		{
		  palette_dialog->freeze_update = TRUE;
 		  palette_dialog_draw_entries (palette_dialog, -1, -1);
		  palette_dialog->freeze_update = FALSE;
		}
	      palette_dialog->color = (GimpPaletteEntry *) list->data;

	      if (active_color == FOREGROUND)
		{
		  if (bevent->state & GDK_CONTROL_MASK)
		    gimp_context_set_background (gimp_context_get_user (),
						 &palette_dialog->color->color);
		  else
		    gimp_context_set_foreground (gimp_context_get_user (),
						 &palette_dialog->color->color);
		}
	      else if (active_color == BACKGROUND)
		{
		  if (bevent->state & GDK_CONTROL_MASK)
		    gimp_context_set_foreground (gimp_context_get_user (),
						 &palette_dialog->color->color);
		  else
		    gimp_context_set_background (gimp_context_get_user (),
						 &palette_dialog->color->color);
		}

	      palette_dialog_draw_entries (palette_dialog, row, col);
	      /*  Update the active color name  */
	      gtk_entry_set_text (GTK_ENTRY (palette_dialog->color_name),
				  palette_dialog->color->name);
	      gtk_widget_set_sensitive (palette_dialog->color_name, TRUE);
	      /* palette_update_current_entry (palette_dialog); */

	      if (bevent->button == 3)
		{
		  if (! GTK_WIDGET_SENSITIVE (palette_dialog->edit_menu_item))
		    {
		      gtk_widget_set_sensitive (palette_dialog->edit_menu_item, TRUE);
		      gtk_widget_set_sensitive (palette_dialog->delete_menu_item, TRUE);
		    }

		  gtk_menu_popup (GTK_MENU (palette_dialog->popup_menu),
				  NULL, NULL, 
				  NULL, NULL, 3,
				  bevent->time);
		}
	    }
	  else
	    {
	      if (bevent->button == 3)
		{
		  if (GTK_WIDGET_SENSITIVE (palette_dialog->edit_menu_item))
		    {
		      gtk_widget_set_sensitive (palette_dialog->edit_menu_item, FALSE);
		      gtk_widget_set_sensitive (palette_dialog->delete_menu_item, FALSE);
		    }

		  gtk_menu_popup (GTK_MENU (palette_dialog->popup_menu),
				  NULL, NULL, 
				  NULL, NULL, 3,
				  bevent->time);
		}
	    }
	}
      break;

    default:
      break;
    }

  return FALSE;
}

/*  functions for drawing & updating the palette dialog color area  **********/

static gint
palette_dialog_draw_color_row (guchar         *colors,
			       gint            n_colors,
			       gint            y,
			       gint            column_highlight,
			       guchar         *buffer,
			       PaletteDialog  *palette_dialog)
{
  guchar    *p;
  guchar     bcolor;
  gint       width, height;
  gint       entry_width;
  gint       entry_height;
  gint       vsize;
  gint       vspacing;
  gint       i, j;
  GtkWidget *preview;

  if (! palette_dialog)
    return -1;

  preview = palette_dialog->color_area;

  bcolor = 0;

  width        = preview->requisition.width;
  height       = preview->requisition.height;
  entry_width  = palette_dialog->col_width;
  entry_height = (ENTRY_HEIGHT * palette_dialog->zoom_factor);

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
	  
	  if (column_highlight >= 0)
	    {
	      guchar *ph;

	      ph = &buffer[3 * column_highlight * (entry_width + SPACING)];

	      for (j = 0 ; j <= entry_width + SPACING; j++)
		{
		  *ph++ = ~bcolor;
		  *ph++ = ~bcolor;
		  *ph++ = ~bcolor;
		}

 	      gtk_preview_draw_row (GTK_PREVIEW (preview), buffer, 0,
				    y + entry_height + 1, width); 
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
      for (i = 0; i < n_colors; i++)
	{
	  for (j = 0; j < SPACING; j++)
	    {
	      *p++ = bcolor;
	      *p++ = bcolor;
	      *p++ = bcolor;
	    }

	  for (j = 0; j < entry_width; j++)
	    {
	      *p++ = colors[i * 3];
	      *p++ = colors[i * 3 + 1];
	      *p++ = colors[i * 3 + 2];
	    }
	}

      for (i = 0; i < (palette_dialog->columns - n_colors); i++)
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
	  if (n_colors == column_highlight)
	    {
	      *p++ = ~bcolor;
	      *p++ = ~bcolor;
	      *p++ = ~bcolor;
	    }
	  else
	    {
	      *p++ = bcolor;
	      *p++ = bcolor;
	      *p++ = bcolor;
	    }
	}

      if (y < 0)
	y += entry_height - vsize;
      for (i = 0; i < vsize; i++, y++)
	{
	  if (column_highlight >= 0)
	    {
	      guchar *ph;

	      ph = &buffer[3 * column_highlight * (entry_width + SPACING)];

	      *ph++ = ~bcolor;
	      *ph++ = ~bcolor;
	      *ph++ = ~bcolor;
	      ph += 3 * (entry_width);
	      *ph++ = ~bcolor;
	      *ph++ = ~bcolor;
	      *ph++ = ~bcolor;
	    }

	  gtk_preview_draw_row (GTK_PREVIEW (preview), buffer, 0, y, width);
	}
      if (y > entry_height)
	y += entry_height - vsize;
    }
  else
    y += entry_height;

  return y;
}

static void
palette_dialog_draw_entries (PaletteDialog *palette_dialog,
			     gint           row_start,
			     gint           column_highlight)
{
  GimpPaletteEntry *entry;
  guchar           *buffer;
  guchar           *colors;
  GList            *list;
  gint              width, height;
  gint              entry_width;
  gint              entry_height;
  gint              index, y;

  if (! (palette_dialog && palette_dialog->palette))
    return;

  width  = palette_dialog->color_area->requisition.width;
  height = palette_dialog->color_area->requisition.height;

  entry_width  = palette_dialog->col_width;
  entry_height = (ENTRY_HEIGHT * palette_dialog->zoom_factor);

  if (entry_width <= 0)
    return;

  colors = g_new (guchar, palette_dialog->columns * 3);
  buffer = g_new (guchar, width * 3);

  if (row_start < 0)
    {
      y = 0;
      list = palette_dialog->palette->colors;
      column_highlight = -1;
    }
  else
    {
      y = (entry_height + SPACING) * row_start;
      list = g_list_nth (palette_dialog->palette->colors,
			 row_start * palette_dialog->columns);
    }

  index = 0;

  for (; list; list = g_list_next (list))
    {
      entry = (GimpPaletteEntry *) list->data;

      gimp_rgb_get_uchar (&entry->color,
			  &colors[index * 3],
			  &colors[index * 3 + 1],
			  &colors[index * 3 + 2]);
      index++;

      if (index == palette_dialog->columns)
	{
	  index = 0;
	  y = palette_dialog_draw_color_row (colors, palette_dialog->columns, y,
					     column_highlight, buffer,
					     palette_dialog);

	  if (y >= height || row_start >= 0)
	    {
	      /* This row only */
	      gtk_widget_draw (palette_dialog->color_area, NULL);
	      g_free (buffer);
	      g_free (colors);
	      return;
	    }
	}
    }

  while (y < height)
    {
      y = palette_dialog_draw_color_row (colors, index, y, column_highlight,
					 buffer, palette_dialog);
      index = 0;
      if (row_start >= 0)
	break;
    }

  g_free (buffer);
  g_free (colors);

  if (! palette_dialog->freeze_update)
    gtk_widget_draw (palette_dialog->color_area, NULL);
}

static void
palette_dialog_scroll_top_left (PaletteDialog *palette_dialog)
{
  GtkAdjustment *hadj;
  GtkAdjustment *vadj;

  if (! (palette_dialog && palette_dialog->scrolled_window))
    return;

  hadj = gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (palette_dialog->scrolled_window));
  vadj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (palette_dialog->scrolled_window));

  if (hadj)
    gtk_adjustment_set_value (hadj, 0.0);
  if (vadj)
    gtk_adjustment_set_value (vadj, 0.0);
}

static void
palette_dialog_redraw (PaletteDialog *palette_dialog)
{
  GtkWidget *parent;
  gint       vsize;
  gint       nrows;
  gint       n_entries;
  gint       preview_width;
  guint      width;

  if (! palette_dialog->palette)
    return;

  width = palette_dialog->color_area->parent->parent->parent->allocation.width;

  if ((palette_dialog->columns_valid) && palette_dialog->last_width == width)
    return;

  palette_dialog->last_width = width;
  palette_dialog->col_width = width / (palette_dialog->columns + 1) - SPACING;
  if (palette_dialog->col_width < 0) palette_dialog->col_width = 0;
  palette_dialog->columns_valid = TRUE;

  n_entries = palette_dialog->palette->n_colors;
  nrows = n_entries / palette_dialog->columns;
  if (n_entries % palette_dialog->columns)
    nrows += 1;

  vsize = nrows * (SPACING + (gint) (ENTRY_HEIGHT * palette_dialog->zoom_factor)) + SPACING;

  parent = palette_dialog->color_area->parent->parent;
  gtk_widget_ref (palette_dialog->color_area->parent);
  gtk_container_remove (GTK_CONTAINER (parent),
			palette_dialog->color_area->parent);

  preview_width =
    (palette_dialog->col_width + SPACING) * palette_dialog->columns + SPACING;

  gtk_preview_size (GTK_PREVIEW (palette_dialog->color_area),
		    preview_width, vsize);

  gtk_container_add (GTK_CONTAINER (parent), palette_dialog->color_area->parent);
  gtk_widget_unref (palette_dialog->color_area->parent);

  palette_dialog_draw_entries (palette_dialog, -1, -1);
}

/*  the palette dialog clist "select_row" callback  **************************/

static void
palette_dialog_list_item_update (GtkWidget      *widget, 
				 gint            row,
				 gint            column,
				 GdkEventButton *event,
				 gpointer        data)
{
  PaletteDialog *palette_dialog;
  GimpPalette   *palette;

  palette_dialog = (PaletteDialog *) data;

  if (palette_dialog->color_notebook_active)
    {
      color_notebook_hide (palette_dialog->color_notebook);
      palette_dialog->color_notebook_active = FALSE;
    }

  if (palette_dialog->color_notebook)
    color_notebook_free (palette_dialog->color_notebook);
  palette_dialog->color_notebook = NULL;

  palette =
    (GimpPalette *) gtk_clist_get_row_data (GTK_CLIST (palette_dialog->clist),
					    row);

  palette_dialog->palette       = palette;
  palette_dialog->columns_valid = FALSE;

  palette_dialog_redraw (palette_dialog);
  palette_dialog_scroll_top_left (palette_dialog);

  /*  Stop errors in case no colors are selected  */ 
  gtk_signal_handler_block (GTK_OBJECT (palette_dialog->color_name),
			    palette_dialog->entry_sig_id);
  gtk_entry_set_text (GTK_ENTRY (palette_dialog->color_name), _("Undefined")); 
  gtk_widget_set_sensitive (palette_dialog->color_name, FALSE);
  gtk_signal_handler_unblock (GTK_OBJECT (palette_dialog->color_name),
			      palette_dialog->entry_sig_id);
}

/*  the color name entry callback  *******************************************/

static void
palette_dialog_color_name_entry_changed (GtkWidget *widget,
					 gpointer   data)
{
  PaletteDialog *palette_dialog;

  palette_dialog = (PaletteDialog *) data;

  g_return_if_fail (palette_dialog->palette != NULL);

  if (palette_dialog->color->name)
    g_free (palette_dialog->color->name);
  palette_dialog->color->name = 
    g_strdup (gtk_entry_get_text (GTK_ENTRY (palette_dialog->color_name)));

  palette_dialog->palette->changed = TRUE;
}

/*  palette zoom functions & callbacks  **************************************/

static void
palette_dialog_redraw_zoom (PaletteDialog *palette_dialog)
{
  if (palette_dialog->zoom_factor > 4.0)
    {
      palette_dialog->zoom_factor = 4.0;
    }
  else if (palette_dialog->zoom_factor < 0.1)
    {
      palette_dialog->zoom_factor = 0.1;
    }

  palette_dialog->columns = COLUMNS;
  palette_dialog->columns_valid = FALSE;
  palette_dialog_redraw (palette_dialog); 

  palette_dialog_scroll_top_left (palette_dialog);
}

static void
palette_dialog_zoomin_callback (GtkWidget *widget,
				gpointer   data)
{
  PaletteDialog *palette_dialog;

  palette_dialog = (PaletteDialog *) data;

  palette_dialog->zoom_factor += 0.1;

  palette_dialog_redraw_zoom (palette_dialog);
}

static void
palette_dialog_zoomout_callback (GtkWidget *widget,
				 gpointer   data)
{
  PaletteDialog *palette_dialog;

  palette_dialog = (PaletteDialog *) data;

  palette_dialog->zoom_factor -= 0.1;

  palette_dialog_redraw_zoom (palette_dialog);
}

/*  the palette edit ops callbacks  ******************************************/

static void
palette_dialog_add_entries_callback (GtkWidget *widget,
				     gchar     *palette_name,
				     gpointer   data)
{
  GimpPalette *palette;

  palette = gimp_palette_new (palette_name);

  palette_entries_list_insert (palette);

  palette_insert_all (palette);
}

static void
palette_dialog_new_callback (GtkWidget *widget,
			     gpointer   data)
{
  GtkWidget *qbox;

  qbox = gimp_query_string_box (_("New Palette"),
				gimp_standard_help_func,
				"dialogs/palette_editor/new_palette.html",
				_("Enter a name for new palette"),
				NULL,
				NULL, NULL,
				palette_dialog_add_entries_callback, data);
  gtk_widget_show (qbox);
}

static void
palette_dialog_do_delete_callback (GtkWidget *widget,
				   gboolean   delete,
				   gpointer   data)
{
  PaletteDialog *palette_dialog;
  GimpPalette   *palette;

  palette_dialog = (PaletteDialog *) data;

  gtk_widget_set_sensitive (palette_dialog->shell, TRUE);

  if (! delete)
    return;

  if (! (palette_dialog && palette_dialog->palette))
    return;

  palette = palette_dialog->palette;

  if (palette->filename)
    gimp_palette_delete (palette);

  palettes_list = g_slist_remove (palettes_list, palette);

  gtk_object_unref (GTK_OBJECT (palette));

  palette_refresh_all ();
}

static void
palette_dialog_delete_callback (GtkWidget *widget,
				gpointer   data)
{
  PaletteDialog *palette_dialog;
  GtkWidget     *dialog;
  gchar         *str;

  palette_dialog = (PaletteDialog *) data;

  if (! (palette_dialog && palette_dialog->palette))
    return;

  gtk_widget_set_sensitive (palette_dialog->shell, FALSE);

  str = g_strdup_printf (_("Are you sure you want to delete\n"
                           "\"%s\" from the list and from disk?"),
                         GIMP_OBJECT (palette_dialog->palette)->name);

  dialog = gimp_query_boolean_box (_("Delete Palette"),
				   gimp_standard_help_func,
				   "dialogs/palette_editor/delete_palette.html",
				   FALSE,
				   str,
				   _("Delete"), _("Cancel"),
				   NULL, NULL,
				   palette_dialog_do_delete_callback,
				   palette_dialog);

  g_free (str);

  gtk_widget_show (dialog);
}

static void
palette_dialog_import_callback (GtkWidget *widget,
				gpointer   data)
{
  if (! import_dialog)
    {
      import_dialog = palette_import_dialog_new ((PaletteDialog *) data);
      gtk_widget_show (import_dialog->dialog);
    }
  else
    {
      gdk_window_raise (import_dialog->dialog->window);
    }
}

static void
palette_dialog_merge_entries_callback (GtkWidget *widget,
				       gchar     *palette_name,
				       gpointer   data)
{
  PaletteDialog    *palette_dialog;
  GimpPalette      *palette;
  GimpPalette      *new_palette;
  GimpPaletteEntry *entry;
  GList            *sel_list;

  new_palette = gimp_palette_new (palette_name);

  palette_dialog = (PaletteDialog *) data;

  sel_list = GTK_CLIST (palette_dialog->clist)->selection;

  while (sel_list)
    {
      gint   row;
      GList *cols;

      row = GPOINTER_TO_INT (sel_list->data);
      palette =
	(GimpPalette *) gtk_clist_get_row_data (GTK_CLIST (palette_dialog->clist), row);

      /* Go through each palette and merge the colors */
      for (cols = palette->colors; cols; cols = g_list_next (cols))
	{
	  entry = (GimpPaletteEntry *) cols->data;

	  gimp_palette_add_entry (new_palette,
				  entry->name,
				  &entry->color);
	}
      sel_list = sel_list->next;
    }

  palette_entries_list_insert (new_palette);

  palette_insert_all (new_palette);
}

static void
palette_dialog_merge_callback (GtkWidget *widget,
			       gpointer   data)
{
  GtkWidget *qbox;

  qbox = gimp_query_string_box (_("Merge Palette"),
				gimp_standard_help_func,
				"dialogs/palette_editor/merge_palette.html",
				_("Enter a name for merged palette"),
				NULL,
				NULL, NULL,
				palette_dialog_merge_entries_callback,
				data);
  gtk_widget_show (qbox);
}

/*  the palette & palette edit action area callbacks  ************************/

static void
palette_dialog_save_callback (GtkWidget *widget,
			      gpointer   data)
{
  palette_save_palettes ();
}

static void
palette_dialog_refresh_callback (GtkWidget *widget,
				 gpointer   data)
{
  palette_refresh_all ();
}

static void
palette_dialog_edit_callback (GtkWidget *widget,
			      gpointer   data)
{
  GimpPalette   *palette = NULL;
  PaletteDialog *palette_dialog;
  GList         *sel_list;

  palette_dialog = (PaletteDialog *) data;

  sel_list = GTK_CLIST (palette_dialog->clist)->selection;

  if (sel_list)
    {
      palette =
	(GimpPalette *) gtk_clist_get_row_data (GTK_CLIST (palette_dialog->clist),
						GPOINTER_TO_INT (sel_list->data));
    }

  palette_create_edit (palette);
}

static void
palette_dialog_close_callback (GtkWidget *widget,
			       gpointer   data)
{
  PaletteDialog *palette_dialog;

  palette_dialog = (PaletteDialog *) data;

  if (palette_dialog)
    {
      if (palette_dialog->color_notebook_active)
	{
	  color_notebook_hide (palette_dialog->color_notebook);
	  palette_dialog->color_notebook_active = FALSE;
	}

      if (palette_dialog == top_level_edit_palette && import_dialog)
	{
	  gtk_widget_destroy (import_dialog->dialog);
	  g_free (import_dialog);
	  import_dialog = NULL;
	}

      if (GTK_WIDGET_VISIBLE (palette_dialog->shell))
	gtk_widget_hide (palette_dialog->shell);
    }
}

/*  the palette dialog color dnd callbacks  **********************************/

static void
palette_dialog_drag_color (GtkWidget *widget,
			   GimpRGB   *color,
			   gpointer   data)
{
  PaletteDialog *palette_dialog;

  palette_dialog = (PaletteDialog *) data;

  if (palette_dialog && palette_dialog->palette && palette_dialog->dnd_color)
    {
      *color = palette_dialog->dnd_color->color;
    }
  else
    {
      gimp_rgba_set (color, 0.0, 0.0, 0.0, 1.0);
    }
}

static void
palette_dialog_drop_color (GtkWidget     *widget,
			   const GimpRGB *color,
			   gpointer       data)
{
  PaletteDialog *palette_dialog;

  palette_dialog = (PaletteDialog *) data;

  if (palette_dialog && palette_dialog->palette)
    {
      palette_dialog->color = gimp_palette_add_entry (palette_dialog->palette,
						      NULL,
						      (GimpRGB *) color);

      palette_update_all (palette_dialog->palette);
    }
}

/*  the palette & palette edit dialog constructor  ***************************/

PaletteDialog *
palette_dialog_new (gboolean editor)
{
  PaletteDialog *palette_dialog;
  GtkWidget     *hbox;
  GtkWidget     *hbox2;
  GtkWidget     *vbox;
  GtkWidget     *scrolledwindow;
  GtkWidget     *palette_region;
  GtkWidget     *entry;
  GtkWidget     *eventbox;
  GtkWidget     *alignment;
  GtkWidget     *frame;
  GtkWidget     *button;
  gchar         *titles[3];

  palette_dialog = g_new0 (PaletteDialog, 1);
  palette_dialog->palette       = default_palette_entries;
  palette_dialog->zoom_factor   = 1.0;
  palette_dialog->columns       = COLUMNS;
  palette_dialog->columns_valid = TRUE;
  palette_dialog->freeze_update = FALSE;

  if (!editor)
    {
      palette_dialog->shell =
	gimp_dialog_new (_("Color Palette Edit"), "color_palette_edit",
			 gimp_standard_help_func,
			 "dialogs/palette_editor/palette_editor.html",
			 GTK_WIN_POS_NONE,
			 FALSE, TRUE, FALSE,

			 _("Save"), palette_dialog_save_callback,
			 palette_dialog, NULL, NULL, FALSE, FALSE,
			 _("Refresh"), palette_dialog_refresh_callback,
			 palette_dialog, NULL, NULL, FALSE, FALSE,
			 _("Close"), palette_dialog_close_callback,
			 palette_dialog, NULL, NULL, TRUE, TRUE,

			 NULL);
    }
  else
    {
      palette_dialog->shell =
	gimp_dialog_new (_("Color Palette"), "color_palette",
			 gimp_standard_help_func,
			 "dialogs/palette_selection.html",
			 GTK_WIN_POS_NONE,
			 FALSE, TRUE, FALSE,

			 _("Edit"), palette_dialog_edit_callback,
			 palette_dialog, NULL, NULL, FALSE, FALSE,
			 _("Close"), palette_dialog_close_callback,
			 palette_dialog, NULL, NULL, TRUE, TRUE,

			 NULL);
     }

  /*  The main container widget  */
  if (editor)
    {
      hbox = gtk_notebook_new ();
      gtk_container_set_border_width (GTK_CONTAINER (hbox), 1);
    }
  else
    {
      hbox = gtk_hbox_new (FALSE, 4);
      gtk_container_set_border_width (GTK_CONTAINER (hbox), 4);
    }
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (palette_dialog->shell)->vbox),
		     hbox);
  gtk_widget_show (hbox);

  vbox = gtk_vbox_new (FALSE, 2);
  gtk_widget_show (vbox);

  palette_dialog->scrolled_window =
    scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_ALWAYS);
  gtk_box_pack_start (GTK_BOX (vbox), scrolledwindow, TRUE, TRUE, 0);
  gtk_widget_show (scrolledwindow);

  eventbox = gtk_event_box_new ();
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolledwindow),
					 eventbox);
  gtk_signal_connect (GTK_OBJECT (eventbox), "button_press_event",
		      GTK_SIGNAL_FUNC (palette_dialog_eventbox_button_press),
		      palette_dialog);
  gtk_widget_show (eventbox);

  alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0); 
  gtk_container_add (GTK_CONTAINER (eventbox), alignment);
  gtk_widget_show (alignment);

  palette_dialog->color_area = palette_region =
    gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_set_dither (GTK_PREVIEW (palette_dialog->color_area),
			  GDK_RGB_DITHER_MAX);
  gtk_preview_size (GTK_PREVIEW (palette_region), PREVIEW_WIDTH, PREVIEW_HEIGHT);
  
  gtk_widget_set_events (palette_region, PALETTE_EVENT_MASK);
  gtk_signal_connect (GTK_OBJECT (palette_dialog->color_area), "event",
		      GTK_SIGNAL_FUNC (palette_dialog_color_area_events),
		      palette_dialog);

  gtk_container_add (GTK_CONTAINER (alignment), palette_region);
  gtk_widget_show (palette_region);

  /*  dnd stuff  */
  gtk_drag_source_set (palette_region,
                       GDK_BUTTON1_MASK | GDK_BUTTON2_MASK,
                       color_palette_target_table, n_color_palette_targets,
                       GDK_ACTION_COPY | GDK_ACTION_MOVE);
  gimp_dnd_color_source_set (palette_region, palette_dialog_drag_color,
			     palette_dialog);

  gtk_drag_dest_set (alignment,
                     GTK_DEST_DEFAULT_HIGHLIGHT |
                     GTK_DEST_DEFAULT_MOTION |
                     GTK_DEST_DEFAULT_DROP,
                     color_palette_target_table, n_color_palette_targets,
                     GDK_ACTION_COPY);
  gimp_dnd_color_dest_set (alignment, palette_dialog_drop_color, palette_dialog);

  /*  The color name entry  */
  hbox2 = gtk_hbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox2, FALSE, FALSE, 0);
  gtk_widget_show (hbox2);

  entry = palette_dialog->color_name = gtk_entry_new ();
  gtk_widget_show (entry);
  gtk_box_pack_start (GTK_BOX (hbox2), entry, TRUE, TRUE, 0);
  gtk_entry_set_text (GTK_ENTRY (entry), _("Undefined"));
  gtk_widget_set_sensitive (entry, FALSE);
  palette_dialog->entry_sig_id =
    gtk_signal_connect (GTK_OBJECT (entry), "changed",
			GTK_SIGNAL_FUNC (palette_dialog_color_name_entry_changed),
			palette_dialog);

  /*  + and - buttons  */
  button = gimp_pixmap_button_new (zoom_in_xpm, NULL);
  gtk_box_pack_start (GTK_BOX (hbox2), button, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (palette_dialog_zoomin_callback),
		      palette_dialog);
  gtk_widget_show (button);

  button = gimp_pixmap_button_new (zoom_out_xpm, NULL);
  gtk_box_pack_start (GTK_BOX (hbox2), button, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (palette_dialog_zoomout_callback),
		      palette_dialog);
  gtk_widget_show (button);
  
  /*  clist preview of palettes  */
  scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_ALWAYS);

  if (editor)
    {
      gtk_notebook_append_page (GTK_NOTEBOOK (hbox), vbox,
				gtk_label_new (_("Palette")));
      gtk_notebook_append_page (GTK_NOTEBOOK (hbox), scrolledwindow,
				gtk_label_new (_("Select")));
    }
  else
    {
      gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
      gtk_box_pack_start (GTK_BOX (hbox), scrolledwindow, TRUE, TRUE, 0);
    }

  gtk_widget_show (scrolledwindow);

  titles[0] = _("Palette");
  titles[1] = _("Ncols");
  titles[2] = _("Name");
  palette_dialog->clist = gtk_clist_new_with_titles (3, titles);
  gtk_widget_set_usize (palette_dialog->clist, 203, 203);
  gtk_clist_set_row_height (GTK_CLIST (palette_dialog->clist),
			    SM_PREVIEW_HEIGHT + 2);
  gtk_clist_set_column_width (GTK_CLIST (palette_dialog->clist), 0,
			      SM_PREVIEW_WIDTH+2);
  gtk_clist_column_titles_passive (GTK_CLIST (palette_dialog->clist));
  gtk_container_add (GTK_CONTAINER (scrolledwindow), palette_dialog->clist);

  if (!editor)
    gtk_clist_set_selection_mode (GTK_CLIST (palette_dialog->clist),
				  GTK_SELECTION_EXTENDED);

  gtk_signal_connect (GTK_OBJECT (palette_dialog->clist), "select_row",
		      GTK_SIGNAL_FUNC (palette_dialog_list_item_update),
		      (gpointer) palette_dialog);
  gtk_widget_show (palette_dialog->clist);
  
  if (!editor) 
    {
      frame = gtk_frame_new (_("Palette Ops"));
      gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);
      gtk_widget_show (frame); 

      vbox = gtk_vbox_new (FALSE, 2);
      gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
      gtk_container_add (GTK_CONTAINER (frame), vbox);
      gtk_widget_show (vbox);
      
      button = gtk_button_new_with_label (_("New"));
      GTK_WIDGET_UNSET_FLAGS (button, GTK_RECEIVES_DEFAULT);
      gtk_misc_set_padding (GTK_MISC (GTK_BIN (button)->child), 2, 0);
      gtk_signal_connect (GTK_OBJECT (button), "clicked",
			  GTK_SIGNAL_FUNC (palette_dialog_new_callback),
			  (gpointer) palette_dialog);
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gimp_help_set_help_data (button, NULL,
			       "dialogs/palette_editor/new_palette.html");
      gtk_widget_show (button);

      button = gtk_button_new_with_label (_("Delete"));
      GTK_WIDGET_UNSET_FLAGS (button, GTK_RECEIVES_DEFAULT);
      gtk_misc_set_padding (GTK_MISC (GTK_BIN (button)->child), 2, 0);
      gtk_signal_connect (GTK_OBJECT (button), "clicked",
			  GTK_SIGNAL_FUNC (palette_dialog_delete_callback),
			  (gpointer) palette_dialog);
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gimp_help_set_help_data (button, NULL,
			       "dialogs/palette_editor/delete_palette.html");
      gtk_widget_show (button);
      
      button = gtk_button_new_with_label (_("Import"));
      GTK_WIDGET_UNSET_FLAGS (button, GTK_RECEIVES_DEFAULT);
      gtk_misc_set_padding (GTK_MISC (GTK_BIN (button)->child), 2, 0);
      gtk_signal_connect (GTK_OBJECT (button), "clicked",
			  GTK_SIGNAL_FUNC (palette_dialog_import_callback),
			  (gpointer) palette_dialog);
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gimp_help_set_help_data (button, NULL,
			       "dialogs/palette_editor/import_palette.html");
      gtk_widget_show (button);

      button = gtk_button_new_with_label (_("Merge"));
      GTK_WIDGET_UNSET_FLAGS (button, GTK_RECEIVES_DEFAULT);
      gtk_misc_set_padding (GTK_MISC (GTK_BIN (button)->child), 2, 0);
      gtk_signal_connect (GTK_OBJECT (button), "clicked",
			  GTK_SIGNAL_FUNC (palette_dialog_merge_callback),
			  (gpointer) palette_dialog);
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gimp_help_set_help_data (button, NULL,
			       "dialogs/palette_editor/merge_palette.html");
      gtk_widget_show (button);
    }

  gtk_widget_realize (palette_dialog->shell);

  palette_dialog->gc = gdk_gc_new (palette_dialog->shell->window);

  /*  fill the clist  */
  palette_clist_init (palette_dialog->clist,
		      palette_dialog->shell,
		      palette_dialog->gc);
  palette_dialog_clist_scroll_to_current (palette_dialog);

  palette_dialog_create_popup_menu (palette_dialog);

  return palette_dialog;
}

/*****************************************************************************/
/*  palette import dialog functions  *****************************************/

/*  functions to create & update the import dialog's gradient selection  *****/

static void
palette_import_select_grad_callback (GtkWidget *widget,
				     gpointer   data)
{
  /*  Popup grad edit box ....  */
  gradient_dialog_create ();
}

static void
palette_import_fill_grad_preview (GtkWidget  *preview,
				  gradient_t *gradient)
{
  guchar   buffer[3*IMPORT_PREVIEW_WIDTH];
  gint     loop;
  guchar  *p = buffer;
  gdouble  dx, cur_x;
  GimpRGB  color;

  dx    = 1.0/ (IMPORT_PREVIEW_WIDTH - 1);
  cur_x = 0;

  for (loop = 0 ; loop < IMPORT_PREVIEW_WIDTH; loop++)
    {
      gradient_get_color_at (gradient, cur_x, &color);

      *p++ = (guchar) (color.r * 255.999);
      *p++ = (guchar) (color.g * 255.999);
      *p++ = (guchar) (color.b * 255.999);

      cur_x += dx;
    }

  for (loop = 0 ; loop < IMPORT_PREVIEW_HEIGHT; loop++)
    {
      gtk_preview_draw_row (GTK_PREVIEW (preview), buffer, 0, loop,
			    IMPORT_PREVIEW_WIDTH);
    }

  gtk_widget_draw (preview, NULL);
}

static void
palette_import_gradient_update (GimpContext *context,
				gradient_t  *gradient,
				gpointer     data)
{
  if (import_dialog && import_dialog->import_type == GRAD_IMPORT)
    {
      /* redraw gradient */
      palette_import_fill_grad_preview (import_dialog->preview, gradient);
      gtk_entry_set_text (GTK_ENTRY (import_dialog->entry), gradient->name);
    }
}

/*  functions to create & update the import dialog's image selection  ********/

static void
palette_import_gimlist_cb (gpointer im,
			   gpointer data)
{
  GSList **l;

  l = (GSList**) data;
  *l = g_slist_prepend (*l, im);
}

static void
palette_import_gimlist_indexed_cb (gpointer im,
				   gpointer data)
{
  GimpImage  *gimage = GIMP_IMAGE (im);
  GSList    **l;

  if (gimp_image_base_type (gimage) == INDEXED)
    {
      l = (GSList**) data;
      *l = g_slist_prepend (*l, im);
    }
}

static void
palette_import_update_image_preview (GimpImage *gimage)
{
  TempBuf *preview_buf;
  gchar   *src, *buf;
  gint     x, y, has_alpha;
  gint     sel_width, sel_height;
  gint     pwidth, pheight;

  import_dialog->gimage = gimage;

  /* Calculate preview size */

  sel_width = gimage->width;
  sel_height = gimage->height;

  if (sel_width > sel_height)
    {
      pwidth  = MIN (sel_width, IMPORT_PREVIEW_WIDTH);
      pheight = sel_height * pwidth / sel_width;
    }
  else
    {
      pheight = MIN (sel_height, IMPORT_PREVIEW_HEIGHT);
      pwidth  = sel_width * pheight / sel_height;
    }
  
  /* Min size is 2 */
  preview_buf = gimp_image_construct_composite_preview (gimage,
							MAX (pwidth, 2),
							MAX (pheight, 2));

  gtk_preview_size (GTK_PREVIEW (import_dialog->preview),
		    preview_buf->width,
		    preview_buf->height);

  buf = g_new (gchar,  IMPORT_PREVIEW_WIDTH * 3);
  src = (gchar *) temp_buf_data (preview_buf);
  has_alpha = (preview_buf->bytes == 2 || preview_buf->bytes == 4);
  for (y = 0; y <preview_buf->height ; y++)
    {
      if (preview_buf->bytes == (1+has_alpha))
	for (x = 0; x < preview_buf->width; x++)
	  {
	    buf[x*3+0] = src[x];
	    buf[x*3+1] = src[x];
	    buf[x*3+2] = src[x];
	  }
      else
	for (x = 0; x < preview_buf->width; x++)
	  {
	    gint stride = 3 + has_alpha;
	    buf[x*3+0] = src[x*stride+0];
	    buf[x*3+1] = src[x*stride+1];
	    buf[x*3+2] = src[x*stride+2];
	  }
      gtk_preview_draw_row (GTK_PREVIEW (import_dialog->preview),
			    (guchar *)buf, 0, y, preview_buf->width);
      src += preview_buf->width * preview_buf->bytes;
    }

  g_free (buf);
  temp_buf_free (preview_buf);

  gtk_widget_hide (import_dialog->preview);
  gtk_widget_draw (import_dialog->preview, NULL); 
  gtk_widget_show (import_dialog->preview);
}

static void
palette_import_image_sel_callback (GtkWidget *widget,
				   gpointer   data)
{
  GimpImage *gimage;
  gchar     *lab;

  gimage = GIMP_IMAGE (data);
  palette_import_update_image_preview (gimage);

  lab = g_strdup_printf ("%s-%d",
			 g_basename (gimp_image_filename (import_dialog->gimage)),
			 pdb_image_to_id (import_dialog->gimage));

  gtk_entry_set_text (GTK_ENTRY (import_dialog->entry), lab);

  g_free (lab);
}

static void
palette_import_image_menu_add (GimpImage *gimage)
{
  GtkWidget *menuitem;
  gchar     *lab;

  lab= g_strdup_printf ("%s-%d",
			g_basename (gimp_image_filename (gimage)),
			pdb_image_to_id (gimage));

  menuitem = gtk_menu_item_new_with_label (lab);

  g_free (lab);

  gtk_widget_show (menuitem);
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
		      GTK_SIGNAL_FUNC (palette_import_image_sel_callback),
		      gimage);
  gtk_menu_append (GTK_MENU (import_dialog->optionmenu1_menu), menuitem);
}

/* Last Param gives us control over what goes in the menu on a delete oper */
static void
palette_import_image_menu_activate (gint        redo,
				    ImportType  type,
				    GimpImage  *del_image)
{
  GSList    *list      = NULL;
  gint       num_images;
  GimpImage *last_img  = NULL;
  GimpImage *first_img = NULL;
  gint       act_num   = -1;
  gint       count     = 0;
  gchar     *lab;

  if (! import_dialog)
    return;

  if (import_dialog->import_type == type && !redo)
    return;

  /* Destroy existing widget if necessary */
  if (import_dialog->image_list)
    {
      if (redo) /* Preserve settings in this case */
        last_img = import_dialog->gimage;
      gtk_widget_hide (import_dialog->image_list);
      gtk_widget_destroy (import_dialog->image_list);
      import_dialog->image_list = NULL;
    }

  import_dialog->import_type= type;

  /* Get list of images */
  if (import_dialog->import_type == INDEXED_IMPORT)
    {
      gimage_foreach (palette_import_gimlist_indexed_cb, &list);
    }
  else
    {
      gimage_foreach (palette_import_gimlist_cb, &list);
    }

  num_images = g_slist_length (list);
      
  if (num_images)
    {
      GtkWidget *optionmenu1;
      GtkWidget *optionmenu1_menu;
      gint       i;

      import_dialog->image_list = optionmenu1 = gtk_option_menu_new ();
      gtk_widget_set_usize (optionmenu1, IMPORT_PREVIEW_WIDTH, -1);
      import_dialog->optionmenu1_menu = optionmenu1_menu = gtk_menu_new ();

      for (i = 0; i < num_images; i++, list = g_slist_next (list))
        {
          if (GIMP_IMAGE (list->data) != del_image)
            {
	      if (first_img == NULL)
	        first_img = GIMP_IMAGE (list->data);
	      palette_import_image_menu_add (GIMP_IMAGE (list->data));
	      if (last_img == GIMP_IMAGE (list->data))
	        act_num = count;
	      else
	        count++;
	    }
	}

      gtk_option_menu_set_menu (GTK_OPTION_MENU (optionmenu1),
				optionmenu1_menu);
      gtk_widget_hide (import_dialog->select);
      gtk_box_pack_start (GTK_BOX (import_dialog->select_area),
            		  optionmenu1, FALSE, FALSE, 0);

      if(last_img != NULL && last_img != del_image)
        palette_import_update_image_preview (last_img);
      else if (first_img != NULL)
	palette_import_update_image_preview (first_img);

      gtk_widget_show (optionmenu1);

      /* reset to last one */
      if (redo && act_num >= 0)
        {
           gchar *lab;

	   lab = g_strdup_printf ("%s-%d",
				  g_basename (gimp_image_filename (import_dialog->gimage)),
				  pdb_image_to_id (import_dialog->gimage));

           gtk_option_menu_set_history (GTK_OPTION_MENU (optionmenu1), act_num);
           gtk_entry_set_text (GTK_ENTRY (import_dialog->entry), lab);

	   g_free (lab);
	}
    }
  g_slist_free (list);

  lab = g_strdup_printf ("%s-%d",
			 g_basename (gimp_image_filename (import_dialog->gimage)),
			 pdb_image_to_id (import_dialog->gimage));

  gtk_entry_set_text (GTK_ENTRY (import_dialog->entry), lab);

  g_free (lab);
}

/*  the import source menu item callbacks  ***********************************/

static void
palette_import_grad_callback (GtkWidget *widget,
			      gpointer   data)
{
  if (import_dialog)
    {
      gradient_t *gradient;

      gradient = gimp_context_get_gradient (gimp_context_get_user ());

      import_dialog->import_type = GRAD_IMPORT;
      if (import_dialog->image_list)
	{
	  gtk_widget_hide (import_dialog->image_list);
	  gtk_widget_destroy (import_dialog->image_list);
	  import_dialog->image_list = NULL;
	}
      gtk_widget_show (import_dialog->select);
      palette_import_fill_grad_preview (import_dialog->preview, gradient);

      gtk_entry_set_text (GTK_ENTRY (import_dialog->entry), gradient->name);
      gtk_widget_set_sensitive (import_dialog->threshold_scale, FALSE);
      gtk_widget_set_sensitive (import_dialog->threshold_text, FALSE);
    }
}

static void
palette_import_image_callback (GtkWidget *widget,
			       gpointer   data)
{
  palette_import_image_menu_activate (FALSE, IMAGE_IMPORT, NULL);
  gtk_widget_set_sensitive (import_dialog->threshold_scale, TRUE);
  gtk_widget_set_sensitive (import_dialog->threshold_text, TRUE);
}

static void
palette_import_indexed_callback (GtkWidget *widget,
				 gpointer   data)
{
  palette_import_image_menu_activate (FALSE, INDEXED_IMPORT, NULL);
  gtk_widget_set_sensitive (import_dialog->threshold_scale, FALSE);
  gtk_widget_set_sensitive (import_dialog->threshold_text, FALSE);
}

/*  functions & callbacks to keep the import dialog uptodate  ****************/

static gint
palette_import_image_count (ImportType type)
{
  GSList *list       = NULL;
  gint    num_images = 0;

  if (type == INDEXED_IMPORT)
    {
      gimage_foreach (palette_import_gimlist_indexed_cb, &list);
    }
  else
    {
      gimage_foreach (palette_import_gimlist_cb, &list);
    }

  num_images = g_slist_length (list);

  g_slist_free (list);

  return num_images;
}

static void
palette_import_image_new (GimpSet   *set,
			  GimpImage *gimage,
			  gpointer   data)
{
  if (! import_dialog)
    return;

  if (! GTK_WIDGET_IS_SENSITIVE (import_dialog->image_menu_item_image))
    {
      gtk_widget_set_sensitive (import_dialog->image_menu_item_image, TRUE);
      return;
    }

  if (! GTK_WIDGET_IS_SENSITIVE (import_dialog->image_menu_item_indexed) &&
      gimp_image_base_type(gimage) == INDEXED)
    {
      gtk_widget_set_sensitive (import_dialog->image_menu_item_indexed, TRUE);
      return;
    }

  /* Now fill in the names if image menu shown */
  if (import_dialog->import_type == IMAGE_IMPORT ||
      import_dialog->import_type == INDEXED_IMPORT)
    {
      palette_import_image_menu_activate (TRUE, import_dialog->import_type,
					  NULL);
    }
}

static void
palette_import_image_destroyed (GimpSet   *set,
				GimpImage *gimage,
				gpointer   data)
{
  if (! import_dialog)
    return;

  if (palette_import_image_count (import_dialog->import_type) <= 1)
    {
      /* Back to gradient type */
      gtk_option_menu_set_history (GTK_OPTION_MENU (import_dialog->type_option), 0);
      palette_import_grad_callback (NULL, NULL);
      if (import_dialog->image_menu_item_image)
	gtk_widget_set_sensitive (import_dialog->image_menu_item_image, FALSE);
      return;
    }

  if (import_dialog->import_type == IMAGE_IMPORT ||
      import_dialog->import_type == INDEXED_IMPORT)
    {
      palette_import_image_menu_activate (TRUE, import_dialog->import_type,
					  gimage);
    }
}

void
palette_import_image_renamed (GimpImage* gimage)
{
  /* Now fill in the names if image menu shown */
  if (import_dialog &&
      (import_dialog->import_type == IMAGE_IMPORT ||
       import_dialog->import_type == INDEXED_IMPORT))
    {
      palette_import_image_menu_activate (TRUE, import_dialog->import_type,
					  NULL);
    }
}

/*  create a palette from a gradient  ****************************************/

static void
palette_import_create_from_grad (gchar *name)
{
  GimpPalette *palette;
  gradient_t  *gradient;

  gradient = gimp_context_get_gradient (gimp_context_get_user ());

  if (gradient)
    {
      /* Add names to entry */
      gdouble dx, cur_x;
      GimpRGB color;
      gint    sample_sz;
      gint    loop;

      palette = gimp_palette_new (name);

      sample_sz = (gint) import_dialog->sample->value;  

      dx    = 1.0 / (sample_sz - 1);
      cur_x = 0;
      
      for (loop = 0; loop < sample_sz; loop++)
	{
	  gradient_get_color_at (gradient, cur_x, &color);

	  cur_x += dx;
	  gimp_palette_add_entry (palette, NULL, &color);
	}

      palette_entries_list_insert (palette);

      palette_insert_all (palette);
    }
}

/*  create a palette from a non-indexed image  *******************************/

typedef struct _ImgColors ImgColors;

struct _ImgColors
{
  guint  count;
  guint  r_adj;
  guint  g_adj;
  guint  b_adj;
  guchar r;
  guchar g;
  guchar b;
};

static gint count_color_entries = 0;

static GHashTable *
palette_import_store_colors (GHashTable *h_array, 
			     guchar     *colors,
			     guchar     *colors_real, 
			     gint        sample_sz)
{
  gpointer   found_color = NULL;
  ImgColors *new_color;
  guint      key_colors = colors[0] * 256 * 256 + colors[1] * 256 + colors[2];

  if (h_array == NULL)
    {
      h_array = g_hash_table_new (g_direct_hash, g_direct_equal);
      count_color_entries = 0;
    }
  else
    {
      found_color = g_hash_table_lookup (h_array, (gpointer) key_colors);
    }

  if (found_color == NULL)
    {
      if (count_color_entries > MAX_IMAGE_COLORS)
	{
	  /* Don't add any more new ones */
	  return h_array;
	}

      count_color_entries++;

      new_color = g_new (ImgColors, 1);

      new_color->count = 1;
      new_color->r_adj = 0;
      new_color->g_adj = 0;
      new_color->b_adj = 0;
      new_color->r = colors[0];
      new_color->g = colors[1];
      new_color->b = colors[2];

      g_hash_table_insert (h_array, (gpointer) key_colors, new_color);
    }
  else
    {
      new_color = (ImgColors *) found_color;
      if(new_color->count < (G_MAXINT - 1))
	new_color->count++;
      
      /* Now do the adjustments ...*/
      new_color->r_adj += (colors_real[0] - colors[0]);
      new_color->g_adj += (colors_real[1] - colors[1]);
      new_color->b_adj += (colors_real[2] - colors[2]);

      /* Boundary conditions */
      if(new_color->r_adj > (G_MAXINT - 255))
	new_color->r_adj /= new_color->count;

      if(new_color->g_adj > (G_MAXINT - 255))
	new_color->g_adj /= new_color->count;

      if(new_color->b_adj > (G_MAXINT - 255))
	new_color->b_adj /= new_color->count;
    }

  return h_array;
}

static void
palette_import_create_sorted_list (gpointer key,
				   gpointer value,
				   gpointer user_data)
{
  GSList    **sorted_list = (GSList**) user_data;
  ImgColors  *color_tab   = (ImgColors *) value;

  *sorted_list = g_slist_prepend (*sorted_list, color_tab);
}

static gint
palette_import_sort_colors (gconstpointer a,
			    gconstpointer b)
{
  ImgColors *s1 = (ImgColors *) a;
  ImgColors *s2 = (ImgColors *) b;

  if(s1->count > s2->count)
    return -1;
  if(s1->count < s2->count)
    return 1;

  return 0;
}

static void
palette_import_create_image_palette (gpointer data,
				     gpointer user_data)
{
  GimpPalette *palette;
  ImgColors   *color_tab;
  gint         sample_sz;
  gchar       *lab;
  GimpRGB      color;

  palette   = (GimpPalette *) user_data;
  color_tab = (ImgColors *) data;

  sample_sz = (gint) import_dialog->sample->value;  

  if (palette->n_colors >= sample_sz)
    return;

  lab = g_strdup_printf ("%s (occurs %u)", _("Untitled"), color_tab->count);

  /* Adjust the colors to the mean of the the sample */
  gimp_rgba_set_uchar
    (&color,
     (guchar) color_tab->r + (color_tab->r_adj / color_tab->count), 
     (guchar) color_tab->g + (color_tab->g_adj / color_tab->count), 
     (guchar) color_tab->b + (color_tab->b_adj / color_tab->count),
     255);

  gimp_palette_add_entry (palette, lab, &color);

  g_free (lab);
}

static gboolean
palette_import_color_print_remove (gpointer key,
				   gpointer value,
				   gpointer user_data)
{
  g_free (value);

  return TRUE;
}

static void
palette_import_image_make_palette (GHashTable *h_array,
				   guchar     *name)
{
  GimpPalette *palette;
  GSList      *sorted_list = NULL;

  g_hash_table_foreach (h_array, palette_import_create_sorted_list,
			&sorted_list);
  sorted_list = g_slist_sort (sorted_list, palette_import_sort_colors);

  palette = gimp_palette_new (name);

  g_slist_foreach (sorted_list, palette_import_create_image_palette, palette);

  /*  Free up used memory
   *  Note the same structure is on both the hash list and the sorted
   *  list. So only delete it once.
   */
  g_hash_table_freeze (h_array);
  g_hash_table_foreach_remove (h_array,
			       palette_import_color_print_remove, NULL);
  g_hash_table_thaw (h_array);
  g_hash_table_destroy (h_array);
  g_slist_free (sorted_list);

  palette_entries_list_insert (palette);

  palette_insert_all (palette);
}

static void
palette_import_create_from_image (GImage *gimage,
				  gchar  *pname)
{
  PixelRegion  imagePR;
  guchar      *image_data;
  guchar      *idata;
  guchar       rgb[MAX_CHANNELS];
  guchar       rgb_real[MAX_CHANNELS];
  gint         has_alpha, indexed;
  gint         width, height;
  gint         bytes, alpha;
  gint         i, j;
  void        *pr;
  gint         d_type;
  GHashTable  *store_array = NULL;
  gint         sample_sz;
  gint         threshold = 1;

  sample_sz = (gint) import_dialog->sample->value;  

  if (gimage == NULL)
    return;

  /*  Get the image information  */
  bytes = gimp_image_composite_bytes (gimage);
  d_type = gimp_image_composite_type (gimage);
  has_alpha = (d_type == RGBA_GIMAGE ||
	       d_type == GRAYA_GIMAGE ||
	       d_type == INDEXEDA_GIMAGE);
  indexed = d_type == INDEXEDA_GIMAGE || d_type == INDEXED_GIMAGE;
  width = gimage->width;
  height = gimage->height;
  pixel_region_init (&imagePR, gimp_image_composite (gimage), 0, 0,
		     width, height, FALSE);

  alpha = bytes - 1;

  threshold = (gint) import_dialog->threshold->value;

  if(threshold < 1)
    threshold = 1;

  /*  iterate over the entire image  */
  for (pr = pixel_regions_register (1, &imagePR);
       pr != NULL;
       pr = pixel_regions_process (pr))
    {
      image_data = imagePR.data;

      for (i = 0; i < imagePR.h; i++)
	{
	  idata = image_data;

	  for (j = 0; j < imagePR.w; j++)
	    {
	      /*  Get the rgb values for the color  */
	      gimp_image_get_color (gimage, d_type, rgb, idata);
	      memcpy (rgb_real, rgb, MAX_CHANNELS); /* Structure copy */

	      rgb[0] = (rgb[0] / threshold) * threshold;
	      rgb[1] = (rgb[1] / threshold) * threshold;
	      rgb[2] = (rgb[2] / threshold) * threshold;

	      store_array =
		palette_import_store_colors (store_array, rgb, rgb_real,
					     sample_sz);

	      idata += bytes;
	    }

	  image_data += imagePR.rowstride;
	}
    }

  /*  Make palette from the store_array  */
  palette_import_image_make_palette (store_array, pname);
}

/*  create a palette from an indexed image  **********************************/

static void
palette_import_create_from_indexed (GImage *gimage,
				    gchar  *pname)
{
  GimpPalette *palette;
  gint         samples;
  gint         count;
  GimpRGB      color;

  samples = (gint) import_dialog->sample->value;  

  if (gimage == NULL)
    return;

  if (gimp_image_base_type (gimage) != INDEXED)
    return;

  palette = gimp_palette_new (pname);

  for (count= 0; count < samples && count < gimage->num_cols; ++count)
    {
      gimp_rgba_set_uchar (&color,
			   gimage->cmap[count*3],
			   gimage->cmap[count*3+1],
			   gimage->cmap[count*3+2],
			   255);

      gimp_palette_add_entry (palette, NULL, &color);
    }

  palette_entries_list_insert (palette);

  palette_insert_all (palette);
}

/*  the palette import action area callbacks  ********************************/

static void
palette_import_close_callback (GtkWidget *widget,
			      gpointer   data)
{
  gtk_widget_destroy (import_dialog->dialog);
  g_free (import_dialog);
  import_dialog = NULL;
}

static void
palette_import_import_callback (GtkWidget *widget,
				gpointer   data)
{
  PaletteDialog *palette_dialog;

  palette_dialog = (PaletteDialog *) data;

  if (import_dialog)
    {
      gchar *pname;

      pname = gtk_entry_get_text (GTK_ENTRY (import_dialog->entry));
      if (!pname || !strlen (pname))
	pname = g_strdup ("tmp");
      else
	pname = g_strdup (pname);

      switch (import_dialog->import_type)
	{
	case GRAD_IMPORT:
	  palette_import_create_from_grad (pname);
	  break;
	case IMAGE_IMPORT:
	  palette_import_create_from_image (import_dialog->gimage, pname);
	  break;
	case INDEXED_IMPORT:
	  palette_import_create_from_indexed (import_dialog->gimage, pname);
	  break;
	default:
	  break;
	}

      palette_import_close_callback (NULL, NULL);
    }
}

/*  the palette import dialog constructor  ***********************************/

static ImportDialog *
palette_import_dialog_new (PaletteDialog *palette)
{
  GtkWidget *dialog;
  GtkWidget *hbox;
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *table;
  GtkWidget *label;
  GtkWidget *spinbutton;
  GtkWidget *button;
  GtkWidget *entry;
  GtkWidget *optionmenu;
  GtkWidget *optionmenu_menu;
  GtkWidget *menuitem;
  GtkWidget *image;
  GtkWidget *hscale;

  import_dialog = g_new (ImportDialog, 1);
  import_dialog->image_list = NULL;
  import_dialog->gimage     = NULL;

  import_dialog->dialog = dialog =
    gimp_dialog_new (_("Import Palette"), "import_palette",
		     gimp_standard_help_func,
		     "dialogs/palette_editor/import_palette.html",
		     GTK_WIN_POS_NONE,
		     FALSE, TRUE, FALSE,

		     _("Import"), palette_import_import_callback,
		     palette, NULL, NULL, FALSE, FALSE,
		     _("Close"), palette_import_close_callback,
		     palette, NULL, NULL, TRUE, TRUE,

		     NULL);

  /*  The main hbox  */
  hbox = gtk_hbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 4);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), hbox);
  gtk_widget_show (hbox);

  /*  The "Import" frame  */
  frame = gtk_frame_new (_("Import"));
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  table = gtk_table_new (4, 2, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /*  The source's name  */
  label = gtk_label_new (_("Name:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND, 0, 0);
  gtk_widget_show (label);

  entry = import_dialog->entry = gtk_entry_new ();
  gtk_table_attach_defaults (GTK_TABLE (table), entry, 1, 2, 0, 1);
  {
    gradient_t* gradient;

    gradient = gimp_context_get_gradient (gimp_context_get_current ());
    gtk_entry_set_text (GTK_ENTRY (entry),
			gradient ? gradient->name : _("new_import"));
  }
  gtk_widget_show (entry);

  /*  The source type  */
  label = gtk_label_new (_("Source:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND, 0, 0);
  gtk_widget_show (label);

  optionmenu = import_dialog->type_option = gtk_option_menu_new ();
  optionmenu_menu = gtk_menu_new ();
  gtk_table_attach_defaults (GTK_TABLE (table), optionmenu, 1, 2, 1, 2);
  menuitem = import_dialog->image_menu_item_gradient = 
    gtk_menu_item_new_with_label (_("Gradient"));
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
		      GTK_SIGNAL_FUNC (palette_import_grad_callback),
		      NULL);
  gtk_menu_append (GTK_MENU (optionmenu_menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = import_dialog->image_menu_item_image =
    gtk_menu_item_new_with_label (_("Image"));
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
		      GTK_SIGNAL_FUNC (palette_import_image_callback),
		      (gpointer) import_dialog);
  gtk_menu_append (GTK_MENU (optionmenu_menu), menuitem);
  gtk_widget_show (menuitem);
  gtk_widget_set_sensitive (menuitem,
			    palette_import_image_count (IMAGE_IMPORT) > 0);

  menuitem = import_dialog->image_menu_item_indexed =
    gtk_menu_item_new_with_label (_("Indexed Palette"));
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
                      GTK_SIGNAL_FUNC (palette_import_indexed_callback),
                      (gpointer) import_dialog);
  gtk_menu_append (GTK_MENU (optionmenu_menu), menuitem);
  gtk_widget_show (menuitem);
  gtk_widget_set_sensitive (menuitem,
			    palette_import_image_count (INDEXED_IMPORT) > 0);

  gtk_option_menu_set_menu (GTK_OPTION_MENU (optionmenu), optionmenu_menu);
  gtk_widget_show (optionmenu);

  /*  The sample size  */
  label = gtk_label_new (_("Sample Size:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND, 0, 0);
  gtk_widget_show (label);

  import_dialog->sample =
    GTK_ADJUSTMENT(gtk_adjustment_new (256, 2, 10000, 1, 10, 10));
  spinbutton = gtk_spin_button_new (import_dialog->sample, 1, 0);
  gtk_table_attach_defaults (GTK_TABLE (table), spinbutton, 1, 2, 2, 3);
  gtk_widget_show (spinbutton);

  /*  The interval  */
  label = import_dialog->threshold_text = gtk_label_new (_("Interval:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 1.0);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 3, 4,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_set_sensitive(label, FALSE);
  gtk_widget_show (label);

  import_dialog->threshold = 
    GTK_ADJUSTMENT (gtk_adjustment_new (1, 1, 128, 1, 1, 1));
  hscale = import_dialog->threshold_scale = 
    gtk_hscale_new (import_dialog->threshold);
  gtk_scale_set_value_pos (GTK_SCALE (hscale), GTK_POS_TOP);
  gtk_scale_set_digits (GTK_SCALE (hscale), 0);
  gtk_table_attach_defaults (GTK_TABLE (table), hscale, 1, 2, 3, 4);
  gtk_widget_set_sensitive (hscale, FALSE);
  gtk_widget_show (hscale);

  /*  The preview frame  */
  frame = gtk_frame_new (_("Preview"));
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  vbox = import_dialog->select_area = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  image = import_dialog->preview = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_set_dither (GTK_PREVIEW (image), GDK_RGB_DITHER_MAX);
  gtk_preview_size (GTK_PREVIEW (image),
		    IMPORT_PREVIEW_WIDTH, IMPORT_PREVIEW_HEIGHT);
  gtk_widget_set_usize (image, IMPORT_PREVIEW_WIDTH, IMPORT_PREVIEW_HEIGHT);
  gtk_box_pack_start (GTK_BOX (vbox), image, FALSE, FALSE, 0);
  gtk_widget_show (image);

  button = import_dialog->select = gtk_button_new_with_label (_("Select"));
  GTK_WIDGET_UNSET_FLAGS (button, GTK_RECEIVES_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked", 
		      GTK_SIGNAL_FUNC (palette_import_select_grad_callback),
		      (gpointer) image);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  /*  Fill with the selected gradient  */
  palette_import_fill_grad_preview
    (image, gimp_context_get_gradient (gimp_context_get_user ()));
  import_dialog->import_type = GRAD_IMPORT;
  gtk_signal_connect (GTK_OBJECT (gimp_context_get_user ()), "gradient_changed",
		      GTK_SIGNAL_FUNC (palette_import_gradient_update),
		      NULL);

  /*  keep the dialog up-to-date  */
  gtk_signal_connect (GTK_OBJECT (image_context), "add",
		      GTK_SIGNAL_FUNC (palette_import_image_new),
		      NULL);
  gtk_signal_connect (GTK_OBJECT (image_context), "remove",
		      GTK_SIGNAL_FUNC (palette_import_image_destroyed),
		      NULL);

  return import_dialog;
}
