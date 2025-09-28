/**************************************************************************/
/*  open_telemetry.cpp                                                    */
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

#include "open_telemetry.h"

#include <godot_cpp/variant/char_string.hpp>
#include <godot_cpp/classes/crypto.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/classes/http_client.hpp>
#include <godot_cpp/classes/tls_options.hpp>
#include <vector>
#include <string>

using namespace godot;

OpenTelemetry::OpenTelemetry() {
	hostname = String("https://otel.logflare.app:443");
	flush_interval_ms = 5000;
	batch_size = 10;
	last_flush_time = 0;
}

OpenTelemetry::~OpenTelemetry() {
	// Cleanup (similar to Shutdown but without return value)
	if (conn) {
		conn.reset();
	}
	if (db) {
		db.reset();
	}
}

void OpenTelemetry::_bind_methods() {
	ClassDB::bind_method(D_METHOD("init_tracer_provider", "name", "host", "attributes"), &OpenTelemetry::init_tracer_provider);
	ClassDB::bind_method(D_METHOD("set_headers", "headers"), &OpenTelemetry::set_headers);
	ClassDB::bind_method(D_METHOD("start_span", "name"), &OpenTelemetry::start_span);
	ClassDB::bind_method(D_METHOD("start_span_with_parent", "name", "parent_span_uuid"), &OpenTelemetry::start_span_with_parent);
	ClassDB::bind_method(D_METHOD("add_event", "span_uuid", "event_name"), &OpenTelemetry::add_event);
	ClassDB::bind_method(D_METHOD("set_attributes", "span_uuid", "attributes"), &OpenTelemetry::set_attributes);
	ClassDB::bind_method(D_METHOD("record_error", "span_uuid", "err"), &OpenTelemetry::record_error);
	ClassDB::bind_method(D_METHOD("end_span", "span_uuid"), &OpenTelemetry::end_span);
	ClassDB::bind_method(D_METHOD("set_flush_interval", "interval_ms"), &OpenTelemetry::set_flush_interval);
	ClassDB::bind_method(D_METHOD("set_batch_size", "size"), &OpenTelemetry::set_batch_size);
	ClassDB::bind_method(D_METHOD("record_metric", "name", "value", "unit", "metric_type", "attributes"), &OpenTelemetry::record_metric);
	ClassDB::bind_method(D_METHOD("log_message", "level", "message", "attributes"), &OpenTelemetry::log_message);
	ClassDB::bind_method(D_METHOD("flush_all"), &OpenTelemetry::flush_all);
	ClassDB::bind_method(D_METHOD("shutdown"), &OpenTelemetry::shutdown);
}

String OpenTelemetry::init_tracer_provider(String p_name, String p_host, Dictionary p_attributes) {
	CharString cs = p_name.utf8();
	char *cstr = cs.ptrw();
	CharString c_host = p_host.utf8();
	char *cstr_host = c_host.ptrw();
	String json_attributes = JSON::stringify(p_attributes, "", true, true);
	CharString c_json_attributes = json_attributes.utf8();
	char *cstr_json_attributes = c_json_attributes.ptrw();
	const char *result = InitTracerProvider(cstr, cstr_host, cstr_json_attributes);
	return String(result);
}

String OpenTelemetry::set_headers(Dictionary p_headers) {
	String json_headers = JSON::stringify(p_headers, "", true, true);
	CharString c_json_headers = json_headers.utf8();
	char *cstr_json_headers = c_json_headers.ptrw();
	const char *result = SetHeaders(cstr_json_headers);
	return String(result);
}

String OpenTelemetry::start_span(String p_name) {
	String span_id = generate_uuid_v7();
	return start_span_with_id(p_name, span_id);
}

String OpenTelemetry::start_span_with_parent(String p_name, String p_parent_span_uuid) {
	String span_id = generate_uuid_v7();
	return start_span_with_parent_id(p_name, p_parent_span_uuid, span_id);
}

