dofile('app/assets/facts.lua')
local cases = {
  {{}, {}, '1', ' = 1'},
  {{1}, {1}, '1', '1 / ((1-z))'},
  {{2,1}, {3,1,1}, '2', 'z / ((1-z)^2 (1-z^3))'},
  {{3,2,1}, {5,3,1,3,1,1}, '16', 'z^4 / ((1-z)^3 (1-z^3)^2 (1-z^5))'}
}
for index, case in ipairs(cases) do
  local facts = young_facts(case[1], case[2], case[3])
  assert(facts:find(case[4], 1, true), 'wrong formula in case ' .. index)
  assert(not facts:find('LUA', 1, true))
  assert(not facts:find('SCRIPTED', 1, true))
  assert(facts:find('SCHUR SPECIALIZATION', 1, true))
end
print('PASS 4 formula cases; no implementation-language labels')
