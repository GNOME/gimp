/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-2002 Spencer Kimball, Peter Mattis, and others
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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libligmabase/ligmabase.h"
#include "libligmaconfig/ligmaconfig.h"

#include "core-types.h"

#include "config/ligmarc.h"

#include "ligma.h"
#include "ligma-data-factories.h"
#include "ligma-gradients.h"
#include "ligma-memsize.h"
#include "ligma-palettes.h"
#include "ligmacontainer.h"
#include "ligmabrush-load.h"
#include "ligmabrush.h"
#include "ligmabrushclipboard.h"
#include "ligmabrushgenerated-load.h"
#include "ligmabrushpipe-load.h"
#include "ligmadataloaderfactory.h"
#include "ligmadynamics.h"
#include "ligmadynamics-load.h"
#include "ligmagradient-load.h"
#include "ligmagradient.h"
#include "ligmamybrush-load.h"
#include "ligmamybrush.h"
#include "ligmapalette-load.h"
#include "ligmapalette.h"
#include "ligmapattern-load.h"
#include "ligmapattern.h"
#include "ligmapatternclipboard.h"
#include "ligmatagcache.h"
#include "ligmatoolpreset.h"
#include "ligmatoolpreset-load.h"

#include "text/ligmafontfactory.h"

#include "ligma-intl.h"


