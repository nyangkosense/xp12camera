-- FLIR Camera HUD Display Script for FlyWithLua
-- Place this file in X-Plane/Resources/plugins/FlyWithLua/Scripts/
-- Simple and safe version

-- Global variables
flir_start_time = flir_start_time or os.time()

function draw_flir_hud()
    -- Only draw when in external view (likely FLIR active)
    local view_type = dataref_table("sim/graphics/view/view_type")
    
    if view_type[0] == 1026 then  -- External view
        -- Get basic aircraft data safely
        local zulu_time = dataref_table("sim/time/zulu_time_sec")[0]
        local latitude = dataref_table("sim/flightmodel/position/latitude")[0]
        local longitude = dataref_table("sim/flightmodel/position/longitude")[0]
        local altitude_msl = dataref_table("sim/flightmodel/position/elevation")[0]
        local ground_speed = dataref_table("sim/flightmodel/position/groundspeed")[0]
        local heading = dataref_table("sim/flightmodel/position/psi")[0]
        
        -- Convert zulu time to HH:MM format
        local hours = math.floor(zulu_time / 3600) % 24
        local minutes = math.floor((zulu_time % 3600) / 60)
        local time_string = string.format("%02d:%02d UTC", hours, minutes)
        
        -- Convert altitude to feet
        local alt_feet = math.floor(altitude_msl * 3.28084)
        
        -- Convert ground speed to knots
        local speed_knots = math.floor(ground_speed * 1.94384)
        
        -- Set text color to FLIR green and bigger size
        graphics.set_color(0.0, 1.0, 0.0, 1.0)
        
        -- Use bigger font size for better visibility
        local font_size = 1.5  -- Scale factor for larger text
        
        -- Top status line (bigger text)
        local status_line = string.format("FLIR CAMERA  %s  LAT:%.4f LON:%.4f  ALT:%dft", 
                                         time_string, latitude, longitude, alt_feet)
        graphics.draw_string(30, SCREEN_HEIGHT - 40, status_line, "large")
        
        -- Navigation data (bigger)
        local nav_line = string.format("HDG:%03d  SPD:%d kts  GS:%.1f m/s", 
                                      math.floor(heading), speed_knots, ground_speed)
        graphics.draw_string(30, SCREEN_HEIGHT - 70, nav_line, "large")
        
        -- Mission timer (bigger)
        local mission_time = os.time() - flir_start_time
        local mission_mins = math.floor(mission_time / 60)
        local mission_secs = mission_time % 60
        local mission_line = string.format("MISSION TIME: %02d:%02d", mission_mins, mission_secs)
        graphics.draw_string(SCREEN_WIDTH - 300, SCREEN_HEIGHT - 40, mission_line, "large")
        
        -- Target status (bigger)
        graphics.draw_string(SCREEN_WIDTH - 300, SCREEN_HEIGHT - 70, "TGT: SCANNING...", "large")
        
        -- Camera status (bigger, bottom)
        graphics.draw_string(30, 90, "ZOOM: ACTIVE  THERMAL: WHT", "large")
        
        -- Aircraft identification (bigger)
        graphics.draw_string(30, SCREEN_HEIGHT - 100, "MARITIME PATROL AIRCRAFT", "large")
        
    end
end

-- Register the drawing function (safe method)
do_every_draw("draw_flir_hud()")

-- Add macro to reset mission timer
add_macro("FLIR: Reset Mission Timer", "flir_start_time = os.time()", "", "activate")

logMsg("FLIR HUD Display Script Loaded (Safe Version)")