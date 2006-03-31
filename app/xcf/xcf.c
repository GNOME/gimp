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

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <glib-object.h>
#include <glib/gstdio.h>

#include "libgimpbase/gimpbase.h"

#include "core/core-types.h"

#include "core/gimp.h"
#include "core/gimpimage.h"
#include "core/gimpparamspecs.h"

#include "pdb/gimpprocedure.h"
#include "pdb/procedural_db.h"

#include "plug-in/plug-ins.h"
#include "plug-in/plug-in-proc-def.h"

#include "xcf.h"
#include "xcf-private.h"
#include "xcf-load.h"
#include "xcf-read.h"
#include "xcf-save.h"

#include "gimp-intl.h"


typedef GimpImage * GimpXcfLoaderFunc (Gimp    *gimp,
				       XcfInfo *info);


static Argument * xcf_load_invoker (ProcRecord   *procedure,
                                    Gimp         *gimp,
                                    GimpContext  *context,
                                    GimpProgress *progress,
				    Argument     *args);
static Argument * xcf_save_invoker (ProcRecord   *procedure,
                                    Gimp         *gimp,
                                    GimpContext  *context,
                                    GimpProgress *progress,
				    Argument     *args);


static ProcRecord xcf_load_procedure =
{
  "gimp-xcf-load",
  "gimp-xcf-load",
  "loads file saved in the .xcf file format",
  "The xcf file format has been designed specifically for loading and "
  "saving tiled and layered images in the GIMP. This procedure will load "
  "the specified file.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  NULL,
  GIMP_INTERNAL,
  0, NULL, 0, NULL,
  { { xcf_load_invoker } }
};

static PlugInProcDef xcf_plug_in_load_proc =
{
  "gimp-xcf-load",
  N_("GIMP XCF image"),
  NULL,
  GIMP_ICON_TYPE_STOCK_ID,
  -1,
  "gimp-wilber",
  NULL, /* ignored for load */
  0,    /* ignored for load */
  0,
  FALSE,
  &xcf_load_procedure,
  TRUE,
  "xcf",
  "",
  "0,string,gimp\\040xcf\\040",
  "image/x-xcf",
  NULL, /* fill me in at runtime */
  NULL, /* fill me in at runtime */
  NULL  /* fill me in at runtime */
};

static ProcRecord xcf_save_procedure =
{
  "gimp-xcf-save",
  "gimp-xcf-save",
  "saves file in the .xcf file format",
  "The xcf file format has been designed specifically for loading and "
  "saving tiled and layered images in the GIMP. This procedure will save "
  "the specified image in the xcf file format.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  NULL,
  GIMP_INTERNAL,
  0, NULL, 0, NULL,
  { { xcf_save_invoker } }
};

static PlugInProcDef xcf_plug_in_save_proc =
{
  "gimp-xcf-save",
  N_("GIMP XCF image"),
  NULL,
  GIMP_ICON_TYPE_STOCK_ID,
  -1,
  "gimp-wilber",
  "RGB*, GRAY*, INDEXED*",
  0, /* fill me in at runtime */
  0,
  FALSE,
  &xcf_save_procedure,
  TRUE,
  "xcf",
  "",
  NULL,
  "image/x-xcf",
  NULL, /* fill me in at runtime */
  NULL, /* fill me in at runtime */
  NULL  /* fill me in at runtime */
};


static GimpXcfLoaderFunc *xcf_loaders[] =
{
  xcf_load_image,   /* version 0 */
  xcf_load_image,   /* version 1 */
  xcf_load_image    /* version 2 */
};


void
xcf_init (Gimp *gimp)
{
  ProcRecord *procedure;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  /* So this is sort of a hack, but its better than it was before.  To
   * do this right there would be a file load-save handler type and
   * the whole interface would change but there isn't, and currently
   * the plug-in structure contains all the load-save info, so it
   * makes sense to use that for the XCF load/save handlers, even
   * though they are internal.  The only thing it requires is using a
   * PlugInProcDef struct.  -josh
   */
  procedure = gimp_procedure_init (&xcf_save_procedure, 5, 0);
  gimp_procedure_add_compat_arg (procedure, gimp,
                                 GIMP_PDB_INT32,
                                 "dummy-param",
                                 "dummy parameter");
  gimp_procedure_add_compat_arg (procedure, gimp,
                                 GIMP_PDB_IMAGE,
                                 "image",
                                 "Input image");
  gimp_procedure_add_compat_arg (procedure, gimp,
                                 GIMP_PDB_DRAWABLE,
                                 "drawable",
                                 "Active drawable of input image");
  gimp_procedure_add_compat_arg (procedure, gimp,
                                 GIMP_PDB_STRING,
                                 "filename",
                                 "The name of the file to save the image in, "
                                 "in the on-disk character set and encoding");
  gimp_procedure_add_compat_arg (procedure, gimp,
                                 GIMP_PDB_STRING,
                                 "raw_filename",
                                 "The basename of the file, in UTF-8");
  procedural_db_register (gimp, procedure);
  xcf_plug_in_save_proc.image_types_val =
    plug_ins_image_types_parse (xcf_plug_in_save_proc.image_types);
  plug_ins_add_internal (gimp, &xcf_plug_in_save_proc);

  procedure = gimp_procedure_init (&xcf_load_procedure, 3, 1);
  gimp_procedure_add_compat_arg (procedure, gimp,
                                 GIMP_PDB_INT32,
                                 "dummy-param",
                                 "dummy parameter");
  gimp_procedure_add_compat_arg (procedure, gimp,
                                 GIMP_PDB_STRING,
                                 "filename",
                                 "The name of the file to load, "
                                 "in the on-disk character set and encoding");
  gimp_procedure_add_compat_arg (procedure, gimp,
                                 GIMP_PDB_STRING,
                                 "raw-filename",
                                 "The basename of the file, in UTF-8");
  gimp_procedure_add_compat_value (procedure, gimp,
                                  GIMP_PDB_IMAGE,
                                  "image",
                                  "Output image");
  procedural_db_register (gimp, procedure);
  xcf_plug_in_load_proc.image_types_val =
    plug_ins_image_types_parse (xcf_plug_in_load_proc.image_types);
  plug_ins_add_internal (gimp, &xcf_plug_in_load_proc);
}

