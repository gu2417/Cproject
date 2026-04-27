# 02 · Features

`requirements.md` §3 의 FR 전체를 카테고리별로 상세 해석한다.
각 기능은 **행위 / 입력 / 출력 / 관련 패킷 / 관련 테이블 / 예외 / 우선순위** 를 포함한다.

## 우선순위 매핑 (요약)

| 우선순위 | 포함 FR |
|----------|--------|
| **P0** | FR-A01, A02, A03 · FR-G01, G03, G05, G09 · FR-O01, O02, O04 |
| **P1** | FR-A04~A06 · FR-F01~F07 · FR-D01~D05 · FR-P01~P06 |
| **P2** | FR-M01, M02, M03, M09 · FR-G06~G08, G10 · FR-C01~C07 · FR-N01, N02 |
| **P3** | FR-M04, M05, M06, M08, M10, M11 · FR-O03, O05 · FR-N03~N06 |
| **Out-of-Scope** | FR-ADM01~ADM05 · FR-M07(리액션) |

## 문서

| 문서 | FR 범위 |
|------|--------|
| [account.md](./account.md) | FR-A01 ~ A06 |
| [friend.md](./friend.md) | FR-F01 ~ F07 |
| [dm.md](./dm.md) | FR-D01 ~ D05 |
| [group_room.md](./group_room.md) | FR-G01 ~ G10 |
| [open_room.md](./open_room.md) | FR-O01 ~ O05 |
| [message.md](./message.md) | FR-M01 ~ M11 |
| [notification.md](./notification.md) | FR-N01 ~ N06 |
| [customization.md](./customization.md) | FR-C01 ~ C07 |
| [mypage.md](./mypage.md) | FR-P01 ~ P06 |
| [admin.md](./admin.md) | FR-ADM01 ~ ADM05 *(범위 외)* |

## 기술 규약 (기능 문서 공통)

- **패킷 링크**는 `08_api/packets/<category>.md` 를 가리킴.
- **테이블 링크**는 `07_database/tables/<name>.md` 를 가리킴.
- 모든 입력은 `05_security/input_validation.md` 규칙을 통과해야 함.
