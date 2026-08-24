# 이음 문법

## 범위

이 문서는 F2에서 지원하는 구조 선언, 모듈 본문 문법과 최소 실행 의미를 정의합니다.

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

## 이름과 Scope 규칙

- 모듈 변수끼리, 함수끼리는 각 모듈의 이름 공간 안에서 중복될 수 없습니다.
- 매개변수와 지역 변수는 함수 Scope를 공유하며 같은 이름을 중복 선언할 수 없습니다.
- 지역 변수는 선언 다음 문장부터 사용할 수 있습니다.
- 호출 인자는 함수의 매개변수, 앞서 선언한 지역 변수 또는 현재 모듈 변수여야 합니다.
- 지역 변수와 매개변수는 같은 이름의 모듈 변수를 가릴 수 있습니다.
- 함수 호출은 현재 모듈 함수를 먼저 찾고, 없으면 직접 `depends`로 선언한 모듈에서 찾습니다.
- 호출 대상이 다른 모듈에만 있으면 `depends` 누락이며, 여러 직접 의존 모듈에 있으면 모호한 호출입니다.
- 호출 인자 수는 대상 함수의 매개변수 수와 같아야 합니다.
- 종료 조건을 표현할 수 없는 현재 문법에서는 직접·간접 재귀 호출을 거부합니다.

## 최소 실행 의미

- 모든 변수와 인자는 값이 없는 unit으로 취급합니다.
- `let`은 이름을 Scope에 추가하며 런타임 동작은 없습니다.
- `call`은 의미 분석에서 결정한 함수를 동기적으로 실행합니다.
- CLI의 `--run <모듈>.<함수>`로 진입 함수를 지정하며 진입 함수는 매개변수가 없어야 합니다.
- 실행 결과는 함수 진입, 호출, 종료 순서와 실행 횟수를 Trace로 출력합니다.

## F2에서 의도적으로 제외한 항목

- 숫자·문자열·불리언 리터럴
- 대입과 초기화 식
- 산술·비교·논리 연산
- 반환문, 조건문과 반복문
- 사용자 정의 값과 함수 반환값
