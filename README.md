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

---

### **1) 액션 시스템 (Actions)**

#### **핵심 타입/역할**

**`ULuxAction`** (`Actions/LuxAction.{h,cpp}`)
액션 시스템의 핵심 클래스로, 플레이어가 입력을 받았을 때부터 액션이 완전히 종료될 때까지의 전체 생명주기를 관리합니다. 

액션이 시작되면 먼저 `ActivateAction`이 호출되어 초기 설정을 수행하고, 그 후 정의된 페이즈 순서대로 진행됩니다. 각 페이즈에 진입할 때마다 `OnPhaseEnter`가 호출되어 해당 페이즈에 필요한 로직을 실행하고, 페이즈가 끝나면 `ExitPhase`를 통해 다음 페이즈로 전환하거나 액션을 종료합니다.

액션 내부에서는 태그 시스템을 통해 현재 상태를 추적하고, 이벤트 시스템으로 다른 시스템들과 통신하며, 태스크 시스템을 통해 비동기 작업들을 관리합니다. 예를 들어, 공격 액션의 경우 "준비" → "실행" → "회복" 페이즈를 거치며, 각 페이즈에서 애니메이션 재생, 데미지 계산, 이펙트 적용 등의 작업을 수행합니다.

**`FLuxActionSpec`, `FActiveLuxActionHandle`** (`LuxActionTypes.{h,cpp}`)
액션의 정의와 실행 중인 인스턴스를 구분하여 관리하는 구조체들입니다.

`FLuxActionSpec`은 액션의 기본 정보를 담고 있으며, 어떤 입력 키(`InputTag`)에 반응할지, 어떤 액션인지(`ActionIdentifierTag`), 현재 레벨은 몇인지, 추가적인 동적 태그들은 무엇인지 등을 정의합니다. 이 정보들은 액션이 실행될 때마다 참조되어 적절한 동작을 결정하는 데 사용됩니다.

`FActiveLuxActionHandle`은 현재 실행 중인 액션 인스턴스를 고유하게 식별하는 핸들입니다. 같은 액션이 여러 번 실행되거나 중첩되어 실행될 수 있기 때문에, 각각을 구분하여 관리할 수 있도록 해줍니다. 이를 통해 특정 액션을 중단하거나, 상태를 조회하거나, 다른 시스템과 연동할 때 정확한 인스턴스를 대상으로 작업할 수 있습니다.

**`UActionSystemComponent`** (`ActionSystemComponent.{h,cpp}`)
액션 시스템의 런타임 허브 역할을 하는 컴포넌트로, 캐릭터나 액터에 부착되어 액션 관련 모든 작업을 중앙에서 관리합니다.

플레이어가 입력을 주면 `TryExecuteAction`이 호출되어 해당 입력에 매핑된 액션을 찾고 실행합니다. 액션이 실행되면 `GrantAction`을 통해 액션을 부여하고, 필요한 경우 예측 키를 생성하여 클라이언트 예측을 지원합니다. 

액션이 진행되는 동안 태그 스택을 복제하여 네트워크 동기화를 보장하고, 이벤트를 브로드캐스트하여 다른 시스템들이 액션의 상태 변화를 감지할 수 있도록 합니다. 또한 `ApplyEffectSpec`을 통해 이펙트를 적용하거나 해제하여 게임플레이에 직접적인 영향을 미칩니다.

액션이 완료되거나 중단되면 `EndAction`을 호출하여 정리 작업을 수행하고, `ExecuteGameplayCue`를 통해 시각적/청각적 피드백을 제공합니다.

#### **페이즈 & 전환 시스템**

**페이즈 태그** (`LuxActionPhaseTags.{h,cpp}`)
액션의 진행 단계를 체계적으로 관리하는 시스템으로, 각 액션이 어떤 상태에 있는지 명확하게 구분합니다.

기본적인 액션 흐름은 `Phase_Action_Begin`에서 시작하여 `Execute` 단계를 거쳐 `Recovery`를 마친 후 `End`로 종료됩니다. `Begin` 페이즈에서는 액션 실행을 위한 초기화 작업을 수행하고, `Execute` 페이즈에서는 실제 게임플레이 효과를 적용하며, `Recovery` 페이즈에서는 액션 완료 후 필요한 정리 작업을 수행합니다.

