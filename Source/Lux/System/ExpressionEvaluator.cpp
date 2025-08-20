#include "System/ExpressionEvaluator.h"
#include <iostream>
#include <sstream>
#include <cmath>
#include <cctype>
#include <cstring>

ExpressionEvaluator::ExpressionEvaluator()
{
}

ExpressionEvaluator::~ExpressionEvaluator()
{
}

/**
 * @brief   이 함수는 피연산자 스택에서 두 개의 값을 꺼내어 주어진 연산자 `op`를 적용하여 결과를 계산합니다.
 *          계산 결과는 다시 피연산자 스택에 푸시됩니다. 덧셈, 뺄셈, 곱셈, 나눗셈, 지수연산을 지원하며,
 *          나눗셈 연산 시 0으로 나누는 경우가 발생하면 `false`를 반환하여 오류를 처리합니다.
 *          성공적으로 연산이 완료되면 `true`를 반환합니다.
 *
 * @param   values 피연산자를 저장하는 스택.
 * @param   op 적용할 연산자.
 * @return  연산이 성공하면 true, 오류가 발생하면 false를 반환합니다.
 */
bool ExpressionEvaluator::ApplyOperator(std::stack<double>& values, char op) {
    if (values.size() < 2) return false;  // 피연산자가 부족한지 확인

    double right = values.top(); values.pop();
    double left = values.top(); values.pop();

    switch (op) {
    case '+': values.push(left + right); break;
    case '-': values.push(left - right); break;
    case '*': values.push(left * right); break;
    case '/':
        if (right == 0) return false;  // 0으로 나누기 오류 처리
        values.push(left / right);
        break;
    case '^': values.push(std::pow(left, right)); break;
    default: return false;  // 알 수 없는 연산자
    }
    return true;
}

/**
 * @brief   주어진 연산자의 우선순위를 반환합니다.
 *          덧셈과 뺄셈은 우선순위 1, 곱셈과 나눗셈은 2, 지수연산은 3을 반환합니다.
 *          알 수 없는 연산자가 입력되면 0을 반환합니다.
 *
 * @param   op 우선순위를 확인할 연산자.
 * @return  해당 연산자의 우선순위를 나타내는 정수 값.
 */
int ExpressionEvaluator::Precedence(char op) const {
    if (op == '+' || op == '-') return 1;
    if (op == '*' || op == '/') return 2;
    if (op == '^') return 3;
    return 0;  // 알 수 없는 연산자의 경우
}

/**
 * @brief   수식 문자열을 토큰 스트림으로 변환합니다.
 *          각 토큰 스트림은 숫자 또는 각 연산자를 담습니다.
 *          유효한 표현식이면 `true`, 그렇지 않으면 `false`를 반환합니다.
 *
 * @param   expression 입력된 수식 문자열.
 * @param   tokens 토큰화된 결과를 저장할 스트림.
 * @return  유효한 표현식이면 true, 아니면 false를 반환합니다.
 */
bool ExpressionEvaluator::Tokenize(const std::string& expression, std::istringstream& tokens) {
    tokens.str(expression);
    return !expression.empty();
}

/**
 * @brief   연산을 처리하는 각 토큰(숫자, 괄호, 연산자, 변수 등)을 처리합니다.
 *          숫자는 피연산자 스택에, 연산자는 연산자 스택에 저장되며, 괄호는 우선순위를 위해 연산을 처리합니다.
 *          변수는 주어진 값을 찾아 필요에 따라 치환하여 처리합니다.
 *          성공적으로 처리되면 `true`, 오류가 발생하면 `false`를 반환합니다.
 *
 * @param   tokens 토큰화된 수식 스트림.
 * @param   values 피연산자를 저장하는 스택.
 * @param   operators 연산자를 저장하는 스택.
 * @return  처리가 성공하면 true, 오류 발생 시 false를 반환합니다.
 */
bool ExpressionEvaluator::ProcessToken(std::istringstream& tokens, std::stack<double>& values, std::stack<char>& operators)
{
    char token;

    // 토큰을 읽는데 실패한 경우 false를 반환하여 루프를 종료
    if (!(tokens >> token))
    {
        return false;
    }

    // 숫자 처리: 토큰을 숫자로 변환하여 스택에 저장
    if (std::isdigit(token) || token == '.')
    {
        tokens.putback(token);
        double value;
        tokens >> value;
        values.push(value);
    }

    // 여는 괄호를 스택에 저장
    else if (token == '(')
    {
        operators.push(token);
    }

    // 닫는 괄호는 여는 괄호까지 모든 연산자를 처리
    else if (token == ')')
    {
        while (!operators.empty() && operators.top() != '(')
        {
            if (!ApplyOperator(values, operators.top())) return false;
            operators.pop();
        }

        if (operators.empty()) return false;  // 괄호 불일치 오류
        operators.pop();  // 여는 괄호 제거
    }

    // 연산자 처리: 우선순위를 고려 연산을 수행
    else if (std::strchr("+-*/^", token))
    {
        while (!operators.empty() && Precedence(operators.top()) >= Precedence(token))
        {
            if (!ApplyOperator(values, operators.top())) return false;
            operators.pop();
        }
        operators.push(token);
    }
    else
    {
        return false; // 유효하지 않은 토큰이면 false 반환
    }

    return true;
}

/**
 * @brief   연산을 평가하는 메인 함수입니다.
 *          수식 문자열을 토큰으로 변환한 후, 각 토큰을 처리하여 피연산자와 연산자를 스택에 저장합니다.
 *          연산자가 모두 처리되면 최종 결과가 피연산자 스택에 남으며, 그 값을 result 변수에 저장합니다.
 *          수식 계산에서 오류가 발생하면 `false`, 성공 시 `true`를 반환합니다.
 *
 * @param   expression 수식 문자열.
 * @param   result 계산 결과를 저장할 참조 변수.
 * @return  계산에 성공하면 true, 실패하면 false를 반환합니다.
 */
bool ExpressionEvaluator::Evaluate(const std::string& expression, double& result) {
    std::istringstream tokens;
    if (!Tokenize(expression, tokens)) return false;

    std::stack<double> values;
    std::stack<char> operators;

    // 남아있는 토큰 순차적 처리
    while (ProcessToken(tokens, values, operators));

    // 남은 연산자 처리
    while (!operators.empty()) 
    {
        if (!ApplyOperator(values, operators.top())) return false;
        operators.pop();
    }

    if (values.size() != 1) return false;  // 결과가 하나 값이 아니면 오류
    result = values.top();

    return true;
}
