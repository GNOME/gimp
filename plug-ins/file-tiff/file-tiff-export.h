/* tiff saving for GIMP
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

#ifndef __FILE_TIFF_EXPORT_H__
#define __FILE_TIFF_EXPORT_H__


gboolean  export_image (GFile         *file,
                        GimpImage     *image,
                        GimpImage     *orig_image,
                        GObject       *config,
                        GimpMetadata  *metadata,
                        GError       **error);

gboolean  save_dialog  (GimpImage     *image,
                        GimpProcedure *procedure,
                        GObject       *config,
                        gboolean       has_alpha,
                        gboolean       is_monochrome,
                        gboolean       is_indexed,
                        gboolean       is_multi_layer,
                        gboolean       classic_tiff_failed);


#endif /* __FILE_TIFF_EXPORT_H__ */
