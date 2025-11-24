#pragma once

#include "CoreMinimal.h"
#include "CharacterBase.h"
#include "SkillBase.h"
#include "EnemyAIStructs.h"
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

    UPROPERTY(EditAnywhere, Category = "AI|Brain")
    TObjectPtr<UEnemyBrainData> BrainData;

    // 이번 턴에 하기로 "확정된" 행동 (플레이어 턴 시작 시 결정됨)
    UPROPERTY(VisibleAnywhere, Category = "AI|State")
    EAIActionType PendingAction = EAIActionType::Wait;

    // Attack 1
    UPROPERTY(EditDefaultsOnly, Category = "AI|Skills")
    TObjectPtr<USkillBase> Skill_A;

    // Attack 2
    UPROPERTY(EditDefaultsOnly, Category = "AI|Skills")
    TObjectPtr<USkillBase> Skill_B;

    // 현재 예약된 스킬
    UPROPERTY(VisibleAnywhere, Category = "AI|State")
    TObjectPtr<USkillBase> ReservedSkill;

    // 방금 공격했는지 체크 (후퇴 판단용)
    bool bJustAttacked = false;


    // ───────── 상태 애니메이션 및 효과 ─────────
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Anim|State")
    TObjectPtr<UAnimMontage> HitReactionMontage;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Anim|State")
    TObjectPtr<UAnimMontage> DeathMontage;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Anim|State")
    TObjectPtr<UParticleSystem> DeathParticle;


    // ───────── 행동 함수 ─────────
    // [신규] 1단계: 행동 결정 (플레이어 턴 시작 시 호출) -> PendingAction 저장
    void DecideNextAction();

    // [신규] 2단계: 결정된 행동 실행 (적 턴에 호출)
    void ExecutePlannedAction();

    // [신규] 월드 방향(동서남북)으로 즉시 이동하는 함수
    void Action_MoveDirectly(EGridDirection WorldDir);

private:
    // 조건 판별기
    bool CheckCondition(EAIConditionType Condition);

    // 행동 실행기
    void PerformAction(EAIActionType ActionType);

    // 이동 통합 함수: 상대 방향(RelativeDir)을 받아 실제 방향으로 변환해 이동
    void Action_Move(EGridDirection RelativeDir);

	// 최적 공격 위치로 이동
    void Action_MoveToBestAttackPos();

    // 스킬 관련
    void Action_FireReserved();
    void Action_ReserveSkill(USkillBase* Skill);
    void Action_RotateToPlayer();

    // 유틸리티
    bool IsPlayerInSkillRange(USkillBase* Skill);
    bool IsPlayerInFrontCone();
    int32 GetMaxAttackRange(USkillBase* Skill);
    bool GetBestMovementToAttack(USkillBase* Skill, EGridDirection& OutWorldDir);


public:
    UFUNCTION(BlueprintCallable, Category = "AI")
    bool ExecuteSkill(USkillBase* SkillToUse);

    UFUNCTION()
    void HandleHealthChanged(int32 NewHP, int32 NewMaxHP);

    void Die();

    UPROPERTY(BlueprintReadOnly, Category = "Battle")
    class APlayerCharacter* PlayerRef;

};
