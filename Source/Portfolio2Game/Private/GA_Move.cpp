#include "GA_Move.h"
#include "PlayerCharacter.h"    // APlayerCharacter의 EndAction(), bCanAct를 사용
#include "BattleManager.h"      // BattleManager의 좌표 계산 함수 사용
#include "GridDataInterface.h"  // BattleManager의 GridActorRef에서 그리드 크기를 가져오기 위함

UGA_Move::UGA_Move()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// (신규) 어빌리티 태그 설정
	FGameplayTag MoveTag = FGameplayTag::RequestGameplayTag(TEXT("Ability.Move"));
	AbilityTags.AddTag(MoveTag);
}

void UGA_Move::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 1. 컴포넌트 가져오기
	ACharacterBase* Character = Cast<ACharacterBase>(ActorInfo->AvatarActor.Get());
	if (!Character || !Character->bCanAct)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ABattleManager* BattleManagerRef = Character->BattleManagerRef;
	TScriptInterface<IGridDataInterface> GridInterface = BattleManagerRef ? BattleManagerRef->GridInterface : nullptr;

	if (!BattleManagerRef || !GridInterface)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true); // 참조 없음
		return;
	}

	// 2. (신규) 태그를 기반으로 이 GA 인스턴스가 어떤 방향인지 결정
	// (CharacterBase::GiveMoveAbilities에서 설정한 태그를 읽음)
	if (AbilityTags.HasTag(FGameplayTag::RequestGameplayTag(TEXT("Ability.Move.Up"))))
	{
		MoveDirection = EGridDirection::Up;
	}
	else if (AbilityTags.HasTag(FGameplayTag::RequestGameplayTag(TEXT("Ability.Move.Down"))))
	{
		MoveDirection = EGridDirection::Down;
	}
	else if (AbilityTags.HasTag(FGameplayTag::RequestGameplayTag(TEXT("Ability.Move.Left"))))
	{
		MoveDirection = EGridDirection::Left;
	}
	else if (AbilityTags.HasTag(FGameplayTag::RequestGameplayTag(TEXT("Ability.Move.Right"))))
	{
		MoveDirection = EGridDirection::Right;
	}
	else
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true); // 방향 태그 없음
		return;
	}

	// 3. 목표 인덱스(칸 번호) 계산
	const int32 CurrentIndex = Character->GridIndex;
	int32 TargetIndex = CurrentIndex;

	// (신규) 그리드 크기(세로 길이)를 인터페이스에서 가져옴
	const int32 GridHeight = IGridDataInterface::Execute_GetGridHeight(GridInterface.GetObject());
	if (GridHeight <= 0)
	{
		UE_LOG(LogTemp, Error, TEXT("GA_Move: GridHeight가 0 또는 음수입니다. BP_GridISM 확인 필요."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// (신규) 세로 우선(Column-Major) 인덱스 계산
	switch (MoveDirection)
	{
	case EGridDirection::Up:	// Y-
		TargetIndex = CurrentIndex - 1;
		break;
	case EGridDirection::Down:	// Y+
		TargetIndex = CurrentIndex + 1;
		break;
	case EGridDirection::Left:	// X-
		TargetIndex = CurrentIndex - GridHeight; // 한 열(Column) 뒤로
		break;
	case EGridDirection::Right: // X+
		TargetIndex = CurrentIndex + GridHeight; // 한 열(Column) 앞으로
		break;
	}

	// 4. (신규) 경계 검사 (C++)
	const int32 GridWidth = IGridDataInterface::Execute_GetGridWidth(GridInterface.GetObject());
	const int32 MaxIndex = (GridWidth * GridHeight) - 1;

	// 4a. 상/하 경계 (Y축, % 연산)
	if (MoveDirection == EGridDirection::Up && (CurrentIndex % GridHeight == 0))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true); // 맨 위 칸(Y=0)
		return;
	}
	if (MoveDirection == EGridDirection::Down && (CurrentIndex % GridHeight == GridHeight - 1))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true); // 맨 아래 칸(Y=4)
		return;
	}
	// 4b. 좌/우 경계 (X축, 전체 인덱스)
	if (TargetIndex < 0 || TargetIndex > MaxIndex)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true); // 맵 밖 (0 미만 또는 34 초과)
		return;
	}

	// 5. 점유/좌표 변환
	// (신규) 인덱스로부터 좌표를 다시 계산
	FIntPoint TargetCoord = IGridDataInterface::Execute_GetGridCoordFromIndex(GridInterface.GetObject(), TargetIndex);

	if (BattleManagerRef->GetCharacterAt(TargetCoord) != nullptr)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true); // 점유됨
		return;
	}

	// 6. 목표 월드 위치 계산
	FVector TargetLocation = BattleManagerRef->GetWorldLocation(TargetCoord);
	TargetLocation.Z += Character->SpawnZOffset;


	// 7. (신규) 순간이동 실행
	Character->SetActorLocation(TargetLocation);

	// 8. (신규) 캐릭터의 논리적 위치(좌표와 인덱스) 업데이트
	Character->MoveToCell(TargetCoord, TargetIndex);

	// 9. 턴 종료 및 어빌리티 종료
	Character->EndAction();
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false); // 성공
}