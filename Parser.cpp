#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <cctype>
#include <vector>

//token type
enum TokenType{
    IDENT, CONST, ASSIGN_OP, ADD_OP, MULT_OP, SEMI_COLON, LEFT_PAREN, RIGHT_PAREN, END_OF_FILE, ERROR
};

//token 
struct Token{
    TokenType type;
    std::string value;
};

// 전역 변수들
Token next_token;
std::string token_string;
std::ifstream input_file;
int id_count, const_count, op_count;
bool error_flag, warning_flag;


// 함수 선언
void lexical();
void program();
void statements();
void statement();
void expression();
void term();
void term_tail();
void factor();
void factor_tail();
int evaluate_expression();
int evaluate_term();
int evaluate_factor();


// Symbol table
std::map<std::string, int> symbol_table;

bool is_ident(const std::string& token) {
    if(token.empty()) return false;
    
    //첫번째가 letter이거나 _인지 확인
    if(!std::isalpha(token[0]) && token[0] != '_') return false;

    //나머지들 확인
    for(size_t i = 1;i < token.length();++i){
        if(!std::isalnum(token[i])&&token[i] != '_') return false;
    }
    return true;
}

bool is_const(const std::string& token) {
    if (token.empty()) return false;

    // 각 character가 digit인지 확인
    for (char ch : token) {
        if (!std::isdigit(ch)) return false;
    }

    return true;
  return true;
}

void lexical() {
    char ch;
    token_string.clear();

    // 공백 건너뛰기
    while (std::isspace(input_file.peek())) input_file.get();

    if (input_file.eof()) {
        next_token = {TokenType::END_OF_FILE, ""};
        return;
    }

    ch = input_file.get();

    // indentifiers, keyword 확인하기
    if (std::isalpha(ch) || ch == '_') {
        token_string += ch;
        while (std::isalnum(input_file.peek()) || input_file.peek() == '_') {
            token_string += input_file.get();
        }
        next_token = {IDENT, token_string};
    }
    // constansts 확인
    else if (std::isdigit(ch)) {
        token_string += ch;
        while (std::isdigit(input_file.peek())) {
            token_string += input_file.get();
        }
        next_token = {CONST, token_string};
    }
    // assignment op인지 확인
    else if (ch == ':') {
        if (input_file.peek() == '=') {
            input_file.get();
            next_token = {ASSIGN_OP, ":="};
        } else {
            next_token = {ERROR, ":"};
        }
    }
    // 나머지들 확인
    else {
        switch (ch) {
            case '+': next_token = {ADD_OP, "+"}; break;
            case '-': next_token = {ADD_OP, "-"}; break;
            case '*': next_token = {MULT_OP, "*"}; break;
            case '/': next_token = {MULT_OP, "/"}; break;
            case ';': next_token = {SEMI_COLON, ";"}; break;
            case '(': next_token = {LEFT_PAREN, "("}; break;
            case ')': next_token = {RIGHT_PAREN, ")"}; break;
            default: next_token = {ERROR, std::string(1, ch)}; break;
        }
    }
}

void program() {
    //a <program> consists of <statements>
    statements();

    // 다 확인후 endoffile
    if (next_token.type != END_OF_FILE) {
        std::cerr << "Parsing error: Extra tokens after program end." << std::endl;
    }
}

void statements() {
    // expect a statement
    statement();

    while (next_token.type == SEMI_COLON) {
        // Consume the semi_colon
        lexical();

        // Parse the next statement
        statement();
    }

    if (next_token.type != SEMI_COLON && next_token.type != END_OF_FILE) {
        std::cerr << "Parsing error: Expected ';' or end of file." << std::endl;
    }
}

void statement() {
    id_count = const_count = op_count = 0; // Reset counts for each statement
    error_flag = warning_flag = false; // Reset flags

    if (next_token.type == IDENT) {
        id_count++; // Increment identifier count
        std::string ident = next_token.value;
        lexical(); // Move to the next token

        if (next_token.type == ASSIGN_OP) {
            lexical(); // Move past the assignment operator
            int value = evaluate_expression(); // Evaluate the expression and get the value
            if (!error_flag) {
                symbol_table[ident] = value; // Assign the value to the identifier in the symbol table
            }
            std::cout << ident << " := ";
            if (error_flag) {
                std::cout << "Unknown; ";
            } else {
                std::cout << value << "; ";
            }
            std::cout << "ID: " << id_count << "; CONST: " << const_count << "; OP: " << op_count << ";" << std::endl;
            if (error_flag) {
                std::cout << "(Error) \"정의되지 않은 변수(" << ident << ")가 참조됨\"" << std::endl;
            } else if (warning_flag) {
                std::cout << "(Warning) \"잘못된 연산자 사용\"" << std::endl;
            } else {
                std::cout << "(OK)" << std::endl;
            }
        } else {
            std::cerr << "Parsing error: Expected assignment operator after identifier." << std::endl;
            error_flag = true;
        }
    } else {
        std::cerr << "Parsing error: Expected identifier at the beginning of the statement." << std::endl;
        error_flag = true;
    }
}


