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
#include "devices.h"
#include "errors.h"
#include "general.h"
#include "gimprc.h"
#include "gradient_header.h"
#include "gradient.h"
#include "interface.h"
#include "palette.h"
#include "palette_entries.h"
#include "session.h"
#include "palette_select.h"

#include "libgimp/gimpintl.h"

#define ENTRY_WIDTH  12
#define ENTRY_HEIGHT 10
#define SPACING 1
#define COLUMNS 16
#define ROWS 11

#define PREVIEW_WIDTH ((ENTRY_WIDTH * COLUMNS) + (SPACING * (COLUMNS + 1)))
#define PREVIEW_HEIGHT ((ENTRY_HEIGHT * ROWS) + (SPACING * (ROWS + 1)))

#define PALETTE_EVENT_MASK GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK | GDK_ENTER_NOTIFY_MASK

/* New palette code... */

#define IMPORT_PREVIEW_WIDTH 80
#define IMPORT_PREVIEW_HEIGHT 80
#define MAX_IMAGE_COLOURS (10000*2)

typedef enum
{
  GRAD_IMPORT = 0,
  IMAGE_IMPORT = 1,
} ImportType;


struct _ImportDialog {
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


typedef struct _ImportDialog ImportDialog ,*ImportDialogP;
typedef struct _Palette _Palette, *PaletteP;

struct _Palette {
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


static void       palette_entries_free (PaletteEntriesP);
static void       palette_entries_load (char *);
static void       palette_entry_free (PaletteEntryP);
static void       palette_entries_save (PaletteEntriesP, char *);

PaletteP          create_palette_dialog ();
static void       palette_draw_entries (PaletteP palette,gint row_start, gint column_highlight);
static void       redraw_palette(PaletteP palette);
static GSList *   palette_entries_insert_list (GSList *list,PaletteEntriesP entries,gint pos);
void       palette_clist_init(GtkWidget *clist, 
			      GtkWidget *shell,
			      GdkGC     *gc);
static void       palette_draw_small_preview(GdkGC *gc, PaletteEntriesP p_entry);
static void       palette_scroll_clist_to_current(PaletteP palette);
static void       palette_update_small_preview(PaletteP palette);
static void       palette_scroll_top_left(PaletteP palette);
static ImportDialogP palette_import_dialog(PaletteP palette);
static void       palette_import_dialog_callback (GtkWidget *w,gpointer client_data);
static void       import_palette_create_from_image (GImage *gimage,guchar *pname,PaletteP palette);
static void       palette_merge_dialog_callback (GtkWidget *w,gpointer client_data);


GSList *palette_entries_list = NULL;
static PaletteEntriesP  default_palette_entries = NULL;
static int              num_palette_entries = 0;
static unsigned char    foreground[3] = { 0, 0, 0 };
static unsigned char    background[3] = { 255, 255, 255 };
static ImportDialogP import_dialog = NULL;

PaletteP top_level_edit_palette = NULL;

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
palette_entries_delete (char *filename)
{
  if (filename)
    unlink (filename);
}

void
palettes_init (int no_data)
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
palette_get_foreground (unsigned char *r,
			unsigned char *g,
			unsigned char *b)
{
  *r = foreground[0];
  *g = foreground[1];
  *b = foreground[2];
}

void
palette_get_background (unsigned char *r,
			unsigned char *g,
			unsigned char *b)
{
  *r = background[0];
  *g = background[1];
  *b = background[2];
}

void
palette_set_foreground (int r,
			int g,
			int b)
{
  unsigned char rr, gg, bb;

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
palette_set_background (int r,
			int g,
			int b)
{
  unsigned char rr, gg, bb;

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
  unsigned char fg_r, fg_g, fg_b;
  unsigned char bg_r, bg_g, bg_b;
  
  palette_get_foreground (&fg_r, &fg_g, &fg_b);
  palette_get_background (&bg_r, &bg_g, &bg_b);
  
  palette_set_foreground (bg_r, bg_g, bg_b);
  palette_set_background (fg_r, fg_g, fg_b);
}

void
palette_init_palettes (int no_data)
{
  if(!no_data)
    datafiles_read_directories (palette_path, palette_entries_load, 0);

}

static void
palette_save_palettes()
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
palette_save_palettes_callback(GtkWidget *w,
			       gpointer   client_data)
{
  palette_save_palettes();
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
	  gtk_widget_destroy(import_dialog->dialog);
	  g_free(import_dialog);
	  import_dialog = NULL;
	}

      gdk_gc_destroy (top_level_edit_palette->gc); 
      
      if (top_level_edit_palette->color_select) 
	color_select_free (top_level_edit_palette->color_select); 
      
      g_free (top_level_edit_palette); 
      
      top_level_edit_palette = NULL; 
    }

  if(top_level_palette)
    {
      gdk_gc_destroy (top_level_palette->gc); 
      session_get_window_info (top_level_palette->shell, &palette_session_info);  
      top_level_palette = NULL;
    }
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
  if (! (fp = fopen (filename, "wb")))
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
		   char            *name,
		   int              r,
		   int              g,
		   int              b)
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
palette_change_color (int r,
		      int g,
		      int b,
		      int state)
{
  if (top_level_edit_palette && top_level_edit_palette->entries) 
    { 
      switch (state) 
 	{ 
 	case COLOR_NEW: 
 	  top_level_edit_palette->color = palette_add_entry (top_level_edit_palette->entries, _("Untitled"), r, g, b); 
	  
	  palette_update_small_preview(top_level_edit_palette);
	  redraw_palette(top_level_edit_palette);
 	  break; 
	  
 	case COLOR_UPDATE_NEW: 
 	  top_level_edit_palette->color->color[0] = r; 
 	  top_level_edit_palette->color->color[1] = g; 
 	  top_level_edit_palette->color->color[2] = b; 
	  palette_update_small_preview(top_level_edit_palette);
	  redraw_palette(top_level_edit_palette);
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
palette_set_active_color (int r,
			  int g,
			  int b,
			  int state)
{
  palette_change_color (r, g, b, state);
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
  unsigned char r, g, b;
  unsigned char *col;

  palette_get_foreground (&r, &g, &b);
  col = (unsigned char *) g_malloc (3);
  col[RED_PIX] = r;
  col[GREEN_PIX] = g;
  col[BLUE_PIX] = b;

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
  unsigned char r, g, b;
  unsigned char *col;

  palette_get_background (&r, &g, &b);
  col = (unsigned char *) g_malloc (3);
  col[RED_PIX] = r;
  col[GREEN_PIX] = g;
  col[BLUE_PIX] = b;

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
    palette_set_foreground (color[RED_PIX], color[GREEN_PIX], color[BLUE_PIX]);

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
    palette_set_background (color[RED_PIX], color[GREEN_PIX], color[BLUE_PIX]);

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


static void
palette_entries_load (char *filename)
{
  PaletteEntriesP entries;
  char            str[512];
  char           *tok;
  FILE           *fp;
  int             r, g, b;
  GSList *list;
  gint pos = 0;
  PaletteEntriesP p_entries = NULL;


  r = g = b = 0;

  entries = (PaletteEntriesP) g_malloc (sizeof (_PaletteEntries));

  entries->filename = g_strdup (filename);
  entries->name = g_strdup (prune_filename (filename));
  entries->colors = NULL;
  entries->n_colors = 0;
  entries->pixmap = NULL;

  /*  Open the requested file  */

  if (!(fp = fopen (filename, "rb")))
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
	    r = atoi (tok);

	  tok = strtok (NULL, " \t");
	  if (tok)
	    g = atoi (tok);

	  tok = strtok (NULL, " \t");
	  if (tok)
	    b = atoi (tok);

	  tok = strtok (NULL, "\n");

	  palette_add_entry (entries, tok, r, g, b);
	} /* if */
    } /* while */

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
      if (strcmp(p_entries->name, entries->name) > 0)
	break;
      pos++;
    }
  
  palette_entries_list = palette_entries_insert_list (palette_entries_list, entries,pos);

  /* Check if the current palette is the default one */
  if (strcmp(default_palette, prune_filename(filename)) == 0)
    default_palette_entries = entries;
}

static PaletteP
new_top_palette()
{
  PaletteP p;

  p = create_palette_dialog();
  palette_clist_init(p->clist,p->shell,p->gc);

  return(p);
}

void 
palette_select_palette_init()
{
  /* Load them if they are not already loaded */
  if(top_level_edit_palette == NULL)
    {
      top_level_edit_palette = new_top_palette();
    }
}

void 
palette_create()
{
  if(top_level_palette == NULL)
    {
      top_level_palette = palette_new_selection(_("Palette"),NULL);
      session_set_window_geometry (top_level_palette->shell, &palette_session_info, TRUE);
      gtk_widget_show(top_level_palette->shell);
    }
  else
    {
      if (!GTK_WIDGET_VISIBLE (top_level_palette->shell))
	{
	  gtk_widget_show (top_level_palette->shell);
	}
      else
	{
	  gdk_window_raise(top_level_palette->shell->window);
	}
    }
}

void 
palette_create_edit(PaletteEntriesP entries)
{
  PaletteP p;

  if(top_level_edit_palette == NULL)
    {

      p = new_top_palette();
      
      gtk_widget_show(p->shell);
      
      palette_draw_entries(p,-1,-1);
      
      top_level_edit_palette = p;

    }
  else
    {
      if (!GTK_WIDGET_VISIBLE (top_level_edit_palette->shell))
	{
	  gtk_widget_show (top_level_edit_palette->shell);
	  palette_draw_entries(top_level_edit_palette,-1,-1);
	}
      else
	{
	  gdk_window_raise(top_level_edit_palette->shell->window);
	}
    }

  if(entries != NULL)
    {
      top_level_edit_palette->entries = entries;
      gtk_clist_unselect_all(GTK_CLIST(top_level_edit_palette->clist));
      palette_scroll_clist_to_current(top_level_edit_palette);
    }
}

static GSList *
palette_entries_insert_list (GSList *        list,
				PaletteEntriesP entries,
				gint pos)
{
  GSList *ret_list;
  /*  add it to the list  */
  num_palette_entries++;
  ret_list = g_slist_insert(list, (void *) entries,pos);

  return ret_list;
}

static void palette_update_small_preview(PaletteP palette)
{
  GSList *list;
  gint pos = 0;
  PaletteEntriesP p_entries = NULL;
  char *num_buf;

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
  
  num_buf = g_strdup_printf("%d",p_entries->n_colors);
  palette_draw_small_preview(palette->gc,p_entries);
  gtk_clist_set_text(GTK_CLIST(palette->clist),pos,1,num_buf);
}

static void
palette_delete_entry (GtkWidget *w,
			 gpointer   client_data)
{
  PaletteEntryP entry;
  GSList *tmp_link;
  PaletteP palette;
  gint pos = 0;

  palette = client_data;
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
      palette_update_small_preview(palette);
      palette_select_set_text_all(palette->entries);
      redraw_palette(palette);
    }
}


static void
palette_new_callback (GtkWidget *w,
			 gpointer   client_data)
{
  PaletteP palette;
  char *num_buf;
  GSList *list;
  gint pos = 0;
  PaletteEntriesP p_entries = NULL;

  palette = client_data;
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

      redraw_palette(palette);

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

      num_buf = g_strdup_printf("%d",p_entries->n_colors);;
      palette_draw_small_preview(palette->gc,p_entries);
      gtk_clist_set_text(GTK_CLIST(palette->clist),pos,1,num_buf);
      palette_select_set_text_all(palette->entries);
    }
}


