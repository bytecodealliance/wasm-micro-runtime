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
        "error": "xxxxxxx"  // if compile failed, the error message
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
