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

#include <stdio.h>
#include <sys/types.h>

#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"

#include "apptypes.h"

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
#include "palette.h"
#include "paletteP.h"
#include "palette_import.h"
#include "session.h"
#include "palette_select.h"

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
static void   palette_load                   (const gchar    *filename);
static void   palette_save_palettes          (void);

static void   palette_dialog_draw_entries    (PaletteDialog  *palette,
					      gint            row_start,
					      gint            column_highlight);
static void   palette_dialog_redraw          (PaletteDialog  *palette);
static void   palette_dialog_scroll_top_left (PaletteDialog  *palette);

static void   palette_insert_all             (GimpPalette    *palette);

static PaletteDialog * palette_dialog_new        (gboolean       editor);


GSList                *palettes_list           = NULL;

PaletteDialog         *top_level_edit_palette  = NULL;
PaletteDialog         *top_level_palette       = NULL;

static GimpPalette    *default_palette_entries = NULL;
static gint            num_palettes            = 0;


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
    datafiles_read_directories (palette_path, palette_load, 0);

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
      palette_import_dialog_destroy ();

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
palette_load (const gchar *filename)
{
  GimpPalette *palette;

  palette = gimp_palette_new_from_file (filename);

  if (palette)
    {
      palettes_list_insert (palette);

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

void
palettes_list_insert (GimpPalette *palette)
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

  /*  update all dialogs  */
  palette_insert_all (palette);
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

  palettes_list_insert (palette);
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
  palette_import_dialog_show ();
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

  palettes_list_insert (new_palette);
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

      if (palette_dialog == top_level_edit_palette)
	{
	  palette_import_dialog_destroy ();
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
			  palette_dialog);
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gimp_help_set_help_data (button, NULL,
			       "dialogs/palette_editor/new_palette.html");
      gtk_widget_show (button);

      button = gtk_button_new_with_label (_("Delete"));
      GTK_WIDGET_UNSET_FLAGS (button, GTK_RECEIVES_DEFAULT);
      gtk_misc_set_padding (GTK_MISC (GTK_BIN (button)->child), 2, 0);
      gtk_signal_connect (GTK_OBJECT (button), "clicked",
			  GTK_SIGNAL_FUNC (palette_dialog_delete_callback),
			  palette_dialog);
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gimp_help_set_help_data (button, NULL,
			       "dialogs/palette_editor/delete_palette.html");
      gtk_widget_show (button);
      
      button = gtk_button_new_with_label (_("Import"));
      GTK_WIDGET_UNSET_FLAGS (button, GTK_RECEIVES_DEFAULT);
      gtk_misc_set_padding (GTK_MISC (GTK_BIN (button)->child), 2, 0);
      gtk_signal_connect (GTK_OBJECT (button), "clicked",
			  GTK_SIGNAL_FUNC (palette_dialog_import_callback),
			  palette_dialog);
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gimp_help_set_help_data (button, NULL,
			       "dialogs/palette_editor/import_palette.html");
      gtk_widget_show (button);

      button = gtk_button_new_with_label (_("Merge"));
      GTK_WIDGET_UNSET_FLAGS (button, GTK_RECEIVES_DEFAULT);
      gtk_misc_set_padding (GTK_MISC (GTK_BIN (button)->child), 2, 0);
      gtk_signal_connect (GTK_OBJECT (button), "clicked",
			  GTK_SIGNAL_FUNC (palette_dialog_merge_callback),
			  palette_dialog);
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
