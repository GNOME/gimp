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
#include "appenv.h"
#include "color_area.h"
#include "color_notebook.h"
#include "datafiles.h"
#include "general.h"
#include "gimpcontext.h"
#include "gimpdnd.h"
#include "gimprc.h"
#include "gimpui.h"
#include "gradient_header.h"
#include "gradient_select.h"
#include "palette.h"
#include "paletteP.h"
#include "palette_entries.h"
#include "session.h"
#include "palette_select.h"
#include "dialog_handler.h"

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
#define MAX_IMAGE_COLORS (10000*2)

typedef enum
{
  GRAD_IMPORT = 0,
  IMAGE_IMPORT = 1,
  INDEXED_IMPORT = 2,
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
  GtkWidget      *shell;
  GtkWidget      *color_area;
  GtkWidget      *scrolled_window;
  GtkWidget      *color_name;
  GtkWidget      *clist;
  GtkWidget      *popup_menu;
  GtkWidget      *popup_small_menu;
  ColorNotebookP  color_notebook;
  gboolean        color_notebook_active;
  PaletteEntries *entries;
  PaletteEntry   *color;
  GdkGC          *gc;
  guint           entry_sig_id;
  gfloat          zoom_factor;  /* range from 0.1 to 4.0 */
  gfloat          xzoom_factor;  
  gint            last_width;
  gint            columns;
  gboolean        freeze_update;
  gboolean        columns_valid;
};

/*  local function prototypes  */
static void  palette_entry_free    (PaletteEntry *);
static void  palette_entries_free  (PaletteEntries *);
static void  palette_entries_load  (gchar *);
static void  palette_entries_save  (PaletteEntries *, gchar *);
static void  palette_save_palettes ();

static void  palette_entries_list_insert    (PaletteEntries *entries);

static void  palette_dialog_draw_entries    (PaletteDialog *palette,
					     gint           row_start,
					     gint           column_highlight);
static void  palette_dialog_redraw          (PaletteDialog *palette);
static void  palette_dialog_scroll_top_left (PaletteDialog *palette);

static PaletteDialog * palette_dialog_new        (gboolean vert);
static ImportDialog  * palette_import_dialog_new (PaletteDialog *palette);


GSList                *palette_entries_list    = NULL;
PaletteDialog         *top_level_edit_palette  = NULL;
PaletteDialog         *top_level_palette       = NULL;

static PaletteEntries *default_palette_entries = NULL;
static gint            num_palette_entries     = 0;
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
palettes_init (gint no_data)
{
  if (!no_data)
    datafiles_read_directories (palette_path, palette_entries_load, 0);
}

void
palette_init_palettes (gint no_data)
{
  palettes_init (no_data);
}

void
palettes_free (void)
{
  PaletteEntries *entries;
  GSList *list;

  for (list = palette_entries_list; list; list = g_slist_next (list))
    {
      entries = (PaletteEntries *) list->data;

      /*  If the palette has been changed, save it, if possible  */
      if (entries->changed)
	/*  save the palette  */
	palette_entries_save (entries, entries->filename);

      palette_entries_free (entries);
    }

  g_slist_free (palette_entries_list);

  num_palette_entries = 0;
  palette_entries_list = NULL;
}

