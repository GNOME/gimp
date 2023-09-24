#ifndef __FRACTALEXPLORER_DIALOGS_H__
#define __FRACTALEXPLORER_DIALOGS_H__

gint        explorer_dialog            (GimpProcedure       *procedure,
                                        GimpProcedureConfig *config);
void        dialog_update_preview      (GimpProcedureConfig *config);

void        set_cmap_preview           (GimpProcedureConfig *config);
void        make_color_map             (GimpProcedureConfig *config);

gchar     * get_line                   (gchar               *buf,
                                        gint                 s,
                                        FILE                *from,
                                        gint                 init);
gint        load_options               (fractalexplorerOBJ  *xxx,
                                        FILE                *fp);
void        explorer_load              (void);

#endif
