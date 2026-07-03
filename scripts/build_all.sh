#!/usr/bin/env bash
# SOMA — compila y corre TODOS los tests. Devuelve !=0 si alguno falla.
set -u
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT" || exit 2
fail=0
for t in tests/test_*.cpp; do
  "$ROOT/scripts/build_test.sh" "$t" || fail=1
done
echo "----"
[ $fail -eq 0 ] && echo "TODOS LOS TESTS PASAN" || echo "HAY TESTS EN FALLO"
exit $fail
