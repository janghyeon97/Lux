

#pragma once

#include "NativeGameplayTags.h"

namespace LuxGameplayTags
{
	LUX_API		FGameplayTag FindTagByString(const FString& TagString, bool bMatchPartialString = false);

#pragma region Stat Tags
	LUX_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Stat_Offense_AttackDamage);
	LUX_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Stat_Offense_AbilityPower);
#pragma endregion


#pragma region Action Tags
	/** @brief 액션의 종류를 나타냅니다. */
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Action_Type_Attack);                   // 기본 공격
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Action_Type_Ability);                  // 모든 능력 (스킬)
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Action_Type_Ability_Normal);           // 일반 능력
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Action_Type_Ability_Ultimate);         // 궁극 능력
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Action_Type_Ability_Movement);         // 이동 능력
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Action_Type_Ability_Damaging);         // 피해 능력
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Action_Type_Ability_CrowdControl);     // 군중 제어 능력
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Action_Type_Ability_Buff);             // 버프 능력
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Action_Type_Ability_Debuff);           // 디버프 능력
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Action_Type_Utility);                  // 유틸리티
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Action_Type_Utility_Cleanse);          // CC 해제
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Action_Type_Utility_Item);             // 아이템
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Action_Type_Utility_Summon);           // 소환

	/** @brief 액션 활성화 실패 이유를 나타냅니다. */
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Action_Fail_IsDead);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Action_Fail_Cooldown);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Action_Fail_Cost);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Action_Fail_Cost_Mana);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Action_Fail_Cost_Health);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Action_Fail_AdditionalCost);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Action_Fail_AlreadyActive);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Action_Fail_TargetInvalid);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Action_Fail_TagsMissing);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Action_Fail_TagsBlocked);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Action_Fail_SourceTags);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Action_Fail_Silenced);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Action_Fail_Networking);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Action_Fail_ActivationGroup);

	/** @brief 액션의 특별한 동작 방식을 나타냅니다. */
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Action_Behavior_SurvivesDeath);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Action_Behavior_CannotBeCancelled);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Action_State_CancelableByMovement);

	/** @brief 일반적인 쿨다운 카테고리를 나타내는 부모 태그입니다. */
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Action_Cooldown);

	/** @brief 액션의 쿨다운을 나타냅니다. */
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Action_Cooldown_Primary);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Action_Cooldown_Secondary);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Action_Cooldown_Tertiary);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Action_Cooldown_Quaternary);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Action_Cooldown_Ultimate);

	/** @brief 액션의 스택 수를 나타냅니다. (콤보 카운트가 아닌 충전 수)*/
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Action_Stack_Primary);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Action_Stack_Secondary);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Action_Stack_Tertiary);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Action_Stack_Quaternary);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Action_Stack_Ultimate);

	/** @brief 액션의 연속 사용 횟수를 나타냅니다. (한 번 사용 시 최대 재사용 가능 횟수)*/
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Action_MultiCast_Primary);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Action_MultiCast_Secondary);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Action_MultiCast_Tertiary);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Action_MultiCast_Quaternary);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Action_MultiCast_Ultimate);

	/** @brief 콤보 카운트를 나타냅니다. */
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Action_Combo_Primary);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Action_Combo_Secondary);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Action_Combo_Tertiary);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Action_Combo_Quaternary);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Action_Combo_Ultimate);
	
#pragma endregion


#pragma region Input Tags
	/** @brief 액션을 발동시키는 입력입니다. */
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Action_Primary);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Action_Secondary);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Action_Tertiary);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Action_Quaternary);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Action_Passive);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Action_Ultimate);

	/** @brief 캐릭터의 기본적인 움직임 입력입니다. */
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Move);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Jump);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Look_Mouse);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Look_Stick);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Crouch);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_AutoRun);
	
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Debug_ToggleActionSystem);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_ToggleMouseCursor);
#pragma endregion


