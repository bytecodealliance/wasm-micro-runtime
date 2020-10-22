__attribute__((import_module("mA")))
__attribute__((import_name("A"))) extern int
A();

__attribute__((import_module("mB")))
__attribute__((import_name("B"))) extern int
B();

int
C()
{
    return 12;
}

int
call_A()
{
    return A();
}

int
call_B()
{
    return B();
}