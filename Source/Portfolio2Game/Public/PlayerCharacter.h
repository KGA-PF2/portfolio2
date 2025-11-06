#pragma once

#include "CoreMinimal.h"
#include "CharacterBase.h"
#include "PlayerCharacter.generated.h"

/** (신규) GAS 입력 바인딩을 위한 Enum 정의 */
namespace PlayerAbilityInputID
{
	const int32 None = 0;
	const int32 MoveUp = 1;
	const int32 MoveDown = 2;
	const int32 MoveLeft = 3;
	const int32 MoveRight = 4;
	const int32 ExecuteSkills = 5;
	const int32 CancelSkills = 6;
}

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

	// ──────────────────────────────
	// (신규) 입력 바인딩 래퍼 함수
	// ──────────────────────────────
private:
	void Input_MoveUp();
	void Input_MoveDown();
	void Input_MoveLeft();
	void Input_MoveRight();
	// (필요시)
	// void Input_ExecuteSkills();
	// void Input_CancelSkills();

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