특수한 상황에서는 `Interrupt` 페이즈가 발생할 수 있는데, 이는 다른 액션이 우선순위가 높거나 플레이어가 액션을 중단했을 때 호출됩니다. 이동 관련 액션의 경우 `Dash`, `Leap`, `Landing` 페이즈를 통해 이동의 시작, 진행, 완료를 세밀하게 제어합니다. 재시전이 가능한 액션의 경우 `WaitRecast`와 `Recast` 페이즈를 통해 재사용 대기 시간과 재실행을 관리합니다.

**전환 규칙/조건** (`Actions/Phase/*`)
페이즈 간의 전환을 결정하는 규칙과 조건을 정의하는 시스템입니다.

`FPhaseTransition`은 어떤 조건에서 다음 페이즈로 넘어갈지를 결정하는 규칙을 담고 있으며, `FPhaseConditionBase`를 상속받아 구체적인 조건을 구현합니다. 예를 들어, `FCondition_NotifyNameEquals`는 애니메이션 노티파이의 이름이 특정 값과 일치할 때 전환 조건을 만족시킵니다.

전환 타입은 `Immediate`로 즉시 전환하거나, `OnTaskEvent`로 특정 태스크 이벤트가 발생했을 때 전환하거나, `OnGameplayEvent`로 게임플레이 이벤트가 발생했을 때 전환하거나, `OnDurationEnd`로 설정된 시간이 끝났을 때 전환하는 방식으로 구분됩니다. 이를 통해 액션의 흐름을 유연하게 제어할 수 있습니다.

#### **태스크(Tasks) — 비동기 유닛**

액션 내에서 수행되는 구체적인 작업들을 담당하는 시스템으로, 각 태스크는 독립적으로 실행되며 액션의 흐름을 제어합니다.

**공통 베이스**
`ULuxActionTask`는 모든 태스크의 기본 클래스로, 태스크의 생명주기를 관리합니다. 태스크가 시작되면 `Activate`가 호출되어 초기화를 수행하고, 완료되거나 중단되면 `End`가 호출되어 정리 작업을 수행합니다. 

특히 중요한 것은 Re-Home 지원으로, 이는 네트워크 환경에서 클라이언트 예측과 서버 권위를 동기화하는 메커니즘입니다. 클라이언트에서 예측한 결과와 서버의 실제 결과가 다를 때, 서버가 올바른 상태로 클라이언트를 "집으로 데려오는" 역할을 합니다.

**애니메이션**
`LuxActionTask_PlayMontageAndWait`는 애니메이션 몽타주를 재생하고 완료될 때까지 기다리는 태스크입니다. 

애니메이션이 시작되면 `Begin` 노티파이를 게시하여 다른 시스템들이 애니메이션 시작을 감지할 수 있게 하고, 애니메이션이 끝나면 `Ended` 노티파이를 게시합니다. 또한 `BlendOut` 노티파이를 통해 애니메이션이 부드럽게 전환되는 시점을 알려줍니다. 이러한 노티파이들은 태그 이벤트로 변환되어 액션의 페이즈 전환 조건으로 활용됩니다.

**이동**
이동 관련 태스크들은 캐릭터의 위치와 움직임을 세밀하게 제어합니다.

`LuxActionTask_LeapToLocation`은 캐릭터가 현재 위치에서 목표 지점까지 곡선을 그리며 도약하도록 합니다. 이는 단순한 직선 이동이 아닌, 점프나 대시와 같은 자연스러운 움직임을 구현할 때 사용됩니다.

`LuxActionTask_FollowSpline`은 미리 정의된 스플라인 경로를 따라 캐릭터를 이동시킵니다. 복잡한 경로나 패턴 이동이 필요한 액션에서 활용됩니다.

`LuxActionTask_LandingControl`은 착지 시 캐릭터의 속도와 중력을 세밀하게 제어합니다. 높은 곳에서 떨어질 때 충격을 줄이거나, 착지 후의 움직임을 부드럽게 만드는 등의 효과를 구현할 수 있습니다.

**흐름/입력**
액션의 흐름을 제어하고 사용자 입력이나 외부 이벤트를 기다리는 태스크들입니다.

`LuxActionTask_WaitDelay`는 지정된 시간만큼 대기하는 태스크로, 액션의 타이밍을 조절할 때 사용됩니다. `LuxActionTask_WaitPhaseDelay`는 특정 페이즈에서만 대기하는 태스크로, 페이즈별로 다른 대기 시간을 설정할 수 있습니다.

