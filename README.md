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

| 기능 | 설명 |
|------|------|
| **액션 상태머신** | `ULuxAction` + `Phase` 전환 규칙으로 Begin/Execute/Recovery/End/Interrupt, Dash/Leap/Landing 등을 선언적으로 구성 |
| **태스크 조립식 설계** | `ULuxActionTask_PlayMontageAndWait`, `..._LeapToLocation`, `..._FollowSpline`, `..._LandingControl`, `..._WaitForServer` 등 비동기 유닛을 조합 |
| **태그 지향 전투/효과** | `ULuxCombatManager`, `ULuxEffect`, `FLuxEffectSpec` + ByCaller 파라미터로 데미지/버프/CC 데이터-드리븐 처리 |
| **쿨다운/패시브** | `ULuxCooldownTracker`로 태그 기반 쿨다운/감소, 이벤트 구독으로 패시브 트리거 |
| **타겟팅/카메라/UI** | `UTargetingComponent`(오버랩/라인/스윕/우선순위), `ULuxCameraComponent`(모드), `LuxHUD`/`UW_ActionIcon`/`UW_ActionTooltip` |

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

| 입력 | 기능 |
|------|------|
| **Q / E / R** | 액션 사용 (프로필/데이터에 매핑된 액션 실행) |
| **LMB / RMB** | 액션 사용 (프로필/데이터에 매핑된 액션 실행) |
| **B** | 액션/효과/태그 상태를 표시하는 디버그 HUD 토글 |

> 입력은 `LuxGameplayTags::InputTag_*`에 매핑되어 있으며, 프로젝트 설정에 따라 변경하실 수 있습니다.

---

## 샘플 액션 쇼케이스 (GIF)

| 액션 | 설명 | GIF |
|------|------|-----|
| **Hoarfrost** | 장판 동결 | ![Hoarfrost](docs/media/aurora_hoarfrost.gif) |
| **Glacial Charge** | 지형 분석 → 얼음길 생성 → 스플라인 돌진 → 재시전 대기 | ![Glacial Charge](docs/media/aurora_glacial_charge.gif) |
| **Frozen Simulacrum** | 방향 섹션 점프 + 분신 | ![Frozen Simulacrum](docs/media/aurora_frozen_simulacrum.gif) |
| **Frozen Sword** | 클라 예측 타격 → 서버 검증 → 데미지 적용 | ![Frozen Sword](docs/media/aurora_frozen_sword.gif) |
| **Cryoseism (궁극기)** | 도약/충돌/착지 제어 + 연쇄 폭발 | ![Cryoseism](docs/media/aurora_cryoseism.gif) |
| **Cryoseism Passive** | 기본공격 적중 시 궁극기 쿨다운 감소 | ![Cryoseism Passive](docs/media/aurora_cryoseism_passive.gif) |

> GIF 경로는 `docs/media/*.gif` 기준 예시입니다. 실제 경로/파일은 리포지토리 환경에 맞게 수정 부탁드립니다.

---

## 기술 스택/핵심 시스템

### 1) 액션 시스템 (Actions)

- 핵심 타입/역할
  - `ULuxAction` (`Source/Lux/ActionSystem/Actions/LuxAction.{h,cpp}`): 액션 수명주기, 페이즈 상태, 태그/이벤트/태스크 관리의 중심 클래스입니다. 주요 훅: `ActivateAction`, `OnPhaseEnter`, `OnActionEnd`, `EnterPhase`, `ExitPhase`.
  - `FLuxActionSpec`, `FActiveLuxActionHandle` (`LuxActionTypes.{h,cpp}`): 액션 정의/인스턴스 식별. Spec에는 `InputTag`, `ActionIdentifierTag`, `Level`, `DynamicTags`, `LastExecutionTime` 등이 포함됩니다.
  - `UActionSystemComponent` (`ActionSystemComponent.{h,cpp}`): 액션 부여/실행/종료, 예측 키 생성, 태그 스택 복제, 이벤트 브로드캐스트, 이펙트 적용/해제 등 런타임 허브 역할을 합니다. 대표 API: `GrantAction`, `TryExecuteAction`, `EndAction`, `SubscribeToTaskEvent`, `ExecuteGameplayCue`.
  - 페이즈 태그: `LuxActionPhaseTags.{h,cpp}` — `Phase_Action_Begin/Execute/Recovery/End/Interrupt`, `Dash/Leap/Landing/WaitRecast/Recast` 등.
  - 전환 규칙/조건: `FPhaseTransition`, `FPhaseConditionBase`, `FCondition_NotifyNameEquals` (`Actions/Phase/*`). 전환 타입은 `Immediate/OnTaskEvent/OnGameplayEvent/OnDurationEnd`를 지원합니다.

