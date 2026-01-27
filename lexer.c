#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LEXEME_LEN 256
#define MAX_ID_LEN 64 // "reasonable" identifier limit

typedef enum {
  TOK_EOF,
  TOK_KEYWORD,
  TOK_IDENTIFIER,
  TOK_INT,
  TOK_FLOAT,
  TOK_STRING,
  TOK_CHAR,
  TOK_OPERATOR,
  TOK_SEPARATOR,
  TOK_UNKNOWN,
  TOK_ERROR
} TokenType;

typedef struct {
  TokenType type;
  char lexeme[MAX_LEXEME_LEN];
  int line;
  int col;
} Token;

/* ---------- Keywords list (extend as needed) ---------- */
static const char *KEYWORDS[] = {
    "if",   "else", "while", "for",      "return", "int",  "float",
    "char", "void", "break", "continue", "struct", "const"};
static const int KEYWORD_COUNT = (int)(sizeof(KEYWORDS) / sizeof(KEYWORDS[0]));

static int is_keyword(const char *s) {
  for (int i = 0; i < KEYWORD_COUNT; i++) {
    if (strcmp(s, KEYWORDS[i]) == 0)
      return 1;
  }
  return 0;
}

/* ---------- Global position tracking ---------- */
static int g_line = 1;
static int g_col = 0; // column of last read character

static int read_char(FILE *fp) {
  int c = fgetc(fp);
  if (c == '\n') {
    g_line++;
    g_col = 0;
  } else if (c != EOF) {
    g_col++;
  }
  return c;
}

static void unread_char(int c, FILE *fp) {
  if (c == EOF)
    return;
  ungetc(c, fp);
  // Rollback column/line tracking conservatively:
  // (Exact rollback for '\n' would require more state; for a simple demo,
  // we avoid unread of '\n' in our scanner logic.)
  if (c != '\n')
    g_col--;
}

static Token make_token(TokenType type, const char *lex, int line, int col) {
  Token t;
  t.type = type;
  strncpy(t.lexeme, lex, MAX_LEXEME_LEN - 1);
  t.lexeme[MAX_LEXEME_LEN - 1] = '\0';
  t.line = line;
  t.col = col;
  return t;
}

/* ---------- Skip whitespace and comments ---------- */
static void skip_whitespace_and_comments(FILE *fp) {
  while (1) {
    int c = read_char(fp);

    // Skip whitespace
    while (isspace(c)) {
      c = read_char(fp);
    }

    // Check for comments
    if (c == '/') {
      int next = read_char(fp);

      // Single-line comment //
      if (next == '/') {
        int ch = read_char(fp);
        while (ch != '\n' && ch != EOF) {
          ch = read_char(fp);
        }
        // Continue outer loop: might be more whitespace/comments
        continue;
      }

      // Multi-line comment /* ... */
      if (next == '*') {
        int prev = 0;
        int ch = read_char(fp);

        while (ch != EOF) {
          if (prev == '*' && ch == '/') {
            break;
          }
          prev = ch;
          ch = read_char(fp);
        }

        // If unterminated, we just stop at EOF (caller will hit EOF)
        // Continue outer loop
        continue;
      }

      // Not a comment: put back both
      unread_char(next, fp);
      unread_char(c, fp);
      return;
    }

    // Not whitespace/comment: put back and return
    unread_char(c, fp);
    return;
  }
}

/* ---------- Read identifier/keyword ---------- */
static Token read_identifier_or_keyword(FILE *fp) {
  int start_line = g_line;
  int start_col = g_col + 1; // about to read first char

  char buf[MAX_LEXEME_LEN];
  int len = 0;

  int c = read_char(fp);
  while (isalnum(c) || c == '_') {
    if (len < MAX_LEXEME_LEN - 1) {
      buf[len++] = (char)c;
    }
    c = read_char(fp);
  }
  buf[len] = '\0';
  unread_char(c, fp);

  // Enforce MAX_ID_LEN for identifiers (keywords are short anyway)
  if (!is_keyword(buf) && (int)strlen(buf) > MAX_ID_LEN) {
    char truncated[MAX_LEXEME_LEN];
    strncpy(truncated, buf, MAX_ID_LEN);
    truncated[MAX_ID_LEN] = '\0';
    return make_token(TOK_IDENTIFIER, truncated, start_line, start_col);
  }

  if (is_keyword(buf))
    return make_token(TOK_KEYWORD, buf, start_line, start_col);
  return make_token(TOK_IDENTIFIER, buf, start_line, start_col);
}

