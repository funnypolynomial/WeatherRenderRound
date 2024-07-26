#! /usr/bin/env python3
'''
Circular Barometer dial, enclosure
3mm thick acrylic, need a void ~18mm deep to accomodate LCD

  1  2  3  4  5  6  7  P 8  9
  || || || || || || || | || ||
  || |<- 6 plates  ->| |    ||
   .                   .     .
   .                   .     .
  ||                   |    ||
  || || || || || || || | || ||
  
  |<--    27mm+PCB        -->|
1..9 are plates, 1 is front face, 2..8 have voids, 9 is back plate
P is PCB

* expanded by half a step all around to allow room for hex standoffs
'''

import inkex
import simplestyle, sys, copy
from math import *
from inksnek import *
import operator

class MyDesign(inkex.Effect):
  def __init__(self):
    inkex.Effect.__init__(self)
  
  def add_LCD_board(self, group, x, y):
    # add outline of board, origin is centre bottom
    inksnek.add_rect(group, x - self.board_base_width/2.0, y, self.board_base_width, self.board_base_height, inksnek.ignore_style, "LRB")
    inksnek.add_circle(group, x, y + self.board_height - self.board_width/2.0, self.board_width/2.0, inksnek.ignore_style)
    
  def m3_hex(self, group, x, y, rotation):
    # hex standoff hole, centred at x,y
    r = 5.75/2.0 # at widest part
    h = sqrt(3.0)*r/2.0 # at narrowest part
    # OR
    h = 5.7/2.0 # at narrowest part, 5.66mm (measured) but bumped
    r = 2.0*h/sqrt(3.0)
    g = inksnek.add_group(group, inksnek.translate_group(x, y) + inksnek.rotate_group(rotation))
    path  = inksnek.path_move_to(-r, 0)
    path += inksnek.path_line_to(-r/2.0, +h)
    path += inksnek.path_line_to(+r/2.0, +h)
    path += inksnek.path_line_to(+r, 0)
    path += inksnek.path_line_to(+r/2.0, -h)
    path += inksnek.path_line_to(-r/2.0, -h)
    path += inksnek.path_close()
    inksnek.add_path(g, path, inksnek.cut_style)
    inksnek.add_circle(g, 0, 0, 3.0/2.0, inksnek.ignore_style)
    
  def bolt_hole(self, group, x, y, nut, hex, fill, rotation):
    if hex:
        self.m3_hex(group, x, y, rotation)
    else:
        if fill:
            inksnek.add_hole(group, x, y, self.plate_mount_nut_radius, inksnek.fill_style)
        inksnek.add_hole(group, x, y, self.plate_mount_hole_radius)
        if nut:
            inksnek.add_circle(group, x, y, self.plate_mount_nut_radius, inksnek.ignore_style)
    
  def add_plate(self, group, plate):
    # add a basic plate at 0, 0
    p = inksnek.path_move_to(0, 0)
    p += inksnek.path_line_by(self.plate_width, 0)
    # right side omitted when adjacent
    if plate % self.plate_cols == 0:
        p += inksnek.path_line_by(0, self.plate_height - self.plate_radius)
    else:
        p += inksnek.path_move_by(0, self.plate_height - self.plate_radius)
    p += inksnek.path_round_by(-self.plate_radius, self.plate_radius, -self.plate_radius)
    p += inksnek.path_round_by(-self.plate_radius, -self.plate_radius, -self.plate_radius)
    p += inksnek.path_line_to(0, 0)
    inksnek.add_path(group, p, inksnek.cut_style)
    head = plate == self.plate_rows*self.plate_cols or plate == 1
    # void for hex just 2 plates deep, 6mm standoff
    # 6mm is too short, cut down 10mm to 9, 3 plates
    hex = self.m3 and plate in range(5, 8)
    fill = self.fill_nut and (plate == 1 or plate == 9)
    self.bolt_hole(group, self.plate_mount_hole_inset, self.plate_mount_hole_inset, head, hex, fill, -15)
    self.bolt_hole(group, self.plate_width - self.plate_mount_hole_inset, self.plate_mount_hole_inset, head, hex, fill, +15)
    self.bolt_hole(group, self.plate_width/2.0, self.plate_height - self.plate_mount_hole_inset, head, hex, fill, 0)
    if plate > 1 and plate <= self.plate_rows*self.plate_cols:
        # show void, interior outline
        ins = self.plate_void_inset
        rad = self.plate_radius - ins
        rnd = 3.0 # round-off the interior corners
        p = inksnek.path_move_to(ins + rnd, ins)
        p += inksnek.path_line_by(self.plate_width - 2.0*ins - 2.0*rnd, 0)
        p += inksnek.path_round_by(rnd, rnd, -rnd)
        p += inksnek.path_line_by(0, self.plate_height - self.plate_radius - 1.25*ins - rnd)
        p += inksnek.path_round_by(-rad, rad, -rad)
        p += inksnek.path_round_by(-rad, -rad, -rad)
        p += inksnek.path_line_to(ins, ins + rnd)
        p += inksnek.path_round_by(rnd, -rnd, -rnd)
        p += inksnek.path_close()
        if plate == self.plate_rows*self.plate_cols:
            inksnek.add_path(group, p, inksnek.ignore_style)
        else:
            inksnek.add_path(group, p, inksnek.cut_style)
        self.add_LCD_board(group, self.plate_width/2, self.board_offset_y)
    if plate == self.plate_rows*self.plate_cols:
        # back plate
        if inksnek.mode == inksnek.DEVEL: # perf check w/ PCB
            inksnek.add_perf_board(group, 0, 0, self.plate_steps_wide + 1, self.plate_steps_high + 1)
        # power
        pwr_x = self.plate_width - self.plate_power_hole_x  # it's on the back of the PCB
        inksnek.add_hole(group, pwr_x, self.plate_power_hole_y, self.plate_power_hole_r)
        text_ht = 3.0
        inksnek.add_annotation(group, pwr_x + self.plate_power_hole_r + 0.5*text_ht, self.plate_power_hole_y - text_ht/2.0, "TIP+5VDC", text_ht, inksnek.etch_style, inksnek.LEFT_ALIGN)
        # btn https://www.jaycar.co.nz/35mm-spst-micro-tactile-switch/p/SP0602 ?
        btn_x = self.plate_width - self.plate_btn_hole_x  # it's on the back of the PCB
        # sits high on PCB so square cutout "mezzanine"
        inksnek.add_rect(group, btn_x - self.plate_btn_hole_size/2.0, self.plate_btn_hole_y - self.plate_btn_hole_size/2.0, self.plate_btn_hole_size, self.plate_btn_hole_size, inksnek.cut_style)
        text_ht = 3.0
        inksnek.add_annotation(group, btn_x, self.plate_btn_hole_y - self.plate_btn_hole_size/2.0 - 1.5*text_ht, "SET", text_ht, inksnek.etch_style, inksnek.CENTRE_ALIGN)
        # MEW
        text_ht = 2.0
        inksnek.add_annotation(group, self.plate_width/2, 0.5*text_ht, "MEW MMXXIV", text_ht, inksnek.etch_style, inksnek.CENTRE_ALIGN)
        # vents
        r = 1.5
        # upper
        inksnek.add_hole(group, self.plate_width/2, self.plate_height - self.plate_void_inset - 3.0*r, r)
        # over sensor, (11.0, 13.0 is between SDA & SCL socket pins, hole is over sensor unit)
        inksnek.add_hole(group, self.plate_width - (11.0 - 2.0)*self.STEP, 13.0*self.STEP, r)
    if plate==8:
        inksnek.add_annotation(group, self.plate_width/2,  self.plate_height - self.plate_void_inset - 10, "TEST", 3.0, inksnek.etch_style, inksnek.CENTRE_ALIGN)
        inksnek.add_annotation(group, self.plate_width/2,  self.plate_height - self.plate_void_inset - 15, "TEST", 2.0, inksnek.etch_style, inksnek.CENTRE_ALIGN)
        self.bolt_hole(group, self.plate_width/2,  self.plate_height - self.plate_void_inset - 20, False, False, False, 0)
        
        
        
  def effect(self):
    # 16 steps between LCD socket and support
    inksnek.setup(self, inksnek.A4, inksnek.ACRYLIC, 3.0, 'mm', inksnek.DEVEL)
    self.STEP = 2.54
    self.plate_steps_wide = 22
    self.plate_steps_high = 26
    self.plate_width = self.plate_steps_wide*self.STEP
    self.plate_radius = self.plate_width/2.0
    self.plate_height = self.plate_steps_high*self.STEP
    self.plate_mount_hole_inset = 2.0*self.STEP # inset from edges of plate
    # total span is 27mms, 30mm m3 not enough, need m4
    # OR m3 x2 with 6.0mm standoff, https://www.jaycar.co.nz/m3-x-63mm-tapped-nylon-spacers-pack-of-25/p/HP0920
    self.m3 = False
    self.fill_nut = True
    
    if self.m3:
        self.plate_mount_hole_radius = 3.1/2.0
        self.plate_mount_nut_radius = 7.0/2.0
    else:
        self.plate_mount_hole_radius = 4.0/2.0
        self.plate_mount_nut_radius = 8.0/2.0
    self.plate_void_inset = 3.0*self.STEP # thickness of "wall"
    
    # mono socket, from back, with detent at top, rhs is +tip
    self.plate_power_hole_x = 17.0*self.STEP
    self.plate_power_hole_y = 6.0*self.STEP
    self.plate_power_hole_r = 5.0 # shaft of plug ~9mm wide

    self.plate_btn_hole_x = 16.5*self.STEP
    self.plate_btn_hole_y = 12.5*self.STEP
    self.plate_btn_hole_size = 7
    
    self.plate_cols = 3
    self.plate_rows = 3
    
    self.board_pins_offset_y = 4.5*self.STEP
    self.board_offset_y = self.board_pins_offset_y - 1.76
    self.board_width = 38.0
    self.board_height = 45.5
    self.board_base_width = 22.92
    self.board_base_height = 11.32
    
    plates = inksnek.add_group(inksnek.top_group, inksnek.translate_group(5.0, 5.0))
    for row in range(self.plate_rows):
        for plate in range(self.plate_cols):
            self.add_plate(inksnek.add_group(plates, inksnek.translate_group(plate*self.plate_width, row*(self.plate_height + 1.0))), plate + row*self.plate_cols + 1)

