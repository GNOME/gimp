
source sphere.tcl

for {set l 15; set i 0} {$l <= 150} {incr l; incr i} {
    set fn [format "/src/t/%04d.jpg" $i]
    puts $fn
    set img [Shadow_Sphere 100 {0 255 128} {255 255 255} $l $TRUE]
    set drw [gimp-image-active-drawable $img]
#    gimp-display-new $img
#    gimp-image-scale $img 352 240
    file-jpeg-save $img $drw $fn 100 $TRUE
    gimp-image-free-shadow $img
    gimp-image-delete $img
}