void
palette_free_palettes (void)
{
  palettes_free ();
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
palette_dialog_free ()
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

/*  palette entries functions  ***********************************************/

static PaletteEntries *
palette_entries_new (gchar *palette_name)
{
  PaletteEntries *entries = NULL;
  gchar *home;
  gchar *local_path;
  gchar *first_token;
  gchar *token;
  gchar *path;

  if (palette_name)
    {
      entries = g_new (PaletteEntries, 1);
      if (palette_path)
	{
	  /*  Get the first path specified in the palette path list  */
	  home = g_get_home_dir ();
	  local_path = g_strdup (palette_path);
	  first_token = local_path;
	  token = xstrsep (&first_token, G_SEARCHPATH_SEPARATOR_S);

	  if (token)
	    {
	      if (*token == '~')
		{
		  if (home)
		    path = g_strdup_printf ("%s%s", home, token + 1);
		  else
		    /* Just ignore the ~ if no HOME ??? */
		    path = g_strdup (token + 1);
		}
	      else
		{
		  path = g_strdup (token);
		}

	      entries->filename = g_strdup_printf ("%s" G_DIR_SEPARATOR_S "%s",
						   path, palette_name);

	      g_free (path);
	    }

	  g_free (local_path);
	}
      else
	entries->filename = NULL;

      entries->name = palette_name;  /*  don't need to copy because this memory is ours  */
      entries->colors = NULL;
      entries->n_colors = 0;
      entries->changed = TRUE;
      entries->pixmap = NULL;

      palette_entries_list_insert (entries);

      palette_save_palettes ();
    }

  return entries;
}

static void
palette_entries_free (PaletteEntries *entries)
{
  PaletteEntry *entry;
  GSList *list;

  for (list = entries->colors; list; list = g_slist_next (list))
    {
      entry = (PaletteEntry *) list->data;

      palette_entry_free (entry);
    }

  g_free (entries->name);

  if (entries->filename)
    g_free (entries->filename);

  g_free (entries);
}

static PaletteEntry *
palette_entries_add_entry (PaletteEntries *entries,
			   gchar          *name,
			   gint            r,
			   gint            g,
			   gint            b)
{
  PaletteEntry *entry;

  if (entries)
    {
      entry = g_new (PaletteEntry, 1);

      entry->color[0] = r;
      entry->color[1] = g;
      entry->color[2] = b;
      entry->name     = g_strdup (name ? name : _("Untitled"));
      entry->position = entries->n_colors;

      entries->colors    = g_slist_append (entries->colors, entry);
      entries->n_colors += 1;
      entries->changed   = TRUE;
      
      return entry;
    }

  return NULL;
}

static void
palette_entries_load (gchar *filename)
{
  PaletteEntries *entries;
  gchar   str[512];
  gchar  *tok;
  FILE   *fp;
  gint    r, g, b;
  gint    linenum;

  r = g = b = 0;

  entries = g_new (PaletteEntries, 1);

  entries->filename = g_strdup (filename);
  entries->name     = g_strdup (g_basename (filename));
  entries->colors   = NULL;
  entries->n_colors = 0;
  entries->pixmap   = NULL;

  /*  Open the requested file  */
  if (! (fp = fopen (filename, "r")))
    {
      palette_entries_free (entries);
      g_warning ("failed to open palette file %s: can't happen?", filename);
      return;
    }

  linenum = 0;

  fread (str, 13, 1, fp);
  str[13] = '\0';
  linenum++;
  if (strcmp (str, "GIMP Palette\n"))
    {
      /* bad magic, but maybe it has \r\n at the end of lines? */
      if (!strcmp (str, "GIMP Palette\r"))
	g_message (_("Loading palette %s:\n"
		     "Corrupt palette:\n"
		     "missing magic header\n"
		     "Does this file need converting from DOS?"), filename);
      else
	g_message (_("Loading palette %s:\n"
		     "Corrupt palette: missing magic header"), filename);
      fclose (fp);
      palette_entries_free (entries);
      return;
    }

  while (!feof (fp))
    {
      if (!fgets (str, 512, fp))
	{
	  if (feof (fp))
	    break;
	  g_message (_("Loading palette %s (line %d):\nRead error"),
		     filename, linenum);
	  fclose (fp);
	  palette_entries_free (entries);
	  return;
	}

      linenum++;

      if (str[0] != '#')
	{
	  tok = strtok (str, " \t");
	  if (tok)
	    r = atoi (tok);
	  else
	    /* maybe we should just abort? */
	    g_message (_("Loading palette %s (line %d):\n"
			 "Missing RED component"), filename, linenum);

	  tok = strtok (NULL, " \t");
	  if (tok)
	    g = atoi (tok);
	  else
	    g_message (_("Loading palette %s (line %d):\n"
			 "Missing GREEN component"), filename, linenum);
	  
	  tok = strtok (NULL, " \t");
	  if (tok)
	    b = atoi (tok);
	  else
	    g_message (_("Loading palette %s (line %d):\n"
			 "Missing BLUE component"), filename, linenum);

	  /* optional name */
	  tok = strtok (NULL, "\n");

	  if (r < 0 || r > 255 ||
	      g < 0 || g > 255 ||
	      b < 0 || b > 255)
	    g_message (_("Loading palette %s (line %d):\n"
			 "RGB value out of range"), filename, linenum);

	  palette_entries_add_entry (entries, tok, r, g, b);
	}
    }

  /*  Clean up  */
  fclose (fp);
  entries->changed = FALSE;

  palette_entries_list_insert (entries);

  /*  Check if the current palette is the default one  */
  if (strcmp (default_palette, g_basename (filename)) == 0)
    default_palette_entries = entries;
}

static void
palette_entry_free (PaletteEntry *entry)
{
  if (entry->name)
    g_free (entry->name);

  g_free (entry);
}

static void
palette_entries_delete (gchar *filename)
{
  if (filename)
    unlink (filename);
}

static void
palette_entries_save (PaletteEntries *palette,
		      gchar          *filename)
{
  PaletteEntry *entry;
  GSList *list;
  FILE *fp;

  if (! filename)
    return;

  /*  Open the requested file  */
  if (! (fp = fopen (filename, "w")))
    {
      g_message (_("can't save palette \"%s\"\n"), filename);
      return;
    }

  fprintf (fp, "GIMP Palette\n");
  fprintf (fp, "# %s -- GIMP Palette file\n", palette->name);

  for (list = palette->colors; list; list = g_slist_next (list))
    {
      entry = (PaletteEntry *) list->data;

      fprintf (fp, "%d %d %d\t%s\n",
	       entry->color[0], entry->color[1], entry->color[2], entry->name);
    }

  /*  Clean up  */
  fclose (fp);
}

static void
palette_save_palettes ()
{
  PaletteEntries *entries;
  GSList *list;

  for (list = palette_entries_list; list; list = g_slist_next (list))
    {
      entries = (PaletteEntries *) list->data;

      /*  If the palette has been changed, save it, if possible  */
      if (entries->changed)
	/*  save the palette  */
	palette_entries_save (entries, entries->filename);
    }
}

static void 
palette_entries_update_small_preview (PaletteEntries *entries,
				      GdkGC          *gc)
{
  guchar rgb_buf[SM_PREVIEW_WIDTH * SM_PREVIEW_HEIGHT * 3];
  GSList *tmp_link;
  gint index;
  PaletteEntry *entry;

  memset (rgb_buf, 0x0, sizeof (rgb_buf));

  gdk_draw_rgb_image (entries->pixmap,
		      gc,
		      0,
		      0,
		      SM_PREVIEW_WIDTH,
		      SM_PREVIEW_HEIGHT,
		      GDK_RGB_DITHER_NORMAL,
		      rgb_buf,
		      SM_PREVIEW_WIDTH*3);

  
  index = 0;
  for (tmp_link = entries->colors; tmp_link; tmp_link = g_slist_next (tmp_link))
    {
      guchar cell[3*3*3];
      gint loop;

      entry = tmp_link->data;
      
      for (loop = 0; loop < 27 ; loop+=3)
	{
	  cell[0+loop] = entry->color[0];
	  cell[1+loop] = entry->color[1];
	  cell[2+loop] = entry->color[2];
	}

      gdk_draw_rgb_image (entries->pixmap,
			  gc,
			  1+(index%((SM_PREVIEW_WIDTH-2)/3))*3,
			  1+(index/((SM_PREVIEW_WIDTH-2)/3))*3,
			  3,
			  3,
			  GDK_RGB_DITHER_NORMAL,
			  cell,
			  3);

      index++;

      if (index >= (((SM_PREVIEW_WIDTH-2)*(SM_PREVIEW_HEIGHT-2))/9))
	break;
    }
}

static void
palette_entries_list_insert (PaletteEntries *entries)
{
  PaletteEntries *p_entries;
  GSList *list;
  gint pos = 0;

  for (list = palette_entries_list; list; list = g_slist_next (list))
    {
      p_entries = (PaletteEntries *) list->data;
      
      /*  to make sure we get something!  */
      if (p_entries == NULL)
	p_entries = default_palette_entries;

      if (strcmp (p_entries->name, entries->name) > 0)
	break;

      pos++;
    }

  /*  add it to the list  */
  num_palette_entries++;
  palette_entries_list = g_slist_insert (palette_entries_list,
					 (gpointer) entries, pos);
}

/*  general palette clist update functions  **********************************/

void
palette_clist_init (GtkWidget *clist, 
		    GtkWidget *shell,
		    GdkGC     *gc)
{
  PaletteEntries *p_entries = NULL;
  GSList *list;
  gint pos;

  pos = 0;
  for (list = palette_entries_list; list; list = g_slist_next (list))
    {
      p_entries = (PaletteEntries *) list->data;

      /*  to make sure we get something!  */
      if (p_entries == NULL)
	p_entries = default_palette_entries;

      palette_clist_insert (clist, shell, gc, p_entries, pos);

      pos++;
    }
}

void
palette_clist_insert (GtkWidget      *clist, 
		      GtkWidget      *shell,
		      GdkGC          *gc,
		      PaletteEntries *entries,
		      gint            pos)
{
  gchar *string[3];

  string[0] = NULL;
  string[1] = g_strdup_printf ("%d", entries->n_colors);
  string[2] = entries->name;

  gtk_clist_insert (GTK_CLIST (clist), pos, string);

  g_free (string[1]);
  
  if (entries->pixmap == NULL)
    {
      entries->pixmap = gdk_pixmap_new (shell->window,
					SM_PREVIEW_WIDTH, 
					SM_PREVIEW_HEIGHT, 
					gtk_widget_get_visual (shell)->depth);
      palette_entries_update_small_preview (entries, gc);
    }
  
  gtk_clist_set_pixmap (GTK_CLIST (clist), pos, 0, entries->pixmap, NULL);
  gtk_clist_set_row_data (GTK_CLIST (clist), pos, (gpointer) entries);
}  

/*  palette dialog clist update functions  ***********************************/

static void
palette_dialog_clist_insert (PaletteDialog  *palette,
			     PaletteEntries *entries)
{
  PaletteEntries *chk_entries;
  GSList *list;
  gint pos;

  pos = 0;
  for (list = palette_entries_list; list; list = g_slist_next (list))
    {
      chk_entries = (PaletteEntries *) list->data;
      
      /*  to make sure we get something!  */
      if (chk_entries == NULL)
	return;

      if (strcmp (entries->name, chk_entries->name) == 0)
	break;

      pos++;
    }

  gtk_clist_freeze (GTK_CLIST (palette->clist));
  palette_clist_insert (palette->clist, palette->shell, palette->gc,
			entries, pos);
  gtk_clist_thaw (GTK_CLIST (palette->clist));
}

static void
palette_dialog_clist_set_text (PaletteDialog  *palette,
			       PaletteEntries *entries)
{
  PaletteEntries *chk_entries = NULL;
  GSList *list;
  gchar *num_buf;
  gint pos;

  pos = 0;
  for (list = palette_entries_list; list; list = g_slist_next (list))
    {
      chk_entries = (PaletteEntries *) list->data;
      
      if (entries == chk_entries)
	break;

      pos++;
    }

  if (chk_entries == NULL)
    return; /* This is actually and error */

  num_buf = g_strdup_printf ("%d", entries->n_colors);;

  gtk_clist_set_text (GTK_CLIST (palette->clist), pos, 1, num_buf);

  g_free (num_buf);
}

static void
palette_dialog_clist_refresh (PaletteDialog *palette)
{
  gtk_clist_freeze (GTK_CLIST (palette->clist));
  gtk_clist_clear (GTK_CLIST (palette->clist));
  palette_clist_init (palette->clist, palette->shell, palette->gc);
  gtk_clist_thaw (GTK_CLIST (palette->clist));

  palette->entries = palette_entries_list->data;
}

static void 
palette_dialog_clist_scroll_to_current (PaletteDialog *palette)
{
  PaletteEntries *p_entries;
  GSList *list;
  gint pos;

  if (palette && palette->entries)
    {
      pos = 0;
      for (list = palette_entries_list; list; list = g_slist_next (list))
	{
	  p_entries = (PaletteEntries *) list->data;
	  
	  if (p_entries == palette->entries)
	    break;

	  pos++;
	}

      gtk_clist_unselect_all (GTK_CLIST (palette->clist));
      gtk_clist_select_row (GTK_CLIST (palette->clist), pos, -1);
      gtk_clist_moveto (GTK_CLIST (palette->clist), pos, 0, 0.0, 0.0); 
    }
}

/*  update functions for all palette dialogs  ********************************/

static void
palette_insert_all (PaletteEntries *entries)
{
  PaletteDialog *palette;

  if ((palette = top_level_palette))
    {
      palette_dialog_clist_insert (palette, entries);

      if (palette->entries == NULL)
	{
	  palette->entries = entries;
	  palette_dialog_redraw (palette);
	}
    }

  if ((palette = top_level_edit_palette))
    {
      palette_dialog_clist_insert (palette, entries);

      palette->entries = entries;
      palette_dialog_redraw (palette);

      palette_dialog_clist_scroll_to_current (palette);
    }

  /*  Update other selectors on screen  */
  palette_select_clist_insert_all (entries);
}

static void
palette_update_all (PaletteEntries *entries)
{
  PaletteDialog *palette;
  GdkGC *gc = NULL;

  if (top_level_palette)
    gc = top_level_palette->gc;
  else if (top_level_edit_palette)
    gc = top_level_edit_palette->gc;

  if (gc)
    palette_entries_update_small_preview (entries, gc);

  if ((palette = top_level_palette))
    {
      if (palette->entries == entries)
	{
	  palette->columns_valid = FALSE;
	  palette_dialog_redraw (palette);
	}
      palette_dialog_clist_set_text (palette, entries);
    }

  if ((palette = top_level_edit_palette))
    {
      if (palette->entries == entries)
	{
	  palette->columns_valid = FALSE;
	  palette_dialog_redraw (palette);
	  palette_dialog_clist_scroll_to_current (palette);
	}
      palette_dialog_clist_set_text (palette, entries);
    }

  /*  Update other selectors on screen  */
  palette_select_set_text_all (entries);
}

static void
palette_draw_all (PaletteEntries *entries,
		  PaletteEntry   *color)
{
  PaletteDialog *palette;
  GdkGC *gc = NULL;

  if (top_level_palette)
    gc = top_level_palette->gc;
  else if (top_level_edit_palette)
    gc = top_level_edit_palette->gc;

  if (gc)
    palette_entries_update_small_preview (entries, gc);

  if ((palette = top_level_palette))
    {
      if (palette->entries == entries)
	{
	  palette_dialog_draw_entries (palette,
				       color->position / palette->columns,
				       color->position % palette->columns);
	}
    }

  if ((palette = top_level_edit_palette))
    {
      if (palette->entries == entries)
	{
	  palette_dialog_draw_entries (palette,
				       color->position / palette->columns,
				       color->position % palette->columns);
	}
    }
}

static void
palette_refresh_all ()
{
  PaletteDialog *palette;

  default_palette_entries = NULL;

  palettes_free ();
  palettes_init (FALSE);

  if ((palette = top_level_palette))
    {
      palette_dialog_clist_refresh (palette);
      palette->columns_valid = FALSE;
      palette_dialog_redraw (palette);
      palette_dialog_clist_scroll_to_current (palette);
    }

  if ((palette = top_level_edit_palette))
    {
      palette_dialog_clist_refresh (palette);
      palette->columns_valid = FALSE;
      palette_dialog_redraw (palette);
      palette_dialog_clist_scroll_to_current (palette);
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
  if (top_level_edit_palette && top_level_edit_palette->entries) 
    {
      switch (state) 
 	{ 
 	case COLOR_NEW: 
 	  top_level_edit_palette->color =
	    palette_entries_add_entry (top_level_edit_palette->entries,
				       _("Untitled"), r, g, b); 
	  palette_update_all (top_level_edit_palette->entries);
 	  break;

 	case COLOR_UPDATE_NEW: 
 	  top_level_edit_palette->color->color[0] = r;
 	  top_level_edit_palette->color->color[1] = g;
 	  top_level_edit_palette->color->color[2] = b;
	  palette_draw_all (top_level_edit_palette->entries,
			    top_level_edit_palette->color);
 	  break; 
	  
 	default: 
 	  break; 
 	} 
    } 
  
  if (active_color == FOREGROUND)
    gimp_context_set_foreground (gimp_context_get_user (), r, g, b);
  else if (active_color == BACKGROUND)
    gimp_context_set_background (gimp_context_get_user (), r, g, b);
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
palette_create_edit (PaletteEntries *entries)
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

  if (entries != NULL)
    {
      top_level_edit_palette->entries = entries;
      palette_dialog_clist_scroll_to_current (top_level_edit_palette);
    }
}

static void
palette_select_callback (gint                r,
			 gint                g,
			 gint                b,
			 ColorNotebookState  state,
			 void               *data)
{
  PaletteDialog *palette;
  guchar *color;
  
  palette = data;

  if (palette && palette->entries)
    {
      switch (state)
	{
	case COLOR_NOTEBOOK_UPDATE:
	  break;

	case COLOR_NOTEBOOK_OK:
	  if (palette->color)
	    {
	      color = palette->color->color;

	      color[0] = r;
	      color[1] = g;
	      color[2] = b;

	      /*  Update either foreground or background colors  */
	      if (active_color == FOREGROUND)
		gimp_context_set_foreground (gimp_context_get_user (), r, g, b);
	      else if (active_color == BACKGROUND)
		gimp_context_set_background (gimp_context_get_user (), r, g, b);

	      palette_draw_all (palette->entries, palette->color);
	    }

	  /* Fallthrough */
	case COLOR_NOTEBOOK_CANCEL:
	  if (palette->color_notebook_active)
	    {
	      color_notebook_hide (palette->color_notebook);
	      palette->color_notebook_active = FALSE;
	    }
	}
    }
}

/*  the palette dialog popup menu & callbacks  *******************************/

static void
palette_dialog_new_entry_callback (GtkWidget *widget,
				   gpointer   data)
{
  PaletteDialog *palette;

  palette = data;

  if (palette && palette->entries)
    {
      guchar col[3];

      if (active_color == FOREGROUND)
	gimp_context_get_foreground (gimp_context_get_user (),
				     &col[0], &col[1], &col[2]);
      else if (active_color == BACKGROUND)
	gimp_context_get_background (gimp_context_get_user (),
				     &col[0], &col[1], &col[2]);

      palette->color = palette_entries_add_entry (palette->entries,
						  _("Untitled"),
						  col[0], col[1], col[2]);

      palette_update_all (palette->entries);
    }
}

static void
palette_dialog_edit_entry_callback (GtkWidget *widget,
				    gpointer   data)
{
  PaletteDialog *palette;
  guchar *color;

  palette = data;
  if (palette && palette->entries && palette->color)
    {
      color = palette->color->color;

      if (!palette->color_notebook)
	{
	  palette->color_notebook =
	    color_notebook_new (color[0], color[1], color[2],
				palette_select_callback, palette,
				FALSE);
	  palette->color_notebook_active = TRUE;
	}
      else
	{
	  if (!palette->color_notebook_active)
	    {
	      color_notebook_show (palette->color_notebook);
	      palette->color_notebook_active = TRUE;
	    }

	  color_notebook_set_color (palette->color_notebook,
				    color[0], color[1], color[2], 1);
	}
    }
}

static void
palette_dialog_delete_entry_callback (GtkWidget *widget,
				      gpointer   data)
{
  PaletteEntry *entry;
  PaletteDialog *palette;
  GSList *tmp_link;
  gint pos = 0;

  palette = data;

  if (palette && palette->entries && palette->color)
    {
      entry = palette->color;
      palette->entries->colors = g_slist_remove (palette->entries->colors, entry);
      palette->entries->n_colors--;
      palette->entries->changed = TRUE;

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
	palette->color = palette_entries_add_entry (palette->entries,
						    _("Black"), 0, 0, 0);

      palette_update_all (palette->entries);
    }
}

static void
palette_dialog_create_popup_menu (PaletteDialog *palette)
{
  GtkWidget *menu;
  GtkWidget *menu_items;

  menu = gtk_menu_new ();
  menu_items = gtk_menu_item_new_with_label (_("Edit"));
  gtk_menu_append (GTK_MENU (menu), menu_items);

  gtk_signal_connect (GTK_OBJECT (menu_items), "activate", 
		      GTK_SIGNAL_FUNC (palette_dialog_edit_entry_callback),
		      (gpointer) palette);
  gtk_widget_show (menu_items);

  menu_items = gtk_menu_item_new_with_label (_("New"));
  gtk_menu_append (GTK_MENU (menu), menu_items);
  gtk_signal_connect (GTK_OBJECT (menu_items), "activate", 
		      GTK_SIGNAL_FUNC (palette_dialog_new_entry_callback),
		      (gpointer) palette);
  gtk_widget_show (menu_items);

  menu_items = gtk_menu_item_new_with_label (_("Delete"));
  gtk_signal_connect (GTK_OBJECT(menu_items), "activate", 
		      GTK_SIGNAL_FUNC (palette_dialog_delete_entry_callback),
		      (gpointer) palette);
  gtk_menu_append (GTK_MENU (menu), menu_items);
  gtk_widget_show (menu_items);

  palette->popup_menu = menu;

  palette->popup_small_menu = menu = gtk_menu_new ();
  menu_items = gtk_menu_item_new_with_label (_("New"));
  gtk_menu_append (GTK_MENU (menu), menu_items);
  gtk_signal_connect (GTK_OBJECT (menu_items), "activate", 
		      GTK_SIGNAL_FUNC (palette_dialog_new_entry_callback),
		      (gpointer) palette);
  gtk_widget_show (menu_items);
}

/*  the color area event callback  *******************************************/

static gint
palette_dialog_color_area_events (GtkWidget     *widget,
				  GdkEvent      *event,
				  PaletteDialog *palette)
{
  GdkEventButton *bevent;
  GSList *tmp_link;
  int r, g, b;
  int entry_width;
  int entry_height;
  int row, col;
  int pos;

  switch (event->type)
    {
    case GDK_EXPOSE:
      palette_dialog_redraw (palette);
      break;
    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *) event;
      entry_width  = (ENTRY_WIDTH  * palette->xzoom_factor) + SPACING;
      entry_height = (ENTRY_HEIGHT * palette->zoom_factor) +  SPACING;
      col = (bevent->x - 1) / entry_width;
      row = (bevent->y - 1) / entry_height;
      pos = row * palette->columns + col;
      
      if ((bevent->button == 1 || bevent->button == 3) && palette->entries)
	{
	  tmp_link = g_slist_nth (palette->entries->colors, pos);
	  if (tmp_link)
	    {
	      if (palette->color)
		{
		  palette->freeze_update = TRUE;
 		  palette_dialog_draw_entries (palette, -1, -1);
		  palette->freeze_update = FALSE;
		}
	      palette->color = tmp_link->data;

	      /*  Update either foreground or background colors  */
	      r = palette->color->color[0];
	      g = palette->color->color[1];
	      b = palette->color->color[2];
	      if (active_color == FOREGROUND)
		{
		  if (bevent->state & GDK_CONTROL_MASK)
		    gimp_context_set_background (gimp_context_get_user (),
						 r, g, b);
		  else
		    gimp_context_set_foreground (gimp_context_get_user (),
						 r, g, b);
		}
	      else if (active_color == BACKGROUND)
		{
		  if (bevent->state & GDK_CONTROL_MASK)
		    gimp_context_set_foreground (gimp_context_get_user (),
						 r, g, b);
		  else
		    gimp_context_set_background (gimp_context_get_user (),
						 r, g, b);
		}

	      palette_dialog_draw_entries (palette, row, col);
	      /*  Update the active color name  */
	      gtk_entry_set_text (GTK_ENTRY (palette->color_name),
				  palette->color->name);
	      gtk_widget_set_sensitive (palette->color_name, TRUE);
	     /*  palette_update_current_entry (palette); */
	      if (bevent->button == 3)
		{
		  /* Popup the edit menu */
		  gtk_menu_popup (GTK_MENU (palette->popup_menu), NULL, NULL, 
				  NULL, NULL, 3,
				  ((GdkEventButton *)event)->time);
		}
	    }
	  else
	    {
	      if (bevent->button == 3)
		{
		  /* Popup the small new menu */
		  gtk_menu_popup (GTK_MENU (palette->popup_small_menu),
				  NULL, NULL, 
				  NULL, NULL, 3,
				  ((GdkEventButton *)event)->time);
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

static int
palette_dialog_draw_color_row (guchar        **colors,
			       gint            ncolors,
			       gint            y,
			       gint            column_highlight,
			       guchar         *buffer,
			       PaletteDialog  *palette)
{
  guchar *p;
  guchar bcolor;
  gint width, height;
  gint entry_width;
  gint entry_height;
  gint vsize;
  gint vspacing;
  gint i, j;
  GtkWidget *preview;

  if (! palette)
    return -1;

  preview = palette->color_area;

  bcolor = 0;

  width = preview->requisition.width;
  height = preview->requisition.height;
  entry_width = (ENTRY_WIDTH * palette->xzoom_factor);
  entry_height = (ENTRY_HEIGHT * palette->zoom_factor);

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
	      guchar *ph = &buffer[3*column_highlight*(entry_width+SPACING)];
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
	      *p++ = colors[i][0];
	      *p++ = colors[i][1];
	      *p++ = colors[i][2];
	    }
	}

      for (i = 0; i < (palette->columns - ncolors); i++)
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
	  if (ncolors == column_highlight)
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
	      guchar *ph = &buffer[3*column_highlight*(entry_width+SPACING)];
	      *ph++ = ~bcolor;
	      *ph++ = ~bcolor;
	      *ph++ = ~bcolor;
	      ph += 3*(entry_width);
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
palette_dialog_draw_entries (PaletteDialog *palette,
			     gint           row_start,
			     gint           column_highlight)
{
  PaletteEntry *entry;
  guchar *buffer;
  guchar **colors;
  GSList *tmp_link;
  gint width, height;
  gint entry_width;
  gint entry_height;
  gint index, y;

  if (palette && palette->entries)
    {
      width  = palette->color_area->requisition.width;
      height = palette->color_area->requisition.height;

      entry_width  = (ENTRY_WIDTH  * palette->xzoom_factor);
      entry_height = (ENTRY_HEIGHT * palette->zoom_factor);

      colors = g_malloc (sizeof (guchar *) * palette->columns * 3);
      buffer = g_malloc (width * 3);

      if (row_start < 0)
	{
	  y = 0;
	  tmp_link = palette->entries->colors;
	  column_highlight = -1;
	}
      else
	{
	  y = (entry_height + SPACING) * row_start;
	  tmp_link = g_slist_nth (palette->entries->colors,
				  row_start * palette->columns);
	}
      index = 0;

      while (tmp_link)
	{
	  entry = tmp_link->data;
	  tmp_link = tmp_link->next;

	  colors[index] = entry->color;
	  index++;

	  if (index == palette->columns)
	    {
	      index = 0;
	      y = palette_dialog_draw_color_row (colors, palette->columns, y,
						 column_highlight, buffer,
						 palette);
	      if (y >= height || row_start >= 0)
		{
		  /* This row only */
		  gtk_widget_draw (palette->color_area, NULL);
		  g_free (buffer);
		  g_free (colors);
		  return;
		}
	    }
	}

      while (y < height)
	{
	  y = palette_dialog_draw_color_row (colors, index, y, column_highlight,
					     buffer, palette);
	  index = 0;
          if (row_start >= 0)
	    break;
	}

      g_free (buffer);
      g_free (colors);

      if (palette->freeze_update == FALSE)
	gtk_widget_draw (palette->color_area, NULL);
    }
}

static void
palette_dialog_scroll_top_left (PaletteDialog *palette)
{
  GtkAdjustment *hadj; 
  GtkAdjustment *vadj; 

  /*  scroll viewport to top left  */
  if (palette && palette->scrolled_window)
    {
      hadj = gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (palette->scrolled_window));
      vadj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (palette->scrolled_window));

      if (hadj)
	gtk_adjustment_set_value (hadj, 0.0);
      if (vadj)
	gtk_adjustment_set_value (vadj, 0.0);
    }
}

static void
palette_dialog_redraw (PaletteDialog *palette)
{
  GtkWidget *parent;
  gint vsize;
  gint nrows;
  gint n_entries;
  gint new_pre_width;
  guint width;
  gfloat new_xzoom;

  width  = palette->color_area->parent->parent->allocation.width;
  new_xzoom = (gfloat) ((gfloat) width /
			(gfloat) COLUMNS - SPACING) / (gfloat) ENTRY_WIDTH;

  if ((palette->columns_valid) && palette->last_width == width)
    return;

  palette->last_width = width;
  palette->columns_valid = TRUE;
  palette->xzoom_factor = new_xzoom;

  n_entries = palette->entries->n_colors;
  nrows = n_entries / palette->columns;
  if (n_entries % palette->columns)
    nrows += 1;

  vsize = nrows * (SPACING + (gint) (ENTRY_HEIGHT * palette->zoom_factor)) + SPACING;

  parent = palette->color_area->parent;
  gtk_widget_ref (palette->color_area);
  gtk_container_remove (GTK_CONTAINER (parent), palette->color_area);

  new_pre_width = (gint) (ENTRY_WIDTH * palette->xzoom_factor);
  new_pre_width = (new_pre_width + SPACING) * palette->columns + SPACING;

  gtk_preview_size (GTK_PREVIEW (palette->color_area),
		    new_pre_width, /*PREVIEW_WIDTH,*/
		    vsize);

  gtk_container_add (GTK_CONTAINER (parent), palette->color_area);
  gtk_widget_unref (palette->color_area);

  palette_dialog_draw_entries (palette, -1, -1);
}

/*  the palette dialog clist "select_row" callback  **************************/

static void
palette_dialog_list_item_update (GtkWidget      *widget, 
				 gint            row,
				 gint            column,
				 GdkEventButton *event,
				 gpointer        data)
{
  PaletteDialog *palette;
  PaletteEntries *p_entries;

  palette = (PaletteDialog *) data;

  if (palette->color_notebook_active)
    {
      color_notebook_hide (palette->color_notebook);
      palette->color_notebook_active = FALSE;
    }

  if (palette->color_notebook)
    color_notebook_free (palette->color_notebook);
  palette->color_notebook = NULL;

  p_entries = 
    (PaletteEntries *) gtk_clist_get_row_data (GTK_CLIST (palette->clist), row);

  palette->entries = p_entries;
  palette->columns_valid = FALSE;

  palette_dialog_redraw (palette);
  palette_dialog_scroll_top_left (palette);

  /*  Stop errors in case no colors are selected  */ 
  gtk_signal_handler_block (GTK_OBJECT (palette->color_name),
			    palette->entry_sig_id);
  gtk_entry_set_text (GTK_ENTRY (palette->color_name), _("Undefined")); 
  gtk_widget_set_sensitive (palette->color_name, FALSE);
  gtk_signal_handler_unblock (GTK_OBJECT (palette->color_name),
			      palette->entry_sig_id);
}

/*  the color name entry callback  *******************************************/

static void
palette_dialog_color_name_entry_changed (GtkWidget *widget,
					 gpointer   data)
{
  PaletteDialog *palette;

  palette = data;

  if (palette->color->name)
    g_free (palette->color->name);
  palette->color->name = 
    g_strdup (gtk_entry_get_text (GTK_ENTRY (palette->color_name)));

  palette->entries->changed = TRUE;
}

/*  palette zoom functions & callbacks  **************************************/

static void
palette_dialog_redraw_zoom (PaletteDialog *palette)
{
  if (palette->zoom_factor > 4.0)
    {
      palette->zoom_factor = 4.0;
    }
  else if (palette->zoom_factor < 0.1)
    {
      palette->zoom_factor = 0.1;
    }
  
  palette->columns = COLUMNS;
  palette->columns_valid = FALSE;
  palette_dialog_redraw (palette); 

  palette_dialog_scroll_top_left (palette);
}

static void
palette_dialog_zoomin_callback (GtkWidget *widget,
				gpointer   data)
{
  PaletteDialog *palette = data;

  palette->zoom_factor += 0.1;
  palette_dialog_redraw_zoom (palette);
}

static void
palette_dialog_zoomout_callback (GtkWidget *widget,
				 gpointer   data)
{
  PaletteDialog *palette = data;

  palette->zoom_factor -= 0.1;
  palette_dialog_redraw_zoom (palette);
}

/*  the palette edit ops callbacks  ******************************************/

static void
palette_dialog_add_entries_callback (GtkWidget *widget,
				     gpointer   data,
				     gpointer   call_data)
{
  PaletteEntries *entries;

  entries = palette_entries_new ((gchar *) call_data);

  palette_insert_all (entries);
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
palette_dialog_delete_callback (GtkWidget *widget,
				gpointer   data)
{
  PaletteDialog *palette;
  PaletteEntries *entries;

  palette = data;

  if (palette && palette->entries)
    {
      entries = palette->entries;
      if (entries && entries->filename)
	palette_entries_delete (entries->filename);

      palette_entries_list = g_slist_remove (palette_entries_list, entries);

      palette_refresh_all ();
    }
}

static void
palette_dialog_import_callback (GtkWidget *widget,
				gpointer   data)
{
  if (!import_dialog)
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
				       gpointer   data,
				       gpointer   call_data)
{
  PaletteDialog *palette;
  PaletteEntries *p_entries;
  PaletteEntries *new_entries;
  GList *sel_list;

  new_entries = palette_entries_new ((gchar *) call_data);

  palette = (PaletteDialog *) data;

  sel_list = GTK_CLIST (palette->clist)->selection;

  while (sel_list)
    {
      gint row;
      GSList *cols;

      row = GPOINTER_TO_INT (sel_list->data);
      p_entries = 
	(PaletteEntries *) gtk_clist_get_row_data (GTK_CLIST (palette->clist), row);

      /* Go through each palette and merge the colors */
      cols = p_entries->colors;
      while (cols)
	{
	  PaletteEntry *entry = cols->data;
	  palette_entries_add_entry (new_entries,
				     entry->name,
				     entry->color[0],
				     entry->color[1],
				     entry->color[2]);
	  cols = cols->next;
	}
      sel_list = sel_list->next;
    }

  palette_insert_all (new_entries);
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
  PaletteEntries *p_entries = NULL;
  PaletteDialog *palette;
  GList *sel_list;

  palette = (PaletteDialog *) data;
  sel_list = GTK_CLIST (palette->clist)->selection;

  while (sel_list)
    {
      gint row;

      row = GPOINTER_TO_INT (sel_list->data);

      p_entries = 
	(PaletteEntries *) gtk_clist_get_row_data (GTK_CLIST (palette->clist),
						   row);

      palette_create_edit (p_entries);

      /* One only */
      return;
    }
}

static void
palette_dialog_close_callback (GtkWidget *widget,
			       gpointer   data)
{
  PaletteDialog *palette;

  palette = data;

  if (palette)
    {
      if (palette->color_notebook_active)
	{
	  color_notebook_hide (palette->color_notebook);
	  palette->color_notebook_active = FALSE;
	}

      if (palette == top_level_edit_palette && import_dialog)
	{
	  gtk_widget_destroy (import_dialog->dialog);
	  g_free (import_dialog);
	  import_dialog = NULL;
	}
      
      if (GTK_WIDGET_VISIBLE (palette->shell))
	gtk_widget_hide (palette->shell);
    }
}

/*  the palette dialog color dnd callbacks  **********************************/

static void
palette_dialog_drag_color (GtkWidget *widget,
			   guchar    *r,
			   guchar    *g,
			   guchar    *b,
			   gpointer   data)
{
  PaletteDialog *palette;

  palette = (PaletteDialog *) data;

  if (palette && palette->entries && palette->color)
    {
      *r = (guchar) palette->color->color[0];
      *g = (guchar) palette->color->color[1];
      *b = (guchar) palette->color->color[2];
    }
}

static void
palette_dialog_drop_color (GtkWidget *widget,
			   guchar      r,
			   guchar      g,
			   guchar      b,
			   gpointer    data)
{
  PaletteDialog *palette;

  palette = (PaletteDialog *) data;

  if (palette && palette->entries)
    {
      palette->color =
	palette_entries_add_entry (palette->entries, _("Untitled"), r, g, b);

      palette_update_all (palette->entries);
    }
}

/*  the palette & palette edit dialog constructor  ***************************/

PaletteDialog *
palette_dialog_new (gint vert)
{
  PaletteDialog *palette;
  GtkWidget *hbox;
  GtkWidget *hbox2;
  GtkWidget *vbox;
  GtkWidget *scrolledwindow;
  GtkWidget *palette_region;
  GtkWidget *entry;
  GtkWidget *alignment;
  GtkWidget *palette_list;
  GtkWidget *frame;
  GtkWidget *button;
  GtkWidget *pixmapwid;
  GdkPixmap *pixmap;
  GdkBitmap *mask;
  GtkStyle  *style;

  palette = g_new (PaletteDialog, 1);

  palette->entries = default_palette_entries;
  palette->color = NULL;
  palette->color_notebook = NULL;
  palette->color_notebook_active = FALSE;
  palette->zoom_factor = 1.0;
  palette->xzoom_factor = 1.0;
  palette->last_width = 0;
  palette->columns = COLUMNS;
  palette->columns_valid = TRUE;
  palette->freeze_update = FALSE;

  if (!vert)
    {
      palette->shell =
	gimp_dialog_new (_("Color Palette Edit"), "color_palette_edit",
			 gimp_standard_help_func,
			 "dialogs/palette_editor/palette_editor.html",
			 GTK_WIN_POS_NONE,
			 FALSE, TRUE, FALSE,

			 _("Save"), palette_dialog_save_callback,
			 palette, NULL, FALSE, FALSE,
			 _("Refresh"), palette_dialog_refresh_callback,
			 palette, NULL, FALSE, FALSE,
			 _("Close"), palette_dialog_close_callback,
			 palette, NULL, TRUE, TRUE,

			 NULL);
    }
  else
    {
      palette->shell =
	gimp_dialog_new (_("Color Palette"), "color_palette",
			 gimp_standard_help_func,
			 "dialogs/palette_selection.html",
			 GTK_WIN_POS_NONE,
			 FALSE, TRUE, FALSE,

			 _("Edit"), palette_dialog_edit_callback,
			 palette, NULL, FALSE, FALSE,
			 _("Close"), palette_dialog_close_callback,
			 palette, NULL, TRUE, TRUE,

			 NULL);
    }

  /*  The main container widget  */
  if (vert)
    {
      hbox = gtk_notebook_new ();
      gtk_container_set_border_width (GTK_CONTAINER (hbox), 1);
    }
  else
    {
      hbox = gtk_hbox_new (FALSE, 4);
      gtk_container_set_border_width (GTK_CONTAINER (hbox), 4);
    }
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (palette->shell)->vbox), hbox);
  gtk_widget_show (hbox);

  vbox = gtk_vbox_new (FALSE, 2);
  gtk_widget_show (vbox);

  alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0); 
  gtk_widget_show (alignment); 

  palette->scrolled_window =
    scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_ALWAYS);
  gtk_box_pack_start (GTK_BOX (vbox), scrolledwindow, TRUE, TRUE, 0);
  gtk_widget_show (scrolledwindow);

  palette->color_area = palette_region = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_set_dither (GTK_PREVIEW (palette->color_area),
			  GDK_RGB_DITHER_MAX);
  gtk_preview_size (GTK_PREVIEW (palette_region), PREVIEW_WIDTH, PREVIEW_HEIGHT);
  
  gtk_widget_set_events (palette_region, PALETTE_EVENT_MASK);
  gtk_signal_connect (GTK_OBJECT (palette->color_area), "event",
		      (GtkSignalFunc) palette_dialog_color_area_events,
		      palette);

  gtk_container_add (GTK_CONTAINER (alignment), palette_region);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolledwindow),
					 alignment);
  gtk_widget_show (palette_region);

  /*  dnd stuff  */
  gtk_drag_source_set (palette_region,
                       GDK_BUTTON1_MASK,
                       color_palette_target_table, n_color_palette_targets,
                       GDK_ACTION_COPY | GDK_ACTION_MOVE);
  gimp_dnd_color_source_set (palette_region, palette_dialog_drag_color, palette);

  gtk_drag_dest_set (alignment,
                     GTK_DEST_DEFAULT_HIGHLIGHT |
                     GTK_DEST_DEFAULT_MOTION |
                     GTK_DEST_DEFAULT_DROP,
                     color_palette_target_table, n_color_palette_targets,
                     GDK_ACTION_COPY);
  gimp_dnd_color_dest_set (alignment, palette_dialog_drop_color, palette);

  /*  The color name entry  */
  hbox2 = gtk_hbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox2, FALSE, FALSE, 0);
  gtk_widget_show (hbox2);

  entry = palette->color_name = gtk_entry_new ();
  gtk_widget_show (entry);
  gtk_box_pack_start (GTK_BOX (hbox2), entry, TRUE, TRUE, 0);
  gtk_entry_set_text (GTK_ENTRY (entry), _("Undefined"));
  palette->entry_sig_id =
    gtk_signal_connect (GTK_OBJECT (entry), "changed",
			GTK_SIGNAL_FUNC (palette_dialog_color_name_entry_changed),
			palette);

  /*  + and - buttons  */
  if (! GTK_WIDGET_REALIZED (palette->shell))
    gtk_widget_realize (palette->shell);
  style = gtk_widget_get_style (palette->shell);

  button = gtk_button_new ();
  gtk_box_pack_start (GTK_BOX (hbox2), button, FALSE, FALSE, 0);
  pixmap = gdk_pixmap_create_from_xpm_d (palette->shell->window, &mask,
					 &style->bg[GTK_STATE_NORMAL], 
					 zoom_in_xpm);
  pixmapwid = gtk_pixmap_new (pixmap, mask);
  gdk_pixmap_unref (pixmap);
  gtk_container_add (GTK_CONTAINER (button), pixmapwid);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (palette_dialog_zoomin_callback),
		      (gpointer) palette);
  gtk_widget_show (pixmapwid);
  gtk_widget_show (button);

  button = gtk_button_new ();
  gtk_box_pack_start (GTK_BOX (hbox2), button, FALSE, FALSE, 0);
  pixmap = gdk_pixmap_create_from_xpm_d (palette->shell->window, &mask,
					 &style->bg[GTK_STATE_NORMAL], 
					 zoom_out_xpm);
  pixmapwid = gtk_pixmap_new (pixmap, mask);
  gdk_pixmap_unref (pixmap);
  gtk_container_add (GTK_CONTAINER (button), pixmapwid);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (palette_dialog_zoomout_callback),
		      (gpointer) palette);
  gtk_widget_show (pixmapwid);
  gtk_widget_show (button);
  
  /*  clist preview of palettes  */
  scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
  palette->clist = palette_list = gtk_clist_new (3);
  gtk_widget_set_usize (palette_list, 203, 203);
  gtk_clist_set_row_height (GTK_CLIST (palette_list), SM_PREVIEW_HEIGHT + 2);
  gtk_signal_connect (GTK_OBJECT (palette->clist), "select_row",
		      GTK_SIGNAL_FUNC (palette_dialog_list_item_update),
		      (gpointer) palette);

  if (vert)
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

  gtk_widget_show (palette_list);

  gtk_clist_set_column_title (GTK_CLIST (palette_list), 0, _("Palette"));
  gtk_clist_set_column_title (GTK_CLIST (palette_list), 1, _("Ncols"));
  gtk_clist_set_column_title (GTK_CLIST (palette_list), 2, _("Name"));
  gtk_clist_column_titles_show (GTK_CLIST (palette_list));

  gtk_container_add (GTK_CONTAINER (scrolledwindow), palette->clist);

  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_ALWAYS);
  
  if (!vert)
    gtk_clist_set_selection_mode (GTK_CLIST (palette_list),
				  GTK_SELECTION_EXTENDED);

  gtk_widget_show (scrolledwindow);

  gtk_clist_set_column_width (GTK_CLIST (palette_list), 0, SM_PREVIEW_WIDTH+2);
  gtk_clist_column_titles_show (GTK_CLIST (palette_list));

  if (!vert) 
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
			  (GtkSignalFunc) palette_dialog_new_callback,
			  (gpointer) palette);
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gtk_widget_show (button);

      button = gtk_button_new_with_label (_("Delete"));
      GTK_WIDGET_UNSET_FLAGS (button, GTK_RECEIVES_DEFAULT);
      gtk_misc_set_padding (GTK_MISC (GTK_BIN (button)->child), 2, 0);
      gtk_signal_connect (GTK_OBJECT (button), "clicked",
			  (GtkSignalFunc) palette_dialog_delete_callback,
			  (gpointer) palette);
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gtk_widget_show (button);
      
      button = gtk_button_new_with_label (_("Import"));
      GTK_WIDGET_UNSET_FLAGS (button, GTK_RECEIVES_DEFAULT);
      gtk_misc_set_padding (GTK_MISC (GTK_BIN (button)->child), 2, 0);
      gtk_signal_connect (GTK_OBJECT (button), "clicked",
			  (GtkSignalFunc) palette_dialog_import_callback,
			  (gpointer) palette);
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gtk_widget_show (button);

      button = gtk_button_new_with_label (_("Merge"));
      GTK_WIDGET_UNSET_FLAGS (button, GTK_RECEIVES_DEFAULT);
      gtk_misc_set_padding (GTK_MISC (GTK_BIN (button)->child), 2, 0);
      gtk_signal_connect (GTK_OBJECT (button), "clicked",
			  (GtkSignalFunc) palette_dialog_merge_callback,
			  (gpointer) palette);
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gtk_widget_show (button);
    }

  palette->gc = gdk_gc_new (palette->shell->window);

  /*  fill the clist  */
  palette_clist_init (palette->clist, palette->shell, palette->gc);
  palette_dialog_clist_scroll_to_current (palette);

  palette_dialog_create_popup_menu (palette);

  return palette;
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
  guchar buffer[3*IMPORT_PREVIEW_WIDTH];
  gint loop;
  guchar *p = buffer;
  gdouble  dx, cur_x;
  gdouble  r, g, b, a;

  dx    = 1.0/ (IMPORT_PREVIEW_WIDTH - 1);
  cur_x = 0;

  for (loop = 0 ; loop < IMPORT_PREVIEW_WIDTH; loop++)
    {
      gradient_get_color_at (gradient, cur_x, &r, &g, &b, &a);
      *p++ = r * 255.0;
      *p++ = g * 255.0;
      *p++ = b * 255.0;
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
  GSList** l;

  l = (GSList**) data;
  *l = g_slist_prepend (*l, im);
}

