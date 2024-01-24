/*
 * Copyright (C) 2023 Liquid Reply GmbH. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

use std::ffi::CStr;
use std::os::raw::c_char;
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

pub fn exception_to_string(raw_exception: *const c_char) -> String {
    let exception = unsafe { CStr::from_ptr(raw_exception) };
    String::from_utf8_lossy(exception.to_bytes()).to_string()
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::ffi::CString;

    #[test]
    fn test_error_buf_empty() {
        let error_buf = [0 as c_char; DEFAULT_ERROR_BUF_SIZE];
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

    #[test]
    fn test_exception_to_string() {
        let exception = "it is an exception";

        let exception_cstr = CString::new(exception).expect("CString::new failed");
        let exception_str = exception_to_string(exception_cstr.as_ptr());
        assert_eq!(exception_str.len(), exception.len());
        assert_eq!(exception_str, exception);
    }
}
