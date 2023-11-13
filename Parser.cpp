#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <cctype>
#include <vector>

//token의 type들
enum TokenType{
    IDENT, CONST, ASSIGN_OP, ADD_OP, MULT_OP, SEMI_COLON, LEFT_PAREN, RIGHT_PAREN, END_OF_FILE, ERROR
};

//token 
struct Token{
    TokenType type;
    std::string value;
};

//global variables
Token next_token;
std::string token_string;
std::ifstream input_file;
int id_count,const_count,op_count;
bool error_flag, warning_flag;

//함수 
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

//symbol table
std::map<std::string, int> symbol_table;

//식별자인지 확인
bool is_ident(const std::string& token){
    if(token.empty()) return false;

    if(!std::isalpha(token[0] != '_'))return false;

    for(size_t i =1; i <token.length();++i){
        if(!std::isalnum(token[i])&&token[i] != '_') return false;
    }
    return true;
}
//상수(const)인지 확인
bool is_const(const std::strinㄹㄹg& token){
    if(token.empty()) return false;

    for(char ch : token){
        if(!std::isdigit(ch)) return false;
    }
    return true;
}

void lexical() {
    char ch;
    token_string.clear();

    //아스키 코드 32이하는 무시
    while(std::isspace(input_file.peek())) input_file.get();

    //파일 끝에 오면 끝내기
    if(input_file.eof()){
        next_token = {TokenType::END_OF_FILE, ""};
        return;
    }

    ch = input_file.get();

    //identifiers인지 확인하기
    if(std::isalpha(ch) || ch == '_'){
        token_string += ch;
        while(std::isalnum(input_file.peek()) || input_file.peek() == '_'){
            token_string += input_file.get();
        }
        next_token = {IDENT, token_string};
    }
    //constants인지 확인
    else if(std::isdigit(ch)){
        token_string += ch;
        while(std::isdigit(input_file.peek())){
            token_string += input_file.get();
        }
        next_token = {CONST, token_string};
    }
    //assignment OP인지 확인
    else if (ch == ':') {
        if (input_file.peek() == '=') {
            input_file.get();
            next_token = {ASSIGN_OP, ":="};
        } else {
            next_token = {ERROR, ":"};
        }
    }
    //나머지들인지 확인
    else{
        switch (ch)
        {
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
    //한 프로그램은 statements로 구성되어 있음
    statements();
    // 다 확인후 endoffile이 아니면 에러
    if (next_token.type != END_OF_FILE) {
        std::cerr << "Parsing error: Extra tokens after program end." << std::endl;
    }
}

void statements() {
    do {
        statement(); // 현재 토큰을 기반으로 statement 처리
        if (next_token.type == SEMI_COLON) {
            lexical(); // 세미콜론을 소비하고 다음 토큰으로 이동
        } else if (next_token.type != END_OF_FILE) {
            std::cerr << "Parsing error: Expected ';' after statement." << std::endl;
            error_flag = true;
            return; // 에러 발생 시 함수 종료
        }
    } while (next_token.type != END_OF_FILE && !error_flag); // 파일의 끝이거나 에러가 발생할 때까지 반복
}
void statement() {
    //count변수들 초기화
    id_count = const_count = op_count = 0;
    //에러,경고문 플레그 초기화
    error_flag = warning_flag = false;

    if(next_token.type == IDENT){
        id_count++;
        std::string ident = next_token.value; //다시 쓰기위함
        lexical(); //다음 토큰 이동 
        if(next_token.type == ASSIGN_OP){
            lexical();
            int value = evaluate_expression();
            if(!error_flag){
                symbol_table[ident] = value;
            }
            std::cout << ident << " := ";
            if (error_flag) {
                std::cout << "Unknown; "; //예외 출력
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
        } else{
            std::cerr << "Parsing error: Expected assignment operator after identifier." << std::endl;
            error_flag = true;
        }
    } else{
        std::cerr << "Parsing error: Expected identifier at the beginning of the statement." << std::endl;
        error_flag = true;
    }

}
//<expression> → <term><term_tail>
void expression() {
    term();

    term_tail();
}

//<term_tail> → <add_op><term><term_tail> | ε
void term_tail() {
    if (next_token.type == ADD_OP) {
        lexical(); //<add_op>해석
        term(); 
        term_tail(); 
    }
}

//term -> <factor><factor_tail>
void term() {
    factor();
    factor_tail();
}

// factor_tail → <mult_op><factor><factor_tail> | ε
void factor_tail() {
    if (next_token.type == MULT_OP) {
        lexical(); 
        factor(); 
        factor_tail(); 
    }
}

//factor → <left_paren><expression><right_paren> | <ident> | <const>
void factor(){
    if (next_token.type == LEFT_PAREN) {
        lexical(); 
        expression(); 
        if (next_token.type == RIGHT_PAREN) {
            lexical(); 
        } else {
            std::cerr << "Parsing error: Expected ')' after expression." << std::endl;
            
        }
    } else if (next_token.type == IDENT) {
        // counter는 한번만 세기
        if (symbol_table.find(next_token.value) == symbol_table.end()) {
            id_count++;
        }
        
        symbol_table[next_token.value] = 0;
        lexical(); 
    } else if (next_token.type == CONST) {
        const_count++; 
        lexical(); 
    } else {
        std::cerr << "Parsing error: Expected expression." << std::endl;
    }
}

int evaluate_expression(){
    int value = evaluate_term();
    while (next_token.type == ADD_OP) {
        op_count++; // Increment operator count
        std::string op = next_token.value;
        lexical();
        if (next_token.type == ADD_OP || next_token.type == MULT_OP) {
            // 연속된 연산자 발견, 경고 표시
            std::cout << "(Warning) 연속된 연산자 발견: " << op << next_token.value << std::endl;
            warning_flag = true; // 경고 플래그 설정
            // 첫 번째 연산자만 처리하고 두 번째 연산자는 무시
            lexical(); // 두 번째 연산자를 소비
        }
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
        lexical(); 
        value = evaluate_expression(); 
        if (next_token.type == RIGHT_PAREN) {
            lexical(); 
        } else {
            std::cerr << "Parsing error: Expected ')' after expression." << std::endl;
            error_flag = true;
        }
    } else if (next_token.type == IDENT) {
        if (symbol_table.find(next_token.value) == symbol_table.end()) {
            std::cerr << "Error: Undefined variable " << next_token.value << std::endl;
            error_flag = true;
            value = 0; 
        } else {
            value = symbol_table[next_token.value];
        }
        id_count++; 
        lexical(); 
    } else if (next_token.type == CONST) {
        value = std::stoi(next_token.value);
        const_count++; 
        lexical(); 
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

    lexical();

    program();

    input_file.close();

    if (!error_flag) {
        std::cout << "Result ==> ";
        for (const auto& pair : symbol_table) {
            std::cout << pair.first << ": " << (pair.second == 0 && error_flag ? "Unknown" : std::to_string(pair.second)) << "; ";
        }
        std::cout << std::endl;
    }

    return 0;
}
