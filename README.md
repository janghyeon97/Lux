## Lux — Unreal Engine 5 액션/효과/태그/타겟팅 프레임워크

게임 클라이언트 개발 포트폴리오 용도의 샘플/레퍼런스 프로젝트입니다.

데이터 주도 액션 시스템과 태그 기반 전투/효과/타겟팅 파이프라인을 제공하며, 캐릭터 "Aurora"의 다양한 액션을 예시로 구현하였습니다.

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

### 1. **데이터 주도 설계 (Data-Driven Design)**
> "프로그래머의 개입 없이 기획자가 게임을 만든다."

액션의 흐름(`Phase`, 전환 조건/행동), 수치(`DataTable`), 툴팁 수식까지 코드와 분리했습니다.  
기획자분들이 직접 밸런싱/로직을 수정하며 빠르게 프로토타이핑하실 수 있도록 설계했습니다.

### 2. **모듈성 및 책임 분리 (Modularity & SRP)**
> "각 부품은 하나의 역할만 완벽하게 수행한다."

`ULuxAction`(흐름 제어), `ULuxActionTask`(비동기 작업), `ULuxCooldownTracker`(쿨다운),  
`ULuxEffect`(효과), `ULuxCombatManager`(전투)처럼 단일 책임을 명확히 분리하여  
테스트 용이성과 재사용성을 높였습니다.

### 3. **네트워크 우선 설계 (Network-First Design)**
> "모든 기능은 멀티플레이어 환경을 가정하고 설계한다."

서버 권위를 기본으로, `PredictionKey` 기반 클라이언트 예측과 Task/Phase **Re-Home** 동기화를 내장했습니다.  
초기 단계부터 지연을 흡수하고 쾌적한 멀티플레이 경험을 목표로 합니다.

---

## 입력 가이드

- **Q / E / R / LMB / RMB**: 액션 사용 (프로필/데이터에 매핑된 액션 실행)
- **B**: 액션/효과/태그 상태를 표시하는 디버그 HUD 토글

> 입력은 `LuxGameplayTags::InputTag_*`에 매핑되어 있으며, 프로젝트 설정에 따라 변경하실 수 있습니다.

---

## 샘플 액션 쇼케이스 (GIF)

- **Hoarfrost** — 장판 동결:  
  ![Hoarfrost](docs/media/aurora_hoarfrost.gif)

- **Glacial Charge** — 지형 분석 → 얼음길 생성 → 스플라인 돌진 → 재시전 대기:  
  ![Glacial Charge](docs/media/aurora_glacial_charge.gif)

- **Frozen Simulacrum** — 방향 섹션 점프 + 분신:  
  ![Frozen Simulacrum](docs/media/aurora_frozen_simulacrum.gif)

- **Frozen Sword** — 클라 예측 타격 → 서버 검증 → 데미지 적용:  
  ![Frozen Sword](docs/media/aurora_frozen_sword.gif)

- **Cryoseism (궁극기)** — 도약/충돌/착지 제어 + 연쇄 폭발:  
  ![Cryoseism](docs/media/aurora_cryoseism.gif)

- **Cryoseism Passive** — 기본공격 적중 시 궁극기 쿨다운 감소:  
  ![Cryoseism Passive](docs/media/aurora_cryoseism_passive.gif)
  
---

## 기술 스택/핵심 시스템

### **1. 액션 시스템 설명**

#### **1.1 ActionSystem 구조 다이어그램**

```
Input → ActionSystemComponent → LuxAction → Phase System → Tasks → Effects
  ↓           ↓                    ↓           ↓          ↓        ↓
InputTag   GrantAction        ActivateAction  Begin    Execute   ApplyEffect
  ↓           ↓                    ↓           ↓          ↓        ↓
Action     TryExecuteAction   OnPhaseEnter   Execute  Recovery  EndAction
  ↓           ↓                    ↓           ↓          ↓        ↓
Spec       EndAction          ExitPhase      End      End      Cleanup
```