`LuxActionTask_WaitGameplayEvent`는 특정 게임플레이 이벤트가 발생할 때까지 기다리는 태스크입니다. 예를 들어, 데미지를 받았을 때나 특정 상태가 되었을 때까지 대기할 수 있습니다.

`LuxActionTask_WaitInputPress/Release`는 사용자가 특정 키를 누르거나 놓을 때까지 기다리는 태스크로, 콤보 시스템이나 차지 시스템을 구현할 때 유용합니다.

`LuxActionTask_WaitForServer`는 서버로부터 응답을 받을 때까지 기다리는 태스크로, 네트워크 동기화가 필요한 작업에서 사용됩니다.

**Aurora 특화**
`LuxActionTask_TraceAndBuildPath`는 Aurora 캐릭터의 특수 능력을 위해 개발된 태스크로, 지형을 분석하여 최적의 이동 경로를 생성합니다.

이 태스크는 먼저 지형에 레이 트레이스를 수행하여 장애물이나 지형의 높낮이를 파악합니다. 그 후 샘플링을 통해 경로상의 여러 지점들을 추출하고, 스무딩 알고리즘을 적용하여 부드러운 경로를 만듭니다. 최종적으로 스플라인 포인트들을 생성하여 `LuxActionTask_FollowSpline`과 연동하여 사용할 수 있도록 합니다.

#### **샘플 액션 구현 포인트**

**`UAuroraAction_Hoarfrost`**
Aurora의 장판 동결 능력으로, 지정된 영역에 얼음 장판을 생성하여 적들을 동결시키는 액션입니다.

액션이 실행되면 서버에서는 `AHoarfrost` 액터를 스폰하여 실제 게임플레이에 영향을 미치는 장판을 생성합니다. 이 장판은 오버랩 이벤트를 통해 적 캐릭터들과 상호작용하며, 동결 효과를 적용합니다. 클라이언트에서는 `Cue_Aurora_Hoarfrost_FreezeSegments` 큐를 실행하여 얼음 장판이 생성되는 시각적 효과와 동결되는 적들의 애니메이션을 표시합니다.

**`UAuroraAction_GlacialCharge`**
Aurora가 얼음 방패를 두르고 목표 지점까지 돌진하는 능력입니다.

액션의 `Begin` 페이즈에서는 `LuxActionTask_TraceAndBuildPath`를 사용하여 현재 위치에서 목표 지점까지의 경로를 분석합니다. 이 과정에서 장애물을 피하고 지형의 높낮이를 고려한 최적 경로를 생성합니다. `Dash` 페이즈에서는 생성된 경로를 따라 `LuxActionTask_FollowSpline`을 사용하여 스플라인을 따라 이동하며, 동시에 `Cue_Aurora_GlacialCharge_FrostShield` 큐를 실행하여 얼음 방패 효과를 표시합니다.

**`UAuroraAction_FrozenSimulacrum`**
Aurora가 자신의 분신을 생성하여 적을 공격하는 능력입니다.

이 액션은 캐릭터의 현재 이동 속도와 조준 방향을 분석하여 8방향 중 어느 섹션에 분신을 생성할지를 결정합니다. 속도가 빠를수록 더 멀리, 조준 방향을 고려하여 적절한 위치에 분신을 배치합니다. 분신이 생성되면 독립적인 AI를 가진 액터로 동작하며, Aurora와 유사한 공격 패턴을 수행합니다.

**`UAuroraAction_FrozenSword`**
Aurora의 기본 공격 능력으로, 클라이언트 예측과 서버 권위를 조합한 시스템을 보여줍니다.

클라이언트에서 공격이 적중할 것으로 예측되면 `Task_Event_Attack_HitPredicted` 이벤트를 발생시켜 즉시 시각적 피드백을 제공합니다. 이는 네트워크 지연을 느끼지 않게 하여 반응성을 향상시킵니다. 서버에서는 `Phase_ProcessHit` 페이즈에서 실제 데미지 계산을 수행하고, 클라이언트 예측과 다를 경우 Re-Home 메커니즘을 통해 올바른 상태로 동기화합니다.

