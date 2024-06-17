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

local startTime = os.clock()
for i=1,10 do
  test()
end
local endTime = os.clock()
print("Time taken was", endTime - startTime)