- 태스크(Tasks) — 비동기 유닛
  - 공통 베이스: `ULuxActionTask` (`Tasks/LuxActionTask.{h,cpp}`) — 라이프사이클(Activate/End), Re-Home 지원.
  - 애니메이션: `LuxActionTask_PlayMontageAndWait` — 노티파이 Begin/End, BlendOut, Ended를 태그 이벤트로 게시.
  - 이동: `LuxActionTask_LeapToLocation`, `LuxActionTask_FollowSpline`, `LuxActionTask_LandingControl` — 곡선 도약, 스플라인 추종, 착지 속도/중력 제어.
  - 흐름/입력: `LuxActionTask_WaitDelay`, `LuxActionTask_WaitPhaseDelay`, `LuxActionTask_WaitGameplayEvent`, `LuxActionTask_WaitInputPress/Release`, `LuxActionTask_WaitForServer`.
  - Aurora 특화: `Aurora/LuxActionTask_TraceAndBuildPath` — 지형 트레이스 기반 경로 샘플링/스무딩/스플라인 포인트 생성.

- 샘플 액션 구현 포인트
  - `UAuroraAction_Hoarfrost` — 서버에서 `AHoarfrost` 스폰, 클라에는 `Cue_Aurora_Hoarfrost_FreezeSegments` 실행.
  - `UAuroraAction_GlacialCharge` — Begin에서 경로 분석(Task), Dash에서 스플라인 추종(Task) + 버블 쉴드 Cue.
  - `UAuroraAction_FrozenSimulacrum` — 속도/조준 기반 8방향 섹션 선택, 분신 스폰.
  - `UAuroraAction_FrozenSword` — 클라 예측 `Task_Event_Attack_HitPredicted` → 서버 `Phase_ProcessHit`에서 데미지 확정.
  - `UAuroraAction_Cryoseism` — Leap/Impact/Landing/Recovery 페이즈, 중력 임시 비활성/복원, `ACryoseismExplosion` 체인 폭발.

### 2) 효과/전투 시스템 (Effects & Combat)

- 효과 스펙/컨텍스트
  - `FLuxEffectSpec`, `FLuxEffectContextHandle` (`Effects/LuxEffectTypes.{h,cpp}`): 템플릿(ULuxEffect CDO) + 컨텍스트(Instigator/Source/Target ASC, SourceAction) + 계산된 Modifiers/태그/기간/주기.
  - ByCaller 파라미터 태그: `Effect_SetByCaller_*` (`LuxGameplayTags.h`) — 물리/마법 Base/Scale/Raw/Final, 크리티컬 확률/배율/판정, 총합 데미지 등.

- 적용 파이프라인
  - `UActionSystemComponent::ApplyEffectSpec*` → `PrepareSpecForApplication` → `CheckApplicationPrerequisites` → `ApplyModifiers` → `AddOrUpdateActiveEffect`.
  - 활성 효과 컨테이너: `FActiveLuxEffectsContainer` (FFastArraySerializer 기반 복제).
  - 속성 시스템: `ULuxAttributeSet` + `FLuxAttributeData` (`Attributes/*`) — `CombatSet`(공속/공격력 등), `ResourceSet`(체력/마나/재생), `MovementSet`, `DefenseSet`.

- 전투 매니저
  - `ULuxCombatManager` (`System/LuxCombatManager.{h,cpp}`): 데미지 공식/CC 적용/전투 로그 기록. `ApplyDamage`, `ApplyCrowdControl`, `RecordCombatLog`.
  - CC 태그: `CrowdControl_*` (`LuxGameplayTags.h`) — Stun/Frozen/Snare/Silence/Slow/Airborne 등, Cue/상태 태그와 연동.

### 3) 쿨다운 시스템 (Cooldown)

- 트래커: `ULuxCooldownTracker` (`Cooldown/LuxCooldownTracker.{h,cpp}`)
  - 시작/중지: `StartCooldown`, `StopCooldown`
  - 감소: `ReduceCooldown`, `ReduceCooldownByPercent`
  - 조회: `GetTimeRemaining`, `GetDuration`
  - 내부 컨테이너: `FCooldownContainer` (FFastArraySerializer 기반 복제)
  - UI 연동: `UW_ActionIcon`이 시작/진행/종료 델리게이트로 원형 게이지 업데이트

### 4) 타겟팅 시스템 (Targeting)

- `UTargetingComponent` (`Targeting/TargetingComponent.{h,cpp}`)
  - 검출 타입: 오버랩(`FindOverlapTarget`), 다중 라인(`FindMultiLineTraceTarget`), 스윕(`FindSweepTraceTarget`) — 거리/각도/크기 가중치 기반 우선순위 선정(`SelectBestTargetByPriority`).
  - 필터: `DistanceTargetFilter`, `HealthTargetFilter`, `TeamTargetFilter`, `VisibilityTargetFilter`.
  - 오버레이 머티리얼 토글: 적대/우호/중립 Material 적용 가능.
  - 네트워킹: `Server_SetDetectionType`/`Client_ConfirmDetectionTypeChange` RPC.

### 5) 카메라 시스템 (Camera)

- `ULuxCameraComponent` (`Camera/LuxCameraComponent.{h,cpp}`)
  - 모드 스택: `PushCameraMode`/`PopCameraMode` — `ULuxCameraMode` 파생 타입으로 모듈식 카메라 로직.
  - 블렌딩: `StartCameraBlend`/`UpdateCameraBlending`.
