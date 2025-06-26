-- FLIR Camera HUD Display Script for FlyWithLua
-- Auto-toggles based on camera view with realistic military HUD

-- Global variables
flir_start_time = flir_start_time or os.time()
flir_hud_enabled = flir_hud_enabled or true
flir_last_view_type = 0

function draw_flir_hud()
    -- Auto-toggle based on camera view
    local view_type = XPLMGetDatai(XPLMFindDataRef("sim/graphics/view/view_type"))
    
    -- Auto-enable when entering camera view, disable when leaving
    if view_type == 1026 and flir_last_view_type ~= 1026 then
        flir_hud_enabled = true
        logMsg("FLIR HUD: Auto-enabled (Camera view active)")
    elseif view_type ~= 1026 and flir_last_view_type == 1026 then
        flir_hud_enabled = false
        logMsg("FLIR HUD: Auto-disabled (Camera view inactive)")
    end
    flir_last_view_type = view_type
    
    -- Only draw if HUD is enabled and in camera view
    if not flir_hud_enabled or view_type ~= 1026 then
        return
    end
    
    -- Get aircraft data
    local zulu_time = XPLMGetDataf(XPLMFindDataRef("sim/time/zulu_time_sec"))
    local latitude = XPLMGetDataf(XPLMFindDataRef("sim/flightmodel/position/latitude"))
    local longitude = XPLMGetDataf(XPLMFindDataRef("sim/flightmodel/position/longitude"))
    local altitude_msl = XPLMGetDataf(XPLMFindDataRef("sim/flightmodel/position/elevation"))
    local ground_speed = XPLMGetDataf(XPLMFindDataRef("sim/flightmodel/position/groundspeed"))
    local heading = XPLMGetDataf(XPLMFindDataRef("sim/flightmodel/position/psi"))
    
    -- Convert values
    local hours = math.floor(zulu_time / 3600) % 24
    local minutes = math.floor((zulu_time % 3600) / 60)
    local time_string = string.format("%02d:%02d Z", hours, minutes)
    local alt_feet = math.floor(altitude_msl * 3.28084)
    local speed_knots = math.floor(ground_speed * 1.94384)
    
    -- Set FLIR green color
    graphics.set_color(0.0, 1.0, 0.0, 1.0)
    
    -- Top status bar - realistic military format
    local status_line = string.format("▶ FLIR-25HD  %s  %.4f°N %.4f°W  MSL:%dft", 
                                     time_string, latitude, longitude, alt_feet)
    graphics.draw_string(20, SCREEN_HEIGHT - 25, status_line, "large")
    
    -- Navigation data
    local nav_line = string.format("▲ HDG:%03d°  SPD:%03d kts  TRK:%03d°  VSI:+0000 fpm", 
                                  math.floor(heading), speed_knots, math.floor(heading))
    graphics.draw_string(20, SCREEN_HEIGHT - 50, nav_line, "large")
    
    -- Mission timer (top right)
    local mission_time = os.time() - flir_start_time
    local mission_hours = math.floor(mission_time / 3600)
    local mission_mins = math.floor((mission_time % 3600) / 60)
    local mission_secs = mission_time % 60
    local mission_line = string.format("REC: %02d:%02d:%02d", mission_hours, mission_mins, mission_secs)
    graphics.draw_string(SCREEN_WIDTH - 180, SCREEN_HEIGHT - 25, mission_line, "large")
    
    -- Target info (top right)
    graphics.draw_string(SCREEN_WIDTH - 180, SCREEN_HEIGHT - 50, "TGT: SCANNING", "large")
    
    -- System status (bottom left)
    graphics.draw_string(20, 80, "◆ SYS: NOMINAL  STAB: ON  IR: WHT", "large")
    graphics.draw_string(20, 105, "● ZOOM: 1.0x  FOV: WIDE  FOCUS: AUTO", "large")
    
    -- Aircraft ID (bottom)
    graphics.draw_string(20, SCREEN_HEIGHT - 75, "✈ MARITIME PATROL A319", "large")
end

-- Toggle function for manual override
function toggle_flir_hud()
    flir_hud_enabled = not flir_hud_enabled
    if flir_hud_enabled then
        logMsg("FLIR HUD: Manually enabled")
    else
        logMsg("FLIR HUD: Manually disabled")
    end
end

-- Register drawing function
do_every_draw("draw_flir_hud()")

-- Add macro for manual toggle (backup)
add_macro("FLIR: Toggle HUD", "toggle_flir_hud()", "", "activate")

logMsg("FLIR HUD Script Loaded - Auto-toggles with camera view, manual toggle available")
