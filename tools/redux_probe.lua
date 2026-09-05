-- Registers a web handler so Lua can be run in PCSX-Redux over HTTP.
-- Loaded at startup with: pcsx-redux -dofile tools/redux_probe.lua
--
-- /api/v1/lua/<name> dispatches to PCSX.WebServer.Handlers[<name>], so a
-- handler must exist before anything can be run remotely. The handler is
-- given urlData/method/headers/form and must return a string.
--
-- Code arrives in the query string, not the body: `form` is populated only
-- for multipart requests, and then from the MIME part *headers* rather than
-- their contents, so it cannot carry a variable of our own.
--
--   ?code=<urlencoded>     run it now, return the result
--   ?append=<urlencoded>   append to a buffer, for scripts too long for a URL
--   ?run=1                 run the buffer
--   ?reset=1               clear the buffer

PCSX.WebServer = PCSX.WebServer or {}
PCSX.WebServer.Handlers = PCSX.WebServer.Handlers or {}

local buffer = ''

local function urldecode(text)
    text = text:gsub('+', ' ')
    return (text:gsub('%%(%x%x)', function(hex)
        return string.char(tonumber(hex, 16))
    end))
end

local function parse_query(query)
    local params = {}
    for pair in (query or ''):gmatch('[^&]+') do
        local key, value = pair:match('^([^=]*)=?(.*)$')
        if key ~= nil and #key > 0 then
            params[urldecode(key)] = urldecode(value)
        end
    end
    return params
end

local function run(source)
    local chunk, compileError = load(source)
    if chunk == nil then return 'error: ' .. tostring(compileError) end
    local ok, result = pcall(chunk)
    if not ok then return 'error: ' .. tostring(result) end
    if result == nil then return '' end
    return tostring(result)
end

PCSX.WebServer.Handlers.eval = function(request)
    local urlData = request.urlData or {}
    local params = parse_query(urlData.query)
    if params.reset ~= nil then
        buffer = ''
        return 'reset'
    end
    if params.append ~= nil then
        buffer = buffer .. params.append
        return 'buffered ' .. tostring(#buffer)
    end
    if params.run ~= nil then return run(buffer) end
    if params.code ~= nil then return run(params.code) end
    return 'error: expected code, append, run or reset'
end

return 'probe handler installed'
