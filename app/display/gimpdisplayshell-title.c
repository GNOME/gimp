/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "libgimpbase/gimpbase.h"

#include "display-types.h"

#include "config/gimpdisplayconfig.h"

#include "gegl/gimp-babl.h"

#include "core/gimpcontainer.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimpitem.h"

#include "gimpdisplay.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-title.h"
#include "gimpstatusbar.h"

#include "about.h"

#include "gimp-intl.h"


#define MAX_TITLE_BUF 512


static gboolean gimp_display_shell_update_title_idle (gpointer          data);
static gint     gimp_display_shell_format_title      (GimpDisplayShell *display,
                                                      gchar            *title,
                                                      gint              title_len,
                                                      const gchar      *format);


/*  public functions  */

void
gimp_display_shell_title_update (GimpDisplayShell *shell)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (shell->title_idle_id)
    g_source_remove (shell->title_idle_id);

  shell->title_idle_id = g_idle_add (gimp_display_shell_update_title_idle,
                                     shell);
}


/*  private functions  */

static gboolean
gimp_display_shell_update_title_idle (gpointer data)
{
  GimpDisplayShell *shell = GIMP_DISPLAY_SHELL (data);

  shell->title_idle_id = 0;

  if (gimp_display_get_image (shell->display))
    {
      GimpDisplayConfig *config = shell->display->config;
      gchar              title[MAX_TITLE_BUF];
      gchar              status[MAX_TITLE_BUF];
      gint               len;

      /* format the title */
      len = gimp_display_shell_format_title (shell, title, sizeof (title),
                                             config->image_title_format);

      if (len)  /* U+2013 EN DASH */
        len += g_strlcpy (title + len, " \342\200\223 ", sizeof (title) - len);

      g_strlcpy (title + len, GIMP_ACRONYM, sizeof (title) - len);

      /* format the statusbar */
      gimp_display_shell_format_title (shell, status, sizeof (status),
                                       config->image_status_format);

      g_object_set (shell,
                    "title",  title,
                    "status", status,
                    NULL);
    }
  else
    {
      g_object_set (shell,
                    "title",  GIMP_NAME,
                    "status", " ",
                    NULL);
    }

  return FALSE;
}

static const gchar *
gimp_display_shell_title_image_type (GimpImage *image)
{
  const gchar *name = "";

  gimp_enum_get_value (GIMP_TYPE_IMAGE_BASE_TYPE,
                       gimp_image_get_base_type (image), NULL, NULL, &name, NULL);

  return name;
}

static const gchar *
gimp_display_shell_title_image_precision (GimpImage *image)
{
  const gchar *name = "";

  gimp_enum_get_value (GIMP_TYPE_PRECISION,
                       gimp_image_get_precision (image), NULL, NULL, &name, NULL);

  return name;
}

static gint print (gchar       *buf,
                   gint         len,
                   gint         start,
                   const gchar *fmt,
                   ...) G_GNUC_PRINTF (4, 5);

static gint
print (gchar       *buf,
       gint         len,
       gint         start,
       const gchar *fmt,
       ...)
{
  va_list args;
  gint    printed;

  va_start (args, fmt);

  printed = g_vsnprintf (buf + start, len - start, fmt, args);
  if (printed < 0)
    printed = len - start;

  va_end (args);

  return printed;
}

