#ifndef __FRAC_H__
#define __FRAC_H__


void xcf_compress_frac_info (int _layer_type);
void xcf_save_compress_frac_init (int _dom_density, double quality);
void xcf_load_compress_frac_init (int _image_scale, int _iterations);

gint xcf_load_frac_compressed_tile (XcfInfo *info, Tile *tile);
gint xcf_save_frac_compressed_tile (XcfInfo *info, Tile *tile);


#endif /* __FRAC_H__ */
