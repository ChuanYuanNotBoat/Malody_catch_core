#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace mce
{

enum class NoteType : std::int32_t
{
    Normal = 0,
    Sound = 1,
    Rain = 3,
};

struct Beat
{
    std::int32_t measure = 0;
    std::int32_t numerator = 0;
    std::int32_t denominator = 1;

    double toDouble() const;
    bool isValid() const;
};

struct Note
{
    std::string id;
    NoteType type = NoteType::Normal;
    Beat beat;
    Beat endBeat;
    std::int32_t x = 256;
    std::string sound;
    std::int32_t volume = 100;
    std::int32_t offsetMs = 0;

    bool isValid() const;
    bool isTimeValid() const;
    bool isXValid() const;
    double startBeatValue() const;
    double endBeatValue() const;
};

struct BpmEntry
{
    Beat beat;
    double bpm = 120.0;

    bool isValid() const;
};

struct MetaData
{
    std::string title = "Untitled";
    std::string titleOriginal;
    std::string artist = "Unknown";
    std::string artistOriginal;
    std::string difficulty = "Normal";
    std::string chartAuthor;
    std::string audioFile;
    std::string backgroundFile;
    std::int32_t previewTimeMs = 0;
    double firstBpm = 120.0;
    std::int32_t offsetMs = 0;
    std::int32_t speed = 1;

    bool isValid() const;
};

struct Chart
{
    std::vector<Note> notes;
    std::vector<BpmEntry> bpmList;
    MetaData meta;

    void clear();
    void addNote(const Note &note);
    bool removeNoteById(const std::string &id);
    void addBpm(const BpmEntry &bpm);
    void sortNotes();
    void sortBpms();
    bool isValid() const;
};

std::string generateId();
NoteType noteTypeFromInt(std::int32_t value);
std::int32_t noteTypeToInt(NoteType type);

} // namespace mce
