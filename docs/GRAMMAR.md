# 이음 문법

## 범위

이 문서는 F1에서 지원하는 구조 선언과 모듈 본문 문법을 정의합니다. F1은 AST 생성까지만 담당하며 이름 해석, Scope, 함수 호출 검증과 실행 의미는 F2 범위입니다.

## EBNF

```text
program        := { NEWLINE | moduleDecl lineEnd | layerDecl lineEnd }

moduleDecl     := MODULE IDENTIFIER [ DEPENDS identifierList ] [ moduleBody ]
moduleBody     := LEFT_BRACE RIGHT_BRACE
                | LEFT_BRACE NEWLINE { NEWLINE | moduleMember NEWLINE } RIGHT_BRACE
moduleMember   := variableDecl | functionDecl

variableDecl   := LET IDENTIFIER
functionDecl   := FN IDENTIFIER LEFT_PAREN [ identifierList ] RIGHT_PAREN functionBody
functionBody   := LEFT_BRACE RIGHT_BRACE
                | LEFT_BRACE NEWLINE { NEWLINE | statement NEWLINE } RIGHT_BRACE
statement      := variableDecl | callStatement
callStatement  := CALL IDENTIFIER LEFT_PAREN [ identifierList ] RIGHT_PAREN

layerDecl      := LAYER IDENTIFIER ABOVE IDENTIFIER
identifierList := IDENTIFIER { COMMA IDENTIFIER }
lineEnd        := NEWLINE | END
```

## 예제

```text
module data {
  fn save(item) {}
}

module service depends data {
  let cache

  fn handle(request) {
    let prepared
    call save(prepared)
  }
}

layer service above data
```

## 줄과 본문 규칙

- 비어 있지 않은 `{` 본문은 여는 중괄호 다음 줄에서 시작합니다.
- `let`, `fn`, `call`은 각각 한 줄에 하나만 작성합니다.
- 비어 있는 모듈과 함수 본문은 `{}`로 작성할 수 있습니다.
- 함수 안에 함수를 선언할 수 없습니다.
- 모듈 바로 아래에는 `let`과 `fn`만 올 수 있습니다.
- 함수 본문에는 `let`과 `call`만 올 수 있습니다.
- `#`부터 줄 끝까지는 주석입니다.

## 식별자와 예약어

- 식별자 형식은 `[A-Za-z_][A-Za-z0-9_]*`입니다.
- `module`, `depends`, `layer`, `above`, `fn`, `let`, `call`은 예약어입니다.
- 함수 매개변수와 호출 인자는 현재 식별자만 지원합니다.

## F1에서 의도적으로 제외한 항목

- 숫자·문자열·불리언 리터럴
- 대입과 초기화 식
- 산술·비교·논리 연산
- 반환문, 조건문과 반복문
- 중복 변수·함수, 미정의 호출과 인자 수 검사
- 실제 함수 호출 실행

마지막 두 항목은 F2의 이름 해석과 최소 인터프리터에서 다룹니다.
