#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Particles/ParticleSystem.h"
#include "NiagaraSystem.h"
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

    // 스킬 카드 배경/커버 이미지
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Visual")
    TObjectPtr<UTexture2D> SkillCover;

    /** 스킬 사용 시 재생할 몽타주 (AnimNotify로 타격 시점 조절) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Visual")
    TObjectPtr<UAnimMontage> SkillMontage;

    /** 타격 지점에 생성될 이펙트 (Cascade Particle System) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Visual")
    TObjectPtr<UParticleSystem> TileEffect;

    // Niagara 이펙트 (Priority High)
    // 이 값이 있으면 TileEffect를 무시하고 이것만 재생합니다.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Visual")
    TObjectPtr<UNiagaraSystem> NiagaraEffect;

    /** 이펙트 생성 위치 미세 조정 (예: 바닥보다 살짝 위 Z=50) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Visual")
    FVector EffectOffset = FVector(0, 0, 50.0f);

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