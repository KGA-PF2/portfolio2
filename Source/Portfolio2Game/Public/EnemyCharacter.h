#pragma once

#include "CoreMinimal.h"
#include "CharacterBase.h"
#include "SkillBase.h"
#include "EnemyAIStructs.h"
#include "Components/WidgetComponent.h"
#include "EnemyCharacter.generated.h"

class APlayerCharacter;
class ABattleManager;

UCLASS()
class AEnemyCharacter : public ACharacterBase
{
    GENERATED_BODY()

public:
    AEnemyCharacter();
    virtual void EndAction() override;
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

    // [신규] 결정된 이동 방향을 저장할 변수 (MoveToBestAttackPos용)
    UPROPERTY(VisibleAnywhere, Category = "AI|State")
    EGridDirection PendingMoveDir;

    // [신규] 회전할 목표 방향 (RotateToPlayer용) - 아이콘 계산을 위해 미리 저장
    UPROPERTY(VisibleAnywhere, Category = "AI|State")
    EGridDirection PendingFaceDir;

    // 방금 공격했는지 체크 (후퇴 판단용)
    bool bJustAttacked = false;


    // ───────── 상태 애니메이션 및 효과 ─────────
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Anim|State")
    TObjectPtr<UAnimMontage> StateMontage;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Anim|State")
    TObjectPtr<UAnimMontage> SpawnMontage;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Anim|State")
    TObjectPtr<UParticleSystem> DeathParticle;

    // ── 이동 ──
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Anim|Move")
    TObjectPtr<UAnimMontage> WalkMontage;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Anim|Move")
    TObjectPtr<UAnimMontage> WalkStopMontage;

    // ── 스킬 A ──
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Anim|Skill_A")
    TObjectPtr<UAnimMontage> Montage_Atk_A;      // 공격

    // ── 스킬 B ──
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Anim|Skill_B")
    TObjectPtr<UAnimMontage> Montage_Atk_B;      // 공격

    // ───────── 헬퍼 함수 ─────────

    // 스킬 데이터에 맞는 공격 몽타주 반환
    UAnimMontage* GetAttackMontageForSkill(USkillBase* SkillDef);

    UAnimMontage* GetReadyMontageForSkill(USkillBase* SkillDef);

    // 공격 준비 모션 재생 (Ready)
    UFUNCTION(BlueprintCallable, Category = "Anim")
    void PlayChargeMontageIfReady();


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
    void Action_ReserveRandomSkill();
    void Action_RotateToPlayer();

    // 유틸리티
    bool IsPlayerInSkillRange(USkillBase* Skill);
    bool IsPlayerInFrontCone();
    int32 GetMaxAttackRange(USkillBase* Skill);
    
    

public:
    bool GetBestMovementToAttack(USkillBase* Skill, EGridDirection& OutWorldDir);

    UFUNCTION(BlueprintCallable, Category = "AI")
    bool ExecuteSkill(USkillBase* SkillToUse);

    UFUNCTION()
    void HandleHealthChanged(int32 NewHP, int32 NewMaxHP);

    void Die();

    UPROPERTY(BlueprintReadOnly, Category = "Battle")
    class APlayerCharacter* PlayerRef;

    UFUNCTION()
    void FinishDying();

    // [신규] 맵 이동 등으로 액터가 파괴될 때 타이머를 끄기 위한 오버라이드
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
    // ───────── UI / 순서 표시 ─────────

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
    TObjectPtr<UWidgetComponent> OrderWidgetComponent;

    // 에디터에서 1st ~ 6th 이미지를 넣을 배열
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
    TArray<TObjectPtr<UTexture2D>> OrderIcons;

    // 에디터에서 스킬 준비된 1st ~ 6th 이미지를 넣을 배열
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
    TArray<TObjectPtr<UTexture2D>> OrderIcons_Red;

    // 순서 번호(1~6)를 받아 위젯을 켜는 함수
    UFUNCTION(BlueprintCallable, Category = "UI")
    void SetActionOrder(int32 OrderIndex, UTexture2D* SubActionIcon, bool bIsDangerous);

    // 위젯을 숨기는 함수
    UFUNCTION(BlueprintCallable, Category = "UI")
    void HideActionOrder();

    // (BP 연동) C++이 텍스처를 주면, BP가 위젯에 이미지를 꽂아주는 이벤트
    UFUNCTION(BlueprintImplementableEvent, Category = "UI")
    void BP_UpdateOrderIcon(UTexture2D* MainIcon, UTexture2D* SubIcon);

    // 마우스 오버 시 보여줄 스킬 정보 위젯 (카드)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
    TObjectPtr<UWidgetComponent> SkillCardWidget;

    // CharacterBase의 함수 오버라이드
    virtual void SetHighlight(bool bEnable) override;

protected:
    // 위젯에 스킬 정보를 채워넣으라고 BP에 요청하는 이벤트
    UFUNCTION(BlueprintImplementableEvent, Category = "UI")
    void BP_UpdateSkillCardInfo(USkillBase* SkillInfo);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    float SkillCardScale = 0.6f; // 원하는 크기로 기본값 설정
};
