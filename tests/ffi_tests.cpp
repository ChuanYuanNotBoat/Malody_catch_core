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

    const uint64_t rev0 = mce_session_chart_revision(session);
    const mce_beat beat{2, 0, 1};
    bool ok = mce_session_note_count(session) == 0 &&
              rev0 > 0 &&
              mce_session_add_normal_note(session, "ffi-a", beat, 128) == 1 &&
              mce_session_note_count(session) == 1 &&
              mce_session_chart_revision(session) > rev0;

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

bool testVersionAndAbi()
{
    const char *version = mce_core_version();
    const int32_t abi = mce_ffi_abi_version();
    return version != nullptr &&
           std::strlen(version) > 0 &&
           abi >= 1;
}

bool testBatchSnapshots()
{
    mce_session *session = mce_session_create();
    if (!session)
        return false;

    const bool okCreate = mce_session_add_normal_note(session, "n1", mce_beat{1, 0, 1}, 100) == 1 &&
                          mce_session_add_normal_note(session, "n2", mce_beat{2, 0, 1}, 200) == 1 &&
                          mce_session_add_normal_note(session, "n3", mce_beat{3, 0, 1}, 300) == 1;
    if (!okCreate)
    {
        mce_session_destroy(session);
        return false;
    }

    mce_note_snapshot out[4]{};
    const int32_t count = mce_session_get_note_snapshots(session, 1, 4, out);
    const bool ok = count == 2 &&
                    std::strcmp(out[0].id, "n2") == 0 &&
                    std::strcmp(out[1].id, "n3") == 0;
    mce_session_destroy(session);
    return ok;
}

bool testRainAddMoveAndSnapshot()
{
    mce_session *session = mce_session_create();
    if (!session)
        return false;

    const bool created = mce_session_add_rain_note(
                             session,
                             "rain-a",
                             mce_beat{3, 0, 1},
                             mce_beat{4, 0, 1},
                             192) == 1;
    if (!created)
    {
        mce_session_destroy(session);
        return false;
    }

    mce_note_snapshot before{};
    const bool snapshotBefore = mce_session_get_note_snapshot(session, 0, &before) == 1;
    if (!snapshotBefore || before.type != 3 || before.end_beat.measure != 4)
    {
        mce_session_destroy(session);
        return false;
    }

    const bool moved = mce_session_move_rain_note(
                           session,
                           "rain-a",
                           mce_beat{5, 0, 1},
                           mce_beat{6, 0, 1},
                           300) == 1;
    if (!moved)
    {
        mce_session_destroy(session);
        return false;
    }

    mce_note_snapshot after{};
    const bool snapshotAfter = mce_session_get_note_snapshot(session, 0, &after) == 1;
    const bool ok = snapshotAfter &&
                    after.type == 3 &&
                    after.beat.measure == 5 &&
                    after.end_beat.measure == 6 &&
                    after.x == 300;

    mce_session_destroy(session);
    return ok;
}

bool testRainValidationReportsError()
{
    mce_session *session = mce_session_create();
    if (!session)
        return false;

    const bool ok = mce_session_add_rain_note(
                        session,
                        "rain-bad",
                        mce_beat{3, 0, 1},
                        mce_beat{2, 0, 1},
                        256) == 0 &&
                    mce_session_note_count(session) == 0 &&
                    std::strlen(mce_session_last_error(session)) > 0;

    mce_session_destroy(session);
    return ok;
}

bool testSoundAddSnapshotAndValidation()
{
    mce_session *session = mce_session_create();
    if (!session)
        return false;

    const bool created = mce_session_add_sound_note(
                             session,
                             "sound-a",
                             mce_beat{7, 1, 4},
                             "clap.wav",
                             80,
                             -15) == 1;
    if (!created)
    {
        mce_session_destroy(session);
        return false;
    }

    mce_note_snapshot note{};
    const bool snapshot = mce_session_get_note_snapshot(session, 0, &note) == 1;
    const bool fieldsOk = snapshot &&
                          note.type == 1 &&
                          std::strcmp(note.id, "sound-a") == 0 &&
                          std::strcmp(note.sound, "clap.wav") == 0 &&
                          note.volume == 80 &&
                          note.offset_ms == -15;
    if (!fieldsOk)
    {
        mce_session_destroy(session);
        return false;
    }

    const bool invalidRejected = mce_session_add_sound_note(
                                     session,
                                     "sound-bad",
                                     mce_beat{8, 0, 1},
                                     "",
                                     50,
                                     0) == 0 &&
                                 std::strlen(mce_session_last_error(session)) > 0;

    mce_session_destroy(session);
    return invalidRejected;
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
        {"Version and ABI", &testVersionAndAbi},
        {"Batch snapshots", &testBatchSnapshots},
        {"Rain add move and snapshot", &testRainAddMoveAndSnapshot},
        {"Rain validation reports error", &testRainValidationReportsError},
        {"Sound add snapshot and validation", &testSoundAddSnapshotAndValidation},
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
