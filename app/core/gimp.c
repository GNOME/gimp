/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
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

#include <string.h> /* strlen */

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"

#include "core-types.h"
#include "pdb/pdb-types.h"

#include "base/tile-manager.h"

#include "pdb/procedural_db.h"

#include "gimp.h"
#include "gimpbrush.h"
#include "gimpbrushgenerated.h"
#include "gimpbrushpipe.h"
#include "gimpbuffer.h"
#include "gimpcontext.h"
#include "gimpcoreconfig.h"
#include "gimpdatafactory.h"
#include "gimpdocuments.h"
#include "gimpgradient.h"
#include "gimpimage.h"
#include "gimpimage-new.h"
#include "gimpimagefile.h"
#include "gimplist.h"
#include "gimppalette.h"
#include "gimppattern.h"
#include "gimpparasite.h"
#include "gimptoolinfo.h"
#include "gimpunit.h"

#include "app_procs.h"

#include "libgimp/gimpintl.h"


static void   gimp_class_init                  (GimpClass   *klass);
static void   gimp_init                        (Gimp        *gimp);

static void   gimp_dispose                     (GObject     *object);
static void   gimp_finalize                    (GObject     *object);

static void   gimp_context_disconnect_callback (GimpContext *context,
						Gimp        *gimp);


static GimpObjectClass *parent_class = NULL;


