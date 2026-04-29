#include "mce/ffi.h"

#include <cstdio>
#include <cstring>
#if defined(_WIN32)
#include <windows.h>
#else
#include <dlfcn.h>
#endif

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
           abi >= 4;
}

bool testCopyLastError()
{
    mce_session *session = mce_session_create();
    if (!session)
        return false;

    // Trigger an error first.
    const bool triggered = mce_session_add_normal_note(session, "bad", mce_beat{1, 0, 1}, 999) == 0;
    if (!triggered)
    {
        mce_session_destroy(session);
        return false;
    }

    char buf[256]{};
    const int32_t copied = mce_session_copy_last_error(session, buf, static_cast<int32_t>(sizeof(buf)));
    const bool copiedOk = copied > 0 && std::strlen(buf) > 0;

    char tiny[5]{};
    const int32_t copiedTiny = mce_session_copy_last_error(session, tiny, static_cast<int32_t>(sizeof(tiny)));
    const bool truncatedOk = copiedTiny == 4 && tiny[4] == '\0';

    const bool invalidOut = mce_session_copy_last_error(session, nullptr, 16) == 0 &&
                            mce_session_copy_last_error(session, buf, 0) == 0;

    mce_session_destroy(session);
    return copiedOk && truncatedOk && invalidOut;
}

bool testStableErrorCode()
{
    mce_session *session = mce_session_create();
    if (!session)
        return false;

    const bool firstOk = mce_session_add_normal_note(session, "ok-a", mce_beat{1, 0, 1}, 100) == 1 &&
                         mce_session_last_error_code(session) == MCE_ERROR_NONE;

    const bool badAdd = mce_session_add_normal_note(session, "bad-a", mce_beat{1, 0, 1}, 999) == 0 &&
                        mce_session_last_error_code(session) == MCE_ERROR_VALIDATION_FAILED;

    mce_note_snapshot note{};
    const bool badIndex = mce_session_get_note_snapshot(session, 99, &note) == 0 &&
                          mce_session_last_error_code(session) == MCE_ERROR_OUT_OF_RANGE;

    const bool badArg = mce_session_remove_note_by_id(session, nullptr) == 0 &&
                        mce_session_last_error_code(session) == MCE_ERROR_INVALID_ARGUMENT;

    const bool missing = mce_session_remove_note_by_id(session, "not-exists") == 0 &&
                         mce_session_last_error_code(session) == MCE_ERROR_NOT_FOUND;

    const bool nameOk = std::strcmp(mce_error_code_name(MCE_ERROR_NOT_FOUND), "not_found") == 0 &&
                        std::strcmp(mce_error_code_name(12345), "unknown") == 0;

    mce_session_destroy(session);
    return firstOk && badAdd && badIndex && badArg && missing && nameOk;
}

