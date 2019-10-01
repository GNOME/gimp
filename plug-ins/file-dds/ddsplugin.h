/*
 * DDS GIMP plugin
 *
 * Copyright (C) 2004-2012 Shawn Kirst <skirst@gmail.com>,
 * with parts (C) 2003 Arne Reuter <homepage@arnereuter.de> where specified.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __DDSPLUGIN_H__
#define __DDSPLUGIN_H__

#define DDS_PLUGIN_VERSION_MAJOR     3
#define DDS_PLUGIN_VERSION_MINOR     9
#define DDS_PLUGIN_VERSION_REVISION  90

#define DDS_PLUGIN_VERSION  \
   ((unsigned int)(DDS_PLUGIN_VERSION_MAJOR << 16) | \
    (unsigned int)(DDS_PLUGIN_VERSION_MINOR <<  8) | \
    (unsigned int)(DDS_PLUGIN_VERSION_REVISION))

typedef struct
{
  gint     compression;
  gint     mipmaps;
  gint     savetype;
  gint     format;
  gint     transindex;
  gint     mipmap_filter;
  gint     mipmap_wrap;
  gboolean gamma_correct;
  gboolean srgb;
  gdouble  gamma;
  gboolean perceptual_metric;
  gboolean preserve_alpha_coverage;
  gdouble  alpha_test_threshold;
} DDSWriteVals;

extern DDSWriteVals dds_write_vals;

extern GimpPDBStatusType read_dds  (GFile         *file,
                                    GimpImage    **image,
                                    gboolean       interactive,
                                    GimpProcedure *procedure,
                                    GObject       *config);
extern GimpPDBStatusType write_dds (GFile         *file,
                                    GimpImage     *image,
                                    GimpDrawable  *drawable,
                                    gboolean       interactive);


#define LOAD_PROC                "file-dds-load"
#define SAVE_PROC                "file-dds-save"

#define DECODE_YCOCG_PROC        "color-decode-ycocg"
#define DECODE_YCOCG_SCALED_PROC "color-decode-ycocg-scaled"
#define DECODE_ALPHA_EXP_PROC    "color-decode-alpha-exp"

#endif /* __DDSPLUGIN_H__ */