String OpenTelemetry::generate_uuid_v7() {
	Ref<Crypto> crypto;
	crypto.instantiate();
	PackedByteArray random_bytes = crypto->generate_random_bytes(8); // Generate 64 random bits

	// Combine random bytes into uint64_t
	uint64_t random_full = 0;
	for (int i = 0; i < 8; ++i) {
		random_full |= ((uint64_t)(random_bytes[i] & 0xFF)) << (i * 8);
	}

	// Extract the random components
	uint16_t rand_a = random_full & 0xFFF; // Lower 12 bits
	uint64_t rand_b = (random_full >> 12) & 0xFFFFFFFFFFFFULL; // Next 48 bits

	// Get current timestamp in milliseconds
	auto now = std::chrono::system_clock::now();
	uint64_t unix_ts_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

	// Build UUID v7 components
	uint32_t time_high = unix_ts_ms >> 16;
	uint16_t time_mid = unix_ts_ms & 0xFFFF;
	uint16_t time_low_ver = ((unix_ts_ms & 0xFFF) << 4) | 0x7; // Set version to 7
	uint16_t clock_seq = (2 << 14) | rand_a; // Set variant to RFC 4122 (2)
	uint64_t node = rand_b;

	// Format as 36-character UUID string
	char buffer[37];
	snprintf(buffer, sizeof(buffer), "%08x-%04x-%04x-%04x-%012lx",
			 time_high, time_mid, time_low_ver, clock_seq, (unsigned long)node);

	return String(buffer);
}

String OpenTelemetry::start_span_with_id(String p_name, String p_span_id) {
	CharString c_name = p_name.utf8();
	char* cstr_name = c_name.ptrw();
	CharString c_span_id = p_span_id.utf8();
	char* cstr_span_id = c_span_id.ptrw();
	char* result = StartSpanWithId(cstr_name, cstr_span_id);
	return String(result);
}

String OpenTelemetry::start_span_with_parent_id(String p_name, String p_parent_span_uuid, String p_span_id) {
	CharString c_name = p_name.utf8();
	char* cstr_name = c_name.ptrw();
	CharString c_parent_id = p_parent_span_uuid.utf8();
	char* cstr_parent_id = c_parent_id.ptrw();
	CharString c_span_id = p_span_id.utf8();
	char* cstr_span_id = c_span_id.ptrw();
	char* result = StartSpanWithParentWithId(cstr_name, cstr_parent_id, cstr_span_id);
	return String(result);
}

void OpenTelemetry::add_event(String p_span_uuid, String p_event_name) {
	CharString c_event_id = p_span_uuid.utf8();
	char *cstr_event_id = c_event_id.ptrw();
	CharString c_event_name = p_event_name.utf8();
	char *cstr_event_name = c_event_name.ptrw();
	AddEvent(cstr_event_id, c_event_name);
}

void OpenTelemetry::set_attributes(String p_span_uuid, Dictionary p_attributes) {
	CharString c_attribute_id = p_span_uuid.utf8();
	char *cstr_attribute_id = c_attribute_id.ptrw();
	String json_attributes = JSON::stringify(p_attributes, "", true, true);
	CharString c_json_attributes = json_attributes.utf8();
	char *cstr_json_attributes = c_json_attributes.ptrw();
	SetAttributes(cstr_attribute_id, cstr_json_attributes);
}

void OpenTelemetry::record_error(String p_span_uuid, String p_error) {
	CharString c_error_id = p_span_uuid.utf8();
	char *cstr_error_id = c_error_id.ptrw();
	CharString c_error = p_error.utf8();
	char *cstr_error = c_error.ptrw();
	RecordError(cstr_error_id, c_error);
}

void OpenTelemetry::end_span(String p_span_uuid) {
	CharString c_span_id = p_span_uuid.utf8();
	char *cstr_span_id = c_span_id.ptrw();
	EndSpan(cstr_span_id);
}

void OpenTelemetry::set_flush_interval(int p_interval_ms) {
	SetFlushInterval(p_interval_ms);
}

void OpenTelemetry::set_batch_size(int p_size) {
	SetBatchSize(p_size);
}

void OpenTelemetry::record_metric(String p_name, float p_value, String p_unit, int p_metric_type, Dictionary p_attributes) {
	CharString c_name = p_name.utf8();
	char *cstr_name = c_name.ptrw();
	String json_attributes = JSON::stringify(p_attributes, "", true, true);
	CharString c_json_attributes = json_attributes.utf8();
	char *cstr_json_attributes = c_json_attributes.ptrw();
	CharString c_unit = p_unit.utf8();
	char *cstr_unit = c_unit.ptrw();
	RecordMetric(cstr_name, (double)p_value, cstr_unit, p_metric_type, cstr_json_attributes);
}

