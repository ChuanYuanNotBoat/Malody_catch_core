#pragma once

#include "mce/model.hpp"

#include <string>
#include <vector>

namespace mce
{

class EditorSession
{
public:
    EditorSession();

    const Chart &chart() const;
    Chart &mutableChart();
    void replaceChart(const Chart &chart, const std::string &actionName = "Replace Chart");

    void addNote(Note note);
    bool removeNoteById(const std::string &id);
    bool moveNoteById(const std::string &id, const Note &replacement);

    bool canUndo() const;
    bool canRedo() const;
    bool undo();
    bool redo();
    std::string nextUndoActionName() const;
    std::string nextRedoActionName() const;

    const std::string &lastError() const;
    void clearHistory();

private:
    struct HistoryEntry
    {
        std::string actionName;
        Chart before;
        Chart after;
    };

    void pushHistory(std::string actionName, Chart before, Chart after);
    void setError(std::string message);
    void clearError();

    Chart m_chart;
    std::vector<HistoryEntry> m_undoStack;
    std::vector<HistoryEntry> m_redoStack;
    std::string m_lastError;
};

} // namespace mce
