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
#include "actionarea.h"
#include "buildmenu.h"
#include "colormaps.h"
#include "color_area.h"
#include "color_select.h"
#include "datafiles.h"
#include "devices.h"
#include "errors.h"
#include "gimprc.h"
#include "gradient_header.h"
#include "gradient.h"
#include "interface.h"
#include "palette.h"
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

#define PREVIEW_WIDTH ((ENTRY_WIDTH * COLUMNS) + (SPACING * (COLUMNS + 1)))
#define PREVIEW_HEIGHT ((ENTRY_HEIGHT * ROWS) + (SPACING * (ROWS + 1)))

#define PALETTE_EVENT_MASK GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK | GDK_ENTER_NOTIFY_MASK

/* New palette code... */

#define IMPORT_PREVIEW_WIDTH 80
#define IMPORT_PREVIEW_HEIGHT 80
#define MAX_IMAGE_COLORS (10000*2)

typedef enum
{
  GRAD_IMPORT = 0,
  IMAGE_IMPORT = 1,
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
  GtkWidget     *image_menu_item_gradient;
  GtkWidget     *optionmenu1_menu;
  GtkWidget     *type_option;
  GtkWidget     *threshold_scale;
  GtkWidget     *threshold_text;
  GtkAdjustment *threshold;
  GtkAdjustment *sample;
  ImportType    import_type;
  GimpImage     *gimage;
};

typedef struct _PaletteDialog PaletteDialog;

struct _PaletteDialog
{
  GtkWidget *shell;
  GtkWidget *color_area;
  GtkWidget *scrolled_window;
  GtkWidget *color_name;
  GtkWidget *clist;
  GtkWidget *popup_menu;
  GtkWidget *popup_small_menu;
  ColorSelectP color_select;
  int color_select_active;
  PaletteEntriesP entries;
  PaletteEntryP color;
  GdkGC *gc;
  guint entry_sig_id;
  gfloat zoom_factor;  /* range from 0.1 to 4.0 */
  gint columns;
  gint freeze_update;
};

/*  This one is called from palette_select.c  */
void  palette_clist_init(GtkWidget *clist, GtkWidget *shell, GdkGC *gc);

/*  Local function prototypes  */
static void  palette_entries_free (PaletteEntriesP);
static void  palette_entries_load (char *);
static void  palette_entry_free   (PaletteEntryP);
static void  palette_entries_save (PaletteEntriesP, char *);

PaletteDialog * create_palette_dialog (gboolean vert);

static void     palette_draw_entries (PaletteDialog *palette, gint row_start,
				      gint column_highlight);
static void     redraw_palette       (PaletteDialog *palette);

static GSList * palette_entries_insert_list (GSList *list,
					     PaletteEntriesP entries, gint pos);
static void     palette_draw_small_preview  (GdkGC *gc, PaletteEntriesP p_entry);
static void     palette_scroll_clist_to_current (PaletteDialog *palette);
static void     palette_update_small_preview    (PaletteDialog *palette);
static void     palette_scroll_top_left         (PaletteDialog *palette);

static ImportDialog * palette_import_dialog (PaletteDialog *palette);
static void  palette_import_dialog_callback (GtkWidget *, gpointer);
static void  import_palette_create_from_image (GImage *gimage,
					       guchar *pname,
					       PaletteDialog *palette);

static void  palette_merge_dialog_callback (GtkWidget *, gpointer);


GSList                 *palette_entries_list = NULL;
static PaletteEntriesP  default_palette_entries = NULL;
static gint             num_palette_entries = 0;
static guchar           foreground[3] = { 0, 0, 0 };
static guchar           background[3] = { 255, 255, 255 };
static ImportDialog    *import_dialog = NULL;

PaletteDialog          *top_level_edit_palette = NULL;
PaletteDialog          *top_level_palette = NULL;

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
palette_entries_delete (gchar *filename)
{
  if (filename)
    unlink (filename);
}

void
palettes_init (gint no_data)
{
  palette_init_palettes (no_data);
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

  num_palette_entries = 0;
  palette_entries_list = NULL;
}


void
palettes_free ()
{
  palette_free_palettes ();
}

void
palette_get_foreground (guchar *r,
			guchar *g,
			guchar *b)
{
  *r = foreground[0];
  *g = foreground[1];
  *b = foreground[2];
}

void
palette_get_background (guchar *r,
			guchar *g,
			guchar *b)
{
  *r = background[0];
  *g = background[1];
  *b = background[2];
}

void
palette_set_foreground (gint r,
			gint g,
			gint b)
{
  guchar rr, gg, bb;

  /*  Foreground  */
  foreground[0] = r;
  foreground[1] = g;
  foreground[2] = b;

  palette_get_foreground (&rr, &gg, &bb);
  if (no_interface == FALSE)
    {
      store_color (&foreground_pixel, rr, gg, bb);
      color_area_update ();
      device_status_update (current_device);
    }
}

void
palette_set_background (gint r,
			gint g,
			gint b)
{
  guchar rr, gg, bb;

  /*  Background  */
  background[0] = r;
  background[1] = g;
  background[2] = b;

  palette_get_background (&rr, &gg, &bb);
  if (no_interface == FALSE)
    {
      store_color (&background_pixel, rr, gg, bb);
      color_area_update ();
    }
}

void
palette_set_default_colors (void)
{
  palette_set_foreground (0, 0, 0);
  palette_set_background (255, 255, 255);
}


void 
palette_swap_colors (void)
{
  guchar fg_r, fg_g, fg_b;
  guchar bg_r, bg_g, bg_b;
  
  palette_get_foreground (&fg_r, &fg_g, &fg_b);
  palette_get_background (&bg_r, &bg_g, &bg_b);
  
  palette_set_foreground (bg_r, bg_g, bg_b);
  palette_set_background (fg_r, fg_g, fg_b);
}

void
palette_init_palettes (gint no_data)
{
  if (!no_data)
    datafiles_read_directories (palette_path, palette_entries_load, 0);

}

static void
palette_select2_set_text_all (PaletteEntriesP entries)
{
  gint pos = 0;
  gchar *num_buf;
  GSList *plist;
  PaletteDialog *pp; 
  PaletteEntriesP p_entries = NULL;
  gchar * num_copy;

  plist = palette_entries_list;
  
  while (plist)
    {
      p_entries = (PaletteEntriesP) plist->data;
      plist = g_slist_next (plist);
      
      if (p_entries == entries)
	    break;
      pos++;
    }

  if(p_entries == NULL)
    return; /* This is actually and error */

  num_buf = g_strdup_printf ("%d", p_entries->n_colors);;

  num_copy = g_strdup (num_buf);

  pp = top_level_palette;
  gtk_clist_set_text (GTK_CLIST (pp->clist), pos, 1, num_copy);
  redraw_palette (pp);
}

static void
palette_select2_refresh_all ()
{
  PaletteDialog *pp; 

  if(!top_level_palette)
    return;

  pp = top_level_palette;
  gtk_clist_freeze (GTK_CLIST(pp->clist));
  gtk_clist_clear (GTK_CLIST(pp->clist));
  palette_clist_init (pp->clist, pp->shell, pp->gc);
  gtk_clist_thaw (GTK_CLIST (pp->clist));
  pp->entries = palette_entries_list->data;
  redraw_palette (pp);
  palette_scroll_clist_to_current (pp);
}

static void
palette_select2_clist_insert_all (PaletteEntriesP p_entries)
{
  PaletteEntriesP chk_entries;
  PaletteDialog *pp; 
  GSList *plist;
  gint pos = 0;

  plist = palette_entries_list;
  
  while (plist)
    {
      chk_entries = (PaletteEntriesP) plist->data;
      plist = g_slist_next (plist);
      
      /*  to make sure we get something!  */
      if (chk_entries == NULL)
	{
	  return;
	}
      if (strcmp (p_entries->name, chk_entries->name) == 0)
	break;
      pos++;
    }

  pp = top_level_palette;
  gtk_clist_freeze (GTK_CLIST (pp->clist));
  palette_insert_clist (pp->clist, pp->shell, pp->gc, p_entries, pos);
  gtk_clist_thaw (GTK_CLIST (pp->clist));

/*   if(gradient_select_dialog) */
/*     { */
/*       gtk_clist_set_text(GTK_CLIST(gradient_select_dialog->clist),n,1,grad->name);   */
/*     } */
}

