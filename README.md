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

### Building with CMake

This module uses CMake for building the GDExtension library independently. The build process compiles the OpenTelemetry C++ dependencies and links them into a platform-specific shared library that Godot can load at runtime.

#### Prerequisites

- CMake 3.20 or later
- C++17 compatible compiler (GCC 8+, Clang 7+, MSVC 2019+)
- OpenSSL development headers
- Platform-specific development tools (see godot-cpp toolchain documentation)

#### Quick Build

Basic build for Linux x86_64 (default):

```bash
cd /path/to/godot-open-telemetry
mkdir build && cd build
cmake ..
make -j$(nproc)
```

The built library will be in `build/bin/` with a name like `opentelemetry.linux.x86_64.so`.

#### Platform-Specific Builds

The CMakeLists.txt automatically detects the target platform and sets appropriate output names.

For Linux:
- x86_64: `opentelemetry.linux.x86_64.so`
- ARM64: `opentelemetry.linux.arm64.so`

For macOS (currently targets arm64):
- `opentelemetry.macos.arm64.dylib`

#### Advanced Configuration

The build supports cross-compilation through standard CMake toolchain files:

```bash
cmake -S . -B build \
      -DCMAKE_TOOLCHAIN_FILE=/path/to/toolchain.cmake \
      -DPLATFORM=linuxbsd \
      -DARCH=x86_64
```

#### Dependencies

The OpenTelemetry libraries are built automatically from the included thirdparty sources. The following libraries are built and linked:

- `opentelemetry_api`: Core trace API
- `opentelemetry_sdk`: SDK implementation
- `opentelemetry_exporter_ostream_span`: Console logging exporter
- `opentelemetry_exporter_otlp_grpc`: OTLP gRPC exporter
- `opentelemetry_exporter_otlp_http`: OTLP HTTP exporter
- `opentelemetry_resources`: Resource detection

#### Build Options

- `WITH_OTLP`: Enable OTLP exporters (default: ON)
- `WITH_OTLP_GRPC`: Enable gRPC OTLP exporter (default: ON)
- `WITH_OTLP_HTTP`: Enable HTTP OTLP exporter (default: ON)
- `BUILD_SHARED_LIBS`: Build shared libraries (forced to OFF for static linking)
- `BUILD_TESTING`: Build unit tests (default: OFF)

#### Dev Container Build

When using the provided dev container, the build is simplified:

```bash
# In dev container, at project root
mkdir build && cd build
cmake ..
make -j$(nproc)
```

The dev container includes all necessary dependencies and build tools pre-installed.
