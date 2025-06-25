-- FLIR Camera HUD Display Script for FlyWithLua
-- Place this file in X-Plane/Resources/plugins/FlyWithLua/Scripts/
-- This provides professional text overlay for the FLIR camera system

-- Only display when FLIR camera is active
-- You can detect this by checking if the external view is active and camera controls are locked

if PLANE_ICAO == nil then PLANE_ICAO = "UNKNOWN" end

function draw_flir_hud()
    -- Only draw when in external view (indicating FLIR might be active)
    local view_type = XPLMGetDataf("sim/graphics/view/view_type")
    
    if view_type == 1026 then  -- External view
        -- Get current aircraft data
        local zulu_time = XPLMGetDataf("sim/time/zulu_time_sec")
        local latitude = XPLMGetDataf("sim/flightmodel/position/latitude")
        local longitude = XPLMGetDataf("sim/flightmodel/position/longitude")
        local altitude_msl = XPLMGetDataf("sim/flightmodel/position/elevation")
        local altitude_agl = XPLMGetDataf("sim/flightmodel/position/y_agl")
        local ground_speed = XPLMGetDataf("sim/flightmodel/position/groundspeed")
        local heading = XPLMGetDataf("sim/flightmodel/position/psi")
        
        -- Convert zulu time to HH:MM format
        local hours = math.floor(zulu_time / 3600) % 24
        local minutes = math.floor((zulu_time % 3600) / 60)
        local time_string = string.format("%02d:%02d UTC", hours, minutes)
        
        -- Convert altitude to feet
        local alt_feet = math.floor(altitude_msl * 3.28084)
        local agl_feet = math.floor(altitude_agl * 3.28084)
        
        -- Convert ground speed to knots
        local speed_knots = math.floor(ground_speed * 1.94384)
        
        -- Set text color to FLIR green
        glColor4f(0.0, 1.0, 0.0, 1.0)
        
        -- Top status line
        local status_line = string.format("FLIR CAMERA  %s  %.4f°N %.4f°W  %dft MSL  %dft AGL", 
                                         time_string, latitude, math.abs(longitude), alt_feet, agl_feet)
        draw_string(30, SCREEN_HEIGHT - 30, status_line)
        
        -- Navigation data
        local nav_line = string.format("HDG: %03d°  SPD: %d kts  GS: %.1f m/s", 
                                      math.floor(heading), speed_knots, ground_speed)
        draw_string(30, SCREEN_HEIGHT - 50, nav_line)
        
        -- Aircraft identification
        local aircraft_line = string.format("ACFT: %s  TAIL: %s", 
                                           PLANE_ICAO, get_aircraft_tailnumber())
        draw_string(30, SCREEN_HEIGHT - 70, aircraft_line)
        
        -- Mission timer (time since FLIR activation)
        local mission_time = os.time() - (flir_start_time or os.time())
        local mission_mins = math.floor(mission_time / 60)
        local mission_secs = mission_time % 60
        local mission_line = string.format("MISSION TIME: %02d:%02d", mission_mins, mission_secs)
        draw_string(SCREEN_WIDTH - 250, SCREEN_HEIGHT - 30, mission_line)
        
        -- Target information (if available)
        draw_string(SCREEN_WIDTH - 250, SCREEN_HEIGHT - 50, "TGT: SCANNING...")
        
        -- Camera status
        local zoom_level = 1.0  -- Would read from plugin if dataref available
        local thermal_mode = "WHT"  -- Would read from plugin if dataref available
        local camera_line = string.format("ZOOM: %.1fx  THERMAL: %s", zoom_level, thermal_mode)
        draw_string(30, 70, camera_line)
        
    end
end

function get_aircraft_tailnumber()
    local tailnum = XPLMGetDatab("sim/aircraft/view/acf_tailnum")
    if tailnum and string.len(tailnum) > 0 then
        return tailnum
    else
        return "UNKNOWN"
    end
end

-- Initialize mission start time when script loads
if flir_start_time == nil then
    flir_start_time = os.time()
end

-- Register the drawing function
do_every_draw("draw_flir_hud()")

-- Add hotkey to reset mission timer
add_macro("FLIR: Reset Mission Timer", "flir_start_time = os.time()", "", "")

logMsg("FLIR HUD Display Script Loaded - Professional text overlay active")