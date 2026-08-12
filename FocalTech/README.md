# FocalTech driver source

This directory contains the standalone FocalTech client used by this fork.

`VoodooPS2FocalTech.cpp` is compiled through the upstream `VoodooPS2Trackpad` Xcode target in an isolated temporary build tree. The upstream project files therefore remain easy to compare and synchronize.

Runtime class:

```text
ApplePS2FocalTech
```

Provider:

```text
ApplePS2MouseDevice
```

Multitouch client:

```text
VoodooInput
```

The driver does not replace VoodooPS2Controller; it uses it as the PS/2 transport layer.
