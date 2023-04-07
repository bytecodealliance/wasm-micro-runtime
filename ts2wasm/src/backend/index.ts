import { ParserContext } from '../frontend.js';
export { ParserContext } from '../frontend.js';

export abstract class Ts2wasmBackend {
    constructor(protected parserContext: ParserContext) {}

    public abstract codegen(options?: any): void;
    public abstract emitBinary(options?: any): Uint8Array;
    public abstract emitText(options?: any): string;
    public abstract dispose(): void;
}


export interface SegmentInfo {
    data: Uint8Array;
    offset: number;
}

export class DataSegmentContext {
    static readonly reservedSpace: number = 1024;
    currentOffset;
    stringOffsetMap;
    /* cache <typeid, itable*>*/
    itableMap;
    dataArray: Array<SegmentInfo> = [];

    constructor() {
        /* Reserve 1024 bytes at beggining */
        this.currentOffset = DataSegmentContext.reservedSpace;
        this.stringOffsetMap = new Map<string, number>();
        this.itableMap = new Map<number, number>();
    }

    addData(data: Uint8Array) {
        /* there is no efficient approach to cache the data buffer,
            currently we don't cache it */
        const offset = this.currentOffset;
        this.currentOffset += data.length;

        this.dataArray.push({
            data: data,
            offset: offset,
        });

        return offset;
    }

    addString(str: string) {
        if (this.stringOffsetMap.has(str)) {
            /* Re-use the string to save space */
            return this.stringOffsetMap.get(str)!;
        }

        const offset = this.currentOffset;
        this.stringOffsetMap.set(str, offset);
        this.currentOffset += str.length + 1;

        const buffer = new Uint8Array(str.length + 1);
        for (let i = 0; i < str.length; i++) {
            const byte = str.charCodeAt(i);
            if (byte >= 256) {
                throw Error('UTF-16 string not supported in data segment');
            }
            buffer[i] = byte;
        }
        buffer[str.length] = 0;

        this.dataArray.push({
            data: buffer,
            offset: offset,
        });

        return offset;
    }

    generateSegment(): SegmentInfo | null {
        const offset = DataSegmentContext.reservedSpace;
        const size = this.currentOffset - offset;

        if (this.dataArray.length === 0) {
            return null;
        }

        const data = new Uint8Array(size);
        this.dataArray.forEach((info) => {
            for (let i = 0; i < info.data.length; i++) {
                const targetOffset =
                    i + info.offset - DataSegmentContext.reservedSpace;
                data[targetOffset] = info.data[i];
            }
        });

        return {
            offset: offset,
            data: data,
        };
    }

    getDataEnd(): number {
        return this.currentOffset;
    }
}

