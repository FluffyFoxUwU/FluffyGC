collectgarbage("incremental")

function test()
  local t = {}

  for i=1,1000 do
    t[i] = {}
    for j=1,100 do
      local var = {}
      var[1] = {812}
      var[2] = {var}
        
      local var2 = {}
      var2[1] = {1, 2, 3}
      var2[2] = {452}
      t[i][j] = var
    end
  end
end

local maxMemory = 0
debug.sethook(function()
  local cur = collectgarbage("count")
  if cur > maxMemory then
    maxMemory = cur
  end
end, "l", 20)

local startTime = os.clock()
for i=1,20 do
  test()
end
collectgarbage("collect")

local endTime = os.clock()
print("Time taken was", endTime - startTime)
print("Maximum memory was ", maxMemory, "KiB")