static void
palette_save_palettes ()
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

      list = g_slist_next (list);
    }
}

static void
palette_save_palettes_callback (GtkWidget *widget,
				gpointer   data)
{
  palette_save_palettes ();
}

static void
palette_entry_free (PaletteEntryP entry)
{
  if (entry->name)
    g_free (entry->name);

  g_free (entry);
}


void
palette_free ()
{
  if (top_level_edit_palette) 
    { 
      if(import_dialog)
	{
	  gtk_widget_destroy (import_dialog->dialog);
	  g_free (import_dialog);
	  import_dialog = NULL;
	}

      gdk_gc_destroy (top_level_edit_palette->gc); 
      
      if (top_level_edit_palette->color_select) 
	color_select_free (top_level_edit_palette->color_select); 
      
      g_free (top_level_edit_palette); 
      
      top_level_edit_palette = NULL; 
    }

  if (top_level_palette)
    {
      gdk_gc_destroy (top_level_palette->gc); 
      session_get_window_info (top_level_palette->shell, &palette_session_info);  
      top_level_palette = NULL;
    }
}

static void
palette_entries_save (PaletteEntriesP  palette,
		      gchar           *filename)
{
  FILE * fp;
  GSList * list;
  PaletteEntryP entry;

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

  list = palette->colors;
  while (list)
    {
      entry = (PaletteEntryP) list->data;
      fprintf (fp, "%d %d %d\t%s\n", entry->color[0], entry->color[1],
	       entry->color[2], entry->name);
      list = g_slist_next (list);
    }

  /*  Clean up  */
  fclose (fp);
}

static PaletteEntryP
palette_add_entry (PaletteEntriesP  entries,
		   gchar           *name,
		   gint             r,
		   gint             g,
		   gint             b)
{
  PaletteEntryP entry;

  if (entries)
    {
      entry = g_new (_PaletteEntry, 1);

      entry->color[0] = r;
      entry->color[1] = g;
      entry->color[2] = b;
      if (name)
	entry->name = g_strdup (name);
      else
	entry->name = g_strdup (_("Untitled"));
      entry->position = entries->n_colors;

      entries->colors = g_slist_append (entries->colors, entry);
      entries->n_colors += 1;

      entries->changed = 1;
      
      return entry;
    }

  return NULL;
}

static void
palette_change_color (gint r,
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
	    palette_add_entry (top_level_edit_palette->entries,
			       _("Untitled"), r, g, b); 
	  
	  palette_update_small_preview (top_level_edit_palette);
	  redraw_palette (top_level_edit_palette);
 	  break; 
	  
 	case COLOR_UPDATE_NEW: 
 	  top_level_edit_palette->color->color[0] = r; 
 	  top_level_edit_palette->color->color[1] = g; 
 	  top_level_edit_palette->color->color[2] = b; 
	  palette_update_small_preview (top_level_edit_palette);
	  redraw_palette (top_level_edit_palette);
 	  break; 
	  
 	default: 
 	  break; 
 	} 
    } 
  
  if (active_color == FOREGROUND)
    palette_set_foreground (r, g, b);
  else if (active_color == BACKGROUND)
    palette_set_background (r, g, b);
}

void
palette_set_active_color (gint r,
			  gint g,
			  gint b,
			  gint state)
{
  palette_change_color (r, g, b, state);
}

static void
palette_entries_load (gchar *filename)
{
  PaletteEntriesP entries;
  PaletteEntriesP p_entries = NULL;
  gchar   str[512];
  gchar  *tok;
  FILE   *fp;
  gint    r, g, b;
  gint    linenum;
  GSList *list;
  gint    pos = 0;

  r = g = b = 0;

  entries = g_new (_PaletteEntries, 1);

  entries->filename = g_strdup (filename);
  entries->name = g_strdup (g_basename (filename));
  entries->colors = NULL;
  entries->n_colors = 0;
  entries->pixmap = NULL;

  /*  Open the requested file  */
  if (!(fp = fopen (filename, "r")))
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
	    g_message (_("Loading palette %s (line %d):\n"
			 "Missing RED component"), filename, linenum);
	    /* maybe we should just abort? */

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

	  palette_add_entry (entries, tok, r, g, b);
	}
    }

  /*  Clean up  */

  fclose (fp);
  entries->changed = 0;

  list = palette_entries_list;

  while (list)
    {
      p_entries = (PaletteEntriesP) list->data;
      list = g_slist_next (list);
      
      /*  to make sure we get something!  */
      if (p_entries == NULL)
	{
	  p_entries = default_palette_entries;
	}
      if (strcmp (p_entries->name, entries->name) > 0)
	break;
      pos++;
    }
  
  palette_entries_list = palette_entries_insert_list (palette_entries_list, entries, pos);

  /* Check if the current palette is the default one */
  if (strcmp (default_palette, g_basename (filename)) == 0)
    default_palette_entries = entries;
}

static PaletteDialog *
new_top_palette (gboolean vert)
{
  PaletteDialog *p;

  p = create_palette_dialog (vert);
  palette_clist_init (p->clist, p->shell, p->gc);

  return p;
}

void 
palette_select_palette_init (void)
{
  /* Load them if they are not already loaded */
  if(top_level_edit_palette == NULL)
    {
      top_level_edit_palette = new_top_palette (FALSE);
    }
}

