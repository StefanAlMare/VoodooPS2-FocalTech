# FocalTech driver source

This directory contains the standalone FocalTech clients used by this fork.

`VoodooPS2FocalTech.cpp` is compiled through the upstream `VoodooPS2Trackpad` Xcode target in an isolated temporary build tree. The upstream project files therefore remain easy to compare and synchronize.

## Protocol backends

### FLT family

Runtime class:

```text
ApplePS2FocalTech
```

Targets the six-byte FocalTech protocol family used by:

```text
FLT0101
FLT0102
FLT0103 (experimental)
```

### FTE family

Runtime class:

```text
ApplePS2FTE0001
```

Targets the separate 8/16-byte `FTE0001` protocol family. This backend is experimental and refuses to probe unless the OpenCore boot argument below is present:

```text
focfte=1
```

See [`../Docs/FTE0001.md`](../Docs/FTE0001.md).

## Common transport/reporting

Provider:

```text
ApplePS2MouseDevice
```

Multitouch client:

```text
VoodooInput
```

Neither backend replaces VoodooPS2Controller; both use it as the PS/2 transport layer.
