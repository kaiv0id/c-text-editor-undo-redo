# Test cases and results

Executed on Windows with GCC 16.2.0 MinGW-w64. Tests drove the interactive menu through redirected standard input and checked the output and saved files.

| ID | Scenario | Expected result | Actual result |
|---|---|---|---|
| T01 | Insert at position 1, then append at end | Text appears at both requested positions | Passed: inserted `Hello`, then appended `, C!` at position 6. |
| T02 | Delete a valid range and undo/redo it | Document returns to each expected snapshot | Passed: deleted a 4-character range, then undid and redid it. |
| T03 | Replace a valid range | Only selected range is replaced | Passed: replaced four characters with `i`. |
| T04 | Undo multiple operations, then redo multiple operations | Snapshots are restored in correct order | Passed: undid two edits and redid both in stack order. |
| T05 | Undo, then perform a new successful edit | Redo history is cleared | Passed: the next Redo reported `Nothing to redo.` |
| T06 | Queue several valid edits and process | Edits execute FIFO | Passed: queued `X` then `Y` at position 2; result was `XY`. |
| T07 | Queue an edit that is invalid at processing time | Failed request stays at queue front | Passed: a delete queued on an empty document remained visible at the front. |
| T08 | Use invalid positions and lengths | Edit fails without changing document | Passed: position 0, zero length, and an out-of-range edit were rejected safely. |
| T09 | Undo/redo on empty stacks | Friendly message; no state change | Passed: empty Undo and Redo reported friendly messages. |
| T10 | Fill history and pending queue to capacity | History discards oldest; enqueue at full queue fails | Passed: history capped at 64; the 33rd queue request was rejected at capacity 32; queue drained to 0. |
| T11 | Save then load a document | File content is restored and load can be undone | Passed: saved `sample`, loaded it over `Xsample`, then undid the load. |
| T12 | Load empty, oversized, or unreadable file | Safe handling; document is unchanged on failure | Passed: empty file load was undoable; 4096-byte file and missing path were rejected without changing text. |
| T13 | Empty document insert/delete/replace boundaries | Valid insert works; invalid ranges fail safely | Passed: empty-document delete/replace ranges failed; a 4095-byte document loaded, and an extra insert was rejected. |

Compile with:

```sh
gcc -std=c99 -Wall -Wextra -pedantic src/text_editor.c -o text_editor
```

## Execution record

- Compiler: GCC 16.2.0 (MinGW-w64 x86_64, UCRT).
- Command: `gcc -std=c99 -Wall -Wextra -pedantic -O2 src/text_editor.c -o text_editor.exe`.
- Build warnings/errors: none; build succeeded.
- Scenario results: T01–T13 passed as recorded above.
