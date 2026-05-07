---
name: phase-run
description: Run GO/NO-GO checklist for a given phase (0-3) and report failing items.
when_to_use: Use when you want to verify a phase is complete before moving to the next.
---
# Phase Run Skill

실행: `/phase-run <0|1|2|3>`

해당 Phase의 GO/NO-GO 체크리스트를 실행하고 미통과 항목을 리포트한다.
체크리스트는 `.omc/plans/c-chat-implementation-plan.md` §1에 정의되어 있다.
