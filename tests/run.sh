#!/bin/sh
# Compiles and runs every tests/*.dhad. The number after the last '_' in a
# file name is the exit code it must produce; if a .txt with the same name
# exists, the program's output must match it.
# Every tests/errors/*.dhad must fail to compile with exactly the message on
# its first line, written as `# expect: <message>`.
# Linux x64 only; on other systems use `make docker-test`.
cd "$(dirname "$0")/.." && make -s || exit 1

fail=0
for f in tests/*.dhad; do
    [ -e "$f" ] || continue   # no tests yet: the glob did not match
    want=${f##*_}; want=${want%.dhad}
    rm -f out out.o out.asm
    output=$(./dhad "$f" >/dev/null && ./out)
    got=$?
    expected="${f%.dhad}.txt"
    if [ "$got" != "$want" ]; then
        echo "FAIL $f (want exit $want, got $got)"; fail=1
    elif [ -f "$expected" ] && [ "$output" != "$(cat "$expected")" ]; then
        echo "FAIL $f (output differs):"; printf '%s\n' "$output"; fail=1
    else
        echo "ok   $f"
    fi
done

for f in tests/errors/*.dhad; do
    [ -e "$f" ] || continue
    want=$(sed -n '1s/^# expect: //p' "$f")
    got=$(./dhad "$f" 2>&1 >/dev/null)
    code=$?
    if [ "$code" != 1 ] || [ "$got" != "$want" ]; then
        echo "FAIL $f (want \"$want\", got exit $code: \"$got\")"; fail=1
    else
        echo "ok   $f"
    fi
done
exit $fail
