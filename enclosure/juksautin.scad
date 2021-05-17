hull() {
    // Kicad model
    rotate([90,0,0]) translate([175,40,65]) import("/home/joell/poista/untitled.stl");
}

module pcb_box(w, h, pcb_thickness, above, below, rail=3, wall=2, dent_size = 5) {
    inside_h = above+below+pcb_thickness;
    difference () {
        // Outer wall
        cube([w+2*wall, h+2*wall, inside_h+2*wall]);
        // Inner wall
        translate([wall+rail,0,wall]) {
            cube([w-2*rail, h+wall, inside_h]);
        }
        // Rail
        translate([wall,wall,wall+below]) {
            cube([w,h, pcb_thickness]);
        }
        // Front opening
        translate([wall,0,wall]) {
            cube([w,wall,inside_h]);
        }
        
        // Cover dent
        for (x = [wall, wall+w-rail]) {
            translate([x,2*wall, wall+below+pcb_thickness+above/2-dent_size/2]) {
                cube([rail,dent_size,dent_size]);
            }
        }
    }
}

module terminal() {
    korkeus = 10;
    syvyys = 5;
    translate([0,syvyys,18]) rotate([0,0,90]) cylinder(7,2,2, $fn=6); // 4 mm hole
    translate([0,0,korkeus]) cube([5.1,5,5.1], center=true);
}

module terminals() {
    reunasta = 2+9;
    translate([reunasta,0,0]) {
        for (i=[0:5]) {
            translate([i*5.08,0,0]) terminal();
        }
    }
}

margin = 0.4;
pcb = 1.6;
pcb_real = 2.2;
pcb_width = 60+margin;
h = 35+margin;
pcb_surface = 6;
pull = 3;

thick = 2+1.2; // Thickness of front panel

module clip() {
    clip_start = 4.4;
    clip_x = 1;
    clip_y = 3.4;
    clip_w = 2.4;
    clip_h = 3.9;
    clip_z = 12.2;
    translate([0,0,clip_z]) linear_extrude(clip_h) polygon([[0,0],[clip_w,0],[clip_w,clip_start+clip_y],[-clip_x,clip_start],[0,clip_start]]);
}

difference () {

    union () {
        // PCB box
        pcb_box(pcb_width, h, pcb_real, 12.5-pcb+margin, pcb_surface-pcb, rail=1.5);
        radius=20;
        holetsu=5;
        translate([radius/2,h/2,0]) {
             difference() {
                cylinder(2,radius,radius, $fn=6);
                translate([-3*radius/4,0,0]) {
                    cylinder(2,holetsu/2, holetsu/2, $fn=6);
                }
            }
         }
         translate([pcb_width+4-radius/2,h/2,0]) {
             difference() {
                cylinder(2,radius,radius, $fn=6);
                translate([3*radius/4,0,0]) {
                    cylinder(2,holetsu/2, holetsu/2, $fn=6);
                }
            }
        }
    }
    
    // switch hole
    hole_w = 9;
    translate([2+27,h+3,2+pcb_surface+3]) {
        cube([10,10,6], center=true);
    }
    
    terminals();
}

// Calculated from magic values
inner_height = pcb_real+ 12.5-pcb+margin+pcb_surface-pcb+2;

translate([80,0,0]) {
    difference () {
        union () {
            // Outer box
            translate([0,-pull+2,0]) {
                cube([pcb_width+2*2,pull-2,inner_height+2]);
            }

            // Indent
            translate([0+2+margin/2,0,2+margin/2]) {
                cube([pcb_width-margin,2,inner_height-margin-2]);
            }
            
            // Clips
            translate([4-0.1,0,0]) clip();
            translate([pcb_width+0.1,0,0]) mirror([1,0,0]) clip();

        }

        // Micro USB
        translate([48+2,0,10]) {
            cube([7.8,5,3], center=true);
        }
        
        terminals();
    }
}