bool testAutoIdGenerationForCreateApis()
{
    mce_session *session = mce_session_create();
    if (!session)
        return false;

    const bool created = mce_session_add_normal_note(session, "", mce_beat{1, 0, 1}, 100) == 1 &&
                         mce_session_add_rain_note(session, nullptr, mce_beat{2, 0, 1}, mce_beat{3, 0, 1}, 120) == 1 &&
                         mce_session_add_sound_note(session, "", mce_beat{4, 0, 1}, "tap.wav", 70, 0) == 1;
    if (!created)
    {
        mce_session_destroy(session);
        return false;
    }

    mce_note_snapshot n0{};
    mce_note_snapshot n1{};
    mce_note_snapshot n2{};
    const bool okSnapshots = mce_session_get_note_snapshot(session, 0, &n0) == 1 &&
                             mce_session_get_note_snapshot(session, 1, &n1) == 1 &&
                             mce_session_get_note_snapshot(session, 2, &n2) == 1;
    const bool idsOk = okSnapshots &&
                       std::strlen(n0.id) > 0 &&
                       std::strlen(n1.id) > 0 &&
                       std::strlen(n2.id) > 0 &&
                       std::strcmp(n0.id, n1.id) != 0 &&
                       std::strcmp(n1.id, n2.id) != 0 &&
                       std::strcmp(n0.id, n2.id) != 0;

    mce_session_destroy(session);
    return idsOk;
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

bool testBpmApisAndUndo()
{
    mce_session *session = mce_session_create();
    if (!session)
        return false;

    const int32_t initialCount = mce_session_bpm_count(session);
    const bool addOk = initialCount >= 1 &&
                       mce_session_add_bpm(session, mce_beat{8, 0, 1}, 180.0) == 1 &&
                       mce_session_bpm_count(session) == initialCount + 1;
    if (!addOk)
    {
        mce_session_destroy(session);
        return false;
    }

    mce_bpm_snapshot added{};
    const bool snapshotOk = mce_session_get_bpm_snapshot(session, initialCount, &added) == 1 &&
                            added.beat.measure == 8 &&
                            added.bpm == 180.0;
    if (!snapshotOk)
    {
        mce_session_destroy(session);
        return false;
    }

    const bool updateOk = mce_session_update_bpm(session, initialCount, mce_beat{9, 0, 1}, 200.0) == 1;
    if (!updateOk)
    {
        mce_session_destroy(session);
        return false;
    }

    mce_bpm_snapshot updated{};
    const bool updatedOk = mce_session_get_bpm_snapshot(session, initialCount, &updated) == 1 &&
                           updated.beat.measure == 9 &&
                           updated.bpm == 200.0;
    if (!updatedOk)
    {
        mce_session_destroy(session);
        return false;
    }

    const bool removeOk = mce_session_remove_bpm(session, initialCount) == 1 &&
                          mce_session_bpm_count(session) == initialCount;
    const bool undoOk = removeOk &&
                        mce_session_undo(session) == 1 &&
                        mce_session_bpm_count(session) == initialCount + 1;

    mce_session_destroy(session);
    return undoOk;
}

bool testMetadataApisAndValidation()
{
    mce_session *session = mce_session_create();
    if (!session)
        return false;

    mce_metadata_snapshot meta{};
    const bool getOk = mce_session_get_metadata(session, &meta) == 1;
    if (!getOk)
    {
        mce_session_destroy(session);
        return false;
    }

    std::snprintf(meta.title, sizeof(meta.title), "My Title");
    std::snprintf(meta.artist, sizeof(meta.artist), "My Artist");
    std::snprintf(meta.difficulty, sizeof(meta.difficulty), "Hard");
    std::snprintf(meta.audio_file, sizeof(meta.audio_file), "song.ogg");
    meta.speed = 2;
    meta.first_bpm = 128.0;
    meta.offset_ms = 15;
    meta.preview_time_ms = 40000;

    const bool setOk = mce_session_set_metadata(session, &meta) == 1;
    if (!setOk)
    {
        mce_session_destroy(session);
        return false;
    }

    mce_metadata_snapshot check{};
    const bool checkOk = mce_session_get_metadata(session, &check) == 1 &&
                         std::strcmp(check.title, "My Title") == 0 &&
                         std::strcmp(check.artist, "My Artist") == 0 &&
                         std::strcmp(check.audio_file, "song.ogg") == 0;
    if (!checkOk)
    {
        mce_session_destroy(session);
        return false;
    }

    mce_metadata_snapshot bad = check;
    bad.audio_file[0] = '\0';
    const bool invalidRejected = mce_session_set_metadata(session, &bad) == 0 &&
                                 mce_session_last_error_code(session) == MCE_ERROR_VALIDATION_FAILED;

    mce_session_destroy(session);
    return invalidRejected;
}

bool testApplyNoteBatchSingleUndoStep()
{
    mce_session *session = mce_session_create();
    if (!session)
        return false;

    const bool seedOk = mce_session_add_normal_note(session, "batch-a", mce_beat{1, 0, 1}, 100) == 1 &&
                        mce_session_add_normal_note(session, "batch-b", mce_beat{2, 0, 1}, 200) == 1;
    if (!seedOk)
    {
        mce_session_destroy(session);
        return false;
    }

    mce_note_batch_op ops[3]{};
    ops[0].op_type = MCE_NOTE_BATCH_OP_MOVE;
    std::snprintf(ops[0].note.id, sizeof(ops[0].note.id), "batch-a");
    ops[0].note.type = 0;
    ops[0].note.beat = mce_beat{3, 0, 1};
    ops[0].note.end_beat = ops[0].note.beat;
    ops[0].note.x = 300;

    ops[1].op_type = MCE_NOTE_BATCH_OP_REMOVE;
    std::snprintf(ops[1].note.id, sizeof(ops[1].note.id), "batch-b");

    ops[2].op_type = MCE_NOTE_BATCH_OP_ADD;
    ops[2].note.type = 1;
    ops[2].note.beat = mce_beat{4, 0, 1};
    ops[2].note.end_beat = ops[2].note.beat;
    std::snprintf(ops[2].note.sound, sizeof(ops[2].note.sound), "batch.wav");
    ops[2].note.volume = 88;
    ops[2].note.offset_ms = -5;

    const uint64_t revBefore = mce_session_chart_revision(session);
    const bool applied = mce_session_apply_note_batch(session, ops, 3) == 1 &&
                         mce_session_last_error_code(session) == MCE_ERROR_NONE &&
                         mce_session_note_count(session) == 2 &&
                         mce_session_chart_revision(session) > revBefore;
    if (!applied)
    {
        mce_session_destroy(session);
        return false;
    }

    mce_note_snapshot notes[4]{};
    const int32_t copied = mce_session_get_note_snapshots(session, 0, 4, notes);
    bool movedFound = false;
    bool removedGone = true;
    bool soundAdded = false;
    for (int32_t i = 0; i < copied; ++i)
    {
        if (std::strcmp(notes[i].id, "batch-a") == 0)
            movedFound = (notes[i].x == 300 && notes[i].beat.measure == 3);
        if (std::strcmp(notes[i].id, "batch-b") == 0)
            removedGone = false;
        if (notes[i].type == 1 && std::strcmp(notes[i].sound, "batch.wav") == 0)
            soundAdded = true;
    }
    if (!(movedFound && removedGone && soundAdded))
    {
        mce_session_destroy(session);
        return false;
    }

    const bool undoOk = mce_session_undo(session) == 1 &&
                        mce_session_note_count(session) == 2;
    if (!undoOk)
    {
        mce_session_destroy(session);
        return false;
    }

    mce_note_snapshot restored[4]{};
    const int32_t restoredCount = mce_session_get_note_snapshots(session, 0, 4, restored);
    bool oldAMovedBack = false;
    bool oldBRestored = false;
    for (int32_t i = 0; i < restoredCount; ++i)
    {
        if (std::strcmp(restored[i].id, "batch-a") == 0)
            oldAMovedBack = (restored[i].x == 100 && restored[i].beat.measure == 1);
        if (std::strcmp(restored[i].id, "batch-b") == 0)
            oldBRestored = true;
    }

    mce_session_destroy(session);
    return oldAMovedBack && oldBRestored;
}

bool testChartSummarySnapshot()
{
    mce_session *session = mce_session_create();
    if (!session)
        return false;

    const bool added = mce_session_add_normal_note(session, "s1", mce_beat{1, 0, 1}, 100) == 1 &&
                       mce_session_add_normal_note(session, "s2", mce_beat{2, 0, 1}, 200) == 1;
    if (!added)
    {
        mce_session_destroy(session);
        return false;
    }

    mce_chart_summary summary{};
    const bool ok = mce_session_get_chart_summary(session, &summary) == 1 &&
                    summary.note_count == 2 &&
                    summary.bpm_count >= 1 &&
                    summary.revision > 0 &&
                    summary.can_undo == 1 &&
                    std::strlen(summary.title) > 0 &&
                    std::strlen(summary.artist) > 0 &&
                    std::strlen(summary.difficulty) > 0;

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

bool testExportedSymbols()
{
#if defined(_WIN32)
    HMODULE module = GetModuleHandleA("malody_catch_core_ffi.dll");
    if (!module)
        module = LoadLibraryA("malody_catch_core_ffi.dll");
    if (!module)
        return false;

    const char *symbols[] = {
        "mce_core_version",
        "mce_ffi_abi_version",
        "mce_session_create",
        "mce_session_last_error_code",
        "mce_error_code_name",
        "mce_session_bpm_count",
        "mce_session_get_bpm_snapshot",
        "mce_session_set_metadata",
        "mce_session_apply_note_batch",
        "mce_session_copy_last_error",
        "mce_session_get_note_snapshots",
        "mce_session_get_chart_summary",
        "mce_session_add_rain_note",
        "mce_session_add_sound_note"};
    for (const char *symbol : symbols)
    {
        if (!GetProcAddress(module, symbol))
            return false;
    }
    return true;
#else
    void *module = dlopen("libmalody_catch_core_ffi.so", RTLD_NOW);
    if (!module)
        return false;

    const char *symbols[] = {
        "mce_core_version",
        "mce_ffi_abi_version",
        "mce_session_create",
        "mce_session_last_error_code",
        "mce_error_code_name",
        "mce_session_bpm_count",
        "mce_session_get_bpm_snapshot",
        "mce_session_set_metadata",
        "mce_session_apply_note_batch",
        "mce_session_copy_last_error",
        "mce_session_get_note_snapshots",
        "mce_session_get_chart_summary",
        "mce_session_add_rain_note",
        "mce_session_add_sound_note"};
    bool ok = true;
    for (const char *symbol : symbols)
    {
        if (!dlsym(module, symbol))
        {
            ok = false;
            break;
        }
    }
    dlclose(module);
    return ok;
#endif
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
        {"Copy last error", &testCopyLastError},
        {"Stable error code", &testStableErrorCode},
        {"Auto id generation for create APIs", &testAutoIdGenerationForCreateApis},
        {"Batch snapshots", &testBatchSnapshots},
        {"BPM APIs and undo", &testBpmApisAndUndo},
        {"Metadata APIs and validation", &testMetadataApisAndValidation},
        {"Apply note batch single undo step", &testApplyNoteBatchSingleUndoStep},
        {"Chart summary snapshot", &testChartSummarySnapshot},
        {"Rain add move and snapshot", &testRainAddMoveAndSnapshot},
        {"Rain validation reports error", &testRainValidationReportsError},
        {"Sound add snapshot and validation", &testSoundAddSnapshotAndValidation},
        {"Exported symbols", &testExportedSymbols},
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
