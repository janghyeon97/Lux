
#include "ActionSystem/ActionSystemGlobals.h"
#include "ActionSystem/ActionSystemComponent.h."
#include "ActionSystem/ActionSystemInterface.h"
#include "LuxActionSystemTypes.h"

UActionSystemGlobals* UActionSystemGlobals::SingletonInstance = nullptr;
bool UActionSystemGlobals::bIsInitialized = false;
FCriticalSection UActionSystemGlobals::SingletonCritialSection;


UActionSystemGlobals& UActionSystemGlobals::Get()
{
	if (bIsInitialized)
	{
		return *SingletonInstance;
	}

	FScopeLock Lock(&SingletonCritialSection);
	if (bIsInitialized)
	{
		return *SingletonInstance;
	}

	SingletonInstance = NewObject<UActionSystemGlobals>(GetTransientPackage(), UActionSystemGlobals::StaticClass());
	SingletonInstance->Init();

	bIsInitialized = true;
	return *SingletonInstance;
}

bool UActionSystemGlobals::IsValid()
{
	return SingletonInstance != nullptr;
}

void UActionSystemGlobals::Init()
{
	// 전역 클래스 초기화 시 필요한 로직을 여기에 추가합니다.
	// 예: 특정 데이터 테이블 로드, 기본 설정 값 캐싱 등
	UE_LOG(LogTemp, Log, TEXT("ActionSystemGlobals has been initialized."));
}

void UActionSystemGlobals::BroadcastGameplayEventToActor(AActor* TargetActor, FGameplayTag EventTag, const FContextPayload& Payload)
{
	if (!TargetActor || !EventTag.IsValid())
	{
		return;
	}

	IActionSystemInterface* ASCInterface = Cast<IActionSystemInterface>(TargetActor);
	if (ASCInterface)
	{
		if (UActionSystemComponent* ASC = ASCInterface->GetActionSystemComponent())
		{
			ASC->HandleGameplayEvent(EventTag, Payload);
		}
	}
	else
	{
		if (UActionSystemComponent* ASC = TargetActor->FindComponentByClass<UActionSystemComponent>())
		{
			ASC->HandleGameplayEvent(EventTag, Payload);
		}
	}
}

void UActionSystemGlobals::BroadcastGameplayEventToAll(const UObject* WorldContextObject, FGameplayTag EventTag, const FContextPayload& Payload)
{
	if (!EventTag.IsValid()) return;

	for (int32 i = ActionSystemComponents.Num() - 1; i >= 0; --i)
	{
		TWeakObjectPtr<UActionSystemComponent> WeakComp = ActionSystemComponents[i];
		if (WeakComp.IsValid())
		{
			WeakComp->HandleGameplayEvent(EventTag, Payload);
		}
		else
		{
			ActionSystemComponents.RemoveAt(i);
		}
	}
}

void UActionSystemGlobals::RegisterComponent(UActionSystemComponent* Component)
{
	if (Component)
	{
		ActionSystemComponents.AddUnique(Component);
	}
}

void UActionSystemGlobals::UnregisterComponent(UActionSystemComponent* Component)
{
	if (Component)
	{
		ActionSystemComponents.Remove(Component);
	}
}