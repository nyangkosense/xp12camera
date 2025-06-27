-- coordinate_debug.lua
-- FlyWithLua script to get screen-to-world coordinates
-- Place this in X-Plane/Resources/plugins/FlyWithLua/Scripts/

-- Global variables
local monitoring = false
local last_mouse_x = 0
local last_mouse_y = 0

-- Function to get current mouse screen coordinates
function get_mouse_coordinates()
    local mouse_x = MOUSE_X
    local mouse_y = MOUSE_Y
    local screen_width = SCREEN_WIDTH
    local screen_height = SCREEN_HEIGHT
    
    return mouse_x, mouse_y, screen_width, screen_height
end

-- Function to get camera information
function get_camera_info()
    local cam_x = get("sim/graphics/view/view_x")
    local cam_y = get("sim/graphics/view/view_y") 
    local cam_z = get("sim/graphics/view/view_z")
    local cam_heading = get("sim/graphics/view/view_heading")
    local cam_pitch = get("sim/graphics/view/view_pitch")
    local cam_roll = get("sim/graphics/view/view_roll")
    local view_type = get("sim/graphics/view/view_type")
    
    return cam_x, cam_y, cam_z, cam_heading, cam_pitch, cam_roll, view_type
end

-- Function to get aircraft position
function get_aircraft_info()
    local ac_x = get("sim/flightmodel/position/local_x")
    local ac_y = get("sim/flightmodel/position/local_y")
    local ac_z = get("sim/flightmodel/position/local_z")
    local ac_lat = get("sim/flightmodel/position/latitude")
    local ac_lon = get("sim/flightmodel/position/longitude")
    local ac_heading = get("sim/flightmodel/position/psi")
    
    return ac_x, ac_y, ac_z, ac_lat, ac_lon, ac_heading
end

-- Function to try screen-to-world conversion (if available in Lua)
function screen_to_world(screen_x, screen_y)
    -- Try various Lua functions that might do screen-to-world conversion
    -- This might not work, but worth trying
    
    local world_x, world_y, world_z = 0, 0, 0
    
    -- Check if these functions exist in FlyWithLua
    if XPLMWorldToLocal then
        logMsg("COORD_DEBUG: XPLMWorldToLocal available")
    end
    
    if XPLMLocalToWorld then
        logMsg("COORD_DEBUG: XPLMLocalToWorld available")
    end
    
    -- Try to get click coordinates (if available)
    local click_x = get("sim/graphics/view/click_3d_x")
    local click_y = get("sim/graphics/view/click_3d_y")
    local click_z = get("sim/graphics/view/click_3d_z")
    
    return world_x, world_y, world_z, click_x, click_y, click_z
end

-- Function to calculate ray-ground intersection
function calculate_ground_hit(cam_x, cam_y, cam_z, heading, pitch)
    local heading_rad = math.rad(heading)
    local pitch_rad = math.rad(pitch)
    
    -- Calculate ray direction
    local ray_x = math.sin(heading_rad) * math.cos(pitch_rad)
    local ray_y = math.sin(pitch_rad)
    local ray_z = math.cos(heading_rad) * math.cos(pitch_rad)
    
    -- Find ground intersection (Y = 0)
    local t = -cam_y / ray_y
    
    local hit_x = cam_x + ray_x * t
    local hit_y = 0
    local hit_z = cam_z + ray_z * t
    
    return hit_x, hit_y, hit_z, t
end

