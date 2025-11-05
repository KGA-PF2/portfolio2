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
 * GA_Move가 이 헤더를 include하여 사용합니다.
 */
UENUM(BlueprintType)
enum class EGridDirection : uint8
{
	Forward,
	Backward,
	Left,
	Right
};

UCLASS()
class PORTFOLIO2GAME_API ACharacterBase : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ACharacterBase(); // 생성자

protected:
	virtual void BeginPlay() override;

	// ❌ Tick을 이용한 이동 로직을 사용하지 않으므로 Tick 관련 선언은 제거합니다.
	// virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	UAbilitySystemComponent* AbilitySystem;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	UBaseAttributeSet* Attributes;

public:
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override { return AbilitySystem; }

	// ───────── 스킬 “보유 목록” (기존과 동일) ─────────
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skills")
	TArray<TSubclassOf<class UGameplayAbility>> SkillList;

	UPROPERTY()
	TArray<FGameplayAbilitySpecHandle> GrantedSkillHandles;

	UFUNCTION(BlueprintCallable, Category = "Skills")
	void GiveAllSkills();

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

	// ───────── 이동/회전 (기존 함수 유지) ─────────
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

	// ❌ OnMovementFinished (이동 완료 콜백)은 GA_Move가 직접 처리하므로 제거합니다.
	// virtual void OnMovementFinished();

	// ───────── 상태 ─────────
public: // 자식 클래스 및 BattleManager에서 접근할 수 있도록 public/protected로 변경
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
	FIntPoint GridCoord = FIntPoint::ZeroValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
	bool bFacingRight = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status")
	bool bDead = false;

	/** (신규) BattleManager에게 물어봐서 1D 인덱스를 가져옵니다. (요구사항 3) */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Grid")
	int32 GetGridIndex() const;

protected:
	// ❌ 부드러운 이동(Tick)에 필요했던 변수들 제거
	// bool bIsMoving = false;
	// FVector MoveTargetLocation;
	// float MoveSpeed = 600.f;

	/** (신규) 모든 캐릭터가 BattleManager를 참조합니다. (GA_Move가 이 참조를 사용) */
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