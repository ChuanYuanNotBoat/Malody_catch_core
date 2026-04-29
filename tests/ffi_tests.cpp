#include "mce/ffi.h"

#include <cstdio>
#include <cstring>

namespace
{
bool testCreateAddSnapshotUndo()
{
    mce_session *session = mce_session_create();
    if (!session)
        return false;

    const mce_beat beat{2, 0, 1};
    bool ok = mce_session_note_count(session) == 0 &&
              mce_session_add_normal_note(session, "ffi-a", beat, 128) == 1 &&
              mce_session_note_count(session) == 1;

    mce_note_snapshot note{};
    ok = ok &&
         mce_session_get_note_snapshot(session, 0, &note) == 1 &&
         std::strcmp(note.id, "ffi-a") == 0 &&
         note.beat.measure == 2 &&
         note.x == 128 &&
         mce_session_can_undo(session) == 1 &&
         mce_session_undo(session) == 1 &&
         mce_session_note_count(session) == 0 &&
         mce_session_can_redo(session) == 1 &&
         mce_session_redo(session) == 1 &&
         mce_session_note_count(session) == 1;

    mce_session_destroy(session);
    return ok;
}

bool testInvalidAddReportsError()
{
    mce_session *session = mce_session_create();
    if (!session)
        return false;

    const mce_beat beat{1, 0, 1};
    const bool ok = mce_session_add_normal_note(session, "bad", beat, 900) == 0 &&
                    mce_session_note_count(session) == 0 &&
                    std::strlen(mce_session_last_error(session)) > 0;

    mce_session_destroy(session);
    return ok;
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
        {"Create add snapshot undo", &testCreateAddSnapshotUndo},
        {"Invalid add reports error", &testInvalidAddReportsError},
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
