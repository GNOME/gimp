/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaprocedure.h
 * Copyright (C) 2019 Michael Natterer <mitch@ligma.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__LIGMA_H_INSIDE__) && !defined (LIGMA_COMPILATION)
#error "Only <libligma/ligma.h> can be included directly."
#endif

#ifndef __LIGMA_PROCEDURE_H__
#define __LIGMA_PROCEDURE_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


/**
 * LigmaRunFunc:
 * @procedure: the #LigmaProcedure that runs.
 * @args:      the @procedure's arguments.
 * @run_data: (closure): the run_data given in ligma_procedure_new().
 *
 * The run function is run during the lifetime of the LIGMA session,
 * each time a plug-in procedure is called.
 *
 * Returns: (transfer full): the @procedure's return values.
 *
 * Since: 3.0
 **/
typedef LigmaValueArray * (* LigmaRunFunc) (LigmaProcedure        *procedure,
                                          const LigmaValueArray *args,
                                          gpointer              run_data);


/**
 * LigmaArgumentSync:
 * @LIGMA_ARGUMENT_SYNC_NONE:     Don't sync this argument
 * @LIGMA_ARGUMENT_SYNC_PARASITE: Sync this argument with an image parasite
 *
 * Methods of syncing procedure arguments.
 *
 * Since: 3.0
 **/
typedef enum
{
  LIGMA_ARGUMENT_SYNC_NONE,
  LIGMA_ARGUMENT_SYNC_PARASITE
} LigmaArgumentSync;


#define LIGMA_TYPE_PROCEDURE            (ligma_procedure_get_type ())
#define LIGMA_PROCEDURE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_PROCEDURE, LigmaProcedure))
#define LIGMA_PROCEDURE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_PROCEDURE, LigmaProcedureClass))
#define LIGMA_IS_PROCEDURE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_PROCEDURE))
#define LIGMA_IS_PROCEDURE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_PROCEDURE))
#define LIGMA_PROCEDURE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_PROCEDURE, LigmaProcedureClass))


typedef struct _LigmaProcedureClass   LigmaProcedureClass;
typedef struct _LigmaProcedurePrivate LigmaProcedurePrivate;

struct _LigmaProcedure
{
  GObject               parent_instance;

  LigmaProcedurePrivate *priv;
};

/**
 * LigmaProcedureClass:
 * @install: called to install the procedure with the main LIGMA
 *   application. This is an implementation detail and must never
 *   be called by any plug-in code.
 * @uninstall: called to uninstall the procedure from the main LIGMA
 *   application. This is an implementation detail and must never
 *   be called by any plug-in code.
 * @run: called when the procedure is executed via ligma_procedure_run().
 *   the default implementation simply calls the procedure's #LigmaRunFunc,
 *   #LigmaProcedure subclasses are free to modify the passed @args and
 *   call their own, subclass-specific run functions.
 * @create_config: called when a #LigmaConfig object is created using
 *   ligma_procedure_create_config().
 *
 * Since: 3.0
 **/
struct _LigmaProcedureClass
{
  GObjectClass parent_class;

  void                  (* install)         (LigmaProcedure         *procedure);
  void                  (* uninstall)       (LigmaProcedure         *procedure);

  LigmaValueArray      * (* run)             (LigmaProcedure         *procedure,
                                             const LigmaValueArray  *args);

  LigmaProcedureConfig * (* create_config)   (LigmaProcedure         *procedure,
                                             GParamSpec           **args,
                                             gint                   n_args);

  gboolean              (* set_sensitivity) (LigmaProcedure         *procedure,
                                             gint                   sensitivity_mask);

  /* Padding for future expansion */
  /*< private >*/
  void (*_ligma_reserved1) (void);
  void (*_ligma_reserved2) (void);
  void (*_ligma_reserved3) (void);
  void (*_ligma_reserved4) (void);
  void (*_ligma_reserved5) (void);
  void (*_ligma_reserved6) (void);
  void (*_ligma_reserved7) (void);
  void (*_ligma_reserved8) (void);
};


GType            ligma_procedure_get_type           (void) G_GNUC_CONST;

LigmaProcedure  * ligma_procedure_new                (LigmaPlugIn           *plug_in,
                                                    const gchar          *name,
                                                    LigmaPDBProcType       proc_type,
                                                    LigmaRunFunc           run_func,
                                                    gpointer              run_data,
                                                    GDestroyNotify        run_data_destroy);

LigmaPlugIn     * ligma_procedure_get_plug_in        (LigmaProcedure        *procedure);
const gchar    * ligma_procedure_get_name           (LigmaProcedure        *procedure);
LigmaPDBProcType  ligma_procedure_get_proc_type      (LigmaProcedure        *procedure);

void             ligma_procedure_set_image_types    (LigmaProcedure        *procedure,
                                                    const gchar          *image_types);
const gchar    * ligma_procedure_get_image_types    (LigmaProcedure        *procedure);

void           ligma_procedure_set_sensitivity_mask (LigmaProcedure        *procedure,
                                                    gint                  sensitivity_mask);
