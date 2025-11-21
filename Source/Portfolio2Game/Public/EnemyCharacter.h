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

    // Attack 1
    UPROPERTY(EditDefaultsOnly, Category = "AI|Skills")
    TObjectPtr<USkillBase> Skill_A;

    // Attack 2
    UPROPERTY(EditDefaultsOnly, Category = "AI|Skills")
    TObjectPtr<USkillBase> Skill_B;


    // ───────── 상태 연출 (피격, 사망) ─────────
    // 맞았을 때 재생할 몽타주 (G키 대체)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Anim|State")
    TObjectPtr<UAnimMontage> HitReactionMontage;

    // 죽었을 때 재생할 몽타주 (T키 대체)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Anim|State")
    TObjectPtr<UAnimMontage> DeathMontage;

    // 죽었을 때 터질 파티클 (SpawnVFX 대체)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Anim|State")
    TObjectPtr<UParticleSystem> DeathParticle;

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

    // 특정 스킬을 GAS로 발동시키는 함수
    UFUNCTION(BlueprintCallable, Category = "AI")
    bool ExecuteSkill(USkillBase* SkillToUse);

    // 체력 변화 감지 및 처리 (자동 호출)
    UFUNCTION()
    void HandleHealthChanged(int32 NewHP, int32 NewMaxHP);

    // 사망 처리
    void Die();

    // [이름 변경] MoveUp -> MoveForward (바라보는 방향 전진)
    UFUNCTION(BlueprintCallable, Category = "AI")
    void MoveForward();

    // ──────────────────────────────
    // 참조
    // ──────────────────────────────
    UPROPERTY(BlueprintReadOnly, Category = "Battle")
    APlayerCharacter* PlayerRef;

};