void
ligma_data_factories_init (Ligma *ligma)
{
  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  ligma->brush_factory =
    ligma_data_loader_factory_new (ligma,
                                  LIGMA_TYPE_BRUSH,
                                  "brush-path",
                                  "brush-path-writable",
                                  "brush-paths",
                                  ligma_brush_new,
                                  ligma_brush_get_standard);
  ligma_object_set_static_name (LIGMA_OBJECT (ligma->brush_factory),
                               "brush factory");
  ligma_data_loader_factory_add_loader (ligma->brush_factory,
                                       "LIGMA Brush",
                                       ligma_brush_load,
                                       LIGMA_BRUSH_FILE_EXTENSION,
                                       TRUE);
  ligma_data_loader_factory_add_loader (ligma->brush_factory,
                                       "LIGMA Brush Pixmap",
                                       ligma_brush_load,
                                       LIGMA_BRUSH_PIXMAP_FILE_EXTENSION,
                                       FALSE);
  ligma_data_loader_factory_add_loader (ligma->brush_factory,
                                       "Photoshop ABR Brush",
                                       ligma_brush_load_abr,
                                       LIGMA_BRUSH_PS_FILE_EXTENSION,
                                       FALSE);
  ligma_data_loader_factory_add_loader (ligma->brush_factory,
                                       "Paint Shop Pro JBR Brush",
                                       ligma_brush_load_abr,
                                       LIGMA_BRUSH_PSP_FILE_EXTENSION,
                                       FALSE);
 ligma_data_loader_factory_add_loader (ligma->brush_factory,
                                       "LIGMA Generated Brush",
                                       ligma_brush_generated_load,
                                       LIGMA_BRUSH_GENERATED_FILE_EXTENSION,
                                       TRUE);
  ligma_data_loader_factory_add_loader (ligma->brush_factory,
                                       "LIGMA Brush Pipe",
                                       ligma_brush_pipe_load,
                                       LIGMA_BRUSH_PIPE_FILE_EXTENSION,
                                       TRUE);

  ligma->dynamics_factory =
    ligma_data_loader_factory_new (ligma,
                                  LIGMA_TYPE_DYNAMICS,
                                  "dynamics-path",
                                  "dynamics-path-writable",
                                  "dynamics-paths",
                                  ligma_dynamics_new,
                                  ligma_dynamics_get_standard);
  ligma_object_set_static_name (LIGMA_OBJECT (ligma->dynamics_factory),
                               "dynamics factory");
  ligma_data_loader_factory_add_loader (ligma->dynamics_factory,
                                       "LIGMA Paint Dynamics",
                                       ligma_dynamics_load,
                                       LIGMA_DYNAMICS_FILE_EXTENSION,
                                       TRUE);

  ligma->mybrush_factory =
    ligma_data_loader_factory_new (ligma,
                                  LIGMA_TYPE_MYBRUSH,
                                  "mypaint-brush-path",
                                  "mypaint-brush-path-writable",
                                  "mypaint-brush-paths",
                                  NULL,
                                  NULL);
  ligma_object_set_static_name (LIGMA_OBJECT (ligma->mybrush_factory),
                               "mypaint brush factory");
  ligma_data_loader_factory_add_loader (ligma->mybrush_factory,
                                       "MyPaint Brush",
                                       ligma_mybrush_load,
                                       LIGMA_MYBRUSH_FILE_EXTENSION,
                                       FALSE);

  ligma->pattern_factory =
    ligma_data_loader_factory_new (ligma,
                                  LIGMA_TYPE_PATTERN,
                                  "pattern-path",
                                  "pattern-path-writable",
                                  "pattern-paths",
                                  NULL,
                                  ligma_pattern_get_standard);
  ligma_object_set_static_name (LIGMA_OBJECT (ligma->pattern_factory),
                               "pattern factory");
  ligma_data_loader_factory_add_loader (ligma->pattern_factory,
                                       "LIGMA Pattern",
                                       ligma_pattern_load,
                                       LIGMA_PATTERN_FILE_EXTENSION,
                                       TRUE);
  ligma_data_loader_factory_add_fallback (ligma->pattern_factory,
                                         "Pattern from GdkPixbuf",
                                         ligma_pattern_load_pixbuf);

  ligma->gradient_factory =
    ligma_data_loader_factory_new (ligma,
                                  LIGMA_TYPE_GRADIENT,
                                  "gradient-path",
                                  "gradient-path-writable",
                                  "gradient-paths",
                                  ligma_gradient_new,
                                  ligma_gradient_get_standard);
  ligma_object_set_static_name (LIGMA_OBJECT (ligma->gradient_factory),
                               "gradient factory");
  ligma_data_loader_factory_add_loader (ligma->gradient_factory,
                                       "LIGMA Gradient",
                                       ligma_gradient_load,
                                       LIGMA_GRADIENT_FILE_EXTENSION,
                                       TRUE);
  ligma_data_loader_factory_add_loader (ligma->gradient_factory,
                                       "SVG Gradient",
                                       ligma_gradient_load_svg,
                                       LIGMA_GRADIENT_SVG_FILE_EXTENSION,
                                       FALSE);

  ligma->palette_factory =
    ligma_data_loader_factory_new (ligma,
                                  LIGMA_TYPE_PALETTE,
                                  "palette-path",
                                  "palette-path-writable",
                                  "palette-paths",
                                  ligma_palette_new,
                                  ligma_palette_get_standard);
  ligma_object_set_static_name (LIGMA_OBJECT (ligma->palette_factory),
                               "palette factory");
  ligma_data_loader_factory_add_loader (ligma->palette_factory,
                                       "LIGMA Palette",
                                       ligma_palette_load,
                                       LIGMA_PALETTE_FILE_EXTENSION,
                                       TRUE);

  ligma->font_factory =
    ligma_font_factory_new (ligma,
                           "font-path");
  ligma_object_set_static_name (LIGMA_OBJECT (ligma->font_factory),
                               "font factory");

  ligma->tool_preset_factory =
    ligma_data_loader_factory_new (ligma,
                                  LIGMA_TYPE_TOOL_PRESET,
                                  "tool-preset-path",
                                  "tool-preset-path-writable",
                                  "tool-preset-paths",
                                  ligma_tool_preset_new,
                                  NULL);
  ligma_object_set_static_name (LIGMA_OBJECT (ligma->tool_preset_factory),
                               "tool preset factory");
  ligma_data_loader_factory_add_loader (ligma->tool_preset_factory,
                                       "LIGMA Tool Preset",
                                       ligma_tool_preset_load,
                                       LIGMA_TOOL_PRESET_FILE_EXTENSION,
                                       TRUE);

  ligma->tag_cache = ligma_tag_cache_new ();
}

