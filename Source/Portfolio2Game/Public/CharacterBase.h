#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "BaseAttributeSet.h"
#include "CharacterBase.generated.h"

UCLASS()
class ACharacterBase : public ACharacter, public IAbilitySystemInterface
{
    GENERATED_BODY()

public:
    ACharacterBase();

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
    UAbilitySystemComponent* AbilitySystem;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
    UBaseAttributeSet* Attributes;

public:
    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override { return AbilitySystem; }

    // ───────── 스킬 “보유 목록” (캐릭터별 고정/설정) ─────────
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skills")
    TArray<TSubclassOf<class UGameplayAbility>> SkillList;   // 사용할 수 있는 스킬 풀

    UPROPERTY()
    TArray<FGameplayAbilitySpecHandle> GrantedSkillHandles; // 부여된 스킬 핸들 보관

    UFUNCTION(BlueprintCallable, Category = "Skills")
    void GiveAllSkills(); // SkillList를 ASC에 부여(시작/스폰 시 한 번)

    // ───────── 스킬 “예약/실행” 큐 ─────────
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skill Queue")
    TArray<TSubclassOf<UGameplayAbility>> QueuedSkills;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill Queue")
    int32 MaxQueuedSkills = 3;

    UFUNCTION(BlueprintCallable, Category = "Skills")
    void ReserveSkill(TSubclassOf<UGameplayAbility> SkillClass); // 예약(큐 유지)

    UFUNCTION(BlueprintCallable, Category = "Skills")
    void ExecuteSkillQueue(); // 예약된 스킬 전부 순차 발동(큐 유지)

    UFUNCTION(BlueprintCallable, Category = "Skills")
    void CancelSkillQueue(); // 전체 취소(수동)

    // ───────── 이동/회전 (턴 제어 없음) ─────────
    UFUNCTION(BlueprintCallable, Category = "Move")
    void MoveToCell(FIntPoint Target);

    UFUNCTION(BlueprintCallable, Category = "Move")
    void Turn(bool bRight);

    // ───────── 상태 ─────────
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
    FIntPoint GridCoord = FIntPoint::ZeroValue;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
    bool bFacingRight = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status")
    bool bDead = false;

    virtual void InitAttributes();
    virtual void ApplyDamage(float Amount);
    virtual void Die();
};