static PaletteEntriesP
palette_create_entries(gpointer   client_data,
			  gpointer   call_data)
{
  char *home;
  char *palette_name;
  char *local_path;
  char *first_token;
  char *token;
  char *path;
  PaletteEntriesP entries = NULL;
  PaletteP palette;
  GSList *list;
  gint pos = 0;
  PaletteEntriesP p_entries = NULL;

  palette = client_data;

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
		  path = g_strdup_printf("%s%s", home, token + 1);
		}
	      else
		{
		  path = g_strdup(token);
		}

	      entries->filename = g_strdup_printf ("%s/%s", path, palette_name);

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

      palette_entries_list = palette_entries_insert_list (palette_entries_list, entries,pos);

      palette_insert_clist(palette->clist,palette->shell,palette->gc,entries,pos);

      palette->entries = entries;

      palette_save_palettes();
    }
  return entries;
}

static void
palette_add_entries_callback (GtkWidget *w,
				 gpointer   client_data,
				 gpointer   call_data)
{
  PaletteEntriesP entries;
  entries = palette_create_entries(client_data,call_data);
  /* Update other selectors on screen */
  palette_select_clist_insert_all(entries);
  palette_scroll_clist_to_current((PaletteP)client_data);
}

static void
palette_new_entries_callback (GtkWidget *w,
				 gpointer   client_data)
{
  query_string_box (_("New Palette"), _("Enter a name for new palette"), NULL,
		    palette_add_entries_callback, client_data);
}

static void
redraw_zoom(PaletteP palette)
{
  if(palette->zoom_factor > 4.0)
    {
      palette->zoom_factor = 4.0;
      return;
    }
  else if(palette->zoom_factor < 0.1)
    {
      palette->zoom_factor = 0.1;
      return;
    }
  
  palette->columns = COLUMNS;

  redraw_palette(palette); 

  palette_scroll_top_left(palette);
}

static void
palette_zoomin(GtkWidget *w,
	       gpointer   client_data)
{
  PaletteP palette = client_data;
  palette->zoom_factor += 0.1;
  redraw_zoom(palette);
}

static void
palette_zoomout (GtkWidget *w,
		 gpointer   client_data)
{
  PaletteP palette = client_data;
  palette->zoom_factor -= 0.1;
  redraw_zoom(palette);
}

static void 
palette_refresh(PaletteP palette)
{
  if(palette)
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

      gtk_clist_freeze(GTK_CLIST(palette->clist));
      gtk_clist_clear(GTK_CLIST(palette->clist));
      palette_clist_init(palette->clist,palette->shell,palette->gc);
      gtk_clist_thaw(GTK_CLIST(palette->clist));

      if(palette->entries == NULL)
	palette->entries = default_palette_entries;

      if(palette->entries == NULL && palette_entries_list)
	palette->entries = palette_entries_list->data;
      
      redraw_palette(palette);

      palette_scroll_clist_to_current(palette);

      palette_select_refresh_all();
    }
  else
    {
      palette_free_palettes (); 
      palette_init_palettes(FALSE);
    }

}

static void
palette_refresh_callback (GtkWidget *w,
			     gpointer client_data)
{
  palette_refresh(client_data);
}

/*****/
static void 
palette_draw_small_preview(GdkGC *gc, PaletteEntriesP p_entry)
{
  guchar rgb_buf[SM_PREVIEW_WIDTH*SM_PREVIEW_HEIGHT*3];
  GSList *tmp_link;
  gint index;
  PaletteEntryP entry;
  
  /*fill_clist_prev(p_entry,rgb_buf,48,16,0.0,1.0);*/
  memset(rgb_buf,0x0,SM_PREVIEW_WIDTH*SM_PREVIEW_HEIGHT*3);
  
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
      int loop;
      
      entry = tmp_link->data;
      tmp_link = tmp_link->next;
      
      for(loop = 0; loop < 27 ; loop+=3)
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
palette_select_callback (int   r,
			    int   g,
			    int   b,
			    ColorSelectState state,
			    void *client_data)
{
  PaletteP palette;
  unsigned char * color;
  GSList *list;
  gint pos = 0;
  PaletteEntriesP p_entries = NULL;
  
  palette = client_data;

  if (palette && palette->entries)
    {
      switch (state) {
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
	  palette_draw_small_preview(palette->gc,palette->entries);

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

	  gtk_clist_set_text(GTK_CLIST(palette->clist),pos,2,p_entries->name);
	  palette_select_set_text_all(palette->entries);
	  }
	/* Fallthrough */
      case COLOR_SELECT_CANCEL:
        color_select_hide (palette->color_select);
	palette->color_select_active = 0;
      }
    }
}

static void 
palette_scroll_clist_to_current(PaletteP palette)
{
  GSList *list;
  gint pos = 0;
  PaletteEntriesP p_entries;

  if(palette && palette->entries)
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

      gtk_clist_unselect_all(GTK_CLIST(palette->clist));
      gtk_clist_select_row(GTK_CLIST(palette->clist),pos,-1);
      gtk_clist_moveto(GTK_CLIST(palette->clist),pos,0,0.0,0.0); 
    }
}

