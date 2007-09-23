/*
 * pcx.c GIMP plug-in for loading & saving PCX files
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

/* This code is based in parts on code by Francisco Bustamante, but the
   largest portion of the code has been rewritten and is now maintained
   occasionally by Nick Lamb njl195@zepler.org.uk */

#include "config.h"

#include <errno.h>
#include <string.h>

#include <glib/gstdio.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define LOAD_PROC      "file-pcx-load"
#define SAVE_PROC      "file-pcx-save"
#define PLUG_IN_BINARY "pcx"

/* Declare local functions.  */

static void   query      (void);
static void   run        (const gchar      *name,
                          gint              nparams,
                          const GimpParam  *param,
                          gint             *nreturn_vals,
                          GimpParam       **return_vals);

static gint32 load_image (const gchar      *filename);
static void   load_1     (FILE             *fp,
                          gint              width,
                          gint              height,
                          guchar           *buffer,
                          guint16           bytes);
static void   load_4     (FILE             *fp,
                          gint              width,
                          gint              height,
                          guchar           *buffer,
                          guint16           bytes);
static void   load_8     (FILE             *fp,
                          gint              width,
                          gint              height,
                          guchar           *buffer,
                          guint16           bytes);
static void   load_24    (FILE             *fp,
                          gint              width,
                          gint              height,
                          guchar           *buffer,
                          guint16           bytes);
static void   readline   (FILE             *fp,
                          guchar           *buffer,
                          gint              bytes);

static gint   save_image (const gchar      *filename,
                          gint32            image,
                          gint32            layer);
static void   save_8     (FILE             *fp,
                          gint              width,
                          gint              height,
                          guchar           *buffer);
static void   save_24    (FILE             *fp,
                          gint              width,
                          gint              height,
                          guchar           *buffer);
static void   writeline  (FILE             *fp,
                          guchar           *buffer,
                          gint              bytes);


const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

MAIN ()

