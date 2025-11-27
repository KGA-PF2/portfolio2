#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnforceCardData.h"
#include "SkillBase.h"
#include "EnforceManager.generated.h"

// 보상 종류 구분
UENUM(BlueprintType)
enum class ERewardType : uint8
{
	EnforceItem, // 강화
	NewSkill     // 신규 스킬
};

class APlayerCharacter;

// UI에 전달할 통합 보상 정보
USTRUCT(BlueprintType)
struct FEnforceRewardInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	ERewardType Type = ERewardType::EnforceItem;

	UPROPERTY(BlueprintReadOnly)
	UEnforceCardData* EnforceData = nullptr;

	UPROPERTY(BlueprintReadOnly)
	USkillBase* NewSkillData = nullptr;
};

UCLASS()
class PORTFOLIO2GAME_API AEnforceManager : public AActor
{
	GENERATED_BODY()

public:
	AEnforceManager();

protected:
	virtual void BeginPlay() override;

public:
	// ───────── 필수 설정 (에디터 할당) ─────────

	// [신규] 플레이어 클래스 (데이터 로드용 본체)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup")
	TSubclassOf<APlayerCharacter> PlayerClass;

	// [신규] 강화 맵 전용 HUD (핸드 + 카드선택지)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup")
	TSubclassOf<UUserWidget> EnforceHUDClass;

	// [신규] 플레이어 스폰 위치 (필요시 지정, 기본 0,0,100)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup")
	FTransform PlayerSpawnTransform = FTransform(FRotator::ZeroRotator, FVector(0, 0, 100.0f));

	// ───────── 데이터 풀 (에디터 설정) ─────────
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pool")
	TArray<UEnforceCardData*> AllEnforceCards;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pool")
	TArray<USkillBase*> AllSkillCards; // 전체 스킬 목록

	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<UUserWidget> TransitionWidgetClass;

	// ───────── 참조 ─────────
	UPROPERTY(BlueprintReadOnly, Category = "Ref")
	APlayerCharacter* PlayerRef;

	UPROPERTY(BlueprintReadOnly, Category = "Ref")
	UUserWidget* HUDRef;

	// ───────── 기능 ─────────
	// 랜덤 보상 3개 뽑기 (30% 스킬, 70% 강화)
	UFUNCTION(BlueprintCallable)
	TArray<FEnforceRewardInfo> GetRandomRewards();

	// [기능 1] 강화 적용 (기존 카드 위에 드롭 시)
	UFUNCTION(BlueprintCallable)
	void ApplyEnforce(int32 SkillIndex, UEnforceCardData* CardData);

	// [기능 2] 신규 스킬 습득 (빈 공간에 드롭 시)
	UFUNCTION(BlueprintCallable)
	void AcquireNewSkill(USkillBase* NewSkill);

	// [신규] 이 맵에서 사용할 고정 카메라 (레벨에 배치하고 할당하세요)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup")
	TObjectPtr<class ACameraActor> StageCamera;

private:
	void SpawnPlayerAndInit(); // 플레이어 소환 및 초기화
	void CompleteStage(); // 저장 및 이동 시작
	void MoveToNextLevel(); // 실제 이동
};