**`UAuroraAction_Cryoseism`**
Aurora의 궁극기로, 복잡한 페이즈 시스템과 연쇄 효과를 보여주는 액션입니다.

액션은 `Leap` 페이즈에서 시작하여 공중으로 점프하고, `Impact` 페이즈에서 지면에 충격을 주며, `Landing` 페이즈에서 착지하고, `Recovery` 페이즈에서 회복하는 구조로 되어 있습니다. `Leap`과 `Landing` 페이즈에서는 중력을 임시로 비활성화하여 자연스러운 점프와 착지를 구현하고, `Impact` 페이즈에서는 `ACryoseismExplosion` 액터를 스폰하여 연쇄 폭발 효과를 생성합니다.

---

### **2) 효과/전투 시스템 (Effects & Combat)**

#### **효과 스펙/컨텍스트**

**`FLuxEffectSpec`, `FLuxEffectContextHandle`** (`Effects/LuxEffectTypes.{h,cpp}`)
게임플레이 효과를 정의하고 적용하는 시스템의 핵심 구조체들입니다.

`FLuxEffectSpec`은 효과의 템플릿 역할을 하며, `ULuxEffect` CDO(Class Default Object)를 기반으로 합니다. 이 템플릿에는 효과가 어떤 속성을 수정할지, 어떤 태그를 적용할지, 얼마나 지속될지 등의 기본 정보가 정의되어 있습니다.

`FLuxEffectContextHandle`은 효과가 적용되는 컨텍스트 정보를 담고 있습니다. 누가 효과를 발생시켰는지(Instigator), 어디서 효과가 시작되었는지(Source), 누구에게 효과가 적용되는지(Target), 어떤 액션에서 효과가 발생했는지(SourceAction) 등의 정보를 포함합니다. 이 정보들은 효과의 계산과 적용 과정에서 중요한 역할을 합니다.

#### **ByCaller 파라미터 태그** (`LuxGameplayTags.h`)

**Effect_SetByCaller_*** 태그 그룹은 효과가 적용될 때 동적으로 값을 설정할 수 있게 해주는 시스템입니다.

**Physical_Base/Scale/Raw/Final**: 물리 데미지와 관련된 값들을 단계별로 관리합니다. `Base`는 기본 데미지 값이고, `Scale`은 계수(레벨이나 버프에 따른 배율), `Raw`는 원시 계산값, `Final`은 최종 적용될 데미지 값입니다.

**Magical_Base/Scale/Raw/Final**: 마법 데미지에 대해서도 동일한 구조로 관리하며, 물리 데미지와는 별도로 계산됩니다.

**Critical_Chance/Multiplier/IsCritical**: 크리티컬 시스템을 위한 값들로, `Chance`는 크리티컬 발생 확률, `Multiplier`는 크리티컬 시 데미지 배율, `IsCritical`은 현재 공격이 크리티컬인지 여부를 나타냅니다.

**Damage_Total**: 모든 계산이 완료된 후의 최종 데미지 값으로, UI 표시나 로그 기록에 사용됩니다.

#### **적용 파이프라인**

효과가 실제로 적용되는 과정은 여러 단계를 거쳐 진행됩니다.

먼저 `UActionSystemComponent::ApplyEffectSpec*`가 호출되어 효과 적용을 시작합니다. 그 후 `PrepareSpecForApplication`에서 효과 스펙을 준비하고, `CheckApplicationPrerequisites`에서 효과를 적용할 수 있는 조건이 충족되었는지 확인합니다. 

조건이 만족되면 `ApplyModifiers`를 통해 실제 속성 수정을 수행하고, 마지막으로 `AddOrUpdateActiveEffect`를 통해 활성 효과 목록에 추가하거나 기존 효과를 업데이트합니다. 이 과정에서 네트워크 동기화와 UI 업데이트도 함께 이루어집니다.

#### **속성 시스템**

캐릭터의 모든 수치적 특성을 체계적으로 관리하는 시스템입니다.

**`CombatSet`**: 전투와 직접적으로 관련된 속성들을 관리합니다. 공격력, 공격 속도, 치명타 확률, 치명타 피해량 등이 포함되며, 이 값들은 데미지 계산의 기본이 됩니다.

**`ResourceSet`**: 캐릭터가 소모하고 회복하는 자원들을 관리합니다. 체력, 마나, 스태미나, 재생 속도 등이 포함되며, 이 값들은 캐릭터의 생존과 능력 사용 가능 여부를 결정합니다.