static void
palette_delete_entries_callback (GtkWidget *w,
				    gpointer   client_data)
{
  PaletteP palette;
  PaletteEntriesP entries;

  palette = client_data;
  if (palette && palette->entries)
    {
      entries = palette->entries;
      if (entries && entries->filename)
	palette_entries_delete (entries->filename);

      palette_entries_list = g_slist_remove(palette_entries_list,entries);

      palette_refresh(palette);
    }
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

      if(import_dialog)
	{
	  gtk_widget_destroy(import_dialog->dialog);
	  g_free(import_dialog);
	  import_dialog = NULL;
	}
      
      if (GTK_WIDGET_VISIBLE (palette->shell))
	gtk_widget_hide (palette->shell);
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
color_name_entry_changed (GtkWidget *widget, gpointer pdata)
{
  PaletteP palette;

  palette = pdata;
  if(palette->color->name)
    g_free(palette->color->name);
  palette->entries->changed = 1;
  palette->color->name =  g_strdup(gtk_entry_get_text(GTK_ENTRY (palette->color_name)));
}


static void
palette_edit_callback (GtkWidget * widget,
			  gpointer   client_data)
{
  PaletteP palette;
  unsigned char *color;

  palette = client_data;
  if (palette && palette->entries && palette->color)
    {
      color = palette->color->color;

      if (!palette->color_select)
	{
	  palette->color_select = color_select_new (color[0], color[1], color[2],
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

	  color_select_set_color (palette->color_select, color[0], color[1], color[2], 1);
	}
    }
}


static void
palette_popup_menu(PaletteP   palette)
{
  GtkWidget *menu;
  GtkWidget *menu_items;

  menu = gtk_menu_new();
  menu_items = gtk_menu_item_new_with_label(_("Edit"));
  gtk_menu_append(GTK_MENU (menu), menu_items);

  gtk_signal_connect(GTK_OBJECT(menu_items), "activate", 
		     GTK_SIGNAL_FUNC(palette_edit_callback), (gpointer)palette);
  gtk_widget_show(menu_items);

  menu_items = gtk_menu_item_new_with_label(_("New"));
  gtk_menu_append(GTK_MENU (menu), menu_items);
  gtk_signal_connect(GTK_OBJECT(menu_items), "activate", 
		     GTK_SIGNAL_FUNC(palette_new_callback), (gpointer)palette);
  gtk_widget_show(menu_items);

  menu_items = gtk_menu_item_new_with_label(_("Delete"));
  gtk_signal_connect(GTK_OBJECT(menu_items), "activate", 
		     GTK_SIGNAL_FUNC(palette_delete_entry), (gpointer)palette);
  gtk_menu_append(GTK_MENU (menu), menu_items);
  gtk_widget_show(menu_items);

  /* Do something interesting when the menuitem is selected */ 
  /*   gtk_signal_connect_object(GTK_OBJECT(menu_items), "activate", */
  /* 			    GTK_SIGNAL_FUNC(menuitem_response), (gpointer) g_strdup(buf)); */
  
  palette->popup_menu = menu;

  palette->popup_small_menu = menu = gtk_menu_new();
  menu_items = gtk_menu_item_new_with_label(_("New"));
  gtk_menu_append(GTK_MENU (menu), menu_items);
  gtk_signal_connect(GTK_OBJECT(menu_items), "activate", 
		     GTK_SIGNAL_FUNC(palette_new_callback), (gpointer)palette);
  gtk_widget_show(menu_items);
}

static gint
palette_color_area_events (GtkWidget *widget,
			      GdkEvent  *event,
			      PaletteP   palette)
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
 		  palette_draw_entries(palette,-1,-1);
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

	      palette_draw_entries(palette,row,col);
	      /*  Update the active color name  */
	      gtk_entry_set_text (GTK_ENTRY (palette->color_name), palette->color->name);
	     /*  palette_update_current_entry (palette); */
	      if(bevent->button == 3)
		{
		  /* Popup the edit menu */
		  gtk_menu_popup (GTK_MENU (palette->popup_menu), NULL, NULL, 
				  NULL, NULL, 3, ((GdkEventButton *)event)->time);
		}
	    }
	  else
	    {
	      if(bevent->button == 3)
		{
		  /* Popup the small new menu */
		  gtk_menu_popup (GTK_MENU (palette->popup_small_menu), NULL, NULL, 
				  NULL, NULL, 3, ((GdkEventButton *)event)->time);
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
palette_insert_clist(GtkWidget *clist, 
		     GtkWidget *shell,
		     GdkGC *gc,
		     PaletteEntriesP p_entries,
		     gint pos)
{
  gchar *string[3];
	
  string[0] = NULL;
  string[1] = g_strdup_printf("%d",p_entries->n_colors);
  string[2] = p_entries->name;
  
  gtk_clist_insert(GTK_CLIST(clist),pos,string);
  
  g_free((void *)string[1]);
  
  if(p_entries->pixmap == NULL)
    p_entries->pixmap = gdk_pixmap_new(shell->window,
				       SM_PREVIEW_WIDTH, 
				       SM_PREVIEW_HEIGHT, 
				       gtk_widget_get_visual(shell)->depth);
  
  palette_draw_small_preview(gc,p_entries);
  gtk_clist_set_pixmap(GTK_CLIST(clist), pos, 0, p_entries->pixmap, NULL);
  gtk_clist_set_row_data(GTK_CLIST(clist),pos,(gpointer)p_entries);
}  

void
palette_clist_init(GtkWidget *clist, 
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
      
      palette_insert_clist(clist,shell,gc,p_entries,pos);
      pos++;
    }
}

static int
palette_draw_color_row (unsigned char **colors,
			   gint             ncolors,
			   gint             y,
			   gint             column_highlight,
			   unsigned char  *buffer,
			   PaletteP      palette)
{
  unsigned char *p;
  unsigned char bcolor;
  int width, height;
  int entry_width;
  int entry_height;
  int vsize;
  int vspacing;
  int i, j;
  GtkWidget      *preview;

  if(!palette)
    return -1;

  preview = palette->color_area;

  bcolor = 0;

  width = preview->requisition.width;
  height = preview->requisition.height;
  entry_width = (ENTRY_WIDTH*palette->zoom_factor);
  entry_height = (ENTRY_HEIGHT*palette->zoom_factor);

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
	  
	  if(column_highlight >= 0)
	    {
	      guchar *ph = &buffer[3*column_highlight*(entry_width+SPACING)];
	      for(j = 0 ; j <= entry_width + SPACING; j++)
		{
		  *ph++ = ~bcolor;
		  *ph++ = ~bcolor;
		  *ph++ = ~bcolor;
		}
 	      gtk_preview_draw_row (GTK_PREVIEW (preview), buffer, 0, y+entry_height+1, width); 
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
	  if(ncolors == column_highlight)
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
	  if(column_highlight >= 0)
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
palette_draw_entries (PaletteP palette,gint row_start, gint column_highlight)
{
  PaletteEntryP entry;
  unsigned char *buffer;
  unsigned char **colors;
  GSList *tmp_link;
  int width, height;
  int entry_width;
  int entry_height;
  int index, y;

  if (palette && palette->entries)
    {
      width = palette->color_area->requisition.width;
      height = palette->color_area->requisition.height;

      entry_width = (ENTRY_WIDTH*palette->zoom_factor);
      entry_height = (ENTRY_HEIGHT*palette->zoom_factor);

      colors = g_malloc (sizeof(unsigned char *) * palette->columns * 2);
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
palette_scroll_top_left(PaletteP palette)
{
  GtkAdjustment *hadj; 
  GtkAdjustment *vadj; 

  /* scroll viewport to top left */
  if(palette && palette->scrolled_window)
    {
      hadj = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(palette->scrolled_window));
      vadj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(palette->scrolled_window));
      
      if(hadj)
	gtk_adjustment_set_value(hadj,0.0);
      if(vadj)
	gtk_adjustment_set_value(vadj,0.0);
    }
}

static void
redraw_palette(PaletteP palette)
{
  gint vsize;
  gint nrows;
  gint n_entries;
  GtkWidget *parent;
  gint new_pre_width;

  n_entries = palette->entries->n_colors;
  nrows = n_entries / palette->columns;
  if (n_entries % palette->columns)
    nrows += 1;

  vsize = nrows*(SPACING + (gint)(ENTRY_HEIGHT*palette->zoom_factor))+1;

  if(vsize < PREVIEW_HEIGHT)
    vsize = PREVIEW_HEIGHT;

  parent = palette->color_area->parent;
  gtk_widget_ref(palette->color_area);
  gtk_container_remove(GTK_CONTAINER(parent),palette->color_area);

  new_pre_width = (gint)(ENTRY_WIDTH*palette->zoom_factor);
  new_pre_width = (new_pre_width+SPACING)*palette->columns+SPACING;
  
  gtk_preview_size(GTK_PREVIEW(palette->color_area),
		  new_pre_width, /*PREVIEW_WIDTH,*/
		   vsize); 

  gtk_container_add(GTK_CONTAINER(parent),palette->color_area);
  gtk_widget_unref(palette->color_area);


  palette_draw_entries(palette,-1,-1);
}

static void
palette_list_item_update(GtkWidget *widget, 
		     gint row,
		     gint column,
		     GdkEventButton *event,
		     gpointer data)
{
  PaletteP palette;
  PaletteEntriesP p_entries;

  palette = (PaletteP)data;

  if (palette->color_select_active)
    {
      palette->color_select_active = 0;
      color_select_hide (palette->color_select);
    }

  if(palette->color_select)
    color_select_free(palette->color_select);
  palette->color_select = NULL;

  p_entries = 
    (PaletteEntriesP)gtk_clist_get_row_data(GTK_CLIST(palette->clist),row);

  palette->entries = p_entries;

  redraw_palette(palette);

  palette_scroll_top_left(palette);

  /* Stop errors in case no colours are selected */ 
  gtk_signal_handler_block(GTK_OBJECT (palette->color_name),palette->entry_sig_id);
  gtk_entry_set_text (GTK_ENTRY (palette->color_name), _("Undefined")); 
  gtk_signal_handler_unblock(GTK_OBJECT (palette->color_name),palette->entry_sig_id);
}


PaletteP
create_palette_dialog ()
{
  GtkWidget *palette_dialog;
  GtkWidget *dialog_vbox3;
  GtkWidget *hbox3;
  GtkWidget *hbox4;
  GtkWidget *vbox4;
  GtkWidget *palette_scrolledwindow;
  GtkWidget *palette_region;
  GtkWidget *color_name;
  GtkWidget *alignment1;
  GtkWidget *palette_list;
  GtkWidget *frame1;
  GtkWidget *vbuttonbox2;
  GtkWidget *new_palette;
  GtkWidget *delete_palette;
  GtkWidget *save_palettes;
  GtkWidget *import_palette;
  GtkWidget *merge_palette;
  GtkWidget *dialog_action_area3;
  GtkWidget *alignment2;
  GtkWidget *hbuttonbox3;
  GtkWidget *close_button;
  GtkWidget *refresh_button;
  GtkWidget *clist_scrolledwindow;
  GtkWidget *plus_button;
  GtkWidget *minus_button;
  PaletteP palette;
  GdkColormap *colormap;
  
  palette = g_malloc (sizeof (_Palette));

  palette->entries = default_palette_entries;
  palette->color = NULL;
  palette->color_select = NULL;
  palette->color_select_active = 0;
  palette->zoom_factor = 1.0;
  palette->columns = COLUMNS;
  palette->freeze_update = FALSE;

  palette->shell = palette_dialog = gtk_dialog_new ();

  gtk_window_set_wmclass (GTK_WINDOW (palette->shell), "color_palette", "Gimp");
  gtk_widget_set_usize (palette_dialog, 615, 200);
  gtk_window_set_title (GTK_WINDOW (palette_dialog), _("Color Palette"));
  gtk_window_set_policy (GTK_WINDOW (palette_dialog), TRUE, TRUE, FALSE);

  dialog_vbox3 = GTK_DIALOG (palette_dialog)->vbox;
  gtk_object_set_data (GTK_OBJECT (palette_dialog), "dialog_vbox3", dialog_vbox3);
  gtk_widget_show (dialog_vbox3);

  hbox3 = gtk_hbox_new (FALSE, 0);
  gtk_object_set_data (GTK_OBJECT (palette_dialog), "hbox3", hbox3);
  gtk_widget_show (hbox3);
  gtk_box_pack_start (GTK_BOX (dialog_vbox3), hbox3, TRUE, TRUE, 0);

  vbox4 = gtk_vbox_new (FALSE, 0);
  gtk_object_set_data (GTK_OBJECT (palette_dialog), "vbox4", vbox4);
  gtk_widget_show (vbox4);
  gtk_box_pack_start (GTK_BOX (hbox3), vbox4, TRUE, TRUE, 0);

  alignment1 = gtk_alignment_new (0.5, 0.5, 0.0, 0.0); 
  gtk_object_set_data (GTK_OBJECT (palette_dialog), "alignment1", alignment1);
  gtk_widget_show (alignment1); 

  palette->scrolled_window = 
    palette_scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (palette_scrolledwindow),
				  GTK_POLICY_AUTOMATIC, 
				  GTK_POLICY_AUTOMATIC);

  gtk_object_set_data (GTK_OBJECT (palette_dialog), "palette_scrolledwindow", palette_scrolledwindow);
  gtk_widget_show (palette_scrolledwindow);
  gtk_box_pack_start (GTK_BOX (vbox4), palette_scrolledwindow, TRUE, TRUE, 0);

  palette->color_area = palette_region = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_set_dither (GTK_PREVIEW (palette->color_area),
			  GDK_RGB_DITHER_MAX);
  gtk_preview_size (GTK_PREVIEW (palette_region), PREVIEW_WIDTH, PREVIEW_HEIGHT);
  gtk_widget_set_events (palette_region, PALETTE_EVENT_MASK);
  gtk_signal_connect (GTK_OBJECT (palette->color_area), "event",
		      (GtkSignalFunc) palette_color_area_events,
		      palette);

  gtk_widget_show (palette_region);
  gtk_container_add (GTK_CONTAINER (alignment1), palette_region);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (palette_scrolledwindow), alignment1);

  hbox4 = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox4), hbox4, FALSE, FALSE, 0);
  gtk_widget_show(hbox4);

  palette->color_name = color_name = gtk_entry_new ();
  gtk_widget_show (color_name);
  gtk_box_pack_start (GTK_BOX (hbox4), color_name, TRUE, TRUE, 0);
  gtk_entry_set_text (GTK_ENTRY (color_name), _("Undefined"));
  palette->entry_sig_id = gtk_signal_connect (GTK_OBJECT (color_name), "changed",
					      GTK_SIGNAL_FUNC (color_name_entry_changed),
					      palette);

  /* + and - buttons */
  plus_button = gtk_button_new_with_label ("+");
  minus_button = gtk_button_new_with_label ("-");
  gtk_box_pack_start (GTK_BOX (hbox4), minus_button, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox4), plus_button, TRUE, TRUE, 0);
  gtk_widget_show(plus_button);
  gtk_widget_show(minus_button);
  gtk_signal_connect (GTK_OBJECT (plus_button), "clicked",
		      (GtkSignalFunc) palette_zoomin,
		      (gpointer) palette);
  gtk_signal_connect (GTK_OBJECT (minus_button), "clicked",
		      (GtkSignalFunc) palette_zoomout,
		      (gpointer) palette);
  
  /* clist preview of palettes */
  clist_scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
  palette->clist = palette_list = gtk_clist_new (3);
  gtk_object_set_data (GTK_OBJECT (palette_dialog), "palette_list", palette_list);
  gtk_clist_set_row_height(GTK_CLIST(palette_list),SM_PREVIEW_HEIGHT+2);
  gtk_signal_connect(GTK_OBJECT(palette->clist), "select_row",
		     GTK_SIGNAL_FUNC(palette_list_item_update),
		     (gpointer) palette);
  gtk_widget_show (palette_list);
  gtk_container_add (GTK_CONTAINER (hbox3), clist_scrolledwindow);
  gtk_widget_set_usize (palette_list, 203, -1);
  gtk_clist_set_column_title(GTK_CLIST(palette_list), 0, _("Palette"));
  gtk_clist_set_column_title(GTK_CLIST(palette_list), 1, _("Ncols"));
  gtk_clist_set_column_title(GTK_CLIST(palette_list), 2, _("Name"));
  gtk_clist_column_titles_show(GTK_CLIST(palette_list));
  gtk_container_add (GTK_CONTAINER (clist_scrolledwindow), GTK_WIDGET(palette->clist));

  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (clist_scrolledwindow),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_ALWAYS);

  gtk_clist_set_selection_mode(GTK_CLIST(palette_list),GTK_SELECTION_EXTENDED);

  gtk_widget_show(clist_scrolledwindow);
    
  gtk_container_border_width (GTK_CONTAINER (palette_list), 4);
  gtk_clist_set_column_width (GTK_CLIST (palette_list), 0, SM_PREVIEW_WIDTH+2);
  /* gtk_clist_set_column_width (GTK_CLIST (palette_list), 1, 80); */
  gtk_clist_column_titles_show (GTK_CLIST (palette_list));

  frame1 = gtk_frame_new (_("Palette Ops"));
  gtk_object_set_data (GTK_OBJECT (palette_dialog), "frame1", frame1);
  gtk_widget_show (frame1);
  gtk_box_pack_end (GTK_BOX (hbox3), frame1, FALSE, FALSE, 7);

  vbuttonbox2 = gtk_vbutton_box_new ();
  gtk_object_set_data (GTK_OBJECT (palette_dialog), "vbuttonbox2", vbuttonbox2);
  gtk_widget_show (vbuttonbox2);
  gtk_container_add (GTK_CONTAINER (frame1), vbuttonbox2);
  gtk_container_border_width (GTK_CONTAINER (vbuttonbox2), 6);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (vbuttonbox2), GTK_BUTTONBOX_SPREAD);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (vbuttonbox2), 0);
  gtk_button_box_set_child_size (GTK_BUTTON_BOX (vbuttonbox2), 44, 22);
  gtk_button_box_set_child_ipadding (GTK_BUTTON_BOX (vbuttonbox2), 17, 0);

  new_palette = gtk_button_new_with_label (_("New"));
  gtk_signal_connect (GTK_OBJECT (new_palette), "clicked",
		      (GtkSignalFunc) palette_new_entries_callback,
		      (gpointer) palette);
  gtk_object_set_data (GTK_OBJECT (palette_dialog), "new_palette", new_palette);
  gtk_widget_show (new_palette);
  gtk_container_add (GTK_CONTAINER (vbuttonbox2), new_palette);

  delete_palette = gtk_button_new_with_label (_("Delete"));
  gtk_signal_connect (GTK_OBJECT (delete_palette), "clicked",
		      (GtkSignalFunc) palette_delete_entries_callback,
		      (gpointer) palette);
  gtk_widget_show (delete_palette);
  gtk_container_add (GTK_CONTAINER (vbuttonbox2), delete_palette);

  save_palettes = gtk_button_new_with_label (_("Save"));
  gtk_signal_connect (GTK_OBJECT (save_palettes), "clicked",
		      (GtkSignalFunc) palette_save_palettes_callback,
		      (gpointer) NULL);
  gtk_widget_show (save_palettes);
  gtk_container_add (GTK_CONTAINER (vbuttonbox2), save_palettes);

  import_palette = gtk_button_new_with_label (_("Import"));
  gtk_signal_connect (GTK_OBJECT (import_palette), "clicked",
		      (GtkSignalFunc) palette_import_dialog_callback,
		      (gpointer) palette);
  gtk_widget_show (import_palette);
  gtk_container_add (GTK_CONTAINER (vbuttonbox2), import_palette);

  merge_palette = gtk_button_new_with_label (_("Merge"));
  gtk_signal_connect (GTK_OBJECT (merge_palette), "clicked",
		      (GtkSignalFunc) palette_merge_dialog_callback,
		      (gpointer) palette);
  gtk_widget_show (merge_palette);
  gtk_container_add (GTK_CONTAINER (vbuttonbox2), merge_palette);

  dialog_action_area3 = GTK_DIALOG (palette_dialog)->action_area;
  gtk_object_set_data (GTK_OBJECT (palette_dialog), "dialog_action_area3", dialog_action_area3);
  gtk_widget_show (dialog_action_area3);
  gtk_container_border_width (GTK_CONTAINER (dialog_action_area3), 2);

  alignment2 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_object_set_data (GTK_OBJECT (palette_dialog), "alignment2", alignment2);
  gtk_widget_show (alignment2);
  gtk_box_pack_start (GTK_BOX (dialog_action_area3), alignment2, TRUE, TRUE, 0);

  hbuttonbox3 = gtk_hbutton_box_new ();
  gtk_object_set_data (GTK_OBJECT (palette_dialog), "hbuttonbox3", hbuttonbox3);
  gtk_widget_show (hbuttonbox3);
  gtk_container_add (GTK_CONTAINER (alignment2), hbuttonbox3);
  gtk_widget_set_usize (hbuttonbox3, 153, 37);
  gtk_container_border_width (GTK_CONTAINER (hbuttonbox3), 8);
  gtk_button_box_set_child_size (GTK_BUTTON_BOX (hbuttonbox3), 85, 26);
  gtk_button_box_set_child_ipadding (GTK_BUTTON_BOX (hbuttonbox3), 4, 0);

  close_button = gtk_button_new_with_label (_("close"));
  gtk_signal_connect(GTK_OBJECT(close_button), "clicked", 
		     GTK_SIGNAL_FUNC(palette_close_callback), (gpointer)palette);

  gtk_widget_show (close_button);
  gtk_container_add (GTK_CONTAINER (hbuttonbox3), close_button);
  gtk_container_border_width (GTK_CONTAINER (close_button), 4);
  GTK_WIDGET_SET_FLAGS (close_button, GTK_CAN_DEFAULT);
  gtk_widget_grab_default (close_button);
  gtk_signal_connect (GTK_OBJECT (palette->shell), "delete_event",
		      GTK_SIGNAL_FUNC (palette_dialog_delete_callback),
		      palette);

  refresh_button = gtk_button_new_with_label (_("refresh"));
  gtk_signal_connect(GTK_OBJECT(refresh_button), "clicked", 
		     GTK_SIGNAL_FUNC(palette_refresh_callback), (gpointer)palette);
  gtk_widget_show (refresh_button);
  gtk_container_add (GTK_CONTAINER (hbuttonbox3), refresh_button);
  gtk_container_border_width (GTK_CONTAINER (refresh_button), 9);

  gtk_widget_realize(palette->shell);
  palette->gc = gdk_gc_new(palette->shell->window);

  colormap = gtk_widget_get_colormap(palette->clist);

  palette_popup_menu(palette);
  
  return palette;
}

