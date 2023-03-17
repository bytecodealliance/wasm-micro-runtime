// const WIDTH = 1200;
// const HEIGHT = 800;

function colour(iteration: number, offset: number, scale: number): number {
    iteration = (iteration * scale + offset) & 1023;
    if (iteration < 256) {
        return iteration;
    } else if (iteration < 512) {
        return 255 - (iteration - 255);
    }
    return 0;
}

function iterateEquation(
    x0: number,
    y0: number,
    maxiterations: number,
): number {
    let a = 0.0,
        b = 0.0,
        rx = 0.0,
        ry = 0.0,
        ab: number;
    let iterations = 0;
    while (iterations < maxiterations && rx * rx + ry * ry <= 4) {
        rx = a * a - b * b + x0;
        ab = a * b;
        ry = ab + ab + y0;
        a = rx;
        b = ry;
        iterations++;
    }
    return iterations;
}

function scale(
    domainStart: number,
    domainLength: number,
    screenLength: number,
    step: number,
): number {
    return domainStart + domainLength * (step * (1.0 / screenLength) - 1);
}

function mandelbrot(
    data: number[],
    HEIGHT: number,
    WIDTH: number,
    maxIterations: number,
    cx: number,
    cy: number,
    diameter: number,
) {
    const verticalDiameter = (diameter * HEIGHT) / WIDTH;
    for (let y = 0; y < HEIGHT; ++y) {
        for (let x = 0; x < WIDTH; ++x) {
            // convert from screen coordinates to mandelbrot coordinates
            const rx = scale(cx, diameter, WIDTH, x);
            const ry = scale(cy, verticalDiameter, HEIGHT, y);
            const iterations = iterateEquation(rx, ry, maxIterations);
            const outside = iterations == maxIterations;
            const idx = (x + y * WIDTH) << 2;
            data[idx + 0] = outside ? 0 : colour(iterations, 0, 4);
            data[idx + 1] = outside ? 0 : colour(iterations, 128, 4);
            data[idx + 2] = outside ? 0 : colour(iterations, 356, 4);
            data[idx + 3] = 255;
        }
    }
}

function getData() {
    const WIDTH = 1200;
    const HEIGHT = 800;
    const data: number[] = new Array(WIDTH * HEIGHT * 4);
    mandelbrot(
        data,
        HEIGHT,
        WIDTH,
        10000,
        -0.743644786,
        0.1318252536,
        0.00029336,
    );
}

getData();
