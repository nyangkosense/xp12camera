-- simple_coords.lua
-- Minimal coordinate debug script

function show_coordinates()
    local mouse_x = MOUSE_X or 0
    local mouse_y = MOUSE_Y or 0
    local screen_w = SCREEN_WIDTH or 1920
    local screen_h = SCREEN_HEIGHT or 1080
    
    local cam_x = get("sim/graphics/view/view_x")
    local cam_y = get("sim/graphics/view/view_y")
    local cam_z = get("sim/graphics/view/view_z")
    local cam_heading = get("sim/graphics/view/view_heading")
    local cam_pitch = get("sim/graphics/view/view_pitch")
    
    logMsg("COORDS: Screen " .. screen_w .. "x" .. screen_h)
    logMsg("COORDS: Mouse (" .. mouse_x .. "," .. mouse_y .. ")")
    logMsg("COORDS: Camera (" .. cam_x .. "," .. cam_y .. "," .. cam_z .. ")")
    logMsg("COORDS: Angles " .. cam_heading .. "°," .. cam_pitch .. "°")
end

add_macro("Show Coords", "show_coordinates()", "show_coordinates", "deactivate")
set_keydown_keycode(290, "show_coordinates")  -- F10

logMsg("Simple coordinate script loaded - Press F10")