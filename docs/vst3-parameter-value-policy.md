# VST3 Parameter Value Policy

## Decision

Singularity's public parameter API should use **plain parameter values**. VST3-normalized
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

The current implementation keeps parameter conversion logic in the VST3 controller,
because VST3 parameters are controller-owned metadata objects:

- Float parameters are registered with the VST3 SDK `RangeParameter` class so the
  host sees the plugin-declared plain `minValue`, `maxValue`, and `defaultValue`.
- The controller uses the SDK parameter object's `toPlain()` and `toNormalized()`
  methods for `IParameterProvider::getParameter()` and `setParameter()`.
- Boolean, stepped, and choice parameters expose VST3 step counts.
- Parameter metadata can carry `units`, `shortName`, adapter-neutral flags (not bypass; the framework provides bypass automatically), and `groupId`.

The audio processor stores VST3 automation internally as normalized values for
sample-accurate processing, then converts those values to plain units before
building the `ParamList` passed to plugin DSP.

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
- Before calling Singularity plugin DSP, the VST3 processor converts normalized
  automation values to plain parameter values.
- When the Singularity UI sets a parameter, the VST3 controller converts the plain
  value to the normalized VST3 value required by the host edit/automation APIs.

## Examples

A plugin author can declare and consume a cutoff parameter like this:

```cpp
{ .id = 100, .name = "Cutoff", .shortName = "Cutoff", .units = "Hz",
  .minValue = 20.0, .maxValue = 20000.0, .defaultValue = 1000.0 }
```

The plugin author should not need to know the VST3-normalized value for `1000 Hz`.
That conversion belongs to the VST3 adapter.
