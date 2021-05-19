open = true;
//open = false;

module screwdriver_hole(width, length, depth) {
    linear_extrude(depth) {
        // The screwdriver opening
        hull() {
            for (pos = [-length/2, length/2]) {
                translate([pos,0,0]) circle(width/2, $fn=6);
            }
        }
    }
}

// Din rail clamp
difference() {
    pos_x = pcb_width/2-19;
    pos_y = h+2;
    linear_extrude(inner_height+2) translate([pos_x,pos_y,0]) rotate([180,0,90]) import("din_clip_01.dxf");
    translate([pos_x,pos_y+12.6,inner_height/2+1]) rotate([0,90,0]) screwdriver_hole(2, 5, 3);
}

translate([2+margin/2,2+margin/2 + (open ? -60 : 0),pcb+pcb_surface]) {
    // Kicad model. Obtain by VRML export from 
    rotate([90,0,0]) translate([175,0,65]) import("pcb.stl");
}

module pcb_box(w, h, pcb_thickness, above, below, rail=3, wall=2, dent_size = 6) {
    inside_h = above+below+pcb_thickness;
    difference () {
        // Outer wall
        cube([w+2*wall, h+2*wall, inside_h+2*wall]);
        // Inner wall
        translate([wall+rail,0,wall]) {
            cube([w-2*rail, h+wall, inside_h]);
        }
        // Rail
        translate([wall,wall-1,wall+below]) {
            cube([w,h+1, pcb_thickness]);
        }
        // Front opening
        translate([wall,-1,wall]) {
            cube([w,wall+1,inside_h]);
        }
        
        // Half a wall from the front
        cube([w+2*wall,wall/2,inside_h+2*wall]);
        
        dent_depth=3;
        translate([wall,2*wall, wall+below+pcb_thickness+above/2-dent_size/2]) {
            cube([w,dent_depth,dent_size]);
        }
    }
}

module mc(x,z,terminals=9, pitch=3.81) {
    translate([x-margin/2,-1,z-margin/2]) cube([5.2+(terminals-1)*pitch+margin,4,7.25+margin]);
}

margin = 0.4;
pcb = 1.6;
pcb_real = 2.2;
pcb_width = 65+margin;
h = 35+margin;
pcb_surface = 3.5;
pull = 3;

thick = 2+1.2; // Thickness of front panel

module clip() {
    clip_start = 4.4;
    clip_x = 1.5;
    clip_y = 3.9;
    clip_w = 2.4;
    clip_h = 5.3;
    clip_z = 7.85;
    translate([0,0,clip_z]) linear_extrude(clip_h) polygon([[0,0],[clip_w,0],[clip_w,clip_start+clip_y],[-clip_x,clip_start],[0,clip_start]]);
}

above=10;

// PCB box
pcb_box(pcb_width, h, pcb_real, above-pcb+margin, pcb_surface-pcb, rail=1.5);

// Calculated from magic values
inner_height = pcb_real+ above-pcb+margin+pcb_surface-pcb+2;

translate([open ? 80 : 0,0,0]) {
    difference () {
        union () {
            // Outer cover
            cube([pcb_width+4,1,inner_height+2]);

            // Indent
            translate([2+margin/2,1,2+margin/2]) {
                cube([pcb_width-margin,1,inner_height-margin-2]);
            }
            
            // Clips
            translate([4-0.3,0,0]) clip();
            translate([pcb_width+0.3,0,0]) mirror([1,0,0]) clip();

        }

        // Micro USB
        translate([53.2+2,0,pcb_surface+5.4]) {
            cube([7.8,5,3], center=true);
        }
        
        mc(8.1,pcb_surface+2.4);
    }
}
