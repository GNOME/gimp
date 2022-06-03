
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

/*
 * This code can be used to read and write FLI movies. It is currently
 * only used for the GIMP fli plug-in, but it can be used for other
 * programs, too.
 */

#include "config.h"

#include <glib/gstdio.h>

#include <libgimp/gimp.h>

#include "fli.h"

#include "libgimp/stdplugins-intl.h"

/*
 * To avoid endian-problems I wrote these functions:
 */
static gboolean
fli_read_char (FILE *f, guchar *value, GError **error)
{
  if (fread (value, 1, 1, f) != 1)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Error reading from file."));
      return FALSE;
    }
  return TRUE;
}

static gboolean
fli_read_short (FILE *f, gushort *value, GError **error)
{
  guchar b[2];

  if (fread (&b, 1, 2, f) != 2)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Error reading from file."));
      return FALSE;
    }

  *value = (gushort) (b[1]<<8) | b[0];
  return TRUE;
}

static gboolean
fli_read_uint32 (FILE *f, guint32 *value, GError **error)
{
  guchar b[4];

  if (fread (&b, 1, 4, f) != 4)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Error reading from file."));
      return FALSE;
    }

  *value = (guint32) (b[3]<<24) | (b[2]<<16) | (b[1]<<8) | b[0];
  return TRUE;
}

static gboolean
fli_write_char (FILE    *f,
                guchar   b,
                GError **error)
{
  if (fwrite (&b, 1, 1, f) != 1)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Error writing to file."));
      return FALSE;
    }
  return TRUE;
}

static gboolean
fli_write_short (FILE     *f,
                 gushort   w,
                 GError  **error)
{
  guchar b[2];

  b[0] = w & 255;
  b[1] = (w >> 8) & 255;

  if (fwrite (&b, 1, 2, f) != 2)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Error writing to file."));
      return FALSE;
    }
  return TRUE;
}

static gboolean
fli_write_uint32 (FILE     *f,
                  guint32   l,
                  GError  **error)
{
  guchar b[4];

  b[0] = l & 255;
  b[1] = (l >> 8) & 255;
  b[2] = (l >> 16) & 255;
  b[3] = (l >> 24) & 255;

  if (fwrite (&b, 1, 4, f) != 4)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Error writing to file."));
      return FALSE;
    }
  return TRUE;
}

gboolean
fli_read_header (FILE          *f,
                 s_fli_header  *fli_header,
                 GError       **error)
{
  goffset actual_size;

  /* Get the actual file size, since filesize in header could be wrong. */
  fseek(f, 0, SEEK_END);
  actual_size = ftell(f);
  fseek(f, 0, SEEK_SET);

  if (! fli_read_uint32 (f, &fli_header->filesize, error) ||  /*  0 */
      ! fli_read_short (f, &fli_header->magic, error)     ||  /*  4 */
      ! fli_read_short (f, &fli_header->frames, error)    ||  /*  6 */
      ! fli_read_short (f, &fli_header->width, error)     ||  /*  8 */
      ! fli_read_short (f, &fli_header->height, error)    ||  /* 10 */
      ! fli_read_short (f, &fli_header->depth, error)     ||  /* 12 */
      ! fli_read_short (f, &fli_header->flags, error))        /* 14 */
    {
      g_prefix_error (error, _("Error reading header. "));
      return FALSE;
    }

  if (fli_header->magic == HEADER_FLI)
    {
      gushort speed;
      /* FLI saves speed in 1/70s */
      if (! fli_read_short (f, &speed, error))  /* 16 */
        {
          g_prefix_error (error, _("Error reading header. "));
          return FALSE;
        }
      fli_header->speed = speed * 14;
    }
  else
    {
      if (fli_header->magic == HEADER_FLC)
        {
          /* FLC saves speed in 1/1000s */
          if (! fli_read_uint32 (f, &fli_header->speed, error))  /* 16 */
            {
              g_prefix_error (error, _("Error reading header. "));
              return FALSE;
            }
        }
      else
        {
          fli_header->magic = NO_HEADER;
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("Invalid header: not a FLI/FLC animation!"));
          return FALSE;
        }
    }

  if (fli_header->width == 0)
    fli_header->width = 320;

  if (fli_header->height == 0)
    fli_header->height = 200;

if (actual_size != fli_header->filesize && actual_size >= 0)
  {
    /* Older versions of GIMP or other apps may incorrectly finish chunks on
     * an odd length, but write filesize as if that last byte was written.
     * Don't fail on off-by-one file size. */
    if (actual_size + 1 != fli_header->filesize)
      {
        g_warning (_("Incorrect file size in header: %u, should be: %u."),
                   fli_header->filesize, (guint) actual_size);
        fli_header->filesize = actual_size;
      }
  }

if (fli_header->frames == 0)
  {
    g_warning (_("Number of frames is 0. Setting to 2."));
    fli_header->frames = 2;
  }

/* A delay longer than 10 seconds is suspicious. */
if (fli_header->speed > 10000 || fli_header->speed == 0)
  {
    g_warning (_("Suspicious frame delay of %ums. Setting delay to 70ms."),
               fli_header->speed);
    fli_header->speed = 70;
  }

  g_debug ("Filesize: %u, magic: %x, frames: %u, wxh: %ux%u, depth: %u, flags: %x, speed: %u",
           fli_header->filesize, fli_header->magic, fli_header->frames,
           fli_header->width, fli_header->height, fli_header->depth,
           fli_header->flags, fli_header->speed);

  return TRUE;
}

