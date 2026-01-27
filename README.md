# Lexical Analyzer (Scanner) Project

A simple but robust lexical analyzer written in C. This tool reads source code and breaks it down into **tokens** (keywords, identifiers, numbers, operators, etc.) while ignoring whitespace and comments.

## Features
- **Tokenizes** standard C-like elements:
  - Keywords (`int`, `return`, `if`, etc.)
  - Identifiers (`main`, `total`, `user_id`)
  - Numbers (Integers `123` and Floats `3.14`)
  - Strings (`"hello"`) and Characters (`'c'`)
  - Operators (`+`, `==`, `!=`, etc.) & Separators (`;`, `{`, `}`)
- **Ignores Comments**:
  - Single-line (`// comment`)
  - Multi-line (`/* comment */`)
- **Error Detection**: Reports unterminated strings or characters.
- **Position Tracking**: Reports line and column number for every token.

## Prerequisites
You need a C compiler. On macOS (Apple Silicon M1/M2/M3), `clang` or `gcc` is standard.
To check if you have it installed, run:
```bash
clang --version
```
If missing, install Xcode Command Line Tools: `xcode-select --install`

## How to Compile

Open your terminal, navigate to this folder, and run:

```bash
clang -std=c11 -Wall -Wextra -pedantic -O2 lexer.c -o lexer
```

*   `-std=c11`: Use C11 standard.
*   `-Wall -Wextra`: Enable all warnings (good for safety).
*   `-O2`: Optimize the code.
*   `-o lexer`: Name the output executable `lexer`.

## How to Run

Pass a text file as an argument to the executable:

```bash
./lexer test.txt
```

### Included Test Files

1.  **Standard Test** (`test.txt`)
    *   Contains mixed code, comments, and whitespace.
    *   Command: `./lexer test.txt`

2.  **Multi-line Comments** (`test_multiline.txt`)
    *   Verifies that complex `/* ... */` blocks are skipped correctly.
    *   Command: `./lexer test_multiline.txt`

3.  **Error Handling** (`test_error.txt`)
    *   Contains a syntax error (unterminated string) to demonstrate error reporting.
    *   Command: `./lexer test_error.txt`

## Understanding the Output

The output format is: `[Line:Col] TOKEN_TYPE  "LEXEME"`

**Example:**
```text
[2:1] KEYWORD     "int"      <-- Found keyword 'int' at line 2
[2:5] IDENTIFIER  "main"     <-- Found identifier 'main' at line 2
[4:17] INT         "10"       <-- Found number '10' at line 4
```

## Troubleshooting

- **"Permission denied"**: Run `chmod +x lexer` to make it executable.
- **"command not found"**: Ensure you use `./lexer` (with the `./`), not just `lexer`.