gint           ligma_procedure_get_sensitivity_mask (LigmaProcedure        *procedure);


void             ligma_procedure_set_menu_label     (LigmaProcedure        *procedure,
                                                    const gchar          *menu_label);
const gchar    * ligma_procedure_get_menu_label     (LigmaProcedure        *procedure);

void             ligma_procedure_add_menu_path      (LigmaProcedure        *procedure,
                                                    const gchar          *menu_path);
GList          * ligma_procedure_get_menu_paths     (LigmaProcedure        *procedure);

void             ligma_procedure_set_icon_name      (LigmaProcedure        *procedure,
                                                    const gchar          *icon_name);
void             ligma_procedure_set_icon_file      (LigmaProcedure        *procedure,
                                                    GFile                *file);
void             ligma_procedure_set_icon_pixbuf    (LigmaProcedure        *procedure,
                                                    GdkPixbuf            *pixbuf);

LigmaIconType     ligma_procedure_get_icon_type      (LigmaProcedure        *procedure);
const gchar    * ligma_procedure_get_icon_name      (LigmaProcedure        *procedure);
GFile          * ligma_procedure_get_icon_file      (LigmaProcedure        *procedure);
GdkPixbuf      * ligma_procedure_get_icon_pixbuf    (LigmaProcedure        *procedure);

void             ligma_procedure_set_documentation  (LigmaProcedure        *procedure,
                                                    const gchar          *blurb,
                                                    const gchar          *help,
                                                    const gchar          *help_id);
const gchar    * ligma_procedure_get_blurb          (LigmaProcedure        *procedure);
const gchar    * ligma_procedure_get_help           (LigmaProcedure        *procedure);
const gchar    * ligma_procedure_get_help_id        (LigmaProcedure        *procedure);

void             ligma_procedure_set_attribution    (LigmaProcedure        *procedure,
                                                    const gchar          *authors,
                                                    const gchar          *copyright,
                                                    const gchar          *date);
const gchar    * ligma_procedure_get_authors        (LigmaProcedure        *procedure);
const gchar    * ligma_procedure_get_copyright      (LigmaProcedure        *procedure);
const gchar    * ligma_procedure_get_date           (LigmaProcedure        *procedure);

GParamSpec     * ligma_procedure_add_argument       (LigmaProcedure        *procedure,
                                                    GParamSpec           *pspec);
GParamSpec     * ligma_procedure_add_argument_from_property
                                                   (LigmaProcedure        *procedure,
                                                    GObject              *config,
                                                    const gchar          *prop_name);

GParamSpec     * ligma_procedure_add_aux_argument   (LigmaProcedure        *procedure,
                                                    GParamSpec           *pspec);
GParamSpec     * ligma_procedure_add_aux_argument_from_property
                                                   (LigmaProcedure        *procedure,
                                                    GObject              *config,
                                                    const gchar          *prop_name);

GParamSpec     * ligma_procedure_add_return_value   (LigmaProcedure        *procedure,
                                                    GParamSpec           *pspec);
GParamSpec     * ligma_procedure_add_return_value_from_property
                                                   (LigmaProcedure        *procedure,
                                                    GObject              *config,
                                                    const gchar          *prop_name);

GParamSpec     * ligma_procedure_find_argument      (LigmaProcedure        *procedure,
                                                    const gchar          *name);
GParamSpec     * ligma_procedure_find_aux_argument  (LigmaProcedure        *procedure,
                                                    const gchar          *name);
GParamSpec     * ligma_procedure_find_return_value  (LigmaProcedure        *procedure,
                                                    const gchar          *name);

GParamSpec    ** ligma_procedure_get_arguments      (LigmaProcedure        *procedure,
                                                    gint                 *n_arguments);
GParamSpec    ** ligma_procedure_get_aux_arguments  (LigmaProcedure        *procedure,
                                                    gint                 *n_arguments);
GParamSpec    ** ligma_procedure_get_return_values  (LigmaProcedure        *procedure,
                                                    gint                 *n_return_values);

void             ligma_procedure_set_argument_sync  (LigmaProcedure        *procedure,
                                                    const gchar          *arg_name,
                                                    LigmaArgumentSync      sync);
LigmaArgumentSync ligma_procedure_get_argument_sync  (LigmaProcedure        *procedure,
                                                    const gchar          *arg_name);

LigmaValueArray * ligma_procedure_new_arguments      (LigmaProcedure        *procedure);
LigmaValueArray * ligma_procedure_new_return_values  (LigmaProcedure        *procedure,
                                                    LigmaPDBStatusType     status,
                                                    GError               *error);

LigmaValueArray * ligma_procedure_run                (LigmaProcedure        *procedure,
                                                    LigmaValueArray       *args);

void             ligma_procedure_extension_ready    (LigmaProcedure        *procedure);

LigmaProcedureConfig *
                 ligma_procedure_create_config      (LigmaProcedure        *procedure);


G_END_DECLS

#endif  /*  __LIGMA_PROCEDURE_H__  */