**`MovementSet`**: 캐릭터의 움직임과 관련된 속성들을 관리합니다. 이동 속도, 가속도, 점프력, 회전 속도 등이 포함되며, 이 값들은 캐릭터의 기동성을 결정합니다.

**`DefenseSet`**: 캐릭터의 방어 능력과 관련된 속성들을 관리합니다. 방어력, 마법 저항력, 회피율, 차단율 등이 포함되며, 이 값들은 받는 데미지를 줄이거나 완전히 피하는 데 영향을 줍니다.

#### **전투 매니저**

**`ULuxCombatManager`** (`System/LuxCombatManager.{h,cpp}`)
전투와 관련된 모든 계산과 처리를 중앙에서 관리하는 시스템입니다.

주요 기능으로는 데미지 공식 계산, 군중제어(CC) 효과 적용, 전투 로그 기록 등이 있습니다. `ApplyDamage` 함수는 공격자의 공격력, 방어자의 방어력, 스킬의 계수, 크리티컬 여부 등을 고려하여 최종 데미지를 계산합니다. 이 과정에서 물리/마법 저항력, 데미지 감소 효과, 보호막 등이 모두 반영됩니다.

`ApplyCrowdControl` 함수는 기절, 빙결, 속박 등의 상태 이상 효과를 적용합니다. 이 함수는 효과의 지속시간, 중첩 가능 여부, 해제 조건 등을 고려하여 적절한 CC 효과를 적용하고, UI와 애니메이션에 반영합니다.

`RecordCombatLog` 함수는 모든 전투 관련 이벤트를 기록하여 디버깅과 분석에 활용할 수 있도록 합니다. 데미지 수치, CC 적용, 스킬 사용 등의 정보가 시간순으로 기록됩니다.

**CC 태그** (`CrowdControl_*`)
군중제어 효과를 체계적으로 관리하는 태그 시스템입니다.

**Stun**: 기절 효과로, 캐릭터가 모든 행동을 할 수 없게 만듭니다. 공격, 이동, 스킬 사용이 모두 불가능하며, 일정 시간 후 자동으로 해제됩니다.

**Frozen**: 빙결 효과로, 캐릭터가 얼음에 갇혀 움직일 수 없게 만듭니다. Stun과 유사하지만, 빙결 해제 시 추가적인 효과나 데미지를 줄 수 있습니다.

**Snare**: 속박 효과로, 캐릭터의 이동만 제한합니다. 공격이나 스킬 사용은 가능하지만, 위치를 바꿀 수 없어 전략적 선택이 제한됩니다.

**Silence**: 침묵 효과로, 캐릭터가 스킬을 사용할 수 없게 만듭니다. 기본 공격과 이동은 가능하지만, 능력 사용이 불가능합니다.

**Slow**: 둔화 효과로, 캐릭터의 이동 속도를 감소시킵니다. 완전히 멈추지는 않지만, 기동성이 크게 저하됩니다.

**Airborne**: 공중부양 효과로, 캐릭터를 공중에 띄웁니다. 이 상태에서는 특정 공격에 더 취약하거나, 추가적인 콤보를 받을 수 있습니다.

---

### **3) 쿨다운 시스템 (Cooldown)**

#### **트래커: `ULuxCooldownTracker`** (`Cooldown/LuxCooldownTracker.{h,cpp}`)

스킬이나 아이템의 재사용 대기 시간을 관리하는 시스템으로, 게임의 밸런싱과 전략적 선택에 중요한 역할을 합니다.

**시작/중지**
`StartCooldown`은 쿨다운을 시작하는 함수로, 스킬을 사용한 직후 호출됩니다. 이 함수는 쿨다운의 총 지속시간을 설정하고, 내부 타이머를 시작하여 시간을 추적합니다. `StopCooldown`은 쿨다운을 강제로 중단시킬 때 사용되며, 특정 아이템이나 효과로 인해 쿨다운이 초기화되는 경우에 활용됩니다.