gboolean
fli_write_header (FILE          *f,
                  s_fli_header  *fli_header,
                  GError       **error)
{
  fli_header->filesize = ftell (f);
  fseek (f, 0, SEEK_SET);

  if (! fli_write_uint32 (f, fli_header->filesize, error) || /* 0 */
      ! fli_write_short (f, fli_header->magic, error)     || /* 4 */
      ! fli_write_short (f, fli_header->frames, error)    || /* 6 */
      ! fli_write_short (f, fli_header->width, error)     || /* 8 */
      ! fli_write_short (f, fli_header->height, error)    || /* 10 */
      ! fli_write_short (f, fli_header->depth, error)     || /* 12 */
      ! fli_write_short (f, fli_header->flags, error))       /* 14 */
    {
      g_prefix_error (error, _("Error writing header. "));
      return FALSE;
    }

  if (fli_header->magic == HEADER_FLI)
    {
      /* FLI saves speed in 1/70s */
      if (! fli_write_short (f, (fli_header->speed + 7) / 14, error)) /* 16 */
        {
          g_prefix_error (error, _("Error writing header. "));
          return FALSE;
        }
    }
  else
    {
      if (fli_header->magic == HEADER_FLC)
        {
          /* FLC saves speed in 1/1000s */
          if (! fli_write_uint32 (f, fli_header->speed, error))   /* 16 */
            {
              g_prefix_error (error, _("Error writing header. "));
              return FALSE;
            }
          fseek (f, 80, SEEK_SET);
          if (! fli_write_uint32 (f, fli_header->oframe1, error) || /* 80 */
              ! fli_write_uint32 (f, fli_header->oframe2, error))   /* 84 */
            {
              g_prefix_error (error, _("Error writing header. "));
              return FALSE;
            }
        }
      else
        {
          g_set_error (error, GIMP_PLUG_IN_ERROR, 0,
                       _("Invalid header: unrecognized magic number!"));
          return FALSE;
        }
    }

  return TRUE;
}

gboolean
fli_read_frame (FILE          *f,
                s_fli_header  *fli_header,
                guchar        *old_framebuf,
                guchar        *old_cmap,
                guchar        *framebuf,
                guchar        *cmap,
                GError       **error)
{
  s_fli_frame   fli_frame;
  gint64        framepos;
  int           c;

  while (TRUE)
    {
      framepos = ftell (f);

      if (framepos < 0 ||
          ! fli_read_uint32 (f, &fli_frame.size, error) ||
          ! fli_read_short (f, &fli_frame.magic, error) ||
          ! fli_read_short (f, &fli_frame.chunks, error))
        {
          g_prefix_error (error, _("Error reading frame. "));
          return FALSE;
        }

      g_debug ("Offset: %u, frame size: %u, magic: %x, chunks: %u",
               (guint) framepos, fli_frame.size, fli_frame.magic, fli_frame.chunks);

      if (framepos + fli_frame.size > fli_header->filesize)
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("Invalid frame size points past end of file!"));
          return FALSE;
        }
      if (fli_frame.magic == FRAME)
        break;
      fseek (f, framepos + fli_frame.size, SEEK_SET);
    }

  if (fli_frame.magic == FRAME)
    {
      fseek (f, framepos + 16, SEEK_SET);
      for (c = 0; c < fli_frame.chunks; c++)
        {
          s_fli_chunk  chunk;
          gint64       chunkpos;
          gboolean     read_ok;

          chunkpos = ftell (f);
          if (chunkpos < 0 ||
              ! fli_read_uint32 (f, &chunk.size, error) ||
              ! fli_read_short (f, &chunk.magic, error))
            {
              g_prefix_error (error, _("Error reading frame. "));
              return FALSE;
            }
          g_debug ("Chunk offset: %u, chunk size: %u, chunk type: %u",
                   (guint) chunkpos, chunk.size, chunk.magic);
          if (chunkpos + chunk.size > fli_header->filesize)
            {
              g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                           _("Invalid chunk size points past end of file!"));
              return FALSE;
            }

          read_ok = TRUE;
          switch (chunk.magic)
            {
            case FLI_COLOR:
              read_ok = fli_read_color (f, fli_header, old_cmap, cmap, error);
              break;
            case FLI_COLOR_2:
              read_ok = fli_read_color_2 (f, fli_header, old_cmap, cmap, error);
              break;
            case FLI_BLACK:
              read_ok = fli_read_black (f, fli_header, framebuf, error);
              break;
            case FLI_BRUN:
              read_ok = fli_read_brun (f, fli_header, framebuf, error);
              break;
            case FLI_COPY:
              read_ok = fli_read_copy (f, fli_header, framebuf, error);
              break;
            case FLI_LC:
              read_ok = fli_read_lc (f, fli_header, old_framebuf, framebuf, error);
              break;
            case FLI_LC_2:
              read_ok = fli_read_lc_2 (f, fli_header, old_framebuf, framebuf, error);
              break;
            case FLI_MINI:
              /* unused, skip */
              break;
            default:
              /* unknown, skip */
              g_debug ("Unrecognized chunk magic: %u", chunk.magic);
              break;
            }
          if (! read_ok)
            return FALSE;

          fseek (f, chunkpos + chunk.size, SEEK_SET);
        }
      if (fli_frame.chunks == 0)
        {
          /* Silence a warning: wxh could in theory be more than INT_MAX. */
          memcpy (framebuf, old_framebuf, (gint64) fli_header->width * fli_header->height);
        }
    }
  else /* unknown, skip */
    {
      g_debug ("Unrecognized frame magic: %u (%x)", fli_frame.magic, fli_frame.magic);
    }

  fseek (f, framepos + fli_frame.size, SEEK_SET);

  return TRUE;
}

