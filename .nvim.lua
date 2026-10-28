-- Style in this project is hand-maintained: brace placement (Allman) and
-- spacing around operators stay exactly as typed, so format-on-save is off.
--
-- This only disables the formatters (conform's clang-format, and the clangd
-- fallback behind it). Auto-indent is a separate mechanism -- `cindent`, which
-- only ever sets leading whitespace -- and keeps working normally.
--
-- Format by hand with <leader>cf; <leader>uf toggles autoformat back on.
vim.g.autoformat = false
