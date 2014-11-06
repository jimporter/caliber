// caliber --name "pre-defined macros" -DUNDEF -UUNDEF -U MACRO -D MACRO
#ifndef MACRO
#error "MACRO not defined"
#endif

#ifdef UNDEF
#error "UNDEF was defined"
#endif

int main() {
}