static void
query (void)
{
  static const GimpParamDef load_args[] =
  {
    { GIMP_PDB_INT32,  "run-mode",     "Interactive, non-interactive" },
    { GIMP_PDB_STRING, "filename",     "The name of the file to load" },
    { GIMP_PDB_STRING, "raw-filename", "The name entered"             }
  };
  static const GimpParamDef load_return_vals[] =
  {
    { GIMP_PDB_IMAGE, "image", "Output image" }
  };

  static const GimpParamDef save_args[] =
  {
    { GIMP_PDB_INT32,    "run-mode",     "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,    "image",        "Input image"                  },
    { GIMP_PDB_DRAWABLE, "drawable",     "Drawable to save"             },
    { GIMP_PDB_STRING,   "filename",     "The name of the file to save the image in" },
    { GIMP_PDB_STRING,   "raw-filename", "The name entered"             }
  };

  gimp_install_procedure (LOAD_PROC,
                          "Loads files in Zsoft PCX file format",
                          "FIXME: write help for pcx_load",
                          "Francisco Bustamante & Nick Lamb",
                          "Nick Lamb <njl195@zepler.org.uk>",
                          "January 1997",
                          N_("ZSoft PCX image"),
			  NULL,
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (load_args),
                          G_N_ELEMENTS (load_return_vals),
                          load_args, load_return_vals);

  gimp_register_file_handler_mime (LOAD_PROC, "image/x-pcx");
  gimp_register_magic_load_handler (LOAD_PROC,
				    "pcx,pcc",
				    "",
				    "0&,byte,10,2&,byte,1,3&,byte,>0,3,byte,<9");

  gimp_install_procedure (SAVE_PROC,
                          "Saves files in ZSoft PCX file format",
                          "FIXME: write help for pcx_save",
                          "Francisco Bustamante & Nick Lamb",
                          "Nick Lamb <njl195@zepler.org.uk>",
                          "January 1997",
                          N_("ZSoft PCX image"),
			  "INDEXED, RGB, GRAY",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (save_args), 0,
                          save_args, NULL);

  gimp_register_file_handler_mime (SAVE_PROC, "image/x-pcx");
  gimp_register_save_handler (SAVE_PROC, "pcx,pcc", "");
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam  values[2];
  GimpRunMode       run_mode;
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;
  gint32            image_ID;
  gint32            drawable_ID;
  GimpExportReturn  export = GIMP_EXPORT_CANCEL;

  run_mode = param[0].data.d_int32;

  INIT_I18N ();

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

  if (strcmp (name, LOAD_PROC) == 0)
    {
      image_ID = load_image (param[1].data.d_string);

      if (image_ID != -1)
	{
	  *nreturn_vals = 2;
	  values[1].type = GIMP_PDB_IMAGE;
	  values[1].data.d_image = image_ID;
	}
      else
	{
	  status = GIMP_PDB_EXECUTION_ERROR;
	}
    }
  else if (strcmp (name, SAVE_PROC) == 0)
    {
      image_ID    = param[1].data.d_int32;
      drawable_ID = param[2].data.d_int32;

      /*  eventually export the image */
      switch (run_mode)
	{
	case GIMP_RUN_INTERACTIVE:
	case GIMP_RUN_WITH_LAST_VALS:
	  gimp_ui_init (PLUG_IN_BINARY, FALSE);
	  export = gimp_export_image (&image_ID, &drawable_ID, "PCX",
				      (GIMP_EXPORT_CAN_HANDLE_RGB |
				       GIMP_EXPORT_CAN_HANDLE_GRAY |
				       GIMP_EXPORT_CAN_HANDLE_INDEXED));
	  if (export == GIMP_EXPORT_CANCEL)
	    {
	      values[0].data.d_status = GIMP_PDB_CANCEL;
	      return;
	    }
	  break;
	default:
	  break;
	}

      switch (run_mode)
	{
	case GIMP_RUN_INTERACTIVE:
	  break;

	case GIMP_RUN_NONINTERACTIVE:
	  if (nparams != 5)
	    status = GIMP_PDB_CALLING_ERROR;
	  break;

	case GIMP_RUN_WITH_LAST_VALS:
	  break;

	default:
	  break;
	}

      if (status == GIMP_PDB_SUCCESS)
	{
	  if (! save_image (param[3].data.d_string, image_ID, drawable_ID))
	    {
	      status = GIMP_PDB_EXECUTION_ERROR;
	    }
	}

      if (export == GIMP_EXPORT_EXPORT)
	gimp_image_delete (image_ID);
    }
  else
    {
      status = GIMP_PDB_CALLING_ERROR;
    }

  values[0].data.d_status = status;
}

guchar mono[6]= { 0, 0, 0, 255, 255, 255 };

static struct
{
  guint8  manufacturer;
  guint8  version;
  guint8  compression;
  guint8  bpp;
  guint16 x1, y1;
  guint16 x2, y2;
  guint16 hdpi;
  guint16 vdpi;
  guint8  colormap[48];
  guint8  reserved;
  guint8  planes;
  guint16 bytesperline;
  guint16 color;
  guint8  filler[58];
} pcx_header;

static struct {
  size_t   size;
  gpointer address;
} pcx_header_buf_xlate[] = {
  { 1,  &pcx_header.manufacturer },
  { 1,  &pcx_header.version },
  { 1,  &pcx_header.compression },
  { 1,  &pcx_header.bpp },
  { 2,  &pcx_header.x1 },
  { 2,  &pcx_header.y1 },
  { 2,  &pcx_header.x2 },
  { 2,  &pcx_header.y2 },
  { 2,  &pcx_header.hdpi },
  { 2,  &pcx_header.vdpi },
  { 48, &pcx_header.colormap },
  { 1,  &pcx_header.reserved },
  { 1,  &pcx_header.planes },
  { 2,  &pcx_header.bytesperline },
  { 2,  &pcx_header.color },
  { 58, &pcx_header.filler },
  { 0,  NULL }
};

static void
pcx_header_from_buffer (guint8 *buf)
{
  gint i;
  gint buf_offset = 0;

  for (i = 0; pcx_header_buf_xlate[i].size != 0; i++)
    {
      g_memmove (pcx_header_buf_xlate[i].address, buf + buf_offset,
                 pcx_header_buf_xlate[i].size);
      buf_offset += pcx_header_buf_xlate[i].size;
    }
}

static void
pcx_header_to_buffer (guint8 *buf)
{
  gint i;
  gint buf_offset = 0;

  for (i = 0; pcx_header_buf_xlate[i].size != 0; i++)
    {
      g_memmove (buf + buf_offset, pcx_header_buf_xlate[i].address,
                 pcx_header_buf_xlate[i].size);
      buf_offset += pcx_header_buf_xlate[i].size;
    }
}

static gint32
load_image (const gchar *filename)
{
  FILE         *fd;
  GimpDrawable *drawable;
  GimpPixelRgn  pixel_rgn;
  guint16       offset_x, offset_y, bytesperline;
  gint32        width, height;
  gint32        image, layer;
  guchar       *dest, cmap[768];
  guint8        header_buf[128];

  fd = g_fopen (filename, "rb");
  if (!fd)
    {
      g_message (_("Could not open '%s' for reading: %s"),
                 gimp_filename_to_utf8 (filename), g_strerror (errno));
      return -1;
    }

  gimp_progress_init_printf (_("Opening '%s'"),
                             gimp_filename_to_utf8 (filename));

  if (fread (header_buf, 128, 1, fd) == 0)
    {
      g_message (_("Could not read header from '%s'"),
                 gimp_filename_to_utf8 (filename));
      return -1;
    }

  pcx_header_from_buffer (header_buf);

  if (pcx_header.manufacturer != 10)
    {
      g_message (_("'%s' is not a PCX file"),
                 gimp_filename_to_utf8 (filename));
      return -1;
    }

  offset_x = GUINT16_FROM_LE (pcx_header.x1);
  offset_y = GUINT16_FROM_LE (pcx_header.y1);
  width = GUINT16_FROM_LE (pcx_header.x2) - offset_x + 1;
  height = GUINT16_FROM_LE (pcx_header.y2) - offset_y + 1;
  bytesperline = GUINT16_FROM_LE (pcx_header.bytesperline);

  if ((width < 0) || (width > GIMP_MAX_IMAGE_SIZE))
    {
      g_message (_("Unsupported or invalid image width: %d"), width);
      return -1;
    }
  if ((height < 0) || (height > GIMP_MAX_IMAGE_SIZE))
    {
      g_message (_("Unsupported or invalid image height: %d"), height);
      return -1;
    }
  if (bytesperline < width)
    {
      g_message (_("Invalid number of bytes per line in PCX header"));
      return -1;
    }

  if (pcx_header.planes == 3 && pcx_header.bpp == 8)
    {
      image= gimp_image_new (width, height, GIMP_RGB);
      layer= gimp_layer_new (image, _("Background"), width, height,
			     GIMP_RGB_IMAGE, 100, GIMP_NORMAL_MODE);
    }
  else
    {
      image= gimp_image_new (width, height, GIMP_INDEXED);
      layer= gimp_layer_new (image, _("Background"), width, height,
			     GIMP_INDEXED_IMAGE, 100, GIMP_NORMAL_MODE);
    }
  gimp_image_set_filename (image, filename);
  gimp_image_add_layer (image, layer, 0);
  gimp_layer_set_offsets (layer, offset_x, offset_y);
  drawable = gimp_drawable_get (layer);

  if (pcx_header.planes == 1 && pcx_header.bpp == 1)
    {
      dest = (guchar *) g_malloc (width * height);
      load_1 (fd, width, height, dest, bytesperline);
      gimp_image_set_colormap (image, mono, 2);
    }
  else if (pcx_header.planes == 4 && pcx_header.bpp == 1)
    {
      dest = (guchar *) g_malloc (width * height);
      load_4 (fd, width, height, dest, bytesperline);
      gimp_image_set_colormap (image, pcx_header.colormap, 16);
    }
  else if (pcx_header.planes == 1 && pcx_header.bpp == 8)
    {
      dest = (guchar *) g_malloc (width * height);
      load_8 (fd, width, height, dest, bytesperline);
      fseek (fd, -768L, SEEK_END);
      fread (cmap, 768, 1, fd);
      gimp_image_set_colormap (image, cmap, 256);
    }
  else if (pcx_header.planes == 3 && pcx_header.bpp == 8)
    {
      dest = (guchar *) g_malloc (width * height * 3);
      load_24 (fd, width, height, dest, bytesperline);
    }
  else
    {
      g_message (_("Unusual PCX flavour, giving up"));
      return -1;
    }

  gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0, width, height, TRUE, FALSE);
  gimp_pixel_rgn_set_rect (&pixel_rgn, dest, 0, 0, width, height);

  g_free (dest);

  gimp_drawable_flush (drawable);
  gimp_drawable_detach (drawable);

  return image;
}

