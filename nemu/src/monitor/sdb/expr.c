#include <isa.h>
#include <memory/vaddr.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>

enum {
  TK_NOTYPE = 256,
  
  TK_OCT,
  // TK_BIN,
  TK_HEX,

  TK_MINUS,
  TK_DEREF,

  TK_REG,

  TK_OR,
  TK_AND,
  TK_EQ,
  TK_NOTEQ,

  /* TODO: Add more token types */

};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces

  {"[0-9]+\\b", TK_OCT},
  {"0[xX][0-9A-Fa-f]+\\b", TK_HEX},
  // {"0[bB][0-1]+\\b", TK_BIN},
  
  {"^[\\$rsgtap][cap0-9]+", TK_REG},

  {"\\+", '+'},         // plus "\\" --> "\" 
  {"\\-", '-'},         // minus
  {"\\*", '*'},         //plus
  {"\\/", '/'},         //division
  {"\\(", '('},
  {"\\)", ')'},

  {"==", TK_EQ},        // equal
  {"!=", TK_NOTEQ},
  {"\\|{2}", TK_OR},  
  {"&&", TK_AND},
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

static Token tokens[65536] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        // Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
        //     i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */
#define CASE_TK(TK) \
      {case TK:    \
      tokens[nr_token].type = TK;\
      strncpy(tokens[nr_token].str, substr_start, substr_len);\
      strcpy(tokens[nr_token].str + substr_len, "\0");\
      nr_token ++;\
      break;}

        switch (rules[i].token_type) {
          case TK_NOTYPE: break;          
          // CASE_TK(TK_BIN)
          CASE_TK(TK_HEX)
          CASE_TK(TK_OCT)
          CASE_TK(TK_REG)
          CASE_TK('+')
          CASE_TK('-')
          CASE_TK('*')
          CASE_TK('/')
          CASE_TK('(')
          CASE_TK(')')
          CASE_TK(TK_OR)
          CASE_TK(TK_AND)
          CASE_TK(TK_EQ)
          CASE_TK(TK_NOTEQ)

          default: TODO();
        }

        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

word_t eval(int p, int q, bool* success);
word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  for (int i = 0; i < nr_token; i ++) {
    if (tokens[i].type == '*' && \
        (i == 0 || (tokens[i - 1].type != TK_OCT &&\
                    tokens[i - 1].type != TK_HEX &&\
                    tokens[i - 1].type != TK_REG &&\
                    tokens[i - 1].type != ')') )) {
      tokens[i].type = TK_DEREF;
    }
    if (tokens[i].type == '-' && \
      (i == 0 || (tokens[i - 1].type != TK_OCT &&\
                  tokens[i - 1].type != TK_HEX &&\
                  tokens[i - 1].type != TK_REG &&\
                  tokens[i - 1].type != ')') )) {
      tokens[i].type = TK_MINUS;
    }
  }
  
  word_t ret = eval(0, nr_token-1, success);
  if(!*success) {
    char tmpE[65536] = {};
    int i = 0, token_pos = 0;
    while(i < nr_token) { 
      strcat(tmpE, tokens[i].str);
      token_pos += i<=ret?strlen(tokens[i].str):0;
      i++;
    };
    strcat(tmpE, "\0");
    printf("invalid expression at token: %d\n%s\n%*.s^\n", ret, tmpE, token_pos, "-");
  }
  /* TODO: Insert codes to evaluate the expression. */
  
  // TODO();
  return ret;
}

bool check_parentheses(int p, int q){
  if (tokens[p].str[0] != '(' || tokens[q].str[0] != ')') return false;
  int stack = 0;
  for(int i = p; i <= q; i++){
    // printf("%s\tstack: %d\n", tokens[i].str, stack);
    switch(tokens[i].str[0]){
      case '(':stack++; break;
      case ')':stack--; if(stack<0)return false;break;
      default:if(stack==0)return false;
    }
  }
  if(stack==0) return true;
  return false;
}

