/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpstock.c
 * Copyright (C) 2001 Michael Natterer <mitch@gimp.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <gtk/gtk.h>
#include <gtk/gtkiconfactory.h>

#include "gimpstock.h"

#include "themes/Default/gimp-stock-pixbufs.h"

#include "libgimp/libgimp-intl.h"


static GtkIconFactory *gimp_stock_factory = NULL;


static GtkIconSet *
sized_with_same_fallback_icon_set_from_inline (const guchar *inline_data,
					       GtkIconSize   size)
{
  GtkIconSource *source;
  GtkIconSet    *set;
  GdkPixbuf     *pixbuf;

  source = gtk_icon_source_new ();

  gtk_icon_source_set_size (source, size);
  gtk_icon_source_set_size_wildcarded (source, FALSE);

  pixbuf = gdk_pixbuf_new_from_inline (-1, inline_data, FALSE, NULL);

  g_assert (pixbuf);

  gtk_icon_source_set_pixbuf (source, pixbuf);

  gdk_pixbuf_unref (pixbuf);

  set = gtk_icon_set_new ();

  gtk_icon_set_add_source (set, source);

  gtk_icon_source_set_size_wildcarded (source, TRUE);
  gtk_icon_set_add_source (set, source);

  gtk_icon_source_free (source);

  return set;
}

static void
add_sized_with_same_fallback (GtkIconFactory *factory,
			      const guchar   *inline_data,
			      GtkIconSize     size,
			      const gchar    *stock_id)
{
  GtkIconSet *set;
  
  set = sized_with_same_fallback_icon_set_from_inline (inline_data, size);
  
  gtk_icon_factory_add (factory, stock_id, set);

  gtk_icon_set_unref (set);
}

static GtkStockItem gimp_stock_items[] =
{
  { GIMP_STOCK_ANCHOR,       N_("Anchor"),            0, 0, "gimp-libgimp" },
  { GIMP_STOCK_DELETE,       N_("_Delete"),           0, 0, "gimp-libgimp" },
  { GIMP_STOCK_DUPLICATE,    N_("Duplicate"),         0, 0, "gimp-libgimp" },
  { GIMP_STOCK_EDIT,         N_("Edit"),              0, 0, "gimp-libgimp" },
  { GIMP_STOCK_LINKED,       N_("Linked"),            0, 0, "gimp-libgimp" },
  { GIMP_STOCK_LOWER,        N_("Lower"),             0, 0, "gimp-libgimp" },
  { GIMP_STOCK_NEW,          N_("New"),               0, 0, "gimp-libgimp" },
  { GIMP_STOCK_PASTE,        N_("Paste"),             0, 0, "gimp-libgimp" },
  { GIMP_STOCK_PASTE_AS_NEW, N_("Paste as New"),      0, 0, "gimp-libgimp" },
  { GIMP_STOCK_PASTE_INTO,   N_("Paste Into"),        0, 0, "gimp-libgimp" },
  { GIMP_STOCK_RAISE,        N_("Raise"),             0, 0, "gimp-libgimp" },
  { GIMP_STOCK_REFRESH,      N_("Refresh"),           0, 0, "gimp-libgimp" },
  { GIMP_STOCK_RESET,        N_("_Reset"),            0, 0, "gimp-libgimp" },
  { GIMP_STOCK_STROKE,       N_("_Stroke"),           0, 0, "gimp-libgimp" },
  { GIMP_STOCK_TO_PATH,      N_("Selection To Path"), 0, 0, "gimp-libgimp" },
  { GIMP_STOCK_TO_SELECTION, N_("To Sleection"),      0, 0, "gimp-libgimp" },
  { GIMP_STOCK_VISIBLE,      N_("Visible"),           0, 0, "gimp-libgimp" }
};

