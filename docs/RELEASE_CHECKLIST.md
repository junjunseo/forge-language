# v0.1.0 PoC 릴리스 체크리스트

검증 완료일: 2026-08-24

- 기준 커밋: `1d29dc7`
- CI 근거: [GitHub Actions run 31395296992](https://github.com/junjunseo/ieum/actions/runs/31395296992)
- 릴리스 근거: [이음 v0.1.0 - PoC 릴리스](https://github.com/junjunseo/ieum/releases/tag/v0.1.0)
- 로컬 재검증: `scripts/test.ps1`로 67개 assert, 버전, 예제 6종 확인

## 1. 기준선 확인

- [x] 릴리스 후보 브랜치가 최신 `main`에서 시작했는지 확인
- [x] `VERSION`의 값이 `0.1.0`인지 확인
- [x] `ieum --version`이 `ieum 0.1.0`을 출력하는지 확인
- [x] 작업 트리에 의도하지 않은 파일이 없는지 확인

## 2. 새 환경 재현

필요 도구는 CMake 3.16 이상과 C++17 컴파일러입니다.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

- [x] README의 명령만 사용해 configure, build, test를 완료
- [x] 테스트 10종(Parser, Pipeline, Checker, 버전, 예제 6종)이 모두 통과

Windows에서 `g++`를 사용할 수 있으면 다음 경로도 확인합니다.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\test.ps1
```

- [x] PowerShell 테스트가 버전, 67개 assert, 예제 6종을 모두 검증

## 3. 예제 검증

| 예제 | 예상 종료 코드 | 대표 진단 |
|---|---:|---|
| `valid.ieum` | 0 | `구조 검사 통과` |
| `implicit_dependency.ieum` | 1 | `[암묵적 의존]` |
| `cyclic_dependency.ieum` | 1 | `[순환 의존]` |
| `layer_violation.ieum` | 1 | `[계층 위반]` |
| `transitive_layer_violation.ieum` | 1 | `[계층 위반]` |
| `invalid_declarations.ieum` | 1 | `[중복 모듈]` |

- [x] 여섯 예제의 종료 코드와 대표 진단이 표와 일치

## 4. CI와 문서

- [x] GitHub Actions의 Ubuntu 작업 통과
- [x] GitHub Actions의 Windows 작업 통과
- [x] README의 버전, 빌드, 테스트, 시연 흐름이 실제 동작과 일치
- [x] `CHANGELOG.md`의 기능과 제한 사항이 구현 상태와 일치

## 5. 게시

- [x] PR 리뷰 및 병합 완료
- [x] `main`에서 전체 테스트 재확인
- [x] `v0.1.0` annotated tag 생성
- [x] 태그와 `CHANGELOG.md`를 기준으로 GitHub Release 작성
