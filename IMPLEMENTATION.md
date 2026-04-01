# Implementation Details

## Architecture

The `behaviour-persistent-default-layer` (pdf) module implements a persistent layer switching behavior for ZMK keyboards. Here's how it works:

### Components

1. **Behavior Driver** (`src/behavior_persistent_default_layer.c`)
   - Implements the ZMK behavior interface
   - Handles key press/release events
   - Manages layer switching
   - Interfaces with the settings subsystem

2. **Module Header** (`include/zmk/behaviors/behavior_persistent_default_layer.h`)
   - Exports the behavior API
   - Defines configuration constants

3. **Device Tree Binding** (`dts/bindings/behaviors/zmk,behaviour-persistent-default-layer.yaml`)
   - Defines the DT compatible string
   - Specifies device tree properties
   - Enables the behavior to be used in keymaps

4. **Build Configuration**
   - `CMakeLists.txt`: Builds the behavior module
   - `Kconfig`: Exposes configuration options
   - `zephyr/module.yml`: Registers the module with ZMK

## Key Features

### 1. Settings Persistence

The module uses Zephyr's Settings subsystem to store/load the active layer:

```c
// Save layer to NVS
settings_save_one("pdf/active", &saved_layer, sizeof(saved_layer));

// Load layer from NVS
read_cb(cb_arg, &saved_layer, sizeof(saved_layer));
```

Data is stored as a single byte representing the layer number (0-255).

### 2. Layer Switching

When a key with the `pdf` behavior is pressed:

```c
// Switch to specified layer
zmk_layer_on(binding->param1);

// Save for persistence
saved_layer = binding->param1;
pdf_settings_save();
```

The layer parameter (`binding->param1`) is passed via device tree and specifies which layer to activate.

### 3. Boot Restoration

During module initialization:

```c
static int pdf_init(const struct device *dev)
{
    // Load settings from persistent storage
    settings_load_subtree("pdf");
    
    // If a layer was saved, activate it
    if (saved_layer > 0) {
        zmk_layer_on(saved_layer);
    }
    
    return 0;
}
```

The `pdf_init` function is called at boot time (POST_KERNEL priority 81) to restore the saved layer.

## Behavior Binding

The behavior is defined with `#binding-cells = <1>`, meaning it takes one parameter:

```dts
pdf: pdf {
    compatible = "zmk,behaviour-persistent-default-layer";
    #binding-cells = <1>;
    default-layer = <0>;
};
```

In keymaps, use it like:
```dts
&pdf 0  // Activate and save layer 0
&pdf 1  // Activate and save layer 1
```

## Data Flow

### On Key Press:
```
User presses key
    ↓
ZMK routes to behavior
    ↓
pdf_on_keydown() called
    ↓
zmk_layer_on(layer_num)
    ↓
settings_save_one("pdf/active", ...)
    ↓
Layer changed and saved
```

### On Boot:
```
Keyboard starts
    ↓
Post-kernel initialization
    ↓
pdf_init() called
    ↓
settings_load_subtree("pdf")
    ↓
Saved layer loaded from NVS
    ↓
zmk_layer_on(saved_layer)
    ↓
Keyboard boots to saved layer
```

## Settings Subsystem Integration

The module registers with Zephyr's settings subsystem:

```c
static struct settings_handler_static pdf_handler = {
    .name = "pdf",
    .h_get = pdf_settings_load,
};

// In init:
settings_register(&pdf_handler);
settings_load_subtree("pdf");
```

Settings are stored in NVS (Non-Volatile Storage) under the key `pdf/active`.

## Thread Safety

The module operates at kernel initialization time and post-kernel phases. Layer switching is thread-safe as it uses ZMK's built-in `zmk_layer_on()` API which handles synchronization.

## Memory Usage

- **Runtime**: ~20-30 bytes (behavior structure, handler)
- **Storage**: 1 byte per layer (saved in NVS)
- Minimal impact on keyboard firmware size

## Limitations

- Can only save one layer at a time (the last active layer)
- Layer number must be valid (0 to number of layers - 1)
- Requires NVS storage to be configured on the board

## Future Enhancements

Potential improvements:
- Save multiple layer states (stack-based layer history)
- Per-key layer memory
- Layer lock/toggle variants
- Export settings handling for debugging

## Dependencies

- ZMK (main firmware)
- Zephyr kernel
- Settings subsystem (CONFIG_SETTINGS)
- NVS storage driver (board-specific)

## Configuration

Required in `prj.conf`:
```
CONFIG_ZMK_BEHAVIOR_PDF=y
CONFIG_SETTINGS=y
CONFIG_NVS=y
```

Optional:
```
CONFIG_ZMK_BEHAVIOR_PDF_MAX_LAYER=7
CONFIG_ZMK_LOG_LEVEL=DBG  # For debugging
```
