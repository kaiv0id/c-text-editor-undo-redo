# Architecture

## Program flow

`main` owns one `Editor` containing the bounded document, undo and redo stacks, and pending circular queue. It repeatedly reads a menu choice, validates operation parameters, then calls the appropriate helper. The program exits cleanly on menu choice 0 or end of input.

## Edit model

Insert, Delete and Replace first work on a temporary document copy. Position, range and capacity checks happen before the candidate is committed. A successful edit records both complete document snapshots in an `Action`, pushes it to Undo and clears Redo. Failed edits leave the document and history unchanged.

Undo pops the latest action, restores its `before` snapshot, and pushes it onto Redo. Redo pops the latest redo action, restores its `after` snapshot and pushes it back to Undo. The stacks retain at most 64 actions; pushing into a full stack drops its oldest action.

## Pending queue

The queue stores up to 32 validated edit requests. It is represented by a fixed array, a front index, and a count. Enqueue writes at `(front + count) % capacity`; dequeue advances `front` modulo capacity. Processing examines the front request before removing it. On success, it applies the edit and removes the queue item. If the edit is invalid for the document as it exists at processing time, processing stops and the request remains at the front for inspection or retry.

## File handling

Save opens the requested path for writing and checks both `fwrite` and `fclose`. Load reads into a bounded temporary buffer and rejects I/O errors, embedded NUL bytes, and files larger than the document capacity. A successful load is recorded as one undoable document replacement.

## Complexity

Text edits use O(n) movement and snapshots, where n is current document length. Undo and Redo copy an O(n) snapshot. Queue insertion and removal are O(1). Processing at most 32 queued operations takes O(q × n) in the worst case. History stores O(h × n) text, with h capped at 64.

## Boundaries

The editor is byte-oriented: positions count bytes, not Unicode code points. Text inputs are lines without embedded newlines. The document capacity is 4095 bytes plus its terminating NUL. The queue has 32 entries, and each undo/redo stack has 64 snapshots.
