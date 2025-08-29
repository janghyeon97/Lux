# Lux

Lux는 게임 클라이언트 개발 포트폴리오를 위해 제작된 샘플/레퍼런스 프로젝트입니다. 데이터 주도 액션 시스템과 태그 기반의 전투, 효과, 타겟팅 파이프라인을 제공하며, "Aurora" 캐릭터의 다양한 액션을 통해 그 기능을 선보입니다.

---

## 목차

- [아키텍처](#아키텍처)

- [Aurora액션 GIF](#Aurora-액션-GIF)

- [핵심 시스템](#핵심-시스템)

- [데이터 주도 워크플로우](#데이터-주도-워크플로우)

- [네트워킹 및 예측](#네트워킹-및-예측)

- [프로젝트 구조](#프로젝트-구조)

- [시작하기](#시작하기)

- [빌드 및 실행](#빌드-및-실행)

- [입력 가이드](#입력-가이드)

- [새 액션 추가하기](#새-액션-추가하기)

---

## 아키텍처

### **1. 데이터 주도 설계 (Data-Driven Design)**
> "프로그래머의 개입 없이 기획자가 게임을 만든다."

Lux는 액션의 흐름(Phase 전환 규칙), 각종 수치(DataTable), 그리고 UI 툴팁까지 코드와 데이터를 분리하였습니다.</br>
이를 통해 기획자는 프로그래머의 도움 없이 직접 게임 로직과 밸런스를 수정하며, 빠르고 유연하게 프로토타입을 제작할 수 있습니다.

### **2. 모듈성 및 책임 분리  (Modularity & SRP)**
> "각 부품은 하나의 역할만 완벽하게 수행한다."

Lux의 시스템은 각자의 역할이 명확하게 구분되어 있습니다. </nr>
예를 들어, ULuxAction은 액션의 전체적인 흐름과 상태를 관리하고, ULuxEffect는 데미지나 버프 같은 액션의 결과를 처리하며, ULuxActionTask는 애니메이션 재생이나 이동과 같은 개별 작업을 수행합니다.

### **3. 네트워크 우선 설계 (Network-First Design)**
> "모든 기능은 멀티플레이어 환경을 가정하고 설계한다."

서버가 모든 핵심 로직의 권위를 가지는 서버-클라이언트 모델을 기반으로 하며, PredictionKey를 활용한 클라이언트 예측과 Re-Home 동기화 메커니즘을 통해 네트워크 지연(Latency)으로 인한 불편함을 최소화했습니다. 이를 통해 플레이어는 쾌적하고 부드러운 멀티플레이 경험을 즐길 수 있습니다.

---

## Aurora 액션 GIF

- ### Hoarfrost
  ![Hoarfrost](Docs/Media/Hoarfrost.gif)
  > 자신의 주변 땅에 서리 고리를 생성합니다. 고리에 닿은 적은 능력 피해를 입고 0.75~1.5초 동안 속박에 걸립니다.

- ### Glacial Charge
  ![Glacial Charge](Docs/Media/Glacial_Charge.gif)
  > 앞으로 질주하며 다른 영웅들이 걸을 수 있는 길을 남깁니다. 7초 안에 재사용 시 생성된 길을 파괴할 수 있습니다.

- ### Frozen Simulacrum
  ![Frozen Simulacrum](Docs/Media/Frozen_Simulacrum.gif)
  > 자신이 움직이는 방향으로 도약하며 적 미니언과 타워를 도발하는 얼음 조각상을 남깁니다. 조각상은 일정 시간 유지되며 조각상 주변 적을 둔화시킵니다

- ### Frozen Sword
  ![Frozen Sword](Docs/Media/Frozen_Sword.gif)
  > 검을 휘둘러 피해를 입힙니다.

- ### Cryoseism (궁극기)
  ![Cryoseism](Docs/Media/Cryoseism.gif)
  > 특정 지역을 얼려 1.5초 동안 모든 적의 이동 속도를 감소시킵니다. 이후 얼음이 폭발하여 적을 1초 동안 기절시키고 피해를 입힙니다. 주변 적에게 추가로 폭발 효과가 확산됩니다.

- ### Cryoseism Passive (패시브)
  ![Cryoseism Passive](Docs/Media/Cryoseism_Passive.gif)
  > 기본공격 적중 시 궁극기 쿨다운 감소합니다.

---

## 핵심 시스템

### ActionSystemComponent 구조

![ActionSystemDiagram](Docs/Media/ActionSystem_Diagram.png)

  ActionSystemComponent는 입력, 액션, 효과 등 모든 게임플레이 로직의 흐름을 중앙에서 조율하는 허브 역할을 합니다.</br>
  플레이어의 입력을 받아 해당하는 액션을 찾아 실행하고, 액션의 생명주기 동안 발생하는 모든 이벤트를 관리하며 다른 시스템과 상호작용합니다.

### 1. 액션 시스템 (Action System)
 액션 시스템은 캐릭터의 모든 행동(스킬, 공격, 이동 등)의 생명주기를 관리하고 실행합니다.

#### 1.1 페이즈 시스템
 - 액션의 진행 단계를 `Phase.Action.Begin / Execute / End` 와 같이 GameplayTag 로 정희합니다.
 - 기본 흐름뿐만 아니라, Interrupt, 이동, 재시전과 같은 특수 상황도 페이즈 전환 규칙을 통해 유연하게 처리합니다.

#### 1.2 태스크 시스템
 - 액션을 구성하는 가장 작은 단위의 작업입니다. 
 - PlayMontageAndWait(애니메이션 재생), LeapToLocation(도약), FollowSpline(경로 이동)과 같은 비동기적인 작업들을 태스크로 구현합니다.

### 2. 게임플레이 이펙트 (LuxEffect)

`ULuxEffect`는 게임플레이에서 발생하는 모든 효과(데미지, 힐링, 버프, 디버프, CC 등)를 정의하는 데이터 에셋입니다. <br>
각 효과는 `FLuxEffectSpec` 구조체를 통해 실행 시점의 세부 정보(지속시간, 레벨, 수치, 대상 등)를 관리합니다.

#### 핵심 특징:
1. **SetByCaller 파라미터:**`FLuxEffectSpec::SetByCallerMagnitude(...)`를 통해 런타임에 동적으로 계산된 수치를 효과에 주입할 수 있습니다. <br>
  > 예를 들어 "캐릭터의 공격력 * 1.5 + 100" 같은 수식을 런타임에 평가하여 데미지를 결정합니다.
2. **태그 기반 시스템**: `GameplayTag`를 활용하여 효과의 종류, 속성, 상호작용을 체계적으로 관리합니다.
3. **스택 관리**: 동일한 효과가 중복 적용될 때의 스택 수와 최대 스택 제한을 설정할 수 있습니다.
4. **지속시간 및 주기**: 즉시 효과와 지속 효과를 모두 지원하며, 주기적으로 적용되는 효과(예: 초당 데미지)도 구현 가능합니다.

### 3. 속성 시스템 (LuxAttributeSet)**

캐릭터의 모든 수치적 특성을 체계적으로 관리하는 시스템입니다.

**속성 세트 구성:**
- **CombatSet**: 공격력, 치명타 확률, 치명타 피해 등 전투 관련 수치
- **ResourceSet**: 체력, 마나, 에너지 등 자원 관련 수치  
- **MovementSet**: 이동 속도, 점프력, 가속도 등 이동 관련 수치
- **DefenseSet**: 방어력, 마법 저항, 회피율 등 방어 관련 수치

**핵심 특징:**
- **서버 권위**: 모든 속성 값의 변경은 서버의 권위를 따르며, 클라이언트는 표시 목적으로만 사용합니다.
- **이펙트 기반 수정**: 속성 값의 변경은 반드시 `ULuxEffect`를 통해 이루어지며, 직접적인 수정은 불가능합니다.
- **수식 기반 계산**: 기본값(BaseValue)과 수정자(Modifier)를 조합하여 최종 값을 계산합니다.
---

## 데이터 주도 워크플로우

- **데이터 테이블**: 액션 레벨별 수치와 파라미터를 DataTable로 관리합니다. (`/Game/Characters/Aurora/DT_Aurora`)

- **InstancedStruct**: 페이즈 전환 조건(`FCondition_NotifyNameEquals`)과 행동을 데이터 에셋으로 구성합니다.

- **SetByCaller 파라미터**: `FLuxEffectSpec::SetByCallerMagnitude(...)`를 통해 런타임에 계산된 수치를 효과에 주입합니다.

- **툴팁 수식**: `UW_ActionTooltip`에서 `{StatName}`, `@ActionData@`, `[Expression]` 같은 형식의 문자열을 파싱하여 동적으로 툴팁을 생성합니다.

---

## 네트워킹 및 예측

- **서버 권위**: 스폰, 데미지, CC, 상태 태그 변화 등 주요 게임 로직은 서버가 주도합니다.

- **클라이언트 예측**: `ULuxActionTask_WaitForServer`와 같은 짧고 보수적인 예측과 Re-Home 메커니즘을 통해 태스크와 페이즈의 소유권을 재조정하여 부드러운 플레이 경험을 제공합니다.

- **효율적인 복제**: `FActiveLuxActionContainer`, `FActiveLuxEffectsContainer` 등은 `FFastArraySerializer`를 기반으로 NetDelta 복제를 사용하여 네트워크 부하를 최소화합니다.

---

## 프로젝트 구조

```
Source/Lux
├── ActionSystem/
│   ├── Actions/      # 액션 정의 (Aurora 예시 포함)
│   ├── Tasks/        # 비동기 작업 단위
│   ├── Effects/      # 게임플레이 효과
│   ├── Attributes/   # 속성 세트
│   └── Targeting/    # 타겟팅 시스템
├── Actors/           # 액션 관련 액터
├── Camera/           # 카메라 컴포넌트 및 모드
├── Character/        # 캐릭터 및 이동 컴포넌트
├── Cues/             # 게임플레이 큐
├── System/           # 핵심 관리자 클래스
└── UI/               # UI 위젯
```

---

## 시작하기

### **빌드 및 실행**

1. **리포지토리 클론**: `git clone <repository_url>`
2. **프로젝트 파일 생성**: `.uproject` 파일을 우클릭하여 "Generate Visual Studio project files"를 선택합니다.
3. **빌드 및 실행**: 에디터 또는 IDE에서 프로젝트를 빌드하고 실행합니다.

---

## 입력 가이드

- **Q / E / R / LMB / RMB**: 액션 사용 (프로필/데이터에 매핑된 액션 실행)
- **B**: 액션, 효과, 태그 상태를 시각적으로 보여주는 디버그 HUD 토글

> 입력은 `LuxGameplayTags::InputTag_*`에 매핑되어 있으며, 프로젝트 설정에서 자유롭게 변경할 수 있습니다.

---

## 새 액션 추가하기

### **1. 클래스 생성**

`ULuxAction`을 상속받는 새 클래스를 생성하고 기본 설정을 추가합니다.

```cpp
ActionIdentifierTag = LuxActionTags::Action_MyAction;
InstancingPolicy    = ELuxActionInstancingPolicy::InstancedPerExecution;
ActivationPolicy    = ELuxActionActivationPolicy::OnInputTriggered;
ActionTags.AddTag(LuxGameplayTags::Action_Type_Ability);
```

### **2. 페이즈 전환 규칙 정의**

애니메이션 노티파이를 기반으로 페이즈를 전환하는 규칙을 추가합니다.

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

### **3. 태스크 조합**

필요한 태스크를 호출하여 액션을 구성합니다.

```cpp
ULuxActionTask_PlayMontageAndWait::PlayMontageAndWait(this, MontageToPlay, 1.0f);
```

---

## 라이선스

이 프로젝트는 포트폴리오 용도로 제작되었으며, 외부 배포 시에는 별도 협의가 필요합니다. 자세한 내용은 문의해 주세요.


