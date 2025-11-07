// GridDataInterface.h

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GridDataInterface.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UGridDataInterface : public UInterface
{
	GENERATED_BODY()
};

class PORTFOLIO2GAME_API IGridDataInterface
{
	GENERATED_BODY()

public:

	// 1. 그리드 데이터 요청 (BP 변수 반환)
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Grid Interface")
	int32 GetGridWidth() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Grid Interface")
	int32 GetGridHeight() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Grid Interface")
	double GetGridSizeX() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Grid Interface")
	double GetGridSizeY() const;

	/** (신규 추가) 그리드 액터의 위치 오프셋을 반환합니다. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Grid Interface")
	FVector GetGridLocationOffset() const;

	/** (신규 추가, BP_TestPlayerController용) ISM 컴포넌트 자체를 반환합니다. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Grid Interface")
	UInstancedStaticMeshComponent* GetGridInstanceComponent() const;

	// 2. 그리드 계산 요청 (BP 로직으로 계산하여 반환)
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Grid Interface")
	int32 GetGridIndexFromCoord(const FIntPoint& Coord) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Grid Interface")
	FIntPoint GetGridCoordFromIndex(int32 Index) const;

	// 3. (신규 추가) 마우스 하이라이트 이벤트 (BP_TestPlayerController가 호출)

	/** 마우스가 특정 타일 위에 올라갔음을 BP_GridISM에게 알립니다. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Grid Interface")
	void MouseOverGrid(int32 InstanceIndex);

	/** 마우스가 그리드 밖으로 나갔음을 BP_GridISM에게 알립니다. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Grid Interface")
	void NotOverGrid();
};