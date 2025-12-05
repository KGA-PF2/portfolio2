#include "GridISM.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "BattleManager.h"
#include "CharacterBase.h"
#include "Components/WidgetComponent.h"

AGridISM::AGridISM()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AGridISM::BeginPlay()
{
	Super::BeginPlay();

	// 1. HP바 미리 생성 (Object Pooling)
	if (HPBarActorClass && GridWidth > 0 && GridHeight > 0)
	{
		int32 TotalTiles = GridWidth * GridHeight;
		HPBarPool.Reserve(TotalTiles);

		// 회전값 (상황에 맞춰 조절)
		FRotator BaseRotation = FRotator(0.0f, 90.0f, 0.0f);

		for (int32 i = 0; i < TotalTiles; ++i)
		{
			// 좌표 계산 (로컬)
			FIntPoint Coord = GetGridCoordFromIndex_Implementation(i);

			// 1. 격자 중앙 좌표 계산
			double CenterX = (Coord.X * GridSizeX) + (GridSizeX * 0.5);
			double CenterY = (Coord.Y * GridSizeY) + (GridSizeY * 0.5);

			// 2. [기존 로직] 위치 보정 (하단 배치)
			// 기존에 쓰시던 변수 그대로 사용하여 위치 계산
			double BaseFinalX = CenterX + (GridSizeX * HPBarYOffsetRatio);
			double BaseFinalY = CenterY;

			FVector BaseRelativeLoc(BaseFinalX, BaseFinalY, HPBarZOffset);

			// 3. [신규] 미세 조정값 추가 (XYZ)
			// BP에서 이 값을 바꾸면, 기존 위치에서 그만큼 더 이동합니다.
			FVector FinalRelativeLoc = BaseRelativeLoc + HPBarAdditionalOffset;


			// 스폰 및 설정
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = this;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			AActor* NewBar = GetWorld()->SpawnActor<AActor>(HPBarActorClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);

			if (NewBar)
			{
				NewBar->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);

				// 최종 위치 적용
				NewBar->SetActorRelativeLocation(FinalRelativeLoc);
				NewBar->SetActorRelativeRotation(BaseRotation);

				// 크기 설정 (격자 가로 너비의 95%)
				UWidgetComponent* WidgetComp = NewBar->FindComponentByClass<UWidgetComponent>();
				if (WidgetComp)
				{
					WidgetComp->SetDrawSize(FVector2D(GridSizeX * HPBarWidthRatio, HPBarHeight));
				}

				// 일단 숨김
				NewBar->SetActorHiddenInGame(true);
				HPBarPool.Add(NewBar);
			}
		}
	}

	CachedBattleManager = Cast<ABattleManager>(
		UGameplayStatics::GetActorOfClass(GetWorld(), ABattleManager::StaticClass()));
}

FIntPoint AGridISM::GetGridCoordFromIndex_Implementation(int32 Index)
{
	if (GridHeight <= 0)
	{
		UE_LOG(LogTemp, Error, TEXT("GridISM: GridHeight <= 0"));
		return FIntPoint(-1, -1);
	}
	const int32 X = Index / GridHeight;  // Column-Major
	const int32 Y = Index % GridHeight;
	return FIntPoint(X, Y);
}

int32 AGridISM::GetGridIndexFromCoord_Implementation(FIntPoint Coord)
{
	if (GridHeight <= 0)
	{
		UE_LOG(LogTemp, Error, TEXT("GridISM: GridHeight <= 0"));
		return -1;
	}
	return (Coord.X * GridHeight) + Coord.Y; // Column-Major
}

void AGridISM::UpdateTileHPBar(int32 Index, bool bShow, int32 CurrentHP, int32 MaxHP)
{
	if (!HPBarPool.IsValidIndex(Index)) return;

	AActor* TargetBar = HPBarPool[Index];
	if (!TargetBar) return;

	if (bShow)
	{
		TargetBar->SetActorHiddenInGame(false);

		// BP_GridHPActor의 HandleHealthChanged 호출
		FString Cmd = FString::Printf(TEXT("HandleHealthChanged %d %d"), CurrentHP, MaxHP);
		TargetBar->CallFunctionByNameWithArguments(*Cmd, *GLog, nullptr, true);
	}
	else
	{
		TargetBar->SetActorHiddenInGame(true);
	}
}

// 마우스가 그리드 칸 위에 있을 때 (매 프레임 호출될 수 있음)
void AGridISM::VisibleGrid_Implementation(FVector GridLocation, FVector GridSize)
{
	// 1. 기존 커서 로직 (유지)
	if (GridCursorActor)
	{
		GridCursorActor->SetActorHiddenInGame(false);
		FVector NewLoc = GridLocation; NewLoc.Z += 5.0f;
		GridCursorActor->SetActorLocation(NewLoc);
	}

	// 2. [신규] 해당 위치의 캐릭터 찾기
	if (CachedBattleManager)
	{
		FVector LocalPos = GetActorTransform().InverseTransformPosition(GridLocation);
		LocalPos -= GridLocationOffset;

		// 그리드 인덱스 계산
		int32 X = FMath::RoundToInt(LocalPos.X / GridSizeX);
		int32 Y = FMath::RoundToInt(LocalPos.Y / GridSizeY);

		// [디버그 2] 계산된 좌표 확인
		UE_LOG(LogTemp, Warning, TEXT("Hover Coord: (%d, %d)"), X, Y);

		if (X >= 0 && X < GridWidth && Y >= 0 && Y < GridHeight)
		{
			ACharacterBase* FoundChar = CachedBattleManager->GetCharacterAt(FIntPoint(X, Y));

			if (FoundChar != LastHoveredCharacter)
			{
				if (IsValid(LastHoveredCharacter)) LastHoveredCharacter->SetHighlight(false);
				if (FoundChar) FoundChar->SetHighlight(true);
				LastHoveredCharacter = FoundChar;
			}
		}
		else
		{
			// 맵 밖을 찍었으면 하이라이트 끄기
			if (IsValid(LastHoveredCharacter))
			{
				LastHoveredCharacter->SetHighlight(false);
				LastHoveredCharacter = nullptr;
			}
		}
	}
}

void AGridISM::HiddenGrid_Implementation()
{
	// 1. 커서 숨김 (기존)
	if (GridCursorActor) GridCursorActor->SetActorHiddenInGame(true);

	// 2. [신규] 하이라이트 끄기
	if (LastHoveredCharacter)
	{
		LastHoveredCharacter->SetHighlight(false);
		LastHoveredCharacter = nullptr;
	}
}