gboolean
fli_write_frame (FILE          *f,
                 s_fli_header  *fli_header,
                 guchar        *old_framebuf,
                 guchar        *old_cmap,
                 guchar        *framebuf,
                 guchar        *cmap,
                 gushort        codec_mask,
                 GError       **error)
{
  s_fli_frame  fli_frame;
  guint32      framepos, frameend;

  framepos = ftell (f);
  fseek (f, framepos + 16, SEEK_SET);

  switch (fli_header->frames)
    {
    case 0:
      fli_header->oframe1 = framepos;
      break;
    case 1:
      fli_header->oframe2 = framepos;
      break;
    }

  fli_frame.size = 0;
  fli_frame.magic = FRAME;
  fli_frame.chunks = 0;

  /*
   * create color chunk
   */
  if (fli_header->magic == HEADER_FLI)
    {
      gboolean more = FALSE;

      if (fli_write_color (f, fli_header, old_cmap, cmap, &more, error))
        {
          if (more)
            fli_frame.chunks++;
        }
      else
        {
          return FALSE;
        }
    }
  else
    {
      if (fli_header->magic == HEADER_FLC)
        {
          gboolean more = FALSE;

          if (fli_write_color_2 (f, fli_header, old_cmap, cmap, &more, error))
            {
              if (more)
                fli_frame.chunks++;
            }
          else
            {
              return FALSE;
            }
        }
      else
        {
          g_set_error (error, GIMP_PLUG_IN_ERROR, 0,
                       _("Invalid header: magic number is wrong!"));
          return FALSE;
        }
    }

#if 0
  if (codec_mask & W_COLOR)
    {
      if (fli_write_color (f, fli_header, old_cmap, cmap))
        fli_frame.chunks++;
    }
  if (codec_mask & W_COLOR_2)
    {
      if (fli_write_color_2 (f, fli_header, old_cmap, cmap))
        fli_frame.chunks++;
    }
#endif
  /* create bitmap chunk */
  if (old_framebuf == NULL)
    {
      if (! fli_write_brun (f, fli_header, framebuf, error))
        return FALSE;
    }
  else
    {
      if (! fli_write_lc (f, fli_header, old_framebuf, framebuf, error))
        return FALSE;
    }
  fli_frame.chunks++;

  frameend = ftell (f);
  fli_frame.size = frameend - framepos;
  fseek (f, framepos, SEEK_SET);
  if (! fli_write_uint32 (f, fli_frame.size, error) ||
      ! fli_write_short (f, fli_frame.magic, error) ||
      ! fli_write_short (f, fli_frame.chunks, error))
    {
      g_prefix_error (error, _("Error writing frame header. "));
      return FALSE;
    }
  fseek (f, frameend, SEEK_SET);
  fli_header->frames++;

  return TRUE;
}

/*
 * palette chunks from the classical Autodesk Animator.
 */
gboolean
fli_read_color (FILE          *f,
                s_fli_header  *fli_header,
                guchar        *old_cmap,
                guchar        *cmap,
                GError       **error)
{
  gushort num_packets, cnt_packets, col_pos;

  col_pos = 0;
  if (! fli_read_short (f, &num_packets, error))
    {
      g_prefix_error (error, _("Error reading palette. "));
      return FALSE;
    }
  for (cnt_packets = num_packets; cnt_packets > 0; cnt_packets--)
    {
      guchar skip_col, num_col, col_cnt;

      if (! fli_read_char (f, &skip_col, error) ||
          ! fli_read_char (f, &num_col, error))
        {
          g_prefix_error (error, _("Error reading palette. "));
          return FALSE;
        }
      if (num_col == 0)
        {
          for (col_pos = 0; col_pos < 768; col_pos++)
            {
              if (! fli_read_char (f, &cmap[col_pos], error))
                {
                  g_prefix_error (error, _("Error reading palette. "));
                  return FALSE;
                }
              cmap[col_pos] = cmap[col_pos] << 2;
            }
          return TRUE;
        }
      for (col_cnt = skip_col; (col_cnt > 0) && (col_pos < 768); col_cnt--)
        {
          cmap[col_pos] = old_cmap[col_pos];col_pos++;
          cmap[col_pos] = old_cmap[col_pos];col_pos++;
          cmap[col_pos] = old_cmap[col_pos];col_pos++;
        }
      for (col_cnt = num_col; (col_cnt > 0) && (col_pos < 768); col_cnt--)
        {
          if (! fli_read_char (f, &cmap[col_pos  ], error) ||
              ! fli_read_char (f, &cmap[col_pos+1], error) ||
              ! fli_read_char (f, &cmap[col_pos+2], error))
            {
              g_prefix_error (error, _("Error reading palette. "));
              return FALSE;
            }
          cmap[col_pos] = cmap[col_pos] << 2; col_pos++;
          cmap[col_pos] = cmap[col_pos] << 2; col_pos++;
          cmap[col_pos] = cmap[col_pos] << 2; col_pos++;
        }
    }

  return TRUE;
}

