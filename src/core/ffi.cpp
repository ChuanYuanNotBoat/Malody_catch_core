#include "mce/ffi.h"

#include "mce/editor_session.hpp"

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <new>
#include <string>

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

bool noteIdExists(const mce_session *session, const std::string &id)
{
    if (!session || id.empty())
        return false;
    const auto &notes = session->impl.chart().notes;
    for (const auto &note : notes)
    {
        if (note.id == id)
            return true;
    }
    return false;
}

bool noteIdExistsInChart(const mce::Chart &chart, const std::string &id)
{
    if (id.empty())
        return false;
    for (const auto &note : chart.notes)
    {
        if (note.id == id)
            return true;
    }
    return false;
}

std::string normalizeCreateId(const mce_session *session, const char *id)
{
    const std::string input = id ? id : "";
    if (!input.empty())
        return input;

    const auto &notes = session->impl.chart().notes;
    std::size_t seq = notes.size() + 1;
    while (true)
    {
        const std::string candidate = "mce-auto-" + std::to_string(seq);
        if (!noteIdExists(session, candidate))
            return candidate;
        ++seq;
    }
}

std::string normalizeCreateIdInChart(const mce::Chart &chart, const char *id)
{
    const std::string input = id ? id : "";
    if (!input.empty())
        return input;

    std::size_t seq = chart.notes.size() + 1;
    while (true)
    {
        const std::string candidate = "mce-auto-" + std::to_string(seq);
        if (!noteIdExistsInChart(chart, candidate))
            return candidate;
        ++seq;
    }
}

std::string fromCString(const char *src)
{
    return src ? std::string(src) : std::string{};
}

mce::Note noteFromSnapshot(const mce_note_snapshot &snapshot)
{
    mce::Note note;
    note.id = fromCString(snapshot.id);
    note.type = mce::noteTypeFromInt(snapshot.type);
    note.beat = toCoreBeat(snapshot.beat);
    note.endBeat = toCoreBeat(snapshot.end_beat);
    note.x = snapshot.x;
    note.sound = fromCString(snapshot.sound);
    note.volume = snapshot.volume;
    note.offsetMs = snapshot.offset_ms;
    return note;
}

void fillBpmSnapshot(const mce::BpmEntry &bpm, mce_bpm_snapshot *out_bpm)
{
    if (!out_bpm)
        return;
    std::memset(out_bpm, 0, sizeof(*out_bpm));
    out_bpm->beat = toFfiBeat(bpm.beat);
    out_bpm->bpm = bpm.bpm;
}

void fillMetadataSnapshot(const mce::MetaData &meta, mce_metadata_snapshot *out_meta)
{
    if (!out_meta)
        return;
    std::memset(out_meta, 0, sizeof(*out_meta));
    copyCString(out_meta->title, static_cast<int>(sizeof(out_meta->title)), meta.title);
    copyCString(out_meta->title_original, static_cast<int>(sizeof(out_meta->title_original)), meta.titleOriginal);
    copyCString(out_meta->artist, static_cast<int>(sizeof(out_meta->artist)), meta.artist);
    copyCString(out_meta->artist_original, static_cast<int>(sizeof(out_meta->artist_original)), meta.artistOriginal);
    copyCString(out_meta->difficulty, static_cast<int>(sizeof(out_meta->difficulty)), meta.difficulty);
    copyCString(out_meta->chart_author, static_cast<int>(sizeof(out_meta->chart_author)), meta.chartAuthor);
    copyCString(out_meta->audio_file, static_cast<int>(sizeof(out_meta->audio_file)), meta.audioFile);
    copyCString(out_meta->background_file, static_cast<int>(sizeof(out_meta->background_file)), meta.backgroundFile);
    out_meta->preview_time_ms = meta.previewTimeMs;
    out_meta->first_bpm = meta.firstBpm;
    out_meta->offset_ms = meta.offsetMs;
    out_meta->speed = meta.speed;
}

