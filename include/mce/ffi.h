#pragma once

#include <stdint.h>

#if defined(_WIN32) && defined(MCE_BUILD_SHARED)
#define MCE_API __declspec(dllexport)
#elif defined(_WIN32)
#define MCE_API
#else
#define MCE_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mce_session mce_session;

typedef struct mce_beat
{
    int32_t measure;
    int32_t numerator;
    int32_t denominator;
} mce_beat;

typedef struct mce_note_snapshot
{
    char id[128];
    int32_t type;
    mce_beat beat;
    mce_beat end_beat;
    int32_t x;
    char sound[260];
    int32_t volume;
    int32_t offset_ms;
} mce_note_snapshot;

typedef struct mce_chart_summary
{
    int32_t note_count;
    int32_t bpm_count;
    uint64_t revision;
    int32_t can_undo;
    int32_t can_redo;
    char title[128];
    char artist[128];
    char difficulty[64];
} mce_chart_summary;

typedef enum mce_error_code
{
    MCE_ERROR_NONE = 0,
    MCE_ERROR_INVALID_SESSION = 1,
    MCE_ERROR_INVALID_ARGUMENT = 2,
    MCE_ERROR_OUT_OF_RANGE = 3,
    MCE_ERROR_VALIDATION_FAILED = 4,
    MCE_ERROR_NOT_FOUND = 5,
    MCE_ERROR_OPERATION_FAILED = 6
} mce_error_code;

MCE_API mce_session *mce_session_create(void);
MCE_API void mce_session_destroy(mce_session *session);
MCE_API const char *mce_session_last_error(const mce_session *session);
MCE_API int32_t mce_session_last_error_code(const mce_session *session);
MCE_API const char *mce_error_code_name(int32_t code);
MCE_API int32_t mce_session_copy_last_error(const mce_session *session,
                                            char *out_error,
                                            int32_t out_capacity);
MCE_API const char *mce_core_version(void);
MCE_API int32_t mce_ffi_abi_version(void);

MCE_API int32_t mce_session_note_count(const mce_session *session);
MCE_API uint64_t mce_session_chart_revision(const mce_session *session);
MCE_API int32_t mce_session_get_note_snapshot(const mce_session *session,
                                              int32_t index,
                                              mce_note_snapshot *out_note);
MCE_API int32_t mce_session_get_note_snapshots(const mce_session *session,
                                               int32_t start_index,
                                               int32_t max_count,
                                               mce_note_snapshot *out_notes);
MCE_API int32_t mce_session_get_chart_summary(const mce_session *session,
                                              mce_chart_summary *out_summary);

MCE_API int32_t mce_session_add_normal_note(mce_session *session,
                                            const char *id,
                                            mce_beat beat,
                                            int32_t x);
MCE_API int32_t mce_session_add_rain_note(mce_session *session,
                                          const char *id,
                                          mce_beat beat,
                                          mce_beat end_beat,
                                          int32_t x);
MCE_API int32_t mce_session_move_rain_note(mce_session *session,
                                           const char *id,
                                           mce_beat beat,
                                           mce_beat end_beat,
                                           int32_t x);
MCE_API int32_t mce_session_add_sound_note(mce_session *session,
                                           const char *id,
                                           mce_beat beat,
                                           const char *sound,
                                           int32_t volume,
                                           int32_t offset_ms);
MCE_API int32_t mce_session_remove_note_by_id(mce_session *session, const char *id);
MCE_API int32_t mce_session_can_undo(const mce_session *session);
MCE_API int32_t mce_session_can_redo(const mce_session *session);
MCE_API int32_t mce_session_undo(mce_session *session);
MCE_API int32_t mce_session_redo(mce_session *session);

#ifdef __cplusplus
}
#endif
