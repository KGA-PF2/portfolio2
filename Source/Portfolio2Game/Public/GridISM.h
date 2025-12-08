#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GridDataInterface.h"
#include "MouseOverGridInterface.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GridISM.generated.h"

class ABattleManager;
class ACharacterBase;

UCLASS()
class PORTFOLIO2GAME_API AGridISM : public AActor, public IGridDataInterface, public IMouseOverGridInterface
{
	GENERATED_BODY()

public:
	AGridISM();

protected:
	virtual void BeginPlay() override;

	// [신규] 카메라 지지대 (거리, 각도 조절용)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	// [신규] 실제 카메라
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> TopDownCamera;

	virtual void OnConstruction(const FTransform& Transform) override;

public:
	// ───────── 그리드 설정 ─────────
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid") int32  GridWidth = 7;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid") int32  GridHeight = 5;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid") double GridSizeX = 100.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid") double GridSizeY = 100.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid") FVector GridLocationOffset;

	// ───────── HP바 설정 ─────────
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid|UI")
	TSubclassOf<AActor> HPBarActorClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid|UI")
	TArray<TObjectPtr<AActor>> HPBarPool;

	// [기존 유지] HP바 높이 오프셋
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid|UI")
	float HPBarZOffset = 0.3f;

	// [기존 유지] HP바 위치 비율 (기본값 -0.45: 하단)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid|UI")
	float HPBarYOffsetRatio = -0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid|UI")
	float HPBarWidthRatio = 0.85f;

	// ★ [신규] HP바 세로 높이 (기본 30.0)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid|UI")
	float HPBarHeight = 30.0f;

	// [신규] 위의 기본 계산 결과에 "추가로" 더할 XYZ 값 (미세 조정용)
	// 기본값 (0,0,0)이면 기존 위치와 완전히 동일합니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid|UI")
	FVector HPBarAdditionalOffset = FVector::ZeroVector;

	// ───────── 기능 ─────────
	UFUNCTION(BlueprintCallable, Category = "Grid|UI")
	void UpdateTileHPBar(int32 Index, bool bShow, int32 CurrentHP = 0, int32 MaxHP = 0);

	virtual void VisibleGrid_Implementation(FVector GridLocation, FVector GridSize) override;

	virtual void HiddenGrid_Implementation() override;

	// 인터페이스 구현
	virtual int32   GetGridWidth_Implementation() const override { return GridWidth; }
	virtual int32   GetGridHeight_Implementation() const override { return GridHeight; }
	virtual double  GetGridSizeX_Implementation() const override { return GridSizeX; }
	virtual double  GetGridSizeY_Implementation() const override { return GridSizeY; }
	virtual FVector GetGridLocationOffset_Implementation() const override { return GridLocationOffset; }
	virtual FIntPoint GetGridCoordFromIndex_Implementation(int32 Index) override;
	virtual int32     GetGridIndexFromCoord_Implementation(FIntPoint Coord) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid|Cursor")
	TObjectPtr<AActor> GridCursorActor;

private:
	// [신규] 직전에 하이라이트 했던 캐릭터 기억용
	UPROPERTY()
	TObjectPtr<ACharacterBase> LastHoveredCharacter;

	// [신규] 배틀 매니저 캐싱 (매번 찾지 않기 위해)
	UPROPERTY()
	TObjectPtr<ABattleManager> CachedBattleManager;

};