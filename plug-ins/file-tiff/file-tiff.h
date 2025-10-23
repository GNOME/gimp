/* tiff for GIMP
 *  -Peter Mattis
 */

#ifndef __FILE_TIFF_H__
#define __FILE_TIFF_H__

#define LOAD_PROC   "file-tiff-load"
#define EXPORT_PROC "file-tiff-export"

typedef enum
{
 GIMP_COMPRESSION_NONE,
 GIMP_COMPRESSION_LZW,
 GIMP_COMPRESSION_PACKBITS,
 GIMP_COMPRESSION_ADOBE_DEFLATE,
 GIMP_COMPRESSION_JPEG,
 GIMP_COMPRESSION_CCITTFAX3,
 GIMP_COMPRESSION_CCITTFAX4
} GimpCompression;


gint              gimp_compression_to_tiff_compression (GimpCompression  compression);
GimpCompression   tiff_compression_to_gimp_compression (gint             compression);
const gchar     * gimp_compression_to_nick             (GimpCompression  compression);


#endif /* __FILE_TIFF_H__ */
