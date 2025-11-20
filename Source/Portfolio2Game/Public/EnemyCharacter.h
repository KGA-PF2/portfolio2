#pragma once

#include "CoreMinimal.h"
#include "CharacterBase.h"
#include "SkillBase.h"
#include "EnemyCharacter.generated.h"

class APlayerCharacter;
class ABattleManager;

UCLASS()
class AEnemyCharacter : public ACharacterBase
{
    GENERATED_BODY()

public:
    AEnemyCharacter();

    // 적 기본 공격 데이터
    UPROPERTY(EditDefaultsOnly, Category = "AI")
    TObjectPtr<USkillBase> NormalAttackSkill;

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

    // [이름 변경] MoveUp -> MoveForward (바라보는 방향 전진)
    UFUNCTION(BlueprintCallable, Category = "AI")
    void MoveForward();

    // ──────────────────────────────
    // 참조
    // ──────────────────────────────
    UPROPERTY(BlueprintReadOnly, Category = "Battle")
    APlayerCharacter* PlayerRef;

};
