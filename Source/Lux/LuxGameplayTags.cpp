
#include "LuxGameplayTags.h"
#include "LuxLogChannels.h"


namespace LuxGameplayTags
{
#include "LuxGameplayTags.h"

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Stat_Offense_AttackDamage, "Stat.Offense.AttackDamage", "공격력 스탯");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Stat_Offense_AbilityPower, "Stat.Offense.AbilityPower", "주문력 스탯");

	/* ======================================== Action Tags ======================================== */

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Action_Type_Attack, "Action.Type.Attack", "기본 공격 타입의 액션입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Action_Type_Ability, "Action.Type.Ability", "모든 능력(스킬) 타입의 최상위 액션입니다. (침묵 등으로 차단)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Action_Type_Ability_Normal, "Action.Type.Ability.Normal", "일반 능력(Q,W,E) 타입의 액션입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Action_Type_Ability_Ultimate, "Action.Type.Ability.Ultimate", "궁극 능력(R) 타입의 액션입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Action_Type_Ability_Movement, "Action.Type.Ability.Movement", "이동 능력 타입의 액션입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Action_Type_Ability_Damaging, "Action.Type.Ability.Damaging", "피해를 입히는 능력 타입의 액션입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Action_Type_Ability_CrowdControl, "Action.Type.Ability.CrowdControl", "CC를 부여하는 능력 타입의 액션입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Action_Type_Ability_Buff, "Action.Type.Ability.Buff", "아군에게 이로운 효과를 주는 버프 타입의 액션입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Action_Type_Ability_Debuff, "Action.Type.Ability.Debuff", "적에게 해로운 효과를 주는 디버프 타입의 액션입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Action_Type_Utility, "Action.Type.Utility", "소환사 주문, 아이템 등 기타 유틸리티 타입의 액션입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Action_Type_Utility_Cleanse, "Action.Type.Utility.Cleanse", "CC 효과를 해제하는 타입의 액션입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Action_Type_Utility_Item, "Action.Type.Utility.Item", "사용 아이템 타입의 액션입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Action_Type_Utility_Summon, "Action.Type.Utility.Summon", "소환수를 생성하는 타입의 액션입니다.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Action_Fail_IsDead, "Action.Fail.IsDead", "사망 상태로 인해 액션 발동에 실패했습니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Action_Fail_Cooldown, "Action.Fail.Cooldown", "재사용 대기시간으로 인해 액션 발동에 실패했습니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Action_Fail_Cost, "Action.Fail.Cost", "자원 부족으로 인해 액션 발동에 실패했습니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Action_Fail_Cost_Mana, "Action.Fail.Cost.Mana", "마나 부족으로 인해 액션 발동에 실패했습니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Action_Fail_Cost_Health, "Action.Fail.Cost.Health", "체력 부족으로 인해 액션 발동에 실패했습니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Action_Fail_AdditionalCost, "Action.Fail.AdditionalCost", "추가 자원 부족으로 인해 액션 발동에 실패했습니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Action_Fail_AlreadyActive, "Action.Fail.AlreadyActive", "이미 활성화된 액션으로 인해 액션 발동에 실패했습니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Action_Fail_TargetInvalid, "Action.Fail.TargetInvalid", "타겟이 유효하지 않아 액션 발동에 실패했습니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Action_Fail_TagsBlocked, "Action.Fail.TagsBlocked", "차단 태그로 인해 액션 발동에 실패했습니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Action_Fail_TagsMissing, "Action.Fail.TagsMissing", "필요 태그 부족으로 인해 액션 발동에 실패했습니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Action_Fail_SourceTags, "Action.Fail.SourceTags", "소스의 태그 조건(필수/차단)을 만족하지 못해 액션 발동에 실패했습니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Action_Fail_Silenced, "Action.Fail.Silenced", "침묵 상태로 인해 액션 발동에 실패했습니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Action_Fail_Networking, "Action.Fail.Networking", "네트워킹 문제로 인해 액션 발동에 실패했습니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Action_Fail_ActivationGroup, "Action.Fail.ActivationGroup", "활성화 그룹 규칙으로 인해 액션 발동에 실패했습니다.");
	
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Action_Behavior_SurvivesDeath, "Action.Behavior.SurvivesDeath", "죽음 이후에도 살아남는 액션 동작입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Action_Behavior_CannotBeCancelled, "Action.Behavior.CannotBeCancelled", "취소할 수 없는 액션입니다.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Action_Cooldown, "Action.Cooldown", "일반적인 쿨다운 카테고리를 나타내는 부모 태그입니다.");
	
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Action_Cooldown_Primary, "Action.Cooldown.Primary", "주 액션이 재사용 대기시간입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Action_Cooldown_Secondary, "Action.Cooldown.Secondary", "보조 액션이 재사용 대기시간입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Action_Cooldown_Tertiary, "Action.Cooldown.Tertiary", "세 번째 액션이 재사용 대기시간입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Action_Cooldown_Quaternary, "Action.Cooldown.Quaternary", "네 번째 액션이 재사용 대기시간입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Action_Cooldown_Ultimate, "Action.Cooldown.Ultimate", "궁극기 액션이 재사용 대기시간입니다.");

	/* ======================================== Action Stack Tags ======================================== */
	
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Action_Stack_Primary, "Action.Stack.Primary", "주 액션의 스택 수.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Action_Stack_Secondary, "Action.Stack.Secondary", "보조 액션의 스택 수.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Action_Stack_Tertiary, "Action.Stack.Tertiary", "세 번째 액션의 스택 수.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Action_Stack_Quaternary, "Action.Stack.Quaternary", "네 번째 액션의 스택 수.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Action_Stack_Ultimate, "Action.Stack.Ultimate", "궁극기 액션의 스택 수.");

	/* ======================================== Action MultiCast Tags ======================================== */
	
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Action_MultiCast_Primary, "Action.MultiCast.Primary", "주 액션의 연속 사용 횟수.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Action_MultiCast_Secondary, "Action.MultiCast.Secondary", "보조 액션의 연속 사용 횟수.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Action_MultiCast_Tertiary, "Action.MultiCast.Tertiary", "세 번째 액션의 연속 사용 횟수.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Action_MultiCast_Quaternary, "Action.MultiCast.Quaternary", "네 번째 액션의 연속 사용 횟수.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Action_MultiCast_Ultimate, "Action.MultiCast.Ultimate", "궁극기 액션의 연속 사용 횟수.");

	/* ======================================== Action Combo Tags ======================================== */
	
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Action_Combo_Primary, "Action.Combo.Primary", "주 액션의 콤보 카운트.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Action_Combo_Secondary, "Action.Combo.Secondary", "보조 액션의 콤보 카운트.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Action_Combo_Tertiary, "Action.Combo.Tertiary", "세 번째 액션의 콤보 카운트.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Action_Combo_Quaternary, "Action.Combo.Quaternary", "네 번째 액션의 콤보 카운트.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Action_Combo_Ultimate, "Action.Combo.Ultimate", "궁극기 액션의 콤보 카운트.");

	/* ======================================== Input Tags ======================================== */

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Action_Primary, "Input.Action.Primary", "주 액션 입력.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Action_Secondary, "Input.Action.Secondary", "보조 액션 입력.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Action_Tertiary, "Input.Action.Tertiary", "세 번째 액션 입력.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Action_Quaternary, "Input.Action.Quaternary", "네 번째 액션 입력.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Action_Passive, "Input.Action.Passive", "패시브 액션 입력.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Action_Ultimate, "Input.Action.Ultimate", "궁극기 액션 입력.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Move, "Input.Move", "이동 입력.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Jump, "Input.Jump", "점프 입력.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Look_Mouse, "Input.Look.Mouse", "마우스 시점 이동 입력.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Look_Stick, "Input.Look.Stick", "컨트롤러 스틱 시점 이동 입력.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Crouch, "Input.Crouch", "웅크리기 입력.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_AutoRun, "Input.AutoRun", "자동 달리기 입력.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Debug_ToggleActionSystem, "Input.Debug.ToggleActionSystem", "액션 시스템 디버그 정보 표시를 토글합니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_ToggleMouseCursor, "Input.ToggleMouseCursor", "마우스 커서 표시를 토글합니다.");

	/* ======================================== State Tags ======================================== */

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Buff_AttackDamage, "State.Buff.AttackDamage", "공격력 증가 버프 상태.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Buff_MoveSpeed, "State.Buff.MoveSpeed", "이동 속도 증가 버프 상태.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Immunity_CrowdControl, "State.Immunity.CrowdControl", "모든 군중 제어 효과에 면역인 상태.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Immunity_CrowdControl_Slow, "State.Immunity.CrowdControl.Slow", "둔화 효과에 면역인 상태.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Debuff_DamageOverTime, "State.Debuff.DamageOverTime", "지속 피해 디버프 상태.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Debuff_ArmorShred, "State.Debuff.ArmorShred", "방어력 감소 디버프 상태.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Block_Movement, "State.Block.Movement", "이동이 차단된 상태");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Block_Rotation, "State.Block.Rotation", "시점 회전이 차단된 상태");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Block_Action, "State.Block.Action", "기본 공격 및 스킬 사용이 차단된 상태");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Block_Attack, "State.Block.Attack", "기본 공격 사용이 차단된 상태");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Block_Ability, "State.Block.Ability", "스킬 사용이 차단된 상태");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Block_DamageImmune, "State.Block.DamageImmune", "피해 면역 상태");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Generic_Alive, "State.Generic.Alive", "생존 상태.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Generic_Dying, "State.Generic.Dying", "죽어가는 중인 상태.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Generic_Dead, "State.Generic.Dead", "죽음 상태.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Generic_InCombat, "State.Generic.InCombat", "전투 중 상태.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Generic_Invulnerable, "State.Generic.Invulnerable", "무적 상태.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Generic_Untargetable, "State.Generic.Untargetable", "타겟팅 불가 상태.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Movement_IsMoving, "State.Movement.IsMoving", "이동 중");

	/* ======================================== Event Tags ======================================== */

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Action_Used, "Event.Action.Used", "액션을 사용했을 때 발생하는 이벤트.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Action_Landed, "Event.Action.Landed", "액션이 대상에게 적중했을 때 발생하는 이벤트.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_BasicAttack_Hit, "Event.BasicAttack.Hit", "기본 공격이 적중했을 때 발생하는 이벤트.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Character_Damaged, "Event.Character.Damaged", "캐릭터가 피해를 입었을 때 발생하는 이벤트.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Character_Healed, "Event.Character.Healed", "캐릭터가 치유되었을 때 발생하는 이벤트.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Character_Killed, "Event.Character.Killed", "캐릭터가 다른 캐릭터를 처치했을 때 발생하는 이벤트.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Character_Died, "Event.Character.Died", "캐릭터가 사망했을 때 발생하는 이벤트.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Character_DealtDamage, "Event.Character.DealtDamage", "캐릭터가 데미지를 입힘.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Movement_Started, "Event.Movement.Started", "캐릭터가 움직이기 시작했을 때 발생하는 이벤트.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Movement_Stopped, "Event.Movement.Stopped", "캐릭터가 움직임을 멈췄을 때 발생하는 이벤트.");

	/** @brief CC 효과가 차단하는 기능에 따른 이벤트를 나타냅니다. */
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_CrowdControl, "Event.CrowdControl", "군중 제어(CC) 효과가 적용되었을 때의 이벤트입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_CrowdControl_Applied, "Event.CrowdControl.Applied", "군중 제어(CC) 효과가 적용되었을 때의 이벤트입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_CrowdControl_Removed, "Event.CrowdControl.Removed", "군중 제어(CC) 효과가 제거되었을 때의 이벤트입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_CrowdControl_Dealt, "Event.CrowdControl.Dealt", "군중 제어(CC) 효과가 적용되었을 때의 이벤트입니다.");
	
	/* ======================================== Task Tags ======================================== */

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Task_Event_Completed, "Task.Event.Completed", "태스크가 성공적으로 완료되었을 때의 이벤트입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Task_Event_Canceled, "Task.Event.Canceled", "태스크가 취소되었을 때의 이벤트입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Task_Event_Failed, "Task.Event.Failed", "태스크가 실패했을 때의 이벤트입니다.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Task_Event_Montage_NotifyBegin, "Task.Event.Montage.NotifyBegin", "몽타주 노티파이가 시작될 때의 이벤트입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Task_Event_Montage_NotifyEnd, "Task.Event.Montage.NotifyEnd", "몽타주 노티파이가 끝날 때의 이벤트입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Task_Event_Montage_Interrupted, "Task.Event.Montage.Interrupted", "몽타주가 다른 애니메이션에 의해 중단되었을 때의 이벤트입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Task_Event_Montage_BlendOut, "Task.Event.Montage.BlendOut", "몽타주가 블렌드 아웃되기 시작할 때의 이벤트입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Task_Event_Montage_Ended, "Task.Event.Montage.Ended", "몽타주가 정상적으로 종료되었을 때의 이벤트입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Task_Event_Montage_AlreadyPlaying, "Task.Event.Montage.AlreadyPlaying", "몽타주가 이미 재생 중이어서 건너뛰었을 때의 이벤트입니다.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Task_Event_TargetData_Ready, "Task.Event.TargetData.Ready", "타겟 데이터가 준비되었을 때의 이벤트입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Task_Event_TargetData_Cancelled, "Task.Event.TargetData.Cancelled", "타겟팅이 취소되었을 때의 이벤트입니다.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Task_Event_Input_Pressed, "Task.Event.Input.Pressed", "지정된 입력이 눌렸을 때의 이벤트입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Task_Event_Input_Released, "Task.Event.Input.Released", "지정된 입력이 떼어졌을 때의 이벤트입니다.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Task_Event_GameplayEvent_Received, "Task.Event.GameplayEvent.Received", "특정 게임플레이 이벤트를 수신했을 때의 이벤트입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Task_Event_Delay_Finished, "Task.Event.Delay.Finished", "지정된 딜레이가 종료되었을 때의 이벤트입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Task_Event_PhaseDelay_Finished, "Task.Event.PhaseDelay.Finished", "지정된 딜레이가 종료되었을 때의 이벤트입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Task_Event_Overlap_Occurred, "Task.Event.Overlap.Occurred", "오버랩이 발생했을 때의 이벤트입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Task_Event_Attack_HitPredicted, "Task.Event.Attack.HitPredicted", "예측 공격이 발생했을 때의 이벤트입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Task_Event_Attack_HitVerified, "Task.Event.Attack.HitVerified", "예측 공격이 발생했을 때의 이벤트입니다.");
	
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Task_Event_Path_Ready, "Task.Event.Path.Ready", "경로 분석 및 생성 완료.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Task_Event_Path_Failed, "Task.Event.Path.Failed", "경로 생성 실패.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Task_Event_Path_Predicted, "Task.Event.Path.Predicted", "클라이언트에서 생성한 경로를 서버로 전송.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Task_Event_Path_ServerValidated, "Task.Event.Path.ServerValidated", "서버에서 클라이언트 경로 검증 완료.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Task_Event_FollowPath_Completed, "Task.Event.Follow.Completed", "경로 따라가기 완료.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Task_Event_FollowPath_Failed, "Task.Event.Follow.Failed", "경로 따라가기 실패.");

	/* ======================================== Effect Tags ======================================== */

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Effect_Type_Physical, "Effect.Type.Physical", "물리 피해를 주는 이펙트입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Effect_Type_Magical, "Effect.Type.Magical", "마법 피해를 주는 이펙트입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Effect_Type_Critical, "Effect.Type.Critical", "치명타 피해를 주는 이펙트입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Effect_Type_CrowdControl, "Effect.Type.CrowdControl", "군중 제어(CC) 이펙트입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Effect_Type_Buff, "Effect.Type.Buff", "버프 이펙트입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Effect_Type_Debuff, "Effect.Type.Debuff", "디버프 이펙트입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Effect_Type_Cooldown, "Effect.Type.Cooldown", "액션의 쿨다운을 적용하는 이펙트입니다.");

	
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Effect_Damage_Type_Physical, "Effect.Damage.Physical", "물리 피해 타입임을 나타냅니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Effect_Damage_Type_Magical, "Effect.Damage.Magical", "마법 피해 타입임을 나타냅니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Effect_Damage_Type_True, "Effect.Damage.True", "고정 피해 타입임을 나타냅니다.");

	/** @brief CC 효과의 종류를 나타냅니다. */
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(CrowdControl_Stun, "CrowdControl.Stun", "기절 효과를 나타냅니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(CrowdControl_Frozen, "CrowdControl.Frozen", "빙결 효과를 나타냅니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(CrowdControl_Snare, "CrowdControl.Snare", "속박(이동 불가) 효과를 나타냅니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(CrowdControl_Silence, "CrowdControl.Silence", "침묵 효과를 나타냅니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(CrowdControl_Slow, "CrowdControl.Slow", "둔화 효과를 나타냅니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(CrowdControl_Airborne, "CrowdControl.Airborne", "공중에 띄우는(에어본) 효과를 나타냅니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(CrowdControl_Taunt, "CrowdControl.Taunt", "도발 효과를 나타냅니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(CrowdControl_Fear, "CrowdControl.Fear", "공포 효과를 나타냅니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(CrowdControl_Blind, "CrowdControl.Blind", "실명 효과를 나타냅니다.");

	/* ======================================== Effect SetByCaller Tags ======================================== */

	/** @brief 일반적인 SetByCaller 태그들 */
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Effect_SetByCaller_Cost, "Effect.SetByCaller.Cost", "Cost 이펙트의 적용량 데이터를 전달합니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Effect_SetByCaller_Duration, "Effect.SetByCaller.Duration", "이펙트의 지속시간을 설정하는 SetByCaller 태그입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Effect_SetByCaller_Magnitude, "Effect.SetByCaller.Magnitude", "이펙트의 수치 데이터를 설정하는 SetByCaller 태그입니다.");
	
	/** @brief 데미지 계산 입력 데이터 */
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Effect_SetByCaller_Physical_Base, "Effect.SetByCaller.Physical.Base", "기본 물리 피해량을 설정하는 SetByCaller 태그입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Effect_SetByCaller_Magical_Base, "Effect.SetByCaller.Magical.Base", "기본 마법 피해량을 설정하는 SetByCaller 태그입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Effect_SetByCaller_Physical_Scale, "Effect.SetByCaller.Physical.Scale", "물리 피해량 계수를 설정하는 SetByCaller 태그입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Effect_SetByCaller_Magical_Scale, "Effect.SetByCaller.Magical.Scale", "마법 피해량 계수를 설정하는 SetByCaller 태그입니다.");
	
	/** @brief 데미지 계산 중간 결과 */
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Effect_SetByCaller_Physical_Raw, "Effect.SetByCaller.Physical.Raw", "원시 물리 피해량(방어력 적용 전)을 설정하는 SetByCaller 태그입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Effect_SetByCaller_Magical_Raw, "Effect.SetByCaller.Magical.Raw", "원시 마법 피해량(마법저항 적용 전)을 설정하는 SetByCaller 태그입니다.");
	
	/** @brief 데미지 계산 최종 결과 */
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Effect_SetByCaller_Physical_Final, "Effect.SetByCaller.Physical.Final", "최종 물리 피해량(방어력 적용 후)을 설정하는 SetByCaller 태그입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Effect_SetByCaller_Magical_Final, "Effect.SetByCaller.Magical.Final", "최종 마법 피해량(마법저항 적용 후)을 설정하는 SetByCaller 태그입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Effect_SetByCaller_Damage_Total, "Effect.SetByCaller.Damage.Total", "총 피해량(모든 타입 합산)을 설정하는 SetByCaller 태그입니다.");
	
	/** @brief 치명타 관련 SetByCaller 태그들 */
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Effect_SetByCaller_Critical_Chance, "Effect.SetByCaller.Critical.Chance", "치명타 확률을 설정하는 SetByCaller 태그입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Effect_SetByCaller_Critical_Multiplier, "Effect.SetByCaller.Critical.Multiplier", "치명타 배율을 설정하는 SetByCaller 태그입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Effect_SetByCaller_Critical_IsCritical, "Effect.SetByCaller.Critical.IsCritical", "치명타 발생 여부를 설정하는 SetByCaller 태그입니다.");
	
	/** @brief 데미지 계산 상세 정보 */
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Effect_SetByCaller_Damage_Source_AttackDamage, "Effect.SetByCaller.Damage.Source.AttackDamage", "시전자의 공격력을 저장하는 SetByCaller 태그입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Effect_SetByCaller_Damage_Source_AbilityPower, "Effect.SetByCaller.Damage.Source.AbilityPower", "시전자의 주문력을 저장하는 SetByCaller 태그입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Effect_SetByCaller_Damage_Target_Armor, "Effect.SetByCaller.Damage.Target.Armor", "대상의 방어력을 저장하는 SetByCaller 태그입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Effect_SetByCaller_Damage_Target_MagicResistance, "Effect.SetByCaller.Damage.Target.MagicResistance", "대상의 마법저항을 저장하는 SetByCaller 태그입니다.");
	
	/** @brief 데미지 계산 공식 관련 */
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Effect_SetByCaller_Damage_Formula_Physical_Mitigation, "Effect.SetByCaller.Damage.Formula.Physical.Mitigation", "물리 피해 감소율을 저장하는 SetByCaller 태그입니다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Effect_SetByCaller_Damage_Formula_Magical_Mitigation, "Effect.SetByCaller.Damage.Formula.Magical.Mitigation", "마법 피해 감소율을 저장하는 SetByCaller 태그입니다.");


	/* ======================================== Init Tags ======================================== */

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InitState_Spawned, "InitState.Spawned", "액터가 스폰된 상태.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InitState_DataAvailable, "InitState.DataAvailable", "데이터를 사용할 수 있는 상태.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InitState_DataInitialized, "InitState.DataInitialized", "데이터가 초기화된 상태.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InitState_GameplayReady, "InitState.GameplayReady", "게임플레이가 준비된 상태.");


	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Movement_Mode_Walking, "Movement.Mode.Walking", "걷기 이동 모드.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Movement_Mode_NavWalking, "Movement.Mode.NavWalking", "네비게이션 걷기 이동 모드.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Movement_Mode_Falling, "Movement.Mode.Falling", "낙하 이동 모드.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Movement_Mode_Swimming, "Movement.Mode.Swimming", "수영 이동 모드.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Movement_Mode_Flying, "Movement.Mode.Flying", "비행 이동 모드.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Movement_Mode_Custom, "Movement.Mode.Custom", "커스텀 이동 모드.");

	// Unreal Movement Modes
	const TMap<uint8, FGameplayTag> MovementModeTagMap =
	{
		{ MOVE_Walking, Movement_Mode_Walking },
		{ MOVE_NavWalking, Movement_Mode_NavWalking },
		{ MOVE_Falling, Movement_Mode_Falling },
		{ MOVE_Swimming, Movement_Mode_Swimming },
		{ MOVE_Flying, Movement_Mode_Flying },
		{ MOVE_Custom, Movement_Mode_Custom }
	};

	// Custom Movement Modes
	const TMap<uint8, FGameplayTag> CustomMovementModeTagMap =
	{
		// Fill these in with your custom modes
	};

	FGameplayTag FindTagByString(const FString& TagString, bool bMatchPartialString)
	{
		const UGameplayTagsManager& Manager = UGameplayTagsManager::Get();
		FGameplayTag Tag = Manager.RequestGameplayTag(FName(*TagString), false);

		if (!Tag.IsValid() && bMatchPartialString)
		{
			FGameplayTagContainer AllTags;
			Manager.RequestAllGameplayTags(AllTags, true);

			for (const FGameplayTag& TestTag : AllTags)
			{
				if (TestTag.ToString().Contains(TagString))
				{
					UE_LOG(LogLux, Warning, TEXT("Could not find exact match for tag [%s] but found partial match on tag [%s]."), *TagString, *TestTag.ToString());
					Tag = TestTag;
					break;
				}
			}
		}

		return Tag;
	}
}