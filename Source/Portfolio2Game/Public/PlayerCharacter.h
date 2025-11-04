#pragma once

#include "CoreMinimal.h"
#include "CharacterBase.h"
#include "PlayerCharacter.generated.h"

class ABattleManager;

/**
 * 턴제 전투용 플레이어 캐릭터
 * - 입력 기반 행동 (이동, 회전, 스킬 예약/사용/취소)
 * - BattleManager와 턴 통신
 */
UCLASS()
class APlayerCharacter : public ACharacterBase
{
    GENERATED_BODY()

public:
    APlayerCharacter();

protected:
    virtual void BeginPlay() override;
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

public:
    // ──────────────────────────────
    // 턴 제어
    // ──────────────────────────────
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Turn")
    bool bCanAct = false; // 내 턴일 때만 입력 허용

    // ──────────────────────────────
    // 참조
    // ──────────────────────────────
    UPROPERTY(BlueprintReadOnly, Category = "Battle")
    ABattleManager* BattleManagerRef;

    // ──────────────────────────────
    // 행동 함수
    // ──────────────────────────────
    UFUNCTION(BlueprintCallable, Category = "Turn")
    void EnableAction(bool bEnable); // 턴 시작/종료 시 호출

    UFUNCTION(BlueprintCallable, Category = "Turn")
    void EndAction(); // 행동 1회 끝났을 때

    // 이동
    UFUNCTION(BlueprintCallable, Category = "Action")
    void HandleMove(FIntPoint TargetCell);

    // 회전
    UFUNCTION(BlueprintCallable, Category = "Action")
    void HandleTurn(bool bRight);

    // 스킬 예약
    UFUNCTION(BlueprintCallable, Category = "Action")
    void HandleReserveSkill(TSubclassOf<UGameplayAbility> SkillClass);

    // 예약된 스킬 전부 실행
    UFUNCTION(BlueprintCallable, Category = "Action")
    void HandleExecuteSkills();

    // 예약된 스킬 취소
    UFUNCTION(BlueprintCallable, Category = "Action")
    void HandleCancelSkills();
};