void
ligma_data_factories_add_builtin (Ligma *ligma)
{
  LigmaData *data;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  /*  add the builtin FG -> BG etc. gradients  */
  ligma_gradients_init (ligma);

  /*  add the color history palette  */
  ligma_palettes_init (ligma);

  /*  add the clipboard brushes  */
  data = ligma_brush_clipboard_new (ligma, FALSE);
  ligma_data_make_internal (data, "ligma-brush-clipboard-image");
  ligma_container_add (ligma_data_factory_get_container (ligma->brush_factory),
                      LIGMA_OBJECT (data));
  g_object_unref (data);

  data = ligma_brush_clipboard_new (ligma, TRUE);
  ligma_data_make_internal (data, "ligma-brush-clipboard-mask");
  ligma_container_add (ligma_data_factory_get_container (ligma->brush_factory),
                      LIGMA_OBJECT (data));
  g_object_unref (data);

  /*  add the clipboard pattern  */
  data = ligma_pattern_clipboard_new (ligma);
  ligma_data_make_internal (data, "ligma-pattern-clipboard-image");
  ligma_container_add (ligma_data_factory_get_container (ligma->pattern_factory),
                      LIGMA_OBJECT (data));
  g_object_unref (data);
}

void
ligma_data_factories_clear (Ligma *ligma)
{
  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  if (ligma->brush_factory)
    ligma_data_factory_data_free (ligma->brush_factory);

  if (ligma->dynamics_factory)
    ligma_data_factory_data_free (ligma->dynamics_factory);

  if (ligma->mybrush_factory)
    ligma_data_factory_data_free (ligma->mybrush_factory);

  if (ligma->pattern_factory)
    ligma_data_factory_data_free (ligma->pattern_factory);

  if (ligma->gradient_factory)
    ligma_data_factory_data_free (ligma->gradient_factory);

  if (ligma->palette_factory)
    ligma_data_factory_data_free (ligma->palette_factory);

  if (ligma->font_factory)
    ligma_data_factory_data_free (ligma->font_factory);

  if (ligma->tool_preset_factory)
    ligma_data_factory_data_free (ligma->tool_preset_factory);
}

void
ligma_data_factories_exit (Ligma *ligma)
{
  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  g_clear_object (&ligma->brush_factory);
  g_clear_object (&ligma->dynamics_factory);
  g_clear_object (&ligma->mybrush_factory);
  g_clear_object (&ligma->pattern_factory);
  g_clear_object (&ligma->gradient_factory);
  g_clear_object (&ligma->palette_factory);
  g_clear_object (&ligma->font_factory);
  g_clear_object (&ligma->tool_preset_factory);
  g_clear_object (&ligma->tag_cache);
}

gint64
ligma_data_factories_get_memsize (Ligma   *ligma,
                                 gint64 *gui_size)
{
  gint64 memsize = 0;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), 0);

  memsize += ligma_object_get_memsize (LIGMA_OBJECT (ligma->named_buffers),
                                      gui_size);
  memsize += ligma_object_get_memsize (LIGMA_OBJECT (ligma->brush_factory),
                                      gui_size);
  memsize += ligma_object_get_memsize (LIGMA_OBJECT (ligma->dynamics_factory),
                                      gui_size);
  memsize += ligma_object_get_memsize (LIGMA_OBJECT (ligma->mybrush_factory),
                                      gui_size);
  memsize += ligma_object_get_memsize (LIGMA_OBJECT (ligma->pattern_factory),
                                      gui_size);
  memsize += ligma_object_get_memsize (LIGMA_OBJECT (ligma->gradient_factory),
                                      gui_size);
  memsize += ligma_object_get_memsize (LIGMA_OBJECT (ligma->palette_factory),
                                      gui_size);
  memsize += ligma_object_get_memsize (LIGMA_OBJECT (ligma->font_factory),
                                      gui_size);
  memsize += ligma_object_get_memsize (LIGMA_OBJECT (ligma->tool_preset_factory),
                                      gui_size);

  memsize += ligma_object_get_memsize (LIGMA_OBJECT (ligma->tag_cache),
                                      gui_size);

  return memsize;
}

void
ligma_data_factories_data_clean (Ligma *ligma)
{
  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  ligma_data_factory_data_clean (ligma->brush_factory);
  ligma_data_factory_data_clean (ligma->dynamics_factory);
  ligma_data_factory_data_clean (ligma->mybrush_factory);
  ligma_data_factory_data_clean (ligma->pattern_factory);
  ligma_data_factory_data_clean (ligma->gradient_factory);
  ligma_data_factory_data_clean (ligma->palette_factory);
  ligma_data_factory_data_clean (ligma->font_factory);
  ligma_data_factory_data_clean (ligma->tool_preset_factory);
}

