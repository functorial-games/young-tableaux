local copy_shape ← λ(lambda)
  local out ← {}
  for i, row in ipairs(lambda) do out[i] ← row end
  return out
end

local same_shape ← λ(a, b)
  if #a ≠ #b then return false end
  for i ← 1, #a do if a[i] ≠ b[i] then return false end end
  return true
end

local shape_text ← λ(lambda)
  if #lambda = 0 then return "[]" end
  local parts ← {}
  for i, row in ipairs(lambda) do parts[i] ← tostring(row) end
  return "[" .. table.concat(parts, ",") .. "]"
end

local derived ← λ(lambda, hooks)
  local size ← 0
  local n_lambda ← 0
  for i, row in ipairs(lambda) do
    size ← size + row
    n_lambda ← n_lambda + (i − 1) × row
  end

  local counts ← {}
  local max_hook ← 0
  for _, hook in ipairs(hooks) do
    counts[hook] ← (counts[hook] or 0) + 1
    if hook > max_hook then max_hook ← hook end
  end
  return size, n_lambda, counts, max_hook
end

young_facts ← λ(lambda, hooks, standard_count)
  return table.concat({
    "Schur specialization",
    "Substitute 1, z, z², … into s_λ to get one function of z.",
    "The plot shows that function; hook lengths determine its poles at roots of unity."
  }, "\n")
end

young_wegert ← λ(lambda, hooks)
  local _, n_lambda, counts, max_hook ← derived(lambda, hooks)
  return {
    n_lambda ← n_lambda,
    max_hook ← max_hook,
    hook_counts ← counts
  }
end

young_layout ← λ()
  return {
    {kind ← "label", text ← "Young Tableaux 0.4.0"},
    {kind ← "separator"},
    {kind ← "field", arg ← 1, text ← "λ: rows"},
    {kind ← "shape"},
    {kind ← "tableau", text ← "Current tableau"},
    {kind ← "wegert", arg ← 160},
    {kind ← "plot_controls"},
    {kind ← "separator"},
    {kind ← "hooks", text ← "Hook lengths: each number counts its cell, cells right, and cells below."},
    {kind ← "output", arg ← 2},
    {kind ← "facts"},
    {kind ← "separator"},
    {kind ← "label", text ← "More operations"}
  }
end

local shape_history ← {}
local shape_current ← nil

local sync_shape ← λ(lambda)
  if shape_current and not same_shape(shape_current, lambda) then
    shape_history ← {}
  end
  shape_current ← copy_shape(lambda)
end

young_shape_add ← λ(lambda, row, column)
  sync_shape(lambda)
  if row < 1 or row > #lambda + 1 then return nil end
  if row = #lambda + 1 then
    if column ≠ 1 then return nil end
  else
    if column ≠ lambda[row] + 1 then return nil end
    if row > 1 and lambda[row − 1] ≤ lambda[row] then return nil end
  end

  local out ← copy_shape(lambda)
  shape_history[#shape_history + 1] ← copy_shape(lambda)
  if row = #lambda + 1 then out[row] ← 1 else out[row] ← out[row] + 1 end
  shape_current ← copy_shape(out)
  return out
end

young_shape_remove ← λ(lambda, row, column)
  sync_shape(lambda)
  if row < 1 or row > #lambda then return nil end
  if column ≠ lambda[row] then return nil end
  if row < #lambda and lambda[row] ≤ lambda[row + 1] then return nil end

  local out ← copy_shape(lambda)
  out[row] ← out[row] − 1
  if out[row] = 0 then table.remove(out, row) end
  shape_history ← {}
  shape_current ← copy_shape(out)
  return out
end

young_shape_undo ← λ(lambda)
  sync_shape(lambda)
  if #shape_history = 0 then return nil end
  local out ← shape_history[#shape_history]
  shape_history[#shape_history] ← nil
  shape_current ← copy_shape(out)
  return copy_shape(out)
end

young_shape_reset ← λ(lambda)
  sync_shape(lambda)
  if #shape_history = 0 then return nil end
  local out ← shape_history[1]
  shape_history ← {}
  shape_current ← copy_shape(out)
  return copy_shape(out)
end

young_shape_history_depth ← λ(lambda)
  sync_shape(lambda)
  return #shape_history
end