-- Main coordinate debug function
function debug_coordinates()
    local mouse_x, mouse_y, screen_w, screen_h = get_mouse_coordinates()
    local cam_x, cam_y, cam_z, cam_heading, cam_pitch, cam_roll, view_type = get_camera_info()
    local ac_x, ac_y, ac_z, ac_lat, ac_lon, ac_heading = get_aircraft_info()
    
    -- Calculate screen center offset
    local center_x = screen_w / 2
    local center_y = screen_h / 2
    local offset_x = mouse_x - center_x
    local offset_y = mouse_y - center_y
    
    -- Try screen-to-world conversion
    local world_x, world_y, world_z, click_x, click_y, click_z = screen_to_world(mouse_x, mouse_y)
    
    -- Calculate ray-ground intersection
    local hit_x, hit_y, hit_z, distance = calculate_ground_hit(cam_x, cam_y, cam_z, cam_heading, cam_pitch)
    
    -- Output all information
    logMsg("=== COORDINATE DEBUG ===")
    logMsg(string.format("SCREEN: %dx%d, Mouse: (%d,%d), Center: (%d,%d), Offset: (%d,%d)", 
           screen_w, screen_h, mouse_x, mouse_y, center_x, center_y, offset_x, offset_y))
    
    logMsg(string.format("CAMERA: Pos(%.0f,%.0f,%.0f) Angles(%.1f°,%.1f°,%.1f°) Type:%d", 
           cam_x, cam_y, cam_z, cam_heading, cam_pitch, cam_roll, view_type))
    
    logMsg(string.format("AIRCRAFT: Pos(%.0f,%.0f,%.0f) GPS(%.6f,%.6f) Heading:%.1f°", 
           ac_x, ac_y, ac_z, ac_lat, ac_lon, ac_heading))
    
    logMsg(string.format("CLICK_3D: (%.0f,%.0f,%.0f)", click_x, click_y, click_z))
    
    logMsg(string.format("RAY_HIT: (%.0f,%.0f,%.0f) Distance:%.0fm", hit_x, hit_y, hit_z, distance))
    
    logMsg("========================")
end

-- Hotkey to trigger coordinate debug
function coordinate_debug_key()
    debug_coordinates()
end

-- Hotkey to toggle monitoring
function toggle_monitoring_key()
    monitoring = not monitoring
    if monitoring then
        logMsg("COORD_DEBUG: Monitoring enabled - move mouse and press F12 for coordinates")
    else
        logMsg("COORD_DEBUG: Monitoring disabled")
    end
end

-- Continuous monitoring function
function do_every_frame()
    if monitoring then
        local mouse_x, mouse_y, _, _ = get_mouse_coordinates()
        
        -- Only update if mouse moved significantly
        if math.abs(mouse_x - last_mouse_x) > 10 or math.abs(mouse_y - last_mouse_y) > 10 then
            last_mouse_x = mouse_x
            last_mouse_y = mouse_y
            
            -- Quick coordinate display
            local cam_x, cam_y, cam_z, cam_heading, cam_pitch = get_camera_info()
            local hit_x, hit_y, hit_z = calculate_ground_hit(cam_x, cam_y, cam_z, cam_heading, cam_pitch)
            
            logMsg(string.format("MOUSE: (%d,%d) → WORLD: (%.0f,%.0f,%.0f)", 
                   mouse_x, mouse_y, hit_x, hit_y, hit_z))
        end
    end
end

-- Register hotkeys
add_macro("Coordinate Debug", "coordinate_debug_key()", "coordinate_debug_key", "deactivate")
add_macro("Toggle Coordinate Monitoring", "toggle_monitoring_key()", "toggle_monitoring_key", "deactivate")

-- Create menu items
add_menu_item("Plugins", "Coordinate Debug (F11)", "coordinate_debug_key")
add_menu_item("Plugins", "Toggle Coord Monitor (F12)", "toggle_monitoring_key")

-- Assign hotkeys
set_keydown_keycode(291, "coordinate_debug_key")  -- F11
set_keydown_keycode(292, "toggle_monitoring_key") -- F12

-- Log startup
logMsg("COORD_DEBUG: Lua script loaded!")
logMsg("COORD_DEBUG: F11 = Debug coordinates, F12 = Toggle monitoring")
logMsg("COORD_DEBUG: Activate FLIR camera, then use hotkeys to get coordinates")