static void
import_dialog_select_grad_callback (GtkWidget *w,
				    gpointer   client_data)
{
  /* Popup grad edit box .... */
  grad_create_gradient_editor();
}

static void
import_dialog_close_callback (GtkWidget *w,
			      gpointer   client_data)
{
  gtk_widget_destroy(import_dialog->dialog);
  g_free(import_dialog);
  import_dialog = NULL;
}

static void
import_palette_create_from_grad(gchar *name,PaletteP palette)
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

      entries = palette_create_entries(palette,name);
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
	  palette_add_entry (palette->entries, _("Untitled"), (gint)r, (gint)g, (gint)b);
	}
      palette_update_small_preview(palette);
      redraw_palette(palette);
      /* Update other selectors on screen */
      palette_select_clist_insert_all(entries);
      palette_scroll_clist_to_current(palette);
    }
}

static void
import_dialog_import_callback (GtkWidget *w,
			       gpointer   client_data)
{
  PaletteP palette;
  palette = client_data;
  if(import_dialog)
    {
      guchar *pname;
      pname = gtk_entry_get_text(GTK_ENTRY(import_dialog->entry));
      if(!pname || strlen(pname) == 0)
	pname = g_strdup("tmp");
      else
	pname = g_strdup(pname);
      switch(import_dialog->import_type)
	{
	case GRAD_IMPORT:
	  import_palette_create_from_grad(pname,palette);
	  break;
	case IMAGE_IMPORT:
	  import_palette_create_from_image(import_dialog->gimage,pname,palette);
	  break;
	default:
	  break;
	}
      import_dialog_close_callback(NULL,NULL);
    }
}