void
xcf_exit (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));
}

static Argument *
xcf_load_invoker (ProcRecord   *procedure,
                  Gimp         *gimp,
                  GimpContext  *context,
                  GimpProgress *progress,
		  Argument     *args)
{
  XcfInfo      info;
  Argument    *return_vals;
  GimpImage   *image   = NULL;
  const gchar *filename;
  gboolean     success = FALSE;
  gchar        id[14];

  gimp_set_busy (gimp);

  filename = g_value_get_string (&args[1].value);

  info.fp = g_fopen (filename, "rb");

  if (info.fp)
    {
      info.cp                    = 0;
      info.filename              = filename;
      info.tattoo_state          = 0;
      info.active_layer          = NULL;
      info.active_channel        = NULL;
      info.floating_sel_drawable = NULL;
      info.floating_sel          = NULL;
      info.floating_sel_offset   = 0;
      info.swap_num              = 0;
      info.ref_count             = NULL;
      info.compression           = COMPRESS_NONE;

      success = TRUE;

      info.cp += xcf_read_int8 (info.fp, (guint8*) id, 14);

      if (strncmp (id, "gimp xcf ", 9) != 0)
	{
	  success = FALSE;
	}
      else if (strcmp (id+9, "file") == 0)
	{
	  info.file_version = 0;
	}
      else if (id[9] == 'v')
	{
	  info.file_version = atoi (id + 10);
	}
      else
	{
	  success = FALSE;
	}

      if (success)
	{
	  if (info.file_version < G_N_ELEMENTS (xcf_loaders))
	    {
	      image = (*(xcf_loaders[info.file_version])) (gimp, &info);

	      if (! image)
		success = FALSE;
	    }
	  else
	    {
	      g_message (_("XCF error: unsupported XCF file version %d "
			   "encountered"), info.file_version);
	      success = FALSE;
	    }
	}

      fclose (info.fp);
    }
  else
    g_message (_("Could not open '%s' for reading: %s"),
	       gimp_filename_to_utf8 (filename), g_strerror (errno));

  return_vals = gimp_procedure_get_return_values (procedure, success);

  if (success)
    gimp_value_set_image (&return_vals[1].value, image);

  gimp_unset_busy (gimp);

  return return_vals;
}

static Argument *
xcf_save_invoker (ProcRecord   *procedure,
                  Gimp         *gimp,
                  GimpContext  *context,
                  GimpProgress *progress,
		  Argument     *args)
{
  XcfInfo      info;
  Argument    *return_vals;
  GimpImage   *image;
  const gchar *filename;
  gboolean     success = FALSE;

  gimp_set_busy (gimp);

  image    = gimp_value_get_image (&args[1].value, gimp);
  filename = g_value_get_string (&args[3].value);

  info.fp = g_fopen (filename, "wb");

  if (info.fp)
    {
      info.cp                    = 0;
      info.filename              = filename;
      info.active_layer          = NULL;
      info.active_channel        = NULL;
      info.floating_sel_drawable = NULL;
      info.floating_sel          = NULL;
      info.floating_sel_offset   = 0;
      info.swap_num              = 0;
      info.ref_count             = NULL;
      info.compression           = COMPRESS_RLE;

      xcf_save_choose_format (&info, image);

      success = xcf_save_image (&info, image);
      if (fclose (info.fp) == EOF)
        {
          g_message (_("Error saving XCF file: %s"), g_strerror (errno));

          success = FALSE;
        }
    }
  else
    g_message (_("Could not open '%s' for writing: %s"),
	       gimp_filename_to_utf8 (filename), g_strerror (errno));

  return_vals = gimp_procedure_get_return_values (procedure, success);

  gimp_unset_busy (gimp);

  return return_vals;
}
