GCC="gcc -Wall -Werror -Wextra -Wconversion -Wpedantic -Wstrict-prototypes -std=gnu17 -o "

rm processmanager
rm clock
rm dummyprog

$GCC processmanager processmanager.c
$GCC clock clock.c
$GCC dummyprog dummyprog.c
