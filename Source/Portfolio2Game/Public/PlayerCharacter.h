// PlayerCharacter.h

#pragma once

#include "CoreMinimal.h"
#include "CharacterBase.h"
#include "PlayerSkillData.h"
#include "PlayerCharacter.generated.h"

// Enhanced Input 헤더
class UInputMappingContext;
class UInputAction;

// 쿨타임이 완료되었을 때 BP로 신호를 보낼 이벤트 디스패처
// 이 스킬이 몇 번째 슬롯/인덱스인지 알려주기 위해 int32 파라미터를 사용합니다.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCooldownFinished, int32, SkillIndex);

/** 스킬이 큐에 추가될 때 UI에 알리기 위한 델리게이트 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSkillSelected_BP, USkillBase*, Skill);

/** 스킬 큐가 비워질 때 (실행 또는 취소) UI에 알리기 위한 델리게이트 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSkillQueueCleared_BP);

/** 플레이어 턴 시작 시 UI 갱신을 위한 델리게이트 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTurnStart_BP);

/** GAS 입력 바인딩용 Enum */
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


public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turn System")
	float InputCooldown = 0.3f; // 0.3초 쿨타임

	/** 스킬이 큐에 추가될 때 BP에서 바인딩할 이벤트 */
	UPROPERTY(BlueprintAssignable, Category = "UI|Event")
	FOnSkillSelected_BP OnSkillSelected_BPEvent;

	/** 큐가 실행/취소되어 비워질 때 BP에서 바인딩할 이벤트 */
	UPROPERTY(BlueprintAssignable, Category = "UI|Event")
	FOnSkillQueueCleared_BP OnSkillQueueCleared_BPEvent;

	/** (신규) 턴 시작 시 BP에서 바인딩할 쿨타임 갱신용 이벤트 */
	UPROPERTY(BlueprintAssignable, Category = "UI|Event")
	FOnTurnStart_BP OnTurnStart_BPEvent;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnCooldownFinished OnCooldownFinished_BPEvent;

	// 모든 쿨타임 감소
	UFUNCTION(BlueprintCallable)
	void ReduceCooldowns();


private:
	bool bInputLocked = false;  // 입력 잠금 여부
	FTimerHandle InputLockTimerHandle;


protected:
	// ❌ (제거) BeginPlay() (부모 클래스(CharacterBase)가 BattleManager를 찾음)

	/** (수정됨) WASD 입력을 Enhanced Input으로 연결합니다. */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/** (수정됨) 컨트롤러에 빙의될 때 ASC 초기화 및 Enhanced Input 컨텍스트를 추가합니다. */
	virtual void PossessedBy(AController* NewController) override;

	// ──────────────────────────────
	// (신규) ENHANCED INPUT (캐릭터용)
	// ──────────────────────────────

	/** 캐릭터 이동(WASD)을 위한 매핑 컨텍스트 (에디터에서 할당) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> PlayerMoveContext;

	/** W키 (앞으로 이동) (에디터에서 할당) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_MoveUp;

	/** S키 (뒤로 이동) (에디터에서 할당) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_MoveDown;

	/** A키 (왼쪽으로 이동) (에디터에서 할당) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_MoveLeft;

	/** D키 (오른쪽으로 이동) (에디터에서 할당) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_MoveRight;

	/** (신규) Enter키 (스킬 큐 실행) (에디터에서 할당) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_ExecuteSkills;

	/** (신규) ESC키 (스킬 큐 취소) (에디터에서 할당) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_CancelSkills;



	// ───────────── 스킬 관련 ─────────────
	
	/** 스킬 큐 실행 (Enter) */
	void Input_ExecuteSkills();

	/** 스킬 큐 취소 (ESC) */
	void Input_CancelSkills();

	// 플레이어가 가진 전체 스킬
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
	TArray<FPlayerSkillData> OwnedSkills;

	// 현재 턴에 사용할 스킬 대기열
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skill")
	TArray<USkillBase*> SkillQueue;

	// 스킬 선택 (UI에서 호출)
	UFUNCTION(BlueprintCallable)
	void SelectSkill(USkillBase* Skill);

	// 스킬 대기열 비우기 (턴 종료 후)
	UFUNCTION(BlueprintCallable)
	void ClearSkillQueue();

	

	// 특정 스킬을 쿨타임 상태로 전환
	void ApplySkillCooldown(USkillBase* UsedSkill);

	// 대기열 비워졌는지 확인
	bool HasQueuedSkill() const { return SkillQueue.Num() > 0; }

private:
	// 입력 래퍼 함수 (기존과 동일)
	void Input_MoveUp();
	void Input_MoveDown();
	void Input_MoveLeft();
	void Input_MoveRight();
	void LockInputTemporarily();
	void UnlockInput();

	/** (신규) 0.2초 딜레이 실행을 위한 타이머 핸들 */
	FTimerHandle SkillQueueTimerHandle;

	/** (신규) 스킬 간 실행 간격 (초) */
	UPROPERTY(EditDefaultsOnly, Category = "Skill")
	float SkillExecutionDelay = 0.2f;

	/** (신규) 큐에서 스킬을 하나씩 꺼내 실행하는 함수 (UI 큐 사용) */
	void ExecuteNextSkillInQueue_UI();

};