**감소**
`ReduceCooldown`은 쿨다운 시간을 절대값으로 감소시키는 함수입니다. 예를 들어, 10초 남은 쿨다운에서 3초를 감소시키면 7초가 됩니다. `ReduceCooldownByPercent`는 퍼센트 기반으로 감소시키는 함수로, 10초 남은 쿨다운에서 30%를 감소시키면 7초가 됩니다. 이 기능들은 아이템 효과나 패시브 스킬에서 쿨다운 감소를 구현할 때 유용합니다.

**조회**
`GetTimeRemaining`은 현재 쿨다운에 남은 시간을 반환하며, UI에 표시하거나 다른 시스템에서 참조할 때 사용됩니다. `GetDuration`은 쿨다운의 총 지속시간을 반환하여, 쿨다운 진행률을 계산하거나 UI 게이지를 업데이트할 때 활용됩니다.

#### **내부 구조**
**컨테이너**: `FCooldownContainer`는 FFastArraySerializer를 기반으로 한 컨테이너로, 네트워크 환경에서 효율적인 동기화를 지원합니다. 이 컨테이너는 여러 개의 쿨다운을 동시에 관리할 수 있으며, 서버와 클라이언트 간의 상태 동기화를 자동으로 처리합니다.

**UI 연동**: `UW_ActionIcon`은 액션 아이콘을 표시하는 UI 위젯으로, 쿨다운 트래커의 델리게이트를 구독하여 실시간으로 상태 변화를 감지합니다. 쿨다운이 시작되면 원형 게이지가 나타나고, 시간이 지남에 따라 게이지가 줄어들며, 쿨다운이 완료되면 게이지가 사라지고 아이콘이 다시 활성화됩니다.

---

### **4) 타겟팅 시스템 (Targeting)**

#### **`UTargetingComponent`** (`Targeting/TargetingComponent.{h,cpp}`)

게임에서 적이나 목표물을 찾고 선택하는 시스템으로, 전투와 상호작용의 핵심이 되는 컴포넌트입니다.

**검출 타입**
**오버랩**: `FindOverlapTarget`은 지정된 범위 내에 있는 모든 타겟을 검출합니다. 이 방식은 원형이나 구형 범위 공격에 적합하며, 범위 내의 모든 적을 동시에 찾아낼 수 있습니다. 검출된 타겟들은 거리순으로 정렬되어 우선순위를 결정하는 데 활용됩니다.

**다중 라인**: `FindMultiLineTraceTarget`은 여러 방향으로 동시에 레이 트레이스를 수행하여 타겟을 검출합니다. 이 방식은 부채 모양이나 크로스 형태의 공격에 적합하며, 각 방향에서 가장 가까운 타겟을 찾아낼 수 있습니다. 검출 정확도가 높지만 계산 비용이 상대적으로 높습니다.

**스윕**: `FindSweepTraceTarget`은 구체나 캡슐 모양의 콜리전을 사용하여 타겟을 검출합니다. 이 방식은 이동하면서 검출하는 경우나, 캐릭터의 크기를 고려한 정확한 검출이 필요한 경우에 적합합니다. 레이 트레이스보다 더 정확한 물리적 상호작용을 시뮬레이션할 수 있습니다.

#### **우선순위 시스템**
**가중치 기반**: `SelectBestTargetByPriority`는 여러 요소를 종합적으로 고려하여 최적의 타겟을 선택합니다. 거리가 가까울수록, 각도가 정면에 있을수록, 크기가 적절할수록 높은 점수를 받아 우선순위가 결정됩니다. 이 시스템은 플레이어의 의도를 가장 잘 반영하는 타겟을 자동으로 선택할 수 있게 해줍니다.

**필터**: 다양한 조건에 따라 타겟을 걸러내는 시스템들입니다. `DistanceTargetFilter`는 최소/최대 거리를 설정하여 너무 가깝거나 먼 타겟을 제외합니다. `HealthTargetFilter`는 체력이 특정 범위 내에 있는 타겟만 선택하거나, 체력이 낮은 타겟을 우선적으로 선택할 수 있게 합니다. `TeamTargetFilter`는 같은 팀이나 적대 팀을 구분하여 적절한 타겟만 선택합니다. `VisibilityTargetFilter`는 시야에 보이지 않는 타겟을 제외하여 투명이나 은신 상태의 적을 무시할 수 있게 합니다.

