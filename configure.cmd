extproc sh

n=configure
test -f "./$n." || { echo "\`./$n' not found !!!"; exit 1; }
"./$n" --enable-shared --enable-static "$@" 2>&1 | tee "$n.log"