static void
load_8 (FILE    *fp,
	gint     width,
	gint     height,
	guchar  *buffer,
	guint16  bytes)
{
  gint    row;
  guchar *line = g_new (guchar, bytes);

  for (row = 0; row < height; buffer += width, ++row)
    {
      readline (fp, line, bytes);
      memcpy (buffer, line, width);
      gimp_progress_update ((double) row / (double) height);
    }

  g_free (line);
}

static void
load_24 (FILE    *fp,
	 gint     width,
	 gint     height,
	 guchar  *buffer,
	 guint16  bytes)
{
  gint    x, y, c;
  guchar *line = g_new (guchar, bytes);

  for (y = 0; y < height; buffer += width * 3, ++y)
    {
      for (c = 0; c < 3; ++c)
	{
	  readline (fp, line, bytes);
	  for (x = 0; x < width; ++x)
	    {
	      buffer[x * 3 + c] = line[x];
	    }
	}
      gimp_progress_update ((double) y / (double) height);
    }

  g_free (line);
}

static void
load_1 (FILE    *fp,
	gint     width,
	gint     height,
	guchar  *buffer,
	guint16  bytes)
{
  gint    x, y;
  guchar *line = g_new (guchar, bytes);

  for (y = 0; y < height; buffer += width, ++y)
    {
      readline (fp, line, bytes);
      for (x = 0; x < width; ++x)
	{
	  if (line[x / 8] & (128 >> (x % 8)))
	    buffer[x] = 1;
	  else
	    buffer[x] = 0;
	}
      gimp_progress_update ((double) y / (double) height);
    }

  g_free (line);
}