#### **추가 기능**
**오버레이 머티리얼**: 검출된 타겟들을 시각적으로 구분할 수 있게 해주는 시스템입니다. 적대적인 타겟은 빨간색, 우호적인 타겟은 파란색, 중립적인 타겟은 노란색 등의 머티리얼을 적용하여 플레이어가 쉽게 구분할 수 있게 합니다. 이 기능은 디버깅이나 개발 중에 특히 유용합니다.

**네트워킹**: 멀티플레이어 환경에서 타겟팅 상태를 동기화하는 RPC 함수들입니다. `Server_SetDetectionType`은 서버에서 검출 타입을 변경할 때 호출되며, `Client_ConfirmDetectionTypeChange`는 클라이언트에서 변경 사항을 확인하고 적용할 때 호출됩니다. 이를 통해 서버와 클라이언트 간의 타겟팅 상태가 일치하도록 보장합니다.

---

### **5) 카메라 시스템 (Camera)**

#### **`ULuxCameraComponent`** (`Camera/LuxCameraComponent.{h,cpp}`)

게임의 시각적 경험을 담당하는 카메라 시스템으로, 다양한 상황에 맞는 카메라 동작을 모듈화하여 관리합니다.

**모드 스택**
`PushCameraMode`와 `PopCameraMode`를 통해 카메라 모드를 스택 형태로 관리합니다. 이는 여러 카메라 모드가 중첩되어 동작할 수 있게 해주며, 예를 들어 기본 카메라 모드 위에 전투 카메라 모드가, 그 위에 특수 스킬 카메라 모드가 쌓일 수 있습니다.

각 카메라 모드는 `ULuxCameraMode`를 상속받아 구현되며, 자신만의 고유한 로직을 가집니다. 카메라 모드가 푸시되면 해당 모드의 `OnActivation`이 호출되어 초기화를 수행하고, 팝되면 `OnDeactivation`이 호출되어 정리 작업을 수행합니다. 이 시스템을 통해 상황에 맞는 카메라 전환이 자연스럽게 이루어집니다.

**블렌딩**
`StartCameraBlend`는 두 카메라 모드 간의 부드러운 전환을 시작하는 함수입니다. 이 함수는 전환 시간, 블렌딩 곡선, 전환 방식 등을 매개변수로 받아 카메라의 위치, 회전, FOV 등을 점진적으로 변화시킵니다.

`UpdateCameraBlending`은 블렌딩이 진행되는 동안 매 프레임마다 호출되어 현재 블렌딩 진행률에 따라 카메라 상태를 업데이트합니다. 이 과정에서 보간 함수를 사용하여 자연스러운 전환을 구현하며, 블렌딩이 완료되면 자동으로 다음 카메라 모드로 전환됩니다.

#### **포커스 모드: `ULuxCameraMode_Focus`**
**목표 추적**: 이 모드는 특정 목표물에 집중하는 카메라 동작을 구현합니다. 목표물이 이동하면 카메라가 자동으로 따라가며, 목표물이 화면 중앙에 유지되도록 시야를 보정합니다. 이는 보스 전투나 중요한 NPC와의 상호작용에서 플레이어가 목표물을 놓치지 않도록 도와줍니다.

**타겟팅 연동**: 타겟팅 컴포넌트와 연동하여 현재 선택된 타겟을 자동으로 추적합니다. 플레이어가 새로운 타겟을 선택하면 카메라가 즉시 해당 타겟으로 시점을 전환하고, 타겟이 사라지거나 변경되면 적절한 카메라 동작을 수행합니다. 이 연동을 통해 전투 중에도 자연스러운 카메라 움직임을 제공할 수 있습니다.

---

### **6) 큐 시스템 (Gameplay Cues)**

#### **매니저/노티파이**: `ULuxCueManager`, `ALuxCueNotify` (`Cues/*`)

게임플레이 큐 시스템은 시각적, 청각적 피드백을 제공하는 시스템으로, 플레이어에게 게임의 상태 변화를 직관적으로 전달합니다.

**Cue 실행/정지/풀링**: `ULuxCueManager`는 모든 큐의 생명주기를 중앙에서 관리합니다. 큐가 실행되면 적절한 노티파이 액터를 생성하거나 기존 것을 재사용하여 효과를 표시합니다. 큐가 완료되면 자동으로 정지하고, 풀링 시스템을 통해 메모리 효율성을 높입니다. 이 시스템은 많은 수의 큐가 동시에 실행되어도 성능 저하를 최소화합니다.