void expression() {
    term();

    term_tail();
}

void term_tail() {
    // term_tail → <add_op><term><term_tail> | ε
    if (next_token.type == ADD_OP) {
        lexical(); // Consume the add operator
        term(); // Parse the next term
        term_tail(); // Look for another term_tail recursively
    }
    // If the next token is not an add_op, it's epsilon (empty), so we do nothing
}

void term() {
    // term → <factor> <factor_tail>
    factor();
    factor_tail();
}

void factor_tail() {
    // factor_tail → <mult_op><factor><factor_tail> | ε
    if (next_token.type == MULT_OP) {
        lexical(); // Consume the mult operator
        factor(); // Parse the next factor
        factor_tail(); // Look for another factor_tail recursively
    }
    // If the next token is not a mult_op, it's epsilon (empty), so we do nothing
}

void factor() {
    // factor → <left_paren><expression><right_paren> | <ident> | <const>
    if (next_token.type == LEFT_PAREN) {
        lexical(); // Consume the left parenthesis
        expression(); // Parse the expression inside the parentheses
        if (next_token.type == RIGHT_PAREN) {
            lexical(); // Consume the right parenthesis
        } else {
            std::cerr << "Parsing error: Expected ')' after expression." << std::endl;
            // Handle the error appropriately
        }
    } else if (next_token.type == IDENT) {
        // Increment identifier count only if it's a new identifier
        if (symbol_table.find(next_token.value) == symbol_table.end()) {
            id_count++;
        }
        // If it's an identifier, look it up in the symbol table or insert it
        symbol_table[next_token.value] = 0;
        lexical(); // Consume the identifier
    } else if (next_token.type == CONST) {
        const_count++; // Increment constant count
        lexical(); // Consume the constant
    } else {
        std::cerr << "Parsing error: Expected expression." << std::endl;
        // Handle the error appropriately
    }
}

int evaluate_expression() {
    int value = evaluate_term();
    while (next_token.type == ADD_OP) {
        op_count++; // Increment operator count
        std::string op = next_token.value;
        lexical();
        if (op == "+") {
            value += evaluate_term();
        } else if (op == "-") {
            value -= evaluate_term();
        } else {
            // Handle other operators or errors
            warning_flag = true; // Set warning flag for incorrect operator
        }
    }
    return value;
}

int evaluate_term() {
    int value = evaluate_factor();
    while (next_token.type == MULT_OP) {
        op_count++; // Increment operator count
        std::string op = next_token.value;
        lexical();
        if (op == "*") {
            value *= evaluate_factor();
        } else if (op == "/") {
            value /= evaluate_factor();
        } else {
            // Handle other operators or errors
            warning_flag = true; // Set warning flag for incorrect operator
        }
    }
    return value;
}



int evaluate_factor() {
    int value = 0;
    if (next_token.type == LEFT_PAREN) {
        lexical(); // Consume the left parenthesis
        value = evaluate_expression(); // Evaluate the expression inside the parentheses
        if (next_token.type == RIGHT_PAREN) {
            lexical(); // Consume the right parenthesis
        } else {
            std::cerr << "Parsing error: Expected ')' after expression." << std::endl;
            error_flag = true;
        }
    } else if (next_token.type == IDENT) {
        if (symbol_table.find(next_token.value) == symbol_table.end()) {
            std::cerr << "Error: Undefined variable " << next_token.value << std::endl;
            error_flag = true;
            value = 0; // Undefined variables are considered as 0
        } else {
            value = symbol_table[next_token.value];
        }
        id_count++; // Increment identifier count
        lexical(); // Consume the identifier
    } else if (next_token.type == CONST) {
        value = std::stoi(next_token.value);
        const_count++; // Increment constant count
        lexical(); // Consume the constant
    } else {
        std::cerr << "Parsing error: Expected '(' or identifier or constant." << std::endl;
        error_flag = true;
    }
    return value;
}



int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <input_file>" << std::endl;
        return 1;
    }

    input_file.open(argv[1]);
    if (!input_file.is_open()) {
        std::cerr << "Error opening file: " << argv[1] << std::endl;
        return 1;
    }

    // Initialize the lexical analyzer
    lexical();

    // Start the parsing process
    program();

    // Close the input file
    input_file.close();

    // Output the symbol table
    if (!error_flag) {
        std::cout << "Result ==> ";
        for (const auto& pair : symbol_table) {
            std::cout << pair.first << ": " << (pair.second == 0 && error_flag ? "Unknown" : std::to_string(pair.second)) << "; ";
        }
        std::cout << std::endl;
    }

    return 0;
}