void 
palette_create (void)
{
  if (top_level_palette == NULL)
    {
      top_level_palette = new_top_palette (TRUE);
      /* top_level_palette = palette_new_selection(_("Palette"),NULL); */
      session_set_window_geometry (top_level_palette->shell,
				   &palette_session_info, TRUE);
      /* register this one only */
      dialog_register (top_level_palette->shell);

      gtk_widget_show (top_level_palette->shell);
      palette_scroll_clist_to_current (top_level_palette);
    }
  else
    {
      if (!GTK_WIDGET_VISIBLE (top_level_palette->shell))
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
palette_create_edit (PaletteEntriesP entries)
{
  PaletteDialog *p;

  if (top_level_edit_palette == NULL)
    {
      p = new_top_palette (FALSE);

      gtk_widget_show(p->shell);
      
      palette_draw_entries(p,-1,-1);
      
      top_level_edit_palette = p;
    }
  else
    {
      if (!GTK_WIDGET_VISIBLE (top_level_edit_palette->shell))
	{
	  gtk_widget_show (top_level_edit_palette->shell);
	  palette_draw_entries (top_level_edit_palette, -1, -1);
	}
      else
	{
	  gdk_window_raise (top_level_edit_palette->shell->window);
	}
    }

  if(entries != NULL)
    {
      top_level_edit_palette->entries = entries;
      gtk_clist_unselect_all (GTK_CLIST (top_level_edit_palette->clist));
      palette_scroll_clist_to_current (top_level_edit_palette);
    }
}

static GSList *
palette_entries_insert_list (GSList          *list,
			     PaletteEntriesP  entries,
			     gint             pos)
{
  GSList *ret_list;

  /*  add it to the list  */
  num_palette_entries++;
  ret_list = g_slist_insert (list, (void *) entries, pos);

  return ret_list;
}

static void
palette_update_small_preview (PaletteDialog *palette)
{
  GSList *list;
  gint pos = 0;
  PaletteEntriesP p_entries = NULL;
  gchar *num_buf;

  list = palette_entries_list;
  
  pos = 0;
  
  while (list)
    {
      p_entries = (PaletteEntriesP) list->data;
      list = g_slist_next (list);
      
      /*  to make sure we get something!  */
      if (p_entries == NULL)
	{
	  p_entries = default_palette_entries;
	}
      if (p_entries == palette->entries)
	break;
      pos++;
    }
  
  num_buf = g_strdup_printf ("%d", p_entries->n_colors);
  palette_draw_small_preview (palette->gc, p_entries);
  gtk_clist_set_text (GTK_CLIST (palette->clist), pos, 1, num_buf);
}

static void
palette_delete_entry (GtkWidget *widget,
		      gpointer   data)
{
  PaletteEntryP entry;
  GSList *tmp_link;
  PaletteDialog *palette;
  gint pos = 0;

  palette = data;
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
	palette->color = palette_add_entry (palette->entries, _("Black"), 0, 0, 0);
      palette_update_small_preview (palette);
      palette_select_set_text_all (palette->entries);
      palette_select2_set_text_all (palette->entries);
      redraw_palette (palette);
    }
}

static void
palette_new_callback (GtkWidget *widget,
		      gpointer   data)
{
  PaletteDialog *palette;
  char *num_buf;
  GSList *list;
  gint pos = 0;
  PaletteEntriesP p_entries = NULL;

  palette = data;
  if (palette && palette->entries)
    {
      if (active_color == FOREGROUND)
	palette->color =
	  palette_add_entry (palette->entries, _("Untitled"),
			     foreground[0], foreground[1], foreground[2]);
      else if (active_color == BACKGROUND)
	palette->color =
	  palette_add_entry (palette->entries, _("Untitled"),
			     background[0], background[1], background[2]);

      redraw_palette (palette);

      list = palette_entries_list;
      
      while (list)
	{
	  p_entries = (PaletteEntriesP) list->data;
	  list = g_slist_next (list);
	  
	  /*  to make sure we get something!  */
	  if (p_entries == NULL)
	    {
	      p_entries = default_palette_entries;
	    }
	  if (p_entries == palette->entries)
	    break;
	  pos++;
	}

      num_buf = g_strdup_printf ("%d", p_entries->n_colors);;
      palette_draw_small_preview (palette->gc, p_entries);
      gtk_clist_set_text (GTK_CLIST (palette->clist), pos, 1, num_buf);
      palette_select_set_text_all (palette->entries);
      palette_select2_set_text_all (palette->entries);
    }
}

static PaletteEntriesP
palette_create_entries (gpointer  data,
			gpointer  call_data)
{
  gchar *home;
  gchar *palette_name;
  gchar *local_path;
  gchar *first_token;
  gchar *token;
  gchar *path;
  PaletteEntriesP entries = NULL;
  PaletteDialog *palette;
  GSList *list;
  gint pos = 0;
  PaletteEntriesP p_entries = NULL;

  palette = data;

  palette_name = (char *) call_data;
  if (palette && palette_name)
    {
      entries = g_new (_PaletteEntries, 1);
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
		  path = g_strdup(token);
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
      entries->changed = 1;
      entries->pixmap = NULL;

      /* Add into the clist */

      list = palette_entries_list;
      
      while (list)
	{
	  p_entries = (PaletteEntriesP) list->data;
	  list = g_slist_next (list);
	  
	  /*  to make sure we get something!  */
	  if (p_entries == NULL)
	    {
	      p_entries = default_palette_entries;
	    }
	  if (strcmp(p_entries->name, entries->name) > 0)
	    break;
	  pos++;
	}

      palette_entries_list =
	palette_entries_insert_list (palette_entries_list, entries, pos);
      palette_insert_clist (palette->clist, palette->shell, palette->gc,
			    entries,pos);

      palette->entries = entries;

      palette_save_palettes ();
    }

  return entries;
}

static void
palette_add_entries_callback (GtkWidget *widget,
			      gpointer   data,
			      gpointer   call_data)
{
  PaletteEntriesP entries;

  entries = palette_create_entries (data, call_data);
  /* Update other selectors on screen */
  palette_select_clist_insert_all (entries);
  palette_select2_clist_insert_all (entries);
  palette_scroll_clist_to_current ((PaletteDialog *) data);
}

static void
palette_new_entries_callback (GtkWidget *widget,
			      gpointer   data)
{
  gtk_widget_show (query_string_box (_("New Palette"),
				     _("Enter a name for new palette"),
				     NULL,
				     NULL, NULL,
				     palette_add_entries_callback, data));
}

static void
redraw_zoom (PaletteDialog *palette)
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

  redraw_palette (palette); 

  palette_scroll_top_left (palette);
}

static void
palette_zoomin (GtkWidget *widget,
		gpointer   data)
{
  PaletteDialog *palette = data;

  palette->zoom_factor += 0.1;
  redraw_zoom (palette);
}

static void
palette_zoomout (GtkWidget *widget,
		 gpointer   data)
{
  PaletteDialog *palette = data;

  palette->zoom_factor -= 0.1;
  redraw_zoom (palette);
}

static void 
palette_refresh (PaletteDialog *palette)
{
  if (palette)
    {
      if (default_palette_entries == palette->entries)
	default_palette_entries = NULL;
      palette->entries = NULL;

      /*  If a color selection dialog is up, hide it  */
      if (palette->color_select_active)
	{
	  palette->color_select_active = 0;
	  color_select_hide (palette->color_select);
	}
      palette_free_palettes ();   /*  free palettes, don't save any modified versions  */
      palette_init_palettes (FALSE);   /*  reload palettes  */

      gtk_clist_freeze (GTK_CLIST (palette->clist));
      gtk_clist_clear (GTK_CLIST (palette->clist));
      palette_clist_init (palette->clist, palette->shell, palette->gc);
      gtk_clist_thaw (GTK_CLIST (palette->clist));

      if (palette->entries == NULL)
	palette->entries = default_palette_entries;

      if(palette->entries == NULL && palette_entries_list)
	palette->entries = palette_entries_list->data;
      
      redraw_palette (palette);

      palette_scroll_clist_to_current (palette);

      palette_select_refresh_all ();
      palette_select2_refresh_all ();
    }
  else
    {
      palette_free_palettes (); 
      palette_init_palettes (FALSE);
    }
}

static void
palette_refresh_callback (GtkWidget *widget,
			  gpointer   data)
{
  palette_refresh (data);
}

/*****/
static void 
palette_draw_small_preview(GdkGC           *gc,
			   PaletteEntriesP  p_entry)
{
  guchar rgb_buf[SM_PREVIEW_WIDTH * SM_PREVIEW_HEIGHT * 3];
  GSList *tmp_link;
  gint index;
  PaletteEntryP entry;

  /*fill_clist_prev(p_entry,rgb_buf,48,16,0.0,1.0);*/
  memset (rgb_buf, 0x0, sizeof (rgb_buf));
  
  gdk_draw_rgb_image (p_entry->pixmap,
		      gc,
		      0,
		      0,
		      SM_PREVIEW_WIDTH,
		      SM_PREVIEW_HEIGHT,
		      GDK_RGB_DITHER_NORMAL,
		      rgb_buf,
		      SM_PREVIEW_WIDTH*3);
  
  tmp_link = p_entry->colors;
  index = 0;

  while (tmp_link)
    {
      guchar cell[3*3*3];
      gint loop;

      entry = tmp_link->data;
      tmp_link = tmp_link->next;
      
      for (loop = 0; loop < 27 ; loop+=3)
	{
	  cell[0+loop] = entry->color[0];
	  cell[1+loop] = entry->color[1];
	  cell[2+loop] = entry->color[2];
	}

      gdk_draw_rgb_image (p_entry->pixmap,
			  gc,
			  1+(index%((SM_PREVIEW_WIDTH-2)/3))*3,
			  1+(index/((SM_PREVIEW_WIDTH-2)/3))*3,
			  3,
			  3,
			  GDK_RGB_DITHER_NORMAL,
			  cell,
			  3);

      index++;
      if(index >= (((SM_PREVIEW_WIDTH-2)*(SM_PREVIEW_HEIGHT-2))/9))
	break;
    }
}


