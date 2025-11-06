#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "BaseAttributeSet.h"
#include "CharacterBase.generated.h"

class ABattleManager; // (신규) BattleManager 전방 선언

/**
 * (신규) GA_Move에서 사용할 이동 방향 Enum.
 * GA_Move.h 파일에서도 이 Enum을 사용하게 됩니다.
 */
UENUM(BlueprintType)
enum class EGridDirection : uint8
{
	Up,	// W (Y-)
	Down,	// S (Y+)
	Left,		// A (X-)
	Right		// D (X+)
};

UCLASS()
class PORTFOLIO2GAME_API ACharacterBase : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ACharacterBase();

protected:
	virtual void BeginPlay() override;

	// ❌ Tick 기반 이동 로직을 사용하지 않으므로 Tick 관련 선언은 제거합니다.

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	UAbilitySystemComponent* AbilitySystem;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	UBaseAttributeSet* Attributes;

public:
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override { return AbilitySystem; }

	// ───────── 스킬 “보유 목록” ─────────
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skills")
	TArray<TSubclassOf<class UGameplayAbility>> SkillList;

	/** (신규) WASD 이동에 사용할 어빌리티 클래스 (에디터에서 할당) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skills|Movement")
	TSubclassOf<class UGameplayAbility> MoveAbility_Up;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skills|Movement")
	TSubclassOf<class UGameplayAbility> MoveAbility_Down;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skills|Movement")
	TSubclassOf<class UGameplayAbility> MoveAbility_Left;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skills|Movement")
	TSubclassOf<class UGameplayAbility> MoveAbility_Right;

	UPROPERTY()
	TArray<FGameplayAbilitySpecHandle> GrantedSkillHandles;

	UFUNCTION(BlueprintCallable, Category = "Skills")
	void GiveAllSkills();

	/** (신규) 이동 어빌리티를 ASC에 부여하는 함수 */
	UFUNCTION(BlueprintCallable, Category = "Skills")
	void GiveMoveAbilities();

	// ───────── 스킬 “예약/실행” 큐 (기존과 동일) ─────────
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skill Queue")
	TArray<TSubclassOf<UGameplayAbility>> QueuedSkills;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill Queue")
	int32 MaxQueuedSkills = 3;

	UFUNCTION(BlueprintCallable, Category = "Skills")
	void ReserveSkill(TSubclassOf<UGameplayAbility> SkillClass);

	UFUNCTION(BlueprintCallable, Category = "Skills")
	void ExecuteSkillQueue();

	UFUNCTION(BlueprintCallable, Category = "Skills")
	void CancelSkillQueue();

	// ───────── 이동/회전 (수정됨) ─────────
	/**
	 * (수정됨) 이 함수는 이제 단순히 데이터(GridCoord)만 업데이트합니다.
	 * 실제 월드 이동은 GA_Move 어빌리티가 담당합니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Move")
	void MoveToCell(FIntPoint Target);

	UFUNCTION(BlueprintCallable, Category = "Move")
	void Turn(bool bRight);

	/**
	 * (신규) GA_Move가 이동 모션을 재생하기 위해 호출할 이벤트입니다.
	 * 블루프린트에서 이 이벤트를 구현하여 애니메이션을 재생하세요.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Move")
	void PlayMoveAnim();

	// ❌ Tick 기반 이동 관련 로직(OnMovementFinished) 제거

	// ───────── 상태 ─────────
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
	FIntPoint GridCoord = FIntPoint::ZeroValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
	bool bFacingRight = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status")
	bool bDead = false;

	/** (신규) BattleManager에게 물어봐서 1D 인덱스를 가져옵니다. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Grid")
	int32 GetGridIndex() const;

protected:
	// ❌ Tick 기반 이동 관련 변수(bIsMoving 등) 제거

	/** (신규) 모든 캐릭터가 BattleManager를 참조합니다. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Battle")
	ABattleManager* BattleManagerRef;

public:
	// ───────── 속성/사망 (기존과 동일) ─────────
	virtual void InitAttributes();
	virtual void ApplyDamage(float Amount);
	virtual void Die();

	/** (신규) GA_Move가 BattleManagerRef에 접근할 수 있도록 Getter 함수를 추가합니다. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Battle")
	ABattleManager* GetBattleManager() const { return BattleManagerRef; }
};