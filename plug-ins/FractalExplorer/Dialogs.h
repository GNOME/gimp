#ifndef __FRACTALEXPLORER_DIALOGS_H__
#define __FRACTALEXPLORER_DIALOGS_H__

gint        explorer_dialog            (void);
void        dialog_update_preview      (void);

void        set_cmap_preview           (void);
void        make_color_map             (void);

void        dialog_change_scale        (void);
void        save_options               (FILE               *fp);
void        save_callback              (void);
void        file_selection_ok          (GtkWidget          *widget,
					GtkFileSelection   *fs,
					gpointer            data);
void        load_file_selection_ok     (GtkWidget          *widget,
					GtkFileSelection   *fs,
					gpointer            data);
void        create_load_file_selection (void);
void        create_file_selection      (void);
gchar     * get_line                   (gchar              *buf,
					gint                s,
					FILE               *from,
					gint                init);
gint        load_options               (fractalexplorerOBJ *xxx,
					FILE               *fp);
void        explorer_load              (void);

#endif
