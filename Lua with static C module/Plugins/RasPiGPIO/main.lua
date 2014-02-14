--Using the Raspberry Pi GPIO module from http://www.andre-simon.de/doku/rpi_gpio_lua/en/rpi_gpio_lua.php
--Plugin for MCserver for Raspberry Pi GPIO toggling
--install: place into MCserverRoot/Plugins/RPiGPIO/main.lua,
--than edit MCserverRoot/settings.ini and add Plugin=RPiGPIO
--usage: place a lever and than write "\assignlast <pinN> [IN]" (the IN flag is optional)
--pin numbers are board pinout numbers


local gpio = require("GPIO")
require("/Plugins/Moje/arbitrary")
--require("/Plugins/Moje/blockHandler")
require("MCmodule")

PLUGIN = nil

tickCounter = 0


--pinMapWiringPi = {18, 27, 22, 23, 24, 25, 4, 2, 3, 8, 7, 10, 9, 11, 14, 15, 28, 29, 30, 31};
pinsBoard = {3, 5, 7, 8, 10, 11, 12, 13, 15, 16, 18, 19, 21, 22, 23, 24, 26}
--wiringPiToBCM = {11, 12, 13, 15, 16, 18, 22}

outputs = {}
infosigns = {}
inputs = {}
last_placed_by_player = {}

function Initialize(Plugin)
	Plugin:SetName("RasPiGPIO")
	Plugin:SetVersion(1)
  

  

  cPluginManager:AddHook(cPluginManager.HOOK_PLAYER_PLACED_BLOCK, MyOnPlayerPlacedBlock)
  cPluginManager:AddHook(cPluginManager.HOOK_PLAYER_BROKEN_BLOCK, MyOnPlayerBrokenBlock)
  cPluginManager:AddHook(cPluginManager.HOOK_PLAYER_USED_BLOCK, MyOnPlayerUsedBlock)
  cPluginManager:AddHook(cPluginManager.HOOK_PLAYER_USED_ITEM, MyOnPlayerUsedItem);
  cPluginManager:AddHook(cPluginManager.HOOK_UPDATED_SIGN, MyOnUpdatedSign)
  cPluginManager:AddHook(cPluginManager.HOOK_DISCONNECT, MyOnDisconnect)
  cPluginManager:AddHook(cPluginManager.HOOK_WORLD_TICK, MyOnWorldTick)
  cPluginManager.BindCommand("/assignlast", "", AssignLast, " ~ Assigns the last lever to the specified GPIO pin")
  cPluginManager.BindCommand("/logio", "", LogIO, " - Logs cached inputs and outputs to server console")
  --cPluginManager.BindCommand("/tellio", "", TellIO, " - Tells cached inputs and outputs to player")
	

 --gpio.warnings(0)

 --gpio.setmode(GPIO.BOARD)



 PLUGIN = Plugin

 LOG("Initialised " .. Plugin:GetName() .. " v." .. Plugin:GetVersion())
 return true
end

function OnDisable()
 Quitting()	--to save the arrays to a file
 
 gpio.cleanup()
 LOG("Disabled " .. PLUGIN:GetName() .. "!")
end

function MyOnWorldTick(World, TimeDelta)
 worldTick(World)
end

function MyOnUpdatedSign(World, BlockX, BlockY, BlockZ, Line1, Line2, Line3, Line4, Player)
 if (Player ~= nil) then	--is not a server
  local last = nil
  --LOG("UPDATING SIGN Line1 = '" .. Line1 .. "'")
  if (Line1 == "\\TEMP") then
   last = #infosigns + 1
   infosigns[last] = {}
   infosigns[last]['X'] = BlockX
   infosigns[last]['Y'] = BlockY
   infosigns[last]['Z'] = BlockZ
   infosigns[last]['INFO'] = "TEMP"
   UpdateSigns(World)
  else if (Line1 == "\\CPU") then
   last = #infosigns + 1
   infosigns[last] = {}
   infosigns[last]['X'] = BlockX
   infosigns[last]['Y'] = BlockY
   infosigns[last]['Z'] = BlockZ
   infosigns[last]['INFO'] = "CPU"
   UpdateSigns(World)
  else if (Line1 == "\\RAM") then
   last = #infosigns + 1
   infosigns[last] = {}
   infosigns[last]['X'] = BlockX
   infosigns[last]['Y'] = BlockY
   infosigns[last]['Z'] = BlockZ
   infosigns[last]['INFO'] = "RAM"
   UpdateSigns(World)
  end
  end
  end
 end
