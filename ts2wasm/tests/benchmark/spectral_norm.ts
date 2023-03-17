export function approximate(n: number): number {
    const u: number[] = new Array(n),
        v: number[] = new Array(n);
    for (let i = 0; i < n; ++i) {
        u[i] = 1.0;
    }
    for (let i = 0; i < 10; ++i) {
        multiplyAtAv(n, u, v);
        multiplyAtAv(n, v, u);
    }
    let vBv = 0.0,
        vv = 0.0;
    for (let i = 0; i < 10; ++i) {
        vBv += u[i] * v[i];
        vv += v[i] * v[i];
    }
    return Math.sqrt(vBv / vv);
}

function a(i: number, j: number): number {
    return 1.0 / (((i + j) * (i + j + 1)) / 2 + i + 1);
}

function multiplyAv(n: number, v: number[], av: number[]) {
    for (let i = 0; i < n - 1; ++i) {
        av[i] = 0.0;
        for (let j = 0; j < n - 1; ++j) {
            av[i] += a(i, j) * v[j];
        }
    }
}

function multiplyAtv(n: number, v: number[], atv: number[]) {
    for (let i = 0; i < n - 1; ++i) {
        atv[i] = 0.0;
        for (let j = 0; j < n - 1; ++j) {
            atv[i] += a(j, i) * v[j];
        }
    }
}

function multiplyAtAv(n: number, v: number[], atAv: number[]) {
    const u: number[] = new Array(n);
    multiplyAv(n, v, u);
    multiplyAtv(n, u, atAv);
}

// console.log(approximate(+process.argv[2]).toFixed(9));
