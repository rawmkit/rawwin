#!/bin/sh
set -eu

rawwin=${1:-./rawwin}
tmp=${TMPDIR:-/tmp}/rawwin-test.$$
mkdir -p "$tmp/bin"
trap 'rm -rf "$tmp"' EXIT HUP INT TERM

fail()
{
  echo "rawwin test failed: $*" >&2
  [ ! -f "$tmp/menu-input" ] || cat "$tmp/menu-input" >&2
  exit 1
}

cat >"$tmp/bin/rawctl" <<'EOS'
#!/bin/sh
set -eu
case ${1-} in
client-list)
  cat <<'EOC'
0x101	AlphaClass	Alpha%desk
0x202	BetaClass	Beta desk
EOC
  ;;
focus-client)
  [ "$#" -eq 2 ] || exit 64
  printf '%s\n' "$2" >>"$RAWWIN_FOCUS_LOG"
  [ "${RAWWIN_FOCUS_FAIL-0}" != 1 ] || exit 1
  ;;
*) exit 64 ;;
esac
EOS
chmod +x "$tmp/bin/rawctl"

cat >"$tmp/bin/rawmenu" <<'EOS'
#!/bin/sh
set -eu
cat >"$RAWWIN_MENU_INPUT"
[ "${RAWWIN_MENU_CANCEL-0}" != 1 ] || exit 1
case ${RAWWIN_MENU_SELECT-0x202} in
0x101) grep '^0x101  ' "$RAWWIN_MENU_INPUT" ;;
0x202) grep '^0x202  ' "$RAWWIN_MENU_INPUT" ;;
*) printf '%s\n' "$RAWWIN_MENU_SELECT" ;;
esac
EOS
chmod +x "$tmp/bin/rawmenu"

export PATH="$tmp/bin:$PATH"
export RAWWIN_FOCUS_LOG="$tmp/focus"
export RAWWIN_MENU_INPUT="$tmp/menu-input"

"$rawwin"
[ "$(cat "$tmp/focus")" = 0x202 ] || fail "selected client was not focused"
grep -qx '0x101  AlphaClass - Alpha%desk' "$tmp/menu-input" ||
  fail "projected percent text was not preserved"
grep -qx '0x202  BetaClass - Beta desk' "$tmp/menu-input" ||
  fail "projected title was not preserved"

: >"$tmp/focus"
RAWWIN_MENU_SELECT=0x101 "$rawwin"
[ "$(cat "$tmp/focus")" = 0x101 ] || fail "first client was not focused"

: >"$tmp/focus"
if RAWWIN_MENU_CANCEL=1 "$rawwin" >"$tmp/cancel.out" 2>"$tmp/cancel.err"; then
  fail "menu cancellation returned success"
fi
[ ! -s "$tmp/focus" ] || fail "menu cancellation focused a client"

: >"$tmp/focus"
if RAWWIN_FOCUS_FAIL=1 "$rawwin" >"$tmp/stale.out" 2>"$tmp/stale.err"; then
  fail "focus failure returned success"
fi
[ "$(cat "$tmp/focus")" = 0x202 ] || fail "focus request was not forwarded"

cat >"$tmp/bin/rawctl" <<'EOS'
#!/bin/sh
set -eu
case ${1-} in
client-list) : ;;
*) exit 64 ;;
esac
EOS
chmod +x "$tmp/bin/rawctl"
if "$rawwin" >"$tmp/empty.out" 2>"$tmp/empty.err"; then
  fail "empty inventory returned success"
fi
grep -q 'no admitted rawm clients' "$tmp/empty.err" ||
  fail "empty inventory diagnostic missing"

echo "rawwin tests: ok"