static void
palette_import_gimlist_indexed_cb (gpointer im,
				   gpointer data)
{
  GimpImage *gimage = GIMP_IMAGE (im);
  GSList** l;

  if (gimage_base_type (gimage) == INDEXED)
    {
      l = (GSList**) data;
      *l = g_slist_prepend (*l, im);
    }
}

static void
palette_import_update_image_preview (GimpImage *gimage)
{
  TempBuf * preview_buf;
  gchar *src, *buf;
  gint x,y,has_alpha;
  gint sel_width, sel_height;
  gint pwidth, pheight;

  import_dialog->gimage = gimage;

  /* Calculate preview size */

  sel_width = gimage->width;
  sel_height = gimage->height;

  if (sel_width > sel_height) {
    pwidth  = MIN(sel_width, IMPORT_PREVIEW_WIDTH);
    pheight = sel_height * pwidth / sel_width;
  } else {
    pheight = MIN(sel_height, IMPORT_PREVIEW_HEIGHT);
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
  gchar *lab;

  gimage = GIMP_IMAGE (data);
  palette_import_update_image_preview (gimage);

  lab = g_strdup_printf ("%s-%d",
			 g_basename (gimage_filename (import_dialog->gimage)),
			 pdb_image_to_id (import_dialog->gimage));

  gtk_entry_set_text (GTK_ENTRY (import_dialog->entry), lab);
}

static void
palette_import_image_menu_add (GimpImage *gimage)
{
  GtkWidget *menuitem;
  gchar *lab = g_strdup_printf ("%s-%d",
				g_basename (gimage_filename (gimage)),
				pdb_image_to_id (gimage));
  menuitem = gtk_menu_item_new_with_label (lab);
  gtk_widget_show (menuitem);
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
		      (GtkSignalFunc) palette_import_image_sel_callback,
		      gimage);
  gtk_menu_append (GTK_MENU (import_dialog->optionmenu1_menu), menuitem);
}

