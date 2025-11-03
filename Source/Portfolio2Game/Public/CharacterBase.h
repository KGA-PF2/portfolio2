// CharacterBase.h
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

    // GAS
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
    UAbilitySystemComponent* AbilitySystem;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
    UBaseAttributeSet* Attributes;

public:
    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override
    {
        return AbilitySystem;
    }


    //스킬

    // 캐릭터가 사용할 수 있는 기본 스킬 목록 (BP에서 설정)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skills")
    TArray<TSubclassOf<class UGameplayAbility>> SkillList;

    // 실제 부여된 스킬 핸들
    UPROPERTY()
    TArray<FGameplayAbilitySpecHandle> GrantedSkillHandles;

    // 전투 시작 시 기본 스킬 부여
    virtual void GiveAllSkills();


    // 상태

    // 격자 위치(14*3 중 하나)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
    FIntPoint GridCoord = FIntPoint::ZeroValue;

    // 좌우만
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
    bool bFacingRight = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status")
    bool bDead = false;


    virtual void InitAttributes();
    virtual void ApplyDamage(float Amount);
    virtual void Die();
};
