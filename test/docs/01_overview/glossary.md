# 용어집

| 용어 | 정의 |
|------|------|
| **Session (세션)** | 접속 중인 클라이언트 하나에 대한 서버측 인메모리 상태. `ClientSession` 구조체로 표현. DB 에 저장되지 않음. |
| **Room (채팅방)** | 여러 사용자가 메시지를 주고받는 단위. 유형은 `group`(초대형) / `open`(공개형) 두 가지. |
| **DM** | Direct Message. 1:1 대화. `messages.room_id = NULL` 로 저장. |
| **Whisper (귓속말)** | 방 안에서 특정 사용자에게만 전달되는 메시지. DB 저장은 `msg_type=2`. |
| **Broadcast** | 서버가 방 멤버 전체(혹은 서버 전체)에게 동일 패킷을 fan-out 하는 행위. |
| **Router** | 수신된 패킷의 `TYPE` 필드를 기준으로 기능별 핸들러로 디스패치하는 서버 계층. |
| **Handler Thread** | 클라이언트 한 명당 하나씩 생성되는 서버 스레드. 소켓 read 루프 담당. |
| **Send / Recv Thread** | 클라이언트의 송신·수신 분리 스레드 쌍. 소켓↔GTK4 UI 를 비동기로 연결. |
| **Pin (핀 메시지)** | 방 상단에 고정되는 중요 메시지. `rooms.pinned_msg_id` 로 참조. |
| **Notice (공지)** | 방 입장 시 상단에 표시되는 공지 문자열. `rooms.notice`. |
| **DND** | Do Not Disturb. 멘션을 제외한 알림을 무음 처리하는 상태. |
| **Open Nick** | 오픈채팅방 내에서만 노출되는 별칭. `room_members.open_nick`. |
| **Prepared Statement** | 플레이스홀더 기반 SQL 실행. 본 프로젝트의 모든 SQL 은 이를 사용(injection 방지). |
| **Thread-local DB Connection** | 각 핸들러 스레드 전용 MYSQL* 연결. pool 미사용. |
| **FR / NFR** | Functional / Non-Functional Requirement. ID 는 `requirements.md` 기준. |
| **GtkApplication** | GTK4 앱 생명주기 관리 객체. `g_application_run()` 호출로 메인 루프 시작. |
| **GtkStack** | 여러 자식 위젯 중 하나만 표시하는 컨테이너. 화면 전환(LOGIN→MAIN→CHAT 등)에 사용. |
| **g_idle_add()** | GTK 메인 루프가 유휴 상태일 때 콜백 실행을 예약하는 함수. recv 스레드에서 UI 업데이트 시 사용. |
| **GObject Signal** | GTK4 의 이벤트 처리 메커니즘. `g_signal_connect()` 로 콜백 연결. |
| **GtkListBox** | 스크롤 가능한 세로 목록 위젯. 친구/방 목록 표시에 사용. |
| **GtkPopoverMenu** | 버튼 클릭 또는 우클릭 시 나타나는 팝업 메뉴 위젯. |
| **GtkRevealer** | 내용을 부드럽게 슬라이드-인/아웃하는 컨테이너. 알림 배너에 사용. |
| **screen_*** | 화면별 GTK4 위젯을 생성·관리하는 클라이언트 모듈군 (screen_login, screen_main, screen_chat 등). |
