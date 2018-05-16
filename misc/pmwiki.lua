-- This is a custom writer for pandoc producing pmWiki format.
-- Created by Andrew Apted, May 2018, based on sample.lua.
-- This script only works on the Eureka user manual
-- (in conjunction with the pmconvert.sh script).

WEBSITE = "http://eureka-editor.sourceforge.net/"
DL_BASE = "http://sourceforge.net/projects/eureka-editor/files/Misc/Samples/"

VERBOSE = false
ANCHOR  = 1

BACK_LINK = "[-[[User.Index | back to the Index]]-]\n"

-- Character escaping
local function escape(s, in_attribute)
    -- FIXME
    return s
end

local function copy_file(src, dest)
    -- NOTE: this will only work on Unix
    local cmd = string.format("cp -n '%s' '%s'", src, dest)

    if VERBOSE then
        io.stderr:write(cmd .. "\n")
    end

    os.execute(cmd)
end

-- stores a link into the "pm/Index" file.
-- returns the anchor name, or nil if nothing happened.
local function add_index_link(tit)
    -- skip it for the cookbook pages
    local pm_file = os.getenv("PM_FILE")
    if pm_file and string.match(pm_file, "cookbook/") then
        return
    end

    local pm_page = os.getenv("PM_PAGE")
    if not pm_page then
        return
    end

    local fp = io.open("pm/Index", "a")
    if not fp then
        if true then
            io.stderr:write("Failure appending to pm/Index\n")
        end
        return
    end

    local anchor = "anchor" .. ANCHOR
    ANCHOR = ANCHOR + 1

    fp:write(string.format("** [[%s#%s | %s]]\n", pm_page, anchor, tit))
    fp:close()

    return anchor
end

-- Blocksep is used to separate block elements.
function Blocksep()
    return "\n"
end

-- This function is called once for the whole document
-- (at the very end).  body is a single string.
function Doc(body, metadata, variables)
    -- add links back to the index page (the TOC)
    body = BACK_LINK .. "\n" .. body .. "\n\n" .. BACK_LINK

	-- disable the usual pmWiki title
    body = "(:notitle:)\n" .. body

    -- convert to a raw pmWiki page file.
    -- several characters need to be escaped...
    body = string.gsub(body, "%%", "%%25")
    body = string.gsub(body, "<",  "%%3c")
    body = string.gsub(body, "\n", "%%0a")

    return "version=pmwiki-2.2.43 urlencoded=1\n" ..
           "charset=UTF-8\n" ..
           "text=" .. body .. "\n"
end

-- The functions that follow render corresponding pandoc elements.
-- s is always a string, attr is always a table of attributes, and
-- items is always an array of strings (the items in a list) except
-- for DefinitionList.

function Str(s)
    return escape(s)
end

function Space()
    return " "
end

function SoftBreak()
    return "\n"
end

function LineBreak()
    return "\\"
end

function Emph(s)
    return "''" .. s .. "''"
end

function Strong(s)
    return "'''" .. s .. "'''"
end

-- this represents the :kbd: elements
function Subscript(s)
    if s == nil or s == "" then
        return ""
    end

    -- handle menu references like "File -> Open Map"
    if string.match(s, "->") then
        return Strong(s)
    end

    local result = ""

    -- check for shift/control/alt
    local low = string.lower(s)

    if string.match(low, "^shift[ -_]") then
        s = string.sub(s, 7)
        return Subscript("shift") .. "-" .. Subscript(s)
    end

    if string.match(low, "^control[ -_]") then
        s = string.sub(s, 9)
        return Subscript("ctrl") .. "-" .. Subscript(s)
    end

    if string.match(low, "^ctrl[ -_]") then
        s = string.sub(s, 6)
        return Subscript("ctrl") .. "-" .. Subscript(s)
    end

    if string.match(low, "^alt[ -_]") then
        s = string.sub(s, 5)
        return Subscript("alt") .. "-" .. Subscript(s)
    end

    -- check for ranges
    if #s == 3 and string.sub(s, 2, 2) == "-" then
        local s1 = string.sub(s, 1, 1)
        local s2 = string.sub(s, 3, 3)
        return Subscript(s1) .. ".." .. Subscript(s2)
    end

    -- handle some other oddities
    if s == ",." then
        return Subscript(",") .. " " .. Subscript(".")
    end

    if s == "[]" then
        return Subscript("[") .. " " .. Subscript("]")
    end

    if low == "spacebar" then
        s = "space"
    end

    -- an awk scripts provide this
    -- (because it seems pandoc has an issue with `\`` in RST)
    if s == "backquote" then
        s = "`"
    end

    -- keys like "shift", "tab" (etc) should be uppercase
    if #s >= 2 then
        s = string.upper(s)
    end

    return "%key%" .. s .. "%%"
end

