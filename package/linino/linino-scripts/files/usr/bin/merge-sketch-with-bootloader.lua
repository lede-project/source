#!/usr/bin/lua

if #arg ~= 1 then
  print("Missing sketch file name")
  return 1
end

local uploaded_name = arg[1]

local io = require("io")
local uploaded_sketch = io.open(uploaded_name)
if not uploaded_sketch then
  print("Unable to open file " .. uploaded_name .. " for reading")
  return 1
end

local sketch = {}
for line in uploaded_sketch:lines(uploaded_name) do
  table.insert(sketch, line)
end
uploaded_sketch:close()

--removes last line
table.remove(sketch)

for line in io.lines("/etc/arduino/Caterina-Yun.hex") do
  table.insert(sketch, line)
end

local final_sketch = io.open(uploaded_name, "w+")
if not final_sketch then
  print("Unable to open file " .. uploaded_name .. " for writing")
  return 1
end

for idx, line in ipairs(sketch) do
  line = string.gsub(line, "\n", "")
  line = string.gsub(line, "\r", "")
  line = string.gsub(line, " ", "")
  if line ~= "" then
    final_sketch:write(line)
    final_sketch:write("\n")
  end
end

final_sketch:flush()
final_sketch:close()

return 0
