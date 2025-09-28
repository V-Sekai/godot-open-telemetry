#include "opentelemetry_wrapper.h"

#include <opentelemetry/trace/provider.h>
#include <opentelemetry/exporters/otlp/otlp_grpc_exporter.h>
#include <opentelemetry/sdk/trace/tracer_provider.h>
#include <opentelemetry/sdk/trace/simple_processor.h>
#include <opentelemetry/trace/scope.h>
#include <opentelemetry/trace/span.h>
#include <opentelemetry/common/attribute_value.h>
#include <opentelemetry/sdk/resource/resource.h>

#include <nlohmann/json.hpp>

#include <map>
#include <memory>
#include <string>
#include <uuid/uuid.h> // or use boost or something, but for simplicity, use strings

// For UUID, since C++ doesn't have built-in, use a simple counter or something.
// The original uses UUID, but for simplicity, use incremental IDs.

static std::map<std::string, std::shared_ptr<opentelemetry::trace::Span>> spans;
static std::shared_ptr<opentelemetry::trace::TracerProvider> tracer_provider;
static std::shared_ptr<opentelemetry::trace::Tracer> tracer;
static int span_id_counter = 0;

std::string generate_span_id() {
    return std::to_string(++span_id_counter);
}

opentelemetry::common::AttributeMap parse_attributes(const std::string& json_str) {
    opentelemetry::common::AttributeMap attributes;
    if (json_str.empty()) return attributes;

    try {
        nlohmann::json j = nlohmann::json::parse(json_str);
        for (auto& [key, value] : j.items()) {
            if (value.is_string()) {
                attributes[key] = opentelemetry::common::AttributeValue(value.get<std::string>());
            } else if (value.is_number_integer()) {
                attributes[key] = opentelemetry::common::AttributeValue(value.get<int64_t>());
            } else if (value.is_number_float()) {
                attributes[key] = opentelemetry::common::AttributeValue(value.get<double>());
            } else if (value.is_boolean()) {
                attributes[key] = opentelemetry::common::AttributeValue(value.get<bool>());
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
        tracer_provider = std::make_shared<opentelemetry::sdk::trace::TracerProvider>(std::move(processor), resource);

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
        it->second->SetAttributes(attributes);
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
        tracer_provider->Shutdown();
        tracer_provider.reset();
        tracer.reset();
        spans.clear();
    }
    return strdup("OK");
}