#include "GA_Move.h"
#include "GameFramework/Actor.h"
#include "CharacterBase.h"

UGA_Move::UGA_Move() {
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UGA_Move::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData) {
    if (ACharacterBase* Char = Cast<ACharacterBase>(ActorInfo->AvatarActor.Get())) {
        // 이동 처리 (GridCoord 변경 등)
    }
    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
