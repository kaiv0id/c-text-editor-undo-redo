# Test cases and results

The following scenarios are required. Record results only after executing them with the stated compiler flags. This file starts with pending results because a compiler has not yet been found in the environment.

| ID | Scenario | Expected result | Actual result |
|---|---|---|---|
| T01 | Insert at position 1, then append at end | Text appears at both requested positions | Pending execution |
| T02 | Delete a valid range and undo/redo it | Document returns to each expected snapshot | Pending execution |
| T03 | Replace a valid range | Only selected range is replaced | Pending execution |
| T04 | Undo multiple operations, then redo multiple operations | Snapshots are restored in correct order | Pending execution |
| T05 | Undo, then perform a new successful edit | Redo history is cleared | Pending execution |
| T06 | Queue several valid edits and process | Edits execute FIFO | Pending execution |
| T07 | Queue an edit that is invalid at processing time | Failed request stays at queue front | Pending execution |
| T08 | Use invalid positions and lengths | Edit fails without changing document | Pending execution |
| T09 | Undo/redo on empty stacks | Friendly message; no state change | Pending execution |
| T10 | Fill history and pending queue to capacity | History discards oldest; enqueue at full queue fails | Pending execution |
| T11 | Save then load a document | File content is restored and load can be undone | Pending execution |
| T12 | Load empty, oversized, or unreadable file | Safe handling; document is unchanged on failure | Pending execution |
| T13 | Empty document insert/delete/replace boundaries | Valid insert works; invalid ranges fail safely | Pending execution |

Compile with:

```sh
gcc -std=c99 -Wall -Wextra -pedantic src/text_editor.c -o text_editor
```

## Execution record

- Compiler: pending availability.
- Command: pending.
- Build warnings/errors: pending.
- Scenario results: pending execution.
