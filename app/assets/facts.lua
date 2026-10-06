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

function young_facts(lambda, hooks, standard_count)
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
    "SCHUR SPECIALIZATION",
    "lambda = " .. shape_text(lambda) .. "   |lambda| = " .. size,
    "n(lambda) = " .. n_lambda,
    "f^lambda / Specht dimension = " .. standard_count,
    "principal specialization:",
    "s_lambda(1,z,z^2,...) = " .. formula,
    "The plot below shows its meromorphic continuation.",
    "Each hook h contributes a factor (1-z^h), so its poles lie at roots of unity."
  }, "\n")
end