#pragma region State Tags
	/** @brief 긍정적인 효과(버프)의 종류(원인)를 나타냅니다. */
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Buff_AttackDamage);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Buff_MoveSpeed);

	/** @brief 캐릭터의 면역 상태를 나타냅니다. */
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Immunity_CrowdControl);         // 모든 CC기 면역
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Immunity_CrowdControl_Slow);    // 둔화 면역

	/** @brief 부정적인 효과(디버프)의 종류(원인)를 나타냅니다. */
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Debuff_DamageOverTime);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Debuff_ArmorShred);

	/** @brief 특정 행동이 '차단되었음'을 나타내는 최종 결과 태그입니다. */
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Block_Movement);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Block_Rotation);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Block_Action);				// 모든 행동 금지
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Block_Attack);				// 기본 공격 금지
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Block_Ability);			// 스킬 사용 금지
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Block_DamageImmune);

	/** @brief 캐릭터의 일반적인 상태를 나타냅니다. */
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Generic_Alive);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Generic_Dying);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Generic_Dead);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Generic_InCombat);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Generic_Invulnerable);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Generic_Untargetable);

	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Movement_IsMoving);
#pragma endregion


#pragma region Event Tags
	/** @brief 액션과 관련된 이벤트입니다. */
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Action_Used);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Action_Landed); // 스킬 적중
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_BasicAttack_Hit); // 기본 공격 적중

	/** @brief 캐릭터의 상태 변화와 관련된 이벤트입니다. */
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Character_Damaged);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Character_Healed);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Character_Killed);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Character_Died);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Character_DealtDamage);				// 캐릭터가 데미지를 입힘
	

	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Movement_Started);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Movement_Stopped);

	/** @brief 군중 제어(CC)와 관련된 이벤트입니다. */
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_CrowdControl);
	LUX_API 	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_CrowdControl_Applied);
	LUX_API 	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_CrowdControl_Removed);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_CrowdControl_Dealt);					// 군중 제어 효과 적용
#pragma endregion


#pragma region Task Tags
	/** @brief LuxTask 상태 변화와 관련된 이벤트입니다. */
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Task_Event_Completed);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Task_Event_Canceled);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Task_Event_Failed);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Task_Event_Montage_NotifyBegin);			// 몽타주 노티파이 시작 발생
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Task_Event_Montage_NotifyEnd);			// 몽타주 노티파이 끝 발생
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Task_Event_Montage_Interrupted);			// 몽타주 Interrupted 발생
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Task_Event_Montage_BlendOut);			// 몽타주 BlendOut 발생
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Task_Event_Montage_Ended);				// 몽타주 Ended 발생
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Task_Event_Montage_AlreadyPlaying);		// 몽타주가 이미 재생 중
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Task_Event_TargetData_Ready);			// 타겟 데이터 준비 완료
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Task_Event_TargetData_Cancelled);		// 타겟팅 취소
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Task_Event_Input_Pressed);				// 입력 눌림
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Task_Event_Input_Released);				// 입력 떼어짐
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Task_Event_GameplayEvent_Received);		// 게임플레이 이벤트 수신
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Task_Event_Delay_Finished);				// 딜레이 종료
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Task_Event_PhaseDelay_Finished);			// 페이즈 딜레이 종료
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Task_Event_Overlap_Occurred);			// 오버랩 발생
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Task_Event_Attack_HitPredicted);			// 예측 공격 히트 발생
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Task_Event_Attack_HitVerified);			// 예측 공격 실행 완료

	/** @brief Aurora 캐릭터의 Task 관련된 이벤트입니다. */
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Task_Event_Path_Ready);					// 경로 생성 완료
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Task_Event_Path_Failed);					// 경로 생성 실패
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Task_Event_Path_Predicted);				// 클라이언트에서 경로 예측 완료
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Task_Event_Path_ServerValidated);		// 서버에서 경로 검증 완료
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Task_Event_FollowPath_Completed);		// 경로 따라가기 완료
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Task_Event_FollowPath_Failed);			// 경로 따라가기 실패
#pragma endregion