void OpenTelemetry::log_message(String p_level, String p_message, Dictionary p_attributes) {
	CharString c_level = p_level.utf8();
	char *cstr_level = c_level.ptrw();
	CharString c_message = p_message.utf8();
	char *cstr_message = c_message.ptrw();
	String json_attributes = JSON::stringify(p_attributes, "", true, true);
	CharString c_json_attributes = json_attributes.utf8();
	char *cstr_json_attributes = c_json_attributes.ptrw();
	LogMessage(cstr_level, cstr_message, cstr_json_attributes);
}

void OpenTelemetry::flush_all() {
	FlushAllBufferedData();
}

String OpenTelemetry::shutdown() {
	char *shutdown_result = Shutdown();
	return String(shutdown_result);
}

// Internal implementation methods (moved from wrapper)
char* OpenTelemetry::InitTracerProvider(const char* name, const char* host, const char* json_attributes) {
	hostname = String(host);
	tracer_name = String(name);
	JSON json;
	resource_attributes = json.parse_string(String(json_attributes));

	// Initialize DuckDB database
	db = std::make_unique<duckdb::DuckDB>(nullptr);
	conn = std::make_unique<duckdb::Connection>(*db);

	// Create tables for spans, metrics, and logs
	auto& conn_ref = *conn;
	conn_ref.Query("CREATE TABLE spans ("
				   "name VARCHAR, "
				   "span_id VARCHAR, "
				   "trace_id VARCHAR, "
				   "parent_span_id VARCHAR, "
				   "start_time_unix_nano BIGINT, "
				   "end_time_unix_nano BIGINT, "
				   "status INTEGER, "
				   "kind INTEGER, "
				   "attributes VARCHAR, "
				   "events VARCHAR)");

	conn_ref.Query("CREATE TABLE metrics ("
				   "name VARCHAR, "
				   "value DOUBLE, "
				   "unit VARCHAR, "
				   "type INTEGER, "
				   "timestamp BIGINT, "
				   "attributes VARCHAR)");

	conn_ref.Query("CREATE TABLE logs ("
				   "level VARCHAR, "
				   "message VARCHAR, "
				   "timestamp BIGINT, "
				   "attributes VARCHAR)");

	// Generate a random trace_id, 64-bit hex
	Ref<Crypto> crypto;
	crypto.instantiate();
	PackedByteArray random_bytes = crypto->generate_random_bytes(8); // 64 bits for trace_id
	String trace_id_hex = "";
	for (int i = 0; i < 8; i++) {
		uint64_t byte = random_bytes[i];
		char byte_str[3];
		snprintf(byte_str, sizeof(byte_str), "%02x", static_cast<uint8_t>(random_bytes[i]));
		trace_id_hex += byte_str;
	}
	trace_id = trace_id_hex;

	last_flush_time = Time::get_singleton()->get_ticks_msec();

	return strdup("OK");
}

char* OpenTelemetry::SetHeaders(const char* json_headers) {
	JSON json;
	headers = json.parse_string(String(json_headers));

	return strdup("OK");
}

char* OpenTelemetry::StartSpanWithId(const char* name, const char* span_id) {
	uint64_t start_time = (uint64_t)(Time::get_singleton()->get_unix_time_from_system() * 1000000000ULL);

	Dictionary span;
	span["name"] = String(name);
	span["span_id"] = String(span_id);
	span["trace_id"] = trace_id;
	span["parent_span_id"] = String("");
	span["start_time_unix_nano"] = start_time;
	span["status"] = 0; // UNSET
	span["attributes"] = Dictionary();
	span["events"] = Array();
	span["kind"] = 1; // INTERNAL

	active_spans[String(span_id)] = span;

	return strdup(span_id);
}

char* OpenTelemetry::StartSpanWithParentWithId(const char* name, const char* parent_span_uuid, const char* span_id) {
	uint64_t start_time = (uint64_t)(Time::get_singleton()->get_unix_time_from_system() * 1000000000ULL);

	Dictionary span;
	span["name"] = String(name);
	span["span_id"] = String(span_id);
	span["trace_id"] = trace_id;
	span["parent_span_id"] = String(parent_span_uuid);
	span["start_time_unix_nano"] = start_time;
	span["status"] = 0;
	span["attributes"] = Dictionary();
	span["events"] = Array();
	span["kind"] = 1;

	active_spans[String(span_id)] = span;

	return strdup(span_id);
}

