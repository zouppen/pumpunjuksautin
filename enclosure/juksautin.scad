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
    // Kicad model via Blender
    rotate([90,0,0]) translate([0,0,0]) import("pcb.stl");
}

module pcb_box(w, h, pcb_thickness, above, below, rail=3, wall=2) {
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
        translate([wall,0,wall]) {
            cube([w,wall,inside_h]);
        }
        
        // Half a wall from the front. +1 offset to avoid UI bug.
        translate([-1,-1,-1]) cube([w+2*wall+2,wall/2+1,inside_h+2*wall+2]);
        
        dent_depth=3;
        translate([wall,2*wall, wall+below+pcb_thickness-margin]) {
            cube([w,dent_depth,above+margin]);
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
    clip_h = 7;
    clip_z = 7.85;
    translate([0,0,clip_z]) linear_extrude(clip_h) polygon([[0,0],[clip_w,0],[clip_w,clip_start+clip_y],[-clip_x,clip_start],[0,clip_start]]);
}

above=12;

// PCB box
pcb_box(pcb_width, h, pcb_real, above-pcb+margin, pcb_surface-pcb, rail=1.5);

// Calculated from magic values
inner_height = pcb_real+ above-pcb+margin+pcb_surface-pcb+2;

translate([open ? 80 : 0,0,0]) with_idc_frame([49.25,0,pcb_surface+0.8],8) {
    difference () {
        union () {
            // Outer cover
            cube([pcb_width+4,1,inner_height+2]);

            // Indent
            translate([2+margin/2,1,2+margin/2]) {
                cube([pcb_width-margin,1,inner_height-margin-2]);
            }
            
            // Clips
            translate([4-0.2,0,0]) clip();
            translate([pcb_width+0.2,0,0]) mirror([1,0,0]) clip();

        }
        
        mc(8.1,pcb_surface+2.4);
    }
}

module with_idc_frame(pos=[0,0,0], depth=5) {
    module obj(tweak=0) {
        translate(pos) rotate([90,0,0]) translate([0,-20,-depth-tweak]) linear_extrude(depth+2*tweak) children();
    }
    
    difference() {
        children();
        obj(0.1) import("idc-inner.svg");
    }
    obj() import("idc-outer.svg");
}
