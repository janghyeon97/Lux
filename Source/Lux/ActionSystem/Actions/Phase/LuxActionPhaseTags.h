#pragma once

#include "NativeGameplayTags.h"

namespace LuxPhaseTags
{
	// ## 기본 액션 생명주기 (Core Action Lifecycle)
	LUX_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Phase_Action_Begin);
	LUX_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Phase_Action_End);
	LUX_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Phase_Action_Interrupt);

	// ## 시전 및 실행 (Casting & Execution)
	LUX_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Phase_Action_Precast);				// 선딜레이 (시전 준비 동작)
	LUX_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Phase_Action_Execute);				// 핵심 로직 실행 (효과 발동)
	LUX_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Phase_Action_Channeling);			// 채널링 (지속 시전)
	LUX_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Phase_Action_Postcast);				// 실행 후 고정 동작
	LUX_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Phase_Action_Recovery);				// 후딜레이 (캔슬 가능)

	// ## 전투 및 타격 (Combat & Targeting)
	LUX_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Phase_Action_Aiming);				// 조준
	LUX_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Phase_Action_HitCheck);				// 타격 판정
	LUX_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Phase_Action_ProcessHit);			// 타격 처리
	LUX_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Phase_Action_WaitForServer);			// 서버 액션 실행 대기
	LUX_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Phase_Action_PredictHit);			// 클라이언트 예측 타겟 검출
	LUX_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Phase_Action_ProjectileLaunch);		// 투사체 발사

	// ## 콤보 및 재시전 (Combo & Recast)
	LUX_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Phase_Action_WaitForNextInput);		// 다음 입력 대기
	LUX_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Phase_Action_ResetCombo);			// 콤보 초기화
	LUX_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Phase_Action_WaitRecast);			// 재시전 대기
	LUX_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Phase_Action_Recast);				// 재시전 실행

	// ## 이동 (Movement)
	LUX_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Phase_Action_Pathfinding);			// 경로 탐색
	LUX_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Phase_Action_Dash);					// 돌진
	LUX_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Phase_Action_Leap);					// 도약
	LUX_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Phase_Action_Landing);				// 착지

	// ## 기타 (Miscellaneous)
	LUX_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Phase_Action_Summon);				// 소환수 생성
	LUX_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Phase_Action_Empower);				// 자기 강화 (버프)
}