static gint
import_dialog_delete_callback (GtkWidget *w,
				   GdkEvent *e,
				   gpointer client_data) 
{
  import_dialog_close_callback(w,client_data);
  return TRUE;
}


static void
palette_merge_entries_callback (GtkWidget *w,
				   gpointer   client_data,
				   gpointer   call_data)
{
  PaletteP palette;
  PaletteEntriesP p_entries;
  PaletteEntriesP new_entries;
  GList *sel_list;

  new_entries = palette_create_entries(client_data,call_data);

  palette = (PaletteP)client_data;

  sel_list = GTK_CLIST(palette->clist)->selection;

  if(sel_list)
    {
      while (sel_list)
	{
	  gint row;
	  GSList *cols;

	  row = GPOINTER_TO_INT (sel_list->data);
	  p_entries = 
	    (PaletteEntriesP)gtk_clist_get_row_data(GTK_CLIST(palette->clist),row);

	  /* Go through each palette and merge the colours */
	  cols = p_entries->colors;
	  while(cols)
	    {
	      PaletteEntryP entry = cols->data;
	      palette_add_entry (new_entries,
				 g_strdup(entry->name),
				 entry->color[0],
				 entry->color[1],
				 entry->color[2]);
	      cols = cols->next;
	    }
	  sel_list = sel_list->next;
	}
    }

  palette_update_small_preview(palette);
  redraw_palette(palette);
  gtk_clist_unselect_all(GTK_CLIST(palette->clist));
  /* Update other selectors on screen */
  palette_select_clist_insert_all(new_entries);
  palette_scroll_clist_to_current(palette);
}