static void
palette_select_callback (gint              r,
			 gint              g,
			 gint              b,
			 ColorSelectState  state,
			 void             *data)
{
  PaletteDialog *palette;
  guchar  *color;
  GSList  *list;
  gint pos = 0;
  PaletteEntriesP p_entries = NULL;
  
  palette = data;

  if (palette && palette->entries)
    {
      switch (state)
	{
	case COLOR_SELECT_UPDATE:
	  break;
	case COLOR_SELECT_OK:
	  if (palette->color)
	    {
	      color = palette->color->color;

	      color[0] = r;
	      color[1] = g;
	      color[2] = b;

	      /*  Update either foreground or background colors  */
	      if (active_color == FOREGROUND)
		palette_set_foreground (r, g, b);
	      else if (active_color == BACKGROUND)
		palette_set_background (r, g, b);
	  
	      palette_draw_entries (palette,
				    palette->color->position/(palette->columns),
				    palette->color->position%(palette->columns));
	      palette_draw_small_preview (palette->gc,palette->entries);

	      /* Add into the clist */
	      list = palette_entries_list;

	      while (list)
		{
		  p_entries = (PaletteEntriesP) list->data;
		  list = g_slist_next (list);

		  /*  to make sure we get something!  */
		  if (p_entries == NULL)
		    {
		      p_entries = default_palette_entries;
		    }
		  if (p_entries == palette->entries)
		    break;
		  pos++;
		}

	      gtk_clist_set_text (GTK_CLIST (palette->clist),
				  pos, 2, p_entries->name);
	      palette_select_set_text_all (palette->entries);
	      palette_select2_set_text_all (palette->entries);
	    }
	  /* Fallthrough */
	case COLOR_SELECT_CANCEL:
	  color_select_hide (palette->color_select);
	  palette->color_select_active = 0;
	}
    }
}

static void 
palette_scroll_clist_to_current (PaletteDialog *palette)
{
  GSList *list;
  gint pos = 0;
  PaletteEntriesP p_entries;

  if (palette && palette->entries)
    {
      list = palette_entries_list;
      
      while (list)
	{
	  p_entries = (PaletteEntriesP) list->data;
	  list = g_slist_next (list);
	  
	  if (p_entries == palette->entries)
	    break;
	  pos++;
	}

      gtk_clist_unselect_all (GTK_CLIST (palette->clist));
      gtk_clist_select_row (GTK_CLIST (palette->clist), pos, -1);
      gtk_clist_moveto (GTK_CLIST (palette->clist), pos, 0, 0.0, 0.0); 
    }
}

static void
palette_delete_entries_callback (GtkWidget *widget,
				 gpointer   data)
{
  PaletteDialog *palette;
  PaletteEntriesP entries;

  palette = data;
  if (palette && palette->entries)
    {
      entries = palette->entries;
      if (entries && entries->filename)
	palette_entries_delete (entries->filename);

      palette_entries_list = g_slist_remove (palette_entries_list, entries);

      palette_refresh (palette);
    }
}

static void
palette_close_callback (GtkWidget *widget,
			gpointer   data)
{
  PaletteDialog *palette;

  palette = data;
  if (palette)
    {
      if (palette->color_select_active)
	{
	  palette->color_select_active = 0;
	  color_select_hide (palette->color_select);
	}

      if(import_dialog)
	{
	  gtk_widget_destroy (import_dialog->dialog);
	  g_free (import_dialog);
	  import_dialog = NULL;
	}
      
      if (GTK_WIDGET_VISIBLE (palette->shell))
	gtk_widget_hide (palette->shell);
    }
}

static gint
palette_dialog_delete_callback (GtkWidget *widget,
				GdkEvent  *event,
				gpointer   data) 
{
  palette_close_callback (widget, data);

  return TRUE;
}


static void
color_name_entry_changed (GtkWidget *widget,
			  gpointer   data)
{
  PaletteDialog *palette;

  palette = data;
  if (palette->color->name)
    g_free (palette->color->name);
  palette->entries->changed = 1;
  palette->color->name = 
    g_strdup (gtk_entry_get_text (GTK_ENTRY (palette->color_name)));
}


