/* ----------------------------------------------------------------------
 * p_plug_in_holes_iter_ALT 
 * ----------------------------------------------------------------------
 */
gint p_plug_in_holes_iter_ALT(GRunModeType run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) 
{
    typedef struct t_plug_in_holes_Vals 
    {
      gdouble   density;
      long      shape;
      long      size;
      long      flag;
    } t_plug_in_holes_Vals; 

    t_plug_in_holes_Vals  buf, *buf_from, *buf_to; 

    if(len_struct != sizeof(t_plug_in_holes_Vals)) 
    {
      fprintf(stderr, "ERROR: p_plug_in_holes_iter_ALT  stored Data missmatch in size %d != %d\n",   
                       (int)len_struct, sizeof(t_plug_in_holes_Vals) ); 
      return -1;  /* ERROR */ 
    }

    gimp_get_data("plug_in_holes_ITER_FROM", g_plugin_data_from); 
    gimp_get_data("plug_in_holes_ITER_TO",   g_plugin_data_to); 

    buf_from = (t_plug_in_holes_Vals *)&g_plugin_data_from[0]; 
    buf_to   = (t_plug_in_holes_Vals *)&g_plugin_data_to[0]; 
    memcpy(&buf, buf_from, sizeof(buf));

    p_delta_gdouble(&buf.density, buf_from->density, buf_to->density, total_steps, current_step);
    p_delta_long(&buf.shape, buf_from->shape, buf_to->shape, total_steps, current_step);
    p_delta_long(&buf.size, buf_from->size, buf_to->size, total_steps, current_step);
    p_delta_long(&buf.flag, buf_from->flag, buf_to->flag, total_steps, current_step);

    gimp_set_data("plug_in_holes", &buf, sizeof(buf)); 

    return 0; /* OK */
}
