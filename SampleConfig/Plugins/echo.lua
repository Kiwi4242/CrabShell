local M = {}

M.name = "echo"

function M.run(arg)
    local i = 1
    local expand = ''
    while (i <= #arg) do
        local i0, i1 = string.find(arg, '%%[ %w]+%%', i)
        if (i0) then
            local var = string.sub(arg, i0+1, i1-1)
            local env = os.getenv(var)
            if (env) then
                expand = expand .. env
            else
                expand = expand .. string.sub(arg, i, i1)
            end
            i = i1 + 1
        else
            local v = string.sub(arg, i, #arg)
            expand = expand .. v
            break
        end
    end
    -- look for redirection, should check for ""
    local i, j = string.find(arg, '>')
    if (i) then
        local cmd = string.sub(arg, 1, i-1)
        local outFile = string.sub(arg, i+1)
        fs = io.open(outFile, 'w')
        fs:write(cmd, '\n')
        fs:close()
        -- print('Writing ', cmd, ' to ', outFile)
    else
        print(expand)
    end
end

return M
