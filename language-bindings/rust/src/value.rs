//! a wasm value. Always used as function parameters and results

#[derive(Debug, PartialEq)]
pub enum WasmValue {
    I32(i32),
    I64(i64),
    F32(f32),
    F64(f64),
    V128(i128),
}

impl WasmValue {
    pub fn encode(&self) -> Vec<u32> {
        match *self {
            WasmValue::I32(value) => {
                let in_u32_array = unsafe { std::mem::transmute::<i32, [u32; 1]>(value) };
                vec![in_u32_array[0]]
            }
            WasmValue::I64(value) => {
                let in_u32_array = unsafe { std::mem::transmute::<i64, [u32; 2]>(value) };
                vec![in_u32_array[0], in_u32_array[1]]
            }
            WasmValue::F32(value) => {
                let in_u32_array = unsafe { std::mem::transmute::<f32, [u32; 1]>(value) };
                vec![in_u32_array[0]]
            }
            WasmValue::F64(value) => {
                let in_u32_array = unsafe { std::mem::transmute::<f64, [u32; 2]>(value) };
                vec![in_u32_array[0], in_u32_array[1]]
            }
            WasmValue::V128(value) => {
                let in_u32_array = unsafe { std::mem::transmute::<i128, [u32; 4]>(value) };
                vec![
                    in_u32_array[0],
                    in_u32_array[1],
                    in_u32_array[2],
                    in_u32_array[3],
                ]
            }
        }
    }

    pub fn decode_to_i32(binary: Vec<u32>) -> WasmValue {
        let binary: [u32; 1] = [binary[0]];
        WasmValue::I32(unsafe { std::mem::transmute::<[u32; 1], i32>(binary) })
    }

    pub fn decode_to_f32(binary: Vec<u32>) -> WasmValue {
        let binary: [u32; 1] = [binary[0]];
        WasmValue::F32(unsafe { std::mem::transmute::<[u32; 1], f32>(binary) })
    }

    pub fn decode_to_i64(binary: Vec<u32>) -> WasmValue {
        let binary: [u32; 2] = [binary[0], binary[1]];
        WasmValue::I64(unsafe { std::mem::transmute::<[u32; 2], i64>(binary) })
    }

    pub fn decode_to_f64(binary: Vec<u32>) -> WasmValue {
        let binary: [u32; 2] = [binary[0], binary[1]];
        WasmValue::F64(unsafe { std::mem::transmute::<[u32; 2], f64>(binary) })
    }

    pub fn decode_to_v128(binary: Vec<u32>) -> WasmValue {
        let binary: [u32; 4] = [binary[0], binary[1], binary[2], binary[3]];
        WasmValue::V128(unsafe { std::mem::transmute::<[u32; 4], i128>(binary) })
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_encode() {
        let params = vec![WasmValue::I32(1), WasmValue::I64(2)];

        let mut ret: Vec<u32> = Vec::new();
        for p in params {
            ret.append(&mut p.encode());
        }

        assert_eq!(ret.len(), 3);
        assert_eq!(ret, vec![1, 2, 0]);
    }

    #[test]
    fn test_encode_decode() {
        let values = vec![
            WasmValue::I32(1),
            WasmValue::I64(2),
            WasmValue::F32(3.0),
            WasmValue::F64(4.0),
            WasmValue::V128(5),
        ];

        let mut binary: Vec<u32> = Vec::new();
        for v in &values {
            binary.append(&mut v.encode());
        }

        let mut decoded_values: Vec<WasmValue> = Vec::new();
        decoded_values.push(WasmValue::decode_to_i32(binary[0..1].to_vec()));
        decoded_values.push(WasmValue::decode_to_i64(binary[1..3].to_vec()));
        decoded_values.push(WasmValue::decode_to_f32(binary[3..4].to_vec()));
        decoded_values.push(WasmValue::decode_to_f64(binary[4..6].to_vec()));
        decoded_values.push(WasmValue::decode_to_v128(binary[6..10].to_vec()));

        assert_eq!(values, decoded_values);
    }
}
