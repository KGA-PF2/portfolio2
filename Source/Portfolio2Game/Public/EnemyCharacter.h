#pragma once

#include "CoreMinimal.h"
#include "CharacterBase.h"
#include "EnemyCharacter.generated.h"

class APlayerCharacter;
class ABattleManager;

UCLASS()
class AEnemyCharacter : public ACharacterBase
{
    GENERATED_BODY()

public:
    AEnemyCharacter();

protected:
    virtual void BeginPlay() override;

public:
    // ──────────────────────────────
    // 행동 함수
    // ──────────────────────────────
    UFUNCTION(BlueprintCallable, Category = "AI")
    void ExecuteAIAction(); // BattleManager가 적 턴마다 호출

    UFUNCTION(BlueprintCallable, Category = "AI")
    void AttackNearestPlayer();

    UFUNCTION(BlueprintCallable, Category = "AI")
    void MoveForward();

    // ──────────────────────────────
    // 참조
    // ──────────────────────────────
    UPROPERTY(BlueprintReadOnly, Category = "Battle")
    APlayerCharacter* PlayerRef;

    UPROPERTY(BlueprintReadOnly, Category = "Battle")
    ABattleManager* BattleManagerRef;
};