static void
palette_merge_dialog_callback (GtkWidget *w,
				gpointer client_data)
{
  query_string_box (_("Merge Palette"), _("Enter a name for merged palette"), NULL,
		    palette_merge_entries_callback, client_data);
}

static void
palette_import_dialog_callback (GtkWidget *w,
				gpointer client_data)
{
  if(!import_dialog)
    {
      import_dialog = palette_import_dialog((PaletteP)client_data);
      gtk_widget_show(import_dialog->dialog);
    }
  else
    {
      gdk_window_raise(import_dialog->dialog->window);
    }
}

static void
palette_import_fill_grad_preview(GtkWidget *preview,gradient_t *gradient)
{
  guchar buffer[3*IMPORT_PREVIEW_WIDTH];
  gint loop;
  guchar *p = buffer;
  double  dx, cur_x;
  double  r, g, b, a;
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


void import_palette_grad_update(gradient_t *grad)
{
  if(import_dialog && import_dialog->import_type == GRAD_IMPORT)
    {
      /* redraw gradient */
      palette_import_fill_grad_preview(import_dialog->preview,grad);
      gtk_entry_set_text (GTK_ENTRY (import_dialog->entry), grad->name);
    }
}

static void
import_grad_callback(GtkWidget *widget, gpointer data)
{
  if(import_dialog)
    {
      import_dialog->import_type = GRAD_IMPORT;
      if(import_dialog->image_list)
	{
	  gtk_widget_hide(import_dialog->image_list);
	  gtk_widget_destroy(import_dialog->image_list);
	  import_dialog->image_list = NULL;
	}
      gtk_widget_show(import_dialog->select);
      palette_import_fill_grad_preview(import_dialog->preview,curr_gradient);

      gtk_entry_set_text (GTK_ENTRY (import_dialog->entry), curr_gradient->name);
      gtk_widget_set_sensitive(import_dialog->threshold_scale,FALSE);
      gtk_widget_set_sensitive(import_dialog->threshold_text,FALSE);
    }
}

static void
gimlist_cb(gpointer im, gpointer data){
	GSList** l=(GSList**)data;
	*l=g_slist_prepend(*l, im);
}

static void
import_image_update_image_preview(GimpImage *gimage)
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

  preview_buf = gimp_image_construct_composite_preview(gimage,
						       MAX(pwidth, 2),
						       MAX(pheight, 2));
  
  gtk_preview_size(GTK_PREVIEW(import_dialog->preview),
		  preview_buf->width,
		  preview_buf->height);

  buf = g_new (gchar,  IMPORT_PREVIEW_WIDTH * 3);
  src = (gchar *)temp_buf_data (preview_buf);
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
      gtk_preview_draw_row (GTK_PREVIEW (import_dialog->preview), (guchar *)buf, 0, y, preview_buf->width);
      src += preview_buf->width * preview_buf->bytes;
    }

  g_free(buf);
  temp_buf_free(preview_buf);

  gtk_widget_hide(import_dialog->preview);
  gtk_widget_draw(import_dialog->preview, NULL); 
  gtk_widget_show(import_dialog->preview);
}

static void
import_image_sel_callback(GtkWidget *widget, gpointer data)
{
  GimpImage *gimage;
  gchar *lab;

  gimage = GIMP_IMAGE(data);
  import_image_update_image_preview(gimage);

  lab = g_strdup_printf("%s-%d",
		       prune_filename(gimage_filename(import_dialog->gimage)),
		       pdb_image_to_id (import_dialog->gimage));

  gtk_entry_set_text (GTK_ENTRY (import_dialog->entry),lab);
}

static void
import_image_menu_add(GimpImage *gimage)
{
  GtkWidget *menuitem;
  gchar *lab = g_strdup_printf("%s-%d",
			       prune_filename(gimage_filename(gimage)),
			       pdb_image_to_id (gimage));
  menuitem = gtk_menu_item_new_with_label(lab);
  gtk_widget_show (menuitem);
  gtk_signal_connect(GTK_OBJECT(menuitem), "activate",
		     (GtkSignalFunc) import_image_sel_callback,
		     gimage);
  gtk_menu_append (GTK_MENU (import_dialog->optionmenu1_menu), menuitem);
}

/* Last Param gives us control over what goes in the menu on a delete oper */
static void
import_image_menu_activate(gint redo,GimpImage * del_image)
{
  GSList *list=NULL;
  gint num_images;
  GimpImage *last_img = NULL;
  GimpImage *first_img = NULL;
  gint act_num = -1;
  gint count = 0;
  gchar *lab;

  if(import_dialog)
    {
      if(import_dialog->import_type == IMAGE_IMPORT)
	{
	  if(!redo)
	    return;
	  else
	    {
	      if(import_dialog->image_list)
		{
		  last_img = import_dialog->gimage;
		  gtk_widget_hide(import_dialog->image_list);
		  gtk_widget_destroy(import_dialog->image_list);
		  import_dialog->image_list = NULL;
		}
	    }
	}
      import_dialog->import_type = IMAGE_IMPORT;

      /* Get list of images */
      gimage_foreach(gimlist_cb, &list);
      num_images = g_slist_length (list);
      
      if (num_images)
	{
	  int i;
	  GtkWidget *optionmenu1;
	  GtkWidget *optionmenu1_menu;

	  import_dialog->image_list = optionmenu1 = gtk_option_menu_new ();
	  gtk_widget_set_usize (optionmenu1,IMPORT_PREVIEW_WIDTH,-1);
	  import_dialog->optionmenu1_menu = optionmenu1_menu = gtk_menu_new ();

	  for (i = 0; i < num_images; i++, list = g_slist_next (list))
	    {
	      if(GIMP_IMAGE(list->data) != del_image)
		{
		  if(first_img == NULL)
		    first_img = GIMP_IMAGE(list->data);
		  import_image_menu_add(GIMP_IMAGE(list->data));
		  if(last_img == GIMP_IMAGE(list->data))
		    act_num = count;
		  else
		    count++;
		}
	    }

	  gtk_option_menu_set_menu (GTK_OPTION_MENU (optionmenu1), optionmenu1_menu);
	  gtk_widget_hide(import_dialog->select);
	  gtk_container_add(GTK_CONTAINER(import_dialog->select_area),optionmenu1);

	  if(last_img != NULL && last_img != del_image)
	      import_image_update_image_preview(last_img);
	  else if(first_img != NULL)
	      import_image_update_image_preview(first_img);

	  gtk_widget_show (optionmenu1);

	  /* reset to last one */
	  if(redo && act_num >= 0)
	    {
	      gchar *lab = g_strdup_printf("%s-%d",
			     prune_filename(gimage_filename(import_dialog->gimage)),
			     pdb_image_to_id (import_dialog->gimage));

	      gtk_option_menu_set_history(GTK_OPTION_MENU(optionmenu1),act_num);
	      gtk_entry_set_text (GTK_ENTRY (import_dialog->entry),lab);
	    }
	}
      g_slist_free(list);

      lab = g_strdup_printf("%s-%d",
			    prune_filename(gimage_filename(import_dialog->gimage)),
			    pdb_image_to_id (import_dialog->gimage));

      gtk_entry_set_text (GTK_ENTRY (import_dialog->entry),lab);
    }
}

static void
import_image_callback(GtkWidget *widget, gpointer data)
{
  import_image_menu_activate(FALSE,NULL);
  gtk_widget_set_sensitive(import_dialog->threshold_scale,TRUE);
  gtk_widget_set_sensitive(import_dialog->threshold_text,TRUE);
}

static gint
image_count()
{
  GSList *list=NULL;
  gint num_images = 0;

  gimage_foreach(gimlist_cb, &list);
  num_images = g_slist_length (list);

  g_slist_free(list);

  return (num_images);
}

