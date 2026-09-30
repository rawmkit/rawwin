rawwin 0.1.0
============

* Establish a standalone interactive rawm window-selector workflow.
* Consume admitted clients only through the validated `rawctl client-list`
  projection instead of parsing rawm's wire snapshot.
* Delegate presentation to `rawmenu` and admitted-client focus to
  `rawctl focus-client`.
* Fail closed when selection is cancelled, malformed, stale, or rejected.
