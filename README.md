# Godot Engine Open Telemetry

This module provides OpenTelemetry tracing capabilities for the Godot game engine. It allows developers to instrument their Godot applications with distributed tracing, enabling better observability and debugging of game logic and performance.

## API Reference

### Initialization

#### `init_tracer_provider(name: String, host: String, attributes: Dictionary) -> String`

Initializes the OpenTelemetry tracer provider.

**Parameters:**
- `name`: A string identifier for this tracer provider
- `host`: The OTLP exporter endpoint (e.g., "localhost:4317")
- `attributes`: Resource attributes as a Dictionary (e.g., version info)

**Returns:** Empty string on success, error message on failure

### Span Management

#### `start_span(name: String) -> String`

Starts a new root span.

**Parameters:**
- `name`: Name of the span

**Returns:** A unique span UUID string

#### `start_span_with_parent(name: String, parent_span_uuid: String) -> String`

Starts a new child span with the specified parent.

**Parameters:**
- `name`: Name of the span
- `parent_span_uuid`: UUID of the parent span

**Returns:** A unique span UUID string

#### `end_span(span_uuid: String) -> void`

Ends the specified span.

**Parameters:**
- `span_uuid`: UUID of the span to end

#### `shutdown() -> String`

Shuts down the OpenTelemetry tracer provider and exports any pending spans.

**Returns:** Empty string on success, error message on failure

### Span Operations

#### `add_event(span_uuid: String, event_name: String) -> void`

Adds a named event to the span.

**Parameters:**
- `span_uuid`: UUID of the span
- `event_name`: Name of the event

#### `set_attributes(span_uuid: String, attributes: Dictionary) -> void`

Sets attributes on the span.

**Parameters:**
- `span_uuid`: UUID of the span
- `attributes`: Dictionary of key-value attribute pairs

#### `record_error(span_uuid: String, error: String) -> void`

Records an error event on the span.

**Parameters:**
- `span_uuid`: UUID of the span
- `error`: Error description or stack trace

### Utilities

#### `generate_uuid_v7() -> String`

Generates a UUID v7 for use as custom span IDs.

**Returns:** A UUID v7 string

## Example Usage

```gdscript
extends Node3D

var otel: OpenTelemetry = Opentelemetry.new()

func _ready() -> void:
	# Initialize tracer provider with resource attributes
	var error = otel.init_tracer_provider("godot", "localhost:4317", Engine.get_version_info())
	if error:
		print("Failed to initialize OpenTelemetry: ", error)

func _process(_delta) -> void:
	# Start a root span for game frame processing
	var frame_span = otel.start_span("game-frame")

	# Add custom attributes
	otel.set_attributes(frame_span, {
		"fps": Engine.get_frames_per_second(),
		"delta": _delta,
		"time": Time.get_time_string_from_system()
	})

	# Child span for physics update
	var physics_span = otel.start_span_with_parent("physics-update", frame_span)
	otel.add_event(physics_span, "physics-step-started")

	# Simulate physics work
	# ... physics calculations ...

	otel.add_event(physics_span, "physics-step-completed")
	otel.end_span(physics_span)

	# Record an error if something went wrong
	if _delta > 1.0:  # Very low FPS
		otel.record_error(frame_span, "Very low frame rate detected: " + str(_delta))

	otel.end_span(frame_span)

func _exit_tree() -> void:
	# Properly shutdown to ensure all spans are exported
	var error = otel.shutdown()
	if error:
		print("Failed to shutdown OpenTelemetry: ", error)
```

## Development

This project uses OpenTelemetry C++ for high-performance tracing. The thirdparty OpenTelemetry libraries are built automatically when compiling the Godot module.

### Dev Container

A dev container is provided for complete development environment with Godot 4.5:

1. Open in VS Code
2. When prompted, click "Reopen in Container" or use Command Palette: `Dev Containers: Reopen in Container`
3. The container will automatically:
   - Build the OpenTelemetry C++ libraries
   - Clone and build Godot 4.5 with the module enabled
   - Set up the complete development environment

The dev container includes:
- CMake and build tools
- OpenTelemetry dependencies (gRPC, protobuf)
- Godot 4.5 with the module integrated
- VS Code extensions for C++, Godot, and Python development

After setup, you can:
- Build the module: `cd /home/vscode/godot && scons platform=linuxbsd target=editor -j$(nproc) module_opentelemetry_enabled=yes`
- Run Godot: `/home/vscode/godot/bin/godot.linuxbsd.editor.x86_64`
- Test the module in a Godot project

Environment variables available:
- `GODOT_PATH`: Path to Godot source (`/home/vscode/godot`)
- `GODOT_BINARY`: Path to built Godot binary

### Building

To build Godot with this module:

1. Clone Godot engine
2. Copy this module to `godot/modules/opentelemetry/`
3. Build Godot with the module enabled

The SCsub file will automatically build the OpenTelemetry C++ libraries during the Godot build process.