#### **1.1.1 LuxAction 설명**

**1. `ULuxAction`** (`Actions/LuxAction.{h,cpp}`)
액션의 전체 생명주기를 관리하는 핵심 클래스로, 페이즈 기반 상태 머신과 태그/이벤트/태스크 시스템을 통합 관리합니다.

**2. `FLuxActionSpec`, `FActiveLuxActionHandle`** (`LuxActionTypes.{h,cpp}`)
액션 정의 정보와 실행 중인 인스턴스를 구분하여 관리하는 구조체들로, 액션의 메타데이터와 런타임 식별을 담당합니다.

**3. `UActionSystemComponent`** (`ActionSystemComponent.{h,cpp}`)
액션 부여/실행/종료를 중앙에서 관리하는 런타임 허브로, 예측 키 생성, 태그 스택 복제, 이펙트 적용을 담당합니다.

##### **1.1.1.1 페이즈 시스템**

**1. 페이즈 태그** (`LuxActionPhaseTags.{h,cpp}`)
액션의 진행 단계를 체계적으로 관리하는 시스템으로, 기본 흐름(Begin→Execute→Recovery→End)과 특수 상황(Interrupt, 이동, 재시전)을 구분합니다.

**2. 전환 규칙/조건** (`Actions/Phase/*`)
페이즈 간 전환을 결정하는 규칙과 조건을 정의하며, 즉시/이벤트/시간 기반 전환을 지원합니다.

##### **1.1.1.2 Task 설명**

액션 내에서 수행되는 구체적인 작업들을 담당하는 시스템으로, 각 태스크는 독립적으로 실행되며 액션의 흐름을 제어합니다.

`ULuxActionTask`는 모든 태스크의 기본 클래스로, 생명주기 관리와 Re-Home 지원을 담당합니다. 주요 태스크들로는 `LuxActionTask_PlayMontageAndWait`(애니메이션), `LuxActionTask_LeapToLocation`(곡선 도약), `LuxActionTask_FollowSpline`(스플라인 추종), `LuxActionTask_LandingControl`(착지 제어), `LuxActionTask_TraceAndBuildPath`(경로 생성) 등이 있습니다.

#### **1.2 LuxEffect 설명**

**1. `FLuxEffectSpec`, `FLuxEffectContextHandle`** (`Effects/LuxEffectTypes.{h,cpp}`)
효과의 템플릿과 컨텍스트 정보를 관리하며, 효과 적용 시 필요한 모든 메타데이터를 제공합니다.

**2. ByCaller 파라미터 태그** (`LuxGameplayTags.h`)
**Effect_SetByCaller_*** 태그 그룹으로 물리/마법 데미지의 Base/Scale/Raw/Final 값과 크리티컬 관련 정보를 동적으로 설정할 수 있습니다.

**3. 적용 파이프라인**
`ApplyEffectSpec*` → `PrepareSpecForApplication` → `CheckApplicationPrerequisites` → `ApplyModifiers` → `AddOrUpdateActiveEffect` 순서로 효과를 적용하며, 네트워크 동기화와 UI 업데이트를 포함합니다.

#### **1.3 LuxAttribute 설명**

**1. 속성 시스템**
**`CombatSet`**: 공격력, 공격 속도, 치명타 관련 속성을 관리합니다.
**`ResourceSet`**: 체력, 마나, 스태미나, 재생 속도를 관리합니다.
**`MovementSet`**: 이동 속도, 가속도, 점프력, 회전 속도를 관리합니다.
**`DefenseSet`**: 방어력, 마법 저항력, 회피율, 차단율을 관리합니다.

**2. 전투 매니저**
**`ULuxCombatManager`** (`System/LuxCombatManager.{h,cpp}`)
데미지 공식 계산, CC 효과 적용, 전투 로그 기록을 중앙에서 관리하는 시스템입니다.