/* ---------- Read number (int/float) ---------- */
static Token read_number(FILE *fp) {
  int start_line = g_line;
  int start_col = g_col + 1;

  char buf[MAX_LEXEME_LEN];
  int len = 0;
  int is_float = 0;

  int c = read_char(fp);
  while (isdigit(c)) {
    if (len < MAX_LEXEME_LEN - 1)
      buf[len++] = (char)c;
    c = read_char(fp);
  }

  if (c == '.') {
    is_float = 1;
    if (len < MAX_LEXEME_LEN - 1)
      buf[len++] = '.';
    c = read_char(fp);
    while (isdigit(c)) {
      if (len < MAX_LEXEME_LEN - 1)
        buf[len++] = (char)c;
      c = read_char(fp);
    }
  }

  buf[len] = '\0';
  unread_char(c, fp);

  return make_token(is_float ? TOK_FLOAT : TOK_INT, buf, start_line, start_col);
}

/* ---------- Read string literal ---------- */
static Token read_string(FILE *fp) {
  int start_line = g_line;
  int start_col = g_col + 1;

  char buf[MAX_LEXEME_LEN];
  int len = 0;

  int quote = read_char(fp); // should be '"'
  (void)quote;

  int c = read_char(fp);
  while (c != EOF && c != '"') {
    if (c == '\n') {
      return make_token(TOK_ERROR, "Unterminated string literal", start_line,
                        start_col);
    }
    if (c == '\\') { // escape sequence
      if (len < MAX_LEXEME_LEN - 2)
        buf[len++] = (char)c;
      c = read_char(fp);
      if (c == EOF)
        return make_token(TOK_ERROR, "Unterminated string literal", start_line,
                          start_col);
      if (len < MAX_LEXEME_LEN - 2)
        buf[len++] = (char)c;
    } else {
      if (len < MAX_LEXEME_LEN - 1)
        buf[len++] = (char)c;
    }
    c = read_char(fp);
  }

  if (c == EOF)
    return make_token(TOK_ERROR, "Unterminated string literal", start_line,
                      start_col);
  buf[len] = '\0';
  return make_token(TOK_STRING, buf, start_line, start_col);
}

/* ---------- Read char literal ---------- */
static Token read_char_literal(FILE *fp) {
  int start_line = g_line;
  int start_col = g_col + 1;

  char buf[MAX_LEXEME_LEN];
  int len = 0;

  int quote = read_char(fp); // should be '\''
  (void)quote;

  int c = read_char(fp);
  if (c == EOF || c == '\n') {
    return make_token(TOK_ERROR, "Unterminated char literal", start_line,
                      start_col);
  }

  if (c == '\\') { // escaped char like '\n'
    if (len < MAX_LEXEME_LEN - 2)
      buf[len++] = (char)c;
    c = read_char(fp);
    if (c == EOF || c == '\n')
      return make_token(TOK_ERROR, "Unterminated char literal", start_line,
                        start_col);
    if (len < MAX_LEXEME_LEN - 2)
      buf[len++] = (char)c;
  } else {
    if (len < MAX_LEXEME_LEN - 1)
      buf[len++] = (char)c;
  }

  int closing = read_char(fp);
  if (closing != '\'') {
    return make_token(TOK_ERROR, "Invalid/unterminated char literal",
                      start_line, start_col);
  }

  buf[len] = '\0';
  return make_token(TOK_CHAR, buf, start_line, start_col);
}