static void
load_4 (FILE    *fp,
	gint     width,
	gint     height,
	guchar  *buffer,
	guint16  bytes)
{
  gint    x, y, c;
  guchar *line = g_new (guchar, bytes);

  for (y = 0; y < height; buffer += width, ++y)
    {
      for (x = 0; x < width; ++x)
        buffer[x] = 0;
      for (c = 0; c < 4; ++c)
	{
	  readline(fp, line, bytes);
	  for (x = 0; x < width; ++x)
	    {
	      if (line[x / 8] & (128 >> (x % 8)))
		buffer[x] += (1 << c);
	    }
	}
      gimp_progress_update ((double) y / (double) height);
    }

  g_free (line);
}

static void
readline (FILE   *fp,
	  guchar *buffer,
	  gint    bytes)
{
  static guchar count = 0, value = 0;

  if (pcx_header.compression)
    {
      while (bytes--)
	{
	  if (count == 0)
	    {
	      value = fgetc (fp);
	      if (value < 0xc0)
		{
		  count = 1;
		}
	      else
		{
		  count = value - 0xc0;
		  value = fgetc (fp);
		}
	    }
	  count--;
	  *(buffer++) = value;
	}
    }
  else
    {
      fread (buffer, bytes, 1, fp);
    }
}

static gint
save_image (const gchar *filename,
	    gint32       image,
	    gint32       layer)
{
  FILE          *fp;
  GimpPixelRgn   pixel_rgn;
  GimpDrawable  *drawable;
  GimpImageType  drawable_type;
  guchar        *cmap= NULL;
  guchar        *pixels;
  gint           offset_x, offset_y;
  guint          width, height;
  gint           colors, i;
  guint8         header_buf[128];

  drawable = gimp_drawable_get (layer);
  drawable_type = gimp_drawable_type (layer);
  gimp_drawable_offsets (layer, &offset_x, &offset_y);
  width = drawable->width;
  height = drawable->height;
  gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0, width, height, FALSE, FALSE);

  gimp_progress_init_printf (_("Saving '%s'"),
                             gimp_filename_to_utf8 (filename));

  pcx_header.manufacturer = 0x0a;
  pcx_header.version = 5;
  pcx_header.compression = 1;

  switch (drawable_type)
    {
    case GIMP_INDEXED_IMAGE:
      cmap = gimp_image_get_colormap (image, &colors);
      pcx_header.bpp = 8;
      pcx_header.bytesperline = GUINT16_TO_LE (width);
      pcx_header.planes = 1;
      pcx_header.color = GUINT16_TO_LE (1);
      break;

    case GIMP_RGB_IMAGE:
      pcx_header.bpp = 8;
      pcx_header.planes = 3;
      pcx_header.color = GUINT16_TO_LE (1);
      pcx_header.bytesperline = GUINT16_TO_LE (width);
      break;

    case GIMP_GRAY_IMAGE:
      pcx_header.bpp = 8;
      pcx_header.planes = 1;
      pcx_header.color = GUINT16_TO_LE (2);
      pcx_header.bytesperline = GUINT16_TO_LE (width);
      break;

    default:
      g_message (_("Cannot save images with alpha channel."));
      return FALSE;
  }

  if ((fp = g_fopen (filename, "wb")) == NULL)
    {
      g_message (_("Could not open '%s' for writing: %s"),
                 gimp_filename_to_utf8 (filename), g_strerror (errno));
      return FALSE;
    }

  pixels = (guchar *) g_malloc (width * height * pcx_header.planes);
  gimp_pixel_rgn_get_rect (&pixel_rgn, pixels, 0, 0, width, height);

  if ((offset_x < 0) || (offset_x > (1<<16)))
    {
      g_message (_("Invalid X offset: %d"), offset_x);
      return FALSE;
    }

  if ((offset_y < 0) || (offset_y > (1<<16)))
    {
      g_message (_("Invalid Y offset: %d"), offset_y);
      return FALSE;
    }

  if (offset_x + width - 1 > (1<<16))
    {
      g_message (_("Right border out of bounds (must be < %d): %d"), (1<<16),
                 offset_x + width - 1);
      return FALSE;
    }

  if (offset_y + height - 1 > (1<<16))
    {
      g_message (_("Bottom border out of bounds (must be < %d): %d"), (1<<16),
                 offset_y + height - 1);
      return FALSE;
    }

  pcx_header.x1 = GUINT16_TO_LE ((guint16)offset_x);
  pcx_header.y1 = GUINT16_TO_LE ((guint16)offset_y);
  pcx_header.x2 = GUINT16_TO_LE ((guint16)(offset_x + width - 1));
  pcx_header.y2 = GUINT16_TO_LE ((guint16)(offset_y + height - 1));

  pcx_header.hdpi = GUINT16_TO_LE (300);
  pcx_header.vdpi = GUINT16_TO_LE (300);
  pcx_header.reserved = 0;

  pcx_header_to_buffer (header_buf);

  fwrite (header_buf, 128, 1, fp);

  switch (drawable_type)
    {
    case GIMP_INDEXED_IMAGE:
      save_8 (fp, width, height, pixels);
      fputc (0x0c, fp);
      fwrite (cmap, colors, 3, fp);
      for (i = colors; i < 256; i++)
	{
	  fputc (0, fp);
          fputc (0, fp);
          fputc (0, fp);
	}
      break;

    case GIMP_RGB_IMAGE:
      save_24 (fp, width, height, pixels);
      break;

    case GIMP_GRAY_IMAGE:
      save_8 (fp, width, height, pixels);
      fputc (0x0c, fp);
      for (i = 0; i < 256; i++)
	{
	  fputc ((guchar) i, fp);
          fputc ((guchar) i, fp);
          fputc ((guchar) i, fp);
	}
      break;

    default:
      return FALSE;
    }

  gimp_drawable_detach (drawable);
  g_free (pixels);

  if (fclose (fp) != 0)
    {
      g_message (_("Writing to file '%s' failed: %s"),
                 gimp_filename_to_utf8 (filename), g_strerror (errno));
      return FALSE;
    }
  return TRUE;
}