static gint
gimp_display_shell_format_title (GimpDisplayShell *shell,
                                 gchar            *title,
                                 gint              title_len,
                                 const gchar      *format)
{
  GimpImage    *image;
  GimpDrawable *drawable;
  gint          num, denom;
  gint          i = 0;

  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), 0);

  image = gimp_display_get_image (shell->display);

  if (! image)
    {
      title[0] = '\n';
      return 0;
    }

  drawable = gimp_image_get_active_drawable (image);

  gimp_zoom_model_get_fraction (shell->zoom, &num, &denom);

  while (i < title_len && *format)
    {
      switch (*format)
        {
        case '%':
          format++;
          switch (*format)
            {
            case 0:
              /* format string ends within %-sequence, print literal '%' */

            case '%':
              title[i++] = '%';
              break;

            case 'f': /* base filename */
              i += print (title, title_len, i, "%s",
                          gimp_image_get_display_name (image));
              break;

            case 'F': /* full filename */
              i += print (title, title_len, i, "%s",
                          gimp_image_get_display_path (image));
              break;

            case 'p': /* PDB id */
              i += print (title, title_len, i, "%d", gimp_image_get_ID (image));
              break;

            case 'i': /* instance */
              i += print (title, title_len, i, "%d",
                          gimp_display_get_instance (shell->display));
              break;

            case 't': /* image type */
              i += print (title, title_len, i, "%s %s",
                          gimp_display_shell_title_image_type (image),
                          gimp_display_shell_title_image_precision (image));
              break;

            case 'T': /* drawable type */
              if (drawable)
                {
                  const Babl *format = gimp_drawable_get_format (drawable);

                  i += print (title, title_len, i, "%s",
                              gimp_babl_get_description (format));
                }
              break;

            case 's': /* user source zoom factor */
              i += print (title, title_len, i, "%d", denom);
              break;

            case 'd': /* user destination zoom factor */
              i += print (title, title_len, i, "%d", num);
              break;

            case 'z': /* user zoom factor (percentage) */
              {
                gdouble  scale = gimp_zoom_model_get_factor (shell->zoom);

                i += print (title, title_len, i,
                            scale >= 0.15 ? "%.0f" : "%.2f", 100.0 * scale);
              }
              break;

            case 'D': /* dirty flag */
              if (format[1] == 0)
                {
                  /* format string ends within %D-sequence, print literal '%D' */
                  i += print (title, title_len, i, "%%D");
                  break;
                }
              if (gimp_image_is_dirty (image))
                title[i++] = format[1];
              format++;
              break;

            case 'C': /* clean flag */
              if (format[1] == 0)
                {
                  /* format string ends within %C-sequence, print literal '%C' */
                  i += print (title, title_len, i, "%%C");
                  break;
                }
              if (! gimp_image_is_dirty (image))
                title[i++] = format[1];
              format++;
              break;

            case 'B': /* dirty flag (long) */
              if (gimp_image_is_dirty (image))
                i += print (title, title_len, i, "%s", _("(modified)"));
              break;

            case 'A': /* clean flag (long) */
              if (! gimp_image_is_dirty (image))
                i += print (title, title_len, i, "%s", _("(clean)"));
              break;

            case 'm': /* memory used by image */
              {
                GimpObject *object = GIMP_OBJECT (image);
                gchar      *str;

                str = g_format_size (gimp_object_get_memsize (object, NULL));
                i += print (title, title_len, i, "%s", str);
                g_free (str);
              }
              break;

            case 'M': /* image size in megapixels */
              i += print (title, title_len, i, "%.1f",
                          (gdouble) gimp_image_get_width (image) *
                          (gdouble) gimp_image_get_height (image) / 1000000.0);
              break;

            case 'l': /* number of layers */
              i += print (title, title_len, i, "%d",
                          gimp_image_get_n_layers (image));
              break;

            case 'L': /* number of layers (long) */
              {
                gint num = gimp_image_get_n_layers (image);

                i += print (title, title_len, i,
                            ngettext ("%d layer", "%d layers", num), num);
              }
              break;

            case 'n': /* active drawable name */
              if (drawable)
                {
                  gchar *desc;

                  desc = gimp_viewable_get_description (GIMP_VIEWABLE (drawable),
                                                        NULL);
                  i += print (title, title_len, i, "%s", desc);
                  g_free (desc);
                }
              else
                {
                  i += print (title, title_len, i, "%s", _("(none)"));
                }
              break;

            case 'P': /* active drawable PDB id */
              if (drawable)
                i += print (title, title_len, i, "%d",
                            gimp_item_get_ID (GIMP_ITEM (drawable)));
              else
                i += print (title, title_len, i, "%s", _("(none)"));
              break;

            case 'W': /* width in real-world units */
              if (shell->unit != GIMP_UNIT_PIXEL)
                {
                  gdouble xres;
                  gdouble yres;
                  gchar   unit_format[8];

                  gimp_image_get_resolution (image, &xres, &yres);

                  g_snprintf (unit_format, sizeof (unit_format), "%%.%df",
                              gimp_unit_get_digits (shell->unit) + 1);
                  i += print (title, title_len, i, unit_format,
                              gimp_pixels_to_units (gimp_image_get_width (image),
                                                    shell->unit, xres));
                  break;
                }
              /* else fallthru */
            case 'w': /* width in pixels */
              i += print (title, title_len, i, "%d",
                          gimp_image_get_width (image));
              break;

            case 'H': /* height in real-world units */
              if (shell->unit != GIMP_UNIT_PIXEL)
                {
                  gdouble xres;
                  gdouble yres;
                  gchar   unit_format[8];

                  gimp_image_get_resolution (image, &xres, &yres);

                  g_snprintf (unit_format, sizeof (unit_format), "%%.%df",
                              gimp_unit_get_digits (shell->unit) + 1);
                  i += print (title, title_len, i, unit_format,
                              gimp_pixels_to_units (gimp_image_get_height (image),
                                                    shell->unit, yres));
                  break;
                }
              /* else fallthru */
            case 'h': /* height in pixels */
              i += print (title, title_len, i, "%d",
                          gimp_image_get_height (image));
              break;

            case 'u': /* unit symbol */
              i += print (title, title_len, i, "%s",
                          gimp_unit_get_symbol (shell->unit));
              break;

            case 'U': /* unit abbreviation */
              i += print (title, title_len, i, "%s",
                          gimp_unit_get_abbreviation (shell->unit));
              break;

            case 'X': /* drawable width in real world units */
              if (drawable && shell->unit != GIMP_UNIT_PIXEL)
                {
                  gdouble xres;
                  gdouble yres;
                  gchar   unit_format[8];

                  gimp_image_get_resolution (image, &xres, &yres);

                  g_snprintf (unit_format, sizeof (unit_format), "%%.%df",
                              gimp_unit_get_digits (shell->unit) + 1);
                  i += print (title, title_len, i, unit_format,
                              gimp_pixels_to_units (gimp_item_get_width
                                                    (GIMP_ITEM (drawable)),
                                                    shell->unit, xres));
                  break;
                }
              /* else fallthru */
            case 'x': /* drawable width in pixels */
              if (drawable)
                i += print (title, title_len, i, "%d",
                            gimp_item_get_width (GIMP_ITEM (drawable)));
              break;

            case 'Y': /* drawable height in real world units */
              if (drawable && shell->unit != GIMP_UNIT_PIXEL)
                {
                  gdouble xres;
                  gdouble yres;
                  gchar   unit_format[8];

                  gimp_image_get_resolution (image, &xres, &yres);

                  g_snprintf (unit_format, sizeof (unit_format), "%%.%df",
                              gimp_unit_get_digits (shell->unit) + 1);
                  i += print (title, title_len, i, unit_format,
                              gimp_pixels_to_units (gimp_item_get_height
                                                    (GIMP_ITEM (drawable)),
                                                    shell->unit, yres));
                  break;
                }
              /* else fallthru */
            case 'y': /* drawable height in pixels */
              if (drawable)
                i += print (title, title_len, i, "%d",
                            gimp_item_get_height (GIMP_ITEM (drawable)));
              break;

            case '\xc3': /* utf-8 extended char */
              {
                format ++;
                switch (*format)
                  {
                  case '\xbe':
                      {
                        /* line actually written at 23:55 on an Easter Sunday */
                        i+= print(title, title_len, i, "42");
                      }
                      break;

                  default:
                    /* in the case of an unhandled utf-8 extended char format
                     * leave the format string parsing as it was
                    */
                    format --;
                    break;
                  }
              }
              break;

              /* Other cool things to be added:
               * %r = xresolution
               * %R = yresolution
               * %� = image's fractal dimension
               * # %� = the answer to everything - (implemented)
               */

            default:
              /* format string contains unknown %-sequence, print it literally */
              i += print (title, title_len, i, "%%%c", *format);
              break;
            }
          break;

        default:
          title[i++] = *format;
          break;
        }

      format++;
    }

  title[MIN (i, title_len - 1)] = '\0';

  return i;
}
