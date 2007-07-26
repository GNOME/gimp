/* simple test program to check the quality settings of existing JPEG files */

#include <stdio.h>
#include <string.h>
#include <glib.h>

#include <jpeglib.h>

#include "jpeg-quality.h"

static gchar *
truncate30 (const gchar *str)
{
  static gchar buf[31];

  if (strlen (str) <= 30)
    sprintf (buf, "%30s", str);
  else
    sprintf (buf, "...%s", str + (strlen (str) - 27));
  return buf;
}

static gboolean
analyze_file (const gchar *filename)
{
  struct jpeg_decompress_struct  cinfo;
  struct jpeg_error_mgr          jerr;
  FILE                          *f;
  gint                           quality;
  GString                       *info;
  gint                           i;

  if ((f = fopen (filename, "rb")) == NULL) {
    g_print ("Cannot open '%s'\n", filename);
    return FALSE;
  }
  info = g_string_new (truncate30 (filename));

  cinfo.err = jpeg_std_error (&jerr);
  jpeg_create_decompress (&cinfo);

  jpeg_stdio_src (&cinfo, f);

  jpeg_read_header (&cinfo, TRUE);

  /* detect JPEG quality */
  quality = jpeg_detect_quality (&cinfo);
  if (quality > 0)
    g_string_append_printf (info, ": quality %02d (exact),  ", quality);
  else if (quality < 0)
    g_string_append_printf (info, ": quality %02d (approx), ", -quality);
  else
    g_string_append_printf (info, ": quality unknown,     ");

  /* detect JPEG sampling factors */
  g_string_append_printf (info, "sampling %dx%d",
                          cinfo.comp_info[0].h_samp_factor,
                          cinfo.comp_info[0].v_samp_factor);
  for (i = 1; i < cinfo.num_components; i++)
    {
      g_string_append_printf (info, ",%dx%d",
                              cinfo.comp_info[i].h_samp_factor,
                              cinfo.comp_info[i].v_samp_factor);
    }
  jpeg_destroy_decompress (&cinfo);
  fclose (f);

  g_print ("%s\n", info->str);
  g_string_free (info, TRUE);

  return TRUE;
}

int
main (int   argc,
      char *argv[])
{
  g_set_prgname ("jpegqual");
  if (argc > 1)
    {
      for (argv++, argc--; argc; argv++, argc--)
        if (! analyze_file (*argv))
          return 1;
      return 0;
    }
  else
    {
      g_print ("Usage:\n"
               "\tjpegqual file [file [...]]\n\n"
               "The JPEG files given on the command line will be analyzed "
               "to check their\n"
               "quality settings and subsampling factors.\n");
      return 1;
    }
}
