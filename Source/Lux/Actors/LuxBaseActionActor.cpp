

#include "Actors/LuxBaseActionActor.h"
#include "ActionSystem/Actions/LuxAction.h"
#include "ActionSystem/Actions/LuxActionLevelData.h"
#include "ActionSystem/ActionSystemComponent.h"


#include "Net/UnrealNetwork.h"

ALuxBaseActionActor::ALuxBaseActionActor()
{
    bReplicates = true;
    bIsActive = false;
}

void ALuxBaseActionActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ALuxBaseActionActor, bIsActive);
}

void ALuxBaseActionActor::BeginPlay()
{
    Super::BeginPlay();

    // 액터가 스폰될 때 bIsActive 상태가 이미 true라면 (늦게 접속한 클라이언트 등)
    if (bIsActive)
	{
		OnRep_IsActive();
	}
}

void ALuxBaseActionActor::Initialize(ULuxAction* Action, bool bAutoStart)
{
    if (!Action)
    {
        return;
    }

    SourceAction = Action;
    ActionIdentifierTag = Action->ActionIdentifierTag;

    FLuxActionSpec* Spec = Action->GetLuxActionSpec();
    if (Spec)
    {
        ActionLevel = Spec->Level;
		ActionSpecHandle = Spec->Handle;
    }
    
    UActionSystemComponent* ASC = Action->GetActionSystemComponent();
    if (ASC)
    {
        SourceASC = ASC;
    }
}

void ALuxBaseActionActor::OnRep_IsActive()
{
    if (bIsActive)
    {
        SetActorTickEnabled(true);
        if (HasAuthority()) // 서버에서만 수명을 설정합니다.
        {
            SetLifeSpan(LifeTime);
        }
    }

    // bIsActive가 동기화될 때 클라이언트에서 처리할 로직 (예: 이펙트 등)
}

void ALuxBaseActionActor::Start()
{
    if (HasAuthority())
    {
        bIsActive = true;
        OnRep_IsActive();
    }
}

void ALuxBaseActionActor::Stop()
{
    // 기본 정지 로직: 필요시 자식에서 오버라이드
    SetActorTickEnabled(false);
    if (HasAuthority())
    {
	    Destroy();
    }
}