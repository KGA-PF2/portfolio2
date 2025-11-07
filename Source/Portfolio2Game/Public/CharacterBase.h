// CharacterBase.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
// (오류 수정) .generated.h 보다 먼저 include
#include "AbilitySystemComponent.h" 
#include "BaseAttributeSet.h"
#include "CharacterBase.generated.h" // 이 파일이 항상 마지막 include여야 함

class ABattleManager;
class UGameplayAbility;
class UGA_Move; // (신규) GA_Move 클래스 전방 선언

UENUM(BlueprintType)
enum class EGridDirection : uint8
{
	Up,		// Y-
	Down,	// Y+
	Left,	// X-
	Right	// X+
};

UCLASS()
class PORTFOLIO2GAME_API ACharacterBase : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ACharacterBase();

protected:
	virtual void BeginPlay() override;

	// ───────── GAS ─────────
public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystem;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UBaseAttributeSet> Attributes;

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override { return AbilitySystem; }

protected:
	UPROPERTY(EditDefaultsOnly, Category = "GAS")
	TArray<TSubclassOf<class UGameplayAbility>> SkillList;

	UPROPERTY(EditDefaultsOnly, Category = "GAS")
	TArray<TSubclassOf<class UGameplayEffect>> EffectList;

	UPROPERTY(Transient)
	TArray<TSubclassOf<UGameplayAbility>> QueuedSkills;

	virtual void InitAttributes();
	virtual void GiveAllSkills();

	/** (수정됨) 4개의 이동 어빌리티 BP 클래스를 에디터에서 받아옵니다. */
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Movement")
	TSubclassOf<UGA_Move> MoveAbility_Up;
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Movement")
	TSubclassOf<UGA_Move> MoveAbility_Down;
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Movement")
	TSubclassOf<UGA_Move> MoveAbility_Left;
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Movement")
	TSubclassOf<UGA_Move> MoveAbility_Right;

	UFUNCTION(BlueprintCallable, Category = "GAS")
	void GiveMoveAbilities();

	// ───────── 턴 관리 ─────────
public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Turn")
	bool bCanAct = false;

	UFUNCTION(BlueprintCallable, Category = "Turn")
	void StartAction();

	UFUNCTION(BlueprintCallable, Category = "Turn")
	void EndAction();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ref")
	TObjectPtr<ABattleManager> BattleManagerRef;

	// ───────── 스킬 큐 ─────────
public:
	UFUNCTION(BlueprintCallable, Category = "Skill")
	void EnqueueSkill(TSubclassOf<UGameplayAbility> SkillClass);

	UFUNCTION(BlueprintCallable, Category = "Skill")
	void ExecuteSkillQueue();

	UFUNCTION(BlueprintCallable, Category = "Skill")
	void CancelSkillQueue();

	// ───────── 이동/회전 (수정됨) ─────────
public:
	/** (수정됨) 캐릭터의 논리적 위치(좌표와 인덱스)를 업데이트합니다. */
	UFUNCTION(BlueprintCallable, Category = "Move")
	void MoveToCell(FIntPoint TargetCoord, int32 TargetIndex);

	UFUNCTION(BlueprintCallable, Category = "Move")
	void Turn(bool bRight);

	UFUNCTION(BlueprintImplementableEvent, Category = "Move")
	void PlayMoveAnim();

	// ───────── 상태 (수정됨) ─────────
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
	FIntPoint GridCoord = FIntPoint::ZeroValue;

	/** (신규) 1D 그리드 인덱스 (칸 번호, 세로 우선) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid", meta = (ExposeOnSpawn = "true"))
	int32 GridIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
	bool bFacingRight = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status")
	bool bDead = false;

	/** (신규) GridIndex 변수를 반환합니다. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Grid")
	int32 GetGridIndex() const;

	// ───────── 스탯/데미지 (오류 수정) ─────────
public:
	/** (오류 수정) EnemyCharacter가 호출할 수 있도록 ApplyDamage를 다시 추가합니다. */
	UFUNCTION(BlueprintCallable, Category = "Attributes")
	void ApplyDamage(float Damage);

	// ❌ (오류 수정) FOnAttributeChangeData 델리게이트 함수 선언을 제거합니다.
	// UFUNCTION()
	// void OnHealthChanged(const FOnAttributeChangeData& Data);

	/** (유지) BP에서 사망 처리를 위한 이벤트 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Status")
	void OnDeath();
};