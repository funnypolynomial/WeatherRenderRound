#! /usr/bin/env python3
'''
Circular Barometer dial (WeatherRenderRound)

'''

import inkex
import simplestyle, sys, copy
from math import *
from inksnek import *
import operator

class MyDesign(inkex.Effect):
  def __init__(self):
    inkex.Effect.__init__(self)

  def add_pressure_ring(self, group):
    # main outer ring
    # labels
    label_height = 15 if self.metric else 16
    label_font = "Sans"
    label_radius = self.radius - 2.5*label_height
    mark_angle = 45 if self.metric else 90
    if self.metric:
        for i in range(5):
            inksnek.add_text(inksnek.add_group(group, inksnek.rotate_group(-90 + i*mark_angle)), 
                0, label_radius, label_height, label_font, 
                str(990 + i*10))
    else:
        for i in range(3):
            inksnek.add_text(inksnek.add_group(group, inksnek.rotate_group(-90 + i*mark_angle)), 
                0, label_radius, label_height, label_font, 
                str(29 + i))
    # hash marks
    mark_radius = label_radius + label_height
    mark_len = label_height
    mark_delta = mark_angle/10.0
    mark_style = str(inkex.Style({'stroke':'#000000',  'stroke-width':'1', 'fill':'none'}))
    mark_style_bold = str(inkex.Style({'stroke':'#000000',  'stroke-width':'2', 'fill':'none'}))
    extra_marks = 5 if self.metric else 0
    angle = -135 - extra_marks*mark_delta
    while angle <= +135 + extra_marks*mark_delta:
        if abs(angle) % mark_angle == 0:
            inksnek.add_line_by(inksnek.add_group(group, inksnek.rotate_group(angle)), 0, mark_radius, 0, mark_len*1.25, mark_style_bold)
        else:
            inksnek.add_line_by(inksnek.add_group(group, inksnek.rotate_group(angle)), 0, mark_radius, 0, mark_len, mark_style)
        angle += mark_delta
    # units
    if self.metric:
        inksnek.add_text(group, 0, -(mark_radius), 1.5*label_height, label_font, "hPa")        
    else:
        inksnek.add_text(group, 0, -(mark_radius), 1.5*label_height, label_font, "inHg")        
  
  def add_subsidiary_ring(self, group, span, sparse, label_start, label_step, units, alt_label = False):
    # small inner rings
    start_angle = -135
    mark_delta = 270.0/span
    mark_radius = self.radius/5.0
    # labels
    label_height = 9.5
    label_font = "Sans"
    label_radius = mark_radius + 0.9*label_height
    for i in range(int(span/label_step) + 1):
        if not alt_label or i % 2:
            inksnek.add_text(inksnek.add_group(group, inksnek.rotate_group(start_angle + i*mark_delta*10)), 
                0, label_radius, label_height, label_font, 
                str(label_start + i*label_step))
    # hash marks
    mark_len = 4
    mark_len_bold = 7
    mark_style = str(inkex.Style({'stroke':'#000000',  'stroke-width':'1', 'fill':'none'}))
    angle = start_angle
    ctr = 0
    while angle <= +135:
        if sparse:
            if ctr % 10 == 0:
                inksnek.add_line_by(inksnek.add_group(group, inksnek.rotate_group(angle)), 0, mark_radius, 0, mark_len_bold, mark_style)
            elif ctr % 5 == 0:
                inksnek.add_line_by(inksnek.add_group(group, inksnek.rotate_group(angle)), 0, mark_radius, 0, mark_len, mark_style)
        else:
            if ctr % 5 == 0:
                inksnek.add_line_by(inksnek.add_group(group, inksnek.rotate_group(angle)), 0, mark_radius, 0, mark_len_bold, mark_style)
            else:
                inksnek.add_line_by(inksnek.add_group(group, inksnek.rotate_group(angle)), 0, mark_radius, 0, mark_len, mark_style)
        angle += mark_delta
        ctr += 1
    if not alt_label:
        inksnek.add_line_by(inksnek.add_group(group, inksnek.rotate_group(angle)), 0, mark_radius, 0, mark_len_bold, mark_style)
    # units
    inksnek.add_text(group, 0, -mark_radius, 1.5*label_height, label_font, units)        
    
  def arrow_head(self, group, r, theta_deg, s):
    org = inksnek.polar_to_rectangular(r, theta_deg)
    if s < 0:
        g = inksnek.add_group(group, inksnek.translate_group(org[0], org[1]) + inksnek.rotate_group(180.0 + theta_deg))
    else:
        g = inksnek.add_group(group, inksnek.translate_group(org[0], org[1]) + inksnek.rotate_group(theta_deg))
    p = inksnek.path_move_to(-1, s*-0.075) # the y is a fudge factor to make the arrowhead look centred on the arc
    p += inksnek.path_arrow_to(0, 0, 15)
    inksnek.add_path(g, p, self.black_stroke_style)
    
        
  def effect(self):
    self.metric = True # hPa vs inHg
    self.radius = 120 # pixels
    self.pressure_label_height = 12
    self.pressure_label_font = "Sans"
    self.pressure_label_radius = self.radius - 2*self.pressure_label_height
    inksnek.setup(self, inksnek.CUSTOM, inksnek.ACRYLIC, 3.0, 'px', inksnek.FINAL)
    dial = inksnek.add_group(inksnek.top_group, inksnek.translate_group(self.radius, -self.radius))
    self.white_fill_style  = inksnek.create_fill_style('#FFFFFF')
    self.skip_fill_style  = inksnek.create_fill_style('#800000')
    self.black_fill_style  = inksnek.create_fill_style('#000000')
    self.black_stroke_style  = inksnek.create_stroke_style('#000000', 1)
    
    inksnek.add_rect(dial, -self.radius, -self.radius, 2.0*self.radius, 2.0*self.radius, self.skip_fill_style)
    inksnek.add_circle(dial, 0, 0, self.radius, self.white_fill_style)
    self.add_pressure_ring(dial)
    subsidiary_x = self.radius/3 + 3
    subsidiary_y = self.radius/3 - 6
    self.add_subsidiary_ring(inksnek.add_group(dial, inksnek.translate_group(-subsidiary_x, -subsidiary_y)), 100, True, 0, 10, "%")
    if self.metric:
        self.add_subsidiary_ring(inksnek.add_group(dial, inksnek.translate_group(+subsidiary_x, -subsidiary_y)), 70, True, -20, 10, "\u00b0""C")
    else:
        self.add_subsidiary_ring(inksnek.add_group(dial, inksnek.translate_group(+subsidiary_x, -subsidiary_y)), 120, True, 0, 10, "\u00b0""F", True)
        
    # trend icons/arrows    
    icon_r = self.radius*0.45 if self.metric else self.radius*0.45
    icon_pos = inksnek.polar_to_rectangular(icon_r, 67.5)
    inksnek.add_text(inksnek.add_group(dial, inksnek.translate_group(-icon_pos[0], +icon_pos[1])), 0, 0, 30, "Sans", "\u26C8")   #cloud     
    inksnek.add_text(inksnek.add_group(dial, inksnek.translate_group(+icon_pos[0], +icon_pos[1])), 0, 0, 30, "Sans", "\u263C")   #sun
    arc_end = 45.0
    arc_start = 9.0
    inksnek.add_arc(dial, 0, 0, icon_r + 10, +arc_start,   +arc_end, self.black_stroke_style)
    inksnek.add_arc(dial, 0, 0, icon_r + 10, -arc_end,   -arc_start, self.black_stroke_style)
    self.arrow_head(dial, icon_r + 10, +arc_end, +1)
    self.arrow_head(dial, icon_r + 10, -arc_end, -1)
