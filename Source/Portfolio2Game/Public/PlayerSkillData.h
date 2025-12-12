#pragma once

#include "CoreMinimal.h"
#include "SkillBase.h"
#include "PlayerSkillData.generated.h"

/**
 * 플레이어가 보유한 스킬 1개에 대한 강화/상태 데이터
 */
USTRUCT(BlueprintType)
struct PORTFOLIO2GAME_API FPlayerSkillData
{
    GENERATED_BODY()

public:
    // 원본 스킬 정보 (고정)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    USkillBase* SkillInfo = nullptr;

    // 강화 단계 (플레이어가 업그레이드한 횟수)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    int32 UpgradeLevel = 0;

    // 공격력 변동값 (+/-)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    int32 DamageDelta = 0;

    // 쿨타임 변동값 (+/-)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    int32 CooldownDelta = 0;

    // 강화 적용 후 “최종 쿨타임”
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    int32 TotalCooldown = 0;

    // 현재 남은 쿨타임 (턴 단위)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    int32 CurrentCooldown = 0;

    // ===== 계산 함수 =====

    // 강화 반영된 공격력
    FORCEINLINE int32 GetEffectiveDamage() const
    {
        if (!SkillInfo) return 0;
        return FMath::Max(0, SkillInfo->BaseDamage + DamageDelta);
    }

    // 강화 반영된 총 쿨타임 (TotalCooldown이 0이면 계산해서 반환)
    FORCEINLINE int32 GetEffectiveTotalCooldown() const
    {
        if (!SkillInfo) return 0;
        return FMath::Max(0, SkillInfo->BaseCooldown + CooldownDelta);
    }

    // 이름
    FORCEINLINE FName GetSkillName() const
    {
        return SkillInfo ? SkillInfo->SkillName : "None";
    }

    // 초기화
    void InitializeFromBase()
    {
        if (!SkillInfo) return;
        TotalCooldown = 1;
        CurrentCooldown = 0;
    }
};