GType 
gimp_get_type (void)
{
  static GType object_type = 0;

  if (! object_type)
    {
      static const GTypeInfo object_info =
      {
        sizeof (GimpClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data     */
	sizeof (Gimp),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_init,
      };

      object_type = g_type_register_static (GIMP_TYPE_OBJECT,
					    "Gimp", 
					    &object_info, 0);
    }

  return object_type;
}

static void
gimp_class_init (GimpClass *klass)
{
  GObjectClass *object_class;

  object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->dispose  = gimp_dispose;
  object_class->finalize = gimp_finalize;
}

static void
gimp_init (Gimp *gimp)
{
  gimp_core_config_init (gimp);

  gimp->create_display_func = NULL;
  gimp->gui_set_busy_func   = NULL;
  gimp->gui_unset_busy_func = NULL;

  gimp->busy                = FALSE;
  gimp->busy_idle_id        = 0;

  gimp_units_init (gimp);

  gimp_parasites_init (gimp);

  gimp->images              = gimp_list_new (GIMP_TYPE_IMAGE,
					     GIMP_CONTAINER_POLICY_WEAK);
  gimp->next_image_ID       = 1;
  gimp->next_guide_ID       = 1;
  gimp->image_table         = g_hash_table_new (g_direct_hash, NULL);

  gimp->next_drawable_ID    = 1;
  gimp->drawable_table      = g_hash_table_new (g_direct_hash, NULL);

  gimp->global_buffer       = NULL;
  gimp->named_buffers       = gimp_list_new (GIMP_TYPE_BUFFER,
					     GIMP_CONTAINER_POLICY_STRONG);
  gimp->brush_factory       = NULL;
  gimp->pattern_factory     = NULL;
  gimp->gradient_factory    = NULL;
  gimp->palette_factory     = NULL;

  procedural_db_init (gimp);

  gimp->tool_info_list      = gimp_list_new (GIMP_TYPE_TOOL_INFO,
					     GIMP_CONTAINER_POLICY_STRONG);
  gimp->standard_tool_info  = NULL;

  gimp_documents_init (gimp);

  gimp->image_base_type_names   = NULL;
  gimp->fill_type_names         = NULL;
  gimp->have_current_cut_buffer = FALSE;

  gimp->context_list            = NULL;
  gimp->standard_context        = NULL;
  gimp->default_context         = NULL;
  gimp->user_context            = NULL;
  gimp->current_context         = NULL;
}

static void
gimp_dispose (GObject *object)
{
  Gimp *gimp;

  gimp = GIMP (object);

  if (gimp->brush_factory)
    gimp_data_factory_data_free (gimp->brush_factory);

  if (gimp->pattern_factory)
    gimp_data_factory_data_free (gimp->pattern_factory);

  if (gimp->gradient_factory)
    gimp_data_factory_data_free (gimp->gradient_factory);

  if (gimp->palette_factory)
    gimp_data_factory_data_free (gimp->palette_factory);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_finalize (GObject *object)
{
  Gimp *gimp;

  gimp = GIMP (object);

  if (gimp->current_context)
    gimp_set_current_context (gimp, NULL);

  if (gimp->user_context)
    gimp_set_user_context (gimp, NULL);

  if (gimp->default_context)
    gimp_set_default_context (gimp, NULL);

  if (gimp->standard_context)
    {
      g_object_unref (G_OBJECT (gimp->standard_context));
      gimp->standard_context = NULL;
    }

  if (gimp->image_base_type_names)
    gimp_image_new_exit (gimp);

  if (gimp->documents)
    gimp_documents_exit (gimp);

  gimp_tool_info_set_standard (gimp, NULL);

  if (gimp->tool_info_list)
    {
      g_object_unref (G_OBJECT (gimp->tool_info_list));
      gimp->tool_info_list = NULL;
    }

  if (gimp->procedural_ht)
    procedural_db_free (gimp);

  if (gimp->brush_factory)
    {
      g_object_unref (G_OBJECT (gimp->brush_factory));
      gimp->brush_factory = NULL;
    }

  if (gimp->pattern_factory)
    {
      g_object_unref (G_OBJECT (gimp->pattern_factory));
      gimp->pattern_factory = NULL;
    }

  if (gimp->gradient_factory)
    {
      g_object_unref (G_OBJECT (gimp->gradient_factory));
      gimp->gradient_factory = NULL;
    }

  if (gimp->palette_factory)
    {
      g_object_unref (G_OBJECT (gimp->palette_factory));
      gimp->palette_factory = NULL;
    }

  if (gimp->named_buffers)
    {
      g_object_unref (G_OBJECT (gimp->named_buffers));
      gimp->named_buffers = NULL;
    }

  if (gimp->global_buffer)
    {
      tile_manager_destroy (gimp->global_buffer);
      gimp->global_buffer = NULL;
    }

  if (gimp->drawable_table)
    {
      g_hash_table_destroy (gimp->drawable_table);
      gimp->drawable_table = NULL;
    }

  if (gimp->image_table)
    {
      g_hash_table_destroy (gimp->image_table);
      gimp->image_table = NULL;
    }

  if (gimp->images)
    {
      g_object_unref (G_OBJECT (gimp->images));
      gimp->images = NULL;
    }

  if (gimp->parasites)
    gimp_parasites_exit (gimp);

  if (gimp->user_units)
    gimp_units_exit (gimp);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

Gimp *
gimp_new (void)
{
  Gimp *gimp;

  gimp = g_object_new (GIMP_TYPE_GIMP, NULL);

  return gimp;
}

void
gimp_initialize (Gimp *gimp)
{
  GimpContext *context;

  static const GimpDataFactoryLoaderEntry brush_loader_entries[] =
  {
    { gimp_brush_load,           GIMP_BRUSH_FILE_EXTENSION           },
    { gimp_brush_load,           GIMP_BRUSH_PIXMAP_FILE_EXTENSION    },
    { gimp_brush_generated_load, GIMP_BRUSH_GENERATED_FILE_EXTENSION },
    { gimp_brush_pipe_load,      GIMP_BRUSH_PIPE_FILE_EXTENSION      }
  };

  static const GimpDataFactoryLoaderEntry pattern_loader_entries[] =
  {
    { gimp_pattern_load, GIMP_PATTERN_FILE_EXTENSION }
  };

  static const GimpDataFactoryLoaderEntry gradient_loader_entries[] =
  {
    { gimp_gradient_load, GIMP_GRADIENT_FILE_EXTENSION },
    { gimp_gradient_load, NULL /* legacy loader */     }
  };

  static const GimpDataFactoryLoaderEntry palette_loader_entries[] =
  {
    { gimp_palette_load, GIMP_PALETTE_FILE_EXTENSION },
    { gimp_palette_load, NULL /* legacy loader */    }
  };

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  gimp->brush_factory =
    gimp_data_factory_new (GIMP_TYPE_BRUSH,
			   (const gchar **) &gimp->config->brush_path,
			   brush_loader_entries,
			   G_N_ELEMENTS (brush_loader_entries),
			   gimp_brush_new,
			   gimp_brush_get_standard);

  gimp->pattern_factory =
    gimp_data_factory_new (GIMP_TYPE_PATTERN,
			   (const gchar **) &gimp->config->pattern_path,
			   pattern_loader_entries,
			   G_N_ELEMENTS (pattern_loader_entries),
			   gimp_pattern_new,
			   gimp_pattern_get_standard);

  gimp->gradient_factory =
    gimp_data_factory_new (GIMP_TYPE_GRADIENT,
			   (const gchar **) &gimp->config->gradient_path,
			   gradient_loader_entries,
			   G_N_ELEMENTS (gradient_loader_entries),
			   gimp_gradient_new,
			   gimp_gradient_get_standard);

  gimp->palette_factory =
    gimp_data_factory_new (GIMP_TYPE_PALETTE,
			   (const gchar **) &gimp->config->palette_path,
			   palette_loader_entries,
			   G_N_ELEMENTS (palette_loader_entries),
			   gimp_palette_new,
			   gimp_palette_get_standard);

  gimp_image_new_init (gimp);

  gimp->standard_context = gimp_create_context (gimp, "Standard", NULL);

  /*  the default context contains the user's saved preferences
   *
   *  TODO: load from disk
   */
  context = gimp_create_context (gimp, "Default", NULL);
  gimp_set_default_context (gimp, context);
  g_object_unref (G_OBJECT (context));

  /*  the initial user_context is a straight copy of the default context
   */
  context = gimp_create_context (gimp, "User", context);
  gimp_set_user_context (gimp, context);

  gimp_set_current_context (gimp, context);
  g_object_unref (G_OBJECT (context));
}

void
gimp_restore (Gimp     *gimp,
	      gboolean  no_data)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  /*  initialize  the global parasite table  */
  app_init_update_status (_("Looking for data files"), _("Parasites"), 0.00);
  gimp_parasiterc_load (gimp);

  /*  initialize the list of gimp brushes    */
  app_init_update_status (NULL, _("Brushes"), 0.18);
  gimp_data_factory_data_init (gimp->brush_factory, no_data); 

  /*  initialize the list of gimp patterns   */
  app_init_update_status (NULL, _("Patterns"), 0.36);
  gimp_data_factory_data_init (gimp->pattern_factory, no_data); 

  /*  initialize the list of gimp palettes   */
  app_init_update_status (NULL, _("Palettes"), 0.54);
  gimp_data_factory_data_init (gimp->palette_factory, no_data); 

  /*  initialize the list of gimp gradients  */
  app_init_update_status (NULL, _("Gradients"), 0.72);
  gimp_data_factory_data_init (gimp->gradient_factory, no_data); 

  /*  initialize  the global parasite table  */
  app_init_update_status (NULL, _("Documents"), 0.90);
  gimp_documents_load (gimp);

  app_init_update_status (NULL, NULL, 1.00);
}

void
gimp_shutdown (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  gimp_data_factory_data_save (gimp->brush_factory);
  gimp_data_factory_data_save (gimp->pattern_factory);
  gimp_data_factory_data_save (gimp->gradient_factory);
  gimp_data_factory_data_save (gimp->palette_factory);
  gimp_documents_save (gimp);
  gimp_parasiterc_save (gimp);
  gimp_unitrc_save (gimp);
}

void
gimp_set_busy (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  /* FIXME: gimp_busy HACK */
  gimp->busy = TRUE;

  if (gimp->gui_set_busy_func)
    gimp->gui_set_busy_func (gimp);
}

static gboolean
gimp_idle_unset_busy (gpointer data)
{
  Gimp *gimp;

  gimp = (Gimp *) data;

  gimp_unset_busy (gimp);

  gimp->busy_idle_id = 0;

  return FALSE;
}

void
gimp_set_busy_until_idle (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  if (! gimp->busy_idle_id)
    {
      gimp_set_busy (gimp);

      gimp->busy_idle_id = g_idle_add_full (G_PRIORITY_HIGH,
					    gimp_idle_unset_busy, gimp,
					    NULL);
    }
}

void
gimp_unset_busy (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  if (gimp->gui_unset_busy_func)
    gimp->gui_unset_busy_func (gimp);

  /* FIXME: gimp_busy HACK */
  gimp->busy = FALSE;
}

GimpImage *
gimp_create_image (Gimp              *gimp,
		   gint               width,
		   gint               height,
		   GimpImageBaseType  type,
		   gboolean           attach_comment)
{
  GimpImage *gimage;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  gimage = gimp_image_new (gimp, width, height, type);

  gimp_container_add (gimp->images, GIMP_OBJECT (gimage));

  if (attach_comment && gimp->config->default_comment)
    {
      GimpParasite *parasite;

      parasite = gimp_parasite_new ("gimp-comment",
				    GIMP_PARASITE_PERSISTENT,
				    strlen (gimp->config->default_comment) + 1,
				    gimp->config->default_comment);
      gimp_image_parasite_attach (gimage, parasite);
      gimp_parasite_free (parasite);
    }

  return gimage;
}

void
gimp_create_display (Gimp      *gimp,
		     GimpImage *gimage)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  if (gimp->create_display_func)
    {
      gimp->create_display_func (gimage);

      g_object_unref (G_OBJECT (gimage));
    }
}

/*
void
gimp_open_file (Gimp        *gimp,
		const gchar *filename,
		gboolean     with_display)
{
  GimpImage *gimage;
  gint       status;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (filename != NULL);

  gimage = file_open_image (gimp,
			    filename,
			    filename,
			    _("Open"),
			    NULL,
			    RUN_INTERACTIVE,
			    &status);

  if (gimage)
    {
      gchar *absolute;

      * enable & clear all undo steps *
      gimp_image_undo_enable (gimage);

      * set the image to clean  *
      gimp_image_clean_all (gimage);

      if (with_display)
	gimp_create_display (gimage->gimp, gimage);

      absolute = file_open_absolute_filename (filename);

      document_index_add (absolute);
      menus_last_opened_add (absolute);

      g_free (absolute);
    }
}
*/

GimpContext *
gimp_create_context (Gimp        *gimp,
		     const gchar *name,
		     GimpContext *template)
{
  GimpContext *context;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (!template || GIMP_IS_CONTEXT (template), NULL);

  /*  FIXME: need unique names here  */
  if (! name)
    name = "Unnamed";

  context = gimp_context_new (gimp, name, template);

  gimp->context_list = g_list_prepend (gimp->context_list, context);

  g_signal_connect_object (G_OBJECT (context), "disconnect",
			   G_CALLBACK (gimp_context_disconnect_callback),
			   G_OBJECT (gimp),
			   0);

  return context;
}

static void
gimp_context_disconnect_callback (GimpContext *context,
				  Gimp        *gimp)
{
  gimp->context_list = g_list_remove (gimp->context_list, context);
}

GimpContext *
gimp_get_standard_context (Gimp *gimp)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  return gimp->standard_context;
}

