#pragma once

#include "CoreMinimal.h"
#include "CharacterBase.h"
#include "PlayerCharacter.generated.h"

// ❌ BattleManager 전방 선언 제거 (부모 클래스인 CharacterBase가 이미 참조)
// class ABattleManager;

/**
 * 턴제 전투용 플레이어 캐릭터 (GAS 입력 바인딩 담당)
 * - 턴 제어 (bCanAct)
 * - 턴 통신 (EndAction)
 * - (수정) 입력 컴포넌트에서 GAS 태그를 활성화
 */
UCLASS()
class PORTFOLIO2GAME_API APlayerCharacter : public ACharacterBase
{
	GENERATED_BODY()

public:
	APlayerCharacter();

protected:
	virtual void BeginPlay() override;

	/** (수정됨) WASD/스킬 입력을 GAS 태그에 바인딩합니다. */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/** (신규) AbilitySystemComponent에 입력을 바인딩하기 위해 PossessedBy에서 호출됩니다. */
	virtual void PossessedBy(AController* NewController) override;

public:
	// ──────────────────────────────
	// 턴 제어
	// ──────────────────────────────
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Turn")
	bool bCanAct = false; // 내 턴일 때만 입력 허용

	

	// ──────────────────────────────
	// 행동 함수
	// ──────────────────────────────
	UFUNCTION(BlueprintCallable, Category = "Turn")
	void EnableAction(bool bEnable); // 턴 시작/종료 시 호출

	UFUNCTION(BlueprintCallable, Category = "Turn")
	void EndAction(); // 행동 1회 끝났을 때

	
};