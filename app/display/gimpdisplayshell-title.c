/* LIGMA - The GNU Image Manipulation Program
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmabase/ligmabase.h"
#include "libligmacolor/ligmacolor.h"
#include "libligmawidgets/ligmawidgets.h"

#include "libligmabase/ligmabase.h"

#include "display-types.h"

#include "config/ligmadisplayconfig.h"

#include "gegl/ligma-babl.h"

#include "core/ligmacontainer.h"
#include "core/ligmadrawable.h"
#include "core/ligmaimage.h"
#include "core/ligmaimage-color-profile.h"
#include "core/ligmaitem.h"

#include "ligmadisplay.h"
#include "ligmadisplayshell.h"
#include "ligmadisplayshell-title.h"
#include "ligmastatusbar.h"

#include "about.h"

#include "ligma-intl.h"


#define MAX_TITLE_BUF 512


static gboolean ligma_display_shell_update_title_idle (gpointer          data);
static gint     ligma_display_shell_format_title      (LigmaDisplayShell *display,
                                                      gchar            *title,
                                                      gint              title_len,
                                                      const gchar      *format);


/*  public functions  */

void
ligma_display_shell_title_update (LigmaDisplayShell *shell)
{
  g_return_if_fail (LIGMA_IS_DISPLAY_SHELL (shell));

  if (shell->title_idle_id)
    g_source_remove (shell->title_idle_id);

  shell->title_idle_id = g_idle_add (ligma_display_shell_update_title_idle,
                                     shell);
}


/*  private functions  */

static gboolean
ligma_display_shell_update_title_idle (gpointer data)
{
  LigmaDisplayShell *shell = LIGMA_DISPLAY_SHELL (data);

  shell->title_idle_id = 0;

  if (ligma_display_get_image (shell->display))
    {
      LigmaDisplayConfig *config = shell->display->config;
      gchar              title[MAX_TITLE_BUF];
      gchar              status[MAX_TITLE_BUF];
      gint               len;

      /* format the title */
      len = ligma_display_shell_format_title (shell, title, sizeof (title),
                                             config->image_title_format);

      if (len)  /* U+2013 EN DASH */
        len += g_strlcpy (title + len, " \342\200\223 ", sizeof (title) - len);

      g_strlcpy (title + len, LIGMA_ACRONYM, sizeof (title) - len);

      /* format the statusbar */
      ligma_display_shell_format_title (shell, status, sizeof (status),
                                       config->image_status_format);

      g_object_set (shell,
                    "title",  title,
                    "status", status,
                    NULL);
    }
  else
    {
      g_object_set (shell,
                    "title",  LIGMA_NAME,
                    "status", " ",
                    NULL);
    }

  return FALSE;
}

static const gchar *
ligma_display_shell_title_image_type (LigmaImage *image)
{
  const gchar *name = "";

  ligma_enum_get_value (LIGMA_TYPE_IMAGE_BASE_TYPE,
                       ligma_image_get_base_type (image), NULL, NULL, &name, NULL);

  return name;
}

