/* ----------------------------------------------------------------------
 * p_plug_in_displace_iter_ALT 
 * ----------------------------------------------------------------------
 */
gint p_plug_in_displace_iter_ALT(GRunModeType run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) 
{
    typedef struct t_plug_in_displace_Vals 
    {
      gdouble   amount_x;
      gdouble   amount_y;
      long      do_x;
      long      do_y;
      gint32    displace_map_x;
      gint32    displace_map_y;
      long      displace_type;
    } t_plug_in_displace_Vals; 

    t_plug_in_displace_Vals  buf, *buf_from, *buf_to; 

    if(len_struct != sizeof(t_plug_in_displace_Vals)) 
    {
      fprintf(stderr, "ERROR: p_plug_in_displace_iter_ALT  stored Data missmatch in size %d != %d\n",   
                       (int)len_struct, sizeof(t_plug_in_displace_Vals) ); 
      return -1;  /* ERROR */ 
    }

    gimp_get_data("plug_in_displace_ITER_FROM", g_plugin_data_from); 
    gimp_get_data("plug_in_displace_ITER_TO",   g_plugin_data_to); 

    buf_from = (t_plug_in_displace_Vals *)&g_plugin_data_from[0]; 
    buf_to   = (t_plug_in_displace_Vals *)&g_plugin_data_to[0]; 
    memcpy(&buf, buf_from, sizeof(buf));

    p_delta_gdouble(&buf.amount_x, buf_from->amount_x, buf_to->amount_x, total_steps, current_step);
    p_delta_gdouble(&buf.amount_y, buf_from->amount_y, buf_to->amount_y, total_steps, current_step);
    p_delta_long(&buf.do_x, buf_from->do_x, buf_to->do_x, total_steps, current_step);
    p_delta_long(&buf.do_y, buf_from->do_y, buf_to->do_y, total_steps, current_step);
    p_delta_drawable(&buf.displace_map_x, buf_from->displace_map_x, buf_to->displace_map_x, total_steps, current_step);
    p_delta_drawable(&buf.displace_map_y, buf_from->displace_map_y, buf_to->displace_map_y, total_steps, current_step);
    p_delta_long(&buf.displace_type, buf_from->displace_type, buf_to->displace_type, total_steps, current_step);

    gimp_set_data("plug_in_displace", &buf, sizeof(buf)); 

    return 0; /* OK */
}
