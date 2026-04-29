#include "mce/ffi.h"

#include "mce/editor_session.hpp"

#include <algorithm>
#include <cstring>
#include <new>

struct mce_session
{
    mce::EditorSession impl;
    int32_t lastErrorCode = MCE_ERROR_NONE;
};

namespace
{
void setLastErrorCode(const mce_session *session, int32_t code)
{
    if (!session)
        return;
    const_cast<mce_session *>(session)->lastErrorCode = code;
}

void setLastErrorCode(mce_session *session, int32_t code)
{
    if (!session)
        return;
    session->lastErrorCode = code;
}

void copyCString(char *dest, int destSize, const std::string &src)
{
    if (!dest || destSize <= 0)
        return;
    const int count = std::min<int>(destSize - 1, static_cast<int>(src.size()));
    if (count > 0)
        std::memcpy(dest, src.data(), static_cast<std::size_t>(count));
    dest[count] = '\0';
}

mce::Beat toCoreBeat(mce_beat beat)
{
    return mce::Beat{beat.measure, beat.numerator, beat.denominator};
}

mce_beat toFfiBeat(const mce::Beat &beat)
{
    return mce_beat{beat.measure, beat.numerator, beat.denominator};
}

int32_t ok(bool value)
{
    return value ? 1 : 0;
}

int32_t currentErrorCode(const mce_session *session)
{
    if (!session)
        return MCE_ERROR_INVALID_SESSION;
    return session->lastErrorCode;
}

const char *errorCodeName(int32_t code)
{
    switch (code)
    {
    case MCE_ERROR_NONE:
        return "none";
    case MCE_ERROR_INVALID_SESSION:
        return "invalid_session";
    case MCE_ERROR_INVALID_ARGUMENT:
        return "invalid_argument";
    case MCE_ERROR_OUT_OF_RANGE:
        return "out_of_range";
    case MCE_ERROR_VALIDATION_FAILED:
        return "validation_failed";
    case MCE_ERROR_NOT_FOUND:
        return "not_found";
    case MCE_ERROR_OPERATION_FAILED:
        return "operation_failed";
    default:
        return "unknown";
    }
}

void fillSnapshot(const mce::Note &note, mce_note_snapshot *out_note)
{
    if (!out_note)
        return;
    std::memset(out_note, 0, sizeof(*out_note));
    copyCString(out_note->id, static_cast<int>(sizeof(out_note->id)), note.id);
    out_note->type = mce::noteTypeToInt(note.type);
    out_note->beat = toFfiBeat(note.beat);
    out_note->end_beat = toFfiBeat(note.endBeat);
    out_note->x = note.x;
    copyCString(out_note->sound, static_cast<int>(sizeof(out_note->sound)), note.sound);
    out_note->volume = note.volume;
    out_note->offset_ms = note.offsetMs;
}
} // namespace

mce_session *mce_session_create(void)
{
    try
    {
        return new mce_session{};
    }
    catch (const std::bad_alloc &)
    {
        return nullptr;
    }
}

void mce_session_destroy(mce_session *session)
{
    delete session;
}

const char *mce_session_last_error(const mce_session *session)
{
    if (!session)
        return "Invalid session.";
    return session->impl.lastError().c_str();
}

int32_t mce_session_last_error_code(const mce_session *session)
{
    return currentErrorCode(session);
}

const char *mce_error_code_name(int32_t code)
{
    return errorCodeName(code);
}

int32_t mce_session_copy_last_error(const mce_session *session,
                                    char *out_error,
                                    int32_t out_capacity)
{
    if (!out_error || out_capacity <= 0)
        return 0;

    const char *message = mce_session_last_error(session);
    if (!message)
    {
        out_error[0] = '\0';
        return 0;
    }

    const int32_t full_len = static_cast<int32_t>(std::strlen(message));
    const int32_t copy_len = std::min<int32_t>(full_len, out_capacity - 1);
    if (copy_len > 0)
        std::memcpy(out_error, message, static_cast<std::size_t>(copy_len));
    out_error[copy_len] = '\0';
    return copy_len;
}

const char *mce_core_version(void)
{
    return "0.3.0";
}

int32_t mce_ffi_abi_version(void)
{
    return 2;
}

