#!@GIMPTCL@

proc gimptcl_query {} {
    puts "howdy from gimptcl_query"
    gimp-install-procedure "test_plug_in" \
	"A Test Of Tclified Plugins" \
	"None Yet" \
	"Eric L. Hernes" \
	"ELH" \
	"1997" \
        "<Image>/Test plug-in" \
	"RGB, GRAY, INDEXED" \
	plugin \
	{
	    {int32 "run_mode" "Interactive, non-interactive"}
	    {image "the image" "some description of it"}
	    {drawable "the drawable" "some description of it"}
	} \
	{
	    {int32 "first return" "some desciption of it too"}
	}
}

proc gimptcl_run {mode image drawable} {
    puts stderr "mode $mode; image: $image; drawable: $drawable"
    if {$mode == 0} {
	destroy .
    }
    gen-bg $image
    gimp-displays-flush
    return 123
}
