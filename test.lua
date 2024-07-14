local WINDOW_SIZE = 200000
local MESSAGE_COUNT = 1000000
local MESSAGE_SIZE = 1024

local function createMessage(n)
  return string.char(n & 0xFF):rep(MESSAGE_SIZE)
end

local worstPushTime = 0
local function pushMessage(store, id)
  local start, ending
  start = os.clock()
  store[id % WINDOW_SIZE] = createMessage(id)
  ending = os.clock()
  local time = ending - start
  if time > worstPushTime then
    worstPushTime = time
  end
end

local store = {}
for i=0, WINDOW_SIZE-1 do
  store[i] = 0
end

local start, ending
start = os.clock()
for i = 0, MESSAGE_COUNT-1 do
  pushMessage(store, i)
end
ending = os.clock()
print("Test took ", ending - start, " seconds")
print("Worst time was ", worstPushTime)

