function blockOn(BlockX, BlockY, BlockZ, World)
 local valid, type, meta, skylight, blocklight = World:GetBlockInfo(BlockX, BlockY, BlockZ)
 if (type == 69 or type == 77 or type == 143) then	--lever, stone button, wooden button
  World:SetBlockMeta(BlockX, BlockY, BlockZ, setbit(meta, 0x8))
  World:SetNextBlockTick(BlockX, BlockY, BlockZ)
 else if (type == 75) then	--inactive redstone torch
  World:SetBlock(BlockX, BlockY, BlockZ, 76, meta)
 else if (type == 123) then	--inactive redstone lamp
  World:SetBlock(BlockX, BlockY, BlockZ, 124, meta)
 end
 end
 end
end

function blockOff(BlockX, BlockY, BlockZ, World)
 local valid, type, meta, skylight, blocklight = World:GetBlockInfo(BlockX, BlockY, BlockZ)
 if (type == 69 or type == 77 or type == 143) then	--lever, stone button, wooden button
  World:SetBlockMeta(BlockX, BlockY, BlockZ, clearbit(meta, 0x8))
 else if (type == 76) then	--active redstone torch
  World:SetBlock(BlockX, BlockY, BlockZ, 75, meta)
 else if (type == 124) then	--active redstone lamp
  World:SetBlock(BlockX, BlockY, BlockZ, 123, meta)
 end
 end
 end
end

function blockState(BlockX, BlockY, BlockZ, World)
 local ret = nil
 local valid, type, meta, skylight, blocklight = World:GetBlockInfo(BlockX, BlockY, BlockZ)
 if (type == 75) then	--inactive redstone torch
  ret = 0
 else if (type == 76) then	--active redstone torch
  ret = 1
 else if (type == 69 or type == 77 or type == 143) then	--lever, stone button, wooden button
  ret = hasbit(meta, 0x8) and 1 or 0
 else if (type == 123) then	--inactive redstone lamp
  ret = 0
 else if (type == 124) then	--active redstone lamp
  ret = 1
 end
 end
 end
 end
 end
 return ret
end
