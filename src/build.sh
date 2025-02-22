GCC="gcc -Wall -Werror -Wextra -Wconversion -Wpedantic -Wstrict-prototypes -std=gnu17 -o "
BIN="../bin/"


mkdir -p ${BIN}

rm ${BIN}processmanager
rm ${BIN}clock
rm ${BIN}pr
rm ${BIN}execpractice.c

$GCC ${BIN}processmanager processmanager.c
$GCC ${BIN}clock clock.c
$GCC ${BIN}pr pr.c
$GCC ${BIN}shell shell.c
$GCC ${BIN}execpractice execpractice.c