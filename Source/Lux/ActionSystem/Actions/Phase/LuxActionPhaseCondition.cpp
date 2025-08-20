

#include "ActionSystem/Actions/Phase/LuxActionPhaseCondition.h"
#include "ActionSystem/Actions/LuxAction.h"
#include "LuxGameplayTags.h"


bool FCondition_PathHasMinPoints::CheckCondition(const ULuxAction& Action, const FContextPayload& Payload) const
{
    const FPayload_PathData* PathData = Payload.GetData<FPayload_PathData>(FName("PathData"));
    if (PathData)
    {
        return PathData->PathPoints.Num() >= MinPoints;
    }

    // 경로 데이터가 페이로드에 없으면 조건을 충족하지 못한 것으로 간주합니다.
    return false;
}

bool FCondition_NotifyNameEquals::CheckCondition(const ULuxAction& Action, const FContextPayload& Payload) const
{
    const FPayload_Name* PathData = Payload.GetData<FPayload_Name>(FName("NotifyName"));
    if (PathData)
    {
        return PathData->Value == RequiredName;
    }

    return false;
}