static void
save_8 (FILE   *fp,
	gint    width,
	gint    height,
	guchar *buffer)
{
  int row;

  for (row = 0; row < height; ++row)
    {
      writeline (fp, buffer, width);
      buffer += width;
      gimp_progress_update ((double) row / (double) height);
    }
}

static void
save_24 (FILE   *fp,
	 gint    width,
	 gint    height,
	 guchar *buffer)
{
  int     x, y, c;
  guchar *line;

  line = (guchar *) g_malloc (width);

  for (y = 0; y < height; ++y)
    {
      for (c = 0; c < 3; ++c)
	{
	  for (x = 0; x < width; ++x)
	    {
	      line[x] = buffer[(3*x) + c];
	    }
	  writeline (fp, line, width);
	}
      buffer += width * 3;
      gimp_progress_update ((double) y / (double) height);
    }
  g_free (line);
}

static void
writeline (FILE   *fp,
	   guchar *buffer,
	   gint    bytes)
{
  guchar  value, count;
  guchar *finish = buffer + bytes;

  while (buffer < finish)
    {
      value = *(buffer++);
      count = 1;

      while (buffer < finish && count < 63 && *buffer == value)
	{
	  count++; buffer++;
	}

      if (value < 0xc0 && count == 1)
	{
	  fputc (value, fp);
	}
      else
	{
	  fputc (0xc0 + count, fp);
	  fputc (value, fp);
	}
    }
}
