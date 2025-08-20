

#include "ActionSystem/Actions/Phase/LuxActionPhaseTags.h"


namespace LuxPhaseTags
{
	// ## 기본 액션 생명주기 (Core Action Lifecycle)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Phase_Action_Begin, "Phase.Action.Begin", "액션의 시작 페이즈입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Phase_Action_End, "Phase.Action.End", "액션이 모든 로직을 마치고 완전히 종료되는 페이즈입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Phase_Action_Interrupt, "Phase.Action.Interrupt", "액션이 외부 요인에 의해 중단되었을 때의 정리 페이즈입니다.");

	// ## 시전 및 실행 (Casting & Execution)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Phase_Action_Precast, "Phase.Action.Precast", "액션의 핵심 로직이 실행되기 전의 선딜레이(준비 동작) 페이즈입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Phase_Action_Execute, "Phase.Action.Execute", "액션의 핵심 로직(데미지, 효과 생성 등)을 실행하는 페이즈입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Phase_Action_Channeling, "Phase.Action.Channeling", "사용자가 입력을 유지하는 동안 지속적으로 효과가 발동되는 채널링 페이즈입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Phase_Action_Postcast, "Phase.Action.Postcast", "핵심 로직 실행 직후, 후딜레이 전의 고정 동작 페이즈입니다. (예: 총기 반동)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Phase_Action_Recovery, "Phase.Action.Recovery", "액션의 후딜레이 상태입니다. 이 상태는 다른 행동으로 캔슬될 수 있습니다.");

	// ## 전투 및 타격 (Combat & Targeting)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Phase_Action_Aiming, "Phase.Action.Aiming", "사용자가 스킬의 목표 지점이나 대상을 지정하는 조준 페이즈입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Phase_Action_HitCheck, "Phase.Action.HitCheck", "타격 판정을 수행하는 범용 구간입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Phase_Action_ProcessHit, "Phase.Action.ProcessHit", "타격 성공 후 데미지 및 각종 효과를 처리하는 범용 페이즈입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Phase_Action_WaitForServer, "Phase.Action.WaitForServer", "클라이언트에서 서버 액션 실행 완료를 기다리는 페이즈입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Phase_Action_PredictHit, "Phase.Action.PredictHit", "클라이언트에서 타겟을 검출하고 서버에 예측 타격을 요청하는 페이즈입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Phase_Action_ProjectileLaunch, "Phase.Action.ProjectileLaunch", "투사체를 발사하는 특정 시점을 나타내는 페이즈입니다.");

	// ## 콤보 및 재시전 (Combo & Recast)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Phase_Action_WaitForNextInput, "Phase.Action.WaitForNextInput", "콤보 등 다음 입력을 기다리는 범용 상태입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Phase_Action_ResetCombo, "Phase.Action.ResetCombo", "콤보를 초기화하는 범용 상태입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Phase_Action_WaitRecast, "Phase.Action.WaitRecast", "스킬의 재시전(Recast) 입력을 기다리는 범용 페이즈입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Phase_Action_Recast, "Phase.Action.Recast", "스킬을 재시전하여 추가 효과를 발동시키는 범용 페이즈입니다.");

	// ## 이동 (Movement)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Phase_Action_Pathfinding, "Phase.Action.Pathfinding", "이동할 경로를 탐색하고 생성하는 범용 페이즈입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Phase_Action_Dash, "Phase.Action.Dash", "생성된 경로 또는 특정 방향으로 돌진하는 범용 페이즈입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Phase_Action_Leap, "Phase.Action.Leap", "특정 지점으로 도약하는 범용 페이즈입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Phase_Action_Landing, "Phase.Action.Landing", "공중에서 땅으로 착지하는 범용 페이즈입니다.");

	// ## 기타 (Miscellaneous)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Phase_Action_Summon, "Phase.Action.Summon", "소환수, 토템, 포탑 등 다른 액터를 생성하는 페이즈입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Phase_Action_Empower, "Phase.Action.Empower", "다음 행동을 강화하거나 자신에게 버프를 거는 페이즈입니다.");
}