**3. CC 태그** (`CrowdControl_*`)
**Stun**: 기절, **Frozen**: 빙결, **Snare**: 속박, **Silence**: 침묵, **Slow**: 둔화, **Airborne**: 공중부양 효과를 관리합니다.

---

### **2. 쿨다운 트래커 및 UI 연동**

#### **`ULuxCooldownTracker`** (`Cooldown/LuxCooldownTracker.{h,cpp}`)

스킬이나 아이템의 재사용 대기 시간을 관리하는 시스템으로, 시작/중지, 감소, 조회 기능을 제공합니다.

**1. 내부 구조**
**컨테이너**: `FCooldownContainer`는 FFastArraySerializer 기반으로 네트워크 동기화를 지원합니다.

**2. UI 연동**
**`UW_ActionIcon`**이 델리게이트로 원형 게이지를 실시간 업데이트합니다.

---

### **3. 타겟팅 시스템**

#### **`UTargetingComponent`** (`Targeting/TargetingComponent.{h,cpp}`)

게임에서 적이나 목표물을 찾고 선택하는 시스템으로, 전투와 상호작용의 핵심이 되는 컴포넌트입니다.

**1. 검출 타입**
**오버랩**: `FindOverlapTarget`은 범위 내 모든 타겟을 검출하며, 원형/구형 범위 공격에 적합합니다.
**다중 라인**: `FindMultiLineTraceTarget`은 여러 방향 레이 트레이스로 부채/크로스 형태 공격에 적합합니다.
**스윕**: `FindSweepTraceTarget`은 구체/캡슐 콜리전으로 정확한 물리적 상호작용을 시뮬레이션합니다.

**2. 우선순위 시스템**
**가중치 기반**: `SelectBestTargetByPriority`는 거리/각도/크기를 종합하여 최적 타겟을 선택합니다.
**필터**: `DistanceTargetFilter`, `HealthTargetFilter`, `TeamTargetFilter`, `VisibilityTargetFilter`로 조건별 타겟을 걸러냅니다.

**3. 추가 기능**
**오버레이 머티리얼**: 적대/우호/중립 타겟을 색상으로 구분하여 시각적 피드백을 제공합니다.
**네트워킹**: `Server_SetDetectionType`/`Client_ConfirmDetectionTypeChange` RPC로 타겟팅 상태를 동기화합니다.

---

### **4. 카메라 시스템**

#### **`ULuxCameraComponent`** (`Camera/LuxCameraComponent.{h,cpp}`)

게임의 시각적 경험을 담당하는 카메라 시스템으로, 다양한 상황에 맞는 카메라 동작을 모듈화하여 관리합니다.

**1. 모드 스택**
`PushCameraMode`/`PopCameraMode`로 카메라 모드를 스택 형태로 관리하며, `ULuxCameraMode` 파생 타입으로 모듈식 카메라 로직을 구현합니다.

**2. 블렌딩**
`StartCameraBlend`/`UpdateCameraBlending`으로 부드러운 카메라 전환을 구현하며, 전환 시간과 블렌딩 곡선을 지원합니다.

**3. 포커스 모드: `ULuxCameraMode_Focus`**
**목표 추적**: 특정 목표물에 집중하는 카메라 동작으로, 자동 시야 보정을 제공합니다.
**타겟팅 연동**: 타겟팅 컴포넌트와 연동하여 선택된 타겟을 자동으로 추적합니다.

---

---

## 데이터 주도 워크플로우

- **레벨 데이터테이블**: 액션 레벨별 수치/파라미터 선언 (`/Game/Characters/Aurora/DT_Aurora`)

- **InstancedStruct 조건/행동**: 페이즈 전환 조건(예: `FCondition_NotifyNameEquals`)과 행동을 데이터로 구성

- **ByCaller 파라미터**: `FLuxEffectSpec::SetByCallerMagnitude(...)`로 런타임 수치 주입

