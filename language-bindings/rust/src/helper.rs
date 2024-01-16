use ::core::ffi::c_char;
use std::string::String;

pub const DEFAULT_ERROR_BUF_SIZE: usize = 128;

pub fn error_buf_to_string(&error_buf: &[c_char; DEFAULT_ERROR_BUF_SIZE]) -> String {
    let error_content: Vec<u8> = error_buf
        .map(|c| c as u8)
        .into_iter()
        .filter(|c| *c > 0)
        .collect();
    String::from_utf8(error_content).unwrap()
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_error_buf_empty() {
        let mut error_buf = [0 as c_char; DEFAULT_ERROR_BUF_SIZE];
        let error_str = error_buf_to_string(&error_buf);
        assert_eq!(error_str.len(), 0);
        assert_eq!(error_str, "");
    }

    #[test]
    fn test_error_buf() {
        let mut error_buf = [0 as c_char; DEFAULT_ERROR_BUF_SIZE];
        error_buf[0] = 'a' as i8;
        error_buf[1] = 'b' as i8;
        error_buf[2] = 'c' as i8;

        let error_str = error_buf_to_string(&error_buf);
        assert_eq!(error_str.len(), 3);
        assert_eq!(error_str, "abc");
    }
}
