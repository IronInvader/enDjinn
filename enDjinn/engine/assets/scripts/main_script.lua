-- main_script.lua

-- Component type for player-specific data
ECS.Components.PlayerControl = {}
-- Component type for attaching scripts to entities
ECS.Components.script = {}

-- Table to track key states for sound playback
local key_was_down = {}

-- Global constant for player speed
local PLAYER_SPEED = 30.0 -- Increased for better visibility
local SHIFT_KEY_SOUND_NAME = "ding_sound"

-- 1. Asset loading and Entity Creation (Executed once at startup)
print("--- Lua ECS Setup Started ---")
ResourceManager_LoadImage("player_texture", "sprites/player_sprite.jpg")
ResourceManager_LoadImage("background_texture", "sprites/bg.jpg")
SoundManager_LoadSound(SHIFT_KEY_SOUND_NAME, "sounds/ding.wav")
-- Note: Though the current sprites and sounds are jokey, they work with any type of image as long as it's described correctly in the path.
-- I would change it to find all assets in a folder, but that requires C++ changes too close to the deadline

print("Assets loading requested.")

-- Create the background entity
local background_entity = ECS.CreateEntity()
ECS.Components.Sprite[background_entity] = Sprite.new()
ECS.Components.Sprite[background_entity].textureName = "background_texture"
ECS.Components.Sprite[background_entity].position = vec2.new(0.0, 0.0)
ECS.Components.Sprite[background_entity].scale = vec2.new(1280.0, 720.0)
ECS.Components.Sprite[background_entity].z = 1.0 -- Furthest back

-- Create the player entity
local player_entity = ECS.CreateEntity()
ECS.Components.Sprite[player_entity] = Sprite.new()
ECS.Components.Sprite[player_entity].textureName = "player_texture"
ECS.Components.Sprite[player_entity].position = vec2.new(0.0, 0.0)
ECS.Components.Sprite[player_entity].scale = vec2.new(20.0, 20.0)
ECS.Components.Sprite[player_entity].z = 0.5 -- In front of background

-- Attach a control component to the player
ECS.Components.PlayerControl[player_entity] = { speed = PLAYER_SPEED }

-- Attach a script component to the player
ECS.Components.script[player_entity] = { name = "PlayerUpdate" }

print("--- Lua ECS Setup Complete ---")


-- 2. Define the PlayerUpdate system (function)
-- This function will be called by the C++ ScriptManager for every entity with a 'script' component whose name is 'PlayerUpdate'
function PlayerUpdate(entity_id, dt)

    -- Get the components for this entity
    local sprite = ECS.Components.Sprite[entity_id]
    local control = ECS.Components.PlayerControl[entity_id]
    
    if not sprite or not control then return end

    local move_amount = control.speed * dt

    
    -- Update position based on input
    -- These are continuous changes, so they use IsKeyPressed
    if IsKeyPressed(KEYBOARD.W) then
        sprite.position.y = sprite.position.y + move_amount
    end
    if IsKeyPressed(KEYBOARD.S) then
        sprite.position.y = sprite.position.y - move_amount
    end
    if IsKeyPressed(KEYBOARD.A) then
        sprite.position.x = sprite.position.x - move_amount
    end
    if IsKeyPressed(KEYBOARD.D) then
        sprite.position.x = sprite.position.x + move_amount
    end

    -- This has a different function to ensure that the sounds only play once per key press
    if IsKeyTriggered(KEYBOARD.LEFT_SHIFT) then
        SoundManager_PlaySound(SHIFT_KEY_SOUND_NAME, 0.8, 0.0, 0)
    end
    
    if IsKeyPressed(KEYBOARD.ESCAPE) then
        QuitGame()
    end
end

function UpdateAllSystems(dt)
    -- This function now contains the logic that was in the C++ UpdateScriptSystem, an old function that's outdated now
    -- This is the main function being called in the loop.
    
    -- 1. Define the component query
    local components_to_query = { "script" }

    -- 2. Run the ECS query
    ECS.ForEach(components_to_query, function(entity_id)
        -- This anonymous function is the callback for ForEach
        
        -- 3. Get the script component for the current entity
        local script_comp = ECS.Components.script[entity_id]
        
        -- 4. Check if the component and its 'name' field are valid
        if script_comp and script_comp.name then
            -- 5. Look up the global function using the name string
            local func_to_run = _G[script_comp.name] -- _G is the global table
            
            -- 6. If it's a valid function, run it
            if type(func_to_run) == "function" then
                func_to_run(entity_id, dt)
            end
        end
    end)
end

function IsKeyTriggered(key)
    -- 1. Check if the key is pressed right now.
    if IsKeyPressed(key) then
        -- 2. If it is, check if we remember it being down last frame.
        if not key_was_down[key] then
            -- It's down now, but wasn't down before. This is the trigger!
            -- A. Remember that it's down for the next frame.
            key_was_down[key] = true 
            -- B. Signal that the trigger just happened.
            return true
        end
        -- If we get here, it means the key is down now AND was down last frame (being held).
        -- So we do nothing and will return false at the end.
    else
        -- 3. The key is not pressed right now. Remember this for the next frame.
        key_was_down[key] = false
    end

    -- 4. If we haven't returned true yet, it wasn't a trigger this frame.
    return false
end