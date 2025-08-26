# Lux

**Unreal Engine 5로 구현한 데이터 기반의 확장 가능한 3인칭 액션 시스템 프로토타입**

[Lux 프로젝트 플레이 영상 GIF]

## 소개

**Lux**는 이전 프로젝트를 통해 얻은 피드백과 기술적 한계를 극복하고자 새롭게 시작한 개인 포트폴리오 프로젝트입니다. 언리얼 엔진의 게임플레이 어빌리티 시스템(GAS)에서 영감을 받아, 데이터 기반 설계를 통해 확장성과 유지보수성을 극대화한 자체 액션 시스템을 구축하는 것을 목표로 했습니다.

이 프로젝트는 단순히 또 하나의 게임을 만드는 것이 아니라, "클라이언트 개발자의 기본기가 부족하다", "디자이너가 만든 포트폴리오 같다"는 피드백을 바탕으로 견고한 아키텍처를 설계하고 기술적 완성도를 증명하는 과정에 집중한 결과물입니다.

## 핵심 기능

-   **컴포넌트 기반 액션 시스템:** `UActionSystemComponent`를 중심으로 행동(`ULuxAction`), 상태(`ULuxAttributeSet`), 효과(`ULuxEffect`)의 역할을 명확히 분리하여, 객체지향 원칙에 기반한 독립적이고 확장 가능한 구조를 설계했습니다.

-   **데이터 기반 스킬 설계 (액션 페이즈 시스템):** 복잡한 스킬 로직을 `Phase`(상태)와 `Behavior`(전략) 단위로 분리하고, 이를 데이터 애셋에서 조합하는 **전략 패턴**을 적용했습니다. 이를 통해 코드 수정 없이 스킬의 흐름과 효과를 유연하게 변경할 수 있습니다.

-   **네트워크 동기화 (클라이언트 예측):** `PredictionKey`를 이용한 클라이언트 예측 및 서버 보정 모델을 구현하여, 네트워크 지연 환경에서도 쾌적한 조작감과 데이터 일관성을 모두 확보했습니다. 또한, 액션의 특성에 따라 `Instancing Policy`를 다르게 적용하여 시스템을 최적화했습니다.

-   **완전 자동화 데이터 파이프라인:** 기획자가 데이터 테이블에 `[@Damage + {AD} * @ADScale]`와 같은 수식을 직접 입력하면, 게임 클라이언트에서 이를 직접 파싱하여 툴팁 UI에 동적으로 반영하는 `ExpressionEvaluator`를 직접 구현했습니다.

-   **독립적인 상호작용 시스템:**
    -   **이벤트 구독 시스템 (옵저버 패턴):** 시스템 간의 직접적인 참조를 제거하여, 기능의 독립성과 확장성을 확보했습니다.
    -   **스택 기반 카메라:** `TargetingComponent`와 연동하여, 평상시와 전투 시의 카메라 모드를 자연스럽게 전환하는 동적인 연출을 구현했습니다.

## 기술적 상세 설명

### 1. 액션 페이즈 시스템 (Action Phase System)

`ULuxAction`은 상태 머신으로 설계되었으며, `Phase`는 스킬의 각 단계를 나타내는 '상태(State)'의 역할을 합니다. 각 `Phase`에서 수행될 구체적인 행동들은 `FPhaseBehaviorBase`를 상속받는 '전략(Strategy)' 객체로 모듈화했습니다.

특히, 언리얼 엔진의 `FInstancedStruct`를 활용하여 이 `Behavior`들을 데이터 애셋에서 직접 조합할 수 있도록 구현함으로써, 프로그래머의 개입 없이 기획자가 스킬 로직을 구성할 수 있는 유연성을 확보했습니다.

```cpp
// ULuxActionPhaseData.h - 데이터 애셋에서 Phase와 Behavior를 설정
UPROPERTY(EditDefaultsOnly, Category = "Phase")
TArray<FInstancedStruct> OnEnterBehaviors;

UPROPERTY(EditDefaultsOnly, Category = "Phase")
TArray<FInstancedStruct> OnExitBehaviors;
