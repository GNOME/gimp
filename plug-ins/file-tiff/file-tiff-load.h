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

#ifndef __FILE_TIFF_LOAD_H__
#define __FILE_TIFF_LOAD_H__

#define LOAD_PROC      "file-tiff-load"

typedef enum
{
  TIFF_REDUCEDFILE    = -2,
  TIFF_MISC_THUMBNAIL = -1
} TIFF_THUMBNAIL_TYPE;

typedef struct
{
  TIFF                   *tif;
  gint                    o_pages;
  gint                    n_pages;
  gint                   *pages;
  gint                   *filtered_pages; /* thumbnail is marked as < 0 */
  gint                    n_filtered_pages;
  gint                    n_reducedimage_pages;
  GtkWidget              *selector;
  GimpPageSelectorTarget  target;
  gboolean                keep_empty_space;
  gboolean                show_reduced;
} TiffSelectedPages;


GimpPDBStatusType load_image  (GimpProcedure        *procedure,
                               GFile                *file,
                               GimpRunMode           run_mode,
                               GimpImage           **image,
                               gboolean             *resolution_loaded,
                               gboolean             *profile_loaded,
                               gboolean             *ps_metadata_loaded,
                               GimpProcedureConfig  *config,
                               GError              **error);


#endif /* __FILE_TIFF_LOAD_H__ */
