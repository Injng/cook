#include "../cook.h"

int main() {
    if (compile( "main.c", "main.o", COMPILER, FLAGS) < 0) {
        return 1;
    }
    if (ln( "main.o", "main") < 0) {
        return 1;
    }
    clean(1, (char*[]){"."}, REMOVE);
    return 0;
}

