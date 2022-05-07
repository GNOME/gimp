/*
 * Written 1998 Jens Ch. Restemeier <jchrr@hrz.uni-bielefeld.de>
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
 *
 */
#ifndef _FLI_H
#define _FLI_H

/** structures */

typedef struct _fli_header
{
  guint32  filesize;
  gushort  magic;
  gushort  frames;
  gushort  width;
  gushort  height;
  gushort  depth;
  gushort  flags;
  guint32  speed;
  guint32  created;
  guint32  creator;
  guint32  updated;
  gushort  aspect_x, aspect_y;
  guint32  oframe1, oframe2;
} s_fli_header;

typedef struct _fli_frame
{
  guint32  size;
  gushort  magic;
  gushort  chunks;
} s_fli_frame;

typedef struct _fli_chunk
{
  guint32  size;
  gushort  magic;
  guchar  *data;
} s_fli_chunk;

/** chunk magics */
#define NO_HEADER       0
#define HEADER_FLI      0xAF11
#define HEADER_FLC      0xAF12
#define FRAME           0xF1FA

/** codec magics */
#define FLI_COLOR       11
#define FLI_BLACK       13
#define FLI_BRUN        15
#define FLI_COPY        16
#define FLI_LC          12
#define FLI_LC_2        7
#define FLI_COLOR_2     4
#define FLI_MINI        18

/** codec masks */
#define W_COLOR         0x0001
#define W_BLACK         0x0002
#define W_BRUN          0x0004
#define W_COPY          0x0008
#define W_LC            0x0010
#define W_LC_2          0x0020
#define W_COLOR_2       0x0040
#define W_MINI          0x0080
#define W_ALL           0xFFFF

/** functions */
gboolean fli_read_header     (FILE          *f,
                              s_fli_header  *fli_header,
                              GError       **error);

gboolean fli_read_frame      (FILE          *f,
                              s_fli_header  *fli_header,
                              guchar        *old_framebuf,
                              guchar        *old_cmap,
                              guchar        *framebuf,
                              guchar        *cmap,
                              GError       **error);

gboolean fli_read_color      (FILE          *f,
                              s_fli_header  *fli_header,
                              guchar        *old_cmap,
                              guchar        *cmap,
                              GError       **error);
gboolean fli_read_color_2    (FILE          *f,
                              s_fli_header  *fli_header,
                              guchar        *old_cmap,
                              guchar        *cmap,
                              GError       **error);
gboolean fli_read_black      (FILE          *f,
                              s_fli_header  *fli_header,
                              guchar        *framebuf,
                              GError       **error);
gboolean fli_read_brun       (FILE          *f,
                              s_fli_header  *fli_header,
                              guchar        *framebuf,
                              GError       **error);
gboolean fli_read_copy       (FILE          *f,
                              s_fli_header  *fli_header,
                              guchar        *framebuf,
                              GError       **error);
gboolean fli_read_lc         (FILE          *f,
                              s_fli_header  *fli_header,
                              guchar        *old_framebuf,
                              guchar        *framebuf,
                              GError       **error);
gboolean fli_read_lc_2       (FILE          *f,
                              s_fli_header  *fli_header,
                              guchar        *old_framebuf,
                              guchar        *framebuf,
                              GError       **error);

gboolean fli_write_header    (FILE          *f,
                              s_fli_header  *fli_header,
                              GError       **error);
gboolean fli_write_frame     (FILE          *f,
                              s_fli_header  *fli_header,
                              guchar        *old_framebuf,
                              guchar        *old_cmap,
                              guchar        *framebuf,
                              guchar        *cmap,
                              gushort        codec_mask,
                              GError       **error);

gboolean  fli_write_color    (FILE          *f,
                              s_fli_header  *fli_header,
                              guchar        *old_cmap,
                              guchar        *cmap,
                              gboolean      *more,
                              GError       **error);
gboolean  fli_write_color_2  (FILE          *f,
                              s_fli_header  *fli_header,
                              guchar        *old_cmap,
                              guchar        *cmap,
                              gboolean      *more,
                              GError       **error);
gboolean fli_write_black     (FILE          *f,
                              s_fli_header  *fli_header,
                              guchar        *framebuf,
                              GError       **error);
gboolean fli_write_brun      (FILE          *f,
                              s_fli_header  *fli_header,
                              guchar        *framebuf,
                              GError       **error);
gboolean fli_write_copy      (FILE          *f,
                              s_fli_header  *fli_header,
                              guchar        *framebuf,
                              GError       **error);
gboolean fli_write_lc        (FILE          *f,
                              s_fli_header  *fli_header,
                              guchar        *old_framebuf,
                              guchar        *framebuf,
                              GError       **error);
gboolean fli_write_lc_2      (FILE          *f,
                              s_fli_header  *fli_header,
                              guchar        *old_framebuf,
                              guchar        *framebuf,
                              GError       **error);

#endif