/* ---------- Operators & separators (handles multi-char) ---------- */
static int is_separator_char(int c) {
  return (c == '(' || c == ')' || c == '{' || c == '}' || c == '[' ||
          c == ']' || c == ';' || c == ',');
}

static Token read_operator_or_separator(FILE *fp) {
  int start_line = g_line;
  int start_col = g_col + 1;

  char lex[4] = {0};
  int c1 = read_char(fp);

  // Separators: single-char
  if (is_separator_char(c1)) {
    lex[0] = (char)c1;
    lex[1] = '\0';
    return make_token(TOK_SEPARATOR, lex, start_line, start_col);
  }

  // Try multi-char operators
  int c2 = read_char(fp);
  lex[0] = (char)c1;
  lex[1] = (char)c2;
  lex[2] = '\0';

  // List of common 2-char operators
  const char *two_ops[] = {"==", "!=", "<=", ">=", "&&", "||", "++",
                           "--", "+=", "-=", "*=", "/=", "%=", "->"};
  int two_ops_count = (int)(sizeof(two_ops) / sizeof(two_ops[0]));

  for (int i = 0; i < two_ops_count; i++) {
    if (strcmp(lex, two_ops[i]) == 0) {
      return make_token(TOK_OPERATOR, lex, start_line, start_col);
    }
  }

  // Not a 2-char operator => unread c2 and treat c1 as single operator/unknown
  unread_char(c2, fp);

  lex[0] = (char)c1;
  lex[1] = '\0';

  // Single-char operators set (extend as needed)
  if (strchr("+-*/%<>=!&|^~?:.", c1)) {
    return make_token(TOK_OPERATOR, lex, start_line, start_col);
  }

  return make_token(TOK_UNKNOWN, lex, start_line, start_col);
}

/* ---------- Get next token ---------- */
static Token next_token(FILE *fp) {
  skip_whitespace_and_comments(fp);

  int c = read_char(fp);
  if (c == EOF)
    return make_token(TOK_EOF, "EOF", g_line, g_col);

  unread_char(c, fp);

  // Decide token type by first char
  c = read_char(fp);
  unread_char(c, fp);

  if (isalpha(c) || c == '_') {
    return read_identifier_or_keyword(fp);
  }
  if (isdigit(c)) {
    return read_number(fp);
  }
  if (c == '"') {
    return read_string(fp);
  }
  if (c == '\'') {
    return read_char_literal(fp);
  }

  // Comments are already skipped, so here / is operator
  return read_operator_or_separator(fp);
}

/* ---------- Token printing ---------- */
static const char *token_name(TokenType t) {
  switch (t) {
  case TOK_EOF:
    return "EOF";
  case TOK_KEYWORD:
    return "KEYWORD";
  case TOK_IDENTIFIER:
    return "IDENTIFIER";
  case TOK_INT:
    return "INT";
  case TOK_FLOAT:
    return "FLOAT";
  case TOK_STRING:
    return "STRING";
  case TOK_CHAR:
    return "CHAR";
  case TOK_OPERATOR:
    return "OPERATOR";
  case TOK_SEPARATOR:
    return "SEPARATOR";
  case TOK_UNKNOWN:
    return "UNKNOWN";
  case TOK_ERROR:
    return "ERROR";
  default:
    return "???";
  }
}

int main(int argc, char **argv) {
  FILE *fp = NULL;

  if (argc < 2) {
    printf("Usage: %s <source_file>\n", argv[0]);
    printf("Example: %s test.txt\n", argv[0]);
    return 1;
  }

  fp = fopen(argv[1], "r");
  if (!fp) {
    perror("fopen");
    return 1;
  }

  printf("Lexical Analysis Output:\n");
  printf("------------------------\n");

  while (1) {
    Token t = next_token(fp);
    printf("[%d:%d] %-10s  \"%s\"\n", t.line, t.col, token_name(t.type),
           t.lexeme);

    if (t.type == TOK_ERROR) {
      printf("Stopping due to error.\n");
      break;
    }
    if (t.type == TOK_EOF)
      break;
  }

  fclose(fp);
  return 0;
}
