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

#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"

#include "apptypes.h"

#include "gimppalette.h"
#include "gimprc.h"
#include "palette_select.h"

#include "libgimp/gimpenv.h"

#include "libgimp/gimpintl.h"


enum
{
  CHANGED,
  LAST_SIGNAL
};

/*  local function prototypes  */

static void  gimp_palette_class_init       (GimpPaletteClass  *klass);
static void  gimp_palette_init             (GimpPalette       *palette);
static void  gimp_palette_destroy          (GtkObject         *object);

static void  gimp_palette_entry_free       (GimpPaletteEntry  *entry);


/*  private variables  */

static guint gimp_palette_signals[LAST_SIGNAL] = { 0 };

static GimpObjectClass *parent_class = NULL;


GtkType
gimp_palette_get_type (void)
{
  static GtkType palette_type = 0;

  if (! palette_type)
    {
      GtkTypeInfo palette_info =
      {
        "GimpPalette",
        sizeof (GimpPalette),
        sizeof (GimpPaletteClass),
        (GtkClassInitFunc) gimp_palette_class_init,
        (GtkObjectInitFunc) gimp_palette_init,
        /* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL,
      };

      palette_type = gtk_type_unique (GIMP_TYPE_OBJECT, &palette_info);
    }

  return palette_type;
}

static void
gimp_palette_class_init (GimpPaletteClass *klass)
{
  GtkObjectClass  *object_class;
  GimpObjectClass *gimp_object_class;

  object_class      = (GtkObjectClass *) klass;
  gimp_object_class = (GimpObjectClass *) klass;

  parent_class = gtk_type_class (GIMP_TYPE_OBJECT);

  gimp_palette_signals[CHANGED] =
    gtk_signal_new ("changed",
                    GTK_RUN_FIRST,
                    object_class->type,
                    GTK_SIGNAL_OFFSET (GimpPaletteClass,
                                       changed),
                    gtk_signal_default_marshaller,
                    GTK_TYPE_NONE, 0);

  gtk_object_class_add_signals (object_class, gimp_palette_signals,
                                LAST_SIGNAL);

  object_class->destroy = gimp_palette_destroy;

  klass->changed = NULL;
}

static void
gimp_palette_init (GimpPalette *palette)
{
  palette->filename  = NULL;

  palette->colors    = NULL;
  palette->n_colors  = 0;

  palette->changed   = TRUE;

  palette->pixmap    = NULL;
}

static void
gimp_palette_destroy (GtkObject *object)
{
  GimpPalette      *palette;
  GimpPaletteEntry *entry;
  GList            *list;

  g_return_if_fail (GIMP_IS_PALETTE (object));

  palette = GIMP_PALETTE (object);

  for (list = palette->colors; list; list = g_list_next (list))
    {
      entry = (GimpPaletteEntry *) list->data;

      gimp_palette_entry_free (entry);
    }

  g_list_free (palette->colors);

  g_free (palette->filename);

  if (palette->pixmap)
    gdk_pixmap_unref (palette->pixmap);

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

GimpPalette *
gimp_palette_new (const gchar *name)
{
  GimpPalette *palette = NULL;
  GList       *pal_path;
  gchar       *pal_dir;

  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (*name != '\0', NULL);

  if (!name || !palette_path)
    return NULL;

  pal_path = gimp_path_parse (palette_path, 16, TRUE, NULL);
  pal_dir  = gimp_path_get_user_writable_dir (pal_path);
  gimp_path_free (pal_path);

  palette = GIMP_PALETTE (gtk_object_new (GIMP_TYPE_PALETTE,
					  "name", name,
					  NULL));

  if (pal_dir)
    {
      palette->filename = g_strdup_printf ("%s%s", pal_dir, name);
      g_free (pal_dir);
    }

  return palette;
}

GimpPalette *
gimp_palette_new_from_file (const gchar *filename)
{
  GimpPalette *palette;
  gchar        str[512];
  gchar       *tok;
  FILE        *fp;
  gint         r, g, b;
  GimpRGB      color;
  gint         linenum;

  g_return_val_if_fail (filename != NULL, NULL);
  g_return_val_if_fail (*filename != '\0', NULL);

  r = g = b = 0;

  /*  Open the requested file  */
  if (! (fp = fopen (filename, "r")))
    {
      g_warning ("Failed to open palette file %s", filename);
      return NULL;
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

      return NULL;
    }

  palette = gimp_palette_new (g_basename (filename));

  while (!feof (fp))
    {
      if (!fgets (str, 512, fp))
	{
	  if (feof (fp))
	    break;

	  g_message (_("Loading palette %s (line %d):\nRead error"),
		     filename, linenum);

	  fclose (fp);
	  gtk_object_sink (GTK_OBJECT (palette));
	  return NULL;
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

	  gimp_rgba_set_uchar (&color,
			       (guchar) r,
			       (guchar) g,
			       (guchar) b,
			       255);

	  gimp_palette_add_entry (palette, tok, &color);
	}
    }

  fclose (fp);

  palette->changed = FALSE;

  return palette;
}

gboolean
gimp_palette_save (GimpPalette *palette)
{
  GimpPaletteEntry *entry;
  GList            *list;
  FILE             *fp;
  guchar            r, g, b;

  g_return_val_if_fail (palette != NULL, FALSE);
  g_return_val_if_fail (GIMP_IS_PALETTE (palette), FALSE);

  if (! (fp = fopen (palette->filename, "w")))
    {
      g_message (_("Can't save palette \"%s\"\n"), palette->filename);
      return FALSE;
    }

  fprintf (fp, "GIMP Palette\n");
  fprintf (fp, "# %s -- GIMP Palette file\n", GIMP_OBJECT (palette)->name);

  for (list = palette->colors; list; list = g_list_next (list))
    {
      entry = (GimpPaletteEntry *) list->data;

      gimp_rgb_get_uchar (&entry->color, &r, &g, &b);

      fprintf (fp, "%d %d %d\t%s\n",
	       r, g, b,
	       entry->name);
    }

  fclose (fp);

  return TRUE;
}

GimpPaletteEntry *
gimp_palette_add_entry (GimpPalette *palette,
			const gchar *name,
			GimpRGB     *color)
{
  GimpPaletteEntry *entry;

  g_return_val_if_fail (palette != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_PALETTE (palette), NULL);

  g_return_val_if_fail (color != NULL, NULL);

  entry = g_new0 (GimpPaletteEntry, 1);

  entry->color    = *color;

  entry->name     = g_strdup (name ? name : _("Untitled"));
  entry->position = palette->n_colors;

  palette->colors    = g_list_append (palette->colors, entry);
  palette->n_colors += 1;

  palette->changed = TRUE;

  gtk_signal_emit (GTK_OBJECT (palette), gimp_palette_signals[CHANGED]);

  return entry;
}

void
gimp_palette_delete_entry (GimpPalette      *palette,
			   GimpPaletteEntry *entry)
{
  GList *list;
  gint   pos = 0;

  g_return_if_fail (palette != NULL);
  g_return_if_fail (GIMP_IS_PALETTE (palette));

  g_return_if_fail (entry != NULL);

  if (g_list_find (palette->colors, entry))
    {
      pos = entry->position;
      gimp_palette_entry_free (entry);

      palette->colors = g_list_remove (palette->colors, entry);

      palette->n_colors--;

      for (list = g_list_nth (palette->colors, pos);
	   list;
	   list = g_list_next (list))
	{
	  entry = (GimpPaletteEntry *) list->data;

	  entry->position = pos++;
	}

      if (palette->n_colors == 0)
	{
	  GimpRGB color;

	  gimp_rgba_set (&color, 0.0, 0.0, 0.0, 1.0);

	  gimp_palette_add_entry (palette,
				  _("Black"),
				  &color);
	}

      palette->changed = TRUE;

      gtk_signal_emit (GTK_OBJECT (palette), gimp_palette_signals[CHANGED]);
    }
}

void
gimp_palette_delete (GimpPalette *palette)
{
  g_return_if_fail (palette != NULL);
  g_return_if_fail (GIMP_IS_PALETTE (palette));

  if (palette->filename)
    unlink (palette->filename);
}

void 
gimp_palette_update_preview (GimpPalette *palette,
			     GdkGC       *gc)
{
  GimpPaletteEntry *entry;
  guchar            rgb_buf[SM_PREVIEW_WIDTH * SM_PREVIEW_HEIGHT * 3];
  GList            *list;
  gint              index;

  g_return_if_fail (palette != NULL);
  g_return_if_fail (GIMP_IS_PALETTE (palette));

  g_return_if_fail (gc != NULL);

  memset (rgb_buf, 0x0, sizeof (rgb_buf));

  gdk_draw_rgb_image (palette->pixmap,
		      gc,
		      0,
		      0,
		      SM_PREVIEW_WIDTH,
		      SM_PREVIEW_HEIGHT,
		      GDK_RGB_DITHER_NORMAL,
		      rgb_buf,
		      SM_PREVIEW_WIDTH * 3);

  index = 0;

  for (list = palette->colors; list; list = g_list_next (list))
    {
      guchar cell[3 * 3 * 3];
      gint   loop;

      entry = (GimpPaletteEntry *) list->data;

      gimp_rgb_get_uchar (&entry->color,
			  &cell[0],
			  &cell[1],
			  &cell[2]);

      for (loop = 3; loop < 27 ; loop += 3)
	{
	  cell[0 + loop] = cell[0];
	  cell[1 + loop] = cell[1];
	  cell[2 + loop] = cell[2];
	}

      gdk_draw_rgb_image (palette->pixmap,
			  gc,
			  1 + (index % ((SM_PREVIEW_WIDTH-2) / 3)) * 3,
			  1 + (index / ((SM_PREVIEW_WIDTH-2) / 3)) * 3,
			  3,
			  3,
			  GDK_RGB_DITHER_NORMAL,
			  cell,
			  3);

      index++;

      if (index >= (((SM_PREVIEW_WIDTH - 2) * (SM_PREVIEW_HEIGHT - 2)) / 9))
	break;
    }
}

static void
gimp_palette_entry_free (GimpPaletteEntry *entry)
{
  g_return_if_fail (entry != NULL);

  g_free (entry->name);
  g_free (entry);
}
