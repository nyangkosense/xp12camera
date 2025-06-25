-- FLIR Camera HUD Display Script for FlyWithLua
-- Simplified version to prevent engine crashes

-- Global variables
flir_start_time = flir_start_time or os.time()

function draw_flir_hud()
    -- Simple check for external view
    local view_type = XPLMGetDatai(XPLMFindDataRef("sim/graphics/view/view_type"))
    
    if view_type == 1026 then  -- External view
        -- Get basic data using direct dataref calls
        local zulu_time = XPLMGetDataf(XPLMFindDataRef("sim/time/zulu_time_sec"))
        local latitude = XPLMGetDataf(XPLMFindDataRef("sim/flightmodel/position/latitude"))
        local longitude = XPLMGetDataf(XPLMFindDataRef("sim/flightmodel/position/longitude"))
        local altitude_msl = XPLMGetDataf(XPLMFindDataRef("sim/flightmodel/position/elevation"))
        local ground_speed = XPLMGetDataf(XPLMFindDataRef("sim/flightmodel/position/groundspeed"))
        local heading = XPLMGetDataf(XPLMFindDataRef("sim/flightmodel/position/psi"))
        
        -- Convert zulu time to HH:MM format
        local hours = math.floor(zulu_time / 3600) % 24
        local minutes = math.floor((zulu_time % 3600) / 60)
        local time_string = string.format("%02d:%02d UTC", hours, minutes)
        
        -- Convert altitude to feet
        local alt_feet = math.floor(altitude_msl * 3.28084)
        
        -- Convert ground speed to knots
        local speed_knots = math.floor(ground_speed * 1.94384)
        
        -- Set text color to FLIR green
        graphics.set_color(0.0, 1.0, 0.0, 1.0)
        
        -- Draw HUD text
        local status_line = string.format("FLIR CAMERA  %s  LAT:%.4f LON:%.4f  ALT:%dft", 
                                         time_string, latitude, longitude, alt_feet)
        graphics.draw_string(30, SCREEN_HEIGHT - 40, status_line, "large")
        
        local nav_line = string.format("HDG:%03d  SPD:%d kts  GS:%.1f m/s", 
                                      math.floor(heading), speed_knots, ground_speed)
        graphics.draw_string(30, SCREEN_HEIGHT - 70, nav_line, "large")
        
        local mission_time = os.time() - flir_start_time
        local mission_mins = math.floor(mission_time / 60)
        local mission_secs = mission_time % 60
        local mission_line = string.format("MISSION TIME: %02d:%02d", mission_mins, mission_secs)
        graphics.draw_string(SCREEN_WIDTH - 300, SCREEN_HEIGHT - 40, mission_line, "large")
        
        graphics.draw_string(SCREEN_WIDTH - 300, SCREEN_HEIGHT - 70, "TGT: SCANNING...", "large")
        graphics.draw_string(30, 90, "ZOOM: ACTIVE  THERMAL: WHT", "large")
        graphics.draw_string(30, SCREEN_HEIGHT - 100, "MARITIME PATROL AIRCRAFT", "large")
    end
end

-- Register drawing function
do_every_draw("draw_flir_hud()")

logMsg("FLIR HUD Simple Script Loaded")