- **툴팁 수식**: `UW_ActionTooltip`이 `{StatName}`, `@ActionData@`, `[Expression]`을 파싱/치환  
  (수식 파서: `System/ExpressionEvaluator.*`)

---

## 네트워킹/예측

- **서버 권위**: 스폰, 데미지, CC, 상태 태그 변화는 서버 주도

- **클라 예측**: 짧고 보수적인 예측(`ULuxActionTask_WaitForServer`)과 **Re-Home** 메커니즘으로 태스크/페이즈 재소유

- **복제 컨테이너**: `FActiveLuxActionContainer`, `FActiveLuxEffectsContainer`, `FCooldownContainer`는  
  FFastArraySerializer 기반 NetDelta 복제를 사용

- **이벤트 복제**: 태스크/페이즈 이벤트를 태그로 게시(`PostTaskEvent`), 섹션/노티파이/타임아웃에 의한 자동 전환 지원

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

1) **리포지토리 클론**

2) **`.uproject` 열기** → 프로젝트 파일 생성

3) **에디터 또는 IDE에서 빌드/실행**

4) **다른 프로젝트에서 사용 시** `*.Build.cs`에 `Lux` 모듈 추가 및 프로젝트 태그 설정 동기화

---

## 빠른 시작: 새 액션 추가

### 1) 클래스 생성 (`ULuxAction` 상속) 및 기본 설정

```cpp
ActionIdentifierTag = LuxActionTags::Action_MyAction;
InstancingPolicy    = ELuxActionInstancingPolicy::InstancedPerExecution;
ActivationPolicy    = ELuxActionActivationPolicy::OnInputTriggered;
ActionTags.AddTag(LuxGameplayTags::Action_Type_Ability);
```

### 2) 페이즈 전환 규칙(예: 노티파이 기반)

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

### 3) 태스크 조합

```cpp
ULuxActionTask_PlayMontageAndWait::PlayMontageAndWait(this, MontageToPlay, 1.0f);
```

### 4) 태그/쿨다운/효과

- **상태 제어**: `AddTags/RemoveTags`
- **쿨다운**: `ASC->GetCooldownTracker()->StartCooldown(Spec.GetCooldownTag(), Duration)`
- **데미지/CC**: `ULuxCombatManager` + `FLuxEffectSpec`(ByCaller)

---

## 디버그/툴링

- **HUD 디버그**: `B` 키로 액션/효과/태그/카메라 상태 오버레이 토글 (`LuxHUD`)

- **쿨다운 UI**: `UW_ActionIcon`이 시작/진행/종료 이벤트로 게이지 갱신

- **로그 채널**: `LogLuxActionSystem`, `LogLuxCooldown`, `LogLuxCombat` 등

---

## 라이선스/문의

- **라이선스**: 프로젝트 전용(내부 포트폴리오 용도). 외부 배포는 별도 협의 부탁드립니다.

- **문의**:
  - 모듈/아키텍처: `Source/Lux/LuxModule.cpp`, `Lux.Build.cs`
  - 액션 시스템: `Source/Lux/ActionSystem/`
  - Aurora 샘플: `ActionSystem/Actions/Aurora/`, `Actors/Action/Aurora/`, `Cues/Aurora/`

---

### 참고 소스 링크 예시

- **`UAuroraAction_Hoarfrost`**: `ActionSystem/Actions/Aurora/AuroraAction_Hoarfrost.cpp`
- **`UAuroraAction_GlacialCharge`**: `ActionSystem/Actions/Aurora/AuroraAction_GlacialCharge.cpp`
- **`UAuroraAction_FrozenSimulacrum`**: `ActionSystem/Actions/Aurora/AuroraAction_FrozenSimulacrum.cpp`
- **`UAuroraAction_FrozenSword`**: `ActionSystem/Actions/Aurora/AuroraAction_FrozenSword.cpp`
- **`UAuroraAction_Cryoseism`**: `ActionSystem/Actions/Aurora/AuroraAction_Cryoseism.cpp`


