#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GridDataInterface.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UGridDataInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * C++ BattleManager가 BP_GroundGrid의 데이터와 계산을 요청하기 위한 C++ 인터페이스입니다.
 * BP_GroundGrid는 이 인터페이스를 "구현(implement)"해야 합니다.
 *
 * (수정: YOURPROJECT_API -> PORTFOLIO2GAME_API)
 */
class PORTFOLIO2GAME_API IGridDataInterface
{
	GENERATED_BODY()

public:

	// ──────────────────────────────
	// 1. 그리드 데이터 요청 (BP 변수 반환)
	// ──────────────────────────────

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Grid Interface")
	int32 GetGridWidth() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Grid Interface")
	int32 GetGridHeight() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Grid Interface")
	double GetGridSizeX() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Grid Interface")
	double GetGridSizeY() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Grid Interface")
	double GetPlayerCellOffsetX() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Grid Interface")
	double GetEnemyCellOffsetX() const;

	// ──────────────────────────────
	// 2. 그리드 계산 요청 (BP 로직으로 계산하여 반환)
	// ──────────────────────────────

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Grid Interface")
	int32 GetGridIndexFromCoord(const FIntPoint& Coord) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Grid Interface")
	FIntPoint GetGridCoordFromIndex(int32 Index) const;
};