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
- State should be stored in a versioned format that can unambiguously restore the
  intended parameter values. Prefer plain values for framework-owned state, while
  remaining careful about backwards compatibility with existing normalized-only
  projects.

## Current implementation status

The VST3 adapter now uses SDK `RangeParameter` metadata for float parameters and
converts between VST3-normalized values and Singularity plain values at the
controller/processor boundary for linear ranges. Persisted VST3 state remains
normalized for compatibility until a dedicated state-versioning migration lands.
Follow-up work should extract shared conversion helpers and extend the same policy
to richer metadata such as units, flags, and named choices with SDK classes such
as `StringListParameter`.

## Examples

A plugin author should be able to declare and consume a cutoff parameter like
this:

```cpp
{ .id = 100, .name = "Cutoff", .minValue = 20.0, .maxValue = 20000.0,
  .defaultValue = 1000.0 }
```

and then use the value directly in DSP:

```cpp
auto cutoffHz = params.get(100, 1000.0);
```

The plugin author should not need to know the VST3-normalized value for `1000 Hz`.
That conversion belongs to the VST3 adapter.

## Migration guidance

Implementation PRs that touch parameters should preserve this boundary:

1. Treat framework-level parameter metadata values as plain values.
2. Convert to/from VST3-normalized values inside the VST3 adapter only.
3. Keep realtime processor state separate from controller-side SDK metadata.
4. Prefer shared conversion helpers so the processor and controller agree on the
   same mapping.
5. Add backwards-compatibility handling before changing persisted state formats.