**액터별/태그별 큐 카운트**: 각 액터가 현재 실행 중인 큐의 개수를 추적하여, 중복 실행을 방지하거나 큐의 우선순위를 조정할 수 있습니다. 태그별로도 큐를 분류하여, 예를 들어 "얼음" 관련 큐나 "폭발" 관련 큐의 개수를 별도로 관리할 수 있습니다. 이를 통해 게임의 시각적 복잡도를 제어하고 성능을 최적화할 수 있습니다.

#### **Aurora 샘플 Cue**

**Cue_Aurora_*** 태그 그룹은 Aurora 캐릭터의 모든 능력에 대한 시각적 피드백을 제공합니다.

**Hoarfrost_FreezeSegments**: 장판 동결 능력의 시각적 표현으로, 얼음 장판이 생성되는 과정과 적들이 동결되는 효과를 담당합니다. 장판의 경계선, 얼음의 크리스탈 패턴, 동결되는 적들의 얼음 효과 등이 단계별로 표시되어 능력의 진행 상황을 명확하게 보여줍니다.

**GlacialCharge_FrostShield**: 얼음 방패 능력의 시각적 표현으로, Aurora 주변에 얼음 방패가 생성되고 유지되는 효과를 담당합니다. 방패의 투명도, 얼음의 질감, 파손 효과 등이 상황에 따라 동적으로 변화하여 능력의 상태를 직관적으로 전달합니다.

**FrozenSword_Primary**: 기본 공격의 시각적 표현으로, 검의 얼음 효과와 공격 궤적을 담당합니다. 공격 방향에 따른 얼음 파편의 분산, 적중 시 얼음 효과, 크리티컬 시 특별한 시각적 피드백 등을 제공합니다.

**Ultimate_***: 궁극기 관련 큐들로, 각각의 궁극기에 맞는 특별한 시각적 효과를 담당합니다. 화면 효과, 파티클 시스템, 카메라 셰이크 등이 조합되어 궁극기의 위력과 특별함을 강조합니다.

---

### **7) 시스템/유틸리티**

**에셋 관리**: `ULuxAssetManager`
게임에 필요한 모든 데이터를 효율적으로 관리하는 시스템으로, 성능 최적화와 메모리 관리를 담당합니다.

이 매니저는 게임 시작 시 필요한 데이터들을 사전에 로드하여 캐시에 저장합니다. 게임 데이터(캐릭터 정보, 스킬 데이터 등), 큐(시각적 효과), 이펙트(능력 효과), CC 데이터(군중제어 정보) 등을 체계적으로 관리하여 런타임에 빠른 접근이 가능하도록 합니다. 또한 메모리 사용량을 모니터링하고, 필요에 따라 사용하지 않는 데이터를 해제하여 최적의 성능을 유지합니다.

**태그 스택**: `FGameplayTagStackContainer`
게임플레이 태그를 스택 형태로 관리하는 시스템으로, 같은 태그가 여러 번 적용될 때 중첩 횟수를 추적합니다.

이 시스템은 상태 효과(버프/디버프), 차단 효과(방어막, 무적), 콤보 카운터 등에 활용됩니다. 예를 들어, 공격력 증가 버프가 여러 번 적용되면 스택 카운트가 증가하고, 각 스택마다 효과가 누적됩니다. 스택이 제거되면 해당 효과도 함께 제거되며, 네트워크 환경에서는 이 정보가 자동으로 동기화되어 모든 클라이언트에서 일관된 상태를 유지합니다.

**로그 채널**: `LogLux*` (`LuxLogChannels.*`)
게임의 각 시스템별로 로그를 분류하여 디버깅과 모니터링을 지원하는 시스템입니다.

`LogLux`는 전체 Lux 프레임워크의 일반적인 로그를 담당하며, `LogLuxActionSystem`은 액션 시스템의 동작 과정을 상세히 기록합니다. `LogLuxCooldown`은 쿨다운 시스템의 상태 변화와 타이밍을 추적하고, `LogLuxCombat`은 전투 관련 모든 이벤트(데미지, CC 적용, 스킬 사용 등)를 기록합니다. 이러한 로그들은 개발 중 문제 해결과 런타임 성능 분석에 매우 유용하며, 플레이어의 게임플레이 분석에도 활용할 수 있습니다.

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