static ImportDialogP
palette_import_dialog(PaletteP palette)
{
  GtkWidget *dialog1;
  GtkWidget *dialog_vbox1;
  GtkWidget *hbox2;
  GtkWidget *import_frame;
  GtkWidget *vbox2;
  GtkWidget *table1;
  GtkWidget *steps;
  GtkWidget *import_name;
  GtkWidget *import_type;
  GtkWidget *spinbutton2;
  GtkWidget *entry1;
  GtkWidget *optionmenu1;
  GtkWidget *optionmenu1_menu;
  GtkWidget *menuitem;
  GtkWidget *preview_frame;
  GtkWidget *vbox1;
  GtkWidget *image1;
  GtkWidget *select;
  GtkWidget *dialog_action_area1;
  GtkWidget *hbox1;
  GtkWidget *hscale1;
  GtkWidget *import;
  GtkWidget *close;

  import_dialog = g_malloc(sizeof(struct _ImportDialog));
  import_dialog->image_list = NULL;
  import_dialog->gimage = NULL;

  import_dialog->dialog = dialog1 = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dialog1), _("Import Palette"));
  gtk_window_set_policy (GTK_WINDOW (dialog1), TRUE, TRUE, FALSE);

  dialog_vbox1 = GTK_DIALOG (dialog1)->vbox;
  gtk_widget_show (dialog_vbox1);

  hbox2 = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox2);
  gtk_box_pack_start (GTK_BOX (dialog_vbox1), hbox2, FALSE, FALSE, 0);

  import_frame = gtk_frame_new (_("Import"));
  gtk_widget_show (import_frame);
  gtk_box_pack_start (GTK_BOX (hbox2), import_frame, TRUE, TRUE, 0);

  vbox2 = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox2);
  gtk_container_add (GTK_CONTAINER (import_frame), vbox2);

  table1 = gtk_table_new (4, 2, FALSE);
  gtk_widget_show (table1);
  gtk_box_pack_start (GTK_BOX (vbox2), table1, TRUE, TRUE, 0);

  steps = gtk_label_new (_("Sample Size:"));
  gtk_widget_show (steps);
  gtk_table_attach (GTK_TABLE (table1), steps, 0, 1, 2, 3,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND, 0, 0);
  gtk_label_set_justify (GTK_LABEL (steps), GTK_JUSTIFY_LEFT);

  import_dialog->threshold_text =
    steps = gtk_label_new (_("Interval:"));
  gtk_widget_show (steps);
  gtk_table_attach (GTK_TABLE (table1), steps, 0, 1, 3, 4,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND, 0, 0);
  gtk_label_set_justify (GTK_LABEL (steps), GTK_JUSTIFY_LEFT);
  gtk_widget_set_sensitive(steps,FALSE);

  import_name = gtk_label_new (_("Name:"));
  gtk_widget_show (import_name);
  gtk_table_attach (GTK_TABLE (table1), import_name, 0, 1, 0, 1,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND, 0, 0);
  gtk_label_set_justify (GTK_LABEL (import_name), GTK_JUSTIFY_LEFT);

  import_type = gtk_label_new (_("Source:"));
  gtk_widget_show (import_type);
  gtk_table_attach (GTK_TABLE (table1), import_type, 0, 1, 1, 2,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND, 0, 0);
  gtk_label_set_justify (GTK_LABEL (import_type), GTK_JUSTIFY_LEFT);

  import_dialog->sample = GTK_ADJUSTMENT(gtk_adjustment_new (256, 2, 10000, 1, 10, 10));
  spinbutton2 = gtk_spin_button_new (import_dialog->sample, 1, 0);
  gtk_widget_show (spinbutton2);
  gtk_table_attach (GTK_TABLE (table1), spinbutton2, 1, 2, 2, 3,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND, 0, 0);

  import_dialog->threshold_scale = 
    hscale1 = 
    gtk_hscale_new (import_dialog->threshold = 
		    GTK_ADJUSTMENT (gtk_adjustment_new (1, 1, 128, 1, 1, 1)));
  gtk_scale_set_value_pos (GTK_SCALE (hscale1), GTK_POS_TOP);
  gtk_scale_set_digits (GTK_SCALE (hscale1), 0);
  gtk_widget_show (hscale1);
  gtk_table_attach (GTK_TABLE (table1), hscale1, 1, 2, 3, 4,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND, 0, 0);
  gtk_widget_set_sensitive(hscale1,FALSE);

  import_dialog->entry = entry1 = gtk_entry_new ();
  gtk_widget_show (entry1);
  gtk_table_attach (GTK_TABLE (table1), entry1, 1, 2, 0, 1,
                    GTK_FILL, 0, 0, 0);
/*   gtk_widget_set_usize (entry1, 100, -1); */
  gtk_entry_set_text (GTK_ENTRY (entry1), (curr_gradient)?curr_gradient->name:_("new_import"));

  import_dialog->type_option = optionmenu1 = gtk_option_menu_new ();
  gtk_widget_show (optionmenu1);
  gtk_table_attach (GTK_TABLE (table1), optionmenu1, 1, 2, 1, 2,
                    GTK_FILL, 0, 0, 0);
  optionmenu1_menu = gtk_menu_new ();
  import_dialog->image_menu_item_gradient = 
    menuitem = gtk_menu_item_new_with_label (_("Gradient"));
  gtk_widget_show (menuitem);
  gtk_signal_connect(GTK_OBJECT(menuitem), "activate",
		     (GtkSignalFunc) import_grad_callback,
		     NULL);
  gtk_menu_append (GTK_MENU (optionmenu1_menu), menuitem);
  import_dialog->image_menu_item_image = menuitem = gtk_menu_item_new_with_label ("Image");
  gtk_widget_show (menuitem);
  gtk_signal_connect(GTK_OBJECT(menuitem), "activate",
		     (GtkSignalFunc) import_image_callback,
		     (gpointer)import_dialog);
  gtk_menu_append (GTK_MENU (optionmenu1_menu), menuitem);

  gtk_option_menu_set_menu (GTK_OPTION_MENU (optionmenu1), optionmenu1_menu);
  gtk_widget_set_sensitive(menuitem,image_count() > 0);

  preview_frame = gtk_frame_new (_("Preview"));
  gtk_widget_show (preview_frame);
  gtk_box_pack_start (GTK_BOX (hbox2), preview_frame, TRUE, TRUE, 0);

  import_dialog->select_area = vbox1 = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox1);
  gtk_container_add (GTK_CONTAINER (preview_frame), vbox1);

  import_dialog->preview = image1 = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_set_dither (GTK_PREVIEW (image1),
			  GDK_RGB_DITHER_MAX);
  gtk_preview_size (GTK_PREVIEW (image1), IMPORT_PREVIEW_WIDTH, IMPORT_PREVIEW_HEIGHT);
  gtk_widget_show (image1);
  gtk_widget_set_usize (image1,IMPORT_PREVIEW_WIDTH, IMPORT_PREVIEW_HEIGHT);
  gtk_box_pack_start (GTK_BOX (vbox1), image1, FALSE, FALSE, 0);

  import_dialog->select = select = gtk_button_new_with_label (_("select"));
  gtk_signal_connect(GTK_OBJECT(select), "clicked", 
		     GTK_SIGNAL_FUNC(import_dialog_select_grad_callback),(gpointer)image1);
  gtk_widget_show (select);
  gtk_box_pack_start (GTK_BOX (vbox1), select, FALSE, FALSE, 0);

  dialog_action_area1 = GTK_DIALOG (dialog1)->action_area;
  gtk_container_border_width (GTK_CONTAINER (dialog_action_area1), 2);
  gtk_widget_show (dialog_action_area1);

  hbox1 = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox1);
  gtk_box_pack_start (GTK_BOX (dialog_action_area1), hbox1, TRUE, TRUE, 0);

  import = gtk_button_new_with_label (_("import"));
  gtk_widget_show (import);
  gtk_signal_connect(GTK_OBJECT(import), "clicked", 
		     GTK_SIGNAL_FUNC(import_dialog_import_callback),(gpointer)palette);
  gtk_container_border_width (GTK_CONTAINER (import), 4);
  gtk_box_pack_end (GTK_BOX (hbox1), import, FALSE, FALSE, 0); 

  close = gtk_button_new_with_label (_("close"));
  gtk_signal_connect(GTK_OBJECT(close), "clicked", 
		     GTK_SIGNAL_FUNC(import_dialog_close_callback),(gpointer)NULL);
  gtk_widget_show (close);
  gtk_box_pack_start (GTK_BOX (hbox1), close, FALSE, FALSE, 0);
  GTK_WIDGET_SET_FLAGS (close, GTK_CAN_DEFAULT);
  gtk_widget_grab_default (close);

  gtk_signal_connect (GTK_OBJECT (dialog1), "delete_event",
		      GTK_SIGNAL_FUNC (import_dialog_delete_callback),
		      (gpointer)palette);

  /* Fill with the selected gradient */
  palette_import_fill_grad_preview(image1,curr_gradient);
  import_dialog->import_type = GRAD_IMPORT;
  return import_dialog;
}


