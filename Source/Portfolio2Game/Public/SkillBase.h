#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SkillBase.generated.h"

UCLASS(BlueprintType)
class USkillBase : public UDataAsset
{
    GENERATED_BODY()

public:
    // ───────── 기본 정보 ─────────
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
    FName SkillName = "None"; // 스킬 이름

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
    int32 BaseDamage = 0;     // 기본 데미지

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
    int32 BaseCooldown = 0;   // 기본 쿨타임 (턴 단위 등)

    /** WBP_SkillQueue가 참조할 스킬 아이콘 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
    TObjectPtr<UTexture2D> SkillIcon;

    // ───────── 공격 범위 (격자 상대좌표) ─────────
    // 내 위치(0,0) 기준 상대좌표 — 격자 시스템 계산에 사용
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
    TArray<FIntPoint> AttackPattern;
};

// EX)
// SkillName: "Slash"
// BaseDamage: 10
// BaseCooldown : 2
// AttackPattern :
//     (1, 0)
//     (2, 0)
//     (2, 1)
//     (2, -1)