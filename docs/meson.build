scdoc = find_program('scdoc', required: true)
raw_w = custom_target(
  'rawwin.1',
  input: 'rawwin.1.scdoc',
  output: 'rawwin.1',
  command: [scdoc],
  feed: true,
  capture: true,
  install: true,
  install_dir: get_option('mandir') / 'man1',
)
