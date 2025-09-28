#include "opentelemetry_wrapper.h"

#include <opentelemetry/trace/provider.h>
#include <opentelemetry/exporters/otlp/otlp_grpc_exporter.h>
#include <opentelemetry/sdk/trace/tracer_provider.h>
#include <opentelemetry/sdk/trace/simple_processor.h>
#include <opentelemetry/trace/scope.h>
#include <opentelemetry/trace/span.h>
#include <opentelemetry/sdk/common/attribute_utils.h>

#include <nlohmann/json.hpp>

#include <map>
#include <memory>
#include <string>
#include <chrono>
#include <random>
#include <sstream>
#include <iomanip>

// UUID v7 compliant span ID generation
static std::map<std::string, opentelemetry::nostd::shared_ptr<opentelemetry::trace::Span>> spans;
static opentelemetry::nostd::shared_ptr<opentelemetry::sdk::trace::TracerProvider> tracer_provider;
static opentelemetry::nostd::shared_ptr<opentelemetry::trace::Tracer> tracer;

// Generate UUID v7 compliant ID
std::string generate_uuid_v7() {
    // Get Unix timestamp in milliseconds (48 bits used)
    auto now = std::chrono::system_clock::now();
    uint64_t unix_ts_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

    // Random generators
    static std::random_device rd;
    static std::mt19937_64 gen(rd());
    static std::uniform_int_distribution<uint16_t> dis12(0, 0xFFF); // 12 bits
    static std::uniform_int_distribution<uint64_t> dis48(0, 0xFFFFFFFFFFFFULL); // 48 bits

    // rand_a: 12 bits
    uint16_t rand_a = dis12(gen);

    // rand_b: lowest 48 bits of 62 bits (we use 48 for node)
    uint64_t rand_b = dis48(gen);

    // Build UUID components
    uint32_t time_high = unix_ts_ms >> 16;  // First 32 bits of timestamp
    uint16_t time_mid = unix_ts_ms & 0xFFFF;  // Next 16 bits
    uint16_t time_low_ver = ((unix_ts_ms & 0xFFF) << 4) | 0x7;  // Last 12 bits << 4 | version (4 bits = 0111)
    uint16_t clock_seq = (2 << 14) | rand_a;  // Var (2 bits = 10) << 14 | rand_a (12 bits)
    uint64_t node = rand_b;  // 48 bits

    // Format as UUID string
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    ss << std::setw(8) << time_high << '-';
    ss << std::setw(4) << time_mid << '-';
    ss << std::setw(4) << time_low_ver << '-';
    ss << std::setw(4) << clock_seq << '-';
    ss << std::setw(12) << node;

    return ss.str();
}

std::string generate_span_id() {
    return generate_uuid_v7();
}

opentelemetry::sdk::common::AttributeMap parse_attributes(const std::string& json_str) {
    opentelemetry::sdk::common::AttributeMap attributes;
    if (json_str.empty()) return attributes;

    try {
        nlohmann::json j = nlohmann::json::parse(json_str);
        for (auto& [key, value] : j.items()) {
            if (value.is_string()) {
                attributes.SetAttribute(key, value.get<std::string>());
            } else if (value.is_number_integer()) {
                attributes.SetAttribute(key, value.get<int64_t>());
            } else if (value.is_number_float()) {
                attributes.SetAttribute(key, value.get<double>());
            } else if (value.is_boolean()) {
                attributes.SetAttribute(key, value.get<bool>());
            }
            // Add more types if needed
        }
    } catch (...) {
        // Ignore parse errors
    }
    return attributes;
}

char* InitTracerProvider(const char* name, const char* host, const char* json_attributes) {
    try {
        // Parse attributes
        auto resource_attributes = parse_attributes(json_attributes);

        // Create resource
        auto resource = opentelemetry::sdk::resource::Resource::Create(resource_attributes);

        // Create OTLP exporter
        opentelemetry::exporter::otlp::OtlpGrpcExporterOptions opts;
        opts.endpoint = host;
        auto exporter = std::make_unique<opentelemetry::exporter::otlp::OtlpGrpcExporter>(opts);

        // Create processor
        auto processor = std::make_unique<opentelemetry::sdk::trace::SimpleSpanProcessor>(std::move(exporter));

        // Create provider
        tracer_provider = opentelemetry::nostd::shared_ptr<opentelemetry::sdk::trace::TracerProvider>(
            new opentelemetry::sdk::trace::TracerProvider(std::move(processor), resource)
        );

        // Set global provider
        opentelemetry::trace::Provider::SetTracerProvider(tracer_provider);

        // Get tracer
        tracer = tracer_provider->GetTracer(name);

        return strdup("OK");
    } catch (const std::exception& e) {
        return strdup(e.what());
    }
}

char* StartSpan(const char* name) {
    if (!tracer) return strdup("Tracer not initialized");

    auto span = tracer->StartSpan(name);
    std::string span_id = generate_span_id();
    spans[span_id] = span;
    return strdup(span_id.c_str());
}

char* StartSpanWithParent(const char* name, const char* parent_span_uuid) {
    if (!tracer) return strdup("Tracer not initialized");

    auto it = spans.find(parent_span_uuid);
    if (it == spans.end()) return strdup("Parent span not found");

    opentelemetry::trace::StartSpanOptions options;
    options.parent = it->second->GetContext();

    auto span = tracer->StartSpan(name, options);
    std::string span_id = generate_span_id();
    spans[span_id] = span;
    return strdup(span_id.c_str());
}

void AddEvent(const char* span_uuid, const char* event_name) {
    auto it = spans.find(span_uuid);
    if (it != spans.end()) {
        it->second->AddEvent(event_name);
    }
}

void SetAttributes(const char* span_uuid, const char* json_attributes) {
    auto it = spans.find(span_uuid);
    if (it != spans.end()) {
        auto attributes = parse_attributes(json_attributes);
        for (auto& attr : attributes) {
            it->second->SetAttribute(attr.first, attr.second);
        }
    }
}

void RecordError(const char* span_uuid, const char* error) {
    auto it = spans.find(span_uuid);
    if (it != spans.end()) {
        it->second->SetStatus(opentelemetry::trace::StatusCode::kError, error);
        it->second->AddEvent("error", {{"error", error}});
    }
}

void EndSpan(const char* span_uuid) {
    auto it = spans.find(span_uuid);
    if (it != spans.end()) {
        it->second->End();
        spans.erase(it);
    }
}

char* Shutdown() {
    if (tracer_provider) {
        tracer_provider->Shutdown(std::chrono::microseconds(5000));
        tracer_provider.reset();
        tracer.reset();
        spans.clear();
    }
    return strdup("OK");
}