end

function AssignLast(Split, Player)
 if (#Split ~= 2 and #Split ~= 3) then
  SendMessageFailure(Player, "You must specify a pin number!")
 else
  local pin = tonumber(Split[2])
  if (FindInArray(pin, pinsBoard) == nil) then
   SendMessageFailure(Player, "Wrong pin number. Must be one of these: " .. ArrayValsToString(pinsBoard))
  else
   local plname = Player:GetName()
   if (last_placed_by_player[plname] ~= nil) then
    if (#Split == 3 and Split[3] == "IN") then
     addInput(last_placed_by_player[plname]['X'], last_placed_by_player[plname]['Y'], last_placed_by_player[plname]['Z'], pin)
     LOG("Input added in C")
    else
     addOutput(last_placed_by_player[plname]['X'], last_placed_by_player[plname]['Y'], last_placed_by_player[plname]['Z'], pin)
     --LOG("Lever on position [" .. outputs[lastl]['X'] .. "," .. outputs[lastl]['Y'] .. "," .. outputs[lastl]['Z'] .. "] has been placed and linked to pin " .. pin .. " by player " .. plname .. ".")
    end
   else
    SendMessageFailure(Player, "No suitable block was placed by you!")
   end
  end
 end
 return true
end

function MyOnPlayerPlacedBlock(Player, BlockX, BlockY, BlockZ, BlockFace, CursorX, CursorY, CursorZ, BlockType, BlockMeta)
 if (BlockType == 69 or BlockType == 75 or BlockType == 76 or BlockType == 77 or BlockType == 143) then
  local plname = Player:GetName()
  if (last_placed_by_player[plname] == nil) then
   last_placed_by_player[plname] = {}
  end
  last_placed_by_player[plname]['X'] = BlockX
  last_placed_by_player[plname]['Y'] = BlockY
  last_placed_by_player[plname]['Z'] = BlockZ
 end
end

function MyOnPlayerUsedItem(Player, BlockX, BlockY, BlockZ, BlockFace, CursorX, CursorY, CursorZ, BlockType, BlockMeta)
 if (Player:GetEquippedItem().m_ItemType == 284 and not (BlockX == -1 and BlockY == 255 and BlockZ == -1)) then
  local plname = Player:GetName()
  if (last_placed_by_player[plname] == nil) then
   last_placed_by_player[plname] = {}
  end
  last_placed_by_player[plname]['X'] = BlockX
  last_placed_by_player[plname]['Y'] = BlockY
  last_placed_by_player[plname]['Z'] = BlockZ
  SendMessage(Player, "Block remembered. Write /assignlast to assign this block to a pin.")
 end
end

function MyOnPlayerBrokenBlock(Player, BlockX, BlockY, BlockZ, BlockFace, BlockType, BlockMeta)
 --if (BlockType == 69) then
  remOutput(BlockX, BlockY, BlockZ)  

  remInput(BlockX, BlockY, BlockZ)  

  i = FindInfoSign(BlockX, BlockY, BlockZ)
  if (i ~= nil) then

   infosigns[i] = nil
  end
  i = FindLastPlacedOfPlayer(BlockX, BlockY, BlockZ)
  if (i ~= nil) then
   --LOG("Last placed block by player " .. i .. " has been removed.")
   last_placed_by_player[i] = nil
  end
 --end
end

function MyOnPlayerUsedBlock(Player, BlockX, BlockY, BlockZ, BlockFace, CursorX, CursorY, CursorZ, BlockType, BlockMeta)

end

--Pri odpojeni se uklidi hracuv posledni pouzity blok
function MyOnDisconnect(Player, Reason)
 last_placed_by_player[Player] = nil
end