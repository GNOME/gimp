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

#include "core-types.h"

#include "base/temp-buf.h"

#include "gimppalette.h"

#include "libgimp/gimpintl.h"


/*  local function prototypes  */

static void       gimp_palette_class_init       (GimpPaletteClass  *klass);
static void       gimp_palette_init             (GimpPalette       *palette);

static void       gimp_palette_finalize         (GObject           *object);

static TempBuf  * gimp_palette_get_new_preview  (GimpViewable      *viewable,
                                                 gint               width,
                                                 gint               height);
static void       gimp_palette_dirty            (GimpData          *data);
static gboolean   gimp_palette_save             (GimpData          *data);
static gchar    * gimp_palette_get_extension    (GimpData          *data);

static void       gimp_palette_entry_free       (GimpPaletteEntry  *entry);


/*  private variables  */

static GimpDataClass *parent_class = NULL;


GType
gimp_palette_get_type (void)
{
  static GType palette_type = 0;

  if (! palette_type)
    {
      static const GTypeInfo palette_info =
      {
        sizeof (GimpPaletteClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_palette_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpPalette),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_palette_init,
      };

      palette_type = g_type_register_static (GIMP_TYPE_DATA,
					     "GimpPalette", 
					     &palette_info, 0);
    }

  return palette_type;
}

static void
gimp_palette_class_init (GimpPaletteClass *klass)
{
  GObjectClass      *object_class;
  GimpViewableClass *viewable_class;
  GimpDataClass     *data_class;

  object_class   = G_OBJECT_CLASS (klass);
  viewable_class = GIMP_VIEWABLE_CLASS (klass);
  data_class     = GIMP_DATA_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize          = gimp_palette_finalize;

  viewable_class->get_new_preview = gimp_palette_get_new_preview;

  data_class->dirty               = gimp_palette_dirty;
  data_class->save                = gimp_palette_save;
  data_class->get_extension       = gimp_palette_get_extension;
}

static void
gimp_palette_init (GimpPalette *palette)
{
  palette->colors    = NULL;
  palette->n_colors  = 0;
  palette->n_columns = 0;
}

