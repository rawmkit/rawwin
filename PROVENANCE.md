# Provenance

rawwin was created as a new rawmkit repository in September 2026 while removing
the transitional `rawmctl window select` implementation from the desktop policy.

The implementation is new MIT-licensed code. It does not copy the old in-tree
rawmctl EWMH/XKB implementation or rawmenu implementation source. The behavioral
workflow is defined only in terms of the public `rawctl` and `rawmenu` command
interfaces.
