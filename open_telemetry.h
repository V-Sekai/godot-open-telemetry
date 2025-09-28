/**************************************************************************/
/*  open_telemetry.h                                                      */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#ifndef OPEN_TELEMETRY_H
#define OPEN_TELEMETRY_H

#include <godot_cpp/classes/crypto.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/method_bind.hpp>
#include <godot_cpp/templates/cowdata.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/classes/http_client.hpp>
#include <godot_cpp/classes/tls_options.hpp>
#include <mutex>
#include <memory>
#include "duckdb.hpp"

namespace godot {

class OpenTelemetry : public RefCounted {
	GDCLASS(OpenTelemetry, RefCounted);

private:
	// Global state (moved from wrapper)
	String hostname;
	Dictionary resource_attributes;
	Dictionary headers;
	Dictionary active_spans;
	String trace_id;
	String tracer_name;
	int flush_interval_ms;
	int batch_size;
	uint64_t last_flush_time;
	std::unique_ptr<duckdb::DuckDB> db;
	std::unique_ptr<duckdb::Connection> conn;
	std::mutex db_mutex;

protected:
	static void _bind_methods();

public:
	OpenTelemetry();
	~OpenTelemetry();

	String init_tracer_provider(String p_name, String p_host, Dictionary p_attributes);
	String set_headers(Dictionary p_headers);
	String start_span(String p_name);
	String start_span_with_parent(String p_name, String p_parent_span_uuid);
	String generate_uuid_v7();
	void add_event(String p_span_uuid, String p_event_name);
	void set_attributes(String p_span_uuid, Dictionary p_attributes);
	void record_error(String p_span_uuid, String p_error);
	void end_span(String p_span_uuid);
	void set_flush_interval(int p_interval_ms);
	void set_batch_size(int p_size);
	void record_metric(String p_name, float p_value, String p_unit, int p_metric_type, Dictionary p_attributes);
	void log_message(String p_level, String p_message, Dictionary p_attributes);
	void flush_all();
	String shutdown();

private:
	String start_span_with_id(String p_name, String p_span_id);
	String start_span_with_parent_id(String p_name, String p_parent_span_uuid, String p_span_id);

	// Internal implementation methods (moved from wrapper)
	char* InitTracerProvider(const char* name, const char* host, const char* json_attributes);
	char* SetHeaders(const char* json_headers);
	char* StartSpanWithId(const char* name, const char* span_id);
	char* StartSpanWithParentWithId(const char* name, const char* parent_span_uuid, const char* span_id);
	void AddEvent(const char* span_uuid, const char* event_name);
	void SetAttributes(const char* span_uuid, const char* json_attributes);
	void RecordError(const char* span_uuid, const char* error);
	void EndSpan(const char* span_uuid);
	void SetFlushInterval(int interval_ms);
	void SetBatchSize(int size);
	void RecordMetric(const char* name, double value, const char* unit, int metric_type, const char* json_attributes);
	void LogMessage(const char* level, const char* message, const char* json_attributes);
	void CheckAndFlush();
	void FlushAllBufferedData();
	char* Shutdown();
};

}

#endif // OPEN_TELEMETRY_H