#pragma region CrwodControl Tags
	/** @brief CC 효과의 종류를 나타냅니다. */
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(CrowdControl_Stun);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(CrowdControl_Frozen);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(CrowdControl_Snare);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(CrowdControl_Silence);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(CrowdControl_Slow);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(CrowdControl_Airborne);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(CrowdControl_Taunt);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(CrowdControl_Fear);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(CrowdControl_Blind);
#pragma endregion


#pragma region Effect Tags
	/** @brief 이펙트의 종류를 나타냅니다. */
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Type_Physical);									// 물리 데미지 타입
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Type_Magical);									// 마법 데미지 타입
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Type_Critical);									// 치명타 데미지 타입
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Type_True);										// 고정 데미지 타입
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Type_CrowdControl);								// 군중제어 효과 타입
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Type_Buff);										// 강화 효과 타입
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Type_Debuff);										// 약화 효과 타입
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Type_Cooldown);									// 쿨다운 효과 타입

	/** @brief 이펙트의 SetByCaller 태그들입니다. */
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_SetByCaller_Cost);								// 스킬 비용
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_SetByCaller_Duration);							// 지속 시간
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_SetByCaller_Magnitude);							// 효과 강도
	
	/** @brief 데미지 계산 입력 데이터 */
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_SetByCaller_Physical_Base);						// 물리 데미지 기본값 (에: 액션의 기본 물리 데미지)
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_SetByCaller_Physical_Scale);						// 물리 데미지 계수 (예: 액션의 캐릭터 공격력 계수)
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_SetByCaller_Magical_Base);						// 마법 데미지 기본값 (예: 액션의 기본 마법 데미지)
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_SetByCaller_Magical_Scale);						// 마법 데미지 계수 (예: 액션의 캐릭터 주문력 계수)
	
	/** @brief 데미지 계산 중간 결과 */
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_SetByCaller_Physical_Raw);						// 물리 데미지 원시값 (예: 방어력 계산 전 물리데미지)
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_SetByCaller_Magical_Raw);							// 마법 데미지 원시값 (예: 마법 방어력 계산 전 마법 데미지)
	
	/** @brief 데미지 계산 최종 결과 */
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_SetByCaller_Physical_Final);						// 물리 데미지 최종값
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_SetByCaller_Magical_Final);						// 마법 데미지 최종값
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_SetByCaller_Damage_Total);						// 총 데미지 (물리데미지 + 마법데미지)
	
	/** @brief 치명타 관련 SetByCaller 태그들 */
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_SetByCaller_Critical_Chance);						// 치명타 확률
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_SetByCaller_Critical_Multiplier);					// 치명타 배율
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_SetByCaller_Critical_IsCritical);					// 치명타 여부
	
	/** @brief 데미지 계산 상세 정보 */
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_SetByCaller_Damage_Source_AttackDamage);			// 공격력 (시전자의 공격력)
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_SetByCaller_Damage_Source_AbilityPower);			// 주문력 (시전자의 주문력)
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_SetByCaller_Damage_Target_Armor);					// 방어력 (피해 대상의 방어력)
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_SetByCaller_Damage_Target_MagicResistance);		// 마법저항력 (피해 대상의 마법 저항력)
 	
	/** @brief 데미지 계산 공식 관련 */
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_SetByCaller_Damage_Formula_Physical_Mitigation);	// 물리 데미지 감소율 (피해 대상의 방어력에 의한 감소율)
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_SetByCaller_Damage_Formula_Magical_Mitigation);	// 마법 데미지 감소율 (피해 대상의 마법 저항력에 의한 감소율)
#pragma endregion


#pragma region Init Tags
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(InitState_Spawned);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(InitState_DataAvailable);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(InitState_DataInitialized);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(InitState_GameplayReady);
#pragma endregion

	// These are mappings from MovementMode enums to GameplayTags associated with those enums (below)
	LUX_API		extern const TMap<uint8, FGameplayTag> MovementModeTagMap;
	LUX_API		extern const TMap<uint8, FGameplayTag> CustomMovementModeTagMap;

	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Movement_Mode_Walking);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Movement_Mode_NavWalking);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Movement_Mode_Falling);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Movement_Mode_Swimming);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Movement_Mode_Flying);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Movement_Mode_Custom);
}