int32_t mce_session_note_count(const mce_session *session)
{
    if (!session)
        return -1;
    setLastErrorCode(session, MCE_ERROR_NONE);
    return static_cast<int32_t>(session->impl.chart().notes.size());
}

uint64_t mce_session_chart_revision(const mce_session *session)
{
    if (!session)
        return 0;
    setLastErrorCode(session, MCE_ERROR_NONE);
    return session->impl.revision();
}

int32_t mce_session_get_note_snapshot(const mce_session *session,
                                      int32_t index,
                                      mce_note_snapshot *out_note)
{
    if (!session || !out_note)
    {
        setLastErrorCode(session, session ? MCE_ERROR_INVALID_ARGUMENT : MCE_ERROR_INVALID_SESSION);
        return 0;
    }

    const auto &notes = session->impl.chart().notes;
    if (index < 0 || static_cast<std::size_t>(index) >= notes.size())
    {
        setLastErrorCode(session, MCE_ERROR_OUT_OF_RANGE);
        return 0;
    }

    const mce::Note &note = notes[static_cast<std::size_t>(index)];
    fillSnapshot(note, out_note);
    setLastErrorCode(session, MCE_ERROR_NONE);
    return 1;
}

int32_t mce_session_get_note_snapshots(const mce_session *session,
                                       int32_t start_index,
                                       int32_t max_count,
                                       mce_note_snapshot *out_notes)
{
    if (!session || !out_notes || start_index < 0 || max_count <= 0)
    {
        setLastErrorCode(session, session ? MCE_ERROR_INVALID_ARGUMENT : MCE_ERROR_INVALID_SESSION);
        return 0;
    }

    const auto &notes = session->impl.chart().notes;
    const std::size_t start = static_cast<std::size_t>(start_index);
    if (start >= notes.size())
    {
        setLastErrorCode(session, MCE_ERROR_OUT_OF_RANGE);
        return 0;
    }

    const std::size_t remaining = notes.size() - start;
    const std::size_t requested = static_cast<std::size_t>(max_count);
    const std::size_t count = std::min(remaining, requested);

    for (std::size_t i = 0; i < count; ++i)
    {
        fillSnapshot(notes[start + i], &out_notes[i]);
    }
    setLastErrorCode(session, MCE_ERROR_NONE);
    return static_cast<int32_t>(count);
}

int32_t mce_session_get_chart_summary(const mce_session *session,
                                      mce_chart_summary *out_summary)
{
    if (!session || !out_summary)
    {
        setLastErrorCode(session, session ? MCE_ERROR_INVALID_ARGUMENT : MCE_ERROR_INVALID_SESSION);
        return 0;
    }

    const auto &chart = session->impl.chart();
    std::memset(out_summary, 0, sizeof(*out_summary));
    out_summary->note_count = static_cast<int32_t>(chart.notes.size());
    out_summary->bpm_count = static_cast<int32_t>(chart.bpmList.size());
    out_summary->revision = session->impl.revision();
    out_summary->can_undo = ok(session->impl.canUndo());
    out_summary->can_redo = ok(session->impl.canRedo());
    copyCString(out_summary->title, static_cast<int>(sizeof(out_summary->title)), chart.meta.title);
    copyCString(out_summary->artist, static_cast<int>(sizeof(out_summary->artist)), chart.meta.artist);
    copyCString(out_summary->difficulty, static_cast<int>(sizeof(out_summary->difficulty)), chart.meta.difficulty);
    setLastErrorCode(session, MCE_ERROR_NONE);
    return 1;
}

int32_t mce_session_add_normal_note(mce_session *session,
                                    const char *id,
                                    mce_beat beat,
                                    int32_t x)
{
    if (!session)
    {
        setLastErrorCode(session, MCE_ERROR_INVALID_SESSION);
        return 0;
    }

    mce::Note note;
    note.id = id ? id : "";
    note.type = mce::NoteType::Normal;
    note.beat = toCoreBeat(beat);
    note.endBeat = note.beat;
    note.x = x;

    const std::size_t before = session->impl.chart().notes.size();
    session->impl.addNote(note);
    const bool added = session->impl.chart().notes.size() == before + 1;
    setLastErrorCode(session, added ? MCE_ERROR_NONE : MCE_ERROR_VALIDATION_FAILED);
    return ok(added);
}