gboolean
fli_write_color (FILE          *f,
                 s_fli_header  *fli_header,
                 guchar        *old_cmap,
                 guchar        *cmap,
                 gboolean      *more,
                 GError       **error)
{
  guint32       chunkpos;
  gushort       num_packets;
  s_fli_chunk   chunk;

  *more = FALSE;
  chunkpos = ftell (f);
  fseek (f, chunkpos + 8, SEEK_SET);
  num_packets = 0;
  if (old_cmap == NULL)
    {
      gushort col_pos;

      num_packets = 1;
      if (! fli_write_char (f, 0, error) || /* skip no color */
          ! fli_write_char (f, 0, error))   /* 256 color */
        {
          g_prefix_error (error, _("Error writing color map. "));
          return FALSE;
        }

      for (col_pos = 0; col_pos < 768; col_pos++)
        {
          if (! fli_write_char (f, cmap[col_pos] >> 2, error))
            {
              g_prefix_error (error, _("Error writing color map. "));
              return FALSE;
            }
        }
    }
  else
    {
      gushort cnt_skip, cnt_col, col_pos, col_start;

      col_pos = 0;
      do
        {
          cnt_skip = 0;
          while ((col_pos < 256)                                      &&
                 (old_cmap[col_pos * 3 + 0] == cmap[col_pos * 3 + 0]) &&
                 (old_cmap[col_pos * 3 + 1] == cmap[col_pos * 3 + 1]) &&
                 (old_cmap[col_pos * 3 + 2] == cmap[col_pos * 3 + 2]))
            {
              cnt_skip++;
              col_pos++;
            }
          col_start = col_pos * 3;
          cnt_col = 0;
          while ((col_pos < 256) &&
                 !((old_cmap[col_pos * 3 + 0] == cmap[col_pos * 3 + 0]) &&
                   (old_cmap[col_pos * 3 + 1] == cmap[col_pos * 3 + 1]) &&
                   (old_cmap[col_pos * 3 + 2] == cmap[col_pos * 3 + 2])))
            {
              cnt_col++;
              col_pos++;
            }
          if (cnt_col > 0)
            {
              num_packets++;

              if (! fli_write_char (f, cnt_skip & 255, error) ||
                  ! fli_write_char (f, cnt_col & 255, error))
                {
                  g_prefix_error (error, _("Error writing color map. "));
                  return FALSE;
                }
              while (cnt_col > 0)
                {
                  if (! fli_write_char (f, cmap[col_start++] >> 2, error) ||
                      ! fli_write_char (f, cmap[col_start++] >> 2, error) ||
                      ! fli_write_char (f, cmap[col_start++] >> 2, error))
                    {
                      g_prefix_error (error, _("Error writing color map. "));
                      return FALSE;
                    }
                  cnt_col--;
                }
            }
        } while (col_pos < 256);
    }

  if (num_packets > 0)
    {
      chunk.size  = ftell (f) - chunkpos;
      chunk.magic = FLI_COLOR;

      fseek (f, chunkpos, SEEK_SET);
      if (! fli_write_uint32 (f, chunk.size, error) ||
          ! fli_write_short (f, chunk.magic, error) ||
          ! fli_write_short (f, num_packets, error))
        {
          g_prefix_error (error, _("Error writing color map. "));
          return FALSE;
        }

      if (chunk.size & 1)
        chunk.size++;

      fseek (f, chunkpos + chunk.size, SEEK_SET);
      *more = TRUE;
      return TRUE;
    }

  fseek (f, chunkpos, SEEK_SET);
  return TRUE;
}

/*
 * palette chunks from Autodesk Animator pro
 */
gboolean
fli_read_color_2 (FILE          *f,
                  s_fli_header  *fli_header,
                  guchar        *old_cmap,
                  guchar        *cmap,
                  GError       **error)
{
  gushort num_packets, cnt_packets, col_pos;

  if (! fli_read_short (f, &num_packets, error))
    {
      g_prefix_error (error, _("Error reading palette. "));
      return FALSE;
    }
  col_pos = 0;
  for (cnt_packets = num_packets; cnt_packets > 0; cnt_packets--)
    {
      guchar skip_col, num_col, col_cnt;

      if (! fli_read_char (f, &skip_col, error) ||
          ! fli_read_char (f, &num_col, error))
        {
          g_prefix_error (error, _("Error reading palette. "));
          return FALSE;
        }
      if (num_col == 0)
        {
          for (col_pos = 0; col_pos < 768; col_pos++)
            {
              if (! fli_read_char (f, &cmap[col_pos], error))
                {
                  g_prefix_error (error, _("Error reading palette. "));
                  return FALSE;
                }
            }
          return TRUE;
        }
      for (col_cnt = skip_col; (col_cnt > 0) && (col_pos < 768); col_cnt--)
        {
          cmap[col_pos] = old_cmap[col_pos];
          col_pos++;
          cmap[col_pos] = old_cmap[col_pos];
          col_pos++;
          cmap[col_pos] = old_cmap[col_pos];
          col_pos++;
        }
      for (col_cnt = num_col; (col_cnt > 0) && (col_pos < 768); col_cnt--)
        {
          if (! fli_read_char (f, &cmap[col_pos++], error) ||
              ! fli_read_char (f, &cmap[col_pos++], error) ||
              ! fli_read_char (f, &cmap[col_pos++], error))
            {
              g_prefix_error (error, _("Error reading palette. "));
              return FALSE;
            }
        }
    }

  return TRUE;
}

