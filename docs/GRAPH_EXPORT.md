# 의존 그래프 DOT 출력

## 목적

검사기가 판단한 모듈 의존 관계와 계층 관계를 같은 근거로 시각화합니다. DOT 파일 생성에는 외부 도구가 필요하지 않으며, 이미지 렌더링에만 선택적으로 Graphviz를 사용합니다.

## 사용법

```powershell
.\build\ieum.exe .\examples\valid.ieum --emit-dot .\build\valid.dot
.\build\ieum.exe .\examples\execution.ieum --run service.main --emit-dot .\build\execution.dot
```

`--run`과 `--emit-dot`의 순서는 바꿀 수 있습니다. 구조 위반이 있는 입력도 그래프를 먼저 저장하고 기존과 동일하게 종료 코드 `1`을 반환합니다. 출력 파일을 열 수 없는 경우 `graph_export=failed` 경고를 남기되 구조 검사 성공은 `0`, 구조 위반은 `1`로 유지합니다.

## 표현 규칙

| 대상 | DOT 표현 |
|---|---|
| 선언된 모듈 | 둥근 사각형 노드 |
| 정상 `depends` | 파란 실선 화살표, 의존하는 모듈에서 의존 대상으로 향함 |
| `layer U above L` | `U`에서 `L`로 향하는 회색 점선 빈 화살표 |
| 순환·계층 위반 경로 | 빨간 굵은 의존 화살표 |
| 미선언 참조 | `(undefined)`가 붙은 빨간 점선 노드 |
| 중복 모듈 | `(duplicate)`가 붙은 빨간 노드 |
| 잘못된 계층 선언 | 빨간 굵은 점선 화살표 |

노드, 의존 간선, 계층 간선은 각각 이름순으로 정렬됩니다. 중복 모듈의 의존 관계는 검사기와 마찬가지로 첫 선언을 기준으로 생성합니다.

## 위반 경로 일치

검사 결과의 `Violation::path`를 텍스트 진단과 그래프 강조가 함께 사용합니다. 순환 경로는 시작 모듈을 마지막에 한 번 더 포함하며, 간접 계층 위반은 실제 의존 경로 전체를 진단에 출력합니다.

예를 들어 `data -> helper -> ui` 경로가 계층을 역행하면 진단과 DOT 모두 `data -> helper`, `helper -> ui` 두 간선을 같은 위반 경로로 표시합니다.

## 스냅샷 검증

`valid`, `implicit_dependency`, `cyclic_dependency`, `layer_violation`, `transitive_layer_violation`, `invalid_declarations` 예제의 기준 DOT은 `test/snapshots/`에 저장합니다. 전체 테스트는 새 출력과 기준 파일을 바이트 단위로 비교하며, 그래프 저장 실패가 검사 종료 코드를 바꾸지 않는지도 확인합니다.