/* Last Param gives us control over what goes in the menu on a delete oper */
static void
palette_import_image_menu_activate (gint        redo,
				    ImportType  type,
				    GimpImage  *del_image)
{
  GSList *list=NULL;
  gint num_images;
  GimpImage *last_img = NULL;
  GimpImage *first_img = NULL;
  gint act_num = -1;
  gint count = 0;
  gchar *lab;

  if (!import_dialog)
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
      gint i;
      GtkWidget *optionmenu1;
      GtkWidget *optionmenu1_menu;

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
           gchar *lab = g_strdup_printf ("%s-%d",
			 g_basename (gimage_filename (import_dialog->gimage)),
			 pdb_image_to_id (import_dialog->gimage));

           gtk_option_menu_set_history (GTK_OPTION_MENU (optionmenu1), act_num);
           gtk_entry_set_text (GTK_ENTRY (import_dialog->entry), lab);
	}
    }
  g_slist_free (list);

  lab = g_strdup_printf ("%s-%d",
			 g_basename (gimage_filename (import_dialog->gimage)),
			 pdb_image_to_id (import_dialog->gimage));

  gtk_entry_set_text (GTK_ENTRY (import_dialog->entry), lab);
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
  GSList *list=NULL;
  gint num_images = 0;

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
  if (!import_dialog)
    return;

  if (!GTK_WIDGET_IS_SENSITIVE (import_dialog->image_menu_item_image))
    {
      gtk_widget_set_sensitive (import_dialog->image_menu_item_image, TRUE);
      return;
    }

  if (!GTK_WIDGET_IS_SENSITIVE (import_dialog->image_menu_item_indexed) &&
      gimage_base_type(gimage) == INDEXED)
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
  if (!import_dialog)
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
  if (import_dialog && (import_dialog->import_type == IMAGE_IMPORT ||
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
  PaletteEntries *entries;
  gradient_t *gradient;

  gradient = gimp_context_get_gradient (gimp_context_get_user ());

  if (gradient)
    {
      /* Add names to entry */
      gdouble  dx, cur_x;
      gdouble  r, g, b, a;

      gint sample_sz;
      gint loop;

      entries = palette_entries_new (name);
      sample_sz = (gint) import_dialog->sample->value;  

      dx    = 1.0 / (sample_sz - 1);
      cur_x = 0;
      
      for (loop = 0; loop < sample_sz; loop++)
	{
	  gradient_get_color_at (gradient, cur_x, &r, &g, &b, &a);
	  r = r * 255.0;
	  g = g * 255.0;
	  b = b * 255.0;
	  cur_x += dx;
	  palette_entries_add_entry (entries, _("Untitled"),
				     (gint) r, (gint) g, (gint) b);
	}

      palette_insert_all (entries);
    }
}

/*  create a palette from a non-indexed image  *******************************/

typedef struct _ImgColors ImgColors;

struct _ImgColors
{
  guint count;
  guint r_adj;
  guint g_adj;
  guint b_adj;
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
  gpointer found_color = NULL;
  ImgColors *new_color;
  guint key_colors = colors[0]*256*256 + colors[1]*256 + colors[2];

  if(h_array == NULL)
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
  ImgColors  *color_tab  = (ImgColors *) value;

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
  PaletteEntries *entries = (PaletteEntries *) user_data;
  ImgColors *color_tab = (ImgColors *) data;
  gint sample_sz;
  gchar *lab;

  sample_sz = (gint) import_dialog->sample->value;  

  if (entries->n_colors >= sample_sz)
    return;

  lab = g_strdup_printf ("%s (occurs %u)", _("Untitled"), color_tab->count);

  /* Adjust the colors to the mean of the the sample */
  palette_entries_add_entry
    (entries, lab, 
     (gint) color_tab->r + (color_tab->r_adj / color_tab->count), 
     (gint) color_tab->g + (color_tab->g_adj / color_tab->count), 
     (gint) color_tab->b + (color_tab->b_adj / color_tab->count));
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
  PaletteEntries *entries;
  GSList *sorted_list = NULL;

  g_hash_table_foreach (h_array, palette_import_create_sorted_list,
			&sorted_list);
  sorted_list = g_slist_sort (sorted_list, palette_import_sort_colors);

  entries = palette_entries_new (name);
  g_slist_foreach (sorted_list, palette_import_create_image_palette, entries);

  /* Free up used memory */
  /* Note the same structure is on both the hash list and the sorted
   * list. So only delete it once.
   */
  g_hash_table_freeze (h_array);
  g_hash_table_foreach_remove (h_array,
			       palette_import_color_print_remove, NULL);
  g_hash_table_thaw (h_array);
  g_hash_table_destroy (h_array);
  g_slist_free (sorted_list);

  palette_insert_all (entries);
}

static void
palette_import_create_from_image (GImage *gimage,
				  guchar *pname)
{
  PixelRegion imagePR;
  guchar *image_data;
  guchar *idata;
  guchar  rgb[MAX_CHANNELS];
  guchar  rgb_real[MAX_CHANNELS];
  gint has_alpha, indexed;
  gint width, height;
  gint bytes, alpha;
  gint i, j;
  void * pr;
  gint d_type;
  GHashTable *store_array = NULL;
  gint sample_sz;
  gint threshold = 1;

  sample_sz = (gint) import_dialog->sample->value;  

  if (gimage == NULL)
    return;

  /*  Get the image information  */
  bytes = gimage_composite_bytes (gimage);
  d_type = gimage_composite_type (gimage);
  has_alpha = (d_type == RGBA_GIMAGE ||
	       d_type == GRAYA_GIMAGE ||
	       d_type == INDEXEDA_GIMAGE);
  indexed = d_type == INDEXEDA_GIMAGE || d_type == INDEXED_GIMAGE;
  width = gimage->width;
  height = gimage->height;
  pixel_region_init (&imagePR, gimage_composite (gimage), 0, 0,
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
	      gimage_get_color (gimage, d_type, rgb, idata);
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
				    guchar *pname)
{
  PaletteEntries *entries;
  gint samples, count;

  samples = (gint) import_dialog->sample->value;  

  if (gimage == NULL)
    return;

  if (gimage_base_type (gimage) != INDEXED)
    return;

  entries = palette_entries_new (pname);

  for (count= 0; count < samples && count < gimage->num_cols; ++count)
    {
      palette_entries_add_entry (entries, NULL,
				 gimage->cmap[count*3],
				 gimage->cmap[count*3+1],
				 gimage->cmap[count*3+2]);
    }

  palette_insert_all (entries);
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
  PaletteDialog *palette;

  palette = data;

  if (import_dialog)
    {
      guchar *pname;

      pname = gtk_entry_get_text (GTK_ENTRY (import_dialog->entry));
      if (!pname || strlen (pname) == 0)
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
  import_dialog->gimage = NULL;

  import_dialog->dialog = dialog =
    gimp_dialog_new (_("Import Palette"), "import_palette",
		     gimp_standard_help_func,
		     "dialogs/palette_editor/import_palette.html",
		     GTK_WIN_POS_NONE,
		     FALSE, TRUE, FALSE,

		     _("Import"), palette_import_import_callback,
		     palette, NULL, FALSE, FALSE,
		     _("Close"), palette_import_close_callback,
		     palette, NULL, TRUE, TRUE,

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
		      (GtkSignalFunc) palette_import_grad_callback,
		      NULL);
  gtk_menu_append (GTK_MENU (optionmenu_menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = import_dialog->image_menu_item_image =
    gtk_menu_item_new_with_label ("Image");
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
		      (GtkSignalFunc) palette_import_image_callback,
		      (gpointer) import_dialog);
  gtk_menu_append (GTK_MENU (optionmenu_menu), menuitem);
  gtk_widget_show (menuitem);
  gtk_widget_set_sensitive (menuitem,
			    palette_import_image_count (IMAGE_IMPORT) > 0);

  menuitem = import_dialog->image_menu_item_indexed =
    gtk_menu_item_new_with_label ("Indexed Palette");
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
                      (GtkSignalFunc) palette_import_indexed_callback,
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
		      GTK_SIGNAL_FUNC (palette_import_gradient_update), NULL);

  /*  keep the dialog up-to-date  */
  gtk_signal_connect (GTK_OBJECT (image_context), "add",
		      GTK_SIGNAL_FUNC (palette_import_image_new),
		      NULL);
  gtk_signal_connect (GTK_OBJECT (image_context), "remove",
		      GTK_SIGNAL_FUNC (palette_import_image_destroyed),
		      NULL);

  return import_dialog;
}
