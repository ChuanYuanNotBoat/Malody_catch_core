#include "mce/editor_session.hpp"

#include <algorithm>
#include <utility>

namespace mce
{

EditorSession::EditorSession()
{
    m_chart.clear();
    m_revision = 1;
}

const Chart &EditorSession::chart() const
{
    return m_chart;
}

Chart &EditorSession::mutableChart()
{
    return m_chart;
}

void EditorSession::replaceChart(const Chart &chart, const std::string &actionName)
{
    Chart next = chart;
    next.sortNotes();
    next.sortBpms();
    pushHistory(actionName.empty() ? "Replace Chart" : actionName, m_chart, next);
    m_chart = std::move(next);
    ++m_revision;
    clearError();
}

void EditorSession::addNote(Note note)
{
    if (note.id.empty())
        note.id = generateId();
    if (!note.isValid())
    {
        setError("Cannot add invalid note.");
        return;
    }

    Chart next = m_chart;
    next.addNote(note);
    pushHistory("Add Note", m_chart, next);
    m_chart = std::move(next);
    ++m_revision;
    clearError();
}

bool EditorSession::removeNoteById(const std::string &id)
{
    if (id.empty())
    {
        setError("Cannot remove note with empty id.");
        return false;
    }

    Chart next = m_chart;
    if (!next.removeNoteById(id))
    {
        setError("Note id not found.");
        return false;
    }

    pushHistory("Remove Note", m_chart, next);
    m_chart = std::move(next);
    ++m_revision;
    clearError();
    return true;
}

bool EditorSession::moveNoteById(const std::string &id, const Note &replacement)
{
    if (id.empty())
    {
        setError("Cannot move note with empty id.");
        return false;
    }
    if (!replacement.isValid())
    {
        setError("Cannot move note to invalid replacement.");
        return false;
    }

    Chart next = m_chart;
    auto it = std::find_if(next.notes.begin(), next.notes.end(), [&](const Note &note) {
        return note.id == id;
    });
    if (it == next.notes.end())
    {
        setError("Note id not found.");
        return false;
    }

    Note updated = replacement;
    updated.id = id;
    *it = updated;
    next.sortNotes();

    pushHistory("Move Note", m_chart, next);
    m_chart = std::move(next);
    ++m_revision;
    clearError();
    return true;
}

bool EditorSession::canUndo() const
{
    return !m_undoStack.empty();
}

bool EditorSession::canRedo() const
{
    return !m_redoStack.empty();
}

bool EditorSession::undo()
{
    if (m_undoStack.empty())
    {
        setError("Nothing to undo.");
        return false;
    }

    HistoryEntry entry = std::move(m_undoStack.back());
    m_undoStack.pop_back();
    m_chart = entry.before;
    m_redoStack.push_back(std::move(entry));
    ++m_revision;
    clearError();
    return true;
}

bool EditorSession::redo()
{
    if (m_redoStack.empty())
    {
        setError("Nothing to redo.");
        return false;
    }

    HistoryEntry entry = std::move(m_redoStack.back());
    m_redoStack.pop_back();
    m_chart = entry.after;
    m_undoStack.push_back(std::move(entry));
    ++m_revision;
    clearError();
    return true;
}

std::string EditorSession::nextUndoActionName() const
{
    return m_undoStack.empty() ? std::string{} : m_undoStack.back().actionName;
}

std::string EditorSession::nextRedoActionName() const
{
    return m_redoStack.empty() ? std::string{} : m_redoStack.back().actionName;
}

std::uint64_t EditorSession::revision() const
{
    return m_revision;
}

const std::string &EditorSession::lastError() const
{
    return m_lastError;
}

void EditorSession::clearHistory()
{
    m_undoStack.clear();
    m_redoStack.clear();
}

void EditorSession::pushHistory(std::string actionName, Chart before, Chart after)
{
    m_undoStack.push_back(HistoryEntry{std::move(actionName), std::move(before), std::move(after)});
    m_redoStack.clear();
}

void EditorSession::setError(std::string message)
{
    m_lastError = std::move(message);
}

void EditorSession::clearError()
{
    m_lastError.clear();
}

} // namespace mce