mce::MetaData toCoreMetadata(const mce_metadata_snapshot &meta)
{
    mce::MetaData out;
    out.title = fromCString(meta.title);
    out.titleOriginal = fromCString(meta.title_original);
    out.artist = fromCString(meta.artist);
    out.artistOriginal = fromCString(meta.artist_original);
    out.difficulty = fromCString(meta.difficulty);
    out.chartAuthor = fromCString(meta.chart_author);
    out.audioFile = fromCString(meta.audio_file);
    out.backgroundFile = fromCString(meta.background_file);
    out.previewTimeMs = meta.preview_time_ms;
    out.firstBpm = meta.first_bpm;
    out.offsetMs = meta.offset_ms;
    out.speed = meta.speed;
    return out;
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
    return "0.5.0";
}

int32_t mce_ffi_abi_version(void)
{
    return 4;
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

int32_t mce_session_bpm_count(const mce_session *session)
{
    if (!session)
        return -1;
    setLastErrorCode(session, MCE_ERROR_NONE);
    return static_cast<int32_t>(session->impl.chart().bpmList.size());
}

int32_t mce_session_get_bpm_snapshot(const mce_session *session,
                                     int32_t index,
                                     mce_bpm_snapshot *out_bpm)
{
    if (!session || !out_bpm)
    {
        setLastErrorCode(session, session ? MCE_ERROR_INVALID_ARGUMENT : MCE_ERROR_INVALID_SESSION);
        return 0;
    }

    const auto &bpms = session->impl.chart().bpmList;
    if (index < 0 || static_cast<std::size_t>(index) >= bpms.size())
    {
        setLastErrorCode(session, MCE_ERROR_OUT_OF_RANGE);
        return 0;
    }

    fillBpmSnapshot(bpms[static_cast<std::size_t>(index)], out_bpm);
    setLastErrorCode(session, MCE_ERROR_NONE);
    return 1;
}

int32_t mce_session_get_metadata(const mce_session *session,
                                 mce_metadata_snapshot *out_metadata)
{
    if (!session || !out_metadata)
    {
        setLastErrorCode(session, session ? MCE_ERROR_INVALID_ARGUMENT : MCE_ERROR_INVALID_SESSION);
        return 0;
    }

    fillMetadataSnapshot(session->impl.chart().meta, out_metadata);
    setLastErrorCode(session, MCE_ERROR_NONE);
    return 1;
}

int32_t mce_session_set_metadata(mce_session *session,
                                 const mce_metadata_snapshot *metadata)
{
    if (!session || !metadata)
    {
        setLastErrorCode(session, session ? MCE_ERROR_INVALID_ARGUMENT : MCE_ERROR_INVALID_SESSION);
        return 0;
    }

    mce::Chart next = session->impl.chart();
    const mce::MetaData meta = toCoreMetadata(*metadata);
    if (!meta.isValid())
    {
        setLastErrorCode(session, MCE_ERROR_VALIDATION_FAILED);
        return 0;
    }
    next.meta = meta;
    session->impl.replaceChart(next, "Set Metadata");
    setLastErrorCode(session, MCE_ERROR_NONE);
    return 1;
}

int32_t mce_session_add_bpm(mce_session *session, mce_beat beat, double bpm)
{
    if (!session)
    {
        setLastErrorCode(session, MCE_ERROR_INVALID_SESSION);
        return 0;
    }

    mce::BpmEntry entry{toCoreBeat(beat), bpm};
    if (!entry.isValid())
    {
        setLastErrorCode(session, MCE_ERROR_VALIDATION_FAILED);
        return 0;
    }

    mce::Chart next = session->impl.chart();
    next.addBpm(entry);
    session->impl.replaceChart(next, "Add BPM");
    setLastErrorCode(session, MCE_ERROR_NONE);
    return 1;
}

int32_t mce_session_update_bpm(mce_session *session,
                               int32_t index,
                               mce_beat beat,
                               double bpm)
{
    if (!session)
    {
        setLastErrorCode(session, MCE_ERROR_INVALID_SESSION);
        return 0;
    }
    if (index < 0)
    {
        setLastErrorCode(session, MCE_ERROR_INVALID_ARGUMENT);
        return 0;
    }

    mce::BpmEntry entry{toCoreBeat(beat), bpm};
    if (!entry.isValid())
    {
        setLastErrorCode(session, MCE_ERROR_VALIDATION_FAILED);
        return 0;
    }

    mce::Chart next = session->impl.chart();
    const std::size_t i = static_cast<std::size_t>(index);
    if (i >= next.bpmList.size())
    {
        setLastErrorCode(session, MCE_ERROR_OUT_OF_RANGE);
        return 0;
    }
    next.bpmList[i] = entry;
    next.sortBpms();
    session->impl.replaceChart(next, "Update BPM");
    setLastErrorCode(session, MCE_ERROR_NONE);
    return 1;
}

int32_t mce_session_remove_bpm(mce_session *session, int32_t index)
{
    if (!session)
    {
        setLastErrorCode(session, MCE_ERROR_INVALID_SESSION);
        return 0;
    }
    if (index < 0)
    {
        setLastErrorCode(session, MCE_ERROR_INVALID_ARGUMENT);
        return 0;
    }

    mce::Chart next = session->impl.chart();
    const std::size_t i = static_cast<std::size_t>(index);
    if (i >= next.bpmList.size())
    {
        setLastErrorCode(session, MCE_ERROR_OUT_OF_RANGE);
        return 0;
    }
    next.bpmList.erase(next.bpmList.begin() + static_cast<std::ptrdiff_t>(i));
    session->impl.replaceChart(next, "Remove BPM");
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
    note.id = normalizeCreateId(session, id);
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
    note.id = normalizeCreateId(session, id);
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
    note.id = normalizeCreateId(session, id);
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

int32_t mce_session_apply_note_batch(mce_session *session,
                                     const mce_note_batch_op *ops,
                                     int32_t op_count)
{
    if (!session)
    {
        setLastErrorCode(session, MCE_ERROR_INVALID_SESSION);
        return 0;
    }
    if (!ops || op_count <= 0)
    {
        setLastErrorCode(session, MCE_ERROR_INVALID_ARGUMENT);
        return 0;
    }

    mce::Chart next = session->impl.chart();
    for (int32_t i = 0; i < op_count; ++i)
    {
        const mce_note_batch_op &op = ops[i];
        if (op.op_type == MCE_NOTE_BATCH_OP_ADD)
        {
            mce::Note note = noteFromSnapshot(op.note);
            note.id = normalizeCreateIdInChart(next, op.note.id);
            if (!note.isValid())
            {
                setLastErrorCode(session, MCE_ERROR_VALIDATION_FAILED);
                return 0;
            }
            next.addNote(note);
            continue;
        }

        const std::string id = fromCString(op.note.id);
        if (id.empty())
        {
            setLastErrorCode(session, MCE_ERROR_INVALID_ARGUMENT);
            return 0;
        }

        if (op.op_type == MCE_NOTE_BATCH_OP_REMOVE)
        {
            if (!next.removeNoteById(id))
            {
                setLastErrorCode(session, MCE_ERROR_NOT_FOUND);
                return 0;
            }
            continue;
        }

        if (op.op_type == MCE_NOTE_BATCH_OP_MOVE)
        {
            mce::Note replacement = noteFromSnapshot(op.note);
            replacement.id = id;
            if (!replacement.isValid())
            {
                setLastErrorCode(session, MCE_ERROR_VALIDATION_FAILED);
                return 0;
            }

            auto it = std::find_if(next.notes.begin(), next.notes.end(), [&](const mce::Note &note) {
                return note.id == id;
            });
            if (it == next.notes.end())
            {
                setLastErrorCode(session, MCE_ERROR_NOT_FOUND);
                return 0;
            }

            *it = replacement;
            next.sortNotes();
            continue;
        }

        setLastErrorCode(session, MCE_ERROR_INVALID_ARGUMENT);
        return 0;
    }

    session->impl.replaceChart(next, "Batch Note Edit");
    setLastErrorCode(session, MCE_ERROR_NONE);
    return 1;
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
