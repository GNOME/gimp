/* tiff loading for GIMP
 *  -Peter Mattis
 *
 * The TIFF loading code has been completely revamped by Nick Lamb
 * njl195@zepler.org.uk -- 18 May 1998
 * And it now gains support for tiles (and doubtless a zillion bugs)
 * njl195@zepler.org.uk -- 12 June 1999
 * LZW patent fuss continues :(
 * njl195@zepler.org.uk -- 20 April 2000
 * The code for this filter is based on "tifftopnm" and "pnmtotiff",
 *  2 programs that are a part of the netpbm package.
 * khk@khk.net -- 13 May 2000
 * Added support for ICCPROFILE tiff tag. If this tag is present in a
 * TIFF file, then a parasite is created and vice versa.
 * peter@kirchgessner.net -- 29 Oct 2002
 * Progress bar only when run interactive
 * Added support for layer offsets - pablo.dangelo@web.de -- 7 Jan 2004
 * Honor EXTRASAMPLES tag while loading images with alphachannel
 * pablo.dangelo@web.de -- 16 Jan 2004
 */

#ifndef __FILE_TIFF_IO_H__
#define __FILE_TIFF_IO_H__

/* Adding support for GeoTIFF Tags */

#define GEOTIFF_MODELPIXELSCALE      33550
#define GEOTIFF_MODELTIEPOINT        33922
#define GEOTIFF_MODELTRANSFORMATION  34264
#define GEOTIFF_KEYDIRECTORY         34735
#define GEOTIFF_DOUBLEPARAMS         34736
#define GEOTIFF_ASCIIPARAMS          34737


static const TIFFFieldInfo geotifftags_fieldinfo[] = {
  { GEOTIFF_MODELPIXELSCALE,      -1, -1, TIFF_DOUBLE, FIELD_CUSTOM, TRUE, TRUE,  "GeoModelPixelScale" },
  { GEOTIFF_MODELTIEPOINT,        -1, -1, TIFF_DOUBLE, FIELD_CUSTOM, TRUE, TRUE,  "GeoTiePoints" },
  { GEOTIFF_MODELTRANSFORMATION,  -1, -1, TIFF_DOUBLE, FIELD_CUSTOM, TRUE, TRUE,  "GeoModelTransformation" },
  { GEOTIFF_KEYDIRECTORY,         -1, -1, TIFF_SHORT,  FIELD_CUSTOM, TRUE, TRUE,  "GeoKeyDirectory" },
  { GEOTIFF_DOUBLEPARAMS,         -1, -1, TIFF_DOUBLE, FIELD_CUSTOM, TRUE, TRUE,  "GeoDoubleParams" },
  { GEOTIFF_ASCIIPARAMS,          -1, -1, TIFF_ASCII,  FIELD_CUSTOM, TRUE, FALSE, "GeoAsciiParams" }
};

TIFF     * tiff_open                  (GFile        *file,
                                       const gchar  *mode,
                                       GError      **error);
gboolean   tiff_got_file_size_error   (void);
void       tiff_reset_file_size_error (void);


#endif /* __FILE_TIFF_IO_H__ */
