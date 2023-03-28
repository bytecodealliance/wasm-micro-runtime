## ts2wasm playground API

### Compile source code
- url: /compile
- payload:
    ``` json
    {
        "code": "xxxxxxx",
        "options": {
            "opt": true,    // enable optimization
            "format": "S-expression"    // output format
        }
    }
    ```
- response:
    ``` json
    {
        "content": "xxxxxxx",   // if compile success, the generated text representation
        "error": "xxxxxxx",  // if compile failed, the error message
        "wasm": "xxxxxxx"    // if compile success, base64 of the generated wasm binary
    }
    ```

### Feedback
- url: /feedback
- payload:
    ``` json
    {
        "user": "user_name",
        "suggests": "suggestion contents"
    }
    ```
- response: 200

### Sample list
- url: /samples
- payload: none
- response:
    ``` json
    [
        "sample1.ts", "sample2.ts", "..."
    ]
    ```

### Sample file
- url: /samples/:name
- payload: none
- response: file content (text/plain)
