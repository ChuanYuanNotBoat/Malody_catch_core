# Mobile Alignment Samples (Core-Side)

## Purpose

Provide stable sample scenarios for cross-repo regression (`core` + `mobile`).

## Sample Set

1. Basic add/move/remove
- add 3 notes
- move one note
- remove one note
- verify note count and summary

2. Batch atomic behavior
- apply mixed valid ops
- apply mixed ops with one invalid remove
- verify failure is atomic and revision unchanged

3. Undo/redo chain
- perform 5 edits
- undo all
- redo all
- verify summary and key note snapshots

4. BPM/meta edit path
- add/update/remove bpm
- update metadata title/artist/audio
- verify snapshots and revision increments

## Output Expectations

- stable pass/fail outcome
- deterministic summary fields
- deterministic error code class on failures