gboolean
fli_write_color_2 (FILE          *f,
                   s_fli_header  *fli_header,
                   guchar        *old_cmap,
                   guchar        *cmap,
                   gboolean      *more,
                   GError       **error)
{
  guint32       chunkpos;
  gushort       num_packets;
  s_fli_chunk   chunk;

  *more = FALSE;
  chunkpos = ftell (f);
  fseek (f, chunkpos + 8, SEEK_SET);
  num_packets = 0;
  if (old_cmap == NULL)
    {
      gushort col_pos;

      num_packets = 1;
      if (! fli_write_char (f, 0, error) || /* skip no color */
          ! fli_write_char (f, 0, error))   /* 256 color */
        {
          g_prefix_error (error, _("Error writing color map. "));
          return FALSE;
        }

      for (col_pos = 0; col_pos < 768; col_pos++)
        {
          if (! fli_write_char (f, cmap[col_pos], error))
            {
              g_prefix_error (error, _("Error writing color map. "));
              return FALSE;
            }
        }
    }
  else
    {
      gushort cnt_skip, cnt_col, col_pos, col_start;

      col_pos = 0;
      do {
          cnt_skip = 0;
          while ((col_pos < 256)                                      &&
                 (old_cmap[col_pos * 3 + 0] == cmap[col_pos * 3 + 0]) &&
                 (old_cmap[col_pos * 3 + 1] == cmap[col_pos * 3 + 1]) &&
                 (old_cmap[col_pos * 3 + 2] == cmap[col_pos * 3 + 2]))
            {
              cnt_skip++; col_pos++;
            }
          col_start = col_pos * 3;
          cnt_col = 0;
          while ((col_pos < 256) &&
                 !((old_cmap[col_pos * 3 + 0] == cmap[col_pos * 3 + 0]) &&
                   (old_cmap[col_pos * 3 + 1] == cmap[col_pos * 3 + 1]) &&
                   (old_cmap[col_pos * 3 + 2] == cmap[col_pos * 3 + 2])))
            {
              cnt_col++;
              col_pos++;
            }
          if (cnt_col > 0)
            {
              num_packets++;
              if (! fli_write_char (f, cnt_skip, error) ||
                  ! fli_write_char (f, cnt_col, error))
                {
                  g_prefix_error (error, _("Error writing color map. "));
                  return FALSE;
                }

              while (cnt_col > 0)
                {
                  if (! fli_write_char (f, cmap[col_start++], error) ||
                      ! fli_write_char (f, cmap[col_start++], error) ||
                      ! fli_write_char (f, cmap[col_start++], error))
                    {
                      g_prefix_error (error, _("Error writing color map. "));
                      return FALSE;
                    }

                  cnt_col--;
                }
            }
      } while (col_pos < 256);
    }

  if (num_packets > 0)
    {
      chunk.size = ftell (f) - chunkpos;
      chunk.magic = FLI_COLOR_2;

      fseek (f, chunkpos, SEEK_SET);
      if (! fli_write_uint32 (f, chunk.size, error) ||
          ! fli_write_short (f, chunk.magic, error) ||
          ! fli_write_short (f, num_packets, error))
        {
          g_prefix_error (error, _("Error writing color map. "));
          return FALSE;
        }

      if (chunk.size & 1)
        chunk.size++;
      fseek (f, chunkpos + chunk.size, SEEK_SET);
      *more = TRUE;
      return TRUE;
    }
  fseek (f, chunkpos, SEEK_SET);

  return TRUE;
}

/*
 * completely black frame
 */
gboolean
fli_read_black (FILE          *f,
                s_fli_header  *fli_header,
                guchar        *framebuf,
                GError       **error)
{
  memset (framebuf, 0, fli_header->width * fli_header->height);

  return TRUE;
}

gboolean
fli_write_black (FILE          *f,
                 s_fli_header  *fli_header,
                 guchar        *framebuf,
                 GError       **error)
{
  s_fli_chunk chunk;

  chunk.size = 6;
  chunk.magic = FLI_BLACK;

  if (! fli_write_uint32 (f, chunk.size, error) ||
      ! fli_write_short (f, chunk.magic, error))
    {
      g_prefix_error (error, _("Error writing black. "));
      return FALSE;
    }

  return TRUE;
}

/*
 * Uncompressed frame
 */
gboolean
fli_read_copy (FILE          *f,
               s_fli_header  *fli_header,
               guchar        *framebuf,
               GError       **error)
{
  if (fread (framebuf, fli_header->width, fli_header->height, f) != fli_header->height)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Error reading from file."));
      return FALSE;
    }

  return TRUE;
}

