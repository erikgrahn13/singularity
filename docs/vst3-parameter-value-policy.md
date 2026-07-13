# VST3 Parameter Value Policy

## Decision

Singularity's public parameter API uses **plain parameter values**. VST3-normalized
values are an implementation detail of the VST3 adapter and should not leak into
plugin DSP, the JavaScript UI API, standalone audio backends, or shared framework
interfaces.

In this policy:

- **Plain value** means the value in the units declared by the plugin author, such
  as hertz, decibels, milliseconds, semitones, boolean states, or integer/choice
  indices.
- **Normalized value** means the VST3 host-facing automation value in the
  inclusive `0.0` to `1.0` range.

## Implemented adapter behavior

- Float parameters are registered with the VST3 SDK `RangeParameter` class so the
  host sees the plugin-declared plain `minValue`, `maxValue`, and `defaultValue`.
- Choice parameters can be declared with `ParamType::Choice` and `choices`; the
  VST3 controller registers them with `StringListParameter` so hosts can display
  choice names.
- Boolean, stepped, and choice parameters expose discrete VST3 step counts.
- Parameter metadata can carry `units`, `shortName`, VST3 flags, and `unitId`, and plugin classes can optionally expose `getParameterGroups()` for VST3 units.
- Shared helper functions in the VST3 adapter own step-count, flag, and
  plain/normalized conversion logic so the controller and processor use the same
  mapping.
- The VST3 processor keeps sample-accurate automation state normalized internally
  and converts values to plain units before building the `ParamList` passed into
  plugin DSP.
- VST3 state is versioned and writes parameter IDs with values, while still
  accepting the previous order-based state format for compatibility.

## Ownership boundaries

### Shared Singularity API

The following framework-facing APIs should deal in plain values:

- `Parameter::minValue`, `Parameter::maxValue`, and `Parameter::defaultValue`.
- `ParamList` values passed to plugin DSP.
- `IParameterProvider::getParameter()` and `IParameterProvider::setParameter()`.
- JavaScript `getParameter()` and `setParameter()` bindings.
- Standalone backend parameter storage and parameter-change queues.

### VST3 adapter

The VST3 adapter owns the conversion between plain and normalized values:

- Host automation input arrives as normalized VST3 values.
- Host-facing parameter registration should expose enough metadata for conversion,
  including range, step count, units, flags, and list choices.
- Before calling Singularity plugin DSP, the VST3 processor should convert
  normalized automation values to plain values.
- When the Singularity UI sets a parameter, the VST3 controller should convert the
  plain value to the normalized VST3 value required by the host edit/automation
  APIs.
- Versioned state should use stable parameter IDs so parameters can be reordered
  without breaking future project loading.

## Examples

A plugin author can declare and consume a cutoff parameter like this:

```cpp
{ .id = 100, .name = "Cutoff", .shortName = "Cutoff", .units = "Hz",
  .minValue = 20.0, .maxValue = 20000.0, .defaultValue = 1000.0 }
```

and then use the value directly in DSP:

```cpp
auto cutoffHz = params.get(100, 1000.0);
```

The plugin author should not need to know the VST3-normalized value for `1000 Hz`.
That conversion belongs to the VST3 adapter.

A choice parameter can declare names for host display:

```cpp
{ .id = 200, .name = "Oscillator Shape", .type = ParamType::Choice,
  .defaultValue = 0.0, .choices = { "Sine", "Saw", "Square" } }
```

DSP receives the selected choice index as a plain value.
