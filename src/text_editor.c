#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEXT_CAPACITY 4096
#define INPUT_CAPACITY 1024
#define HISTORY_CAPACITY 64
#define QUEUE_CAPACITY 32

typedef struct {
    char before[TEXT_CAPACITY];
    char after[TEXT_CAPACITY];
    char description[80];
} Action;

typedef struct {
    Action items[HISTORY_CAPACITY];
    size_t count;
} ActionStack;

typedef enum { OP_INSERT, OP_DELETE, OP_REPLACE } OperationType;

typedef struct {
    OperationType type;
    size_t position;
    size_t length;
    char text[INPUT_CAPACITY];
} Operation;

typedef struct {
    Operation items[QUEUE_CAPACITY];
    size_t front;
    size_t count;
} PendingQueue;

typedef struct {
    char text[TEXT_CAPACITY];
    ActionStack undo;
    ActionStack redo;
    PendingQueue pending;
} Editor;

static int read_line(const char *prompt, char *buffer, size_t capacity) {
    size_t length;
    int ch;
    fputs(prompt, stdout);
    if (fgets(buffer, (int)capacity, stdin) == NULL) return 0;
    length = strlen(buffer);
    if (length > 0 && buffer[length - 1] == '\n') {
        buffer[length - 1] = '\0';
    } else {
        while ((ch = getchar()) != '\n' && ch != EOF) { }
        puts("Input was too long and has been discarded.");
        buffer[0] = '\0';
        return -1;
    }
    if (length > 1 && buffer[length - 2] == '\r') buffer[length - 2] = '\0';
    return 1;
}

static int read_size(const char *prompt, size_t *value) {
    char line[INPUT_CAPACITY];
    char *end;
    unsigned long parsed;
    int result = read_line(prompt, line, sizeof line);
    if (result <= 0) return result;
    if (line[0] == '\0' || line[0] == '-') return 0;
    errno = 0;
    parsed = strtoul(line, &end, 10);
    while (isspace((unsigned char)*end)) ++end;
    if (errno == ERANGE || end == line || *end != '\0') return 0;
    *value = (size_t)parsed;
    return 1;
}

static int stack_push(ActionStack *stack, const Action *action) {
    if (stack->count == HISTORY_CAPACITY) {
        memmove(&stack->items[0], &stack->items[1],
                (HISTORY_CAPACITY - 1) * sizeof stack->items[0]);
        --stack->count;
    }
    stack->items[stack->count++] = *action;
    return 1;
}

static int apply_operation(char *text, const Operation *op) {
    size_t current = strlen(text);
    size_t position = op->position;
    size_t replacement_length = strlen(op->text);
    if (position == 0 || position > current + (op->type == OP_INSERT ? 1U : 0U))
        return 0;
    if (op->type == OP_INSERT) {
        if (current + replacement_length >= TEXT_CAPACITY) return 0;
        memmove(text + position - 1 + replacement_length, text + position - 1,
                current - (position - 1) + 1);
        memcpy(text + position - 1, op->text, replacement_length);
    } else {
        if (op->length == 0 || op->length > current - (position - 1)) return 0;
        if (op->type == OP_DELETE) {
            memmove(text + position - 1, text + position - 1 + op->length,
                    current - (position - 1 + op->length) + 1);
        } else {
            if (current - op->length + replacement_length >= TEXT_CAPACITY) return 0;
            memmove(text + position - 1 + replacement_length,
                    text + position - 1 + op->length,
                    current - (position - 1 + op->length) + 1);
            memcpy(text + position - 1, op->text, replacement_length);
        }
    }
    return 1;
}

static int perform(Editor *editor, const Operation *op) {
    Action action;
    char candidate[TEXT_CAPACITY];
    const char *name = op->type == OP_INSERT ? "Insert" :
                       op->type == OP_DELETE ? "Delete" : "Replace";
    memcpy(candidate, editor->text, sizeof candidate);
    if (!apply_operation(candidate, op)) return 0;
    if (strcmp(candidate, editor->text) == 0) return 1;
    memcpy(action.before, editor->text, sizeof action.before);
    memcpy(action.after, candidate, sizeof action.after);
    (void)snprintf(action.description, sizeof action.description,
                   "%s at position %zu", name, op->position);
    memcpy(editor->text, candidate, sizeof editor->text);
    stack_push(&editor->undo, &action);
    editor->redo.count = 0;
    return 1;
}

