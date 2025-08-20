#pragma once

#include "NativeGameplayTags.h"


/**
 * @namespace LuxActionTags
 * @brief Lux 캐릭터의 모든 액션 관련 태그들을 정의하는 네임스페이스입니다.
 * 
 * 이 네임스페이스는 Lux 캐릭터의 고유 액션들을 식별하고 분류하는 데 사용됩니다.
 */
namespace LuxActionTags
{
    /** =============================== Aurora Action Tags =============================== */

	/** @brief Aurora의 얼음 고리 액션입니다. */
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Action_Aurora_Hoarfrost);
	
	/** @brief Aurora의 대쉬 액션입니다. */
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Action_Aurora_GlacialCharge);
	
	/** @brief Aurora의 궁극기 액션입니다. */
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Action_Aurora_Cryoseism);
	
	/** @brief Aurora의 얼음 환상 액션입니다. */
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Action_Aurora_FrozenSimulacrum);

    /** @brief Aurora의 서리검 액션입니다. (기본 공격)*/
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Action_Aurora_FrozenSword);
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Action_Aurora_FrozenSword_ComboCount);

	/** @brief Aurora의 Cryoseism 패시브 액션입니다. (기본 공격 적중 시 궁극기 쿨다운 감소) */
	LUX_API		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Action_Aurora_CryoseismPassive);
} 