-- This is a custom writer for pandoc producing pmWiki format.
-- Created by Andrew Apted, May 2018, based on sample.lua.
--
-- Invoke with: pandoc -t pmwiki.lua

-- Character escaping
local function escape(s, in_attribute)
    -- FIXME
    return s
end

-- Helper function to convert an attributes table into
-- a string that can be put into HTML tags.
local function attributes(attr)
  local attr_table = {}
  for x,y in pairs(attr) do
    if y and y ~= "" then
      table.insert(attr_table, ' ' .. x .. '="' .. escape(y,true) .. '"')
    end
  end
    return table.concat(attr_table)
end

-- Blocksep is used to separate block elements.
function Blocksep()
    return "\n"
end

-- This function is called once for the whole document.
-- body is a single string.
function Doc(body, metadata, variables)
    return body .. '\n'
end

-- The functions that follow render corresponding pandoc elements.
-- s is always a string, attr is always a table of attributes, and
-- items is always an array of strings (the items in a list).
-- Comments indicate the types of other variables.

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

function Subscript(s)
    -- TODO : this represents the :kbd: elements
    return "KBD:" .. s
end

function Superscript(s)
    -- TODO : this represents the :download: elements
    return "FILE:" .. s
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
    -- FIXME
    return "LINK:<" .. src .. "><" .. s .. ">"
end

function Image(s, src, tit, attr)
    -- FIXME
    return "IMG:<" .. src .. "><" .. s .. ">"
end

function CaptionedImage(src, tit, caption, attr)
    -- fix up some image filenames...
    if string.match(src, "capture_") then
        local base = os.getenv("PM_BASE")
        if base then
            src = string.gsub(src, "capture_", base .. "_")
        end
    end

    -- FIXME
    return "CAPIMG:<" .. src .. "><" .. caption .. ">"
end

function Code(s, attr)
    return "@@" .. escape(s) .. "@@"
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

-- lev is an integer, the header level.
function Header(lev, s, attr)
    if lev <= 1 then
        return "(:notitle:)\n" .. "!" .. s
    elseif lev <= 2 then
        return "!!" .. s
    else
        return "!!!" .. s
    end
end

function BlockQuote(s)
    return "(:table border=1 bgcolor=#eeeeee:)\n" .. "(:cell:)\n" .. s .. "(:tableend:)"
end

function HorizontalRule()
    return "----"
end

function CodeBlock(s, attr)
    return "@@[@" .. escape(s) .. "@]@@"
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
        table.insert(buffer, ":" .. k .. ":" ..  table.concat(v, "\n"))
        end
    end
    return table.concat(buffer, "\n") .. "\n"
end

-- Convert pandoc alignment to something HTML can use.
-- align is AlignLeft, AlignRight, AlignCenter, or AlignDefault.
function html_align(align)
  if align == 'AlignLeft' then
    return 'left'
  elseif align == 'AlignRight' then
    return 'right'
  elseif align == 'AlignCenter' then
    return 'center'
  else
    return 'left'
  end
end

-- Caption is a string, aligns is an array of strings,
-- widths is an array of floats, headers is an array of
-- strings, rows is an array of arrays of strings.
function Table(caption, aligns, widths, headers, rows)
    -- not needed
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

