#include "mce/editor_session.hpp"

#include <cstdio>

namespace
{
mce::Note makeNote(const char *id, int beat, int x)
{
    mce::Note note;
    note.id = id;
    note.type = mce::NoteType::Normal;
    note.beat = {beat, 0, 1};
    note.endBeat = note.beat;
    note.x = x;
    return note;
}

bool testAddUndoRedo()
{
    mce::EditorSession session;
    session.mutableChart().notes.clear();

    session.addNote(makeNote("a", 1, 128));
    if (session.chart().notes.size() != 1 || !session.canUndo())
        return false;

    if (!session.undo() || !session.chart().notes.empty() || !session.canRedo())
        return false;

    if (!session.redo())
        return false;
    return session.chart().notes.size() == 1 && session.chart().notes.front().id == "a";
}

bool testMoveKeepsId()
{
    mce::EditorSession session;
    session.mutableChart().notes.clear();
    session.addNote(makeNote("a", 1, 128));

    mce::Note moved = makeNote("different", 3, 256);
    if (!session.moveNoteById("a", moved))
        return false;

    const auto &notes = session.chart().notes;
    return notes.size() == 1 &&
           notes.front().id == "a" &&
           notes.front().beat.measure == 3 &&
           notes.front().x == 256;
}

bool testInvalidEditDoesNotMutate()
{
    mce::EditorSession session;
    session.mutableChart().notes.clear();
    session.addNote(makeNote("a", 1, 128));

    mce::Note invalid = makeNote("bad", 2, 999);
    session.addNote(invalid);

    return session.chart().notes.size() == 1 &&
           !session.lastError().empty();
}
} // namespace

int main()
{
    struct Case
    {
        const char *name;
        bool (*fn)();
    };

    const Case cases[] = {
        {"Add undo redo", &testAddUndoRedo},
        {"Move keeps id", &testMoveKeepsId},
        {"Invalid edit does not mutate", &testInvalidEditDoesNotMutate},
    };

    int failed = 0;
    for (const Case &c : cases)
    {
        if (!c.fn())
        {
            std::fprintf(stderr, "FAILED: %s\n", c.name);
            ++failed;
        }
        else
        {
            std::fprintf(stdout, "PASSED: %s\n", c.name);
        }
    }

    return failed == 0 ? 0 : 1;
}
