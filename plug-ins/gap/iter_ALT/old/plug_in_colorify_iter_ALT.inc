/* ----------------------------------------------------------------------
 * p_Colorify_iter_ALT 
 * ----------------------------------------------------------------------
 */
gint p_Colorify_iter_ALT(GRunModeType run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) 
{
    typedef struct t_Colorify_Vals 
    {
      t_color   color;
    } t_Colorify_Vals; 

    t_Colorify_Vals  buf, *buf_from, *buf_to; 

    if(len_struct != sizeof(t_Colorify_Vals)) 
    {
      fprintf(stderr, "ERROR: p_Colorify_iter_ALT  stored Data missmatch in size %d != %d\n",   
                       (int)len_struct, sizeof(t_Colorify_Vals) ); 
      return -1;  /* ERROR */ 
    }

    gimp_get_data("Colorify_ITER_FROM", g_plugin_data_from); 
    gimp_get_data("Colorify_ITER_TO",   g_plugin_data_to); 

    buf_from = (t_Colorify_Vals *)&g_plugin_data_from[0]; 
    buf_to   = (t_Colorify_Vals *)&g_plugin_data_to[0]; 
    memcpy(&buf, buf_from, sizeof(buf));

    p_delta_color(&buf.color, &buf_from->color, &buf_to->color, total_steps, current_step);

    gimp_set_data("Colorify", &buf, sizeof(buf)); 

    return 0; /* OK */
}
