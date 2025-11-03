#pragma once
#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_Move.generated.h"

UCLASS()
class UGA_Move : public UGameplayAbility {
    GENERATED_BODY()

public:
    UGA_Move();

    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;
};
