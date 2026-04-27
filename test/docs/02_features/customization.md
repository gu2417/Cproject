# 유저 커스터마이징 (FR-C01 ~ C07)

모든 설정은 `user_settings` 테이블 또는 `users`/`room_members` 컬럼에 영속.
**클라이언트 전용** 렌더링 설정(색상·테마)과 **서버가 알아야 하는** 설정(dnd, status, open_nick)을 구분한다.

## FR-C01 내 메시지 색상 · P2

| 값 | `red | green | yellow | blue | magenta | cyan | white` |
| 저장 | `user_settings.msg_color` |
| 효과 | **송신 시점** 이 아니라 **수신자가 내 메시지로 식별될 때** 색상 적용. 즉 브로드캐스트 content 는 평문이고, 각 클라이언트는 자기 `msg_color` 로 자기 메시지만 색칠 |

## FR-C02 닉네임 색상 · P2

| 저장 | `user_settings.nick_color` |
| 주의 | 이 값은 **본인 클라이언트에서만** 자기 닉을 렌더할 때 사용. 타 유저 닉 색상은 전용 팔레트 로테이션(해시 → index) |

## FR-C03 테마 · P2

| 값 | `dark | light` |
| 저장 | `user_settings.theme` |
| 효과 | 배경·박스 라인 색 프리셋 |

## FR-C04 타임스탬프 형식 · P2

| 값 | 0=`HH:MM`, 1=`HH:MM:SS`, 2=`MM-DD HH:MM` |
| 저장 | `user_settings.ts_format` |

## FR-C05 상태메시지 · P2

| 저장 | `users.status_msg` |
| 패킷 | `PROFILE_UPDATE` |

## FR-C06 온라인 상태 · P2

| 값 | `online | busy | invisible` |
| 저장 | `users.online_status` (0/1/2). `invisible` 은 **서버는 0 으로 응답** (설계 결정, glossary 참조) |
| 패킷 | `STATUS_CHANGE` |

## FR-C07 오픈채팅 닉네임 · P3

| 저장 | `room_members.open_nick` |
| 패킷 | `ROOM_SET_OPEN_NICK` |
| 적용 | 해당 방 메시지 브로드캐스트 시 우선 사용 |

## 기본값

```text
msg_color=cyan, nick_color=yellow, theme=dark, ts_format=0, dnd=0, online_status=1
```

## 관련 문서

- 테이블: [`user_settings.md`](../07_database/tables/user_settings.md)
- 패킷: [`08_api/packets/settings.md`](../08_api/packets/settings.md)