static const gchar *
ligma_display_shell_title_image_precision (LigmaImage *image)
{
  const gchar *name = "";

  ligma_enum_get_value (LIGMA_TYPE_PRECISION,
                       ligma_image_get_precision (image), NULL, NULL, &name, NULL);

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
ligma_display_shell_format_title (LigmaDisplayShell *shell,
                                 gchar            *title,
                                 gint              title_len,
                                 const gchar      *format)
{
  LigmaImage    *image;
  LigmaDrawable *drawable = NULL;
  GList        *drawables;
  gint          num, denom;
  gint          i = 0;

  g_return_val_if_fail (LIGMA_IS_DISPLAY_SHELL (shell), 0);

  image = ligma_display_get_image (shell->display);

  if (! image)
    {
      title[0] = '\n';
      return 0;
    }

  /* LIGMA window title only take single selected layer into account so
   * far (not sure how we could have multi-layer concept there, except
   * wanting never-ending window titles!).
   * When multiple drawables are selected, we just display nothing.
   */
  drawables = ligma_image_get_selected_drawables (image);
  if (g_list_length (drawables) == 1)
    drawable = drawables->data;
  g_list_free (drawables);

  ligma_zoom_model_get_fraction (shell->zoom, &num, &denom);

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
                          ligma_image_get_display_name (image));
              break;

            case 'F': /* full filename */
              i += print (title, title_len, i, "%s",
                          ligma_image_get_display_path (image));
              break;

            case 'p': /* PDB id */
              i += print (title, title_len, i, "%d", ligma_image_get_id (image));
              break;

            case 'i': /* instance */
              i += print (title, title_len, i, "%d",
                          ligma_display_get_instance (shell->display));
              break;

            case 't': /* image type */
              i += print (title, title_len, i, "%s %s",
                          ligma_display_shell_title_image_type (image),
                          ligma_display_shell_title_image_precision (image));
              break;

            case 'T': /* drawable type */
              if (drawable)
                {
                  const Babl *format = ligma_drawable_get_format (drawable);

                  i += print (title, title_len, i, "%s",
                              ligma_babl_format_get_description (format));
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
                gdouble  scale = ligma_zoom_model_get_factor (shell->zoom);

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
              if (ligma_image_is_dirty (image))
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
              if (! ligma_image_is_dirty (image))
                title[i++] = format[1];
              format++;
              break;

            case 'B': /* dirty flag (long) */
              if (ligma_image_is_dirty (image))
                i += print (title, title_len, i, "%s", _("(modified)"));
              break;

            case 'A': /* clean flag (long) */
              if (! ligma_image_is_dirty (image))
                i += print (title, title_len, i, "%s", _("(clean)"));
              break;

            case 'N': /* not-exported flag */
              if (format[1] == 0)
                {
                  /* format string ends within %E-sequence, print literal '%E' */
                  i += print (title, title_len, i, "%%N");
                  break;
                }
              if (ligma_image_is_export_dirty (image))
                title[i++] = format[1];
              format++;
              break;

            case 'E': /* exported flag */
              if (format[1] == 0)
                {
                  /* format string ends within %E-sequence, print literal '%E' */
                  i += print (title, title_len, i, "%%E");
                  break;
                }
              if (! ligma_image_is_export_dirty (image))
                title[i++] = format[1];
              format++;
              break;

            case 'm': /* memory used by image */
              {
                LigmaObject *object = LIGMA_OBJECT (image);
                gchar      *str;

                str = g_format_size (ligma_object_get_memsize (object, NULL));
                i += print (title, title_len, i, "%s", str);
                g_free (str);
              }
              break;

            case 'M': /* image size in megapixels */
              i += print (title, title_len, i, "%.1f",
                          (gdouble) ligma_image_get_width (image) *
                          (gdouble) ligma_image_get_height (image) / 1000000.0);
              break;

            case 'l': /* number of layers */
              i += print (title, title_len, i, "%d",
                          ligma_image_get_n_layers (image));
              break;

            case 'L': /* number of layers (long) */
              {
                gint num = ligma_image_get_n_layers (image);

                i += print (title, title_len, i,
                            ngettext ("%d layer", "%d layers", num), num);
              }
              break;

            case 'n': /* active drawable name */
              if (drawable)
                {
                  gchar *desc;

                  desc = ligma_viewable_get_description (LIGMA_VIEWABLE (drawable),
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
                            ligma_item_get_id (LIGMA_ITEM (drawable)));
              else
                i += print (title, title_len, i, "%s", _("(none)"));
              break;

            case 'W': /* width in real-world units */
              if (shell->unit != LIGMA_UNIT_PIXEL)
                {
                  gdouble xres;
                  gdouble yres;
                  gchar   unit_format[8];

                  ligma_image_get_resolution (image, &xres, &yres);

                  g_snprintf (unit_format, sizeof (unit_format), "%%.%df",
                              ligma_unit_get_scaled_digits (shell->unit, xres));
                  i += print (title, title_len, i, unit_format,
                              ligma_pixels_to_units (ligma_image_get_width (image),
                                                    shell->unit, xres));
                  break;
                }
              /* else fallthru */

            case 'w': /* width in pixels */
              i += print (title, title_len, i, "%d",
                          ligma_image_get_width (image));
              break;

            case 'H': /* height in real-world units */
              if (shell->unit != LIGMA_UNIT_PIXEL)
                {
                  gdouble xres;
                  gdouble yres;
                  gchar   unit_format[8];

                  ligma_image_get_resolution (image, &xres, &yres);

                  g_snprintf (unit_format, sizeof (unit_format), "%%.%df",
                              ligma_unit_get_scaled_digits (shell->unit, yres));
                  i += print (title, title_len, i, unit_format,
                              ligma_pixels_to_units (ligma_image_get_height (image),
                                                    shell->unit, yres));
                  break;
                }
              /* else fallthru */

            case 'h': /* height in pixels */
              i += print (title, title_len, i, "%d",
                          ligma_image_get_height (image));
              break;

            case 'u': /* unit symbol */
              i += print (title, title_len, i, "%s",
                          ligma_unit_get_symbol (shell->unit));
              break;

            case 'U': /* unit abbreviation */
              i += print (title, title_len, i, "%s",
                          ligma_unit_get_abbreviation (shell->unit));
              break;

            case 'X': /* drawable width in real world units */
              if (drawable && shell->unit != LIGMA_UNIT_PIXEL)
                {
                  gdouble xres;
                  gdouble yres;
                  gchar   unit_format[8];

                  ligma_image_get_resolution (image, &xres, &yres);

                  g_snprintf (unit_format, sizeof (unit_format), "%%.%df",
                              ligma_unit_get_scaled_digits (shell->unit, xres));
                  i += print (title, title_len, i, unit_format,
                              ligma_pixels_to_units (ligma_item_get_width
                                                    (LIGMA_ITEM (drawable)),
                                                    shell->unit, xres));
                  break;
                }
              /* else fallthru */

            case 'x': /* drawable width in pixels */
              if (drawable)
                i += print (title, title_len, i, "%d",
                            ligma_item_get_width (LIGMA_ITEM (drawable)));
              break;

            case 'Y': /* drawable height in real world units */
              if (drawable && shell->unit != LIGMA_UNIT_PIXEL)
                {
                  gdouble xres;
                  gdouble yres;
                  gchar   unit_format[8];

                  ligma_image_get_resolution (image, &xres, &yres);

                  g_snprintf (unit_format, sizeof (unit_format), "%%.%df",
                              ligma_unit_get_scaled_digits (shell->unit, yres));
                  i += print (title, title_len, i, unit_format,
                              ligma_pixels_to_units (ligma_item_get_height
                                                    (LIGMA_ITEM (drawable)),
                                                    shell->unit, yres));
                  break;
                }
              /* else fallthru */

            case 'y': /* drawable height in pixels */
              if (drawable)
                i += print (title, title_len, i, "%d",
                            ligma_item_get_height (LIGMA_ITEM (drawable)));
              break;

            case 'o': /* image's color profile name */
              {
                LigmaColorManaged *managed = LIGMA_COLOR_MANAGED (image);
                LigmaColorProfile *profile;

                profile = ligma_color_managed_get_color_profile (managed);

                i += print (title, title_len, i, "%s",
                            ligma_color_profile_get_label (profile));
              }
              break;

            case 'e': /* display's offsets in pixels */
              {
                gdouble scale    = ligma_zoom_model_get_factor (shell->zoom);
                gdouble offset_x = shell->offset_x / scale;
                gdouble offset_y = shell->offset_y / scale;

                i += print (title, title_len, i,
                            scale >= 0.15 ? "%.0fx%.0f" : "%.2fx%.2f",
                            offset_x, offset_y);
              }
              break;

            case 'r': /* view rotation angle in degrees */
              {
                i += print (title, title_len, i, "%.1f", shell->rotate_angle);
              }
              break;

            case '\xc3': /* utf-8 extended char */
              {
                format ++;
                switch (*format)
                  {
                  case '\xbe':
                    /* line actually written at 23:55 on an Easter Sunday */
                    i += print (title, title_len, i, "42");
                    break;

                  default:
                    /* in the case of an unhandled utf-8 extended char format
                     * leave the format string parsing as it was
                    */
                    format--;
                    break;
                  }
              }
              break;

              /* Other cool things to be added:
               * %r = xresolution
               * %R = yresolution
               * %ø = image's fractal dimension
               * %þ = the answer to everything - (implemented)
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
