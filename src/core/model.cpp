#include "mce/model.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <random>
#include <sstream>

namespace mce
{
namespace
{
bool noteLess(const Note &a, const Note &b)
{
    const double beatA = a.startBeatValue();
    const double beatB = b.startBeatValue();
    if (beatA != beatB)
        return beatA < beatB;

    if (a.type == NoteType::Sound && b.type != NoteType::Sound)
        return false;
    if (a.type != NoteType::Sound && b.type == NoteType::Sound)
        return true;

    return a.x < b.x;
}

bool bpmLess(const BpmEntry &a, const BpmEntry &b)
{
    return a.beat.toDouble() < b.beat.toDouble();
}
} // namespace

double Beat::toDouble() const
{
    if (denominator == 0)
        return static_cast<double>(measure);
    return static_cast<double>(measure) + static_cast<double>(numerator) / denominator;
}

bool Beat::isValid() const
{
    return denominator > 0 && numerator >= 0;
}

bool Note::isValid() const
{
    if (!isTimeValid())
        return false;

    switch (type)
    {
    case NoteType::Normal:
        return isXValid();
    case NoteType::Sound:
        return !sound.empty() && volume >= 0 && volume <= 100;
    case NoteType::Rain:
        return isXValid() && endBeatValue() >= startBeatValue();
    default:
        return false;
    }
}

bool Note::isTimeValid() const
{
    if (!beat.isValid())
        return false;
    if (type == NoteType::Rain && !endBeat.isValid())
        return false;
    return type != NoteType::Rain || endBeatValue() >= startBeatValue();
}

bool Note::isXValid() const
{
    if (type == NoteType::Sound)
        return true;
    return x >= 0 && x <= 512;
}

double Note::startBeatValue() const
{
    return beat.toDouble();
}

double Note::endBeatValue() const
{
    return type == NoteType::Rain ? endBeat.toDouble() : beat.toDouble();
}

bool BpmEntry::isValid() const
{
    return beat.isValid() && bpm > 0.0;
}

bool MetaData::isValid() const
{
    return !title.empty() && !artist.empty() && !audioFile.empty();
}

void Chart::clear()
{
    notes.clear();
    bpmList.clear();
    meta = MetaData{};
    bpmList.push_back(BpmEntry{Beat{0, 0, 1}, 120.0});
}

void Chart::addNote(const Note &note)
{
    notes.push_back(note);
    sortNotes();
}

bool Chart::removeNoteById(const std::string &id)
{
    const auto it = std::find_if(notes.begin(), notes.end(), [&](const Note &note) {
        return note.id == id;
    });
    if (it == notes.end())
        return false;
    notes.erase(it);
    return true;
}

void Chart::addBpm(const BpmEntry &bpm)
{
    bpmList.push_back(bpm);
    sortBpms();
}

void Chart::sortNotes()
{
    std::sort(notes.begin(), notes.end(), noteLess);
}

void Chart::sortBpms()
{
    std::sort(bpmList.begin(), bpmList.end(), bpmLess);
}

bool Chart::isValid() const
{
    return !notes.empty() || !bpmList.empty();
}

std::string generateId()
{
    static std::atomic<std::uint64_t> counter{0};

    const auto now = static_cast<std::uint64_t>(
        std::chrono::steady_clock::now().time_since_epoch().count());

    thread_local std::mt19937_64 rng{std::random_device{}()};
    const std::uint64_t randomPart = rng();
    const std::uint64_t sequence = ++counter;

    std::ostringstream out;
    out << std::hex << now << '-' << randomPart << '-' << sequence;
    return out.str();
}

NoteType noteTypeFromInt(std::int32_t value)
{
    switch (value)
    {
    case 1:
        return NoteType::Sound;
    case 3:
        return NoteType::Rain;
    case 0:
    default:
        return NoteType::Normal;
    }
}

std::int32_t noteTypeToInt(NoteType type)
{
    return static_cast<std::int32_t>(type);
}

} // namespace mce