static void
palette_edit_callback (GtkWidget *widget,
		       gpointer   data)
{
  PaletteDialog *palette;
  guchar *color;

  palette = data;
  if (palette && palette->entries && palette->color)
    {
      color = palette->color->color;

      if (!palette->color_select)
	{
	  palette->color_select =
	    color_select_new (color[0], color[1], color[2],
			      palette_select_callback, palette,
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

	  color_select_set_color (palette->color_select,
				  color[0], color[1], color[2], 1);
	}
    }
}


static void
palette_popup_menu (PaletteDialog *palette)
{
  GtkWidget *menu;
  GtkWidget *menu_items;

  menu = gtk_menu_new ();
  menu_items = gtk_menu_item_new_with_label (_("Edit"));
  gtk_menu_append (GTK_MENU (menu), menu_items);

  gtk_signal_connect (GTK_OBJECT (menu_items), "activate", 
		      GTK_SIGNAL_FUNC (palette_edit_callback),
		      (gpointer) palette);
  gtk_widget_show (menu_items);

  menu_items = gtk_menu_item_new_with_label (_("New"));
  gtk_menu_append (GTK_MENU (menu), menu_items);
  gtk_signal_connect (GTK_OBJECT (menu_items), "activate", 
		      GTK_SIGNAL_FUNC (palette_new_callback),
		      (gpointer) palette);
  gtk_widget_show (menu_items);

  menu_items = gtk_menu_item_new_with_label (_("Delete"));
  gtk_signal_connect (GTK_OBJECT(menu_items), "activate", 
		      GTK_SIGNAL_FUNC (palette_delete_entry),
		      (gpointer) palette);
  gtk_menu_append (GTK_MENU (menu), menu_items);
  gtk_widget_show (menu_items);

  /* Do something interesting when the menuitem is selected */ 
  /*   gtk_signal_connect_object(GTK_OBJECT(menu_items), "activate", */
  /* 			    GTK_SIGNAL_FUNC(menuitem_response), (gpointer) g_strdup(buf)); */

  palette->popup_menu = menu;

  palette->popup_small_menu = menu = gtk_menu_new ();
  menu_items = gtk_menu_item_new_with_label (_("New"));
  gtk_menu_append (GTK_MENU (menu), menu_items);
  gtk_signal_connect (GTK_OBJECT (menu_items), "activate", 
		      GTK_SIGNAL_FUNC (palette_new_callback),
		      (gpointer) palette);
  gtk_widget_show (menu_items);
}

static gint
palette_color_area_events (GtkWidget     *widget,
			   GdkEvent      *event,
			   PaletteDialog *palette)
{
  GdkEventButton *bevent;
  GSList *tmp_link;
  int r, g, b;
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
      entry_width = (ENTRY_WIDTH*palette->zoom_factor)+SPACING;
      entry_height = (ENTRY_HEIGHT*palette->zoom_factor)+SPACING;
      col = (bevent->x - 1) / entry_width;
      row = (bevent->y - 1) / entry_height;
      pos = row * palette->columns + col;
      
      if ((bevent->button == 1 || bevent->button == 3) && palette->entries)
	{
	  tmp_link = g_slist_nth (palette->entries->colors, pos);
	  if (tmp_link)
	    {
	      if(palette->color)
		{
		  palette->freeze_update = TRUE;
 		  palette_draw_entries (palette, -1, -1);
		  palette->freeze_update = FALSE;
		}
	      palette->color = tmp_link->data;

	      /*  Update either foreground or background colors  */
	      r = palette->color->color[0];
	      g = palette->color->color[1];
	      b = palette->color->color[2];
	      if (active_color == FOREGROUND)
		palette_set_foreground (r, g, b);
	      else if (active_color == BACKGROUND)
		palette_set_background (r, g, b);

	      palette_draw_entries (palette, row, col);
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
	      if(bevent->button == 3)
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

void 
palette_insert_clist (GtkWidget       *clist, 
		      GtkWidget       *shell,
		      GdkGC           *gc,
		      PaletteEntriesP  p_entries,
		      gint             pos)
{
  gchar *string[3];

  string[0] = NULL;
  string[1] = g_strdup_printf ("%d", p_entries->n_colors);
  string[2] = p_entries->name;

  gtk_clist_insert (GTK_CLIST (clist), pos, string);

  g_free (string[1]);
  
  if (p_entries->pixmap == NULL)
    p_entries->pixmap = gdk_pixmap_new (shell->window,
					SM_PREVIEW_WIDTH, 
					SM_PREVIEW_HEIGHT, 
					gtk_widget_get_visual (shell)->depth);
  
  palette_draw_small_preview (gc, p_entries);
  gtk_clist_set_pixmap (GTK_CLIST (clist), pos, 0, p_entries->pixmap, NULL);
  gtk_clist_set_row_data (GTK_CLIST (clist), pos, (gpointer) p_entries);
}  

void
palette_clist_init (GtkWidget *clist, 
		    GtkWidget *shell,
		    GdkGC     *gc)
{
  GSList *list;
  PaletteEntriesP p_entries = NULL;
  gint pos = 0;

  list = palette_entries_list;

  while (list)
    {
      p_entries = (PaletteEntriesP) list->data;
      list = g_slist_next (list);

      /*  to make sure we get something!  */
      if (p_entries == NULL)
	{
	  p_entries = default_palette_entries;
	}

      palette_insert_clist (clist, shell, gc, p_entries, pos);
      pos++;
    }
}

static int
palette_draw_color_row (guchar        **colors,
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

  if (!palette)
    return -1;

  preview = palette->color_area;

  bcolor = 0;

  width = preview->requisition.width;
  height = preview->requisition.height;
  entry_width = (ENTRY_WIDTH * palette->zoom_factor);
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
palette_draw_entries (PaletteDialog *palette,
		      gint           row_start,
		      gint           column_highlight)
{
  PaletteEntryP entry;
  guchar *buffer;
  guchar **colors;
  GSList *tmp_link;
  gint width, height;
  gint entry_width;
  gint entry_height;
  gint index, y;

  if (palette && palette->entries)
    {
      width = palette->color_area->requisition.width;
      height = palette->color_area->requisition.height;

      entry_width = (ENTRY_WIDTH * palette->zoom_factor);
      entry_height = (ENTRY_HEIGHT * palette->zoom_factor);

      colors = g_malloc (sizeof (guchar *) * palette->columns * 3);
      buffer = g_malloc (width * 3);

      if(row_start < 0)
	{
	  y = 0;
	  tmp_link = palette->entries->colors;
	  column_highlight = -1;
	}
      else
	{
	  y = (entry_height+SPACING)*row_start;
	  tmp_link = g_slist_nth (palette->entries->colors, row_start*palette->columns);
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
	      y = palette_draw_color_row (colors, palette->columns, y, column_highlight, buffer, palette);
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
	  y = palette_draw_color_row (colors, index, y, column_highlight, buffer, palette);
	  index = 0;
          if(row_start >= 0)
	    break;
	}

      g_free (buffer);
      g_free (colors);

      if(palette->freeze_update == FALSE)
	gtk_widget_draw (palette->color_area, NULL);
    }
}

static void
palette_scroll_top_left (PaletteDialog *palette)
{
  GtkAdjustment *hadj; 
  GtkAdjustment *vadj; 

  /* scroll viewport to top left */
  if(palette && palette->scrolled_window)
    {
      hadj = gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (palette->scrolled_window));
      vadj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (palette->scrolled_window));
      
      if(hadj)
	gtk_adjustment_set_value (hadj, 0.0);
      if(vadj)
	gtk_adjustment_set_value (vadj, 0.0);
    }
}

static void
redraw_palette (PaletteDialog *palette)
{
  GtkWidget *parent;
  gint vsize;
  gint nrows;
  gint n_entries;
  gint new_pre_width;

  n_entries = palette->entries->n_colors;
  nrows = n_entries / palette->columns;
  if (n_entries % palette->columns)
    nrows += 1;

  vsize = nrows* (SPACING + (gint)(ENTRY_HEIGHT*palette->zoom_factor)) + SPACING;

  parent = palette->color_area->parent;
  gtk_widget_ref (palette->color_area);
  gtk_container_remove (GTK_CONTAINER (parent), palette->color_area);

  new_pre_width = (gint) (ENTRY_WIDTH * palette->zoom_factor);
  new_pre_width = (new_pre_width + SPACING) * palette->columns + SPACING;

  gtk_preview_size (GTK_PREVIEW (palette->color_area),
		    new_pre_width, /*PREVIEW_WIDTH,*/
		    vsize);

  gtk_container_add (GTK_CONTAINER (parent), palette->color_area);
  gtk_widget_unref (palette->color_area);

  palette_draw_entries (palette, -1, -1);
}

static void
palette_list_item_update (GtkWidget      *widget, 
			  gint            row,
			  gint            column,
			  GdkEventButton *event,
			  gpointer        data)
{
  PaletteDialog *palette;
  PaletteEntriesP p_entries;

  palette = (PaletteDialog *) data;

  if (palette->color_select_active)
    {
      palette->color_select_active = 0;
      color_select_hide (palette->color_select);
    }

  if (palette->color_select)
    color_select_free (palette->color_select);
  palette->color_select = NULL;

  p_entries = 
    (PaletteEntriesP) gtk_clist_get_row_data (GTK_CLIST (palette->clist), row);

  palette->entries = p_entries;

  redraw_palette (palette);

  palette_scroll_top_left (palette);

  /* Stop errors in case no colors are selected */ 
  gtk_signal_handler_block (GTK_OBJECT (palette->color_name),
			    palette->entry_sig_id);
  gtk_entry_set_text (GTK_ENTRY (palette->color_name), _("Undefined")); 
  gtk_widget_set_sensitive (palette->color_name, FALSE);
  gtk_signal_handler_unblock (GTK_OBJECT (palette->color_name),
			      palette->entry_sig_id);
}

static void
palette_edit_palette_callback (GtkWidget *widget,
			       gpointer   data)
{
  PaletteEntriesP p_entries = NULL;
  PaletteDialog *palette;
  GList *sel_list;

  palette = (PaletteDialog *) data;
  sel_list = GTK_CLIST (palette->clist)->selection;

  if (sel_list)
    {
      while (sel_list)
	{
	  gint row;

	  row = GPOINTER_TO_INT (sel_list->data);

	  p_entries = 
	    (PaletteEntriesP)gtk_clist_get_row_data (GTK_CLIST (palette->clist), row);

	  palette_create_edit (p_entries);

	  /* One only */
	  return;
	}
    }
}

PaletteDialog *
create_palette_dialog (gint vert)
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
  
  static ActionAreaItem vert_action_items[] =
  {
    { N_("Edit"), palette_edit_palette_callback, NULL, NULL },
    { N_("Close"), palette_close_callback, NULL, NULL }
  };
  static ActionAreaItem horz_action_items[] =
  {
    { N_("Save"), palette_save_palettes_callback, NULL, NULL },
    { N_("Refresh"), palette_refresh_callback, NULL, NULL },
    { N_("Close"), palette_close_callback, NULL, NULL }
  };

  palette = g_new (PaletteDialog, 1);

  palette->entries = default_palette_entries;
  palette->color = NULL;
  palette->color_select = NULL;
  palette->color_select_active = 0;
  palette->zoom_factor = 1.0;
  palette->columns = COLUMNS;
  palette->freeze_update = FALSE;

  palette->shell = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (palette->shell), "color_palette", "Gimp");
  
  if (!vert)
    {
      gtk_widget_set_usize (palette->shell, 615, 200);
      gtk_window_set_title (GTK_WINDOW (palette->shell),
			    _("Color Palette Edit"));
    }
  else
    {
      gtk_widget_set_usize (palette->shell, 230, 300);
      gtk_window_set_title (GTK_WINDOW (palette->shell),
			    _("Color Palette"));
    }

  /*  Handle the wm delete event  */
  gtk_signal_connect (GTK_OBJECT (palette->shell), "delete_event",
		      GTK_SIGNAL_FUNC (palette_dialog_delete_callback),
		      palette);

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
  if (!vert)
    {
      gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
    }
  gtk_widget_show (vbox);

  alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0); 
  gtk_widget_show (alignment); 

  palette->scrolled_window =
    scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow),
				  GTK_POLICY_AUTOMATIC, 
				  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (vbox), scrolledwindow, TRUE, TRUE, 0);
  gtk_widget_show (scrolledwindow);

  palette->color_area = palette_region = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_set_dither (GTK_PREVIEW (palette->color_area),
			  GDK_RGB_DITHER_MAX);
  gtk_preview_size (GTK_PREVIEW (palette_region), PREVIEW_WIDTH, PREVIEW_HEIGHT);
  
  gtk_widget_set_events (palette_region, PALETTE_EVENT_MASK);
  gtk_signal_connect (GTK_OBJECT (palette->color_area), "event",
		      (GtkSignalFunc) palette_color_area_events,
		      palette);

  gtk_widget_show (palette_region);
  gtk_container_add (GTK_CONTAINER (alignment), palette_region);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolledwindow),
					 alignment);

  /*  The color name entry  */
  hbox2 = gtk_hbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox2, FALSE, FALSE, 0);
  gtk_widget_show(hbox2);

  entry = palette->color_name = gtk_entry_new ();
  gtk_widget_show (entry);
  gtk_box_pack_start (GTK_BOX (hbox2), entry, TRUE, TRUE, 0);
  gtk_entry_set_text (GTK_ENTRY (entry), _("Undefined"));
  palette->entry_sig_id =
    gtk_signal_connect (GTK_OBJECT (entry), "changed",
			GTK_SIGNAL_FUNC (color_name_entry_changed),
			palette);

  /*  + and - buttons  */
  gtk_widget_realize (palette->shell);
  style = gtk_widget_get_style (palette->shell);

  button = gtk_button_new ();
  GTK_WIDGET_UNSET_FLAGS (button, GTK_RECEIVES_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (palette_zoomin), (gpointer) palette);
  gtk_box_pack_start (GTK_BOX (hbox2), button, FALSE, FALSE, 0);

  pixmap = gdk_pixmap_create_from_xpm_d (palette->shell->window, &mask,
					 &style->bg[GTK_STATE_NORMAL], 
					 zoom_in_xpm);
  pixmapwid = gtk_pixmap_new (pixmap, mask);
  gtk_container_add (GTK_CONTAINER (button), pixmapwid);
  gtk_widget_show (pixmapwid);
  gtk_widget_show (button);

  button = gtk_button_new ();
  GTK_WIDGET_UNSET_FLAGS (button, GTK_RECEIVES_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (palette_zoomout), (gpointer) palette);
  gtk_box_pack_start (GTK_BOX (hbox2), button, FALSE, FALSE, 0);

  pixmap = gdk_pixmap_create_from_xpm_d (palette->shell->window, &mask,
					 &style->bg[GTK_STATE_NORMAL], 
					 zoom_out_xpm);
  pixmapwid = gtk_pixmap_new (pixmap, mask);
  gtk_container_add (GTK_CONTAINER (button), pixmapwid);
  gtk_widget_show (pixmapwid);
  gtk_widget_show (button);
  
  /*  clist preview of palettes  */
  scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
  palette->clist = palette_list = gtk_clist_new (3);
  gtk_clist_set_row_height (GTK_CLIST (palette_list), SM_PREVIEW_HEIGHT + 2);
  gtk_signal_connect (GTK_OBJECT (palette->clist), "select_row",
		      GTK_SIGNAL_FUNC (palette_list_item_update),
		      (gpointer) palette);
  if (vert)
    {
      gtk_widget_set_usize (palette_list, 203, 90);

      gtk_notebook_append_page (GTK_NOTEBOOK (hbox), vbox,
				gtk_label_new (_("Palette")));
      gtk_notebook_append_page (GTK_NOTEBOOK (hbox), scrolledwindow,
				gtk_label_new (_("Select")));
      gtk_widget_show (palette_list);
    }
  else
    {
      gtk_container_add (GTK_CONTAINER (hbox), scrolledwindow);
      gtk_widget_set_usize (palette_list, 203, -1);
      gtk_widget_show (palette_list);
    }

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
      gtk_box_pack_end (GTK_BOX (hbox), frame, FALSE, FALSE, 0);
      gtk_widget_show (frame); 

      vbox = gtk_vbox_new (FALSE, 2);
      gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
      gtk_container_add (GTK_CONTAINER (frame), vbox);
      gtk_widget_show (vbox);
      
      button = gtk_button_new_with_label (_("New"));
      GTK_WIDGET_UNSET_FLAGS (button, GTK_RECEIVES_DEFAULT);
      gtk_misc_set_padding (GTK_MISC (GTK_BIN (button)->child), 2, 0);
      gtk_signal_connect (GTK_OBJECT (button), "clicked",
			  (GtkSignalFunc) palette_new_entries_callback,
			  (gpointer) palette);
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gtk_widget_show (button);

      button = gtk_button_new_with_label (_("Delete"));
      GTK_WIDGET_UNSET_FLAGS (button, GTK_RECEIVES_DEFAULT);
      gtk_misc_set_padding (GTK_MISC (GTK_BIN (button)->child), 2, 0);
      gtk_signal_connect (GTK_OBJECT (button), "clicked",
			  (GtkSignalFunc) palette_delete_entries_callback,
			  (gpointer) palette);
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gtk_widget_show (button);
      
      button = gtk_button_new_with_label (_("Import"));
      GTK_WIDGET_UNSET_FLAGS (button, GTK_RECEIVES_DEFAULT);
      gtk_misc_set_padding (GTK_MISC (GTK_BIN (button)->child), 2, 0);
      gtk_signal_connect (GTK_OBJECT (button), "clicked",
			  (GtkSignalFunc) palette_import_dialog_callback,
			  (gpointer) palette);
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gtk_widget_show (button);

      button = gtk_button_new_with_label (_("Merge"));
      GTK_WIDGET_UNSET_FLAGS (button, GTK_RECEIVES_DEFAULT);
      gtk_misc_set_padding (GTK_MISC (GTK_BIN (button)->child), 2, 0);
      gtk_signal_connect (GTK_OBJECT (button), "clicked",
			  (GtkSignalFunc) palette_merge_dialog_callback,
			  (gpointer) palette);
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gtk_widget_show (button);
    }

  if (!vert)
    {
      horz_action_items[0].user_data = palette;
      horz_action_items[1].user_data = palette;
      horz_action_items[2].user_data = palette;
      build_action_area (GTK_DIALOG (palette->shell), horz_action_items, 3, 2);
    }
  else
    {
      vert_action_items[0].user_data = palette;
      vert_action_items[1].user_data = palette;
      build_action_area (GTK_DIALOG (palette->shell), vert_action_items, 2, 1);
    }

  palette->gc = gdk_gc_new (palette->shell->window);

  palette_popup_menu (palette);

  return palette;
}