void
gimp_set_default_context (Gimp        *gimp,
			  GimpContext *context)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (! context || GIMP_IS_CONTEXT (context));

  if (gimp->default_context)
    {
      g_object_unref (G_OBJECT (gimp->default_context));
    }

  gimp->default_context = context;

  if (gimp->default_context)
    {
      g_object_ref (G_OBJECT (gimp->default_context));
    }
}

GimpContext *
gimp_get_default_context (Gimp *gimp)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  return gimp->default_context;
}

void
gimp_set_user_context (Gimp        *gimp,
		       GimpContext *context)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (! context || GIMP_IS_CONTEXT (context));

  if (gimp->user_context)
    {
      g_object_unref (G_OBJECT (gimp->user_context));
    }

  gimp->user_context = context;

  if (gimp->user_context)
    {
      g_object_ref (G_OBJECT (gimp->user_context));
    }
}

GimpContext *
gimp_get_user_context (Gimp *gimp)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  return gimp->user_context;
}

void
gimp_set_current_context (Gimp        *gimp,
			  GimpContext *context)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (! context || GIMP_IS_CONTEXT (context));

  if (gimp->current_context)
    {
      g_object_unref (G_OBJECT (gimp->current_context));
    }

  gimp->current_context = context;

  if (gimp->current_context)
    {
      g_object_ref (G_OBJECT (gimp->current_context));
    }
}

GimpContext *
gimp_get_current_context (Gimp *gimp)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  return gimp->current_context;
}