- 포커스 모드: `ULuxCameraMode_Focus` — 목표 추적/시야 보정, 타겟팅 컴포넌트와 연동.

### 6) 큐 시스템 (Gameplay Cues)

- 매니저/노티파이: `ULuxCueManager`, `ALuxCueNotify` (`Cues/*`)
  - Cue 실행/정지/풀링, 액터별/태그별 큐 카운트 조회.
  - Aurora 샘플: `Cue_Aurora_Hoarfrost_FreezeSegments`, `Cue_Aurora_GlacialCharge_FrostShield`, `Cue_Aurora_FrozenSword_Primary`, `Cue_Aurora_Ultimate_*`.

### 7) 시스템/유틸리티

- 에셋 관리: `ULuxAssetManager` — 게임 데이터/큐/이펙트/CC 데이터 사전 로드 및 캐시.
- 태그 스택: `FGameplayTagStackContainer` — 스택 카운트형 태그(상태/차단/콤보 등) 복제/동기화.
- 로그 채널: `LogLux`, `LogLuxActionSystem`, `LogLuxCooldown`, `LogLuxCombat` (`LuxLogChannels.*`).

---

## 데이터 주도 워크플로우

| 구성 요소 | 설명 | 예시 |
|-----------|------|------|
| **레벨 데이터테이블** | 액션 레벨별 수치/파라미터 선언 | `/Game/Characters/Aurora/DT_Aurora` |
| **InstancedStruct 조건/행동** | 페이즈 전환 조건과 행동을 데이터로 구성 | `FCondition_NotifyNameEquals` |
| **ByCaller 파라미터** | `FLuxEffectSpec::SetByCallerMagnitude(...)`로 런타임 수치 주입 | 데미지, 지속시간 등 |
| **툴팁 수식** | `UW_ActionTooltip`이 `{StatName}`, `@ActionData@`, `[Expression]`을 파싱/치환 | 수식 파서: `System/ExpressionEvaluator.*` |

---

## 네트워킹/예측

| 영역 | 설명 |
|------|------|
| **서버 권위** | 스폰, 데미지, CC, 상태 태그 변화는 서버 주도 |
| **클라 예측** | 짧고 보수적인 예측(`ULuxActionTask_WaitForServer`)과 **Re-Home** 메커니즘으로 태스크/페이즈 재소유 |
| **복제 컨테이너** | `FActiveLuxActionContainer`, `FActiveLuxEffectsContainer`, `FCooldownContainer`는 FFastArraySerializer 기반 NetDelta 복제를 사용 |
| **이벤트 복제** | 태스크/페이즈 이벤트를 태그로 게시(`PostTaskEvent`), 섹션/노티파이/타임아웃에 의한 자동 전환 지원 |

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

| 단계 | 작업 |
|------|------|
| **1** | 리포지토리 클론 |
| **2** | `.uproject` 열기 → 프로젝트 파일 생성 |
| **3** | 에디터 또는 IDE에서 빌드/실행 |
| **4** | 다른 프로젝트에서 사용 시 `*.Build.cs`에 `Lux` 모듈 추가 및 프로젝트 태그 설정 동기화 |

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

| 도구 | 설명 |
|------|------|
| **HUD 디버그** | `B` 키로 액션/효과/태그/카메라 상태 오버레이 토글 (`LuxHUD`) |
| **쿨다운 UI** | `UW_ActionIcon`이 시작/진행/종료 이벤트로 게이지 갱신 |
| **로그 채널** | `LogLuxActionSystem`, `LogLuxCombat`, `LogLuxCooldown` 등 |

---

## 라이선스/문의

| 항목 | 내용 |
|------|------|
| **라이선스** | 프로젝트 전용(내부 포트폴리오 용도). 외부 배포는 별도 협의 부탁드립니다. |
| **모듈/아키텍처** | `Source/Lux/LuxModule.cpp`, `Lux.Build.cs` |
| **액션 시스템** | `Source/Lux/ActionSystem/` |
| **Aurora 샘플** | `ActionSystem/Actions/Aurora/`, `Actors/Action/Aurora/`, `Cues/Aurora/` |

---

### 참고 소스 링크 예시

| 액션 클래스 | 파일 경로 |
|-------------|-----------|
| `UAuroraAction_Hoarfrost` | `ActionSystem/Actions/Aurora/AuroraAction_Hoarfrost.cpp` |
| `UAuroraAction_GlacialCharge` | `ActionSystem/Actions/Aurora/AuroraAction_GlacialCharge.cpp` |
| `UAuroraAction_FrozenSimulacrum` | `ActionSystem/Actions/Aurora/AuroraAction_FrozenSimulacrum.cpp` |
| `UAuroraAction_FrozenSword` | `ActionSystem/Actions/Aurora/AuroraAction_FrozenSword.cpp` |
| `UAuroraAction_Cryoseism` | `ActionSystem/Actions/Aurora/AuroraAction_Cryoseism.cpp` |