int32_t mce_session_add_rain_note(mce_session *session,
                                  const char *id,
                                  mce_beat beat,
                                  mce_beat end_beat,
                                  int32_t x)
{
    if (!session)
    {
        setLastErrorCode(session, MCE_ERROR_INVALID_SESSION);
        return 0;
    }

    mce::Note note;
    note.id = id ? id : "";
    note.type = mce::NoteType::Rain;
    note.beat = toCoreBeat(beat);
    note.endBeat = toCoreBeat(end_beat);
    note.x = x;

    const std::size_t before = session->impl.chart().notes.size();
    session->impl.addNote(note);
    const bool added = session->impl.chart().notes.size() == before + 1;
    setLastErrorCode(session, added ? MCE_ERROR_NONE : MCE_ERROR_VALIDATION_FAILED);
    return ok(added);
}

int32_t mce_session_move_rain_note(mce_session *session,
                                   const char *id,
                                   mce_beat beat,
                                   mce_beat end_beat,
                                   int32_t x)
{
    if (!session || !id)
    {
        setLastErrorCode(session, session ? MCE_ERROR_INVALID_ARGUMENT : MCE_ERROR_INVALID_SESSION);
        return 0;
    }

    mce::Note note;
    note.id = id;
    note.type = mce::NoteType::Rain;
    note.beat = toCoreBeat(beat);
    note.endBeat = toCoreBeat(end_beat);
    note.x = x;

    const bool moved = session->impl.moveNoteById(id, note);
    setLastErrorCode(session, moved ? MCE_ERROR_NONE : MCE_ERROR_NOT_FOUND);
    return ok(moved);
}

int32_t mce_session_add_sound_note(mce_session *session,
                                   const char *id,
                                   mce_beat beat,
                                   const char *sound,
                                   int32_t volume,
                                   int32_t offset_ms)
{
    if (!session)
    {
        setLastErrorCode(session, MCE_ERROR_INVALID_SESSION);
        return 0;
    }

    mce::Note note;
    note.id = id ? id : "";
    note.type = mce::NoteType::Sound;
    note.beat = toCoreBeat(beat);
    note.endBeat = note.beat;
    note.x = -1;
    note.sound = sound ? sound : "";
    note.volume = volume;
    note.offsetMs = offset_ms;

    const std::size_t before = session->impl.chart().notes.size();
    session->impl.addNote(note);
    const bool added = session->impl.chart().notes.size() == before + 1;
    setLastErrorCode(session, added ? MCE_ERROR_NONE : MCE_ERROR_VALIDATION_FAILED);
    return ok(added);
}

int32_t mce_session_remove_note_by_id(mce_session *session, const char *id)
{
    if (!session || !id)
    {
        setLastErrorCode(session, session ? MCE_ERROR_INVALID_ARGUMENT : MCE_ERROR_INVALID_SESSION);
        return 0;
    }
    const bool removed = session->impl.removeNoteById(id);
    setLastErrorCode(session, removed ? MCE_ERROR_NONE : MCE_ERROR_NOT_FOUND);
    return ok(removed);
}

int32_t mce_session_can_undo(const mce_session *session)
{
    if (!session)
        return 0;
    setLastErrorCode(session, MCE_ERROR_NONE);
    return ok(session->impl.canUndo());
}

int32_t mce_session_can_redo(const mce_session *session)
{
    if (!session)
        return 0;
    setLastErrorCode(session, MCE_ERROR_NONE);
    return ok(session->impl.canRedo());
}

int32_t mce_session_undo(mce_session *session)
{
    if (!session)
    {
        setLastErrorCode(session, MCE_ERROR_INVALID_SESSION);
        return 0;
    }
    const bool done = session->impl.undo();
    setLastErrorCode(session, done ? MCE_ERROR_NONE : MCE_ERROR_OPERATION_FAILED);
    return ok(done);
}

int32_t mce_session_redo(mce_session *session)
{
    if (!session)
    {
        setLastErrorCode(session, MCE_ERROR_INVALID_SESSION);
        return 0;
    }
    const bool done = session->impl.redo();
    setLastErrorCode(session, done ? MCE_ERROR_NONE : MCE_ERROR_OPERATION_FAILED);
    return ok(done);
}
