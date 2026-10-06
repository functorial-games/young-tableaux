dofile('app/assets/facts.lua')

local cases = {
  {{}, {}, '1'},
  {{1}, {1}, '1'},
  {{2,1}, {3,1,1}, '2'},
  {{3,2,1}, {5,3,1,3,1,1}, '16'}
}
for index, case in ipairs(cases) do
  local facts = young_facts(case[1], case[2], case[3])
  assert(not facts:find('LUA', 1, true))
  assert(not facts:find('SCRIPTED', 1, true))
  assert(facts:find('Schur specialization', 1, true))
  assert(facts:find('Substitute 1, z, z², … into s_λ', 1, true))
  assert(facts:find('roots of unity', 1, true))
  assert(not facts:find('n(λ)', 1, true))
  assert(not facts:find(' / ', 1, true))
end

local wegert = young_wegert({3,2,1}, {5,3,1,3,1,1})
assert(wegert.n_lambda == 4)
assert(wegert.max_hook == 5)
assert(wegert.hook_counts[1] == 3)
assert(wegert.hook_counts[3] == 2)
assert(wegert.hook_counts[5] == 1)

local layout = young_layout()
local saw_shape, saw_wegert, saw_hooks, saw_facts = false, false, false, false
for _, item in ipairs(layout) do
  local text = item.text or ''
  assert(not text:lower():find('conjugate blocks', 1, true))
  assert(not text:lower():find('kitchen sink', 1, true))
  assert(not text:find('Derived facts', 1, true))
  assert(not text:find('WEGERT PLOT', 1, true))
  if item.kind == 'shape' then saw_shape = true end
  if item.kind == 'wegert' then assert(saw_shape); saw_wegert = true end
  if item.kind == 'hooks' then
    assert(saw_wegert)
    assert(text:find('each number counts its cell', 1, true))
    saw_hooks = true
  end
  if item.kind == 'facts' then assert(saw_hooks); saw_facts = true end
end
assert(saw_shape and saw_wegert and saw_hooks and saw_facts)

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

shape = young_shape_remove({3,2,1}, 2, 2)
assert(shape[1] == 3 and shape[2] == 1 and shape[3] == 1)
assert(young_shape_remove({2,2}, 1, 2) == nil)
shape = young_shape_remove({1}, 1, 1)
assert(#shape == 0)

print('PASS compact Schur explanation, Wegert preparation, layout, and direct shape editing')
