use crate::RuntimeError;

pub const DEFAULT_ERROR_BUF_SIZE: usize = 128;

//TODO: std::os::raw::c_char?
/// Convert a error buffer to a `RuntimeError`
pub fn error_buf_to_runtime_error(&error_buf: &[i8; DEFAULT_ERROR_BUF_SIZE]) -> RuntimeError {
    let error_u8 = error_buf.map(|c| c as u8);
    let error_str = match String::from_utf8(error_u8.to_vec()) {
        Ok(s) => s,
        Err(e) => return RuntimeError::CompilationError(e.to_string()),
    };
    //TODO: remove trailing zeros
    return RuntimeError::CompilationError(error_str);
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_error_buf() {
        let mut error_buf = [0i8; DEFAULT_ERROR_BUF_SIZE];
        error_buf[0] = 'a' as i8;
        error_buf[1] = 'b' as i8;
        error_buf[2] = 'c' as i8;

        let runtime_error = error_buf_to_runtime_error(&error_buf);
        println!("--> runtime_error {:?}", runtime_error);
        // assert_eq!(
        //     runtime_error,
        //     RuntimeError::CompilationError("abc".to_string())
        // );
    }
}
