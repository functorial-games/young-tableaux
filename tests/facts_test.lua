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

local wegert = young_wegert({3,2,1}, {5,3,1,3,1,1})
assert(wegert.n_lambda == 4)
assert(wegert.max_hook == 5)
assert(wegert.hook_counts[1] == 3)
assert(wegert.hook_counts[3] == 2)
assert(wegert.hook_counts[5] == 1)

local layout = young_layout()
local saw_shape, saw_wegert, saw_derived = false, false, false
for _, item in ipairs(layout) do
  assert(not (item.text or ''):lower():find('conjugate blocks', 1, true))
  assert(not (item.text or ''):lower():find('kitchen sink', 1, true))
  if item.kind == 'shape' then saw_shape = true end
  if item.kind == 'wegert' then assert(saw_shape); saw_wegert = true end
  if item.kind == 'label' and item.text == 'DERIVED FACTS' then
    assert(saw_wegert)
    saw_derived = true
  end
end
assert(saw_shape and saw_wegert and saw_derived)

local shape = young_shape_add({2,1}, 1, 3)
assert(shape[1] == 3 and shape[2] == 1)
shape = young_shape_add(shape, 2, 2)
assert(shape[1] == 3 and shape[2] == 2)
assert(young_shape_history_depth(shape) == 2)
shape = young_shape_undo(shape)
assert(shape[1] == 3 and shape[2] == 1)
shape = young_shape_reset(shape)
assert(shape[1] == 2 and shape[2] == 1)
assert(young_shape_history_depth(shape) == 0)
assert(young_shape_add({2,2}, 2, 3) == nil)

print('PASS formula, Wegert preparation, scripted layout, and click-add state')