static struct
{
  const gchar   *stock_id;
  gconstpointer  inline_data;
}
gimp_stock_pixbufs[] =
{
  { GIMP_STOCK_ANCHOR,                   stock_button_anchor                   },
  { GIMP_STOCK_DELETE,                   stock_button_delete                   },
  { GIMP_STOCK_DUPLICATE,                stock_button_duplicate                },
  { GIMP_STOCK_EDIT,                     stock_button_edit                     },
  { GIMP_STOCK_LINKED,                   stock_button_linked                   },
  { GIMP_STOCK_LOWER,                    stock_button_lower                    },
  { GIMP_STOCK_NEW,                      stock_button_new                      },
  { GIMP_STOCK_PASTE,                    stock_button_paste                    },
  { GIMP_STOCK_PASTE_AS_NEW,             stock_button_paste_as_new             },
  { GIMP_STOCK_PASTE_INTO,               stock_button_paste_into               },
  { GIMP_STOCK_RAISE,                    stock_button_raise                    },
  { GIMP_STOCK_REFRESH,                  stock_button_refresh                  },
  { GIMP_STOCK_RESET,                    stock_button_refresh                  },
  { GIMP_STOCK_STROKE,                   stock_button_stroke                   },
  { GIMP_STOCK_TO_PATH,                  stock_button_to_path                  },
  { GIMP_STOCK_TO_SELECTION,             stock_button_to_selection             },
  { GIMP_STOCK_VISIBLE,                  stock_button_eye                      },

  { GIMP_STOCK_TOOL_AIRBRUSH,            stock_tool_button_airbrush            },
  { GIMP_STOCK_TOOL_BEZIER_SELECT,       stock_tool_button_bezier_select       },
  { GIMP_STOCK_TOOL_BLEND,               stock_tool_button_blend               },
  { GIMP_STOCK_TOOL_BLUR,                stock_tool_button_blur                },
  { GIMP_STOCK_TOOL_BRIGHTNESS_CONTRAST, stock_tool_button_brightness_contrast },
  { GIMP_STOCK_TOOL_BUCKET_FILL,         stock_tool_button_bucket_fill         },
  { GIMP_STOCK_TOOL_BY_COLOR_SELECT,     stock_tool_button_by_color_select     },
  { GIMP_STOCK_TOOL_CLONE,               stock_tool_button_clone               },
  { GIMP_STOCK_TOOL_COLOR_BALANCE,       stock_tool_button_color_balance       },
  { GIMP_STOCK_TOOL_COLOR_PICKER,        stock_tool_button_color_picker        },
  { GIMP_STOCK_TOOL_CROP,                stock_tool_button_crop                },
  { GIMP_STOCK_TOOL_CURVES,              stock_tool_button_curves              },
  { GIMP_STOCK_TOOL_DODGE,               stock_tool_button_dodge               },
  { GIMP_STOCK_TOOL_ELLIPSE_SELECT,      stock_tool_button_ellipse_select      },
  { GIMP_STOCK_TOOL_ERASER,              stock_tool_button_eraser              },
  { GIMP_STOCK_TOOL_FLIP,                stock_tool_button_flip                },
  { GIMP_STOCK_TOOL_FREE_SELECT,         stock_tool_button_free_select         },
  { GIMP_STOCK_TOOL_FUZZY_SELECT,        stock_tool_button_fuzzy_select        },
  { GIMP_STOCK_TOOL_HISTOGRAM,           stock_tool_button_histogram           },
  { GIMP_STOCK_TOOL_HUE_SATURATION,      stock_tool_button_hue_saturation      },
  { GIMP_STOCK_TOOL_INK,                 stock_tool_button_ink                 },
  { GIMP_STOCK_TOOL_ISCISSORS,           stock_tool_button_iscissors           },
  { GIMP_STOCK_TOOL_LEVELS,              stock_tool_button_levels              },
  { GIMP_STOCK_TOOL_MEASURE,             stock_tool_button_measure             },
  { GIMP_STOCK_TOOL_MOVE,                stock_tool_button_move                },
  { GIMP_STOCK_TOOL_PAINTBRUSH,          stock_tool_button_paintbrush          },
  { GIMP_STOCK_TOOL_PATH,                stock_tool_button_path                },
  { GIMP_STOCK_TOOL_PENCIL,              stock_tool_button_pencil              },
  { GIMP_STOCK_TOOL_PERSPECTIVE,         stock_tool_button_perspective         },
  { GIMP_STOCK_TOOL_POSTERIZE,           stock_tool_button_posterize           },
  { GIMP_STOCK_TOOL_RECT_SELECT,         stock_tool_button_rect_select         },
  { GIMP_STOCK_TOOL_ROTATE,              stock_tool_button_rotate              },
  { GIMP_STOCK_TOOL_SCALE,               stock_tool_button_scale               },
  { GIMP_STOCK_TOOL_SHEAR,               stock_tool_button_shear               },
  { GIMP_STOCK_TOOL_SMUDGE,              stock_tool_button_smudge              },
  { GIMP_STOCK_TOOL_TEXT,                stock_tool_button_text                },
  { GIMP_STOCK_TOOL_THRESHOLD,           stock_tool_button_threshold           },
  { GIMP_STOCK_TOOL_ZOOM,                stock_tool_button_zoom                }
};

void
gimp_stock_init (void)
{
  static gboolean initialized = FALSE;

  gint i;

  if (initialized)
    return;

  gimp_stock_factory = gtk_icon_factory_new ();

  for (i = 0; i < G_N_ELEMENTS (gimp_stock_pixbufs); i++)
    {
      add_sized_with_same_fallback (gimp_stock_factory,
				    gimp_stock_pixbufs[i].inline_data,
				    GTK_ICON_SIZE_BUTTON,
				    gimp_stock_pixbufs[i].stock_id);
    }

  gtk_icon_factory_add_default (gimp_stock_factory);

  gtk_stock_add_static (gimp_stock_items, G_N_ELEMENTS (gimp_stock_items));

  initialized = TRUE;
}
