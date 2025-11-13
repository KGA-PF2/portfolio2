// MouseOverGridInterface.h

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "MouseOverGridInterface.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UMouseOverGridInterface : public UInterface
{
	GENERATED_BODY()
};

class PORTFOLIO2GAME_API IMouseOverGridInterface
{
	GENERATED_BODY()

public:
	/**
	 * 마우스가 그리드 위에 있을 때 하이라이트를 표시하도록 BP에서 구현할 함수
	 * @param GridLocation 하이라이트 메시가 표시될 월드 위치
	 * @param GridSize 하이라이트 메시의 크기 (X, Y)
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mouse Over Grid")
	void VisibleGrid(FVector GridLocation, FVector GridSize);

	/**
	 * 마우스가 그리드에서 벗어났을 때 하이라이트를 숨기도록 BP에서 구현할 함수
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mouse Over Grid")
	void HiddenGrid();
};