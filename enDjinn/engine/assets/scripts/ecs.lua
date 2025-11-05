-- Create the ECS table
ECS = {}

-- Initialize internal state
ECS._nextEntityID = 1
ECS.Components = {}
ECS.LiveEntities = {} -- Keep track of currently active entities for easy iteration

-- Function: Create a new entity ID
function ECS.CreateEntity()
    local e = ECS._nextEntityID
    ECS._nextEntityID = ECS._nextEntityID + 1
    ECS.LiveEntities[e] = true -- Mark as live
    return e
end

-- Function: Destroy an entity
function ECS.DestroyEntity(e)
    if e == nil then return end

    -- Remove the entity from the active list
    ECS.LiveEntities[e] = nil 
    
    -- Remove the entity's components from all component tables
    for _, component_table in pairs(ECS.Components) do
        component_table[e] = nil
    end
end

-- Function: The __index metamethod to dynamically create component tables
local component_metatable = {
    __index = function(table, component_name)
        -- If a component type is accessed for the first time, create a new table for it.
        table[component_name] = {}
        return table[component_name]
    end
}
setmetatable(ECS.Components, component_metatable)


-- Function: Iterate over entities with a specific set of components
function ECS.ForEach(components, callback)
    -- Lua uses 1-indexing for the components array
    local required_count = #components
    if required_count == 0 then return end -- No components specified

    -- 1. Find the component table for the first component (candidate pool)
    local first_component_name = components[1]
    local candidate_pool = ECS.Components[first_component_name]

    if candidate_pool == nil then
        -- This component type hasn't been added to any entity yet
        return 
    end

    -- 2. Iterate over all entities in the candidate pool
    for entity_id, _ in pairs(candidate_pool) do
        local has_all_components = true
        
        -- 3. Check if the entity has all other required components
        for i = 2, required_count do
            local current_component_name = components[i]
            
            -- Check if the entity_id exists in the current component table
            if ECS.Components[current_component_name][entity_id] == nil then
                has_all_components = false
                break -- Exit the inner loop early, this entity doesn't match
            end
        end

        -- 4. If the entity has all required components, run the callback
        if has_all_components then
            callback(entity_id)
        end
    end
end

-- Expose utility function for dropping a component (setting to nil)
-- ECS.DropComponent("sprite", my_entity),
function ECS.DropComponent(component_name, entity_id)
    if ECS.Components[component_name] then
        ECS.Components[component_name][entity_id] = nil
    end
end