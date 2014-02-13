gpio = require("GPIO")

function FindOutputIndex(X, Y, Z)
 local ret = nil
 for i, output in pairs(outputs) do
  if (output ['X'] == X and output ['Y'] == Y and output ['Z'] == Z) then
   ret = i
   break
  end
 end
 return ret
end

function FindInputIndex(X, Y, Z)
 local ret = nil
 for i, input in pairs(inputs) do
  if (input ['X'] == X and input ['Y'] == Y and input ['Z'] == Z) then
   ret = i
   break
  end
 end
 return ret
end

function FindInfoSign(X, Y, Z)
 local ret = nil
 for i, sign in pairs(infosigns) do
  if (sign['X'] == X and sign['Y'] == Y and sign['Z'] == Z) then
   ret = i
   break
  end
 end
 return ret
end

function FindLastPlacedOfPlayer(X, Y, Z)
 local ret = nil
 for plname, block in pairs(last_placed_by_player) do
  if (block['X'] == X and block['Y'] == Y and block['Z'] == Z) then
   ret = plname
   break
  end
 end
 return ret
end

function FindInArray(val, ar)
 ret = nil
 for k,v in pairs(ar) do
  if (v == val) then
   ret = k
   break
  end
 end
 return ret
end

function ArrayValsToString(ar)
 ret = ""
 for k,v in pairs(ar) do
  ret = ret .. v .. ", "
 end
 if (ret ~= "") then
  ret = string.sub(ret, 1, #ret-2)	--get rid of last coma and space
 end
 return ret
end

function LogIO(Split, Player)
 LOG(StrIO())
 return true
end

function TellIO(Split, Player)
 SendMessage(Player, StrIO())
 return true
end

function StrIO()
 ret = "IOs:\n"
 for i, input in pairs(inputs) do
  ret = ret .. ("Input " .. i .. ":\n")
  for var, val in pairs(input) do
   ret = ret .. ("- " .. var .. " = " .. val .. "\n")
  end
 end
 for i, output in pairs(outputs) do
  ret = ret .. ("Output " .. i .. ":\n")
  for var, val in pairs(output) do
   ret = ret .. ("- " .. var .. " = " .. val .. "\n")
  end
 end
 return ret
end

function UpdateSigns(World)
 UpdateTemp()
 UpdateCPU()
 UpdateRAM()
 for key,sign in pairs(infosigns) do
  local type = World:GetBlock(sign['X'], sign['Y'], sign['Z'])
  if (type ~= 63 and type ~= 68) then	--to prevent errors when sign has been removed
   infosigns[key] = nil
  else
   if (sign['INFO'] == "TEMP") then
    World:SetSignLines(sign['X'], sign['Y'], sign['Z'], "CPU temperature", TEMP, "deg C", "")
   else if (sign['INFO'] == "CPU") then
    World:SetSignLines(sign['X'], sign['Y'], sign['Z'], "CPU usage:", CPU, "%", "")
   else
    World:SetSignLines(sign['X'], sign['Y'], sign['Z'], "RAM usage:", RAM_used .. "MB", "of", RAM_total .. "MB")  end
   end
  end
 end
end

function writePin(pin, val)
 pin = pin*1

 --for i, v in pairs(pinMap) do
 -- if (i == pin) then
 --  gpio.setup(v, gpio.OUT)
 --  gpio.output(v, val)
 --  break
 -- end
 --end

 gpio.setup(pin, gpio.OUT)
 gpio.output(pin, val)
end

function readPin(pin)
 --LOG(pin)
 --LOG(pinMap[pin])
 --gpio.setup(pinMap[pin], gpio.IN)
 --return (gpio.input(pinMap[pin]))
 gpio.setup(pin, gpio.IN)
 return (gpio.input(pin))
end


--Periodically updated stuff
TEMP = nil
CPU = nil
RAM_used = nil
RAM_total = nil

function UpdateTemp()
 local t = io.open("/sys/class/thermal/thermal_zone0/temp")
 local tmp = t:read("*all")
 t:close()
 TEMP = string.format("%g", tonumber(tmp)/1000)
end


--CPU calc stuff
last_total_jiffies = nil
last_work_jiffies = nil

function UpdateCPU()
 local p = io.open("/proc/stat")
 local tmp = p:read("*all")
 local total = 1
 local work = 0
 p:close()
 v1, v2, v3, v4, v5, v6, v7 = tmp:match("(%d+) (%d+) (%d+) (%d+) (%d+) (%d+) (%d+)")
 local total_jiffies = v1+v2+v3+v4+v5+v6+v7
 local work_jiffies = v1+v2+v3
 if (last_total_jiffies ~= nil and last_work_jiffies ~= nil) then
  total = total_jiffies - last_total_jiffies
  work = work_jiffies - last_work_jiffies
 end
 last_total_jiffies = total_jiffies
 last_work_jiffies = work_jiffies
 CPU = string.format("%g", work/total*100)
end

function UpdateRAM()
 local f = io.open("/proc/meminfo")
 local tmp = f:read("*all")
 local RAM_buffers, RAM_cached
 f:close()
 RAM_total, RAM_used, RAM_buffers, RAM_cached = tmp:match("(%d+).-(%d+).-(%d+).-(%d+)")
 RAM_used = string.format("%g", (RAM_total - RAM_used - RAM_buffers - RAM_cached)/1024)
 RAM_total = string.format("%g", RAM_total/1024)
end

--[[
   Save Table to File
   Load Table from File
   v 1.0
   
   Lua 5.2 compatible
   
   Only Saves Tables, Numbers and Strings
   Insides Table References are saved
   Does not save Userdata, Metatables, Functions and indices of these
   ----------------------------------------------------
   table.save( table , filename )
   
   on failure: returns an error msg
   
   ----------------------------------------------------
   table.load( filename or stringtable )
   
   Loads a table that has been saved via the table.save function
   
   on success: returns a previously saved table
   on failure: returns as second argument an error msg
   ----------------------------------------------------
   
   Licensed under the same terms as Lua itself.
]]--
do
   -- declare local variables
   --// exportstring( string )
   --// returns a "Lua" portable version of the string
   local function exportstring( s )
      return string.format("%q", s)
   end

   --// The Save Function
   function table.save(  tbl,filename )
      local charS,charE = "   ","\n"
      local file,err = io.open( filename, "wb" )
      if err then return err end

      -- initiate variables for save procedure
      local tables,lookup = { tbl },{ [tbl] = 1 }
      file:write( "return {"..charE )

      for idx,t in ipairs( tables ) do
         file:write( "-- Table: {"..idx.."}"..charE )
         file:write( "{"..charE )
         local thandled = {}

         for i,v in ipairs( t ) do
            thandled[i] = true
            local stype = type( v )
            -- only handle value
            if stype == "table" then
               if not lookup[v] then
                  table.insert( tables, v )
                  lookup[v] = #tables
               end
               file:write( charS.."{"..lookup[v].."},"..charE )
            elseif stype == "string" then
               file:write(  charS..exportstring( v )..","..charE )
            elseif stype == "number" then
               file:write(  charS..tostring( v )..","..charE )
            end
         end

         for i,v in pairs( t ) do
            -- escape handled values
            if (not thandled[i]) then
            
               local str = ""
               local stype = type( i )
               -- handle index
               if stype == "table" then
                  if not lookup[i] then
                     table.insert( tables,i )
                     lookup[i] = #tables
                  end
                  str = charS.."[{"..lookup[i].."}]="
               elseif stype == "string" then
                  str = charS.."["..exportstring( i ).."]="
               elseif stype == "number" then
                  str = charS.."["..tostring( i ).."]="
               end
            
               if str ~= "" then
                  stype = type( v )
                  -- handle value
                  if stype == "table" then
                     if not lookup[v] then
                        table.insert( tables,v )
                        lookup[v] = #tables
                     end
                     file:write( str.."{"..lookup[v].."},"..charE )
                  elseif stype == "string" then
                     file:write( str..exportstring( v )..","..charE )
                  elseif stype == "number" then
                     file:write( str..tostring( v )..","..charE )
                  end
               end
            end
         end
         file:write( "},"..charE )
      end
      file:write( "}" )
      file:close()
   end
   
   --// The Load Function
   function table.load( sfile )
      local ftables,err = loadfile( sfile )
      if err then return _,err end
      local tables = ftables()
      for idx = 1,#tables do
         local tolinki = {}
         for i,v in pairs( tables[idx] ) do
            if type( v ) == "table" then
               tables[idx][i] = tables[v[1]]
            end
            if type( i ) == "table" and tables[i[1]] then
               table.insert( tolinki,{ i,tables[i[1]] } )
            end
         end
         -- link indices
         for _,v in ipairs( tolinki ) do
            tables[idx][v[2]],tables[idx][v[1]] =  tables[idx][v[1]],nil
         end
      end
      return tables[1]
   end
-- close do
end

-- ChillCode

--Bitwise functions

function bit(p)
  return 2 ^ (p - 1)  -- 1-based indexing
end

-- Typical call:  if hasbit(x, bit(3)) then ...
function hasbit(x, p)
  return x % (p + p) >= p      
end

function setbit(x, p)
  return hasbit(x, p) and x or x + p
end

function clearbit(x, p)
  return hasbit(x, p) and x - p or x
end