void OpenTelemetry::AddEvent(const char* span_uuid, const char* event_name) {
	String span_id_str = String(span_uuid);
	if (active_spans.has(span_id_str)) {
		Dictionary span = active_spans[span_id_str];
		Array events = span["events"];
		Dictionary event;
		event["name"] = String(event_name);
		event["time_unix_nano"] = (uint64_t)(Time::get_singleton()->get_unix_time_from_system() * 1000000000ULL);
		event["attributes"] = Dictionary();
		events.push_back(event);
		span["events"] = events;
		active_spans[span_id_str] = span;
	}
}

void OpenTelemetry::SetAttributes(const char* span_uuid, const char* json_attributes) {
	String span_id_str = String(span_uuid);
	if (active_spans.has(span_id_str)) {
		JSON json;
		Dictionary attrs = json.parse_string(String(json_attributes));
		Dictionary span = active_spans[span_id_str];
		Dictionary existing_attrs = span["attributes"];
		for (const Variant &key : attrs.keys()) {
			existing_attrs[key] = attrs[key];
		}
		span["attributes"] = existing_attrs;
		active_spans[span_id_str] = span;
	}
}

void OpenTelemetry::RecordError(const char* span_uuid, const char* error) {
	String span_id_str = String(span_uuid);
	if (active_spans.has(span_id_str)) {
		Dictionary span = active_spans[span_id_str];

		Dictionary event;
		event["name"] = "error";
		event["time_unix_nano"] = (uint64_t)(Time::get_singleton()->get_unix_time_from_system() * 1000000000ULL);
		Dictionary event_attrs;
		event_attrs["error"] = String(error);
		event["attributes"] = event_attrs;

		Array events = span["events"];
		events.push_back(event);
		span["events"] = events;

		span["status"] = 2; // ERROR

		active_spans[span_id_str] = span;
	}
}

void OpenTelemetry::EndSpan(const char* span_uuid) {
	String span_id_str = String(span_uuid);
	if (active_spans.has(span_id_str)) {
		Dictionary span = active_spans[span_id_str];
		uint64_t end_time = (uint64_t)(Time::get_singleton()->get_unix_time_from_system() * 1000000000ULL);
		span["end_time_unix_nano"] = end_time;

		// Insert span into DuckDB
		std::lock_guard<std::mutex> lock(db_mutex);
		JSON json;
		String attributes_json = json.stringify(span["attributes"]);
		String events_json = json.stringify(span["events"]);

		std::string query = "INSERT INTO spans VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
		auto prepared = conn->Prepare(query);
		prepared->Execute(
			std::string(span["name"].operator String().utf8().get_data()),
			std::string(span["span_id"].operator String().utf8().get_data()),
			std::string(span["trace_id"].operator String().utf8().get_data()),
			std::string(span["parent_span_id"].operator String().utf8().get_data()),
			span["start_time_unix_nano"].operator uint64_t(),
			span["end_time_unix_nano"].operator uint64_t(),
			span["status"].operator int(),
			span["kind"].operator int(),
			std::string(attributes_json.utf8().get_data()),
			std::string(events_json.utf8().get_data())
		);

		active_spans.erase(span_id_str);

		// Check if we should flush based on batch size
		CheckAndFlush();
	}
}

void OpenTelemetry::SetFlushInterval(int interval_ms) {
	flush_interval_ms = interval_ms;
}

void OpenTelemetry::SetBatchSize(int size) {
	batch_size = size;
}

void OpenTelemetry::RecordMetric(const char* name, double value, const char* unit, int metric_type, const char* json_attributes) {
	uint64_t timestamp = (uint64_t)(Time::get_singleton()->get_unix_time_from_system() * 1000000000ULL);

	// Insert metric into DuckDB
	std::lock_guard<std::mutex> lock(db_mutex);
	std::string attributes_json = "{}";
	if (json_attributes && strlen(json_attributes) > 0) {
		attributes_json = json_attributes;
	}

	std::string query = "INSERT INTO metrics VALUES (?, ?, ?, ?, ?, ?)";
	auto prepared = conn->Prepare(query);
	prepared->Execute(
		std::string(name),
		value,
		std::string(unit),
		metric_type,
		timestamp,
		attributes_json
	);

	// Check if we should flush based on batch size
	CheckAndFlush();
}

