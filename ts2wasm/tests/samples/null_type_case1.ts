class A {
    x = 10;
    static foo() {
        return 10;
    }
}

function test1() {
    const c: number | null = null;
    return c as null;
}

function test2() {
    const c: number | null = 10;
    return c as number;
}

function test3() {
    const a: A | null = new A();
    return a.x;
}

function test4() {
    const a: A | null | null = null;
    return a;
}

export function nullTypeTest() {
    let temp = test1();
    const res1 = test2();
    const res2 = test3();
    temp = test4();
    return res1 + res2;
}
