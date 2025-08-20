
#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GameplayTagContainer.h" 
#include "HAL/CriticalSection.h"
#include "ActionSystem/Actions/LuxPayload.h"
#include "ActionSystemGlobals.generated.h"

class AActor;
class UActionSystemComponent;



/**
 * @file ActionSystemGlobals.h
 * @brief 액션 시스템의 전역 설정 및 GameplayEvent 방송을 담당합니다.
 *
 * =====================================================================================
 *
 * [ GameplayEvent 철학: 디커플링(Decoupling)을 통한 유연성 확보 ]
 *
 * GameplayEvent는 코드 간의 직접적인 의존성을 끊어내기 위한 '신호(Signal)' 시스템입니다.
 * 이벤트 송신자(Sender)는 수신자(Receiver)가 누구인지, 무슨 일을 할지 전혀 알 필요가 없습니다.
 * 그저 "특정 상황이 발생했다"는 신호탄(Event)을 월드에 쏘아 올릴 뿐입니다.
 *
 * 이로 인해 시스템의 유연성과 확장성이 극적으로 향상됩니다.
 * 예를 들어, '얼음 화살' 코드를 전혀 수정하지 않고도, 이 화살에 맞았을 때
 * '슬로우 효과'를 '빙결 효과'로 바꾸거나, 특정 몬스터는 오히려 '체력을 회복'하게 만드는 등
 * 다양한 상호작용을 데이터 레벨(Action 애셋)에서 쉽게 추가하고 변경할 수 있습니다.
 *
 * =====================================================================================
 *
 * [ GameplayEvent 기본 사용법 ]
 *
 * 1. 상황 정의 및 태그 생성: 어떤 상황에 대한 이벤트인지 FGameplayTag로 정의합니다.
 * (예: "Event.Combat.CriticalHit")
 *
 * 2. 리스너 액션(Action) 생성: 해당 태그를 수신 대기하는 ULuxAction을 만듭니다.
 * - ULuxAction의 'EventTriggerTags' 배열에 위에서 만든 태그를 추가합니다.
 *
 * 3. 트리거(Trigger) 지점 찾기: 이벤트가 발생해야 하는 코드상의 위치를 찾습니다.
 * - 예: 데미지 계산 로직에서 치명타가 확정되는 순간
 *
 * 4. 이벤트 방송(Broadcast): 트리거 지점에서 UActionSystemGlobals::Get().Broadcast...() 함수를 호출하여
 * 월드에 이벤트를 방송합니다.
 *
 * =====================================================================================
 *
 * [ GameplayEvent 활용 예시 ]
 *
 * - 치명타 발생 시 특수 효과
 * - 이벤트: Event.Combat.Landed.CriticalHit
 * - 효과: 플레이어는 '공격력 증가' 버프를 받고, UI는 'CRITICAL!' 텍스트를 출력합니다.
 *
 * - 상태 이상 시너지
 * - 이벤트: Event.Damage.Taken.Electricity
 * - 조건부 효과: 만약 피격자가 'State.Debuff.Wet'(물에 젖음) 상태라면, '광역 감전' 액션을 추가로 발동합니다.
 *
 * - 퀘스트 아이템 획득
 * - 이벤트: Event.Quest.ItemAcquired.CursedKey (전역 방송)
 * - 효과: 퀘스트 시스템은 퀘스트를 완료 처리하고, 멀리 떨어진 마을의 NPC는 플레이어에 대한 대사를 변경합니다.
 *
 * - 완벽한 방어(패링) 성공
 * - 이벤트: Event.Combat.ParrySuccess
 * - 효과: 적은 '비틀거림' 상태에 빠지고, 플레이어는 '반격' 액션을 사용할 수 있게 됩니다.
 *
 * =====================================================================================
 */
UCLASS(Config = Game, defaultconfig)
class LUX_API UActionSystemGlobals : public UObject
{
	GENERATED_BODY()

public:
	/** 싱글톤 인스턴스를 반환합니다. */
	static UActionSystemGlobals& Get();

	/** 싱글톤 인스턴스가 유효한지 확인합니다. */
	static bool IsValid();

	/** 특정 액터에게 게임플레이 이벤트를 방송(Broadcast)합니다.  */
	UFUNCTION(BlueprintCallable, Category = "ActionSystem|Globals", meta = (DisplayName = "Broadcast Gameplay Event to Actor"))
	virtual void BroadcastGameplayEventToActor(AActor* TargetActor, FGameplayTag EventTag, const FContextPayload& Payload);

	/** 등록된 모든 ActionSystemComponent에 게임플레이 이벤트를 방송합니다. */
	UFUNCTION(BlueprintCallable, Category = "ActionSystem|Globals", meta = (WorldContext = "WorldContextObject", DisplayName = "Broadcast Gameplay Event to All"))
	virtual void BroadcastGameplayEventToAll(const UObject* WorldContextObject, FGameplayTag EventTag, const FContextPayload& Payload);

	/** ActionSystemComponent를 전역 관리 목록에 등록/해제합니다.*/
	void RegisterComponent(UActionSystemComponent* Component);
	void UnregisterComponent(UActionSystemComponent* Component);

protected:
	virtual void Init();

protected:
	static bool bIsInitialized;

	/* ActionSystemGlobals 싱글톤 인스턴스 */
	static UActionSystemGlobals* SingletonInstance;

	/** 활성화된 모든 ActionSystemComponent */
	UPROPERTY()
	TArray<TWeakObjectPtr<UActionSystemComponent>> ActionSystemComponents;

	static FCriticalSection SingletonCritialSection;
};