void OpenTelemetry::LogMessage(const char* level, const char* message, const char* json_attributes) {
	uint64_t timestamp = (uint64_t)(Time::get_singleton()->get_unix_time_from_system() * 1000000000ULL);

	// Insert log into DuckDB
	std::lock_guard<std::mutex> lock(db_mutex);
	std::string attributes_json = "{}";
	if (json_attributes && strlen(json_attributes) > 0) {
		attributes_json = json_attributes;
	}

	std::string query = "INSERT INTO logs VALUES (?, ?, ?, ?)";
	auto prepared = conn->Prepare(query);
	prepared->Execute(
		std::string(level),
		std::string(message),
		timestamp,
		attributes_json
	);

	// Check if we should flush based on batch size
	CheckAndFlush();
}

void OpenTelemetry::CheckAndFlush() {
	uint64_t current_time = Time::get_singleton()->get_ticks_msec();
	bool should_flush_time = (current_time - last_flush_time) >= (uint64_t)flush_interval_ms;

	bool should_flush_batch = false;
	{
		std::lock_guard<std::mutex> lock(db_mutex);
		auto spans_result = conn->Query("SELECT COUNT(*) FROM spans");
		auto metrics_result = conn->Query("SELECT COUNT(*) FROM metrics");
		auto logs_result = conn->Query("SELECT COUNT(*) FROM logs");

		int64_t spans_count = spans_result->GetValue(0, 0).GetValue<int64_t>();
		int64_t metrics_count = metrics_result->GetValue(0, 0).GetValue<int64_t>();
		int64_t logs_count = logs_result->GetValue(0, 0).GetValue<int64_t>();

		should_flush_batch = spans_count >= batch_size ||
						   metrics_count >= batch_size ||
						   logs_count >= batch_size;
	}

	if (should_flush_time || should_flush_batch) {
		FlushAllBufferedData();
	}
}

