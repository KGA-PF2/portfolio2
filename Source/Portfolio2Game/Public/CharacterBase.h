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

// HP 변경을 BP_HPBarActor에게 알릴 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHealthChangedSignature, int32, NewHealth, int32, NewMaxHealth);

// [신규] 상태 변경 알림 델리게이트 (true: 바쁨/잠금, false: 한가함/해제)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCharacterBusyChanged, bool, bIsBusy);

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

	// BP에서 설정 가능한 Z오프셋 (기본값 100)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
	float SpawnZOffset = 100.f;

protected:
	virtual void BeginPlay() override;

	// ───────── GAS ─────────
public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystem;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UBaseAttributeSet> Attributes;

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override { return AbilitySystem; }

	// HP바 높이 조절용 변수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	float HPBarZOffset = -150.0f;

	// HP바 세로 길이 (두께) 조절용 변수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	float HPBarHeight = 15.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "UI")
	TSubclassOf<AActor> HPBarActorClass;

	UPROPERTY(BlueprintReadWrite, Category = "UI")
	TObjectPtr<AActor> HPBarActor;

	// HP 변경 델리게이트 인스턴스
	UPROPERTY(BlueprintAssignable, Category = "Attributes")
	FOnHealthChangedSignature OnHealthChanged;

	//CachedGridIndex 변수 추가(이동 전 위치 기억용).
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid")
	int32 CachedGridIndex = -1;

	UFUNCTION()
	void OnHealthChanged_Wrapper(int32 CurrentHP, int32 MaxHP);

	// GAS의 체력 변경 감지용 함수 선언
	virtual void OnHealthAttributeChanged(const FOnAttributeChangeData& Data);

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
	virtual void StartAction();

	UFUNCTION(BlueprintCallable, Category = "Turn")
	virtual void EndAction();

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

	// 공용 공격 어빌리티 BP 클래스
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill")
	TSubclassOf<UGameplayAbility> GenericAttackAbilityClass;

	// ───────── 이동/회전 (수정됨) ─────────
public:
	/** (수정됨) 캐릭터의 논리적 위치(좌표와 인덱스)를 업데이트합니다. */
	UFUNCTION(BlueprintCallable, Category = "Move")
	void MoveToCell(FIntPoint TargetCoord, int32 TargetIndex);

	// 이동 속도 (BP에서 조절 가능, 기본값 600)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move|Visual")
	float MoveSpeed = 600.0f;

	// 이동 시 재생할 몽타주 (뛰기/걷기 모션)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Move|Visual")
	TObjectPtr<UAnimMontage> RunMontage;

	// 현재 이동에 사용할 '정지' 몽타주 (이동 시작 시 EnemyCharacter가 채워줌)
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Move|Visual")
	TObjectPtr<UAnimMontage> CurrentStopMontage;

	UFUNCTION(BlueprintImplementableEvent, Category = "Move")
	void PlayMoveAnim();

	// 매 프레임 이동 처리를 위해 Tick 오버라이드
	virtual void Tick(float DeltaTime) override;

	// [신규] UI나 컨트롤러가 바인딩할 이벤트
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnCharacterBusyChanged OnBusyStateChanged;

protected:
	// 이동 시작 지점 (보간용)
	FVector VisualMoveStartLocation;

	// 이동 경과 시간
	float VisualMoveTimeElapsed = 0.0f;

	// 이동 총 소요 시간
	float VisualMoveDuration = 0.3f;

	// 이동 중인지 체크하는 플래그
	bool bIsVisualMoving = false;

	// 이동할 목표 월드 좌표
	FVector VisualMoveDestination;

	// 이동 시작 함수
	void StartVisualMove(const FVector& TargetLocation);

	// 정지 애니메이션 대기용 타이머
	FTimerHandle StopAnimTimerHandle;

	// 정지 애니메이션이 끝나면 호출될 함수
	void OnStopAnimEnded();

	// ───────── 상태 (수정됨) ─────────
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
	FIntPoint GridCoord = FIntPoint::ZeroValue;

	/** (신규) 1D 그리드 인덱스 (칸 번호, 세로 우선) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid", meta = (ExposeOnSpawn = "true"))
	int32 GridIndex = 0;


	// 회전 관련
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Anim|Rotation")
	TObjectPtr<UAnimMontage> Montage_RotateCCW; // Q (반시계)

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Anim|Rotation")
	TObjectPtr<UAnimMontage> Montage_RotateCW;  // E (시계)

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Anim|Rotation")
	TObjectPtr<UAnimMontage> Montage_Rotate180; // R (뒤로)

	/*UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
	bool bFacingRight = true;*/

	// 회전 대기용 변수
	EGridDirection PendingRotationDirection;

	// [신규] 회전 요청 함수 (애니메이션 체크 로직 포함)
	void RequestRotation(EGridDirection NewDir, UAnimMontage* MontageToPlay);

	// [신규] 실제 회전 적용 및 턴 종료 (애니메이션 종료 후 호출)
	UFUNCTION()
	void FinalizeRotation(UAnimMontage* Montage, bool bInterrupted);

	// [신규] 현재 바라보고 있는 방향 (기본값: Right)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
	EGridDirection FacingDirection = EGridDirection::Right;

	// [신규] 해당 방향으로 즉시 회전 (턴 소모 포함)
	UFUNCTION(BlueprintCallable, Category = "Move")
	void RotateToDirection(EGridDirection NewDir, bool bConsumeTurn = true);

	// [신규] 유틸리티: 방향 Enum -> 월드 회전값(Rotator) 변환
	FRotator GetRotationFromEnum(EGridDirection Dir) const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status")
	bool bDead = false;

	/** (신규) GridIndex 변수를 반환합니다. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Grid")
	int32 GetGridIndex() const;

	// ───────── 스탯/데미지 ─────────
public:
	/** EnemyCharacter가 호출할 수 있도록 ApplyDamage를 다시 추가합니다. */
	UFUNCTION(BlueprintCallable, Category = "Attributes")
	void ApplyDamage(float Damage);

	// ❌  FOnAttributeChangeData 델리게이트 함수 선언을 제거합니다.
	// UFUNCTION()
	// void OnHealthChanged(const FOnAttributeChangeData& Data);

	/** BP에서 사망 처리를 위한 이벤트 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Status")
	void OnDeath();
};