/* Stuff to keep dialog uptodate */

void
palette_import_image_new(GimpImage * gimage)
{
  if(!import_dialog)
    return;

  if(!GTK_WIDGET_IS_SENSITIVE(import_dialog->image_menu_item_image))
    {
      gtk_widget_set_sensitive(import_dialog->image_menu_item_image,TRUE);
      return;
    }

  /* Now fill in the names if image menu shown */
  if(import_dialog->import_type == IMAGE_IMPORT)
    {
      import_image_menu_activate(TRUE,NULL);
    }
}

void
palette_import_image_destroyed(GimpImage* gimage)
{
  /* Now fill in the names if image menu shown */

  if(!import_dialog)
    return;

  if(image_count() <= 1)
    {
      /* Back to gradient type */
      gtk_option_menu_set_history(GTK_OPTION_MENU(import_dialog->type_option),0);
      import_grad_callback(NULL,NULL);
      if(import_dialog->image_menu_item_image)
	gtk_widget_set_sensitive(import_dialog->image_menu_item_image,FALSE);
      return;
    }

  if(import_dialog->import_type == IMAGE_IMPORT)
    {
      import_image_menu_activate(TRUE,gimage);
    }
}

void
palette_import_image_renamed(GimpImage* gimage)
{
  /* Now fill in the names if image menu shown */
  if(import_dialog && import_dialog->import_type == IMAGE_IMPORT)
    {
      import_image_menu_activate(TRUE,NULL);
    }
}

struct _img_colours {
  guint count;
  guint r_adj;
  guint g_adj;
  guint b_adj;
  guchar r;
  guchar g;
  guchar b;
};

typedef struct  _img_colours ImgColours, *ImgColoursP;
static int count_colour_entries = 0;

static void
create_storted_list(gpointer key,gpointer value,gpointer user_data)
{
  GSList **sorted_list = (GSList**)user_data;
  ImgColoursP colour_tab = (ImgColoursP)value;

  *sorted_list = g_slist_prepend(*sorted_list,colour_tab);
}

static void 
create_image_palette(gpointer data,gpointer user_data)
{
  PaletteP palette = (PaletteP)user_data;
  ImgColoursP colour_tab = (ImgColoursP)data;
  gint sample_sz;
  gchar *lab;

  sample_sz = (gint)import_dialog->sample->value;  

  if(palette->entries->n_colors >= sample_sz)
    return;

  lab = g_strdup_printf("%s (occurs %u)",_("Untitled"),colour_tab->count);

  /* Adjust the colours to the mean of the the sample */
  palette_add_entry (palette->entries, lab, 
		     (gint)colour_tab->r + (colour_tab->r_adj/colour_tab->count), 
		     (gint)colour_tab->g + (colour_tab->g_adj/colour_tab->count), 
		     (gint)colour_tab->b + (colour_tab->b_adj/colour_tab->count));
}

static gboolean
colour_print_remove(gpointer key,gpointer value,gpointer user_data)
{
  g_free(value);
  return TRUE;
}

static gint 
sort_colours (gconstpointer a,gconstpointer  b)
{
  ImgColoursP s1 = (ImgColoursP) a;
  ImgColoursP s2 = (ImgColoursP) b;

  if(s1->count > s2->count)
    return -1;
  if(s1->count < s2->count)
    return 1;
  return 0;
}

static void
import_image_make_palette(GHashTable *h_array,guchar *name, PaletteP palette)
{
  GSList * sorted_list = NULL;
  PaletteEntriesP entries;

  g_hash_table_foreach(h_array,create_storted_list,&sorted_list);
  sorted_list = g_slist_sort(sorted_list,sort_colours);

  entries = palette_create_entries(palette,name);
  g_slist_foreach(sorted_list,create_image_palette,palette);

  /* Free up used memory */
  /* Note the same structure is on both the hash list and the sorted
   * list. So only delete it once.
   */
  g_hash_table_freeze(h_array);
  g_hash_table_foreach_remove(h_array,colour_print_remove,NULL);
  g_hash_table_thaw(h_array);
  g_hash_table_destroy(h_array);
  g_slist_free(sorted_list);

  /* Redraw with new palette */
  palette_update_small_preview(palette);
  redraw_palette(palette);
  /* Update other selectors on screen */
  palette_select_clist_insert_all(entries);
  palette_scroll_clist_to_current(palette);
}

static GHashTable *
store_colours(GHashTable *h_array, 
	      guchar * colours,
	      guchar * colours_real, 
	      gint sample_sz)
{
  gpointer found_colour = NULL;
  ImgColoursP new_colour;
  guint key_colours = colours[0]*256*256+colours[1]*256+colours[2];

  if(h_array == NULL)
    {
      h_array = g_hash_table_new(g_direct_hash,g_direct_equal);
      count_colour_entries = 0;
    }
  else
    {
      found_colour = g_hash_table_lookup(h_array,(gpointer)key_colours);
    }

  if(found_colour == NULL)
    {
      if(count_colour_entries > MAX_IMAGE_COLOURS)
	{
	  /* Don't add any more new ones */
	  return h_array;
	}

      count_colour_entries++;

      new_colour = g_new(ImgColours,1);

      new_colour->count = 1;
      new_colour->r_adj = 0;
      new_colour->g_adj = 0;
      new_colour->b_adj = 0;
      new_colour->r = colours[0];
      new_colour->g = colours[1];
      new_colour->b = colours[2];

      g_hash_table_insert(h_array,(gpointer)key_colours,new_colour);
    }
  else
    {
      new_colour = (ImgColoursP)found_colour;
      if(new_colour->count < (G_MAXINT - 1))
	new_colour->count++;
      
      /* Now do the adjustments ...*/
      new_colour->r_adj += (colours_real[0] - colours[0]);
      new_colour->g_adj += (colours_real[1] - colours[1]);
      new_colour->b_adj += (colours_real[2] - colours[2]);

      /* Boundary conditions */
      if(new_colour->r_adj > (G_MAXINT - 255))
	new_colour->r_adj /= new_colour->count;

      if(new_colour->g_adj > (G_MAXINT - 255))
	new_colour->g_adj /= new_colour->count;

      if(new_colour->b_adj > (G_MAXINT - 255))
	new_colour->b_adj /= new_colour->count;

    }

  return h_array;
}

static void
import_palette_create_from_image (GImage *gimage,guchar *pname,PaletteP palette)
{
  PixelRegion imagePR;
  unsigned char *image_data;
  unsigned char *idata;
  guchar rgb[MAX_CHANNELS];
  guchar rgb_real[MAX_CHANNELS];
  int has_alpha, indexed;
  int width, height;
  int bytes, alpha;
  int i, j;
  void * pr;
  int d_type;
  GHashTable *store_array = NULL;
  gint sample_sz;
  gint threshold = 1;

  sample_sz = (gint)import_dialog->sample->value;  

  if(gimage == NULL)
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
  pixel_region_init (&imagePR, gimage_composite (gimage), 0, 0, width, height, FALSE);

  alpha = bytes - 1;

  threshold = (gint)import_dialog->threshold->value;

  if(threshold < 1)
    threshold = 1;

  /*  iterate over the entire image  */
  for (pr = pixel_regions_register (1, &imagePR); pr != NULL; pr = pixel_regions_process (pr))
    {
      image_data = imagePR.data;

      for (i = 0; i < imagePR.h; i++)
	{
	  idata = image_data;

	  for (j = 0; j < imagePR.w; j++)
	    {
	      /*  Get the rgb values for the color  */
	      gimage_get_color (gimage, d_type, rgb, idata);
	      memcpy(rgb_real,rgb,MAX_CHANNELS); /* Structure copy */

	      rgb[0] = (rgb[0]/threshold)*threshold;
	      rgb[1] = (rgb[1]/threshold)*threshold;
	      rgb[2] = (rgb[2]/threshold)*threshold;

	      store_array = store_colours(store_array,rgb,rgb_real,sample_sz);

	      idata += bytes;
	    }

	  image_data += imagePR.rowstride;
	}
    }

  /* Make palette from the store_array */
  import_image_make_palette(store_array,pname,palette);
}

