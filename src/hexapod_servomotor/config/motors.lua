#!/usr/bin/lua

--[[

--]]


Motors = {}

local LegsComponents = {
    "coxa",
    "femur",
    "tibia"
}

local function table_contains(tbl, x)
    local found = false
    for _, v in pairs(tbl) do
        if v == x then
            found = true
        end
    end
    return found
end

local function generate_prefix_name(side, name)
    local tmpname = ""
    local _side = string.upper(side)
    assert(_side == "L" or _side == "R",
        "[ERROR] invalid prefix for side: \"" .. side .. "\". It should be \"L\" or \"R\"")
    if _side == "L" then
        tmpname = _side .. "_" .. name
    elseif _side == "R" then
        tmpname = _side .. "_" .. name
    else
        error("[ERROR] invalid prefix for side: \"" .. side .. "\". It should be \"L\" or \"R\"")
    end
    return tmpname
end

local postfix_index = 1
local count = 0
local postfixies = { "A", "B", "C" }

local function reset_counter()
    if not (count < 3) then
        count = count % 3
    end
end

local function generate_postfix_name(name)
    if not (count < 3) then
        postfix_index = postfix_index + 1
    end
    if postfix_index > #postfixies then
        postfix_index = postfix_index % #postfixies
    end
    local complete_name = (name .. postfixies[postfix_index])
    reset_counter()
    count = count + 1
    return complete_name
end

local function generate_name(side, name)
    local _tmpname = generate_prefix_name(side, name)
    if _tmpname == "" then
        return
    end
    local _complete = generate_postfix_name(_tmpname)
    return _complete
end

function generate_motors_configuration(num_motors, side --[[string]])
    for i = 1, num_motors, 1 do
        local pin = (i - 1)
        local _index_leg = (pin % #LegsComponents) + 1
        local name = generate_name(side, LegsComponents[_index_leg])
        table.insert(Motors, { name, pin })
    end
end

function print_recap()
    print("pass here")
    for _, motor in pairs(Motors) do
        for k, value in pairs(motor) do
            if k == 1 then
                config = string.format("Motor: name: %s", value)

            else
                config = config .. string.format("\t pin: %3d", value)
            end
        end
        print(config)
    end
end
