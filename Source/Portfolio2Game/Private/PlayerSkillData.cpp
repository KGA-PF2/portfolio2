#pragma once
#include "CoreMinimal.h"
#include "SkillBase.h"
#include "PlayerSkillData.generated.h"

USTRUCT(BlueprintType)
struct PORTFOLIO2GAME_API FPlayerSkillData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    USkillBase* SkillInfo = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 CurrentLevel = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 CurrentCooldown = 0;
};