-- this represents the :download: elements
function Superscript(s)
    local url = DL_BASE .. s .. "/download"

    -- copy the file too
    local dir = os.getenv("PM_FILE")
    if dir then
        dir = string.match(dir, "^(.*/)[^/]*$")
    end
    if dir then
        local src = dir .. s
        local dest = "pm/download/" .. s

        copy_file(src, dest)
    end

    return "[[" .. url .. " | " .. s .. "]]"
end

function SmallCaps(s)
    -- not needed
    return s
end

function Strikeout(s)
    -- probably not needed
    return s
end

function Link(s, src, tit, attr)
    -- currently only external links are handled properly.
    -- internal links (to other wiki pages) are not supported
    -- (and probably not needed).

    -- handle :target: weirdness in cookbook/traps
    if string.match(src, "_images/") then
        return s
    end

    return "[[" .. src .. " | " .. s .. "]]"
end

function Image(s, src, tit, attr)
    return CaptionedImage(src, tit, s, attr)
end

function CaptionedImage(src, tit, caption, attr)
    local url = src

    -- fix up some image filenames...
    if string.match(url, "capture_") then
        local base = os.getenv("PM_BASE")
        if base then
            url = string.gsub(url, "capture_", base .. "_")
        end
    end

    if string.match(url, "http:/") or string.match(url, "https:/") then
        -- Ok, we have an absolute URL
    else
        -- copy the image file too
        local dir = os.getenv("PM_FILE")
        if dir then
            dir = string.match(dir, "^(.*/)[^/]*$")
        end
        if dir then
            -- source must be original name, dest must be modified name
            local src_name = dir .. src
            local dest_name = "pm/user/" .. url

            copy_file(src_name, dest_name)
        end

        -- make an absolute URL
        url = WEBSITE .. "user/" .. url
    end

    if caption == "" or caption == "image" then
        -- ignore it
    else
        url = url .. '"' .. caption .. '"'
    end

    return url
end

function Code(s, attr)
    return "@@" .. escape(s) .. "@@"
end

function CodeBlock(s, attr)
    return "(:table id=codebox:)\n" .. "(:cell:)\n" .. escape(s) .. "\n(:tableend:)"
end

function InlineMath(s)
    -- not needed
    return escape(s)
end

function DisplayMath(s)
    -- not needed
    return escape(s)
end

function Note(s)
    -- not needed (footnotes)
    return ""
end

function Span(s, attr)
    -- not needed
    return s
end

function RawInline(format, str)
  if format == "html" then
    return str
  end
end

function Cite(s, cs)
    -- not needed
    return s
end

function Plain(s)
    return s
end

function Para(s)
    return s .. "\n"
end

-- lev is an integer >= 1
function Header(lev, s, attr)
    if lev <= 1 then
        return "!" .. s
    elseif lev <= 2 then
        local anchor = add_index_link(s)
        if anchor then
            return "[[#" .. anchor .. "]]\n" .. "!!" .. s
        end
        return "!!" .. s
    else
        return "!!!" .. s
    end
end

function BlockQuote(s)
    local class = "noticebox"

    local s2 = s
    if #s2 > 100 then
        s2 = string.sub(s, 1, 100)
    end
    s2 = string.lower(s2)

    if string.match(s2, "warning") then
        class = "warningbox"
        s = string.gsub(s, "warning", "Warning:", 1)
    elseif string.match(s2, "note") then
        s = string.gsub(s, "note", "Note:", 1)
    end

    return "(:table id=" .. class .. ":)\n" .. "(:cell:)\n" .. s .. "(:tableend:)"
end

function HorizontalRule()
    return "----"
end

function BulletList(items)
    local buffer = {}
    for _, item in pairs(items) do
        table.insert(buffer, "* " .. item)
    end
    return table.concat(buffer, "\n") .. "\n"
end

function OrderedList(items)
    local buffer = {}
    for _, item in pairs(items) do
        table.insert(buffer, "# " .. item)
    end
    return table.concat(buffer, "\n") .. "\n"
end

function DefinitionList(items)
    local buffer = {}
    for _,item in pairs(items) do
        for k, v in pairs(item) do
        table.insert(buffer, "\n:" .. k .. ": " ..  table.concat(v, "\n"))
        end
    end
    return table.concat(buffer, "\n") .. "\n"
end

-- Caption is a string, aligns is an array of strings,
-- widths is an array of floats, headers is an array of
-- strings, rows is an array of arrays of strings.
function Table(caption, aligns, widths, headers, rows)
    -- not needed (thankfully!)
    return ""
end

function RawBlock(format, str)
  if format == "html" then
    return str
  end
end

function Div(s, attr)
    -- probably not needed
    return "(:div:)" .. s .. "(:divend:)"
end

-- The following code will produce runtime warnings when you haven't defined
-- all of the functions you need for the custom writer, so it's useful
-- to include when you're working on a writer.
local meta = {}
meta.__index =
  function(_, key)
    io.stderr:write(string.format("WARNING: Undefined function '%s'\n",key))
    return function() return "" end
  end
setmetatable(_G, meta)

