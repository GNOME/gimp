/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "apptypes.h"

#include "core/gimpcontext.h"

#include "context_manager.h"
#include "gimptoolinfo.h"
#include "temp_buf.h"

#include "gimprectselecttool.h"


static void      gimp_tool_info_class_init      (GimpToolInfoClass *klass);
static void      gimp_tool_info_init            (GimpToolInfo      *tool_info);

static void      gimp_tool_info_destroy         (GtkObject         *object);
static TempBuf * gimp_tool_info_get_new_preview (GimpViewable      *viewable,
						 gint               width,
						 gint               height);


static GimpDataClass *parent_class = NULL;


GtkType
gimp_tool_info_get_type (void)
{
  static GtkType tool_info_type = 0;

  if (! tool_info_type)
    {
      GtkTypeInfo tool_info_info =
      {
        "GimpToolInfo",
        sizeof (GimpToolInfo),
        sizeof (GimpToolInfoClass),
        (GtkClassInitFunc) gimp_tool_info_class_init,
        (GtkObjectInitFunc) gimp_tool_info_init,
        /* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL,
      };

      tool_info_type = gtk_type_unique (GIMP_TYPE_DATA, &tool_info_info);
    }

  return tool_info_type;
}

static void
gimp_tool_info_class_init (GimpToolInfoClass *klass)
{
  GtkObjectClass    *object_class;
  GimpViewableClass *viewable_class;

  object_class   = (GtkObjectClass *) klass;
  viewable_class = (GimpViewableClass *) klass;

  parent_class = gtk_type_class (GIMP_TYPE_DATA);

  object_class->destroy = gimp_tool_info_destroy;

  viewable_class->get_new_preview = gimp_tool_info_get_new_preview;
}

void
gimp_tool_info_init (GimpToolInfo *tool_info)
{
  tool_info->tool_type    = GTK_TYPE_NONE;

  tool_info->blurb        = NULL;
  tool_info->help         = NULL;

  tool_info->menu_path    = NULL;
  tool_info->menu_accel   = NULL;

  tool_info->help_domain  = NULL;
  tool_info->help_data    = NULL;

  tool_info->icon_data    = NULL;

  tool_info->context      = NULL;

  tool_info->tool_options = NULL;
}

static void
gimp_tool_info_destroy (GtkObject *object)
{
  GimpToolInfo *tool_info;

  tool_info = (GimpToolInfo *) object;

  g_free (tool_info->blurb);
  g_free (tool_info->help);

  g_free (tool_info->menu_path);
  g_free (tool_info->menu_accel);

  g_free (tool_info->help_domain);
  g_free (tool_info->help_data);

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static TempBuf *
gimp_tool_info_get_new_preview (GimpViewable *viewable,
				gint          width,
				gint          height)
{
  GimpToolInfo *tool_info;
  TempBuf      *temp_buf;
  TempBuf      *return_buf;
  guchar        opaque[4] = { 0, 0, 0, 0 };
  gint          offset_x = 0;
  gint          offset_y = 0;
  gint          r, s, cnt;
  guchar        value;
  guchar       *data;
  guchar       *p;

  static gint colors[8][3] =
  {
    { 0x00, 0x00, 0x00 }, /* a -   0 */
    { 0x24, 0x24, 0x24 }, /* b -  36 */
    { 0x49, 0x49, 0x49 }, /* c -  73 */
    { 0x6D, 0x6D, 0x6D }, /* d - 109 */
    { 0x92, 0x92, 0x92 }, /* e - 146 */
    { 0xB6, 0xB6, 0xB6 }, /* f - 182 */
    { 0xDB, 0xDB, 0xDB }, /* g - 219 */
    { 0xFF, 0xFF, 0xFF }, /* h - 255 */
  };

  tool_info = GIMP_TOOL_INFO (viewable);

#define TOOL_INFO_WIDTH  22
#define TOOL_INFO_HEIGHT 22

  temp_buf = temp_buf_new (TOOL_INFO_WIDTH, TOOL_INFO_HEIGHT, 4, 0, 0, opaque);

  data = temp_buf_data (temp_buf);

  p = data;

  for (r = 0; r < TOOL_INFO_HEIGHT; r++)
    {
      for (s = 0, cnt = 0; s < TOOL_INFO_WIDTH; s++)
        {
          value = tool_info->icon_data[r][s];

          if (value != '.')
            {
              *p++ = colors[value - 'a'][0];
              *p++ = colors[value - 'a'][1];
              *p++ = colors[value - 'a'][2];
              *p++ = 255;
            }
	  else
	    {
	      p += 4;
	    }
	}
    }

  if (width > TOOL_INFO_WIDTH)
    offset_x = (width - TOOL_INFO_WIDTH) / 2;

  if (height > TOOL_INFO_HEIGHT)
    offset_y = (height - TOOL_INFO_HEIGHT) / 2;

  return_buf = temp_buf_scale (temp_buf, width, height);

  temp_buf_free (temp_buf);

  return return_buf;
}

GimpToolInfo *
gimp_tool_info_new (GtkType       tool_type,
                    gboolean      tool_context,
		    const gchar  *identifier,
		    const gchar  *blurb,
		    const gchar  *help,
		    const gchar  *menu_path,
		    const gchar  *menu_accel,
		    const gchar  *help_domain,
		    const gchar  *help_data,
		    const gchar **icon_data)
{
  GimpToolInfo *tool_info;

  tool_info = gtk_type_new (GIMP_TYPE_TOOL_INFO);

  gimp_object_set_name (GIMP_OBJECT (tool_info), identifier);

  tool_info->tool_type     = tool_type;

  tool_info->blurb         = g_strdup (blurb);
  tool_info->help          = g_strdup (help);

  tool_info->menu_path     = g_strdup (menu_path);
  tool_info->menu_accel    = g_strdup (menu_accel);

  tool_info->help_domain   = g_strdup (help_domain);
  tool_info->help_data     = g_strdup (help_data);

  tool_info->icon_data     = icon_data;

  if (tool_context)
    {
      tool_info->context = gimp_context_new (identifier,
                                             global_tool_context);
    }

  return tool_info;
}

GimpToolInfo *
gimp_tool_info_get_standard (void)
{
  static GimpToolInfo *standard_tool_info = NULL;

  if (! standard_tool_info)
    {
      standard_tool_info =
	gimp_tool_info_new (GIMP_TYPE_RECT_SELECT_TOOL,
                            FALSE,
			    "gimp:standard_tool",
			    "Standard Tool",
			    "Well something must be broken",
			    NULL, NULL,
			    NULL, NULL,
			    NULL);
    }

  return standard_tool_info;
}