static void
import_dialog_select_grad_callback (GtkWidget *widget,
				    gpointer   data)
{
  /* Popup grad edit box .... */
  grad_create_gradient_editor ();
}

static void
import_dialog_close_callback (GtkWidget *widget,
			      gpointer   data)
{
  gtk_widget_destroy (import_dialog->dialog);
  g_free (import_dialog);
  import_dialog = NULL;
}

static void
import_palette_create_from_grad (gchar         *name,
				 PaletteDialog *palette)
{
  PaletteEntriesP entries;

  if(curr_gradient)
    {
      /* Add names to entry */
      double  dx, cur_x;
      double  r, g, b, a;
      extern void grad_get_color_at(double, double *, double *, double *, double *);
      gint sample_sz;
      gint loop;

      entries = palette_create_entries (palette, name);
      sample_sz = (gint)import_dialog->sample->value;  

      dx    = 1.0/ (sample_sz - 1);
      cur_x = 0;
      
      for (loop = 0 ; loop < sample_sz; loop++)
	{
	  grad_get_color_at(cur_x, &r, &g, &b, &a);
	  r = r * 255.0;
	  g = g * 255.0;
	  b = b * 255.0;
	  cur_x += dx;
	  palette_add_entry (palette->entries, _("Untitled"),
			     (gint) r, (gint) g, (gint) b);
	}
      palette_update_small_preview (palette);
      redraw_palette (palette);
      /* Update other selectors on screen */
      palette_select_clist_insert_all (entries);
      palette_select2_clist_insert_all (entries);
      palette_scroll_clist_to_current (palette);
    }
}

