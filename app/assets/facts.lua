local function copy_shape(lambda)
  local out = {}
  for i, row in ipairs(lambda) do out[i] = row end
  return out
end

local function same_shape(a, b)
  if #a ~= #b then return false end
  for i = 1, #a do if a[i] ~= b[i] then return false end end
  return true
end

local function shape_text(lambda)
  if #lambda == 0 then return "[]" end
  local parts = {}
  for i, row in ipairs(lambda) do parts[i] = tostring(row) end
  return "[" .. table.concat(parts, ",") .. "]"
end

local function factor_text(h, multiplicity)
  local base = h == 1 and "(1-z)" or ("(1-z^" .. h .. ")")
  if multiplicity == 1 then return base end
  return base .. "^" .. multiplicity
end

local function derived(lambda, hooks)
  local size = 0
  local n_lambda = 0
  for i, row in ipairs(lambda) do
    size = size + row
    n_lambda = n_lambda + (i - 1) * row
  end

  local counts = {}
  local max_hook = 0
  for _, hook in ipairs(hooks) do
    counts[hook] = (counts[hook] or 0) + 1
    if hook > max_hook then max_hook = hook end
  end
  return size, n_lambda, counts, max_hook
end

function young_facts(lambda, hooks, standard_count)
  local size, n_lambda, counts, max_hook = derived(lambda, hooks)

  local factors = {}
  for hook = 1, max_hook do
    local multiplicity = counts[hook]
    if multiplicity then
      factors[#factors + 1] = factor_text(hook, multiplicity)
    end
  end

  local numerator
  if n_lambda == 0 then numerator = "1"
  elseif n_lambda == 1 then numerator = "z"
  else numerator = "z^" .. n_lambda end

  local denominator = #factors == 0 and "1" or table.concat(factors, " ")
  local formula = denominator == "1" and numerator or (numerator .. " / (" .. denominator .. ")")

  return table.concat({
    "Schur specialization",
    "λ = " .. shape_text(lambda) .. "   |λ| = " .. size,
    "n(λ) = " .. n_lambda,
    "f^λ / Specht dimension = " .. standard_count,
    "Principal specialization:",
    "s_λ(1,z,z^2,...) = " .. formula,
    "The plot shows its meromorphic continuation.",
    "Each hook h contributes a factor (1-z^h), so its poles lie at roots of unity."
  }, "\n")
end

function young_wegert(lambda, hooks)
  local _, n_lambda, counts, max_hook = derived(lambda, hooks)
  return {
    n_lambda = n_lambda,
    max_hook = max_hook,
    hook_counts = counts
  }
end

function young_layout()
  return {
    {kind="label", text="Young Tableaux 0.3.2"},
    {kind="separator"},
     {kind="field", arg=1, text="λ: rows"},
    {kind="shape"},
    {kind="wegert", arg=160},
    {kind="plot_controls"},
    {kind="separator"},
    {kind="label", text="Derived facts"},
    {kind="output", arg=0},
    {kind="output", arg=2},
    {kind="hooks", text="Hook cells"},
    {kind="facts"},
    {kind="separator"},
    {kind="label", text="More operations"}
  }
end

local shape_history = {}
local shape_current = nil

local function sync_shape(lambda)
  if shape_current and not same_shape(shape_current, lambda) then
    shape_history = {}
  end
  shape_current = copy_shape(lambda)
end

function young_shape_add(lambda, row, column)
  sync_shape(lambda)
  if row < 1 or row > #lambda + 1 then return nil end
  if row == #lambda + 1 then
    if column ~= 1 then return nil end
  else
    if column ~= lambda[row] + 1 then return nil end
    if row > 1 and lambda[row - 1] <= lambda[row] then return nil end
  end

  local out = copy_shape(lambda)
  shape_history[#shape_history + 1] = copy_shape(lambda)
  if row == #lambda + 1 then out[row] = 1 else out[row] = out[row] + 1 end
  shape_current = copy_shape(out)
  return out
end

function young_shape_remove(lambda, row, column)
  sync_shape(lambda)
  if row < 1 or row > #lambda then return nil end
  if column ~= lambda[row] then return nil end
  if row < #lambda and lambda[row] <= lambda[row + 1] then return nil end

  local out = copy_shape(lambda)
  out[row] = out[row] - 1
  if out[row] == 0 then table.remove(out, row) end
  shape_history = {}
  shape_current = copy_shape(out)
  return out
end

function young_shape_undo(lambda)
  sync_shape(lambda)
  if #shape_history == 0 then return nil end
  local out = shape_history[#shape_history]
  shape_history[#shape_history] = nil
  shape_current = copy_shape(out)
  return copy_shape(out)
end

function young_shape_reset(lambda)
  sync_shape(lambda)
  if #shape_history == 0 then return nil end
  local out = shape_history[1]
  shape_history = {}
  shape_current = copy_shape(out)
  return copy_shape(out)
end

function young_shape_history_depth(lambda)
  sync_shape(lambda)
  return #shape_history
end
