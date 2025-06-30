-- FLIR Camera HUD Display Script for FlyWithLua with realistic military HUD that auto-toggles based on camera view
--
-- MIT License
-- 
-- Copyright (c) 2025 sebastian <sebastian@eingabeausgabe.io>
-- 
-- Permission is hereby granted, free of charge, to any person obtaining a copy
-- of this software and associated documentation files (the "Software"), to deal
-- in the Software without restriction, including without limitation the rights
-- to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
-- copies of the Software, and to permit persons to whom the Software is
-- furnished to do so, subject to the following conditions:
-- 
-- The above copyright notice and this permission notice shall be included in all
-- copies or substantial portions of the Software.
-- 
-- THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
-- IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
-- FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
-- AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
-- LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
-- OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
-- SOFTWARE.

flir_start_time = flir_start_time or os.time()
flir_hud_enabled = flir_hud_enabled or true
flir_last_view_type = 0

function draw_flir_hud()
    local view_type = XPLMGetDatai(XPLMFindDataRef("sim/graphics/view/view_type"))
    
    if view_type == 1026 and flir_last_view_type ~= 1026 then
        if flir_hud_enabled then
            logMsg("FLIR HUD: Camera view active")
        end
    elseif view_type ~= 1026 and flir_last_view_type == 1026 then
        if flir_hud_enabled then
            logMsg("FLIR HUD: Camera view inactive")
        end
    end
    flir_last_view_type = view_type
    
    if not flir_hud_enabled or view_type ~= 1026 then
        return
    end
    
    local zulu_time = XPLMGetDataf(XPLMFindDataRef("sim/time/zulu_time_sec"))
    local latitude = XPLMGetDataf(XPLMFindDataRef("sim/flightmodel/position/latitude"))
    local longitude = XPLMGetDataf(XPLMFindDataRef("sim/flightmodel/position/longitude"))
    local altitude_msl = XPLMGetDataf(XPLMFindDataRef("sim/flightmodel/position/elevation"))
    local ground_speed = XPLMGetDataf(XPLMFindDataRef("sim/flightmodel/position/groundspeed"))
    local heading = XPLMGetDataf(XPLMFindDataRef("sim/flightmodel/position/psi"))
    
    local hours = math.floor(zulu_time / 3600) % 24
    local minutes = math.floor((zulu_time % 3600) / 60)
    local time_string = string.format("%02d:%02d Z", hours, minutes)
    local alt_feet = math.floor(altitude_msl * 3.28084)
    local speed_knots = math.floor(ground_speed * 1.94384)
    
    graphics.set_color(0.0, 1.0, 0.0, 1.0)
    
    local status_line = string.format("▶ FLIR-25HD  %s  %.4f°N %.4f°W  MSL:%dft", 
                                     time_string, latitude, longitude, alt_feet)
    graphics.draw_string(20, SCREEN_HEIGHT - 25, status_line, "large")
    
    local nav_line = string.format("▲ HDG:%03d°  SPD:%03d kts  TRK:%03d°  VSI:+0000 fpm", 
                                  math.floor(heading), speed_knots, math.floor(heading))
    graphics.draw_string(20, SCREEN_HEIGHT - 50, nav_line, "large")
    
    local mission_time = os.time() - flir_start_time
    local mission_hours = math.floor(mission_time / 3600)
    local mission_mins = math.floor((mission_time % 3600) / 60)
    local mission_secs = mission_time % 60
    local mission_line = string.format("REC: %02d:%02d:%02d", mission_hours, mission_mins, mission_secs)
    graphics.draw_string(SCREEN_WIDTH - 180, SCREEN_HEIGHT - 25, mission_line, "large")
    
    graphics.draw_string(SCREEN_WIDTH - 180, SCREEN_HEIGHT - 50, "TGT: SCANNING", "large")
    
    graphics.draw_string(20, 80, "◆ SYS: NOMINAL  STAB: ON  IR: WHT", "large")
    graphics.draw_string(20, 105, "● ZOOM: 1.0x  FOV: WIDE  FOCUS: AUTO", "large")
    
    graphics.draw_string(20, SCREEN_HEIGHT - 75, "✈ MARITIME PATROL A319", "large")
end

function toggle_flir_hud()
    flir_hud_enabled = not flir_hud_enabled
    if flir_hud_enabled then
        logMsg("FLIR HUD: Enabled")
    else
        logMsg("FLIR HUD: Disabled")
    end
end

do_every_draw("draw_flir_hud()")
add_macro("FLIR: Toggle HUD", "toggle_flir_hud()", "", "activate")

function flir_hud_key_handler()
    if VKEY == 120 then
        toggle_flir_hud()
    end
end

do_on_keystroke("flir_hud_key_handler()")

logMsg("FLIR HUD Script Loaded - Press F9 to toggle")