static void
import_dialog_import_callback (GtkWidget *widget,
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
	  import_palette_create_from_grad (pname, palette);
	  break;
	case IMAGE_IMPORT:
	  import_palette_create_from_image (import_dialog->gimage,
					    pname, palette);
	  break;
	default:
	  break;
	}
      import_dialog_close_callback (NULL, NULL);
    }
}

static gint
import_dialog_delete_callback (GtkWidget *widget,
			       GdkEvent  *event,
			       gpointer   data) 
{
  import_dialog_close_callback (widget, data);

  return TRUE;
}


static void
palette_merge_entries_callback (GtkWidget *widget,
				gpointer   data,
				gpointer   call_data)
{
  PaletteDialog *palette;
  PaletteEntriesP p_entries;
  PaletteEntriesP new_entries;
  GList *sel_list;

  new_entries = palette_create_entries (data, call_data);

  palette = (PaletteDialog *) data;

  sel_list = GTK_CLIST (palette->clist)->selection;

  if (sel_list)
    {
      while (sel_list)
	{
	  gint row;
	  GSList *cols;

	  row = GPOINTER_TO_INT (sel_list->data);
	  p_entries = 
	    (PaletteEntriesP)gtk_clist_get_row_data (GTK_CLIST (palette->clist), row);

	  /* Go through each palette and merge the colors */
	  cols = p_entries->colors;
	  while (cols)
	    {
	      PaletteEntryP entry = cols->data;
	      palette_add_entry (new_entries,
				 g_strdup (entry->name),
				 entry->color[0],
				 entry->color[1],
				 entry->color[2]);
	      cols = cols->next;
	    }
	  sel_list = sel_list->next;
	}
    }

  palette_update_small_preview (palette);
  redraw_palette (palette);
  gtk_clist_unselect_all (GTK_CLIST (palette->clist));
  /* Update other selectors on screen */
  palette_select_clist_insert_all (new_entries);
  palette_select2_clist_insert_all (new_entries);
  palette_scroll_clist_to_current (palette);
}

static void
palette_merge_dialog_callback (GtkWidget *widget,
			       gpointer   data)
{
  gtk_widget_show (query_string_box (_("Merge Palette"),
				     _("Enter a name for merged palette"),
				     NULL,
				     NULL, NULL,
				     palette_merge_entries_callback,
				     data));
}

static void
palette_import_dialog_callback (GtkWidget *widget,
				gpointer   data)
{
  if (!import_dialog)
    {
      import_dialog = palette_import_dialog ((PaletteDialog *) data);
      gtk_widget_show (import_dialog->dialog);
    }
  else
    {
      gdk_window_raise (import_dialog->dialog->window);
    }
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
  extern void grad_get_color_at(double, double *, double *, double *, double *);
  gradient_t *last_grad;

  dx    = 1.0/ (IMPORT_PREVIEW_WIDTH - 1);
  cur_x = 0;

  last_grad = curr_gradient;
  curr_gradient = gradient;
  for (loop = 0 ; loop < IMPORT_PREVIEW_WIDTH; loop++)
    {
      grad_get_color_at(cur_x, &r, &g, &b, &a);
      *p++ = r * 255.0;
      *p++ = g * 255.0;
      *p++ = b * 255.0;
      cur_x += dx;
    }
  curr_gradient = last_grad;

  for (loop = 0 ; loop < IMPORT_PREVIEW_HEIGHT; loop++)
    {
      gtk_preview_draw_row (GTK_PREVIEW (preview), buffer, 0, loop, IMPORT_PREVIEW_WIDTH);
    }
  gtk_widget_draw (preview, NULL);
}


void
import_palette_grad_update (gradient_t *grad)
{
  if (import_dialog && import_dialog->import_type == GRAD_IMPORT)
    {
      /* redraw gradient */
      palette_import_fill_grad_preview (import_dialog->preview, grad);
      gtk_entry_set_text (GTK_ENTRY (import_dialog->entry), grad->name);
    }
}

static void
import_grad_callback (GtkWidget *widget,
		      gpointer   data)
{
  if (import_dialog)
    {
      import_dialog->import_type = GRAD_IMPORT;
      if (import_dialog->image_list)
	{
	  gtk_widget_hide (import_dialog->image_list);
	  gtk_widget_destroy (import_dialog->image_list);
	  import_dialog->image_list = NULL;
	}
      gtk_widget_show (import_dialog->select);
      palette_import_fill_grad_preview (import_dialog->preview, curr_gradient);

      gtk_entry_set_text (GTK_ENTRY (import_dialog->entry), curr_gradient->name);
      gtk_widget_set_sensitive (import_dialog->threshold_scale, FALSE);
      gtk_widget_set_sensitive (import_dialog->threshold_text, FALSE);
    }
}

static void
gimlist_cb (gpointer im,
	    gpointer data)
{
  GSList** l;

  l = (GSList**) data;
  *l = g_slist_prepend (*l, im);
}

static void
import_image_update_image_preview (GimpImage *gimage)
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
import_image_sel_callback (GtkWidget *widget,
			   gpointer   data)
{
  GimpImage *gimage;
  gchar *lab;

  gimage = GIMP_IMAGE (data);
  import_image_update_image_preview (gimage);

  lab = g_strdup_printf ("%s-%d",
			 g_basename (gimage_filename (import_dialog->gimage)),
			 pdb_image_to_id (import_dialog->gimage));

  gtk_entry_set_text (GTK_ENTRY (import_dialog->entry), lab);
}

static void
import_image_menu_add (GimpImage *gimage)
{
  GtkWidget *menuitem;
  gchar *lab = g_strdup_printf ("%s-%d",
				g_basename (gimage_filename (gimage)),
				pdb_image_to_id (gimage));
  menuitem = gtk_menu_item_new_with_label (lab);
  gtk_widget_show (menuitem);
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
		      (GtkSignalFunc) import_image_sel_callback,
		      gimage);
  gtk_menu_append (GTK_MENU (import_dialog->optionmenu1_menu), menuitem);
}

/* Last Param gives us control over what goes in the menu on a delete oper */
static void
import_image_menu_activate (gint       redo,
			    GimpImage *del_image)
{
  GSList *list=NULL;
  gint num_images;
  GimpImage *last_img = NULL;
  GimpImage *first_img = NULL;
  gint act_num = -1;
  gint count = 0;
  gchar *lab;

  if (import_dialog)
    {
      if (import_dialog->import_type == IMAGE_IMPORT)
	{
	  if (!redo)
	    return;
	  else
	    {
	      if (import_dialog->image_list)
		{
		  last_img = import_dialog->gimage;
		  gtk_widget_hide (import_dialog->image_list);
		  gtk_widget_destroy (import_dialog->image_list);
		  import_dialog->image_list = NULL;
		}
	    }
	}
      import_dialog->import_type = IMAGE_IMPORT;

      /* Get list of images */
      gimage_foreach (gimlist_cb, &list);
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
		  import_image_menu_add (GIMP_IMAGE (list->data));
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
	      import_image_update_image_preview (last_img);
	  else if (first_img != NULL)
	      import_image_update_image_preview (first_img);

	  gtk_widget_show (optionmenu1);

	  /* reset to last one */
	  if(redo && act_num >= 0)
	    {
	      gchar *lab =
		g_strdup_printf ("%s-%d",
				 g_basename (gimage_filename (import_dialog->gimage)),
				 pdb_image_to_id (import_dialog->gimage));

	      gtk_option_menu_set_history (GTK_OPTION_MENU (optionmenu1), act_num);
	      gtk_entry_set_text (GTK_ENTRY (import_dialog->entry), lab);
	    }
	}
      g_slist_free (list);

      lab =
	g_strdup_printf ("%s-%d",
			 g_basename (gimage_filename (import_dialog->gimage)),
			 pdb_image_to_id (import_dialog->gimage));

      gtk_entry_set_text (GTK_ENTRY (import_dialog->entry), lab);
    }
}

