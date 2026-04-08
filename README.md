# ZMK Persistent Default Layer (PDF) Behavior

A ZMK module that provides persistent layer switching functionality. When you activate a layer with the `pdf` (persistent default layer) behavior, it saves that layer selection to the keyboard's settings and restores it on boot. This mimics QMK's PDF function.

## Features

- **Persistent Layer Selection**: Saves the active layer to NVS (Non-Volatile Storage)
- **Auto-Restore**: Automatically switches to the saved layer on keyboard startup
- **Simple Integration**: Easy to integrate into existing ZMK keyboards
- **Configurable**: Supports any layer number within the keyboard's layer configuration

## Installation

1. Add this module to your ZMK config's `west.yml`:

```yaml
manifest:
  remotes:
    - name: zmkfirmware
      url-base: https://github.com/zmkfirmware
    - name: your-username
      url-base: https://github.com/your-username
  projects:
    - name: zmk
      remote: zmkfirmware
      revision: main
    - name: zmk-default-layer
      remote: your-username
      url: /zmk-default-layer.git
      revision: main
  self:
    path: config
```

2. Enable the behavior in your `build.yaml` or `west build` command:

```
west build -d build/left -b nice_nano_v2 -- -DZMK_EXTRA_MODULES=...path/to/zmk-default-layer
```

## Usage

### Device Tree Definition

Define the behavior in your `.keymap` file or device tree:

```dts
/ {
    behaviors {
        pdf: pdf {
            compatible = "zmk,behaviour-persistent-default-layer";
            #binding-cells = <1>;
            default-layer = <0>;
        };
    };
};
```

### Keymap Example

```dts
// Switch to layer 0 (and save it)
&pdf 0

// Switch to layer 1 (and save it)
&pdf 1

// Switch to layer 2 (and save it)
&pdf 2
```

Complete keymap example:

```dts
#include <behaviors.dtsi>
#include <dt-bindings/zmk/keys.h>

/ {
    behaviors {
        pdf: pdf {
            compatible = "zmk,behaviour-persistent-default-layer";
            #binding-cells = <1>;
            default-layer = <0>;
        };
    };

    keymap {
        compatible = "zmk,keymap";

        base_layer {
            label = "Base";
            bindings = <
                &pdf 0  &kp Q      &kp W      &kp E      // ...
                &kp A   &kp S      &kp D      &kp F      // ...
            >;
        };

        function_layer {
            label = "Fn";
            bindings = <
                &pdf 1  &kp F1     &kp F2     &kp F3     // ...
                &kp F4  &kp F5     &kp F6     &kp F7     // ...
            >;
        };
    };
};
```

## Configuration

Enable the behavior in your `prj.conf`:

```
CONFIG_ZMK_BEHAVIOR_PDF=y
```

## How It Works

1. **Initialization**: On keyboard startup, the module loads the saved layer from NVS
2. **Layer Activation**: When a `pdf` key is pressed, it activates the specified layer
3. **Persistence**: The activated layer is saved to NVS for later restoration
4. **Boot Restoration**: On next boot, the saved layer is automatically activated

## Technical Details

- Uses Zephyr's Settings subsystem for persistent storage
- Saves/loads data from the `pdf/active` key in NVS
- Thread-safe layer switching using ZMK's `zmk_layer_on()` API
- Minimal memory footprint (single byte storage for layer number)

## Troubleshooting

### Layer not persisting
- Ensure settings storage is enabled in your build
- Check that `CONFIG_SETTINGS=y` is set in your `prj.conf`
- Verify NVS storage is properly configured for your board

### Layer not restoring on boot
- Check the debug logs: `CONFIG_ZMK_LOG_LEVEL=DBG`
- Ensure the module is properly included in your build
- Verify the layer number is valid (0 to max layers - 1)

## License

MIT License - See LICENSE file

## Contributing

Contributions are welcome! Please submit issues and pull requests.