void OpenTelemetry::FlushAllBufferedData() {
	std::lock_guard<std::mutex> lock(db_mutex);

	Ref<HTTPClient> http;
	http.instantiate();
	Ref<TLSOptions> tls;
	tls.instantiate();
	http->connect_to_host("otel.logflare.app", 443, tls);
	PackedStringArray headers_array;
	headers_array.push_back("Content-Type: application/json");

	// Add custom headers
	for (const Variant &key : headers.keys()) {
		headers_array.push_back(String(key) + ": " + String(headers[key]));
	}

	uint64_t current_time = Time::get_singleton()->get_ticks_msec();

	// Flush traces
	{
		auto spans_result = conn->Query("SELECT * FROM spans");
		if (spans_result->RowCount() > 0) {
			Dictionary root;
			Array resourceSpans;
			Dictionary resourceSpan;
			resourceSpan["resource"] = resource_attributes;

			Array scopeSpans;
			Dictionary scopeSpan;
			Dictionary scope_dict;
			scope_dict["name"] = tracer_name;
			scope_dict["version"] = "1.0.0";
			scopeSpan["scope"] = scope_dict;

			Array spansArray;
			for (size_t i = 0; i < spans_result->RowCount(); i++) {
				Dictionary span;
				span["name"] = String(spans_result->GetValue(0, i).GetValue<std::string>().c_str());
				span["span_id"] = String(spans_result->GetValue(1, i).GetValue<std::string>().c_str());
				span["trace_id"] = String(spans_result->GetValue(2, i).GetValue<std::string>().c_str());
				span["parent_span_id"] = String(spans_result->GetValue(3, i).GetValue<std::string>().c_str());
				span["start_time_unix_nano"] = spans_result->GetValue(4, i).GetValue<uint64_t>();
				span["end_time_unix_nano"] = spans_result->GetValue(5, i).GetValue<uint64_t>();
				span["status"] = spans_result->GetValue(6, i).GetValue<int32_t>();
				span["kind"] = spans_result->GetValue(7, i).GetValue<int32_t>();

				// Parse JSON strings back to Dictionaries
				JSON json;
				String attributes_json = String(spans_result->GetValue(8, i).GetValue<std::string>().c_str());
				span["attributes"] = json.parse_string(attributes_json);

				String events_json = String(spans_result->GetValue(9, i).GetValue<std::string>().c_str());
				span["events"] = json.parse_string(events_json);

				spansArray.push_back(span);
			}
			scopeSpan["spans"] = spansArray;

			scopeSpans.push_back(scopeSpan);
			resourceSpan["scopeSpans"] = scopeSpans;

			resourceSpans.push_back(resourceSpan);
			root["resourceSpans"] = resourceSpans;

			JSON json;
			String jsonPayload = json.stringify(root);
			http->request(HTTPClient::Method::METHOD_POST, "/v1/traces", headers_array, jsonPayload);

			// Clear spans table
			conn->Query("DELETE FROM spans");
		}
	}

	// Flush metrics
	{
		auto metrics_result = conn->Query("SELECT * FROM metrics");
		if (metrics_result->RowCount() > 0) {
			Dictionary root;
			Array resourceMetrics;
			Dictionary resourceMetric;
			resourceMetric["resource"] = resource_attributes;

			Array scopeMetrics;
			Dictionary scopeMetric;
			Dictionary scope_dict;
			scope_dict["name"] = tracer_name;
			scope_dict["version"] = "1.0.0";
			scopeMetric["scope"] = scope_dict;

			Array metricsArray;
			for (size_t i = 0; i < metrics_result->RowCount(); i++) {
				Dictionary metric;
				metric["name"] = String(metrics_result->GetValue(0, i).GetValue<std::string>().c_str());
				metric["value"] = metrics_result->GetValue(1, i).GetValue<double>();
				metric["unit"] = String(metrics_result->GetValue(2, i).GetValue<std::string>().c_str());
				metric["type"] = metrics_result->GetValue(3, i).GetValue<int32_t>();
				metric["timestamp"] = metrics_result->GetValue(4, i).GetValue<uint64_t>();

				JSON json;
				String attributes_json = String(metrics_result->GetValue(5, i).GetValue<std::string>().c_str());
				metric["attributes"] = json.parse_string(attributes_json);

				metricsArray.push_back(metric);
			}
			scopeMetric["metrics"] = metricsArray;

			scopeMetrics.push_back(scopeMetric);
			resourceMetric["scopeMetrics"] = scopeMetrics;

			resourceMetrics.push_back(resourceMetric);
			root["resourceMetrics"] = resourceMetrics;

			JSON json;
			String jsonPayload = json.stringify(root);
			http->request(HTTPClient::Method::METHOD_POST, "/v1/metrics", headers_array, jsonPayload);

			// Clear metrics table
			conn->Query("DELETE FROM metrics");
		}
	}

	// Flush logs
	{
		auto logs_result = conn->Query("SELECT * FROM logs");
		if (logs_result->RowCount() > 0) {
			Dictionary root;
			Array resourceLogs;
			Dictionary resourceLog;
			resourceLog["resource"] = resource_attributes;

			Array scopeLogs;
			Dictionary scopeLog;
			Dictionary scope_dict;
			scope_dict["name"] = tracer_name;
			scope_dict["version"] = "1.0.0";
			scopeLog["scope"] = scope_dict;

			Array logRecordsArray;
			for (size_t i = 0; i < logs_result->RowCount(); i++) {
				Dictionary log_record;
				log_record["level"] = String(logs_result->GetValue(0, i).GetValue<std::string>().c_str());
				log_record["message"] = String(logs_result->GetValue(1, i).GetValue<std::string>().c_str());
				log_record["timestamp"] = logs_result->GetValue(2, i).GetValue<uint64_t>();

				JSON json;
				String attributes_json = String(logs_result->GetValue(3, i).GetValue<std::string>().c_str());
				log_record["attributes"] = json.parse_string(attributes_json);

				logRecordsArray.push_back(log_record);
			}
			scopeLog["logRecords"] = logRecordsArray;

			scopeLogs.push_back(scopeLog);
			resourceLog["scopeLogs"] = scopeLogs;

			resourceLogs.push_back(resourceLog);
			root["resourceLogs"] = resourceLogs;

			JSON json;
			String jsonPayload = json.stringify(root);
			http->request(HTTPClient::Method::METHOD_POST, "/v1/logs", headers_array, jsonPayload);

			// Clear logs table
			conn->Query("DELETE FROM logs");
		}
	}

	last_flush_time = current_time;
}

char* OpenTelemetry::Shutdown() {
	FlushAllBufferedData(); // Flush any remaining buffered data
	active_spans.clear();
	conn.reset();
	db.reset();
	return strdup("OK");
}