word_t tokenStr2num(int q, bool* success) {
  int rts = 0;
  switch(tokens[q].type){
    case TK_OCT: sscanf(tokens[q].str, "%u", &rts); return rts;
    case TK_HEX: sscanf(tokens[q].str, "%i", &rts); return rts;
    case TK_REG: {
      bool str2val_success;
      word_t val = isa_reg_str2val(tokens[q].str, &str2val_success);
      if(str2val_success) return val;
      else {
          *success = false;
          Log("Invalid reg name: %s at position %d", tokens[q].str, q);
          return q;
        }
    }break;
    default:
        *success = false; 
        Log("String to number failed: %s at position %d", tokens[q].str, q);
        return q; 
  }
  return q; 
}

int dominant_operator(int p, int q, bool* success) {
  int op = -1;
  int lev=0xf;
  int inBarcket=0;
  for(int i=p; i<q; i++){
    int tmpOp = i;
    int tmpLev = 0xf;
    switch(tokens[i].type){
      case TK_OR:tmpOp=i;tmpLev=0;break; 
      case TK_AND:tmpOp=i;tmpLev=1;break;
      case TK_EQ:;
      case TK_NOTEQ:tmpOp=i;tmpLev=2;break;
      case '+':;
      case '-':tmpOp=i;tmpLev=3;break;
      case '*':;  
      case '/':tmpOp=i;tmpLev=4;break;
      case TK_DEREF:;
      case TK_MINUS:tmpOp=i;tmpLev=6;break;
      case '(':inBarcket++;break;
      case ')':inBarcket--;break;
      default: continue;
    }
    if (inBarcket!=0) continue;
    if (tmpLev <= lev){
      op = tmpOp;
      lev = tmpLev;
    }
  }
  if (op == -1){
    Log("No dominant operator at position: %d", p);
    *success = false;
    return p;
  }
  return op;
}

word_t eval(int p, int q, bool* success) {
  if (!*success) return p; 
  if (p > q) {
    /* Bad expression */
    Log("Bad expression: p: %d\t q:%d", p, q);
    *success = false;
    return p;
  }
  else if (p == q) {
    /* Single token.
     * For now this token should be a number.
     * Return the value of the number.
     */
    word_t ret = tokenStr2num(q, success);
    if(*success) return ret;
    else return q;
  }
  else if (check_parentheses(p, q) == true) {
    /* The expression is surrounded by a matched pair of parentheses.
     * If that is the case, just throw away the parentheses.
     */
    return eval(p + 1, q - 1, success);
  }
  else {
    int op = dominant_operator(p, q, success);//the position of 主运算符 in the token expression;
    if (!*success) return op; 

    word_t val2 = 0;

    switch(tokens[op].type) {
      case TK_DEREF:
        val2 = eval(op + 1, q, success);
        if (!*success) return val2; 
        return vaddr_read(val2, 4);
        break;
      case TK_MINUS:
        val2 = - eval(op + 1, q, success);
        return val2;
        break;
      default: val2 = eval(op + 1, q, success);break;
    }
    if (!*success) return val2; 

    word_t val1 = eval(p, op - 1, success);
    if (!*success) return val1; 

    switch (tokens[op].type) {
      case TK_MINUS: return val2;break;
      case TK_DEREF: return val2;break;
      case '+': return val1 + val2;break;
      case '-': return val1 - val2;break;
      case '*': return val1 * val2;break;
      case '/': return val1 / val2;break;
      case TK_EQ: return val1 == val2;break;  
      case TK_NOTEQ: return val1 != val2;break;
      case TK_AND: return val1 && val2;break;
      case TK_OR: return val1 || val2;break;
      default:
        *success = false;
        Log("No this op %s at position %d", tokens[op].str, op);
        return op; 
        break;
    }
  }
  return 0;
}
