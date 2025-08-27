## Lux — Unreal Engine 5 액션/효과/태그/타겟팅 프레임워크

게임 클라이언트 개발 포트폴리오 용도의 샘플/레퍼런스 프로젝트입니다. 데이터 주도 액션 시스템과 태그 기반 전투/효과/타겟팅 파이프라인을 제공하며, 캐릭터 “Aurora”의 다양한 액션을 예시로 구현하였습니다.

- **엔진**: Unreal Engine 5.x
- **언어**: C++
- **플랫폼**: Windows (타 플랫폼 확장 가능)
- **모듈**: `Lux`

---

## 목차

- [주요 하이라이트](#주요-하이라이트)
- [아키텍처 설계 철학](#아키텍처-설계-철학)
- [입력 가이드](#입력-가이드)
- [샘플 액션 쇼케이스 (GIF)](#샘플-액션-쇼케이스-gif)
- [기술 스택/핵심 시스템](#기술-스택핵심-시스템)
- [데이터 주도 워크플로우](#데이터-주도-워크플로우)
- [네트워킹/예측](#네트워킹예측)
- [프로젝트 구조](#프로젝트-구조)
- [빌드/실행 방법](#빌드실행-방법)
- [빠른 시작: 새 액션 추가](#빠른-시작-새-액션-추가)
- [디버그/툴링](#디버그툴링)
- [라이선스/문의](#라이선스문의)

---

## 주요 하이라이트

- **액션 상태머신**: `ULuxAction` + `Phase` 전환 규칙으로 Begin/Execute/Recovery/End/Interrupt, Dash/Leap/Landing 등을 선언적으로 구성
- **태스크 조립식 설계**: `ULuxActionTask_PlayMontageAndWait`, `..._LeapToLocation`, `..._FollowSpline`, `..._LandingControl`, `..._WaitForServer` 등 비동기 유닛을 조합
- **태그 지향 전투/효과**: `ULuxCombatManager`, `ULuxEffect`, `FLuxEffectSpec` + ByCaller 파라미터로 데미지/버프/CC 데이터-드리븐 처리
- **쿨다운/패시브**: `ULuxCooldownTracker`로 태그 기반 쿨다운/감소, 이벤트 구독으로 패시브 트리거
- **타겟팅/카메라/UI**: `UTargetingComponent`(오버랩/라인/스윕/우선순위), `ULuxCameraComponent`(모드), `LuxHUD`/`UW_ActionIcon`/`UW_ActionTooltip`

---

## 아키텍처 설계 철학

1. **데이터 주도 설계 (Data-Driven Design)**
   > “프로그래머의 개입 없이 기획자가 게임을 만든다.”

   액션의 흐름(`Phase`, 전환 조건/행동), 수치(`DataTable`), 툴팁 수식까지 코드와 분리했습니다. 기획자분들이 직접 밸런싱/로직을 수정하며 빠르게 프로토타이핑하실 수 있도록 설계했습니다.

2. **모듈성 및 책임 분리 (Modularity & SRP)**
   > “각 부품은 하나의 역할만 완벽하게 수행한다.”

   `ULuxAction`(흐름 제어), `ULuxActionTask`(비동기 작업), `ULuxCooldownTracker`(쿨다운), `ULuxEffect`(효과), `ULuxCombatManager`(전투)처럼 단일 책임을 명확히 분리하여 테스트 용이성과 재사용성을 높였습니다.

3. **네트워크 우선 설계 (Network-First Design)**
   > “모든 기능은 멀티플레이어 환경을 가정하고 설계한다.”

   서버 권위를 기본으로, `PredictionKey` 기반 클라이언트 예측과 Task/Phase **Re-Home** 동기화를 내장했습니다. 초기 단계부터 지연을 흡수하고 쾌적한 멀티플레이 경험을 목표로 합니다.

---

## 입력 가이드

- **Q / E / R / LMB / RMB**: 액션 사용 (프로필/데이터에 매핑된 액션 실행)
- **B**: 액션/효과/태그 상태를 표시하는 디버그 HUD 토글

> 입력은 `LuxGameplayTags::InputTag_*`에 매핑되어 있으며, 프로젝트 설정에 따라 변경하실 수 있습니다.

---

## 샘플 액션 쇼케이스 (GIF)

- Hoarfrost — 장판 동결:  
  ![Hoarfrost](docs/media/aurora_hoarfrost.gif)
- Glacial Charge — 지형 분석 → 얼음길 생성 → 스플라인 돌진 → 재시전 대기:  
  ![Glacial Charge](docs/media/aurora_glacial_charge.gif)
- Frozen Simulacrum — 방향 섹션 점프 + 분신:  
  ![Frozen Simulacrum](docs/media/aurora_frozen_simulacrum.gif)
- Frozen Sword — 클라 예측 타격 → 서버 검증 → 데미지 적용:  
  ![Frozen Sword](docs/media/aurora_frozen_sword.gif)
- Cryoseism (궁극기) — 도약/충돌/착지 제어 + 연쇄 폭발:  
  ![Cryoseism](docs/media/aurora_cryoseism.gif)
- Cryoseism Passive — 기본공격 적중 시 궁극기 쿨다운 감소:  
  ![Cryoseism Passive](docs/media/aurora_cryoseism_passive.gif)

---

## 기술 스택/핵심 시스템

- **액션 시스템**: `Source/Lux/ActionSystem/Actions/`
  - 베이스: `ULuxAction`, 스펙/핸들: `FLuxActionSpec`, `FActiveLuxActionHandle`
  - 페이즈 태그: `LuxPhaseTags::*`
  - 태스크: `.../Tasks/LuxActionTask_*.{h,cpp}`

- **효과/전투**: `.../Effects/`, `.../System/LuxCombatManager.*`
  - 이펙트 스펙: `FLuxEffectSpec`(ByCaller 파라미터), 동적 태그/부여 태그
  - 데미지 계산/로그/CC 적용

- **쿨다운**: `.../Cooldown/LuxCooldownTracker.*`
  - 태그 기반 시작/중지/감소/퍼센트 감소, UI와 이벤트 연동

- **게임플레이 태그**: `LuxGameplayTags.*`, `LuxActionTags.*`, `LuxActionPhaseTags.*`, `LuxCueTags.*`

- **타겟팅/카메라**: `TargetingComponent`, `LuxCameraComponent`, `LuxCameraMode_*`

- **UI**: `LuxHUD`, `UW_ActionIcon`, `UW_ActionTooltip`(툴팁 수식 파싱/평가)

---

## 데이터 주도 워크플로우

- **레벨 데이터테이블**: 액션 레벨별 수치/파라미터 선언 (`/Game/Characters/Aurora/DT_Aurora`)
- **InstancedStruct 조건/행동**: 페이즈 전환 조건(예: `FCondition_NotifyNameEquals`)과 행동을 데이터로 구성
- **ByCaller 파라미터**: `FLuxEffectSpec::SetByCallerMagnitude(...)`로 런타임 수치 주입
- **툴팁 수식**: `UW_ActionTooltip`이 `{StatName}`, `@ActionData@`, `[Expression]`을 파싱/치환

---

## 네트워킹/예측

- **서버 권위**: 스폰, 데미지, CC, 상태 태그 변화는 서버 주도
- **클라 예측**: 짧고 보수적인 예측(`ULuxActionTask_WaitForServer`)과 **Re-Home** 메커니즘으로 동기화
- **태스크/페이즈 이벤트 복제**: `PostTaskEvent`/구독/타임아웃 전환 지원

---

## 프로젝트 구조

```text
Source/Lux
  - ActionSystem/
    - Actions/ (Aurora 예시 포함)
    - Tasks/ (PlayMontage, Leap, FollowSpline, LandingControl, Wait*, ...)
    - Effects/, Executions/, Cooldown/, Attributes/
    - Targeting/
  - Actors/ (Hoarfrost, GlacialPath, FrozenSimulacrum, CryoseismExplosion)
  - Camera/ (Component/Mode)
  - Character/ (Character/Pawn/Movement/Components)
  - Cues/ (GameplayCue Notifies)
  - System/ (CombatManager, AssetManager, TagStack 등)
  - UI/
  - LuxGameplayTags.*, LuxActionTags.*, LuxActionPhaseTags.*, LuxCueTags.*
```

---

## 빌드/실행 방법

1) 리포지토리 클론  
2) `.uproject` 열기 → 프로젝트 파일 생성  
3) 에디터 또는 IDE에서 빌드/실행  
4) 다른 프로젝트에서 사용 시 `*.Build.cs`에 `Lux` 모듈 추가 및 프로젝트 태그 설정 동기화

---

## 빠른 시작: 새 액션 추가

1) 클래스 생성 (`ULuxAction` 상속) 및 기본 설정

```cpp
ActionIdentifierTag = LuxActionTags::Action_MyAction;
InstancingPolicy    = ELuxActionInstancingPolicy::InstancedPerExecution;
ActivationPolicy    = ELuxActionActivationPolicy::OnInputTriggered;
ActionTags.AddTag(LuxGameplayTags::Action_Type_Ability);
```

2) 페이즈 전환 규칙(예: 노티파이 기반)

```cpp
FPhaseTransition T;
T.TransitionType = EPhaseTransitionType::OnTaskEvent;
T.EventTag       = LuxGameplayTags::Task_Event_Montage_NotifyBegin;
T.NextPhaseTag   = LuxPhaseTags::Phase_Action_Execute;

FInstancedStruct Cond = FInstancedStruct::Make<FCondition_NotifyNameEquals>();
Cond.GetMutable<FCondition_NotifyNameEquals>().RequiredName = FName("StartExecute");
T.Conditions.Add(Cond);

PhaseTransitionRules.FindOrAdd(LuxPhaseTags::Phase_Action_Begin).Add(T);
```

3) 태스크 조합

```cpp
ULuxActionTask_PlayMontageAndWait::PlayMontageAndWait(this, MontageToPlay, 1.0f);
```

4) 태그/쿨다운/효과
- 상태 제어: `AddTags/RemoveTags`
- 쿨다운: `ASC->GetCooldownTracker()->StartCooldown(Spec.GetCooldownTag(), Duration)`
- 데미지/CC: `ULuxCombatManager` + `FLuxEffectSpec`(ByCaller)

---

## 디버그/툴링

- **HUD 디버그**: `B` 키로 액션/효과/태그/카메라 상태 오버레이 토글 (`LuxHUD`)
- **쿨다운 UI**: `UW_ActionIcon`이 시작/진행/종료 이벤트로 게이지 갱신
- **로그 채널**: `LogLuxActionSystem`, `LogLuxCooldown`, `LogLuxCombat` 등

---

## 라이선스/문의

- 라이선스: 프로젝트 전용(내부 포트폴리오 용도). 외부 배포는 별도 협의 부탁드립니다.
- 문의:  
  - 모듈/아키텍처: `Source/Lux/LuxModule.cpp`, `Lux.Build.cs`  
  - 액션 시스템: `Source/Lux/ActionSystem/`  
  - Aurora 샘플: `ActionSystem/Actions/Aurora/`, `Actors/Action/Aurora/`, `Cues/Aurora/`

---

### 참고 소스 링크 예시

- `UAuroraAction_Hoarfrost`: `ActionSystem/Actions/Aurora/AuroraAction_Hoarfrost.cpp`
- `UAuroraAction_GlacialCharge`: `ActionSystem/Actions/Aurora/AuroraAction_GlacialCharge.cpp`
- `UAuroraAction_FrozenSimulacrum`: `ActionSystem/Actions/Aurora/AuroraAction_FrozenSimulacrum.cpp`
- `UAuroraAction_FrozenSword`: `ActionSystem/Actions/Aurora/AuroraAction_FrozenSword.cpp`
- `UAuroraAction_Cryoseism`: `ActionSystem/Actions/Aurora/AuroraAction_Cryoseism.cpp`


