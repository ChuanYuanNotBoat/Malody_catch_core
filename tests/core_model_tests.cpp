#include "mce/model.hpp"

#include <cstdio>

namespace
{
bool testBeatValidity()
{
    return mce::Beat{2, 1, 4}.isValid() &&
           !mce::Beat{2, 1, 0}.isValid() &&
           !mce::Beat{2, -1, 4}.isValid();
}

bool testNoteValidation()
{
    mce::Note normal;
    normal.id = mce::generateId();
    normal.type = mce::NoteType::Normal;
    normal.beat = {1, 0, 1};
    normal.endBeat = normal.beat;
    normal.x = 256;

    mce::Note invalidX = normal;
    invalidX.x = 900;

    mce::Note sound = normal;
    sound.type = mce::NoteType::Sound;
    sound.x = -1;
    sound.sound = "hit.wav";

    mce::Note rain = normal;
    rain.type = mce::NoteType::Rain;
    rain.beat = {1, 0, 1};
    rain.endBeat = {2, 0, 1};

    mce::Note invalidRain = rain;
    invalidRain.endBeat = {0, 3, 4};

    return normal.isValid() &&
           !invalidX.isValid() &&
           sound.isValid() &&
           rain.isValid() &&
           !invalidRain.isValid();
}

bool testChartSortingAndRemoval()
{
    mce::Chart chart;
    chart.clear();
    chart.notes.clear();

    mce::Note later;
    later.id = "later";
    later.beat = {4, 0, 1};
    later.endBeat = later.beat;
    later.x = 100;

    mce::Note earlier = later;
    earlier.id = "earlier";
    earlier.beat = {1, 0, 1};
    earlier.endBeat = earlier.beat;

    chart.addNote(later);
    chart.addNote(earlier);

    if (chart.notes.size() != 2 || chart.notes.front().id != "earlier")
        return false;
    if (!chart.removeNoteById("later"))
        return false;
    return chart.notes.size() == 1 && chart.notes.front().id == "earlier";
}

bool testBpmSorting()
{
    mce::Chart chart;
    chart.clear();
    chart.bpmList.clear();

    chart.addBpm({{8, 0, 1}, 180.0});
    chart.addBpm({{0, 0, 1}, 120.0});
    chart.addBpm({{4, 1, 2}, 150.0});

    return chart.bpmList.size() == 3 &&
           chart.bpmList[0].beat.measure == 0 &&
           chart.bpmList[1].beat.measure == 4 &&
           chart.bpmList[1].beat.numerator == 1 &&
           chart.bpmList[2].beat.measure == 8;
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
        {"Beat validity", &testBeatValidity},
        {"Note validation", &testNoteValidation},
        {"Chart sorting and removal", &testChartSortingAndRemoval},
        {"BPM sorting", &testBpmSorting},
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
