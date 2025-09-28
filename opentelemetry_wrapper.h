#ifndef OPENTELEMETRY_WRAPPER_H
#define OPENTELEMETRY_WRAPPER_H

#include <string>

#ifdef __cplusplus
extern "C" {
#endif

char* InitTracerProvider(const char* name, const char* host, const char* json_attributes);
char* StartSpanWithId(const char* name, const char* span_id);
char* StartSpanWithParentWithId(const char* name, const char* parent_span_uuid, const char* span_id);
void AddEvent(const char* span_uuid, const char* event_name);
void SetAttributes(const char* span_uuid, const char* json_attributes);
void RecordError(const char* span_uuid, const char* error);
void EndSpan(const char* span_uuid);
char* Shutdown();

#ifdef __cplusplus
}
#endif

#endif // OPENTELEMETRY_WRAPPER_H