gboolean
fli_write_copy (FILE          *f,
                s_fli_header  *fli_header,
                guchar        *framebuf,
                GError       **error)
{
  guint32      chunkpos;
  s_fli_chunk  chunk;

  chunkpos = ftell (f);
  fseek (f, chunkpos + 6, SEEK_SET);
  if (fwrite (framebuf, fli_header->width, fli_header->height, f) != fli_header->height)
    {
      g_prefix_error (error, _("Error writing frame. "));
      return FALSE;
    }
  chunk.size = ftell (f) - chunkpos;
  chunk.magic = FLI_COPY;

  if (chunk.size & 1)
    {
      /* Dummy char to make the chunk end on an even boundary. */
      if (! fli_write_char (f, 0, error))
        {
          g_prefix_error (error, _("Error writing frame. "));
          return FALSE;
        }
      chunk.size++;
    }

  fseek (f, chunkpos, SEEK_SET);
  if (! fli_write_uint32 (f, chunk.size, error) ||
      ! fli_write_short (f, chunk.magic, error))
    {
      g_prefix_error (error, _("Error writing frame. "));
      return FALSE;
    }

  fseek (f, chunkpos + chunk.size, SEEK_SET);
  return TRUE;
}

/*
 * This is a RLE algorithm, used for the first image of an animation
 */
gboolean
fli_read_brun (FILE          *f,
               s_fli_header  *fli_header,
               guchar        *framebuf,
               GError       **error)
{
  gushort  yc;
  guchar  *pos;

  for (yc = 0; yc < fli_header->height; yc++)
    {
      guchar pc, pcnt;
      size_t n, xc;

      if (! fli_read_char (f, &pc, error))
        {
          g_prefix_error (error, _("Error reading compressed data. "));
          return FALSE;
        }
      xc = 0;
      pos = framebuf + (fli_header->width * yc);
      n = (size_t) fli_header->width * (fli_header->height - yc);
      for (pcnt = pc; pcnt > 0; pcnt--)
        {
          guchar ps;

          if (! fli_read_char (f, &ps, error))
            {
              g_prefix_error (error, _("Error reading compressed data. "));
              return FALSE;
            }
          if (ps & 0x80)
            {
              gushort len;

              for (len = -(signed char) ps; len > 0 && xc < n; len--)
                {
                  if (! fli_read_char (f, &pos[xc++], error))
                    {
                      g_prefix_error (error, _("Error reading compressed data. "));
                      return FALSE;
                    }
                }
              if (len > 0 && xc >= n)
                {
                  g_set_error (error, G_FILE_ERROR, 0,
                               _("Overflow reading compressed data. Possibly corrupt file."));
                  return FALSE;
                }
            }
          else
            {
              guchar  val;
              size_t  len;

              len = MIN (n - xc, ps);
              if (! fli_read_char (f, &val, error))
                {
                  g_prefix_error (error, _("Error reading compressed data. "));
                  return FALSE;
                }
              memset (&(pos[xc]), val, len);
              xc+=len;
            }
        }
    }
  return TRUE;
}

gboolean
fli_write_brun (FILE          *f,
                s_fli_header  *fli_header,
                guchar        *framebuf,
                GError       **error)
{
  guint32       chunkpos;
  s_fli_chunk   chunk;
  gushort       yc;
  guchar       *linebuf;

  chunkpos = ftell (f);
  fseek (f, chunkpos + 6, SEEK_SET);

  for (yc = 0; yc < fli_header->height; yc++)
    {
      gushort  xc, t1, pc, tc;
      guint32  linepos, lineend, bc;

      linepos = ftell (f); bc = 0;
      fseek (f, 1, SEEK_CUR);
      linebuf = framebuf + (yc * fli_header->width);
      xc = 0; tc = 0; t1 = 0;
      while (xc < fli_header->width)
        {
          pc = 1;
          while ((pc < 120)                      &&
                 ((xc + pc) < fli_header->width) &&
                 (linebuf[xc + pc] == linebuf[xc]))
            {
              pc++;
            }
          if (pc > 2)
            {
              if (tc > 0)
                {
                  bc++;
                  if (! fli_write_char (f, (tc - 1)^0xFF, error) ||
                      fwrite (linebuf + t1, 1, tc, f) != tc)
                    {
                      g_prefix_error (error, _("Error writing frame. "));
                      return FALSE;
                    }
                  tc = 0;
                }
              bc++;
              if (! fli_write_char (f, pc, error) ||
                  ! fli_write_char (f, linebuf[xc], error))
                {
                  g_prefix_error (error, _("Error writing frame. "));
                  return FALSE;
                }
              t1 = xc + pc;
            }
          else
            {
              tc+=pc;
              if (tc > 120)
                {
                  bc++;
                  if (! fli_write_char (f, (tc - 1)^0xFF, error) ||
                      fwrite (linebuf + t1, 1, tc, f) != tc)
                    {
                      g_prefix_error (error, _("Error writing frame. "));
                      return FALSE;
                    }
                  tc = 0;
                  t1 = xc + pc;
                }
            }
          xc+=pc;
        }
      if (tc > 0)
        {
          bc++;
          if (! fli_write_char (f, (tc - 1)^0xFF, error) ||
              fwrite (linebuf + t1, 1, tc, f) != tc)
            {
              g_prefix_error (error, _("Error writing frame. "));
              return FALSE;
            }
          tc = 0;
        }
      lineend = ftell (f);
      fseek (f, linepos, SEEK_SET);
      if (! fli_write_char (f, bc, error))
        {
          g_prefix_error (error, _("Error writing frame. "));
          return FALSE;
        }
      fseek (f, lineend, SEEK_SET);
    }

  chunk.size = ftell (f) - chunkpos;
  chunk.magic = FLI_BRUN;

  if (chunk.size & 1)
    {
      /* Dummy char to make the chunk end on an even boundary. */
      if (! fli_write_char (f, 0, error))
        {
          g_prefix_error (error, _("Error writing frame. "));
          return FALSE;
        }
      chunk.size++;
    }

  fseek (f, chunkpos, SEEK_SET);
  if (! fli_write_uint32 (f, chunk.size, error) ||
      ! fli_write_short (f, chunk.magic, error))
    {
      g_prefix_error (error, _("Error writing frame. "));
      return FALSE;
    }

  fseek (f, chunkpos + chunk.size, SEEK_SET);
  return TRUE;
}

