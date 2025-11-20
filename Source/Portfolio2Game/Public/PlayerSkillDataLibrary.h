#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PlayerSkillData.h"
#include "PlayerSkillDataLibrary.generated.h"

UCLASS()
class PORTFOLIO2GAME_API UPlayerSkillDataLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** 스킬 데이터를 받아 강화된 데미지를 계산합니다. */
	UFUNCTION(BlueprintPure, Category = "Skill|Data")
	static int32 GetEffectiveDamage(const FPlayerSkillData& SkillData);

	/** 스킬 데이터를 받아 강화된 총 쿨타임을 계산합니다. */
	UFUNCTION(BlueprintPure, Category = "Skill|Data")
	static int32 GetEffectiveTotalCooldown(const FPlayerSkillData& SkillData);

	/** 스킬 데이터에서 스킬 이름을 가져옵니다. */
	UFUNCTION(BlueprintPure, Category = "Skill|Data")
	static FName GetSkillName(const FPlayerSkillData& SkillData);
};