static int enqueue(PendingQueue *queue, const Operation *op) {
    size_t tail;
    if (queue->count == QUEUE_CAPACITY) return 0;
    tail = (queue->front + queue->count) % QUEUE_CAPACITY;
    queue->items[tail] = *op;
    ++queue->count;
    return 1;
}

static int dequeue(PendingQueue *queue, Operation *op) {
    if (queue->count == 0) return 0;
    *op = queue->items[queue->front];
    queue->front = (queue->front + 1) % QUEUE_CAPACITY;
    --queue->count;
    return 1;
}

static void undo(Editor *editor) {
    Action action;
    if (editor->undo.count == 0) { puts("Nothing to undo."); return; }
    action = editor->undo.items[--editor->undo.count];
    memcpy(editor->text, action.before, sizeof editor->text);
    (void)stack_push(&editor->redo, &action);
    printf("Undid: %s\n", action.description);
}

static void redo(Editor *editor) {
    Action action;
    if (editor->redo.count == 0) { puts("Nothing to redo."); return; }
    action = editor->redo.items[--editor->redo.count];
    memcpy(editor->text, action.after, sizeof editor->text);
    (void)stack_push(&editor->undo, &action);
    printf("Redid: %s\n", action.description);
}

static int make_operation(Operation *op, OperationType type) {
    int result;
    memset(op, 0, sizeof *op);
    op->type = type;
    result = read_size("Position (1-based): ", &op->position);
    if (result <= 0) { puts("Invalid position input."); return 0; }
    if (type != OP_INSERT) {
        result = read_size("Number of characters: ", &op->length);
        if (result <= 0 || op->length == 0) { puts("Length must be a positive integer."); return 0; }
    }
    if (type != OP_DELETE) {
        result = read_line(type == OP_INSERT ? "Text to insert: " : "Replacement text: ",
                           op->text, sizeof op->text);
        if (result <= 0) { puts("Invalid text input."); return 0; }
    }
    return 1;
}

static int save_file(const Editor *editor, const char *path) {
    FILE *file = fopen(path, "w");
    size_t length = strlen(editor->text);
    int ok;
    if (file == NULL) return 0;
    ok = fwrite(editor->text, 1, length, file) == length;
    if (fclose(file) != 0) ok = 0;
    return ok;
}

static int load_file(Editor *editor, const char *path) {
    char loaded[TEXT_CAPACITY];
    size_t count;
    FILE *file = fopen(path, "rb");
    Operation op;
    if (file == NULL) return 0;
    count = fread(loaded, 1, sizeof loaded, file);
    if (ferror(file) || (!feof(file) && count == sizeof loaded)) { fclose(file); return 0; }
    if (fclose(file) != 0 || memchr(loaded, '\0', count) != NULL) return 0;
    loaded[count] = '\0';
    memset(&op, 0, sizeof op);
    op.type = OP_REPLACE;
    op.position = 1;
    op.length = strlen(editor->text);
    if (op.length == 0) {
        op.type = OP_INSERT;
        op.position = 1;
    }
    if (count >= INPUT_CAPACITY) {
        Action action = {0};
        memcpy(action.before, editor->text, sizeof action.before);
        memcpy(action.after, loaded, count + 1);
        (void)snprintf(action.description, sizeof action.description, "Load file");
        memset(editor->text, 0, sizeof editor->text);
        memcpy(editor->text, loaded, count + 1);
        stack_push(&editor->undo, &action);
        editor->redo.count = 0;
        return 1;
    }
    memcpy(op.text, loaded, count + 1);
    return perform(editor, &op);
}

static void show_history(const char *label, const ActionStack *stack) {
    size_t i;
    printf("%s (%zu):\n", label, stack->count);
    for (i = stack->count; i > 0; --i) printf("  %zu. %s\n", stack->count - i + 1, stack->items[i - 1].description);
    if (stack->count == 0) puts("  (empty)");
}

static void show_pending(const PendingQueue *queue) {
    size_t i;
    puts("Pending operations (FIFO):");
    if (queue->count == 0) { puts("  (empty)"); return; }
    for (i = 0; i < queue->count; ++i) {
        const Operation *op = &queue->items[(queue->front + i) % QUEUE_CAPACITY];
        printf("  %zu. %s at %zu\n", i + 1,
               op->type == OP_INSERT ? "Insert" : op->type == OP_DELETE ? "Delete" : "Replace",
               op->position);
    }
}