/*
 * This is the delta-compression method from the classic Autodesk
 * Animator.  It's basically the RLE method from above, but it
 * supports skipping unchanged lines at the beginning and end of an
 * image, and unchanged pixels in a line. This chunk is used in FLI
 * files.
 */
gboolean
fli_read_lc (FILE          *f,
             s_fli_header  *fli_header,
             guchar        *old_framebuf,
             guchar        *framebuf,
             GError       **error)
{
  gushort  yc, firstline, numline;
  guchar  *pos;

  memcpy (framebuf, old_framebuf, fli_header->width * fli_header->height);

  if (! fli_read_short (f, &firstline, error) ||
      ! fli_read_short (f, &numline, error))
    {
      g_prefix_error (error, _("Error reading compressed data. "));
      return FALSE;
    }

  if (numline > fli_header->height || fli_header->height - numline < firstline)
    return TRUE;

  for (yc = 0; yc < numline; yc++)
    {
      guchar  pc, pcnt;
      size_t  n, xc;

      if (! fli_read_char (f, &pc, error))
        {
          g_prefix_error (error, _("Error reading compressed data. "));
          return FALSE;
        }
      xc = 0;
      pos = framebuf + (fli_header->width * (firstline + yc));
      n = (size_t) fli_header->width * (fli_header->height - firstline - yc);
      for (pcnt = pc; pcnt > 0; pcnt--)
        {
          guchar ps, skip;

          if (! fli_read_char (f, &skip, error) ||
              ! fli_read_char (f, &ps, error))
            {
              g_prefix_error (error, _("Error reading compressed data. "));
              return FALSE;
            }

          xc += MIN (n - xc, skip);
          if (ps & 0x80)
            {
              guchar val;
              size_t len;

              ps = -(signed char) ps;
              if (! fli_read_char (f, &val, error))
                {
                  g_prefix_error (error, _("Error reading compressed data. "));
                  return FALSE;
                }
              len = MIN (n - xc, ps);
              memset (&(pos[xc]), val, len);
              xc += len;
            }
          else
            {
              size_t len, len_read;

              len = MIN (n - xc, ps);
              if (len > 0)
                {
                  len_read = fread (&pos[xc], len, 1, f);
                  if (len_read != 1)
                    {
                      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                                   _("Error reading from file."));
                      g_prefix_error (error, _("Error reading compressed data. "));
                      return FALSE;
                    }
                }
              xc += len;
            }
        }
    }
  return TRUE;
}

gboolean
fli_write_lc (FILE          *f,
              s_fli_header  *fli_header,
              guchar        *old_framebuf,
              guchar        *framebuf,
              GError       **error)
{
  guint32       chunkpos;
  s_fli_chunk   chunk;
  gushort       yc, firstline, numline, lastline;
  guchar       *linebuf, *old_linebuf;

  chunkpos = ftell (f);
  fseek (f, chunkpos + 6, SEEK_SET);

  /* first check, how many lines are unchanged at the beginning */
  firstline = 0;
  while ((memcmp (old_framebuf + (firstline * fli_header->width),
                  framebuf + (firstline * fli_header->width),
                  fli_header->width) == 0) &&
         (firstline < fli_header->height))
    firstline++;

  /* then check from the end, how many lines are unchanged */
  if (firstline < fli_header->height)
    {
      lastline = fli_header->height - 1;
      while ((memcmp (old_framebuf + (lastline * fli_header->width),
                      framebuf + (lastline * fli_header->width),
                      fli_header->width) == 0) &&
             (lastline > firstline))
        lastline--;
      numline = (lastline - firstline) + 1;
    }
  else
    {
      numline = 0;
    }
  if (numline == 0)
    firstline = 0;

  if (! fli_write_short (f, firstline, error) ||
      ! fli_write_short (f, numline, error))
    {
      g_prefix_error (error, _("Error writing frame. "));
      return FALSE;
    }

  for (yc = 0; yc < numline; yc++)
    {
      gushort xc, sc, cc, tc;
      guint32 linepos, lineend, bc;

      linepos = ftell (f); bc = 0;
      fseek (f, 1, SEEK_CUR);

      linebuf = framebuf + ((firstline + yc)*fli_header->width);
      old_linebuf = old_framebuf + ((firstline + yc)*fli_header->width);
      xc = 0;
      while (xc < fli_header->width)
        {
          sc = 0;
          while ((xc < fli_header->width)         &&
                 (linebuf[xc] == old_linebuf[xc]) &&
                 (sc < 255))
            {
              xc++;
              sc++;
            }
          if (! fli_write_char (f, sc, error))
            {
              g_prefix_error (error, _("Error writing frame. "));
              return FALSE;
            }
          cc = 1;
          while ((linebuf[xc] == linebuf[xc + cc]) &&
                 ((xc + cc)<fli_header->width)     &&
                 (cc < 120))
            {
              cc++;
            }
          if (cc > 2)
            {
              bc++;
              if (! fli_write_char (f, (cc - 1)^0xFF, error) ||
                  ! fli_write_char (f, linebuf[xc], error))
                {
                  g_prefix_error (error, _("Error writing frame. "));
                  return FALSE;
                }
              xc += cc;
            }
          else
            {
              tc = 0;
              do {
                  sc = 0;
                  while ((linebuf[tc + xc + sc] == old_linebuf[tc + xc + sc]) &&
                         ((tc + xc + sc)<fli_header->width)                   &&
                         (sc < 5))
                    {
                      sc++;
                    }
                  cc = 1;
                  while ((linebuf[tc + xc] == linebuf[tc + xc + cc]) &&
                         ((tc + xc + cc)<fli_header->width)          &&
                         (cc < 10))
                    {
                      cc++;
                    }
                  tc++;
              } while ((tc < 120) &&
                       (cc < 9)   &&
                       (sc < 4)   &&
                       ((xc + tc) < fli_header->width));
              bc++;
              if (! fli_write_char (f, tc, error) ||
                  fwrite (linebuf + xc, tc, 1, f) != 1)
                {
                  g_prefix_error (error, _("Error writing frame. "));
                  return FALSE;
                }
              xc += tc;
            }
        }
      lineend = ftell (f);
      fseek (f, linepos, SEEK_SET);
      if (! fli_write_char (f, bc, error))
        {
          g_prefix_error (error, _("Error writing frame. "));
          return FALSE;
        }
      fseek (f, lineend, SEEK_SET);
    }

  chunk.size = ftell (f) - chunkpos;
  chunk.magic = FLI_LC;

  if (chunk.size & 1)
    {
      /* Dummy char to make the chunk end on an even boundary. */
      if (! fli_write_char (f, 0, error))
        {
          g_prefix_error (error, _("Error writing frame. "));
          return FALSE;
        }
      chunk.size++;
    }

  fseek (f, chunkpos, SEEK_SET);
  if (! fli_write_uint32 (f, chunk.size, error) ||
      ! fli_write_short (f, chunk.magic, error))
    {
      g_prefix_error (error, _("Error writing frame. "));
      return FALSE;
    }

  fseek (f, chunkpos + chunk.size, SEEK_SET);
  return TRUE;
}


