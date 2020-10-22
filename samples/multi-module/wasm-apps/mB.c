__attribute__((import_module("mA")))
__attribute__((import_name("A"))) extern int
A();

int
B()
{
    return 11;
}

int
call_A()
{
    return A();
}

