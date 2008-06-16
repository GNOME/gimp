import unittest
import gimp
import gtk
import gobject

class TestAllWidgetFunctions(unittest.TestCase):
    
    @classmethod
    def suite(cls):
        import unittest
        return unittest.makeSuite(cls,'test')

    
    def _testBoolGetterSetter(self, setter, getter):
        setter(True)
        assert getter() == True
        setter(False)
        assert getter() == False

    def _testColorGetterSetter(self, setter, getter):
        setter(gimp.color.RGB(1,2,3))
        getter()


    def testAspectPreview(self):
        # FIXME
        # drawable = gimp.Image(100,100)
        # ap = gimp.ui.AspectPreview(drawable, False)
        # ap = gimp.ui.AspectPreview(drawable, True)
        pass

    def testBrowser(self):
        browser = gimp.ui.Browser()
        browser.add_search_types((".gif", 1), (".png", 2))
        browser.set_widget(gtk.Button("some label"))
        browser.show_message("Some label")

    def testBrushSelectButton(self):
        bs = gimp.ui.BrushSelectButton("Some title", gimp.context.get_brush(), 1.0, 10,
                                       gimp.enums.NORMAL_MODE)
        # FIXME
        bs.set_brush(str(gimp.context.get_brush()), 1.0, 10, gimp.enums.NORMAL_MODE)
        brush = bs.get_brush()

    def testButton(self):
        button = gimp.ui.Button()
        button.extended_clicked(gtk.gdk.BUTTON1_MASK)

    def testCellRendererColor(self):
        crc = gimp.ui.CellRendererColor()
        crc.props.color = gimp.color.RGB(255,0,0)
        crc.props.opaque = True
        crc.props.icon_size = 10

    def testCellRendererToggle(self):
        crt = gimp.ui.CellRendererToggle("Some stock id")
        crt.props.stock_id = "foobar"
        crt.props.stock_size = 2

    def testChainButton(self):
        cb = gimp.ui.ChainButton(gimp.ui.CHAIN_TOP)
        self._testBoolGetterSetter(cb.set_active, cb.get_active)

    def testChannelComboBox(self):
        ccb = gimp.ui.ChannelComboBox(lambda value: True)
        ccb = gimp.ui.ChannelComboBox(lambda value, data: False, "Some Data")

    def testColorArea(self):
        ca = gimp.ui.ColorArea(gimp.color.RGB(255, 0, 0), gimp.ui.COLOR_AREA_FLAT,
                               gtk.gdk.BUTTON1_MASK)
        ca.set_color(gimp.color.RGB(255, 0, 0))
        color = ca.get_color()
        has_alpha = ca.has_alpha()
        ca.set_type(gimp.ui.COLOR_AREA_SMALL_CHECKS)
        ca.set_draw_border(True)

    def testColorButton(self):
        cb = gimp.ui.ColorButton("Some title", 100, 40, gimp.color.RGB(200,255,10),
                                 gimp.ui.COLOR_AREA_FLAT)
        cb.set_color(gimp.color.RGB(255, 1, 2, 3))
        color = cb.get_color()
        has_alpha = cb.has_alpha()
        cb.set_type(gimp.ui.COLOR_AREA_SMALL_CHECKS)
        cb.set_update(True)
        update = cb.get_update()

    def testColorDisplay(self):
        # FIXME
        pass

    def testColorDisplayStack(self):
        # FIXME
        pass

    def testColorHexEntry(self):
        che = gimp.ui.ColorHexEntry()
        che.set_color(gimp.color.RGB(0,0,0))
        color = che.get_color()

    def testColorNotebook(self):
        cn = gimp.ui.ColorNotebook()
        page = cn.set_has_page(gimp._ui.ColorSelector.__gtype__, False)

    def testColorProfileComboBox(self):
        cps = gimp.ui.ColorProfileStore("history")
        cpcb = gimp.ui.ColorProfileComboBox(gtk.Dialog(), cps)
        cpcb.add("Some/filename", "Some label")
        cpcb.set_active("Some/filename", "Some label")
        cpcb.get_active()

    def testColorProfileStore(self):
        cps = gimp.ui.ColorProfileStore("history")
        cps.add("Some/filename", "Some label")
        
    def testColorScale(self):
        cs = gimp.ui.ColorScale(gtk.ORIENTATION_VERTICAL, gimp.ui.COLOR_SELECTOR_SATURATION)
        cs.set_channel(gimp.ui.COLOR_SELECTOR_GREEN)
        cs.set_color(gimp.color.RGB(1,2,3), gimp.color.HSV(3,2,1))

    def testColorSelection(self):
        cs = gimp.ui.ColorSelection()
        self._testBoolGetterSetter(cs.set_show_alpha, cs.get_show_alpha)
        self._testColorGetterSetter(cs.set_color, cs.get_color)
        self._testColorGetterSetter(cs.set_old_color, cs.get_old_color)
        cs.reset()
        cs.color_changed()
        #cs.set_config(None)

    def testDialog(self):
        dialog = gimp.ui.Dialog("Some title", "Some role", None, 0,
                                lambda id: True, "Some help id",
                                ("foo", 1, "bar", 2))
        dialog.add_button("batz", 2)
        # dialog.run()
        dialog.set_transient()
    def testDrawableComboBox(self):
        dcb = gimp.ui.DrawableComboBox(lambda value: True)
        dcb = gimp.ui.DrawableComboBox(lambda value, data: False, "Some data")

    def testDrawablePreview(self):
        image = gimp.Image(100, 100)
        dp = gimp.ui.DrawablePreview(image)

    def testEnumComboBox(self):
        ecb = gimp.ui.EnumComboBox(gimp.ui.ColorSelectorChannel.__gtype__)
        ecb.set_stock_prefix("FOOBAR")

    def testEnumLabel(self):
        el = gimp.ui.EnumLabel(gimp.ui.ColorSelectorChannel.__gtype__, 0)
        el.set_value(1)

    def testEnumStore(self):
        es = gimp.ui.EnumStore(gimp.ui.ColorSelectorChannel.__gtype__)
        es = gimp.ui.EnumStore(gimp.ui.ColorSelectorChannel.__gtype__, 1, 3)
        es.set_stock_prefix("FOOBAR")

    def testFontSelectButton(self):
        fsb = gimp.ui.FontSelectButton("Some title", "Some font")
        fsb.set_font("Arial")
        font = fsb.get_font()

    def testFrame(self):
        frame = gimp.ui.Frame("Some title")

    def testGradientSelectButton(self):
        gsb = gimp.ui.GradientSelectButton("Some title",
                                           gimp.context.get_gradient())
        # FIXME
        gsb.set_gradient(str(gimp.context.get_gradient()))
        gradient = gsb.get_gradient()

    def testHintBox(self):
        hb = gimp.ui.HintBox("Some hint")
    
    def testImageComboBox(self):
        icb = gimp.ui.ImageComboBox(lambda value: True)
        icb = gimp.ui.ImageComboBox(lambda value, data: False, "Some data")

    def testIntComboBox(self):
        icb = gimp.ui.IntComboBox(("foo", 1))
        icb.prepend(("bar", 2))
        icb.append(("batz", 3))
        icb.set_active(1)
        active = icb.get_active()
        icb.set_sensitivity(lambda value: True)
        icb.set_sensitivity(lambda value, data: False, "Some data")

    def testIntStore(self):
        intstore = gimp.ui.IntStore()
        intstore.lookup_by_value(10)

    def testLayerComboBox(self):
        lcb = gimp.ui.LayerComboBox(lambda value: True)
        lcb = gimp.ui.LayerComboBox(lambda value, data: False, "Some data")

    def testMemsizeEntry(self):
        me = gimp.ui.MemsizeEntry(10, 0, 100)
        me.set_value(20)
        value = me.get_value()

    def testNumberPairEntry(self):
        npe = gimp.ui.NumberPairEntry("-", True, 1.5, 2)
        npe.set_default_values(4.3, 5)
        default_values = npe.get_default_values()
        npe.set_values(1, 2.4)
        values = npe.get_values()
        npe.set_default_text("MOO")
        default_text = npe.get_default_text()
        npe.set_ratio(2)
        ratio = npe.get_ratio()
        npe.set_aspect(4)
        aspect = npe.get_aspect()
        self._testBoolGetterSetter(npe.set_user_override, npe.get_user_override)

    def testOffsetAreas(self):
        oa = gimp.ui.OffsetArea(200, 100)
        oa.set_pixbuf(gtk.gdk.pixbuf_new_from_file("../../data/images/wilber.png"))
        oa.set_size(10,20)
        oa.set_offsets(30, 49)

    def testPageSelector(self):
        ps = gimp.ui.PageSelector()
        ps.set_n_pages(10)
        n_pages = ps.get_n_pages()
        ps.set_target(gimp.ui.gimp.ui.PAGE_SELECTOR_TARGET_IMAGES)
        target = ps.get_target()
        # FIXME
        # ps.set_page_thumbnail()
        # ps.get_page_thumbnail()
        ps.set_page_label(1, "Some label")
        label = ps.get_page_label(0)
        ps.select_all()
        ps.unselect_all()
        ps.select_page(0)
        ps.unselect_page(1)
        ps.page_is_selected(0)
        ps.get_selected_pages()
        ps.select_range("2,4-6")
        ps.get_selected_range()

    def testPaletteSelectButton(self):
        psb = gimp.ui.PaletteSelectButton("Some title",
                                          gimp.context.get_palette())
        # FIXME
        psb.set_palette(str(gimp.context.get_palette()))
        psb.get_palette()
        
    def testPathEditor(self):
        pe = gimp.ui.PathEditor("Some title", "Some/path")
        pe.set_path("Some/pther/path")
        pe.get_path()
        pe.set_writable_path("foo/bar")
        pe.get_writable_path()
        pe.set_dir_writable("foo/bar", True)
        pe.get_dir_writable("foo/bar")
        
    def testPatternSelectButton(self):
        psb = gimp.ui.PatternSelectButton("Some title",
                                         gimp.context.get_pattern())
        # FIXME
        psb.set_pattern(str(gimp.context.get_pattern()))
        psb.get_pattern()
        
    def testPickButton(self):
        pb = gimp.ui.PickButton()
        
    def testPreview(self):
        # FIXME
        pass

    def testPreviewArea(self):
        # FIXME
        pass

    def testProcBrowserDialog(self):
        pbd = gimp.ui.ProcBrowserDialog("Some title", "Some role")
        pbd.get_selected()
        pbd = gimp.ui.ProcBrowserDialog("Some title", "Some role",
                                       lambda id: True, "some help id")
        pbd = gimp.ui.ProcBrowserDialog("Some title", "Some role",
                                       lambda id:False, "some help id",
                                       ("foo", 0, "bar", 1))
        
    def testProgressBar(self):
        pb = gimp.ui.ProgressBar()

    def testScrolledPreview(self):
        # FIXME
        pass

    def testSelectButton(self):
        sb = gimp.ui.SelectButton()
        sb.close_popup()

    def testSizeEntry(self):
        return
        # FIXME
        se = gimp.ui.SizeEntry(3, gimp.enums.UNIT_PIXEL, "%a", True, False, True, 100,
                               gimp.ui.SIZE_ENTRY_UPDATE_NONE)
        se.add_field(gtk.SpinButton(), gtk.SpinButton())
        se.attach_label("foo", 0, 0, 0.5)
        se.set_resolution(0, 3, True)
        se.set_size(0, 0, 100)
        se.set_value_boundaries(0, 10, 20)
        se.get_value(0)
        se.set_value(0, 4.4)
        se.set_refval_boundaries(0, 0, 10)
        se.set_refval_digits(0, 2)
        se.set_refval(0, 1.3)
        se.get_refval(0)
        se.get_unit()
        se.set_unit(gimp.enums.UNIT_PIXEL)
        se.show_unit_menu(True)
        se.set_pixel_digits(2)
        se.grab_focus()
        se.set_activates_default(True)
        # FIXME
        # se.get_help_widget(0) 

    def testStringComboBox(self):

        scb = gimp.ui.StringComboBox(gtk.ListStore(gobject.TYPE_STRING,
                                     gobject.TYPE_STRING), 0, 1)
        scb.set_active("foo")
        scb.get_active()

    def testUnitMenu(self):
        um = gimp.ui.UnitMenu("%y%a", gimp.enums.UNIT_PIXEL, True, True, True)
        um.set_unit(gimp.enums.UNIT_INCH)
        um.get_unit()
        um.set_pixel_digits(10)
        um.get_pixel_digits()

    def testVectorsComboBox(self):
        vcb = gimp.ui.VectorsComboBox(lambda value: True)
        vcb = gimp.ui.VectorsComboBox(lambda value, data: False, "Some data")

    def testZoomModel(self):
        zm = gimp.ui.ZoomModel()
        zm.set_range(5, 5.5)
        zm.zoom(gimp.ui.ZOOM_IN, 3)
        zm.get_factor()
        zm.get_fraction()

    def testZoomPreview(self):
       image = gimp.Image(100, 100)
       zp = gimp.ui.ZoomPreview(image)
       assert zp.get_drawable() == image
       zp.get_model()
       zp.get_factor()

if __name__ == "__main__":
    runner = unittest.TextTestRunner()
    runner.run(TestAllWidgetFunctions.suite())