static void process_pending(Editor *editor) {
    while (editor->pending.count > 0) {
        Operation op = editor->pending.items[editor->pending.front];
        if (!perform(editor, &op)) {
            printf("Could not process queued operation at position %zu; it remains at the front of the queue.\n", op.position);
            return;
        }
        (void)dequeue(&editor->pending, &op);
        puts("Processed one queued operation.");
    }
    puts("Pending queue is empty.");
}

static void show_menu(const Editor *editor) {
    size_t characters = strlen(editor->text);
    puts("\n+-------------------------------------------------------------+");
    puts("|                    ADVANCED C TEXT EDITOR                   |");
    puts("|                    Undo  /  Redo  /  Queue                  |");
    puts("+------------------------------+------------------------------+");
    printf("| %-28s | %-28s |\n", "1  Insert text", "2  Delete text");
    printf("| %-28s | %-28s |\n", "3  Replace text", "4  Undo last edit");
    printf("| %-28s | %-28s |\n", "5  Redo edit", "6  Queue an edit");
    printf("| %-28s | %-28s |\n", "7  Process queue", "8  View pending queue");
    printf("| %-28s | %-28s |\n", "9  View edit history", "10  Display document");
    printf("| %-28s | %-28s |\n", "11  Save document", "12  Load document");
    printf("| %-28s | %-28s |\n", "13  Show statistics", "0  Exit");
    puts("+------------------------------+------------------------------+");
    printf("  Document: %zu/%d characters  |  Undo: %zu/%d  |  Redo: %zu/%d\n",
           characters, TEXT_CAPACITY - 1, editor->undo.count, HISTORY_CAPACITY,
           editor->redo.count, HISTORY_CAPACITY);
    printf("  Pending edits: %zu/%d  |  Positions are 1-based\n",
           editor->pending.count, QUEUE_CAPACITY);
}

int main(void) {
    static Editor editor;
    char line[INPUT_CAPACITY];
    puts("Welcome! Your text editor is ready.");
    for (;;) {
        size_t choice;
        show_menu(&editor);
        if (!read_size("\n  Select an option [0-13]: ", &choice)) { if (feof(stdin)) break; puts("Enter a valid menu number."); continue; }
        if (choice == 0) break;
        if (choice >= 1 && choice <= 3) {
            Operation op;
            if (make_operation(&op, choice == 1 ? OP_INSERT : choice == 2 ? OP_DELETE : OP_REPLACE)) {
                if (perform(&editor, &op)) puts("Operation applied."); else puts("Operation failed: check position, length and document capacity.");
            }
        } else if (choice == 4) undo(&editor);
        else if (choice == 5) redo(&editor);
        else if (choice == 6) {
            Operation op;
            size_t kind;
            if (!read_size("Queue action: 1 Insert, 2 Delete, 3 Replace: ", &kind) || kind < 1 || kind > 3) puts("Invalid action.");
            else if (make_operation(&op, kind == 1 ? OP_INSERT : kind == 2 ? OP_DELETE : OP_REPLACE)) {
                if (enqueue(&editor.pending, &op)) puts("Operation queued."); else puts("Queue is full.");
            }
        } else if (choice == 7) process_pending(&editor);
        else if (choice == 8) show_pending(&editor.pending);
        else if (choice == 9) { show_history("Undo stack", &editor.undo); show_history("Redo stack", &editor.redo); }
        else if (choice == 10) printf("Document (%zu characters): %s\n", strlen(editor.text), editor.text);
        else if (choice == 11 || choice == 12) {
            int result = read_line("File path: ", line, sizeof line);
            if (result > 0) {
                int ok = choice == 11 ? save_file(&editor, line) : load_file(&editor, line);
                puts(ok ? (choice == 11 ? "Document saved." : "Document loaded.") : "File operation failed.");
            }
        } else if (choice == 13) {
            printf("Characters: %zu/%d\nUndo actions: %zu/%d\nRedo actions: %zu/%d\nPending: %zu/%d\n",
                   strlen(editor.text), TEXT_CAPACITY - 1, editor.undo.count, HISTORY_CAPACITY,
                   editor.redo.count, HISTORY_CAPACITY, editor.pending.count, QUEUE_CAPACITY);
        } else puts("Unknown menu choice.");
    }
    puts("Goodbye.");
    return 0;
}
