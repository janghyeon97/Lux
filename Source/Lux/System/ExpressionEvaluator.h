// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include <string>
#include <unordered_map>
#include <stdexcept>
#include <stack>
#include <sstream>

/**
 * 수식 문자열을 평가하는 클래스
 * 아이템이나 액션 설명에서 사용되는 동적 수치 계산을 위해 사용됩니다.
 */
class LUX_API ExpressionEvaluator
{
public:
	ExpressionEvaluator();
	~ExpressionEvaluator();

public:
    // 수식을 평가하는 메인 함수. 결과가 result에 저장되며, 성공 시 true, 실패 시 false 반환
    bool Evaluate(const std::string& expression, double& result);

private:
    // 수식을 문자열 토큰으로 변환
    bool Tokenize(const std::string& expression, std::istringstream& tokens);

    // 연산자의 우선순위 반환
    int Precedence(char op) const;

    // 연산자를 적용하여 피연산자 스택에서 값을 계산
    bool ApplyOperator(std::stack<double>& values, char op);

    // 수식의 각 토큰을 처리하는 함수
    bool ProcessToken(std::istringstream& tokens, std::stack<double>& values, std::stack<char>& operators);
};
