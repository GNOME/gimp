/* ----------------------------------------------------------------------
 * p_plug_in_spread_iter_ALT 
 * ----------------------------------------------------------------------
 */
gint p_plug_in_spread_iter_ALT(GRunModeType run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) 
{
    typedef struct t_plug_in_spread_Vals 
    {
      gdouble   spread_amount_x;
      gdouble   spread_amount_y;
    } t_plug_in_spread_Vals; 

    t_plug_in_spread_Vals  buf, *buf_from, *buf_to; 

    if(len_struct != sizeof(t_plug_in_spread_Vals)) 
    {
      fprintf(stderr, "ERROR: p_plug_in_spread_iter_ALT  stored Data missmatch in size %d != %d\n",   
                       (int)len_struct, sizeof(t_plug_in_spread_Vals) ); 
      return -1;  /* ERROR */ 
    }

    gimp_get_data("plug_in_spread_ITER_FROM", g_plugin_data_from); 
    gimp_get_data("plug_in_spread_ITER_TO",   g_plugin_data_to); 

    buf_from = (t_plug_in_spread_Vals *)&g_plugin_data_from[0]; 
    buf_to   = (t_plug_in_spread_Vals *)&g_plugin_data_to[0]; 
    memcpy(&buf, buf_from, sizeof(buf));

    p_delta_gdouble(&buf.spread_amount_x, buf_from->spread_amount_x, buf_to->spread_amount_x, total_steps, current_step);
    p_delta_gdouble(&buf.spread_amount_y, buf_from->spread_amount_y, buf_to->spread_amount_y, total_steps, current_step);

    gimp_set_data("plug_in_spread", &buf, sizeof(buf)); 

    return 0; /* OK */
}