static void
import_image_callback (GtkWidget *widget,
		       gpointer   data)
{
  import_image_menu_activate (FALSE, NULL);
  gtk_widget_set_sensitive (import_dialog->threshold_scale, TRUE);
  gtk_widget_set_sensitive (import_dialog->threshold_text, TRUE);
}

static gint
image_count ()
{
  GSList *list=NULL;
  gint num_images = 0;

  gimage_foreach (gimlist_cb, &list);
  num_images = g_slist_length (list);

  g_slist_free (list);

  return num_images;
}

static ImportDialog *
palette_import_dialog (PaletteDialog *palette)
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

  static ActionAreaItem action_items[] =
  {
    { N_("Import"), import_dialog_import_callback, NULL, NULL },
    { N_("Close"), import_dialog_close_callback, NULL, NULL }
  };

  import_dialog = g_new (ImportDialog, 1);
  import_dialog->image_list = NULL;
  import_dialog->gimage = NULL;

  import_dialog->dialog = dialog = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (dialog), "import_palette", "Gimp");
  gtk_window_set_title (GTK_WINDOW (dialog), _("Import Palette"));

  /*  Handle the wm delete event  */
  gtk_signal_connect (GTK_OBJECT (dialog), "delete_event",
		      GTK_SIGNAL_FUNC (import_dialog_delete_callback),
		      (gpointer)palette);

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
  gtk_entry_set_text (GTK_ENTRY (entry),
		      curr_gradient ? curr_gradient->name : _("new_import"));
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
		      (GtkSignalFunc) import_grad_callback,
		      NULL);
  gtk_menu_append (GTK_MENU (optionmenu_menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = import_dialog->image_menu_item_image =
    gtk_menu_item_new_with_label ("Image");
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
		      (GtkSignalFunc) import_image_callback,
		      (gpointer) import_dialog);
  gtk_menu_append (GTK_MENU (optionmenu_menu), menuitem);
  gtk_widget_show (menuitem);

  gtk_option_menu_set_menu (GTK_OPTION_MENU (optionmenu), optionmenu_menu);
  gtk_widget_set_sensitive (menuitem, image_count() > 0);
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
		      GTK_SIGNAL_FUNC (import_dialog_select_grad_callback),
		      (gpointer) image);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  /*  The action area  */
  action_items[0].user_data = palette;
  action_items[1].user_data = palette;
  build_action_area (GTK_DIALOG (dialog), action_items, 2, 1);

  /*  Fill with the selected gradient  */
  palette_import_fill_grad_preview (image, curr_gradient);
  import_dialog->import_type = GRAD_IMPORT;
  return import_dialog;
}

/* Stuff to keep dialog uptodate */

void
palette_import_image_new (GimpImage * gimage)
{
  if (!import_dialog)
    return;

  if (!GTK_WIDGET_IS_SENSITIVE (import_dialog->image_menu_item_image))
    {
      gtk_widget_set_sensitive (import_dialog->image_menu_item_image, TRUE);
      return;
    }

  /* Now fill in the names if image menu shown */
  if (import_dialog->import_type == IMAGE_IMPORT)
    {
      import_image_menu_activate (TRUE, NULL);
    }
}

void
palette_import_image_destroyed (GimpImage* gimage)
{
  /* Now fill in the names if image menu shown */

  if (!import_dialog)
    return;

  if (image_count() <= 1)
    {
      /* Back to gradient type */
      gtk_option_menu_set_history (GTK_OPTION_MENU (import_dialog->type_option), 0);
      import_grad_callback (NULL, NULL);
      if (import_dialog->image_menu_item_image)
	gtk_widget_set_sensitive (import_dialog->image_menu_item_image, FALSE);
      return;
    }

  if (import_dialog->import_type == IMAGE_IMPORT)
    {
      import_image_menu_activate (TRUE, gimage);
    }
}

void
palette_import_image_renamed (GimpImage* gimage)
{
  /* Now fill in the names if image menu shown */
  if(import_dialog && import_dialog->import_type == IMAGE_IMPORT)
    {
      import_image_menu_activate (TRUE, NULL);
    }
}

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

static void
create_storted_list (gpointer key,
		     gpointer value,
		     gpointer user_data)
{
  GSList    **sorted_list = (GSList**) user_data;
  ImgColors  *color_tab  = (ImgColors *) value;

  *sorted_list = g_slist_prepend (*sorted_list, color_tab);
}

static void 
create_image_palette (gpointer data,
		      gpointer user_data)
{
  PaletteDialog *palette = (PaletteDialog *) user_data;
  ImgColors *color_tab = (ImgColors *) data;
  gint sample_sz;
  gchar *lab;

  sample_sz = (gint) import_dialog->sample->value;  

  if (palette->entries->n_colors >= sample_sz)
    return;

  lab = g_strdup_printf ("%s (occurs %u)", _("Untitled"), color_tab->count);

  /* Adjust the colors to the mean of the the sample */
  palette_add_entry (palette->entries, lab, 
		     (gint)color_tab->r + (color_tab->r_adj/color_tab->count), 
		     (gint)color_tab->g + (color_tab->g_adj/color_tab->count), 
		     (gint)color_tab->b + (color_tab->b_adj/color_tab->count));
}

static gboolean
color_print_remove (gpointer key,
		    gpointer value,
		    gpointer user_data)
{
  g_free (value);

  return TRUE;
}

static gint 
sort_colors (gconstpointer a,
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
import_image_make_palette (GHashTable    *h_array,
			   guchar        *name,
			   PaletteDialog *palette)
{
  GSList * sorted_list = NULL;
  PaletteEntriesP entries;

  g_hash_table_foreach (h_array, create_storted_list, &sorted_list);
  sorted_list = g_slist_sort (sorted_list, sort_colors);

  entries = palette_create_entries (palette, name);
  g_slist_foreach (sorted_list, create_image_palette, palette);

  /* Free up used memory */
  /* Note the same structure is on both the hash list and the sorted
   * list. So only delete it once.
   */
  g_hash_table_freeze (h_array);
  g_hash_table_foreach_remove (h_array, color_print_remove, NULL);
  g_hash_table_thaw (h_array);
  g_hash_table_destroy (h_array);
  g_slist_free (sorted_list);

  /* Redraw with new palette */
  palette_update_small_preview (palette);
  redraw_palette (palette);
  /* Update other selectors on screen */
  palette_select_clist_insert_all (entries);
  palette_select2_clist_insert_all (entries);
  palette_scroll_clist_to_current (palette);
}

static GHashTable *
store_colors (GHashTable *h_array, 
	      guchar     *colors,
	      guchar     *colors_real, 
	      gint        sample_sz)
{
  gpointer found_color = NULL;
  ImgColors *new_color;
  guint key_colors = colors[0]*256*256+colors[1]*256+colors[2];

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
import_palette_create_from_image (GImage        *gimage,
				  guchar        *pname,
				  PaletteDialog *palette)
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
		store_colors (store_array, rgb, rgb_real, sample_sz);

	      idata += bytes;
	    }

	  image_data += imagePR.rowstride;
	}
    }

  /* Make palette from the store_array */
  import_image_make_palette (store_array, pname, palette);
}
