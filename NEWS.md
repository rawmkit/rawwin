rawwin 0.1.0
============

* Establish a standalone interactive rawm window-selector workflow.
* Consume rawm's authoritative client inventory only through `rawctl clients`.
* Delegate presentation to `rawmenu` and admitted-client focus to
  `rawctl focus-client`.
* Fail closed when selection is cancelled, malformed, stale, or rejected.
