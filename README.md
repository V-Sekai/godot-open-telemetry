# Godot Engine Open Telemetry

```gdscript
extends Node3D

var otel: OpenTelemetry = Opentelemetry.new()

func _ready() -> void:
	var error = otel.init_tracer_provider("godot", "localhost:4317", Engine.get_version_info())
	print(error)

func _process(_delta) -> void:
	var parent_span_id = otel.start_span("test-_ready")
	var span_id = otel.start_span_with_parent("test-child", parent_span_id)
	otel.add_event(span_id, "test-event")
	otel.set_attributes(span_id, {"test-key": "test-value"})
	otel.record_error(span_id, str(get_stack()))
	otel.end_span(span_id)
	otel.end_span(parent_span_id)

func _exit_tree() -> void:
	otel.shutdown()
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
