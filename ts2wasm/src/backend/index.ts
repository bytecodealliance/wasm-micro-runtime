import { ParserContext } from '../frontend.js';
export { ParserContext } from '../frontend.js';

export abstract class Ts2wasmBackend {
    constructor(protected parserContext: ParserContext) {}

    public abstract codegen(options?: any): void;
    public abstract emitBinary(options?: any): Uint8Array;
    public abstract emitText(options?: any): string;
    public abstract dispose(): void;
}
