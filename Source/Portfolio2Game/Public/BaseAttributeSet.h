#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "BaseAttributeSet.generated.h"

// 오프라인용, 단순 Getter/Setter 매크로 정의
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

UCLASS()
class UBaseAttributeSet : public UAttributeSet
{
    GENERATED_BODY()

public:
    // === HP ===
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
    FGameplayAttributeData HP;
    ATTRIBUTE_ACCESSORS(UBaseAttributeSet, HP);

    // === Max HP ===
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
    FGameplayAttributeData MaxHP;
    ATTRIBUTE_ACCESSORS(UBaseAttributeSet, MaxHP);

public:
    // === 체력 관련 유틸 ===
    FORCEINLINE bool IsDead() const
    {
        return HP.GetCurrentValue() <= 0.f;
    }

    // 체력 초기화
    FORCEINLINE void InitHealth(float NewMaxHP)
    {
        MaxHP.SetBaseValue(NewMaxHP);
        MaxHP.SetCurrentValue(NewMaxHP);
        HP.SetBaseValue(NewMaxHP);
        HP.SetCurrentValue(NewMaxHP);
    }

    // 대미지 적용
    FORCEINLINE void ApplyDamage(float Damage)
    {
        const float NewHP = FMath::Clamp(HP.GetCurrentValue() - Damage, 0.f, MaxHP.GetCurrentValue());
        HP.SetCurrentValue(NewHP);
    }

    // 회복
    FORCEINLINE void Heal(float Amount)
    {
        const float NewHP = FMath::Clamp(HP.GetCurrentValue() + Amount, 0.f, MaxHP.GetCurrentValue());
        HP.SetCurrentValue(NewHP);
    }
};