static void
gimp_palette_finalize (GObject *object)
{
  GimpPalette *palette;

  g_return_if_fail (GIMP_IS_PALETTE (object));

  palette = GIMP_PALETTE (object);

  if (palette->colors)
    {
      g_list_foreach (palette->colors, (GFunc) gimp_palette_entry_free, NULL);
      g_list_free (palette->colors);
      palette->colors = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static TempBuf *
gimp_palette_get_new_preview (GimpViewable *viewable,
			      gint          width,
			      gint          height)
{
  GimpPalette      *palette;
  GimpPaletteEntry *entry;
  TempBuf          *temp_buf;
  guchar           *buf;
  guchar           *b;
  GList            *list;
  guchar            white[3] = { 255, 255, 255 };
  gint              columns;
  gint              rows;
  gint              x, y, i;

  palette = GIMP_PALETTE (viewable);

  temp_buf = temp_buf_new (width, height,
                           3,
                           0, 0,
                           white);

#define CELL_SIZE 4

  columns = width  / CELL_SIZE;
  rows    = height / CELL_SIZE;

  buf = temp_buf_data (temp_buf);
  b   = g_new (guchar, width * 3);

  memset (b, 255, width * 3);

  list = palette->colors;

  for (y = 0; y < rows && list; y++)
    {
      for (x = 0; x < columns && list; x++)
	{
	  entry = (GimpPaletteEntry *) list->data;

	  list = g_list_next (list);

	  gimp_rgb_get_uchar (&entry->color,
			      &b[x * CELL_SIZE * 3 + 0],
			      &b[x * CELL_SIZE * 3 + 1],
			      &b[x * CELL_SIZE * 3 + 2]);

	  for (i = 1; i < CELL_SIZE; i++)
	    {
	      b[(x * CELL_SIZE + i) * 3 + 0] = b[(x * CELL_SIZE) * 3 + 0];
	      b[(x * CELL_SIZE + i) * 3 + 1] = b[(x * CELL_SIZE) * 3 + 1];
	      b[(x * CELL_SIZE + i) * 3 + 2] = b[(x * CELL_SIZE) * 3 + 2];
	    }
	}

      for (i = 0; i < CELL_SIZE; i++)
	{
	  memcpy (buf + ((y * CELL_SIZE + i) * width) * 3, b, width * 3);
	}
    }

#undef CELL_SIZE

  g_free (b);

  return temp_buf;
}

GimpData *
gimp_palette_new (const gchar *name)
{
  GimpPalette *palette = NULL;

  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (*name != '\0', NULL);

  palette = GIMP_PALETTE (gtk_object_new (GIMP_TYPE_PALETTE,
					  "name", name,
					  NULL));

  gimp_data_dirty (GIMP_DATA (palette));

  return GIMP_DATA (palette);
}

GimpData *
gimp_palette_get_standard (void)
{
  static GimpPalette *standard_palette = NULL;

  if (! standard_palette)
    {
      standard_palette = GIMP_PALETTE (g_object_new (GIMP_TYPE_PALETTE, NULL));

      gimp_object_set_name (GIMP_OBJECT (standard_palette), "Standard");
    }

  return GIMP_DATA (standard_palette);
}

GimpData *
gimp_palette_load (const gchar *filename)
{
  GimpPalette *palette;
  gchar        str[1024];
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

  palette = g_object_new (GIMP_TYPE_PALETTE, NULL);

  gimp_data_set_filename (GIMP_DATA (palette), filename);

  if (! fgets (str, 1024, fp))
    {
      g_message (_("Loading palette %s (line %d):\nRead error"),
		 filename, linenum);

      fclose (fp);
      g_object_unref (G_OBJECT (palette));
      return NULL;
    }

  linenum++;

  if (! strncmp (str, "Name: ", strlen ("Name: ")))
    {
      gimp_object_set_name (GIMP_OBJECT (palette),
			    g_strstrip (&str[strlen ("Name: ")]));

      if (! fgets (str, 1024, fp))
	{
	  g_message (_("Loading palette %s (line %d):\nRead error"),
		     filename, linenum);

	  fclose (fp);
	  g_object_unref (G_OBJECT (palette));
	  return NULL;
	}

      linenum++;

      if (! strncmp (str, "Columns: ", strlen ("Columns: ")))
	{
	  gint columns;

	  columns = atoi (g_strstrip (&str[strlen ("Columns: ")]));

	  if (columns < 0 || columns > 256)
	    {
	      g_message (_("Loading palette %s (line %d):\n"
			   "Invalid number or columns"),
			 filename, linenum);

	      columns = 0;
	    }

	  palette->n_columns = columns;

	  if (! fgets (str, 1024, fp))
	    {
	      g_message (_("Loading palette %s (line %d):\nRead error"),
			 filename, linenum);

	      fclose (fp);
	      g_object_unref (G_OBJECT (palette));
	      return NULL;
	    }

	  linenum++;
	}
    }
  else /* old palette format */
    {
      g_warning ("old palette format %s", filename);

      gimp_object_set_name (GIMP_OBJECT (palette), g_basename (filename));
    }

  while (! feof (fp))
    {
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

      if (! fgets (str, 1024, fp))
	{
	  if (feof (fp))
	    break;

	  g_message (_("Loading palette %s (line %d):\nRead error"),
		     filename, linenum);

	  fclose (fp);
	  g_object_unref (G_OBJECT (palette));
	  return NULL;
	}

      linenum++;
    }

  fclose (fp);

  GIMP_DATA (palette)->dirty = FALSE;

  return GIMP_DATA (palette);
}

static void
gimp_palette_dirty (GimpData *data)
{
  if (GIMP_DATA_CLASS (parent_class)->dirty)
    GIMP_DATA_CLASS (parent_class)->dirty (data);
}

static gboolean
gimp_palette_save (GimpData *data)
{
  GimpPalette      *palette;
  GimpPaletteEntry *entry;
  GList            *list;
  FILE             *fp;
  guchar            r, g, b;

  palette = GIMP_PALETTE (data);

  if (! (fp = fopen (GIMP_DATA (palette)->filename, "w")))
    {
      g_message (_("Can't save palette \"%s\"\n"),
		 GIMP_DATA (palette)->filename);
      return FALSE;
    }

  fprintf (fp, "GIMP Palette\n");
  fprintf (fp, "Name: %s\n", GIMP_OBJECT (palette)->name);
  fprintf (fp, "Columns: %d\n#\n", CLAMP (palette->n_columns, 0, 256));

  for (list = palette->colors; list; list = g_list_next (list))
    {
      entry = (GimpPaletteEntry *) list->data;

      gimp_rgb_get_uchar (&entry->color, &r, &g, &b);

      fprintf (fp, "%3d %3d %3d\t%s\n",
	       r, g, b,
	       entry->name);
    }

  fclose (fp);

  if (GIMP_DATA_CLASS (parent_class)->save)
    return GIMP_DATA_CLASS (parent_class)->save (data);

  return TRUE;
}

static gchar *
gimp_palette_get_extension (GimpData *data)
{
  return GIMP_PALETTE_FILE_EXTENSION;
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

  gimp_data_dirty (GIMP_DATA (palette));

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

      gimp_data_dirty (GIMP_DATA (palette));
    }
}

static void
gimp_palette_entry_free (GimpPaletteEntry *entry)
{
  g_return_if_fail (entry != NULL);

  g_free (entry->name);
  g_free (entry);
}
