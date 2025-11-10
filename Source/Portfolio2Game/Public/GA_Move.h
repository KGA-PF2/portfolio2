#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "CharacterBase.h" 
#include "GA_Move.generated.h"


UCLASS()
class PORTFOLIO2GAME_API UGA_Move : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Move();


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid Movement")
	EGridDirection MoveDirection;


	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
	) override;
};