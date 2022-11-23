/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * ligmaprocedureconfig.h
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

#ifndef __LIGMA_PROCEDURE_CONFIG_H__
#define __LIGMA_PROCEDURE_CONFIG_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define LIGMA_TYPE_PROCEDURE_CONFIG            (ligma_procedure_config_get_type ())
#define LIGMA_PROCEDURE_CONFIG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_PROCEDURE_CONFIG, LigmaProcedureConfig))
#define LIGMA_PROCEDURE_CONFIG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_PROCEDURE_CONFIG, LigmaProcedureConfigClass))
#define LIGMA_IS_PROCEDURE_CONFIG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_PROCEDURE_CONFIG))
#define LIGMA_IS_PROCEDURE_CONFIG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_PROCEDURE_CONFIG))
#define LIGMA_PROCEDURE_CONFIG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_PROCEDURE_CONFIG, LigmaProcedureConfigClass))


typedef struct _LigmaProcedureConfigClass   LigmaProcedureConfigClass;
typedef struct _LigmaProcedureConfigPrivate LigmaProcedureConfigPrivate;

struct _LigmaProcedureConfig
{
  GObject                     parent_instance;

  LigmaProcedureConfigPrivate *priv;
};

struct _LigmaProcedureConfigClass
{
  GObjectClass  parent_class;

  /* Padding for future expansion */
  void (* _ligma_reserved1) (void);
  void (* _ligma_reserved2) (void);
  void (* _ligma_reserved3) (void);
  void (* _ligma_reserved4) (void);
  void (* _ligma_reserved5) (void);
  void (* _ligma_reserved6) (void);
  void (* _ligma_reserved7) (void);
  void (* _ligma_reserved8) (void);
};


GType   ligma_procedure_config_get_type      (void) G_GNUC_CONST;

LigmaProcedure *
        ligma_procedure_config_get_procedure (LigmaProcedureConfig *config);

void    ligma_procedure_config_set_values    (LigmaProcedureConfig  *config,
                                             const LigmaValueArray *values);
void    ligma_procedure_config_get_values    (LigmaProcedureConfig  *config,
                                             LigmaValueArray       *values);

void    ligma_procedure_config_begin_run     (LigmaProcedureConfig  *config,
                                             LigmaImage            *image,
                                             LigmaRunMode           run_mode,
                                             const LigmaValueArray *args);
void    ligma_procedure_config_end_run       (LigmaProcedureConfig  *config,
                                             LigmaPDBStatusType     status);

LigmaMetadata *
        ligma_procedure_config_begin_export  (LigmaProcedureConfig  *config,
                                             LigmaImage            *original_image,
                                             LigmaRunMode           run_mode,
                                             const LigmaValueArray *args,
                                             const gchar          *mime_type);
void    ligma_procedure_config_end_export    (LigmaProcedureConfig  *config,
                                             LigmaImage            *exported_image,
                                             GFile                *file,
                                             LigmaPDBStatusType     status);

void    ligma_procedure_config_save_metadata (LigmaProcedureConfig  *config,
                                             LigmaImage            *exported_image,
                                             GFile                *file);


G_END_DECLS

#endif /* __LIGMA_PROCEDURE_CONFIG_H__ */