/*
 * This is an enhanced version of the old delta-compression used by
 * the autodesk animator pro. It's word-oriented, and supports
 * skipping larger parts of the image. This chunk is used in FLC
 * files.
 */
gboolean
fli_read_lc_2 (FILE          *f,
               s_fli_header  *fli_header,
               guchar        *old_framebuf,
               guchar        *framebuf,
               GError       **error)
{
  gushort  yc, lc, numline;
  guchar  *pos;
  guint32  len_read;

  memcpy (framebuf, old_framebuf, fli_header->width * fli_header->height);
  yc = 0;

  if (! fli_read_short (f, &numline, error))
    {
      g_prefix_error (error, _("Error reading compressed data. "));
      return FALSE;
    }
  if (numline > fli_header->height)
    {
      g_warning ("Number of lines %u larger than frame height %u.", numline, fli_header->height);
      numline = fli_header->height;
    }

  for (lc = 0; lc < numline; lc++)
    {
      gushort pc, pcnt, lpf, lpn;
      size_t  n, xc;

      if (! fli_read_short (f, &pc, error))
        {
          g_prefix_error (error, _("Error reading compressed data. "));
          return FALSE;
        }

      lpf = 0; lpn = 0;
      while (pc & 0x8000)
        {
          if (pc & 0x4000)
            {
              yc += -(signed short) pc;
            }
          else
            {
              lpf = 1;
              lpn = pc & 0xFF;
            }

          if (! fli_read_short (f, &pc, error))
            {
              g_prefix_error (error, _("Error reading compressed data. "));
              return FALSE;
            }
        }
      yc = MIN (yc, fli_header->height);
      xc = 0;
      pos = framebuf + (fli_header->width * yc);
      n = (size_t) fli_header->width * (fli_header->height - yc);
      for (pcnt = pc; pcnt > 0; pcnt--)
        {
          guchar ps, skip;

          if (! fli_read_char (f, &skip, error) ||
              ! fli_read_char (f, &ps, error))
            {
              g_prefix_error (error, _("Error reading compressed data. "));
              return FALSE;
            }

          xc += MIN (n - xc, skip);
          if (ps & 0x80)
            {
              guchar v1, v2;

              ps = -(signed char) ps;
              if (! fli_read_char (f, &v1, error) ||
                  ! fli_read_char (f, &v2, error))
                {
                  g_prefix_error (error, _("Error reading compressed data. "));
                  return FALSE;
                }
              while (ps > 0 && xc + 1 < n)
                {
                  pos[xc++] = v1;
                  pos[xc++] = v2;
                  ps--;
                }
            }
          else
            {
              size_t len;

              len = MIN ((n - xc)/2, ps);
              if (len > 0)
                {
                  len_read = fread (&pos[xc], len, 2, f);
                  if (len_read != 2)
                    {
                      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                                   _("Error reading from file."));
                      g_prefix_error (error, _("Error reading compressed data. "));
                      return FALSE;
                    }
                }
              xc += len << 1;
            }
        }
      if (lpf)
        pos[xc] = lpn;
      yc++;
    }
  return TRUE;
}