void
ligma_data_factories_load (Ligma               *ligma,
                          LigmaInitStatusFunc  status_callback)
{
  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  /*  initialize the list of ligma brushes    */
  status_callback (NULL, _("Brushes"), 0.1);
  ligma_data_factory_data_init (ligma->brush_factory, ligma->user_context,
                               ligma->no_data);

  /*  initialize the list of ligma dynamics   */
  status_callback (NULL, _("Dynamics"), 0.15);
  ligma_data_factory_data_init (ligma->dynamics_factory, ligma->user_context,
                               ligma->no_data);

  /*  initialize the list of mypaint brushes    */
  status_callback (NULL, _("MyPaint Brushes"), 0.2);
  ligma_data_factory_data_init (ligma->mybrush_factory, ligma->user_context,
                               ligma->no_data);

  /*  initialize the list of ligma patterns   */
  status_callback (NULL, _("Patterns"), 0.3);
  ligma_data_factory_data_init (ligma->pattern_factory, ligma->user_context,
                               ligma->no_data);

  /*  initialize the list of ligma palettes   */
  status_callback (NULL, _("Palettes"), 0.4);
  ligma_data_factory_data_init (ligma->palette_factory, ligma->user_context,
                               ligma->no_data);

  /*  initialize the list of ligma gradients  */
  status_callback (NULL, _("Gradients"), 0.5);
  ligma_data_factory_data_init (ligma->gradient_factory, ligma->user_context,
                               ligma->no_data);

  /*  initialize the color history   */
  status_callback (NULL, _("Color History"), 0.55);
  ligma_palettes_load (ligma);

  /*  initialize the list of ligma fonts   */
  status_callback (NULL, _("Fonts"), 0.6);
  ligma_data_factory_data_init (ligma->font_factory, ligma->user_context,
                               ligma->no_fonts);

  /*  initialize the list of ligma tool presets if we have a GUI  */
  if (! ligma->no_interface)
    {
      status_callback (NULL, _("Tool Presets"), 0.7);
      ligma_data_factory_data_init (ligma->tool_preset_factory, ligma->user_context,
                                   ligma->no_data);
    }

  /* update tag cache */
  status_callback (NULL, _("Updating tag cache"), 0.75);
  ligma_tag_cache_load (ligma->tag_cache);
  ligma_tag_cache_add_container (ligma->tag_cache,
                                ligma_data_factory_get_container (ligma->brush_factory));
  ligma_tag_cache_add_container (ligma->tag_cache,
                                ligma_data_factory_get_container (ligma->dynamics_factory));
  ligma_tag_cache_add_container (ligma->tag_cache,
                                ligma_data_factory_get_container (ligma->mybrush_factory));
  ligma_tag_cache_add_container (ligma->tag_cache,
                                ligma_data_factory_get_container (ligma->pattern_factory));
  ligma_tag_cache_add_container (ligma->tag_cache,
                                ligma_data_factory_get_container (ligma->gradient_factory));
  ligma_tag_cache_add_container (ligma->tag_cache,
                                ligma_data_factory_get_container (ligma->palette_factory));
  ligma_tag_cache_add_container (ligma->tag_cache,
                                ligma_data_factory_get_container (ligma->font_factory));
  ligma_tag_cache_add_container (ligma->tag_cache,
                                ligma_data_factory_get_container (ligma->tool_preset_factory));
}

void
ligma_data_factories_save (Ligma *ligma)
{
  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  ligma_tag_cache_save (ligma->tag_cache);

  ligma_data_factory_data_save (ligma->brush_factory);
  ligma_data_factory_data_save (ligma->dynamics_factory);
  ligma_data_factory_data_save (ligma->mybrush_factory);
  ligma_data_factory_data_save (ligma->pattern_factory);
  ligma_data_factory_data_save (ligma->gradient_factory);
  ligma_data_factory_data_save (ligma->palette_factory);
  ligma_data_factory_data_save (ligma->font_factory);
  ligma_data_factory_data_save (ligma->tool_preset_factory);

  ligma_palettes_save (ligma);
}
