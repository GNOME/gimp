/* tiff for LIGMA
 *  -Peter Mattis
 */

#ifndef __FILE_TIFF_H__
#define __FILE_TIFF_H__


typedef enum
{
 LIGMA_COMPRESSION_NONE,
 LIGMA_COMPRESSION_LZW,
 LIGMA_COMPRESSION_PACKBITS,
 LIGMA_COMPRESSION_ADOBE_DEFLATE,
 LIGMA_COMPRESSION_JPEG,
 LIGMA_COMPRESSION_CCITTFAX3,
 LIGMA_COMPRESSION_CCITTFAX4
} LigmaCompression;


gint             ligma_compression_to_tiff_compression (LigmaCompression compression);
LigmaCompression  tiff_compression_to_ligma_compression (gint            compression);


#endif /* __FILE_TIFF_H__ */
